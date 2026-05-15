"""benchmark_remote.py -- lightweight remote dispatch for the benchmark harness.

Per benchmark/CLAUDE.md "Where benchmarks run -- dynamic combination set":

  1. Resolve the exec node's parent compile node from remote_nodes.json.
  2. rsync the source tree to that compile node.
  3. cmake -S ... && cmake --build on the compile node (matching toolchain).
  4. rsync the resulting binary + workload data to the exec node.
  5. Run the benchmark on the exec node.

This module is the minimal-but-functional implementation of that flow.
We DO NOT reuse test/remote_build.py because that file is built for a
different lifecycle (per-test source rsync, ccache slots, qmake-era
shims). Benchmarks reuse the same SSH/rsync primitives but with their
own bookkeeping: one source rsync per benchmark batch, one build per
(compile_node, benchmark), then any number of profiler invocations on
the paired exec nodes.

Returns are kept simple: each function returns (rc, message) so the
caller can decide whether to skip / fail-fast / proceed. No timeouts
are silently swallowed -- a connect timeout to one node never blocks
the rest of the batch.
"""
import json
import os
import shlex
import subprocess
import sys
import time

import benchmark_helpers as bh


REPO_ROOT = bh.REPO_ROOT
REMOTE_NODES_JSON = bh.REMOTE_NODES_JSON

# Tunable knobs. Generous defaults: a cold mips-gentoo cmake configure
# can easily walk 10+ minutes when the find_package(Threads) try_compile
# chain rebuilds on a slow filesystem.
RSYNC_TIMEOUT          = 1800
CMAKE_CONFIGURE_TIMEOUT = 900
CMAKE_BUILD_TIMEOUT     = 3600
RUN_TIMEOUT_DEFAULT     = 1200


# ---- ssh / rsync primitives ---------------------------------------------

def _ssh_host(user, host):
    """Return `user@host`, wrapping IPv6 only in brackets for rsync."""
    return f"{user}@{host}"


def _rsync_host(user, host):
    if ":" in host and not host.startswith("["):
        host = f"[{host}]"
    return f"{user}@{host}"


def ssh_run(user, host, port, cmd, timeout=120):
    """Run `cmd` on the remote via SSH. Returns (rc, stdout, stderr).

    `cmd` is a single shell-string -- caller is responsible for quoting
    pieces that contain spaces / special chars (use shlex.quote()).
    """
    argv = ["ssh", "-o", "ConnectTimeout=10", "-o", "BatchMode=yes",
            "-o", "ServerAliveInterval=30",
            "-p", str(port), _ssh_host(user, host), cmd]
    try:
        p = subprocess.run(argv, capture_output=True, text=True,
                           timeout=timeout)
        return p.returncode, p.stdout, p.stderr
    except subprocess.TimeoutExpired:
        return -1, "", f"ssh timeout after {timeout}s"


def rsync_to(user, host, port, src, dst, timeout=RSYNC_TIMEOUT,
             extra_args=None):
    """rsync local `src` -> remote `dst` (path on host). When `src` ends
    with '/', the contents are copied; otherwise the dir is created at
    dst. Returns (rc, stderr_message)."""
    args = ["rsync", "-art", "--delete",
            "--exclude=.git", "--exclude=build/", "--exclude=*.o",
            "--exclude=moc_*", "--exclude=Makefile",
            "--exclude=__pycache__/", "--exclude=.qtcreator/",
            "-e", f"ssh -p {port} -o ConnectTimeout=10 -o BatchMode=yes"]
    if extra_args:
        args.extend(extra_args)
    args += [src, f"{_rsync_host(user, host)}:{dst}"]
    try:
        p = subprocess.run(args, capture_output=True, text=True,
                           timeout=timeout)
        return p.returncode, (p.stdout + p.stderr)
    except subprocess.TimeoutExpired:
        return -1, f"rsync timeout after {timeout}s"


def rsync_from(user, host, port, src, dst, timeout=RSYNC_TIMEOUT):
    """rsync remote `src` -> local `dst`."""
    args = ["rsync", "-art",
            "-e", f"ssh -p {port} -o ConnectTimeout=10 -o BatchMode=yes",
            f"{_rsync_host(user, host)}:{src}", dst]
    try:
        p = subprocess.run(args, capture_output=True, text=True,
                           timeout=timeout)
        return p.returncode, (p.stdout + p.stderr)
    except subprocess.TimeoutExpired:
        return -1, f"rsync timeout after {timeout}s"


