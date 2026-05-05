#!/usr/bin/env python3
"""
testingremote.py — Remote compilation testing on the CC test nodes.

Opens a single SSH connection per node, uploads sources via rsync,
compiles each .pro file with c++11 / no EXTRA CatchChallenger flags,
then removes the work dir on the remote node.

Per-node config is loaded from test/remote_nodes.json via remote_build.
Only nodes with "compile" in their `tests` list are exercised here.
"""

# Drop the .pyc cache for this process so import diagnostic / build_paths /
# remote_build never lands a __pycache__/ dir in the source tree.  Set
# before the first LOCAL import; stdlib bytecode is unaffected.
import sys
sys.dont_write_bytecode = True


import os, sys, subprocess, json, time
import remote_build as _rb
from remote_build import (REMOTE_NODES, configure_timeout_for,
                          compile_timeout_for)
import diagnostic
from cmd_helpers import (SSH_OPTS_LIST, RSYNC_SSH_E, SSH_TIMEOUT_MARKER,
                         is_ssh_timeout, clamp_ssh, clamp_local)

DIAG = diagnostic.parse_diag_args()

# ── config ─────────────────────────────────────────────────────────────────
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Build the (label, ssh_host, ssh_port, linker, remote_dir) tuple list
# from the shared JSON, filtering on the "compile" test opt-in. The
# fourth field used to be `use_mold` (bool); it is now the linker name
# string ("" / "mold" / "lld" / "bfd" / "gold") read via
# remote_build._node_linker (handles back-compat for old `has_mold`).
SERVERS = []
for _n in REMOTE_NODES:
    if "compile" not in _n.get("tests", []):
        continue
    SERVERS.append((
        _n["label"],
        f"{_n['ssh']['user']}@{_n['ssh']['host']}",
        _n["ssh"]["port"],
        _rb._node_linker(_n),
        _n["work_dir"],
    ))

# .pro files to test (paths relative to ROOT)
PRO_FILES = [
    "client/qtcpu800x600/qtcpu800x600.pro",
    "client/qtopengl/catchchallenger-qtopengl.pro",
    "tools/stats/stats.pro",
    "tools/map2png/map2png.pro",
    "tools/datapack-explorer-generator-cli/datapack-explorer-generator.pro",
    "server/epoll/catchchallenger-server-filedb.pro",
    "server/epoll/catchchallenger-server-cli-epoll.pro",
]

# .pro files whose binary can be staged + run on an execution node. The
# success marker on the binary's stdout is "correctly bind:" — only
# server targets emit that. Clients need GUI/X11 (handled by other
# scripts), and tools like stats/map2png exit too quickly or need args
# we can't auto-pick remotely.
EXEC_PRO_FILES = {
    "server/epoll/catchchallenger-server-filedb.pro",
    "server/epoll/catchchallenger-server-cli-epoll.pro",
}

# Map a runnable .pro to its cmake target — the binary lives at
# build-remote/<target>/<binary> after the "one binary per CMakeLists"
# refactor (no more <output_subdir> intermediate path).
# Kept in sync with cmake_helpers._PRO_TO_CMAKE; we only enumerate the
# server entries here so we don't drag client/tool subdirs into exec.
_EXEC_BIN_FOR_PRO = {
    "server/epoll/catchchallenger-server-filedb.pro":
        "catchchallenger-server-cli-epoll",
    "server/epoll/catchchallenger-server-cli-epoll.pro":
        "catchchallenger-server-cli-epoll",
}

# Per-node remote_dir is stored in SERVERS; the constant below is kept only
# as a fallback for ad-hoc callers — it is not used in the loops below.
REMOTE_DIR = "/tmp/catchchallenger-test"
CXX_VERSION = "c++11"
COMPILE_TIMEOUT = 600
RSYNC_TIMEOUT = 300
EXEC_RSYNC_TIMEOUT = 300
EXEC_BIND_TIMEOUT = 60
NICE_PREFIX = ["nice", "-n", "19"]

# Datapack source on the local machine. Staged onto each exec node next
# to the binary so the server has something to load. Pick the smaller
# `test` maincode when available — same trick as remote_build._detect_maincode.
_LOCAL_DATAPACKS = [
    "/home/user/Desktop/CatchChallenger/CatchChallenger-datapack",
    "/home/user/Desktop/CatchChallenger/datapack-pkmn",
]