# ---- compile-node build --------------------------------------------------

def compile_node_for_exec(cfg, exec_label):
    """Return (compile_node_dict, exec_node_dict) for an exec_label, by
    scanning the parent nodes[] entries. None pair on no match."""
    for node in cfg.get("nodes", []):
        for ex in node.get("execution_nodes", []):
            if ex.get("label") == exec_label:
                return node, ex
    return None, None


def stage_source_on_compile_node(compile_node, repo_root=REPO_ROOT,
                                  verbose=False):
    """rsync the local repo to <compile_node.work_dir>/sources/.
    Returns (rc, message)."""
    user = compile_node["ssh"]["user"]
    host = compile_node["ssh"]["host"]
    port = compile_node["ssh"].get("port", 22)
    work = compile_node["work_dir"]
    # ensure parent dir exists; nothing else cleaned -- a long-lived
    # sources/ tree means incremental rsync only carries diffs.
    ssh_run(user, host, port, f"mkdir -p {shlex.quote(work)}/sources",
            timeout=30)
    rc, msg = rsync_to(user, host, port, repo_root + "/",
                       f"{work}/sources/")
    if verbose:
        print(f"[remote] rsync {compile_node['label']!r}: rc={rc}")
        if rc != 0:
            print(msg, file=sys.stderr)
    return rc, msg


def build_on_compile_node(compile_node, cmake_src_subdir, build_subdir,
                          cmake_defs=None, target=None, verbose=False):
    """cmake -S <work>/sources/<src_subdir> -B <work>/build-bench/<build_subdir>
    then cmake --build. Returns (rc, message, remote_bin_dir).
    """
    user = compile_node["ssh"]["user"]
    host = compile_node["ssh"]["host"]
    port = compile_node["ssh"].get("port", 22)
    work = compile_node["work_dir"]
    src  = f"{work}/sources/{cmake_src_subdir}"
    bld  = f"{work}/build-bench/{build_subdir}"

    defs = []
    if cmake_defs:
        for k, v in cmake_defs.items():
            defs.append(f"-D{k}={v}")
    ninja = "-G Ninja" if _have_remote_tool(user, host, port, "ninja") else ""

    cfg_cmd = (f"mkdir -p {shlex.quote(bld)} && "
               f"cmake -S {shlex.quote(src)} -B {shlex.quote(bld)} "
               f"{ninja} -DCMAKE_BUILD_TYPE=Release {' '.join(defs)}")
    rc, sout, serr = ssh_run(user, host, port, cfg_cmd,
                             timeout=CMAKE_CONFIGURE_TIMEOUT)
    if verbose:
        print(f"[remote] cmake configure {compile_node['label']!r}: rc={rc}")
    if rc != 0:
        return rc, f"cmake configure failed:\n{sout}\n{serr}", bld

    build_cmd = f"cmake --build {shlex.quote(bld)} -j$(nproc)"
    if target:
        build_cmd += f" --target {shlex.quote(target)}"
    rc, sout, serr = ssh_run(user, host, port, build_cmd,
                             timeout=CMAKE_BUILD_TIMEOUT)
    if verbose:
        print(f"[remote] cmake build {compile_node['label']!r}: rc={rc}")
    if rc != 0:
        return rc, f"cmake build failed:\n{sout}\n{serr}", bld
    return 0, "ok", bld


def _have_remote_tool(user, host, port, tool):
    rc, sout, _ = ssh_run(user, host, port,
                          f"command -v {shlex.quote(tool)} >/dev/null && echo Y || echo N",
                          timeout=10)
    return rc == 0 and sout.strip() == "Y"


# ---- exec-node staging ---------------------------------------------------

def push_binary_to_exec(compile_node, exec_node, remote_build_dir, bin_name,
                        extras=None, verbose=False):
    """Pull binary from compile_node, push it to exec_node's work_dir.
    `extras` is an optional list of additional files (relative to
    remote_build_dir) to ship alongside the binary -- useful for
    datapack symlinks, fixtures, etc.

    Returns (rc, exec_bin_path, message).
    """
    # 1) pull binary to local /tmp staging
    staging = os.path.join("/tmp",
                           f"cc-bench-stage-{exec_node['label']}-{int(time.time())}")
    os.makedirs(staging, exist_ok=True)

    cu = compile_node["ssh"]["user"]
    ch = compile_node["ssh"]["host"]
    cp = compile_node["ssh"].get("port", 22)
    src = f"{remote_build_dir}/{bin_name}"
    rc, msg = rsync_from(cu, ch, cp, src, staging + "/")
    if rc != 0:
        return rc, None, f"pull from compile node failed: {msg}"

    if extras:
        for e in extras:
            rc, msg = rsync_from(cu, ch, cp, f"{remote_build_dir}/{e}",
                                  staging + "/")
            if rc != 0 and verbose:
                print(f"[remote] WARN: extra {e!r} pull rc={rc}: {msg}", file=sys.stderr)

    # 2) push to exec node work_dir
    eu = exec_node["user"]
    eh = exec_node["host"]
    ep = exec_node.get("port", 22)
    ewd = exec_node["work_dir"]
    ssh_run(eu, eh, ep,
            f"mkdir -p {shlex.quote(ewd)}",
            timeout=30)
    rc, msg = rsync_to(eu, eh, ep, staging + "/", f"{ewd}/")
    if rc != 0:
        return rc, None, f"push to exec node failed: {msg}"
    return 0, f"{ewd}/{bin_name}", "ok"


# ---- run profiler on exec node -------------------------------------------

def run_remote_cmd(exec_node, cmd_str, timeout=RUN_TIMEOUT_DEFAULT,
                   env=None):
    """Run an arbitrary command on the exec node and return
    (rc, stdout, stderr). `env` is a dict of VAR=VAL prefixes."""
    user = exec_node["user"]
    host = exec_node["host"]
    port = exec_node.get("port", 22)
    prefix = ""
    if env:
        for k, v in env.items():
            prefix += f"{k}={shlex.quote(str(v))} "
    return ssh_run(user, host, port, prefix + cmd_str, timeout=timeout)


def remote_time_v(exec_node, cmd_str, timeout=RUN_TIMEOUT_DEFAULT):
    """Run `/usr/bin/time -v <cmd>` on the exec node and parse rusage.

    Returns the same dict shape as bh.measure_time_v(). When /usr/bin/time
    is missing the harness should not call this -- profilers_runnable_on()
    is the gate."""
    out = {"wall_s": None, "user_s": None, "sys_s": None,
           "max_rss_kb": None, "vol_ctx": None, "invol_ctx": None,
           "minor_pf": None, "major_pf": None, "rc": None}
    full = f"/usr/bin/time -v {cmd_str}"
    rc, _sout, serr = run_remote_cmd(exec_node, full, timeout=timeout)
    out["rc"] = rc
    for line in serr.splitlines():
        s = line.strip()
        if s.startswith("Elapsed (wall clock) time"):
            out["wall_s"] = bh._parse_wall(s.split(": ", 1)[1])
        elif s.startswith("User time (seconds):"):
            try: out["user_s"] = float(s.split(":", 1)[1])
            except: pass
        elif s.startswith("System time (seconds):"):
            try: out["sys_s"] = float(s.split(":", 1)[1])
            except: pass
        elif s.startswith("Maximum resident set size (kbytes):"):
            try: out["max_rss_kb"] = int(s.split(":", 1)[1])
            except: pass
        elif s.startswith("Voluntary context switches:"):
            try: out["vol_ctx"] = int(s.split(":", 1)[1])
            except: pass
        elif s.startswith("Involuntary context switches:"):
            try: out["invol_ctx"] = int(s.split(":", 1)[1])
            except: pass
        elif s.startswith("Minor (reclaiming a frame) page faults:"):
            try: out["minor_pf"] = int(s.split(":", 1)[1])
            except: pass
        elif s.startswith("Major (requiring I/O) page faults:"):
            try: out["major_pf"] = int(s.split(":", 1)[1])
            except: pass
    return out


def remote_perf_stat(exec_node, cmd_str, timeout=RUN_TIMEOUT_DEFAULT):
    """Run `perf stat -x , -e <events> <cmd>` on the exec node, parse
    the CSV output. Returns event->value dict or None when perf is
    unavailable / kernel.perf_event_paranoid blocks the user.

    Mirrors measure_perf_stat() in benchmark_helpers: a non-zero rc
    from the wrapped command (timeout 124, SIGINT-exit 130) does not
    invalidate the counters perf already wrote to stderr -- parse
    them regardless and fail only on empty / unparseable output."""
    full = ("perf stat -x , "
            "-e cycles,instructions,branch-misses,cache-misses " + cmd_str)
    rc, _sout, serr = run_remote_cmd(exec_node, full, timeout=timeout)
    out = {}
    for line in serr.splitlines():
        f = line.split(",")
        if len(f) < 3:
            continue
        val = f[0].strip()
        evt = f[2].strip() if len(f) > 2 else ""
        if val in ("<not supported>", "<not counted>", ""):
            continue
        try: out[evt] = int(val)
        except ValueError:
            try: out[evt] = float(val)
            except ValueError: pass
    return out or None