# ── colours ─────────────────────────────────────────────────────────────────
C_GREEN = "\033[92m"
C_RED = "\033[91m"
C_CYAN = "\033[96m"
C_RESET = "\033[0m"

results = []
_last_log_time = [time.monotonic()]

SCRIPT_NAME = os.path.basename(__file__)
from test_config import FAILED_JSON


def load_failed_cases():
    """Load failed cases for this script from failed.json.
    Returns None (run everything), [] (skip all), or [names] (resume only those)."""
    if not os.path.isfile(FAILED_JSON):
        return None
    try:
        with open(FAILED_JSON, "r") as f:
            data = json.load(f)
    except (json.JSONDecodeError, IOError):
        return None
    if SCRIPT_NAME not in data:
        return None
    return data[SCRIPT_NAME]


def should_run(test_name, failed_cases):
    """Check if test should run. None=run all, []=skip all, [..]=only listed."""
    if failed_cases is None:
        return True
    idx = 0
    while idx < len(failed_cases):
        if failed_cases[idx] == test_name:
            return True
        idx += 1
    return False


def save_failed_cases():
    """Update failed.json with current failures for this script."""
    data = {}
    if os.path.isfile(FAILED_JSON):
        try:
            with open(FAILED_JSON, "r") as f:
                data = json.load(f)
        except (json.JSONDecodeError, IOError):
            data = {}
    failed = []
    idx = 0
    while idx < len(results):
        if not results[idx][1]:
            failed.append(results[idx][0])
        idx += 1
    data[SCRIPT_NAME] = failed
    with open(FAILED_JSON, "w") as f:
        json.dump(data, f, indent=2)


def log_info(msg):
    print(f"{C_CYAN}[INFO]{C_RESET} {msg}")

def log_pass(name, detail=""):
    now = time.monotonic()
    elapsed = now - _last_log_time[0]
    _last_log_time[0] = now
    results.append((name, True, detail, elapsed))
    print(f"{C_GREEN}[PASS]{C_RESET} {name}  {detail}  ({elapsed:.1f}s)")

def log_fail(name, detail=""):
    now = time.monotonic()
    elapsed = now - _last_log_time[0]
    _last_log_time[0] = now
    results.append((name, False, detail, elapsed))
    print(f"{C_RED}[FAIL]{C_RESET} {name}  {detail}  ({elapsed:.1f}s)")
    li = 0
    _ctx = diagnostic.last_cmd_lines()
    while li < len(_ctx):
        print(_ctx[li])
        li += 1