def remote_callgrind(exec_node, cmd_str, work_dir, timeout=None):
    """Run callgrind on the exec node and return instruction count.
    The output file lives in <work_dir>/callgrind.out -- it is parsed
    in-band on the remote with callgrind_annotate to avoid pulling the
    profile back."""
    if timeout is None:
        timeout = RUN_TIMEOUT_DEFAULT * 30   # callgrind is ~30x slower
    out_file = f"{work_dir}/callgrind.out"
    full = (f"rm -f {shlex.quote(out_file)} && "
            f"valgrind --tool=callgrind --quiet "
            f"--callgrind-out-file={shlex.quote(out_file)} {cmd_str} && "
            f"callgrind_annotate {shlex.quote(out_file)} | "
            f"awk '/PROGRAM TOTALS/ {{gsub(\",\",\"\",$1); print $1; exit}}'")
    rc, sout, _serr = run_remote_cmd(exec_node, full, timeout=timeout)
    if rc != 0:
        return None
    for line in sout.splitlines():
        s = line.strip()
        if not s:
            continue
        try:
            return int(s)
        except ValueError:
            pass
    return None


def remote_binary_size(exec_node, bin_path):
    rc, sout, _serr = run_remote_cmd(
        exec_node,
        f"stat -c %s {shlex.quote(bin_path)} 2>/dev/null",
        timeout=10)
    if rc != 0:
        return None
    s = sout.strip()
    try:
        return int(s)
    except ValueError:
        return None


# ---- the high-level "run a benchmark binary on a remote exec node" -------

def run_benchmark_on_exec(compile_node, exec_node, cmake_src_subdir,
                          build_subdir, bin_name, runtime_cmd,
                          profilers, cmake_defs=None, extras=None,
                          run_timeout=RUN_TIMEOUT_DEFAULT, verbose=False):
    """Full flow for one benchmark, one exec node:
      1. stage source on compile node
      2. cmake configure+build on compile node
      3. pull binary; push to exec node
      4. run each profiler that the (already-filtered) profilers list contains

    `runtime_cmd` is a shell-quoted command STRING run from the
    exec_node's work_dir; if it includes the binary, use a relative
    path like './bin --foo 1'.

    Returns a dict { profiler -> result } where result is either
    parsed metrics or None on failure. `binary-size` reads the file
    size on the exec node (the binary may be stripped differently per
    arch)."""
    out = {}
    if verbose:
        print(f"[remote] {exec_node['label']!r}: staging source on "
              f"{compile_node['label']!r}")
    rc, msg = stage_source_on_compile_node(compile_node, verbose=verbose)
    if rc != 0:
        return {p: None for p in profilers}, f"stage failed: {msg}"
    if verbose:
        print(f"[remote] {exec_node['label']!r}: building")
    rc, msg, remote_bld = build_on_compile_node(
        compile_node, cmake_src_subdir, build_subdir,
        cmake_defs=cmake_defs, verbose=verbose)
    if rc != 0:
        return {p: None for p in profilers}, msg
    if verbose:
        print(f"[remote] {exec_node['label']!r}: pushing binary")
    rc, exec_bin, msg = push_binary_to_exec(
        compile_node, exec_node, remote_bld, bin_name,
        extras=extras, verbose=verbose)
    if rc != 0:
        return {p: None for p in profilers}, msg
    # The runtime_cmd may embed ./<bin_name>; make sure we run with
    # work_dir as cwd so the relative path resolves.
    ewd = exec_node["work_dir"]
    cmd_in_work = f"cd {shlex.quote(ewd)} && chmod +x {shlex.quote(bin_name)} && {runtime_cmd}"
    for p in profilers:
        if p == "rusage":
            out[p] = remote_time_v(exec_node, cmd_in_work, timeout=run_timeout)
        elif p == "perf-stat":
            out[p] = remote_perf_stat(exec_node, cmd_in_work, timeout=run_timeout)
        elif p == "callgrind":
            out[p] = remote_callgrind(exec_node, cmd_in_work, ewd,
                                      timeout=run_timeout * 30)
        elif p == "binary-size":
            out[p] = remote_binary_size(exec_node, exec_bin)
        else:
            out[p] = None
    return out, "ok"