def ssh_cmd(host, port, command, timeout=COMPILE_TIMEOUT):
    """Run a command over SSH, return (returncode, output).

    Every connection forces ConnectTimeout=5 (cmd_helpers.SSH_OPTS_LIST)
    so a dead remote fails fast. The `timeout` arg is silently clamped
    to SSH_MAX_TIMEOUT (1200s) by clamp_ssh().

    On TimeoutExpired we return (-1, "<SSH_TIMEOUT_MARKER>: <secs>s").
    Callers test `out.startswith(SSH_TIMEOUT_MARKER)` (or use the
    `_is_timeout` predicate) so a timeout is distinguishable from a
    build/runtime rc!=0 — every caller must emit a log_fail with the
    timeout duration so the failure summary line explicitly says
    "TIMEOUT" instead of a generic rc=-1."""
    timeout = clamp_ssh(timeout)
    ssh_args = ["ssh"] + SSH_OPTS_LIST + ["-p", str(port), host, command]
    diagnostic.record_cmd(ssh_args, None)
    try:
        p = subprocess.run(ssh_args, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"{SSH_TIMEOUT_MARKER}: {timeout}s"


def _is_timeout(out):
    """Backward-compat alias for is_ssh_timeout (cmd_helpers). Existing
    call sites import via this module name; new code can use the
    centralised one."""
    return is_ssh_timeout(out)


def _rsync_host(host):
    """Wrap IPv6 addresses in brackets for rsync (e.g. user@[2803::1])."""
    if "@" in host:
        u, addr = host.split("@", 1)
        if ":" in addr and not addr.startswith("["):
            return f"{u}@[{addr}]"
    return host


def rsync_to_remote(host, port, remote_dir):
    """Upload source tree to remote server.

    Sources go into <remote_dir>/sources/; build artefacts (build-remote/,
    ccache/, tmp/) sit alongside, not inside the source tree."""
    log_info(f"rsync to {host}:{remote_dir}/sources")
    # ssh user accounts may not have the parent dir yet. Also nuke
    # stale build-remote/ + loose layout-mismatched files left by a
    # previous run; same rationale as remote_build._rsync_to_remote.
    ssh_cmd(host, port,
            f"mkdir -p {remote_dir}/sources && "
            f"find {remote_dir} -mindepth 1 -maxdepth 1 "
            f"-not -name sources -not -name ccache -not -name tmp "
            f"-exec rm -rf {{}} + 2>/dev/null; true",
            timeout=30)
    rsync_args = [
        "rsync", "-art", "--delete",
        "--exclude=.git", "--exclude=build/", "--exclude=*.o",
        "--exclude=moc_*", "--exclude=Makefile",
        "-e", f"{RSYNC_SSH_E} -p {port}",
        ROOT + "/", f"{_rsync_host(host)}:{remote_dir}/sources/"
    ]
    try:
        p = subprocess.run(rsync_args, timeout=clamp_local(RSYNC_TIMEOUT),
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if p.returncode == 0:
            log_pass(f"rsync to {host}")
            return True
        log_fail(f"rsync to {host}", f"rc={p.returncode}")
        out = p.stdout.decode(errors="replace")
        if out.strip():
            idx = 0
            lines = out.splitlines()
            while idx < len(lines) and idx < 20:
                print(f"  | {lines[idx]}")
                idx += 1
        return False
    except subprocess.TimeoutExpired:
        log_fail(f"rsync to {host}", f"TIMEOUT after {RSYNC_TIMEOUT}s")
        return False


def detect_cmake(host, port):
    """Detect a usable cmake on the remote server."""
    rc, out = ssh_cmd(host, port, "which cmake 2>/dev/null", timeout=15)
    if _is_timeout(out):
        log_fail(f"detect cmake on {host}", "TIMEOUT after 15s")
        return None
    if rc == 0 and out.strip():
        cmake = out.strip().splitlines()[0]
        log_info(f"detected cmake: {cmake}")
        return cmake
    log_fail(f"detect cmake on {host}", "cmake not found")
    return None


# Phase 5 (qmake -> CMake): builds on remote nodes now drive cmake too.
# The .pro -> cmake target mapping is the same one local builds use; we
# import it lazily here to avoid a hard dependency on cmake_helpers when
# cmake isn't available on the remote (the function returns False
# cleanly).
def build_pro_remote(host, port, cmake, pro_rel, label, use_mold, remote_dir):
    """Build a target on the remote server via cmake. The pro_rel is
    used purely as a key for the .pro -> cmake target mapping in
    cmake_helpers.py.

    Configure and build are run as two separate SSH invocations so each
    phase is bounded by its own per-node timeout from remote_nodes.json
    (cmake_configure_timeout / cmake_compile_timeout, defaults 180/600s).
    A hung configure or runaway link can no longer eat the whole
    combined budget."""
    import cmake_helpers as _ch
    suffix = diagnostic.build_dir_suffix(DIAG)
    name = f"compile {pro_rel} ({label})"
    try:
        target, configure_flags, source_subdir = _ch.pro_to_cmake_target(pro_rel)
    except KeyError:
        log_fail(name, f"no cmake target mapping for {pro_rel}")
        return False
    build_dir = f"{remote_dir}/build-remote{suffix}/{target}"
    # Per "one binary per CMakeLists.txt" refactor, point -S at the
    # specific subdir's CMakeLists.txt (no root project anymore).
    cmake_source = f"{remote_dir}/sources/{source_subdir}"
    # `use_mold` is now a linker name string ("" / "mold" / "lld" /
    # "bfd" / "gold"). Empty = system default; otherwise pass
    # -fuse-ld=<linker>. Old bool callers (True) coerce to "mold".
    linker_name = use_mold if isinstance(use_mold, str) else ("mold" if use_mold else "")
    mold_args = ""
    linker_path_prefix = ""
    if linker_name:
        # See remote_build.py: lld on MIPS rejects text relocations from
        # cmake's non-PIC compiler probe; -Wl,-z,notext is harmless on
        # other linkers/arches and unblocks the test build.
        # Also pin -fno-lto so the lto-wrapper doesn't fan out at link
        # time (see cmake_helpers.py for the rationale).
        mold_args = (f"-DCMAKE_EXE_LINKER_FLAGS="
                     f"\"-fuse-ld={linker_name} -Wl,-z,notext -fno-lto\"")
        if linker_name == "lld":
            # ld.lld may live under a versioned prefix that is not on
            # $PATH (Gentoo /usr/lib/llvm/<ver>/bin, Debian/Ubuntu
            # /usr/lib/llvm-<ver>/bin); on Fedora/RHEL it's already in
            # /usr/bin and the loop is a no-op.
            linker_path_prefix = (
                "PATH=\"$(for d in /usr/lib/llvm/*/bin /usr/lib/llvm-*/bin; "
                "do [ -x \"$d/ld.lld\" ] && printf '%s:' \"$d\"; done)$PATH\" "
            )
    flag_args = " ".join(configure_flags)
    cfg_t = configure_timeout_for(label)
    bld_t = compile_timeout_for(label)

    log_info(f"configuring {target} on {label} (timeout {cfg_t}s)")
    # Per-node has_ninja / has_ccache switches from remote_nodes.json,
    # both default true. has_ccache=false sets CCACHE_DISABLE=1 instead
    # of CCACHE_DIR, has_ninja=false drops the -G Ninja flag.
    from remote_build import get_node as _get_node
    _node = _get_node(label)
    # Pin to /usr/bin/ninja so the remote cmake doesn't pick a pip shim.
    _gen = ("-G Ninja -DCMAKE_MAKE_PROGRAM=/usr/bin/ninja "
            if (not _node or _node.get("has_ninja", True)) else "")
    if _node is None or _node.get("has_ccache", True):
        _ccache_env = f"CCACHE_DIR={remote_dir}/ccache "
        _ccache_mkdir = f"mkdir -p {remote_dir}/ccache && "
    else:
        _ccache_env = "CCACHE_DISABLE=1 "
        _ccache_mkdir = ""
    init_cflags = "-fno-lto"
    init_cxxflags = "-fno-lto"
    if linker_name == "lld":
        init_cflags = "-fPIC " + init_cflags
        init_cxxflags = "-fPIC " + init_cxxflags
    pic_args = (f"-DCMAKE_C_FLAGS_INIT='{init_cflags}' "
                f"-DCMAKE_CXX_FLAGS_INIT='{init_cxxflags}' ")
    configure_cmd = (
        f"mkdir -p {build_dir} && {_ccache_mkdir}"
        f"{linker_path_prefix}{_ccache_env}"
        f"{cmake} {_gen}-S {cmake_source} -B {build_dir} "
        # Force PIC so the lld compiler-feature probes (which would
        # otherwise emit R_MIPS_32 / TEXTREL relocations) link cleanly.
        # PIC is the gcc default on most distros so it's a no-op
        # elsewhere; only applied when linker=lld.
        f"-DCMAKE_BUILD_TYPE=Debug {pic_args}-DCMAKE_CXX_STANDARD={CXX_VERSION.replace('c++','')} "
        f"{flag_args} {mold_args} "
        f"2>&1"
    )
    rc, out = ssh_cmd(host, port, configure_cmd, timeout=cfg_t)
    if _is_timeout(out):
        log_fail(name, f"configure TIMEOUT after {cfg_t}s")
        return False
    if rc != 0:
        log_fail(name, f"configure rc={rc}")
        lines = out.splitlines()
        start = max(0, len(lines) - 30)
        idx = start
        while idx < len(lines):
            print(f"  | {lines[idx]}")
            idx += 1
        return False

    log_info(f"building {target} on {label} (timeout {bld_t}s)")
    build_cmd = (
        f"{_ccache_env}"
        f"{cmake} --build {build_dir} --target {target} -j$(nproc) 2>&1"
    )
    rc, out = ssh_cmd(host, port, build_cmd, timeout=bld_t)
    if _is_timeout(out):
        log_fail(name, f"build TIMEOUT after {bld_t}s")
        return False
    if rc == 0:
        log_pass(name)
        return True
    log_fail(name, f"rc={rc}")
    lines = out.splitlines()
    start = len(lines) - 30
    if start < 0:
        start = 0
    idx = start
    while idx < len(lines):
        print(f"  | {lines[idx]}")
        idx += 1
    return False


def cleanup_remote(host, port, remote_dir):
    """Remove the work dir on a remote node after tests, preserving
    <work_dir>/ccache (persistent compiler cache) and <work_dir>/sources
    (incremental rsync target — preserved across runs so the next rsync
    only carries diffs)."""
    if not remote_dir or len(remote_dir) < 6 or remote_dir == "/":
        return
    cmd = (f"find {remote_dir} -mindepth 1 -maxdepth 1 "
           f"-not -name ccache -not -name sources "
           f"-exec rm -rf {{}} + 2>/dev/null; true")
    ssh_cmd(host, port, cmd, timeout=60)


# ── execution-node staging + run ─────────────────────────────────────────────

def _pick_local_datapack():
    """Return (datapack_dir, maincode) for the first available local datapack.
    Prefers the smaller `test` maincode when present so a slow exec node
    finishes loading inside EXEC_BIND_TIMEOUT seconds. Returns (None, None)
    when no datapack source is found — caller skips the run phase."""
    di = 0
    while di < len(_LOCAL_DATAPACKS):
        dp = _LOCAL_DATAPACKS[di]
        di += 1
        if not os.path.isdir(dp):
            continue
        map_main = os.path.join(dp, "map", "main")
        if not os.path.isdir(map_main):
            continue
        entries = sorted(os.listdir(map_main))
        # prefer "test"
        ei = 0
        while ei < len(entries):
            if entries[ei] == "test" and os.path.isdir(os.path.join(map_main, entries[ei])):
                return dp, "test"
            ei += 1
        # fall back to first dir
        ei = 0
        while ei < len(entries):
            if os.path.isdir(os.path.join(map_main, entries[ei])):
                return dp, entries[ei]
            ei += 1
    return None, None


def _make_exec_server_properties(maincode):
    """Minimal server-properties.xml for an exec-node smoke run. Only the
    fields BaseServer reads at startup are listed; everything else falls
    back to NormalServerGlobal::checkSettingsFile defaults. We pin
    mainDatapackCode to whatever datapack we just staged so the server
    doesn't bail with "datapack/map/main/.../" don't exists."""
    return (
        "<?xml version=\"1.0\"?>\n"
        "<options>\n"
        "  <general>\n"
        f"    <max-players value=\"10\"/>\n"
        f"    <pvp value=\"true\"/>\n"
        "  </general>\n"
        "  <content>\n"
        f"    <mainDatapackCode value=\"{maincode}\"/>\n"
        "    <subDatapackCode value=\"\"/>\n"
        "  </content>\n"
        "  <network>\n"
        "    <server-port value=\"61917\"/>\n"
        "    <server-ip value=\"\"/>\n"
        "    <httpDatapackMirror value=\"\"/>\n"
        "  </network>\n"
        "</options>\n"
    )


def _scp_to_exec(local_path, exec_node):
    """rsync a single file or directory tree onto an exec node. The
    work_dir is created if missing. Returns (ok, detail) — `detail` is
    "" on success, "TIMEOUT after Ns" on subprocess.TimeoutExpired,
    "ssh mkdir failed/TIMEOUT" on the SSH probe failing, or "rc=N" on
    a non-zero rsync exit. Caller emits log_fail using detail so the
    test summary always carries the timeout duration when one fired."""
    user = exec_node["user"]
    host = exec_node["host"]
    port = int(exec_node.get("port", 22))
    work = exec_node["work_dir"]
    rc, out = ssh_cmd(f"{user}@{host}", port, f"mkdir -p {work}", timeout=15)
    if _is_timeout(out):
        return False, "ssh mkdir TIMEOUT after 15s"
    if rc != 0:
        return False, f"ssh mkdir rc={rc}"
    rsync_args = [
        "rsync", "-a", "--delete",
        "-e", f"{RSYNC_SSH_E} -p {port}",
        local_path, f"{_rsync_host(user + '@' + host)}:{work}/"
    ]
    diagnostic.record_cmd(rsync_args, None)
    eff_timeout = clamp_local(EXEC_RSYNC_TIMEOUT)
    try:
        p = subprocess.run(rsync_args, timeout=eff_timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except subprocess.TimeoutExpired:
        return False, f"rsync TIMEOUT after {eff_timeout}s"
    if p.returncode == 0:
        return True, ""
    return False, f"rsync rc={p.returncode}"


def _push_text_to_exec(content, remote_path, exec_node):
    """Write a small text file (server-properties.xml) on the exec node
    via a single SSH `cat > path` invocation."""
    user = exec_node["user"]
    host = exec_node["host"]
    port = int(exec_node.get("port", 22))
    # Quote-safe heredoc: the XML has no embedded XML-ish 'EOF' sentinel
    cmd = f"cat > {remote_path} << 'CCEOF'\n{content}CCEOF\n"
    rc, _ = ssh_cmd(f"{user}@{host}", port, cmd, timeout=15)
    return rc == 0


def _stage_runtime_to_exec(exec_node, dp_local, maincode):
    """Symlink the pre-staged remote datapack into <work>/datapack +
    push server-properties.xml. The datapack itself was rsynced to
    <datapack_cache>/<id>/ on this exec_node by stage_datapacks.py at
    all.sh startup (CLAUDE.md "Datapack staging cache"); per-test
    setup is now `ln -sfn <cache>/<id> <work>/datapack` instead of an
    rsync. Maincode pruning is dropped: the staged cache is shared
    between tests and we must not mutate it; mainDatapackCode in
    server-properties.xml selects which maincode the server actually
    loads, so leaving the others on disk is harmless.

    Returns (ok, detail). On any timeout the detail says so."""
    import datapack_stage as _ds
    work = exec_node["work_dir"]
    user = exec_node["user"]
    host = exec_node["host"]
    port = int(exec_node.get("port", 22))
    cache_target = _ds.staged_remote(exec_node, dp_local)
    # Replace any leftover datapack/ from a prior run with a symlink to
    # the persistent cache. `ln -sfn <target> <link>` is idempotent —
    # re-runs just refresh the symlink target (cheap, atomic). Done in
    # ONE ssh round-trip including mkdir + the link.
    cmd = (f"mkdir -p {work} && rm -rf {work}/datapack && "
           f"ln -sfn {cache_target} {work}/datapack")
    rc, out = ssh_cmd(f"{user}@{host}", port, cmd, timeout=30)
    if _is_timeout(out):
        return False, "ssh symlink TIMEOUT after 30s"
    if rc != 0:
        return False, (f"ssh symlink datapack rc={rc} "
                       f"(cache target {cache_target}): "
                       + (out.strip()[-200:] if out else ""))
    if not _push_text_to_exec(_make_exec_server_properties(maincode),
                              f"{work}/server-properties.xml", exec_node):
        return False, "push server-properties.xml failed"
    return True, ""


def _run_on_exec_node(compile_host, compile_port, compile_remote_dir,
                      target, pro_rel, compile_label,
                      exec_node):
    """Stage the binary built on `compile_host` onto `exec_node`, then run
    it and wait for "correctly bind:". Returns True on success.

    Per "one binary per CMakeLists.txt" refactor: the binary lands at
    `<build-remote>/<target>/<target>` on the compile node (each binary's
    own self-contained build dir; no nested output_subdir)."""
    binary_remote = (f"{compile_remote_dir}/build-remote"
                     f"{diagnostic.build_dir_suffix(DIAG)}"
                     f"/{target}/{target}")
    exec_label = exec_node.get("label", exec_node["host"])
    test_name = f"exec {pro_rel} ({compile_label} → {exec_label})"

    # Pull the binary from the compile node onto our local staging tmpdir,
    # then push it to the exec node. SSH-only path — avoids needing the
    # compile node and exec node to know about each other.
    local_stage = f"/tmp/cc-exec-{compile_label}-{exec_label}"
    # Bound the local cleanup too — `rm -rf` shouldn't hang, but if the
    # tmpdir is on a stuck NFS / fuse mount it could; 60s is plenty.
    # On TimeoutExpired we still try to mkdir below; if that fails the
    # rsync_pull will fail too and the test FAILs.
    try:
        subprocess.run(["rm", "-rf", local_stage], timeout=60)
    except subprocess.TimeoutExpired:
        log_fail(test_name, "local rm -rf TIMEOUT after 60s (stuck mount?)")
        return False
    os.makedirs(local_stage, exist_ok=True)
    rsync_pull = [
        "rsync", "-a",
        "-e", f"{RSYNC_SSH_E} -p {compile_port}",
        f"{_rsync_host(compile_host)}:{binary_remote}",
        local_stage + "/",
    ]
    diagnostic.record_cmd(rsync_pull, None)
    pull_timeout = clamp_local(EXEC_RSYNC_TIMEOUT)
    try:
        p = subprocess.run(rsync_pull, timeout=pull_timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except subprocess.TimeoutExpired:
        log_fail(test_name, f"rsync from compile node TIMEOUT after {pull_timeout}s")
        return False
    if p.returncode != 0:
        log_fail(test_name, f"rsync from compile node rc={p.returncode}")
        return False
    local_binary = os.path.join(local_stage, target)
    if not os.path.isfile(local_binary):
        log_fail(test_name, f"binary not pulled from compile node: {binary_remote}")
        return False

    # Push binary to exec node
    ok, detail = _scp_to_exec(local_binary, exec_node)
    if not ok:
        log_fail(test_name, "rsync binary to exec node: " + detail)
        return False

    # Stage runtime (datapack + server-properties.xml). Cached across
    # exec runs on the same node within this session — but cheap to redo.
    dp_local, maincode = _pick_local_datapack()
    if dp_local is None:
        log_fail(test_name, "no local datapack to stage")
        return False
    ok, detail = _stage_runtime_to_exec(exec_node, dp_local, maincode)
    if not ok:
        log_fail(test_name, "datapack/server-properties stage: " + detail)
        return False

    # Run on exec node, capture output until "correctly bind:" or timeout.
    user = exec_node["user"]
    host = exec_node["host"]
    port = int(exec_node.get("port", 22))
    work = exec_node["work_dir"]
    bin_path = f"{work}/{target}"
    # chmod +x just in case the rsync didn't preserve mode bits across
    # exotic filesystems, then run with timeout(1) so a hung server can't
    # block the test forever even when ssh_cmd's own timeout fires.
    run_cmd = (
        f"chmod +x {bin_path} && "
        f"cd {work} && "
        f"timeout {EXEC_BIND_TIMEOUT}s ./{target} 2>&1 | head -80"
    )
    rc, out = ssh_cmd(f"{user}@{host}", port, run_cmd,
                      timeout=EXEC_BIND_TIMEOUT + 30)
    # stop any leftover server (in case "correctly bind:" appeared but
    # the head -80 closed the pipe before SIGTERM)
    ssh_cmd(f"{user}@{host}", port,
            f"pkill -f {target} 2>/dev/null; true", timeout=10)
    if _is_timeout(out):
        log_fail(test_name,
                 f"exec ssh TIMEOUT after {EXEC_BIND_TIMEOUT + 30}s")
        return False
    if "correctly bind:" in out:
        log_pass(test_name, "correctly bind: detected")
        return True
    log_fail(test_name, "no 'correctly bind:' before timeout")
    lines = out.splitlines()
    start = max(0, len(lines) - 20)
    li = start
    while li < len(lines):
        print(f"  | {lines[li]}")
        li += 1
    return False


def _exec_node_enabled(exec_node):
    """Per-entry `enabled` flag, defaulting to True when missing — mirrors
    the top-level node `enabled` semantics so disabling a flaky exec box
    doesn't require deleting its config."""
    return bool(exec_node.get("enabled", True))


def run_exec_phase(compile_label, compile_host, compile_port,
                   compile_remote_dir, exec_nodes, failed_cases):
    """For each exec node configured on the compile node, push the
    compiled server binary plus runtime files and verify it starts.
    Entries with `enabled: false` are skipped silently (no test recorded)
    so the failed.json resume list isn't polluted with disabled nodes."""
    if not exec_nodes:
        return
    pi = 0
    while pi < len(PRO_FILES):
        pro_rel = PRO_FILES[pi]
        pi += 1
        if pro_rel not in EXEC_PRO_FILES:
            continue
        target = _EXEC_BIN_FOR_PRO.get(pro_rel)
        if target is None:
            continue
        ei = 0
        while ei < len(exec_nodes):
            exec_node = exec_nodes[ei]
            ei += 1
            exec_label = exec_node.get("label", exec_node["host"])
            if not _exec_node_enabled(exec_node):
                log_info(f"exec node {exec_label} disabled, skipping {pro_rel}")
                continue
            test_name = f"exec {pro_rel} ({compile_label} → {exec_label})"
            if not should_run(test_name, failed_cases):
                continue
            _run_on_exec_node(compile_host, compile_port, compile_remote_dir,
                              target, pro_rel, compile_label,
                              exec_node)


def cleanup_exec_node(exec_node):
    """Wipe the work_dir on an exec node after tests, preserving any
    `ccache/` subdir. Compile nodes own a `<work>/ccache` slot; if an
    operator ever configures an exec_node with the same work_dir as a
    compile node (overlap), this guard keeps the cache intact even
    though no exec phase puts ccache there itself."""
    user = exec_node["user"]
    host = exec_node["host"]
    port = int(exec_node.get("port", 22))
    work = exec_node["work_dir"]
    if not work or len(work) < 6 or work == "/":
        return
    cmd = (f"find {work} -mindepth 1 -maxdepth 1 -not -name ccache "
           f"-exec rm -rf {{}} + 2>/dev/null; true")
    ssh_cmd(f"{user}@{host}", port, cmd, timeout=60)


def test_server(label, host, port, use_mold, remote_dir, failed_cases=None,
                exec_nodes=None):
    """Run all compilation tests on a single remote server then clean up.
    If exec_nodes is non-empty, after compile each runnable server target
    is pushed to those exec nodes and started — see EXEC_PRO_FILES."""
    print(f"\n{C_CYAN}{'='*60}")
    print(f"  Testing on: {label} ({host}:{port})")
    print(f"{'='*60}{C_RESET}\n")

    if not should_run(f"rsync to {host}", failed_cases):
        need_rsync = False
        idx = 0
        while idx < len(PRO_FILES):
            if should_run(f"compile {PRO_FILES[idx]} ({label})", failed_cases):
                need_rsync = True
                break
            idx += 1
        if not need_rsync:
            log_info(f"skipping {label}: no failed tests to resume")
            return

    try:
        if not rsync_to_remote(host, port, remote_dir):
            return

        cmake = detect_cmake(host, port)
        if cmake is None:
            return

        idx = 0
        while idx < len(PRO_FILES):
            if should_run(f"compile {PRO_FILES[idx]} ({label})", failed_cases):
                build_pro_remote(host, port, cmake, PRO_FILES[idx], label,
                                 use_mold, remote_dir)
            idx += 1

        # After compile, optionally fan-out to exec nodes — copy the just-
        # built server binary + datapack/server-properties.xml onto each
        # configured exec node and verify it boots ("correctly bind:").
        # No-op when the node has no execution_nodes entries.
        if exec_nodes:
            run_exec_phase(label, host, port, remote_dir, exec_nodes,
                           failed_cases)
    finally:
        # Clean up exec nodes BEFORE the compile node so a binary leak
        # there can't outlive the run when something fails mid-test.
        # Skip disabled entries — we never touched them, and a stray
        # rm -rf on a wrong work_dir on a disabled-because-flaky machine
        # is exactly the wrong move.
        if exec_nodes:
            ei = 0
            while ei < len(exec_nodes):
                if _exec_node_enabled(exec_nodes[ei]):
                    cleanup_exec_node(exec_nodes[ei])
                ei += 1
        cleanup_remote(host, port, remote_dir)


def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — Remote Compilation Testing")
    print(f"{'='*60}{C_RESET}\n")

    failed_cases = load_failed_cases()
    if failed_cases is not None and len(failed_cases) == 0:
        log_info("all previously passed, skipping (delete test/failed.json for full re-run)")
        return

    if diagnostic.is_active(DIAG):
        log_info(f"diagnostic mode: {diagnostic.describe(DIAG)}")
    idx = 0
    while idx < len(SERVERS):
        label, host, port, use_mold, remote_dir = SERVERS[idx]
        # filter nodes that don't have the compiler we need for the diag mode
        node = next((n for n in REMOTE_NODES if n["label"] == label), None)
        if node is not None and not diagnostic.node_supports(node, DIAG):
            need = diagnostic.compiler_name(DIAG)
            log_info(f"skipping {label}: node has no {need} (diag mode)")
            idx += 1
            continue
        # exec_nodes is the per-compile-node list of remote machines that
        # receive the just-built binary + runtime files. Defaults to []
        # (no run phase) when the JSON entry omits it.
        exec_nodes = node.get("execution_nodes", []) if node else []
        test_server(label, host, port, use_mold, remote_dir, failed_cases,
                    exec_nodes=exec_nodes)
        idx += 1

    # summary
    print(f"\n{C_CYAN}{'='*60}")
    print("  Summary")
    print(f"{'='*60}{C_RESET}")
    passed = sum(1 for r in results if r[1])
    failed = sum(1 for r in results if not r[1])
    total_elapsed = sum(r[3] for r in results)
    idx = 0
    while idx < len(results):
        name, ok, detail, elapsed = results[idx]
        tag = f"{C_GREEN}PASS{C_RESET}" if ok else f"{C_RED}FAIL{C_RESET}"
        print(f"  [{tag}] {name}  {detail}  ({elapsed:.1f}s)")
        idx += 1
    print(f"  total elapsed: {total_elapsed:.1f}s")
    print()
    print(f"  {C_GREEN}{passed} passed{C_RESET}, {C_RED}{failed} failed{C_RESET}")
    save_failed_cases()
    if failed:
        sys.exit(1)


if __name__ == "__main__":
    main()
