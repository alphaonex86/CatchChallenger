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
import threading
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


# Which benchmark's remote work these [remote] lines belong to. Set once by
# each benchmark*.py (set_benchmark_label) so every remote-dispatch line is
# attributable when several benchmarks share a logfile / a batch run. One
# benchmark per process, so a module global is unambiguous.
_BENCH_LABEL = ""


def set_benchmark_label(name):
    global _BENCH_LABEL
    _BENCH_LABEL = name or ""


def _rlog(msg, err=False):
    """Print a remote-dispatch line tagged with the current benchmark, e.g.
    `[remote:benchmarkmapmanager] 'pentium-m': pushing binary`. Falls back
    to a bare `[remote]` tag when no benchmark label was set."""
    tag = f"[remote:{_BENCH_LABEL}]" if _BENCH_LABEL else "[remote]"
    print(f"{tag} {msg}", file=sys.stderr if err else sys.stdout, flush=True)


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
            "-o", "StrictHostKeyChecking=no",
            "-o", "UserKnownHostsFile=/dev/null",
            # LogLevel=ERROR drops the per-connection "Warning: Permanently
            # added '<host>' to the list of known hosts." line that
            # UserKnownHostsFile=/dev/null otherwise emits on every ssh.
            "-o", "LogLevel=ERROR",
            "-p", str(port), _ssh_host(user, host), cmd]
    proc = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            text=True)
    try:
        stdout, stderr = proc.communicate(timeout=timeout)
        if proc.returncode != 0:
            bh.note_ssh_failure(host, host, proc.returncode, stderr)
        return proc.returncode, stdout, stderr
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()
        return -1, "", f"ssh timeout after {timeout}s"


# ---- diskless NFS-LXC exec node bring-up --------------------------------
# Self-contained (benchmark_remote intentionally does not import
# test/remote_build). Inert for every ordinary exec node: a node whose
# lxc_nfs block is absent or has enabled=false returns immediately, so
# these are safe to call unconditionally around every benchmark.

def _lxc_nfs_cfg(exec_node):
    blk = exec_node.get("lxc_nfs")
    if not isinstance(blk, dict) or not bool(blk.get("enabled", False)):
        return None
    return blk


# Container is always this fixed name (no lxc_name field).
NFS_LXC_CONTAINER_NAME = "catchchallenger"


def _nfs_lxc_script(cfg, host_addr, action, mac=None):
    """action: 'up' = stop-all/mount/chroot/lxc-start; 'down' =
    lxc-stop + umount. `host_addr` is the exec node's own top-level
    host (bridge-side address). Empty host_addr / guest_* stay
    UNCONFIGURED (never assigned)."""
    q = shlex.quote
    nm = cfg["nfs_mount"]
    export_path = cfg.get("nfs_export", "/")
    name = NFS_LXC_CONTAINER_NAME
    # Where the runtime-generated container config is bind-mounted inside
    # the chroot (a device tmpfs is bound here; see prep_conf below).
    conf_mnt = nm.rstrip("/") + "/run/lxc-cc"
    conf_path = "/run/lxc-cc/" + name + ".conf"   # path AS SEEN in the chroot
    if action == "down":
        parts = [
            f"chroot {q(nm)} /bin/bash -c "
            f"{q(f'lxc-stop -n {q(name)} -k 2>/dev/null || true')} "
            f"2>/dev/null || true",
            # release the tmpfs config bind first
            f"umount {q(conf_mnt)} 2>/dev/null || true",
        ]
        # unmount the nested host binds (reverse order) so the final umount
        # of the rootfs isn't blocked by busy submounts. Lazy (-l) fallback:
        # a plain umount can report EBUSY on a bind whose source is still in
        # use on the device (e.g. /proc, /sys); lazy detach always succeeds
        # and the bind is reaped once free -- the device's real /proc, /sys,
        # /dev are untouched (only the <nm>/* view is detached).
        for p in ("/sys/fs/cgroup", "/dev/shm", "/dev/pts", "/dev",
                  "/sys", "/proc"):
            parts.append(f"umount {q(nm + p)} 2>/dev/null || "
                         f"umount -l {q(nm + p)} 2>/dev/null || true")
        # recursive umount when available (coreutils), else plain, else lazy
        parts.append(f"umount -R {q(nm)} 2>/dev/null || "
                     f"umount {q(nm)} 2>/dev/null || "
                     f"umount -l {q(nm)} 2>/dev/null || true")
        return "\n".join(parts)
    bridge = cfg["bridge"]
    # Container rootfs inside the mounted test-box rootfs. Overridable via
    # the lxc_nfs.lxc_rootfs field; defaults to the conventional location.
    lxc_rootfs = cfg.get("lxc_rootfs", "/var/lib/lxc/rootfs")
    addr = []
    if bridge:
        addr.append(f"ip link set {q(bridge)} up 2>/dev/null || true")
        if host_addr:
            addr.append(f"ip addr add {q(host_addr)} dev {q(bridge)} 2>/dev/null || true")
    # lxc-start with an EXPLICIT config file (-f) generated at runtime in
    # tmpfs -- no pre-baked /var/lib/lxc/<name>/config needed in the rootfs,
    # and nothing is written to the NFS rootfs / device flash. The chroot's
    # /proc, /sys, /dev are bind-mounted from the device BEFORE the chroot
    # (see host_binds in the up script) -- lxc-start as the container
    # monitor needs a working /proc (lxc_check_inherited reads
    # /proc/self/fd) and a functional /dev (ptmx/null).
    inner = ["set -e", *addr,
             f"lxc-start -n {q(name)} -f {q(conf_path)} -d",
             f"lxc-wait -n {q(name)} -s RUNNING -t 60 2>/dev/null || true"]
    if mac:
        # Pin the persisted stable MAC on the guest eth0 (down/set/up)
        # so ARP/NDP stay constant across every start.
        set_mac = (f"ip link set dev eth0 down 2>/dev/null; "
                   f"ip link set dev eth0 address {q(mac)} 2>/dev/null; "
                   f"ip link set dev eth0 up 2>/dev/null")
        inner.append(f"lxc-attach -n {q(name)} -- sh -c {q(set_mac)} "
                     f"2>/dev/null || true")
    if cfg.get("guest_ipv4"):
        inner.append(f"lxc-attach -n {q(name)} -- ip -4 addr add {q(cfg['guest_ipv4'])} dev eth0 2>/dev/null || true")
    if cfg.get("guest_ipv6"):
        inner.append(f"lxc-attach -n {q(name)} -- ip -6 addr add {q(cfg['guest_ipv6'])} dev eth0 2>/dev/null || true")
    # Try NFSv4 first; fall back to NFSv3 if the server denies access
    # (older OpenWrt or custom NFS configs may not export / via v4).
    # The nfs_export field defaults to "/" when unset.
    #
    # IPv6 server literals MUST be bracketed for mount ([addr]:/path) --
    # an unbracketed "2803:1920::2:10:/" is ambiguous (the colons run into
    # the path) and mount rejects it. IPv4 / hostnames pass through. This
    # matters because the export ACL may grant only the box's IPv6: mount
    # the server's IPv4 and the connection comes from a non-granted source,
    # so the server returns "access denied".
    srv = cfg["nfs"]
    if ":" in srv and not srv.startswith("["):
        srv = "[" + srv + "]"
    nfs_export = q(srv + ":" + export_path)
    # nolock on the v3 fallback: embedded boxes frequently run no rpc.statd,
    # and NLM locking then fails the mount with "rpc.statd is not running
    # but is required for remote locking".
    mount_attempts = " || ".join([
        f"mount -t nfs4 {nfs_export} {q(nm)}",
        f"mount -t nfs -o vers=3,nolock {nfs_export} {q(nm)}",
    ])
    # Container config built at runtime. The lxc.net.0 block is appended at
    # runtime (see net_append) because the right link type depends on what
    # `bridge` actually is on the device. rootfs points at the prepared dir
    # in the test-box rootfs (override with lxc_nfs.lxc_rootfs).
    conf_lines = [
        "lxc.uts.name = " + name,
        "lxc.rootfs.path = dir:" + lxc_rootfs,
        # Headless container started inside a chrooted NFS rootfs that has
        # no controlling tty and no devpts: disable console + pty allocation,
        # otherwise lxc-start aborts at "Failed to allocate terminal" /
        # "Failed to open terminal multiplexer device".
        "lxc.console.path = none",
        "lxc.pty.max = 0",
        # The chroot host has no /proc, /sys, or cgroup mounts set up; let
        # lxc auto-mount them into the container's namespace.
        "lxc.mount.auto = proc:mixed sys:ro cgroup:mixed",
    ]
    # Operator-supplied raw lxc config lines from remote_nodes.json
    # (lxc_nfs.lxc_extra_config: a list of strings). Appended VERBATIM so
    # the box's specifics are encoded without a code change -- e.g. a
    # populated rootfs (`lxc.rootfs.path = ...` overriding the default),
    # auto-mounting proc/sys/cgroup for the container
    # (`lxc.mount.auto = proc:mixed sys:ro cgroup:mixed`), cgroup limits,
    # idmaps, etc. Each entry becomes one config line.
    extra = cfg.get("lxc_extra_config", [])
    if isinstance(extra, str):
        extra = [extra]
    for line in extra:
        conf_lines.append(str(line))
    conf_text = "\n".join(conf_lines) + "\n"
    # Pick the tmpfs mount with the MOST free space on the device, write the
    # config there, and bind-mount it under <nm>/run/lxc-cc so the chrooted
    # lxc-start sees it at conf_path. Pure RAM: never touches the NFS rootfs
    # or the device flash. df -k / awk are busybox-portable; fall back to
    # /tmp when no tmpfs is mounted.
    # Network block, appended at runtime with link-type AUTO-DETECTION:
    # attaching a veth to `bridge` only works when it is a real bridge
    # device. A plain NIC or a VLAN sub-interface (e.g. eth0.5) is NOT a
    # bridge -- veth attach fails with "Operation not permitted". For those
    # we use macvlan (bridge mode), which attaches a container NIC directly
    # to the interface without needing a Linux bridge. Detection: a real
    # bridge has /sys/class/net/<if>/bridge. Guest MAC/IPs are still pinned
    # via lxc-attach after start.
    conf_file = '"$T/lxc-cc/' + name + '.conf"'
    mac_echo = ('  echo "lxc.net.0.hwaddr = ' + mac + '"\n') if mac else ""
    net_append = (
        f"B={q(bridge)}\n"
        "{\n"
        '  if [ -d "/sys/class/net/$B/bridge" ]; then\n'
        '    echo "lxc.net.0.type = veth"\n'
        "  else\n"
        '    echo "lxc.net.0.type = macvlan"\n'
        '    echo "lxc.net.0.macvlan.mode = bridge"\n'
        "  fi\n"
        '  echo "lxc.net.0.link = $B"\n'
        '  echo "lxc.net.0.flags = up"\n'
        '  echo "lxc.net.0.name = eth0"\n'
        f"{mac_echo}"
        f"}} >> {conf_file}"
    )
    prep_conf = "\n".join([
        "T=$(awk '$3==\"tmpfs\"{print $2}' /proc/mounts 2>/dev/null | "
        "while read m; do a=$(df -k \"$m\" 2>/dev/null | "
        "awk 'NR==2{print $4+0}'); [ -n \"$a\" ] && echo \"$a $m\"; done | "
        "sort -rn | head -1 | awk '{print $2}')",
        '[ -z "$T" ] && T=/tmp',
        'mkdir -p "$T/lxc-cc"',
        f"printf '%s' {q(conf_text)} > \"$T/lxc-cc/{name}.conf\"",
        net_append,
        f"mkdir -p {q(conf_mnt)}",
        f'mount --bind "$T/lxc-cc" {q(conf_mnt)}',
    ])
    # Bind the device's ALREADY-WORKING /proc, /sys, /dev (incl. a
    # functional /dev/null, /dev/ptmx) and the cgroup hierarchy into the
    # freshly-mounted rootfs BEFORE chroot. Bind (not a fresh devtmpfs/proc
    # mount inside the chroot) because mounting a new devtmpfs over /dev
    # leaves /dev/null non-functional and breaks every `2>/dev/null` in the
    # chroot. lxc-start (container monitor) needs these to enumerate fds and
    # place the container cgroup.
    host_binds = "\n".join([
        f"mount --bind /proc {q(nm + '/proc')} 2>/dev/null || true",
        f"mount --bind /sys {q(nm + '/sys')} 2>/dev/null || true",
        f"mount --bind /dev {q(nm + '/dev')} 2>/dev/null || true",
        f"mount --bind /sys/fs/cgroup {q(nm + '/sys/fs/cgroup')} "
        f"2>/dev/null || true",
    ])
    return "\n".join([
        "set -e",
        "lxc container stop -k all 2>/dev/null || lxc-stop -a -k 2>/dev/null || true",
        f"mkdir -p {q(nm)}",
        f"umount {q(nm)} 2>/dev/null || true",
        f"({mount_attempts})",
        host_binds,
        prep_conf,
        f"chroot {q(nm)} /bin/bash -c {q(chr(10).join(inner))}",
    ])


def _exec_ssh(exec_node):
    """(user, host, port, host_addr) for the exec node's OWN pre-chroot
    SSH (key auth). host_addr == host is the bridge-side address."""
    return (exec_node["user"], exec_node["host"],
            int(exec_node.get("port", 22)), exec_node["host"])


def _runtime_exec_node(exec_node):
    """Return the exec_node dict the BENCHMARK should ssh/rsync to once
    the NFS-LXC bring-up has finished. For an ordinary exec node this
    is exec_node unchanged. For an lxc_nfs node, the device's pre-chroot
    `host` is swapped for the container's `guest_ipv6` (or guest_ipv4
    when ipv6 is empty) -- that's where the rootfs and work_dir live;
    the pre-chroot address only owns the bring-up/teardown SSH.
    `user`, `port`, `work_dir` are kept (by convention the container
    accepts the same login as the device's management shell)."""
    cfg = _lxc_nfs_cfg(exec_node)
    if cfg is None:
        return exec_node
    guest = cfg.get("guest_ipv6") or cfg.get("guest_ipv4")
    if not guest:
        return exec_node
    runtime = dict(exec_node)
    runtime["host"] = guest
    runtime["lxc_nfs"] = None  # downstream code must not re-bring-up
    return runtime


def wait_for_ssh(user, host, port, attempts=20, delay=2.0, verbose=False):
    """Poll the host with a cheap `ssh ... true` until it answers, or
    `attempts` is exhausted. Returns True on success, False otherwise.
    `lxc-start` returns immediately after the container PID exists but
    well before sshd inside is accepting connections -- without this
    poll the first rsync after bring-up races and fails."""
    for i in range(1, attempts + 1):
        rc, _o, _e = ssh_run(user, host, port, "true", timeout=5)
        if rc == 0:
            if verbose and i > 1:
                _rlog(f"ssh to {user}@{host}:{port} ready after "
                      f"{i} attempt(s)")
            return True
        time.sleep(delay)
    if verbose:
        _rlog(f"ssh to {user}@{host}:{port} NOT ready after "
              f"{attempts} attempts", err=True)
    return False


def nfs_lxc_bring_up(exec_node, verbose=False):
    """Return (ok, tail). `tail` is the last lines of the bring-up stderr
    on failure (empty otherwise). Every return path is a 2-tuple so callers
    can unpack unconditionally -- the no-op cases (ordinary node / lxc_nfs
    misconfigured) return (True, "")."""
    cfg = _lxc_nfs_cfg(exec_node)
    if cfg is None:
        return True, ""
    if not cfg.get("nfs") or not cfg.get("nfs_mount"):
        if verbose:
            _rlog(f"{exec_node.get('label','?')}: lxc_nfs enabled "
                  f"but nfs/nfs_mount unset — skipping bring-up")
        return True, ""
    u, h, p, ha = _exec_ssh(exec_node)
    # Stable per-container MAC from lab storage (minted once, reused
    # forever). Key on the exec-node label so each lxc_nfs box keeps
    # its own MAC even though the container name is shared.
    mac = bh.lab_mac_for(exec_node.get("label") or exec_node.get("host"))
    rc, _o, e = ssh_run(u, h, p, _nfs_lxc_script(cfg, ha, "up", mac=mac),
                        timeout=300)
    tail = "\n".join(e.splitlines()[-20:]) if e.strip() else ""
    if rc != 0 and verbose:
        _rlog(f"{exec_node.get('label','?')}: nfs-lxc bring-up "
              f"rc={rc}: {tail}")
    return rc == 0, tail


def nfs_lxc_prepare(exec_node, verbose=False):
    """Bring up the NFS-LXC container (no-op for ordinary nodes) and
    return (runtime_node, msg). `runtime_node` targets the guest address
    with its lxc_nfs block cleared, so any later run_benchmark_on_exec
    won't try to re-bring-up. On an ordinary node runtime_node IS
    exec_node. On failure returns (None, msg).

    Caller MUST call nfs_lxc_teardown(<ORIGINAL exec_node>) in a finally
    block -- teardown needs the device pre-chroot address + lxc_nfs that
    only the original node carries."""
    ok, tail = nfs_lxc_bring_up(exec_node, verbose=verbose)
    if not ok:
        msg = "nfs-lxc bring-up failed"
        if tail:
            msg += "\n" + tail
        return None, msg
    rt = _runtime_exec_node(exec_node)
    if rt is not exec_node:
        if verbose:
            _rlog(f"{exec_node.get('label','?')}: lxc_nfs guest is "
                  f"{rt['user']}@{rt['host']}:{rt.get('port',22)} -- "
                  f"switching downstream rsync/ssh to the container")
        if not wait_for_ssh(rt["user"], rt["host"], int(rt.get("port", 22)),
                            verbose=verbose):
            return None, (f"nfs-lxc guest ssh not reachable at "
                          f"{rt['user']}@{rt['host']}")
    return rt, "ok"


def nfs_lxc_teardown(exec_node, verbose=False):
    cfg = _lxc_nfs_cfg(exec_node)
    if cfg is None:
        return
    if not cfg.get("nfs") or not cfg.get("nfs_mount"):
        return
    u, h, p, ha = _exec_ssh(exec_node)
    ssh_run(u, h, p, _nfs_lxc_script(cfg, ha, "down"), timeout=120)


def rsync_to(user, host, port, src, dst, timeout=RSYNC_TIMEOUT,
             extra_args=None, delete=True):
    """rsync local `src` -> remote `dst` (path on host). When `src` ends
    with '/', the contents are copied; otherwise the dir is created at
    dst. Returns (rc, stderr_message).

    `delete` (default True) mirrors src->dst with --delete so removals
    propagate -- correct for the datapack tree. MUST be False when
    pushing a single binary into a shared work_dir that already holds
    sibling files (datapack/, server-properties.xml): --delete would
    wipe everything not in the (binary-only) source and the server then
    can't find its datapack."""
    args = ["rsync", "-art"]
    if delete:
        args.append("--delete")
    args += ["--exclude=.git", "--exclude=/build/", "--exclude=*.o",
            "--exclude=moc_*", "--exclude=Makefile",
            "--exclude=__pycache__/", "--exclude=.qtcreator/",
            "-e", f"ssh -p {port} -o ConnectTimeout=10 -o BatchMode=yes -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR"]
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
            "-e", f"ssh -p {port} -o ConnectTimeout=10 -o BatchMode=yes -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR",
            f"{_rsync_host(user, host)}:{src}", dst]
    try:
        p = subprocess.run(args, capture_output=True, text=True,
                           timeout=timeout)
        return p.returncode, (p.stdout + p.stderr)
    except subprocess.TimeoutExpired:
        return -1, f"rsync timeout after {timeout}s"


# ---- time sync ------------------------------------------------------------

def sync_remote_time(user, host, port, timeout=15):
    """Check the remote node's clock; if it differs from local by >5 s,
    set it via `sudo date -s`.  Silent on match, logs on set.

    Returns True if time was acceptable (or was fixed), False if we
    couldn't check/set."""
    try:
        local_ts = int(time.time())
    except Exception:
        return False
    rc, sout, _serr = ssh_run(user, host, port, "date +%s", timeout=timeout)
    if rc != 0:
        return False
    try:
        remote_ts = int(sout.strip())
    except (ValueError, TypeError):
        return False
    diff = abs(local_ts - remote_ts)
    if diff <= 5:
        return True
    # time skew beyond threshold -- fix it
    set_cmd = f"sudo -n date -s '@{local_ts}' >/dev/null 2>&1"
    rc, _so, _se = ssh_run(user, host, port, set_cmd, timeout=timeout)
    if rc != 0:
        # try without sudo (e.g. already root or container with CAP_SYS_TIME)
        set_cmd = f"date -s '@{local_ts}' >/dev/null 2>&1"
        rc, _so, _se = ssh_run(user, host, port, set_cmd, timeout=timeout)
    if rc == 0:
        print(f"[tsync] {user}@{host}:{port} clock was {diff}s off, "
              f"corrected to {local_ts}", file=sys.stderr)
        return True
    print(f"[tsync] {user}@{host}:{port} clock is {diff}s off but "
          f"could not set time (rc={rc})", file=sys.stderr)
    return False


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
        _rlog(f"rsync {compile_node['label']!r}: rc={rc}")
        if rc != 0:
            print(msg, file=sys.stderr)
    return rc, msg


def build_on_compile_node(compile_node, cmake_src_subdir, build_subdir,
                          cmake_defs=None, target=None, verbose=False,
                          use_ninja=None):
    """cmake -S <work>/sources/<src_subdir> -B <work>/build-bench/<build_subdir>
    then cmake --build. Returns (rc, message, remote_bin_dir).

    Before building, syncs the remote clock (ninja refuses to build
    when system time is wrong -- 'manifest still dirty after 100
    tries'). A 5 s tolerance avoids pointless tsync spam.

    `use_ninja`:
      * None  (default) — auto-detect: use Ninja if it exists on the
        compile node, else fall back to Make.
      * True            — force Ninja (cmake -G Ninja).
      * False           — skip Ninja even when installed (use Make).
    """
    user = compile_node["ssh"]["user"]
    host = compile_node["ssh"]["host"]
    port = compile_node["ssh"].get("port", 22)
    work = compile_node["work_dir"]
    sync_remote_time(user, host, port)
    src  = f"{work}/sources/{cmake_src_subdir}"
    bld  = f"{work}/build-bench/{build_subdir}"

    defs = []
    if cmake_defs:
        for k, v in cmake_defs.items():
            # shlex.quote the whole -Dkey=value: values like
            # "-O2 -DNDEBUG" contain a space, and defs is later joined
            # with ' ' into a remote shell string -- without quoting the
            # remote shell splits it into two argv tokens and cmake reads
            # the "-DNDEBUG" half as a malformed cache-var define
            # ("Parse error in command line argument: NDEBUG").
            defs.append(shlex.quote(f"-D{k}={v}"))
    if use_ninja is None:
        ninja = "-G Ninja" if _have_remote_tool(user, host, port, "ninja") else ""
    elif use_ninja:
        ninja = "-G Ninja"
    else:
        ninja = ""
    # ccache when the compile node has it (project rule: accelerators
    # picked up when present, fallback otherwise). This path rsyncs a
    # fresh source tree per test, so WITHOUT ccache every remote build
    # is a full cold compile -- on the qemu-mips compile node that is
    # the exact tarpit that blew CMAKE_BUILD_TIMEOUT. ccache is
    # bit-identical to a plain compile so no measured metric changes.
    if _have_remote_tool(user, host, port, "ccache"):
        defs.append("-DCMAKE_C_COMPILER_LAUNCHER=ccache")
        defs.append("-DCMAKE_CXX_COMPILER_LAUNCHER=ccache")

    cfg_cmd = (f"mkdir -p {shlex.quote(bld)} && "
               f"cmake -S {shlex.quote(src)} -B {shlex.quote(bld)} "
               f"{ninja} -DCMAKE_BUILD_TYPE=Release {' '.join(defs)}")
    rc, sout, serr = ssh_run(user, host, port, cfg_cmd,
                             timeout=CMAKE_CONFIGURE_TIMEOUT)
    if verbose:
        _rlog(f"cmake configure {compile_node['label']!r}: rc={rc}")
    if rc != 0:
        return rc, f"cmake configure failed:\n{sout}\n{serr}", bld
    # Record which libs (system .so vs vendored copy) + version this
    # compile node's build resolved, parsed from the configure log. The
    # system-version probe (pkg-config) must run ON the compile node, so
    # pass an ssh shell rather than the local default. Keyed by
    # compile-node label; the caller aliases it onto the exec node.
    def _compile_shell(cmd, timeout=15):
        _rc, _out, _err = ssh_run(user, host, port, cmd, timeout=timeout)
        return _out
    bh.record_libs(compile_node["label"], sout, shell=_compile_shell)

    build_cmd = f"cmake --build {shlex.quote(bld)} -j$(nproc)"
    if target:
        build_cmd += f" --target {shlex.quote(target)}"
    rc, sout, serr = ssh_run(user, host, port, build_cmd,
                             timeout=CMAKE_BUILD_TIMEOUT)
    if verbose:
        _rlog(f"cmake build {compile_node['label']!r}: rc={rc}")
    if rc != 0:
        return rc, f"cmake build failed:\n{sout}\n{serr}", bld
    return 0, "ok", bld


def _have_remote_tool(user, host, port, tool):
    rc, sout, _ = ssh_run(user, host, port,
                          f"command -v {shlex.quote(tool)} >/dev/null && echo Y || echo N",
                          timeout=10)
    return rc == 0 and sout.strip() == "Y"


_QT6_PROBE_MEMO = {}   # compile_node label -> bool


def compile_node_has_qt6(compile_node):
    """True when the compile node can satisfy find_package(Qt6) -- a
    prerequisite for the GUI-linked benchmarks (bot-actions is Qt6
    Widgets). Probes for a qmake6/qmake binary OR a Qt6Config.cmake
    anywhere under the usual prefixes. Memoized per compile node.

    Some nodes carry has_gui=true in remote_nodes.json but no actual
    Qt6 dev files (it was uninstalled, or the flag is aspirational);
    a benchmark that needs Qt6 must SKIP there, not FAIL -- a missing
    toolchain is not a perf regression."""
    label = compile_node.get("label")
    if label in _QT6_PROBE_MEMO:
        return _QT6_PROBE_MEMO[label]
    user = compile_node["ssh"]["user"]
    host = compile_node["ssh"]["host"]
    port = compile_node["ssh"].get("port", 22)
    probe = ("command -v qmake6 >/dev/null 2>&1 && echo Y && exit 0; "
             "command -v qmake >/dev/null 2>&1 && echo Y && exit 0; "
             "for d in /usr/lib/*/cmake/Qt6 /usr/lib/cmake/Qt6 "
             "/usr/local/lib/cmake/Qt6 /opt/*/lib/*/cmake/Qt6 "
             "/opt/*/lib/cmake/Qt6; do "
             "test -f \"$d/Qt6Config.cmake\" && echo Y && exit 0; done; "
             "echo N")
    rc, sout, _ = ssh_run(user, host, port, probe, timeout=15)
    have = (rc == 0 and "Y" in sout.split())
    _QT6_PROBE_MEMO[label] = have
    return have


# ---- exec-node staging ---------------------------------------------------

# Media extensions a headless SERVER never needs (no sound, no rendering).
# Subtracted from CATCHCHALLENGER_EXTENSION_ALLOWED to get the data subset.
_DATAPACK_MEDIA_EXT = {"png", "jpg", "jpeg", "gif", "bmp", "webp", "ico",
                       "svg", "tga", "psd",                       # images
                       "ogg", "opus", "wav", "mp3", "mid", "midi",
                       "flac", "aac", "m4a", "mod", "xm", "s3m"}  # audio
# Fallback if GeneralVariable.hpp can't be read/parsed (keep the run working).
_DATAPACK_KEEP_EXT_FALLBACK = ["tmx", "xml", "tsx", "js"]
_DATAPACK_KEEP_EXT_CACHE = None


def datapack_keep_extensions():
    """The datapack extensions a headless server keeps, derived AT RUNTIME
    from CATCHCHALLENGER_EXTENSION_ALLOWED in
    general/base/GeneralVariable.hpp (the single source of truth for which
    extensions a datapack may contain) minus the media subset. Parsed once
    and memoized; falls back to the known data set if the header is
    unreadable. Updating the #define automatically updates this list."""
    global _DATAPACK_KEEP_EXT_CACHE
    if _DATAPACK_KEEP_EXT_CACHE is not None:
        return _DATAPACK_KEEP_EXT_CACHE
    keep = None
    hpp = os.path.join(REPO_ROOT, "general", "base", "GeneralVariable.hpp")
    try:
        with open(hpp, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                if "CATCHCHALLENGER_EXTENSION_ALLOWED" in line and "#define" in line:
                    q1 = line.find('"')
                    q2 = line.find('"', q1 + 1)
                    if q1 != -1 and q2 != -1:
                        allowed = [e.strip().lower()
                                   for e in line[q1 + 1:q2].split(";")
                                   if e.strip()]
                        keep = [e for e in allowed
                                if e not in _DATAPACK_MEDIA_EXT]
                    break
    except OSError as e:
        print(f"[remote] WARN: cannot read {hpp}: {e}", file=sys.stderr)
    if not keep:
        keep = list(_DATAPACK_KEEP_EXT_FALLBACK)
    _DATAPACK_KEEP_EXT_CACHE = keep
    return keep


def server_datapack_excludes():
    """rsync args for a SERVER datapack push (headless: data files only).
    Whitelist: keep the non-media allowed extensions
    (datapack_keep_extensions(), loaded from GeneralVariable.hpp at runtime),
    drop all else. Paired with rsync's --delete (always on for datapack) +
    --delete-excluded so already-pushed media/strays are pruned too, not just
    skipped.

    `--include=*/` keeps rsync descending into every directory, so the full
    tree is recreated even where a folder held only excluded files -- in
    particular skin/fighter/ survives as EMPTY folders, which a no-cache
    (XML-parse) boot enumerates to build the fighter-skin list. The trailing
    `--exclude=*` drops images (png/jpg/gif), audio (ogg/opus) and anything
    outside CATCHCHALLENGER_EXTENSION_ALLOWED in one rule."""
    # Prune .git before --include=*/ so the source datapack's git checkout
    # isn't recreated as hundreds of empty folders. First-match-wins.
    args = ["--delete-excluded", "--exclude=.git", "--exclude=.git/",
            "--include=*/"]
    for ext in datapack_keep_extensions():
        args.append(f"--include=*.{ext}")
    args.append("--exclude=*")
    return args


def rsync_datapack_to_exec(exec_node, local_datapack, remote_subdir="datapack",
                           timeout=600, server_mode=False):
    """rsync a local datapack directory to exec_node's work_dir/<remote_subdir>.

    Used by benchmarks that need the datapack on the exec node (serversave,
    botactions). Returns (rc, message).

    `server_mode` (default False) pushes the datapack verbatim. True strips
    audio/images via server_datapack_excludes() -- the server is headless and
    never needs them (see server/CLAUDE.md)."""
    eu  = exec_node["user"]
    eh  = exec_node["host"]
    ep  = exec_node.get("port", 22)
    ewd = exec_node["work_dir"]
    ssh_run(eu, eh, ep, f"mkdir -p {shlex.quote(ewd)}", timeout=15)
    dst = f"{ewd}/{remote_subdir}"
    extra = server_datapack_excludes() if server_mode else None
    return rsync_to(eu, eh, ep, local_datapack.rstrip("/") + "/", dst + "/",
                    timeout=timeout, extra_args=extra)


def _server_properties_xml(maincode, port, max_players=200):
    """The minimal server-properties.xml used by every server-running
    benchmark. Single source so the cache-gen copy (compile node) and
    the runtime copy (exec node) are byte-identical -- a difference in
    maincode would invalidate a pre-generated datapack cache."""
    return (
        '<?xml version="1.0"?>\n'
        '<configuration>\n'
        f'    <server-port value="{port}"/>\n'
        '    <automatic_account_creation value="true"/>\n'
        f'    <max-players value="{max_players}"/>\n'
        '    <httpDatapackMirror value=""/>\n'
        '    <master>\n'
        f'        <external-server-port value="{port}"/>\n'
        '    </master>\n'
        '    <content>\n'
        f'        <mainDatapackCode value="{maincode}"/>\n'
        '        <subDatapackCode value=""/>\n'
        '    </content>\n'
        '</configuration>\n'
    )


def write_server_properties_on_exec(exec_node, maincode, port, run_subdir=None,
                                    max_players=200):
    """Write a minimal server-properties.xml on the exec node.

    `run_subdir` defaults to exec_node['work_dir']."""
    eu  = exec_node["user"]
    eh  = exec_node["host"]
    ep  = exec_node.get("port", 22)
    ewd = run_subdir or exec_node["work_dir"]
    xml_content = _server_properties_xml(maincode, port, max_players)
    escaped = xml_content.replace("'", "'\\''")
    rc, _so, _se = ssh_run(eu, eh, ep,
                           f"mkdir -p {shlex.quote(ewd)} && "
                           f"printf '%s' '{escaped}' > "
                           f"{shlex.quote(ewd + '/server-properties.xml')}",
                           timeout=15)
    return rc


# ---- pre-generated datapack HPS cache ------------------------------------
# `<server> save` parses the XML datapack and writes datapack-cache.bin
# (HPS binary). On a normal boot the server only *loads* that cache when
# its mtime matches server-properties.xml (freshness check in
# server/cli/main-unix.cpp); otherwise it re-parses the XML -- minutes on
# a constrained MIPS/ARM box. Generate the cache ONCE on the compile node
# (same arch+libc as all its child exec nodes, so the binary cache is
# byte-valid on each) and ship it next to the binary: every exec node's
# server boot becomes an instant cache load instead of a full parse.

_CACHE_GEN_MEMO = {}   # compile_node label -> (rc, remote_cache_path, msg)


def pregenerate_datapack_cache(compile_node, bin_remote_dir, bin_name,
                               local_datapack, maincode, server_port=61920,
                               timeout=1800, verbose=False):
    """Run `<server> save` on the compile node, return (rc,
    remote_cache_path, msg). Memoized per compile node: the 2nd+ exec
    node sharing this compile node reuses the already-generated cache.

    The cache lands in a stable <work_dir>/cache-gen/ dir (NOT the
    per-exec build dir) so it is generated once and reused. Caller ships
    it to each exec node with stage_cache_on_exec()."""
    label = compile_node.get("label")
    if label in _CACHE_GEN_MEMO:
        return _CACHE_GEN_MEMO[label]
    user = compile_node["ssh"]["user"]
    host = compile_node["ssh"]["host"]
    port = compile_node["ssh"].get("port", 22)
    work = compile_node["work_dir"]
    gen  = f"{work}/cache-gen"
    cache_path = f"{gen}/datapack-cache.bin"

    if not local_datapack or not os.path.isdir(local_datapack):
        res = (1, None, f"local datapack missing: {local_datapack}")
        _CACHE_GEN_MEMO[label] = res
        return res

    ssh_run(user, host, port, f"mkdir -p {shlex.quote(gen)}", timeout=30)
    # `server save` parses the XML datapack (no cache yet) -> headless server
    # push: strip audio/images, keep skin/fighter as empty folders.
    rc, msg = rsync_to(user, host, port, local_datapack.rstrip("/") + "/",
                       f"{gen}/datapack/", timeout=timeout,
                       extra_args=server_datapack_excludes())
    if rc != 0:
        res = (rc, None, f"cache-gen datapack rsync failed: {msg[:120]}")
        _CACHE_GEN_MEMO[label] = res
        return res

    xml = _server_properties_xml(maincode, server_port)
    escaped = xml.replace("'", "'\\''")
    setup = (f"cp -f {shlex.quote(bin_remote_dir + '/' + bin_name)} "
             f"{shlex.quote(gen + '/' + bin_name)} && "
             f"chmod +x {shlex.quote(gen + '/' + bin_name)} && "
             f"printf '%s' '{escaped}' > {shlex.quote(gen + '/server-properties.xml')} && "
             f"rm -f {shlex.quote(cache_path)} {shlex.quote(cache_path + '.tmp')}")
    rc, _o, e = ssh_run(user, host, port, setup, timeout=120)
    if rc != 0:
        res = (rc, None, f"cache-gen setup failed: {e[:120]}")
        _CACHE_GEN_MEMO[label] = res
        return res

    save_cmd = f"cd {shlex.quote(gen)} && ./{bin_name} save"
    rc, _o, e = ssh_run(user, host, port, save_cmd, timeout=timeout)
    # save writes datapack-cache.bin.tmp then renames to .bin; tolerate
    # either in case the rename path differs by build.
    rc_chk, sout, _ = ssh_run(
        user, host, port,
        f"if test -f {shlex.quote(cache_path)}; then echo Y; "
        f"elif test -f {shlex.quote(cache_path + '.tmp')}; then "
        f"mv {shlex.quote(cache_path + '.tmp')} {shlex.quote(cache_path)} && echo Y; "
        f"else echo N; fi", timeout=30)
    if sout.strip() != "Y":
        res = (1, None, f"cache-gen save produced no cache (rc={rc}): {e[:160]}")
        _CACHE_GEN_MEMO[label] = res
        return res
    if verbose:
        _rlog(f"datapack cache pre-generated on {label!r}: {cache_path}")
    res = (0, cache_path, "ok")
    _CACHE_GEN_MEMO[label] = res
    return res


def stage_cache_on_exec(compile_node, exec_node, remote_cache_path,
                        verbose=False):
    """Pull the pre-generated datapack-cache.bin from the compile node and
    push it next to the binary in the exec node's work_dir, then set its
    mtime to match server-properties.xml so the server's freshness check
    accepts it (main-unix.cpp loads the cache only when cache.mtime ==
    xml.mtime). Returns (rc, msg). server-properties.xml MUST already be
    written on the exec node (write_server_properties_on_exec)."""
    staging = os.path.join("/tmp",
                           f"cc-bench-cache-{exec_node['label']}-{int(time.time())}")
    os.makedirs(staging, exist_ok=True)
    cu = compile_node["ssh"]["user"]
    ch = compile_node["ssh"]["host"]
    cp = compile_node["ssh"].get("port", 22)
    rc, msg = rsync_from(cu, ch, cp, remote_cache_path, staging + "/")
    if rc != 0:
        return rc, f"cache pull from compile node failed: {msg[:120]}"

    eu  = exec_node["user"]
    eh  = exec_node["host"]
    ep  = exec_node.get("port", 22)
    ewd = exec_node["work_dir"]
    local_cache = os.path.join(staging, "datapack-cache.bin")
    if not os.path.isfile(local_cache):
        return 1, "cache file absent after pull"
    rc, msg = rsync_to(eu, eh, ep, local_cache, f"{ewd}/datapack-cache.bin")
    if rc != 0:
        return rc, f"cache push to exec node failed: {msg[:120]}"
    rc, _o, e = ssh_run(
        eu, eh, ep,
        f"touch -r {shlex.quote(ewd + '/server-properties.xml')} "
        f"{shlex.quote(ewd + '/datapack-cache.bin')}", timeout=15)
    if rc != 0:
        return rc, f"cache mtime match failed: {e[:120]}"
    if verbose:
        _rlog(f"{exec_node['label']!r}: staged pre-generated cache "
              f"+ matched mtime to server-properties.xml")
    return 0, "ok"


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
                _rlog(f"WARN: extra {e!r} pull rc={rc}: {msg}", err=True)

    # 2) push to exec node work_dir
    eu = exec_node["user"]
    eh = exec_node["host"]
    ep = exec_node.get("port", 22)
    ewd = exec_node["work_dir"]
    ssh_run(eu, eh, ep,
            f"mkdir -p {shlex.quote(ewd)}",
            timeout=30)
    # delete=False: work_dir already holds the datapack + server-properties.xml
    # staged by earlier steps; a mirroring --delete here would wipe them and
    # the server would fail with "Datapack directory missing".
    rc, msg = rsync_to(eu, eh, ep, staging + "/", f"{ewd}/", delete=False)
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
    """Run timed `cmd_str` on the exec node and parse rusage.

    Prefers `/usr/bin/time -v` (GNU time, gives RSS + ctx-switches).
    Falls back to bash TIMEFORMAT when the binary is absent — only
    wall/user/sys are available in that case; RSS fields remain None.
    Returns the same dict shape as bh.measure_time_v()."""
    out = {"wall_s": None, "user_s": None, "sys_s": None,
           "max_rss_kb": None, "vol_ctx": None, "invol_ctx": None,
           "minor_pf": None, "major_pf": None, "rc": None, "error": None,
           "stdout": None}

    # Detect /usr/bin/time on the remote node once per call (cheap SSH).
    have_time_v = _remote_have_tool_cached(exec_node, "/usr/bin/time")

    if have_time_v:
        full = f"/usr/bin/time -v sh -c {shlex.quote(cmd_str)}"
        rc, _sout, serr = run_remote_cmd(exec_node, full, timeout=timeout)
        out["rc"] = rc
        out["stdout"] = _sout
        if rc != 0:
            out["error"] = serr.strip()[:500] if serr else f"exit code {rc}"
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
    else:
        # Fallback: bash TIMEFORMAT gives wall/user/sys; RSS not available.
        # TIMEFORMAT uses %R=real %U=user %S=sys in seconds with decimals.
        marker = "BASHTIME:"
        bash_cmd = (f'TIMEFORMAT="{marker}%R %U %S"; '
                    f'{{ time sh -c {shlex.quote(cmd_str)}; }} 2>&1')
        rc, combined, _ = run_remote_cmd(exec_node, bash_cmd, timeout=timeout)
        out["rc"] = rc
        lines = combined.splitlines()
        stdout_lines = []
        for line in lines:
            if line.startswith(marker):
                parts = line[len(marker):].split()
                if len(parts) >= 3:
                    try: out["wall_s"] = float(parts[0])
                    except: pass
                    try: out["user_s"] = float(parts[1])
                    except: pass
                    try: out["sys_s"] = float(parts[2])
                    except: pass
            else:
                stdout_lines.append(line)
        out["stdout"] = "\n".join(stdout_lines)
        if rc != 0:
            out["error"] = f"exit code {rc}"
    return out


# Cache for per-exec-node /usr/bin/time probe so remote_time_v doesn't
# add a round-trip on every call within a batch.
_remote_time_v_cache = {}


def _remote_have_tool_cached(exec_node, tool):
    key = (exec_node.get("host"), exec_node.get("port", 22), tool)
    if key not in _remote_time_v_cache:
        eu = exec_node["user"]
        eh = exec_node["host"]
        ep = exec_node.get("port", 22)
        rc, sout, _ = ssh_run(eu, eh, ep,
                               f"command -v {shlex.quote(tool)} >/dev/null 2>&1 "
                               f"&& echo Y || echo N", timeout=10)
        _remote_time_v_cache[key] = (rc == 0 and sout.strip() == "Y")
    return _remote_time_v_cache[key]


def _perf_no_hw_sampling(serr):
    """True when perf's stderr signals the node's PMU can't sample
    hardware events -- sys_perf_event_open EINVAL on a counter-only PMU
    (no IRQ), as on the armv6_1176 (RPi Zero W). Used both to trigger the
    cpu-clock software-event fallback and to downgrade a final failure to
    SKIP (node limitation) instead of FAIL (regression)."""
    s = (serr or "").lower()
    return ("sys_perf_event_open" in s
            or "returned with 22" in s
            or "sampling events not supported" in s
            or "no irqs for pmu" in s)


def _perf_paranoid_blocked(serr):
    """True when perf's stderr signals kernel.perf_event_paranoid (or a
    missing CAP_PERFMON) denies the user access -- e.g. the atom-n455
    'Access to performance monitoring and observability operations is
    limited.' message. We don't own these nodes, so we won't sysctl
    them: such a run is SKIPped (missing datum), not FAILed."""
    s = (serr or "").lower()
    return ("perf_event_paranoid" in s
            or "access to performance monitoring" in s
            or "permission denied" in s and "perf" in s)


def remote_perf_stat(exec_node, cmd_str, timeout=RUN_TIMEOUT_DEFAULT):
    """Run `perf stat -x , -e <events> <cmd>` on the exec node, parse
    the CSV output. Returns event->value dict or None when perf is
    unavailable / kernel.perf_event_paranoid blocks the user.

    Mirrors measure_perf_stat() in benchmark_helpers: a non-zero rc
    from the wrapped command (timeout 124, SIGINT-exit 130) does not
    invalidate the counters perf already wrote to stderr -- parse
    them regardless and fail only on empty / unparseable output.
    Wrap cmd_str in a shell so the cd; chmod; ./bin chaining works."""
    full = ("perf stat -x , "
            "-e cycles,instructions,branch-misses,cache-misses "
            f"sh -c {shlex.quote(cmd_str)}")
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


def remote_callgrind(exec_node, bin_cmd, work_dir, timeout=None,
                     toggle_collect=None, setup_prefix=None):
    """Run callgrind on the exec node and return instruction count.
    The output file lives in <work_dir>/callgrind.out -- it is parsed
    in-band on the remote with callgrind_annotate to avoid pulling the
    profile back. On failure returns dict with rc and error fields.

    bin_cmd is the bare binary invocation (e.g. './benchmark_min_network
    --ticks 200 ...') run DIRECTLY under valgrind -- NOT wrapped in
    `sh -c`. Wrapping in sh -c made valgrind profile the shell, which then
    fork-exec'd the real binary as an untraced child (default
    --trace-children=no): the profile captured only sh's loader (~hundreds
    of KB of dynamic-linking) and the toggle never fired -> empty -> FAIL.
    setup_prefix (e.g. 'cd <wd> && chmod +x <bin>') runs in the ssh shell
    BEFORE valgrind so cwd/relative-datapack resolve, but valgrind itself
    execs the binary directly (same PID, traced).

    toggle_collect: a function-name glob (e.g. '*min_network*'). When set,
    adds --collect-atstart=no --toggle-collect=<glob> so only that function
    (+ callees) is counted, excluding process startup. Header-free: valgrind
    resolves the glob against the binary's symbols at runtime, so it works
    on every node regardless of cross-sysroot valgrind headers."""
    if timeout is None:
        timeout = RUN_TIMEOUT_DEFAULT * 30   # callgrind is ~30x slower
    out_file = f"{work_dir}/callgrind.out"
    err_file = f"{work_dir}/callgrind.vgerr"
    prefix = (setup_prefix + " && ") if setup_prefix else ""
    toggle = ("" if not toggle_collect
              else f"--collect-atstart=no --toggle-collect={shlex.quote(toggle_collect)} ")
    # Drop --quiet so valgrind's own diagnostics (missing tool, unhandled
    # instruction, "Collected: 0") land in err_file and can be surfaced on
    # failure — a bare [FAIL] with no reason is undebuggable. valgrind's
    # exit status is captured separately so an empty/short profile reports
    # WHY rather than vanishing into a generic failure.
    full = (
        f"command -v valgrind >/dev/null 2>&1 || {{ echo CC_NO_VALGRIND; exit 0; }}; "
        f"rm -f {shlex.quote(out_file)} {shlex.quote(err_file)}; "
        f"{prefix}"
        f"valgrind --tool=callgrind {toggle}"
        f"--callgrind-out-file={shlex.quote(out_file)} "
        f"{bin_cmd} 2>{shlex.quote(err_file)}; "
        f"echo CC_VGRC=$?; "
        f"callgrind_annotate {shlex.quote(out_file)} 2>>{shlex.quote(err_file)} "
        f"| grep 'PROGRAM TOTALS'; "
        f"echo CC_VGERR_TAIL:; tail -6 {shlex.quote(err_file)} 2>/dev/null")
    rc, sout, serr = run_remote_cmd(exec_node, full, timeout=timeout)
    if "CC_NO_VALGRIND" in (sout or ""):
        return {"rc": 127, "error": "valgrind not installed on node "
                "(setup.py should install it; node may have failed that step)"}
    if rc != 0:
        tail = (serr or sout or "").strip()[:500]
        return {"rc": rc, "error": tail or f"exit code {rc}"}
    ir = None
    vgrc = None
    vgerr_lines = []
    in_tail = False
    for line in (sout or "").splitlines():
        s = line.strip()
        if s.startswith("CC_VGRC="):
            try: vgrc = int(s.split("=", 1)[1])
            except ValueError: pass
        elif s == "CC_VGERR_TAIL:":
            in_tail = True
        elif in_tail:
            if s: vgerr_lines.append(s)
        elif "PROGRAM TOTALS" in s:
            try: ir = int(s.split()[0].replace(",", ""))
            except (ValueError, IndexError): ir = None
    # ~hundreds of Ir = crash-prologue-only / nothing collected; a real
    # workload is millions. Surface the valgrind diagnostic on failure.
    if ir is not None and ir >= 100000:
        return ir
    reason = " | ".join(vgerr_lines[-3:]) or "no PROGRAM TOTALS in annotate output"
    hint = ""
    if (ir is None or ir == 0) and toggle_collect:
        hint = (" — collected ~0 Ir; --toggle-collect glob may match no symbol "
                "in this binary (stripped? inlined?)")
    return {"rc": vgrc if vgrc is not None else 0,
            "error": f"callgrind Ir={ir}: {reason}{hint}"[:500]}


def remote_nproc(exec_node):
    """Core count of the exec node (`nproc`), or None. Feeds the
    '<node>-CPU-<cores>' tag in profile artefact filenames."""
    rc, sout, _ = run_remote_cmd(exec_node, "nproc", timeout=30)
    if rc == 0 and sout:
        try:
            return int(sout.strip().splitlines()[0])
        except (ValueError, IndexError):
            return None
    return None


def exec_node_dict(node):
    """Normalise a bh.benchmark_exec_nodes() entry (ssh_user/ssh_host/...)
    into the {label,user,host,port,work_dir,lxc_nfs,ninja} shape the
    remote helpers (push_binary_to_exec, nfs_lxc_prepare, ...) expect."""
    return {"label":    node["label"],
            "user":     node.get("ssh_user"),
            "host":     node.get("ssh_host"),
            "port":     node.get("ssh_port", 22),
            "work_dir": node.get("work_dir") or f"/tmp/cc-bench-{node['label']}",
            "lxc_nfs":  node.get("lxc_nfs"),
            "ninja":    node.get("ninja")}


def profile_on_exec(compile_node, exec_node, cmake_src_subdir, build_subdir,
                    bin_name, runtime_cmd, tools, bench_name,
                    cmake_defs=None, extras=None, local_dest_dir=None,
                    stage_fn=None, run_timeout=RUN_TIMEOUT_DEFAULT,
                    verbose=False, callgrind_toggle=None):
    """--profile on ONE exec node: build on its compile node, push the
    binary, optionally stage runtime fixtures (`stage_fn(runtime_node)` —
    e.g. rsync datapack + write server-properties.xml), run each profiler
    in `tools` ('callgrind' / 'perf') there, and rsync each artefact back
    to local_dest_dir named '<tool-base>.<bench>-<exec_label>-CPU-<cores>
    .<stamp>'. A node missing the profiler tool is SKIPped (not failed).
    Returns a list of (tool, local_artefact_path_or_None, status_msg)."""
    import datetime
    if local_dest_dir is None:
        local_dest_dir = bh.tmpfs_scratch_dir("cc-bench-profile")
    rc, msg, bld = build_for_fleet(compile_node, cmake_src_subdir, build_subdir,
                                   cmake_defs=cmake_defs,
                                   use_ninja=exec_node.get("ninja"),
                                   verbose=verbose)
    if rc != 0:
        return [(t, None, f"build failed: {msg}") for t in tools]
    runtime_node, msg = nfs_lxc_prepare(exec_node, verbose=verbose)
    if runtime_node is None:
        nfs_lxc_teardown(exec_node, verbose=verbose)
        return [(t, None, f"bring-up failed: {msg}") for t in tools]
    results = []
    try:
        if stage_fn is not None:
            ok, smsg = stage_fn(runtime_node)
            if not ok:
                return [(t, None, f"stage failed: {smsg}") for t in tools]
        rc, exec_bin, msg = push_binary_to_exec(
            compile_node, runtime_node, bld, bin_name, extras=extras,
            verbose=verbose)
        if rc != 0:
            return [(t, None, f"push failed: {msg}") for t in tools]
        ewd = runtime_node["work_dir"]
        cores = remote_nproc(runtime_node)
        tag = bh.profile_node_tag(exec_node["label"], cores)
        eu = runtime_node["user"]
        eh = runtime_node["host"]
        ep = runtime_node.get("port", 22)
        base = (f"cd {shlex.quote(ewd)} && "
                f"chmod +x {shlex.quote(bin_name)} && ")
        for t in tools:
            stamp = datetime.datetime.utcnow().strftime("%Y%m%dT%H-%M-%SZ")
            cmd_str = (runtime_cmd.get(t) or runtime_cmd.get("rusage")
                       if isinstance(runtime_cmd, dict) else runtime_cmd)
            if t == "callgrind":
                if not _remote_have_tool_cached(runtime_node, "valgrind"):
                    results.append((t, None, "SKIP: valgrind missing on node"))
                    continue
                rfile = f"{ewd}/callgrind.out"
                toggle = ("" if not callgrind_toggle
                          else f"--collect-atstart=no --toggle-collect={shlex.quote(callgrind_toggle)} ")
                # Run the binary DIRECTLY under valgrind (cd already done by
                # `base`). NOT via `sh -c {cmd}`: that profiles the shell,
                # which fork-exec's the binary as an untraced child, so the
                # profile captures only sh's loader and the toggle never
                # fires -> empty artefact.
                full = base + (f"rm -f {shlex.quote(rfile)} && "
                               f"valgrind --tool=callgrind --quiet {toggle}"
                               f"--callgrind-out-file={shlex.quote(rfile)} "
                               f"{cmd_str}")
                tmo = run_timeout * 30
                local_name = f"callgrind.out.{bench_name}-{tag}.{stamp}"
            else:  # perf
                if not _remote_have_tool_cached(runtime_node, "perf"):
                    results.append((t, None, "SKIP: perf missing on node"))
                    continue
                rfile = f"{ewd}/perf.data"
                # DWARF call graph (not -g/frame-pointer) so stacks resolve
                # on i386/armv6; binary run directly (no sh -c frame noise).
                full = base + (f"rm -f {shlex.quote(rfile)} && "
                               f"perf record --call-graph=dwarf -o {shlex.quote(rfile)} "
                               f"{cmd_str}")
                tmo = run_timeout * 2
                local_name = f"perf.data.{bench_name}-{tag}.{stamp}"
            rc, _sout, serr = run_remote_cmd(runtime_node, full, timeout=tmo)
            if rc != 0 and t == "perf" and _perf_no_hw_sampling(serr):
                # Some PMUs (e.g. armv6_1176 on the RPi Zero W) count but
                # raise no IRQ, so perf's default hardware 'cycles' event
                # can't sample: sys_perf_event_open returns EINVAL (22).
                # Retry with the software 'cpu-clock' event (hrtimer-based,
                # needs no PMU interrupt) so we still get a usable profile.
                full = base + (f"rm -f {shlex.quote(rfile)} && "
                               f"perf record --call-graph=dwarf -e cpu-clock "
                               f"-o {shlex.quote(rfile)} "
                               f"{cmd_str}")
                rc, _sout, serr = run_remote_cmd(runtime_node, full,
                                                 timeout=tmo)
            if rc != 0:
                if t == "perf" and _perf_no_hw_sampling(serr):
                    # No hardware sampling and the software-event fallback
                    # also failed -- a node limitation, not a regression.
                    results.append((t, None,
                                    "SKIP: perf hw sampling unsupported on node"))
                elif t == "perf" and _perf_paranoid_blocked(serr):
                    # kernel.perf_event_paranoid too high for this user and
                    # we won't sysctl a node we don't own -- missing datum,
                    # not a regression.
                    results.append((t, None,
                                    "SKIP: perf_event_paranoid blocks access on node"))
                else:
                    results.append((t, None,
                                    f"run rc={rc}: {(serr or '').strip()[:200]}"))
                continue
            local_path = os.path.join(local_dest_dir, local_name)
            rc2, m2 = rsync_from(eu, eh, ep, rfile, local_path)
            if rc2 != 0:
                results.append((t, None, f"pull failed: {m2}"))
            else:
                results.append((t, local_path, "ok"))
        return results
    finally:
        nfs_lxc_teardown(exec_node, verbose=verbose)


def profile_fleet(bench_name, tools, local_cmd, local_cwd, local_timeouts,
                  remote_spec=None, verbose=True):
    """Drive --profile across the WHOLE fleet: the local host AND every
    benchmark exec node (benchmark=true, loadavg<1.0). One artefact per
    (node, tool), filename-tagged '<node>-CPU-<cores>' so nothing collides.

    local_cmd       argv for the local one-shot profile (None to skip local).
    local_timeouts  {tool: seconds} for the local run.
    remote_spec     None  => remote skipped (e.g. botactions: its server+bot
                            workload isn't remote-wired, local-only by design).
                    dict  => {cmake_src_subdir, build_subdir_base, bin_name,
                            runtime_cmd, cmake_defs?, extras?, stage_fn?}.
                    stage_fn(runtime_node) -> (ok, msg) stages per-benchmark
                            runtime fixtures on the exec node before the run.

    Returns list of (node_label, tool, artefact_path_or_None, msg)."""
    out = []
    if local_cmd is not None:
        for t in tools:
            p = bh.profile_once(local_cmd, t, cwd=local_cwd,
                                timeout=(local_timeouts or {}).get(t),
                                node_label="local", cpu_cores=os.cpu_count())
            out.append(("local", t, p, "ok" if p else "fail/missing-tool"))
    if remote_spec is None:
        print(f"{bh.C_YELLOW}[profile] {bench_name}: remote nodes not wired "
              f"for this benchmark — local only{bh.C_RESET}")
        return out
    nodes = bh.benchmark_exec_nodes()
    if not nodes:
        print(f"{bh.C_YELLOW}[profile] {bench_name}: no benchmark exec node "
              f"available (benchmark=true AND loadavg<1.0){bh.C_RESET}")
        return out
    for node in nodes:
        label = node["label"]
        cn = node.get("compile_node")
        if cn is None:
            print(f"{bh.C_YELLOW}[profile] {label}: no compile node; "
                  f"skip{bh.C_RESET}")
            for t in tools:
                out.append((label, t, None, "no compile node"))
            continue
        en = exec_node_dict(node)
        sub = f"{remote_spec['build_subdir_base']}-profile-{cn.get('label', 'cn')}"
        res = profile_on_exec(
            cn, en, remote_spec["cmake_src_subdir"], sub,
            remote_spec["bin_name"], remote_spec["runtime_cmd"], tools,
            bench_name, cmake_defs=remote_spec.get("cmake_defs"),
            extras=remote_spec.get("extras"),
            stage_fn=remote_spec.get("stage_fn"), verbose=verbose,
            callgrind_toggle=remote_spec.get("callgrind_toggle"))
        for (t, path, msg) in res:
            out.append((label, t, path, msg))
            if path:
                print(f"{bh.C_GREEN}[profile] {label} {t}: {path}{bh.C_RESET}")
            else:
                print(f"{bh.C_YELLOW}[profile] {label} {t}: {msg}{bh.C_RESET}")
    return out


def start_server_popen(exec_node, bin_name):
    """Launch the server on the exec node in the FOREGROUND over a dedicated
    SSH Popen.  Python holds the connection open; no shell job-control, no
    exec/setsid/nohup required on the remote.

    Returns (ssh_proc, pid_str, reason) where ssh_proc is the live Popen and
    pid_str is the remote server PID (obtained via a second SSH call).
    On success reason is "ok".  On failure returns (None, None, reason) where
    reason classifies the cause (e.g. an illegal-instruction crash on a CPU
    that lacks an instruction set the binary was built for).  Caller must
    eventually call stop_server_popen(ssh_proc, exec_node, pid_str)."""
    eu  = exec_node["user"]
    eh  = exec_node["host"]
    ep  = exec_node.get("port", 22)
    ewd = exec_node["work_dir"]

    # Kill any port-conflict leftovers from a previous run.  Match on the
    # full command line (-f), NOT -x: the binary name is 26 chars and the
    # kernel truncates the process comm to 15 (TASK_COMM_LEN), so an -x
    # exact-comm match never fires for it.
    ssh_run(eu, eh, ep,
            f"pkill -f {shlex.quote('./' + bin_name)} 2>/dev/null; true",
            timeout=10)

    # Connection 1: run server — stdin /dev/null, output to server.log.  The
    # remote shell backgrounds the server, records its PID to server.pid, then
    # `wait`s on it, so the SSH connection lifetime tracks the server.  Python
    # keeps this Popen alive; closing it disconnects sshd which SIGHUPs the
    # remote shell.  We read the PID from server.pid (reliable regardless of
    # the binary's comm-name length, unlike pgrep -x).
    #
    # The cd/chmod/rm prep MUST be synchronous and finish before the launch:
    # the `&` only backgrounds the single `./bin` command inside the braced
    # group, NOT the whole `&&` chain.  Earlier this read
    #   cd … && rm -f server.pid && ./bin … & echo $! >server.pid
    # where the trailing `&` made the ENTIRE `&&` list async — so `rm -f
    # server.pid` raced the foreground `echo $! >server.pid` and frequently
    # deleted the pid file *after* it was written.  The server bound fine but
    # the harness saw no pid and reported a bogus server-start-failed.  Bracing
    # the launch keeps `rm` synchronous and captures the server's own pid.
    argv = ["ssh", "-o", "ConnectTimeout=10", "-o", "BatchMode=yes",
            "-o", "ServerAliveInterval=30",
            "-o", "StrictHostKeyChecking=no",
            "-o", "UserKnownHostsFile=/dev/null",
            "-o", "LogLevel=ERROR",
            "-p", str(ep), _ssh_host(eu, eh),
            f"cd {shlex.quote(ewd)} && chmod +x {shlex.quote(bin_name)} && "
            f"rm -f server.log server.pid server.rc && "
            f"{{ ./{bin_name} </dev/null >server.log 2>&1 & "
            f"echo $! >server.pid; wait $!; echo $? >server.rc; }}"]
    proc = subprocess.Popen(argv, stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL)

    # Connection 2: read the PID written by the launch shell — give the remote
    # a moment to start and create server.pid.  Fall back to `pgrep -f` if the
    # file is empty/missing: the remote login shell may be a sh variant whose
    # `$!`/async-list semantics differ, and a server that actually bound must
    # be detected regardless.  pgrep -f matches the full command line (the
    # 26-char binary name is truncated to 15 in comm, so -x can't be used).
    pid = None
    deadline = time.monotonic() + 8
    while time.monotonic() < deadline:
        time.sleep(0.5)
        rc, sout, _ = ssh_run(
            eu, eh, ep,
            f"cat {shlex.quote(ewd + '/server.pid')} 2>/dev/null", timeout=10)
        cand = sout.strip() if rc == 0 else ""
        if cand.isdigit():
            pid = cand
            break
        rc2, sout2, _ = ssh_run(
            eu, eh, ep,
            f"pgrep -f {shlex.quote('./' + bin_name)} 2>/dev/null | head -n1",
            timeout=10)
        cand2 = sout2.strip() if rc2 == 0 else ""
        if cand2.isdigit():
            pid = cand2
            break
    if not pid or not pid.isdigit():
        proc.terminate()
        proc.wait()
        return None, None, _classify_server_start_failure(exec_node)
    return proc, pid, "ok"


def _missing_baseline_isa(exec_node):
    """Read the exec node's /proc/cpuinfo flags and return a comma-separated
    list of the common x86 baseline ISA extensions the CPU lacks (in the
    order a too-modern -march would emit them).  Empty string when nothing
    relevant is missing or the arch is non-x86 (no flags line / different
    feature names).  Pentium III reports no 'sse2'; an i486 reports no
    'cmov'; an Atom/Geode may lack 'ssse3'/'sse4_1'."""
    eu  = exec_node["user"]
    eh  = exec_node["host"]
    ep  = exec_node.get("port", 22)
    _, flags_txt, _ = ssh_run(
        eu, eh, ep,
        "grep -m1 '^flags' /proc/cpuinfo 2>/dev/null | cut -d: -f2-",
        timeout=10)
    flags = set(flags_txt.split())
    if not flags:
        return ""
    # Order matters: report the lowest missing tier first — that is the one a
    # rebuild must target.  Names match /proc/cpuinfo (sse4_1, not sse4.1).
    missing = []
    i = 0
    while i < len(_X86_BASELINE_ISA):
        name = _X86_BASELINE_ISA[i]
        if name not in flags:
            missing.append(name.upper().replace("_", "."))
        i += 1
    return ", ".join(missing)


_X86_BASELINE_ISA = ["cmov", "sse", "sse2", "ssse3", "sse4_1", "sse4_2"]


def _classify_server_start_failure(exec_node):
    """Read the remote server.log / server.rc after a failed launch and turn
    it into a human-readable reason.  The common failure on old x86 nodes
    (pentium3, pentium-m) is the binary using an instruction the CPU lacks
    (SSE2/SSSE3/CMOV emitted by a too-modern -march) — the kernel kills it
    with SIGILL and bash prints 'Illegal instruction'.  Surface that instead
    of a bare 'rc=-1' so the operator knows it is a build-arch mismatch, not
    a benchmark bug."""
    eu  = exec_node["user"]
    eh  = exec_node["host"]
    ep  = exec_node.get("port", 22)
    ewd = exec_node["work_dir"]
    _, rc_txt, _ = ssh_run(
        eu, eh, ep, f"cat {shlex.quote(ewd + '/server.rc')} 2>/dev/null",
        timeout=10)
    _, tail, _ = ssh_run(
        eu, eh, ep,
        f"tail -n 3 {shlex.quote(ewd + '/server.log')} 2>/dev/null "
        f"| tr '\\n' '|'", timeout=10)
    rc_txt = rc_txt.strip()
    log    = tail.strip().strip("|")
    # 128+SIGILL(4)=132 ; bash/coreutils also print the text below.
    if rc_txt == "132" or "llegal instruction" in log:
        missing = _missing_baseline_isa(exec_node)
        if missing:
            return (f"illegal-instruction: missing {missing} — binary built "
                    f"for a too-modern -march; rebuild for this arch")
        return ("illegal-instruction: binary uses a CPU instruction this "
                "host lacks (SSE2/SSSE3/CMOV from a too-modern -march) — "
                "rebuild for this arch")
    if rc_txt and rc_txt.isdigit() and int(rc_txt) > 128:
        sig = int(rc_txt) - 128
        return (f"server-killed-by-signal-{sig}"
                + (f": {log[:120]}" if log else ""))
    if log:
        return f"server-start-failed: {log[:140]}"
    return "server-start-failed: no server.log (binary missing or chmod failed)"


def stop_server_popen(ssh_proc, exec_node, pid):
    """Kill the remote server started by start_server_popen and close the
    SSH connection."""
    if pid:
        eu  = exec_node["user"]
        eh  = exec_node["host"]
        ep  = exec_node.get("port", 22)
        ssh_run(eu, eh, ep, f"kill {pid} 2>/dev/null; true", timeout=10)
    if ssh_proc and ssh_proc.poll() is None:
        ssh_proc.terminate()
        try:
            ssh_proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            ssh_proc.kill()
            ssh_proc.wait()


def measure_remote_server_boot(exec_node, bin_name, ready_timeout=120,
                               poll_s=0.5, verbose=False):
    """Start the server on the exec node, time wall-seconds from launch
    until 'correctly bind:' appears in server.log, then stop it. Returns
    (boot_s | None, msg).

    Uses start_server_popen: no exec/setsid/nohup needed on the remote.
    boot_s is None on crash-before-bind or timeout."""
    eu  = exec_node["user"]
    eh  = exec_node["host"]
    ep  = exec_node.get("port", 22)
    ewd = exec_node["work_dir"]

    t0 = time.monotonic()
    ssh_proc, pid, start_reason = start_server_popen(exec_node, bin_name)
    if ssh_proc is None:
        return None, start_reason
    boot_s = None
    try:
        deadline = time.monotonic() + ready_timeout
        while time.monotonic() < deadline:
            time.sleep(poll_s)
            rc, out, _ = ssh_run(
                eu, eh, ep,
                f"grep -q 'correctly bind:' {shlex.quote(ewd + '/server.log')} "
                f"2>/dev/null && echo Y || echo N", timeout=10)
            if rc == 0 and out.strip() == "Y":
                boot_s = time.monotonic() - t0
                break
            rc2, alive, _ = ssh_run(
                eu, eh, ep,
                f"kill -0 {pid} 2>/dev/null && echo Y || echo N", timeout=10)
            if rc2 == 0 and alive.strip() == "N":
                _, tail, _ = ssh_run(
                    eu, eh, ep,
                    f"tail -n 3 {shlex.quote(ewd + '/server.log')} 2>/dev/null "
                    f"| tr '\\n' '|'", timeout=10)
                reason = (tail.strip().strip("|") or "no server.log")[:160]
                return None, f"server-exited-before-bind: {reason}"
    finally:
        stop_server_popen(ssh_proc, exec_node, pid)
    if boot_s is None:
        return None, f"not-ready-in-{ready_timeout}s"
    if verbose:
        _rlog(f"{exec_node.get('label','?')}: server bound in {boot_s:.2f}s")
    return boot_s, "ok"


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


# ---- memory fragmentation / interrupt metrics ----------------------------

def remote_sys_snapshot(exec_node):
    """Capture a one-shot system snapshot from the exec node.

    Returns a dict with keys:
      buddyinfo    -- raw text of /proc/buddyinfo (str or None)
      pagetypeinfo -- raw text of /proc/pagetypeinfo (str or None)
      interrupts   -- dict {irq_label: int count} parsed from /proc/interrupts
                      (summed across all CPUs; None on failure)

    This is cheap (read-only procfs), call it before and after a run to
    compute interrupt deltas.
    """
    out = {"buddyinfo": None, "pagetypeinfo": None, "interrupts": None}
    # buddyinfo + pagetypeinfo in one SSH call
    rc, sout, _ = run_remote_cmd(
        exec_node,
        "echo '===buddyinfo==='; cat /proc/buddyinfo 2>/dev/null; "
        "echo '===pagetypeinfo==='; cat /proc/pagetypeinfo 2>/dev/null",
        timeout=10)
    if rc == 0:
        buddy_lines = []
        page_lines = []
        section = None
        for line in sout.splitlines():
            if line == "===buddyinfo===":
                section = "buddy"
            elif line == "===pagetypeinfo===":
                section = "page"
            elif section == "buddy":
                buddy_lines.append(line)
            elif section == "page":
                page_lines.append(line)
        if buddy_lines:
            out["buddyinfo"] = "\n".join(buddy_lines)
        if page_lines:
            out["pagetypeinfo"] = "\n".join(page_lines)
    # /proc/interrupts — sum all CPU columns per row, key on the IRQ label
    rc2, sout2, _ = run_remote_cmd(
        exec_node, "cat /proc/interrupts 2>/dev/null", timeout=10)
    if rc2 == 0:
        irq_map = {}
        lines = sout2.splitlines()
        ncpus = 1
        if lines:
            # header: "           CPU0  CPU1 ..."
            ncpus = len(lines[0].split())
        for line in lines[1:]:
            parts = line.split()
            if len(parts) < 2:
                continue
            # IRQ number/name is first field (strip trailing colon)
            irq_key = parts[0].rstrip(":")
            # sum numeric fields (CPU columns)
            total = 0
            for col in parts[1:1 + ncpus]:
                try:
                    total += int(col)
                except ValueError:
                    break
            # label comes after the CPU columns and optional chip/type fields;
            # for readability keep the raw key (number or name)
            irq_map[irq_key] = total
        out["interrupts"] = irq_map
    return out


def interrupts_delta(before, after):
    """Compute per-IRQ delta between two interrupts dicts (from
    remote_sys_snapshot). Returns {irq: delta} for IRQs that changed.
    Either argument may be None (returns empty dict)."""
    if not before or not after:
        return {}
    delta = {}
    for k, v in after.items():
        prev = before.get(k, 0)
        if v != prev:
            delta[k] = v - prev
    return delta


def remote_vm_sample(exec_node, pid):
    """Read one sample of Vm* fields from /proc/<pid>/status.

    Returns a dict {field_name: int_kb} for all 'Vm...' lines, or None
    when the process no longer exists (normal: the process exited between
    the sample and the read).
    """
    rc, sout, _ = run_remote_cmd(
        exec_node,
        f"grep '^Vm' /proc/{int(pid)}/status 2>/dev/null",
        timeout=5)
    if rc != 0 or not sout.strip():
        return None
    sample = {}
    for line in sout.splitlines():
        # format: "VmRSS:   1234 kB"
        parts = line.split()
        if len(parts) >= 2:
            key = parts[0].rstrip(":")
            try:
                sample[key] = int(parts[1])
            except ValueError:
                pass
    return sample if sample else None


def vm_samples_stats(samples):
    """Reduce a list of {field: int_kb} dicts (from remote_vm_sample) to
    {field: {min, max, median, avg}} dicts.  Fields missing in some
    samples are ignored for those samples only.

    Returns {} when samples is empty.
    """
    if not samples:
        return {}
    # gather per-field lists
    per_field = {}
    for s in samples:
        if s is None:
            continue
        for k, v in s.items():
            if k not in per_field:
                per_field[k] = []
            per_field[k].append(v)
    result = {}
    for k, vals in per_field.items():
        if not vals:
            continue
        vals_sorted = sorted(vals)
        n = len(vals_sorted)
        mid = n // 2
        median = (vals_sorted[mid] if n % 2 == 1
                  else (vals_sorted[mid - 1] + vals_sorted[mid]) / 2.0)
        result[k] = {
            "min": vals_sorted[0],
            "max": vals_sorted[-1],
            "median": median,
            "avg": sum(vals) / n,
            "unit": "kB",
            "n_samples": n,
        }
    return result


class VmSampler:
    """Background thread that polls /proc/<pid>/status Vm* fields on a
    remote exec node every `interval` seconds.

    Usage:
        sampler = VmSampler(exec_node, pid, interval=1.0)
        sampler.start()
        # ... run workload ...
        sampler.stop()
        stats = sampler.stats()   # {field: {min, max, median, avg, unit, n_samples}}
    """

    def __init__(self, exec_node, pid, interval=1.0):
        self._exec_node = exec_node
        self._pid = int(pid)
        self._interval = float(interval)
        self._samples = []
        self._stop_event = threading.Event()
        self._thread = threading.Thread(target=self._run, daemon=True)

    def start(self):
        self._stop_event.clear()
        self._thread.start()

    def stop(self):
        self._stop_event.set()
        self._thread.join(timeout=self._interval * 3 + 5)

    def _run(self):
        while not self._stop_event.is_set():
            s = remote_vm_sample(self._exec_node, self._pid)
            if s is not None:
                self._samples.append(s)
            self._stop_event.wait(self._interval)

    def stats(self):
        return vm_samples_stats(self._samples)

    def raw_samples(self):
        return list(self._samples)


# ---- the high-level "run a benchmark binary on a remote exec node" -------

def run_benchmark_on_exec(compile_node, exec_node, cmake_src_subdir,
                          build_subdir, bin_name, runtime_cmd,
                          profilers, cmake_defs=None, extras=None,
                          run_timeout=RUN_TIMEOUT_DEFAULT, verbose=False,
                          callgrind_toggle=None):
    """Diskless NFS-LXC wrapper: bring the exec node's container up
    before the benchmark and guarantee teardown after (try/finally),
    even on early-return failures. Both calls are no-ops for ordinary
    exec nodes, so this is inert unless lxc_nfs.enabled was flipped on."""
    runtime_node, msg = nfs_lxc_prepare(exec_node, verbose=verbose)
    if runtime_node is None:
        nfs_lxc_teardown(exec_node, verbose=verbose)
        return ({p: None for p in profilers}, msg)
    use_ninja = exec_node.get("ninja")  # None -> auto, True/False -> override
    try:
        return _run_benchmark_on_exec_inner(
            compile_node, runtime_node, cmake_src_subdir, build_subdir,
            bin_name, runtime_cmd, profilers, cmake_defs=cmake_defs,
            extras=extras, run_timeout=run_timeout, verbose=verbose,
            use_ninja=use_ninja,
            callgrind_toggle=callgrind_toggle)
    finally:
        nfs_lxc_teardown(exec_node, verbose=verbose)


def build_for_fleet(compile_node, cmake_src_subdir, build_subdir,
                    cmake_defs=None, use_ninja=None, verbose=False):
    """Phase-1 build: stage source + cmake build on ONE compile node.
    Returns (rc, msg, remote_build_dir). Meant to be run once per compile
    node (its exec nodes share arch, so one binary serves them all)."""
    if verbose:
        _rlog(f"{compile_node['label']!r}: staging source")
    rc, msg = stage_source_on_compile_node(compile_node, verbose=verbose)
    if rc != 0:
        return rc, f"stage failed: {msg}", None
    if verbose:
        _rlog(f"{compile_node['label']!r}: building {build_subdir}")
    return build_on_compile_node(compile_node, cmake_src_subdir, build_subdir,
                                 cmake_defs=cmake_defs, verbose=verbose,
                                 use_ninja=use_ninja)


def push_and_run_profilers(compile_node, exec_node, remote_bld, bin_name,
                           runtime_cmd, profilers, extras=None,
                           run_timeout=RUN_TIMEOUT_DEFAULT, verbose=False,
                           callgrind_toggle=None):
    """Phase-2 run on ONE exec node (already-built `remote_bld`):
    nfs-lxc bring-up + push binary/extras + run each profiler + teardown.
    Returns (out_dict, msg). Safe to call from a worker thread -- it only
    touches its own exec node (ssh/rsync), no shared state.

    `runtime_cmd` is either a single command STRING (same for every
    profiler) or a {profiler -> command-string} DICT -- the latter lets a
    benchmark use a different workload mode per profiler (e.g. fixed-time
    --ms for rusage/perf-stat but fixed-iteration --ticks for callgrind)."""
    runtime_node, msg = nfs_lxc_prepare(exec_node, verbose=verbose)
    if runtime_node is None:
        nfs_lxc_teardown(exec_node, verbose=verbose)
        # Bring-up / unreachable node: infra failure, not a perf regression.
        return {p: None for p in profilers}, f"SKIP:bringup-failed: {msg}"
    try:
        if verbose:
            _rlog(f"{exec_node['label']!r}: pushing binary")
        rc, exec_bin, msg = push_binary_to_exec(
            compile_node, runtime_node, remote_bld, bin_name,
            extras=extras, verbose=verbose)
        if rc != 0:
            # Push failed (e.g. 'No route to host'): infra, mark SKIP.
            return {p: None for p in profilers}, f"SKIP:push-failed: {msg}"
        ewd = runtime_node["work_dir"]

        def _rc_str(prof):
            return (runtime_cmd.get(prof) or runtime_cmd.get("rusage")
                    if isinstance(runtime_cmd, dict) else runtime_cmd)

        _setup = f"cd {shlex.quote(ewd)} && chmod +x {shlex.quote(bin_name)}"

        def _cmd(prof):
            return f"{_setup} && {_rc_str(prof)}"

        snap_before = remote_sys_snapshot(runtime_node)
        out = {}
        for p in profilers:
            if p == "rusage":
                out[p] = remote_time_v(runtime_node, _cmd(p), timeout=run_timeout)
            elif p == "perf-stat":
                out[p] = remote_perf_stat(runtime_node, _cmd(p), timeout=run_timeout)
            elif p == "callgrind":
                # binary run DIRECTLY under valgrind (not via sh -c); setup
                # prefix does cd/chmod first.
                out[p] = remote_callgrind(runtime_node, _rc_str(p), ewd,
                                          timeout=run_timeout * 30,
                                          toggle_collect=callgrind_toggle,
                                          setup_prefix=_setup)
            elif p == "binary-size":
                out[p] = remote_binary_size(runtime_node, exec_bin)
            else:
                out[p] = None
        snap_after = remote_sys_snapshot(runtime_node)
        out["sys_mem_frag"] = {
            "buddyinfo_before":    snap_before.get("buddyinfo"),
            "buddyinfo_after":     snap_after.get("buddyinfo"),
            "pagetypeinfo_before": snap_before.get("pagetypeinfo"),
            "pagetypeinfo_after":  snap_after.get("pagetypeinfo"),
            "interrupts_delta":    interrupts_delta(
                snap_before.get("interrupts"), snap_after.get("interrupts")),
        }
        return out, "ok"
    finally:
        nfs_lxc_teardown(exec_node, verbose=verbose)


def run_profiler_fleet(specs, verbose=False, max_workers=8):
    """Two-phase PARALLEL fleet runner for profiler-based benchmarks.

    `specs` -- one dict per exec node, each with:
        exec_node, compile_node, cmake_src_subdir, build_subdir_base,
        bin_name, runtime_cmd, profilers   (+ optional cmake_defs, extras,
        run_timeout)

    Phase 1: every UNIQUE compile node builds in parallel (one build dir
             `<build_subdir_base>-<compile_label>` shared by its exec
             nodes). Blocks until ALL builds finish -- "firstly all remote
             nodes do compilation".
    Phase 2: every exec node whose compile build succeeded runs in parallel
             -- bring-up + push binary/cache + profilers on the exec node
             corresponding to its compile node.

    Returns {exec_label: (out_dict, msg)}. Recording/progress is left to
    the caller (done serially after this returns, so Progress needs no
    lock)."""
    import concurrent.futures as cf
    by_cn = {}
    for s in specs:
        by_cn.setdefault(s["compile_node"]["label"], []).append(s)

    # ---- Phase 1: parallel build, one per compile node ------------------
    builds = {}   # compile_label -> (rc, msg, build_dir)

    def _build(cl):
        s = by_cn[cl][0]
        sub = f"{s['build_subdir_base']}-{cl}"
        use_ninja = s["exec_node"].get("ninja")
        return cl, build_for_fleet(
            s["compile_node"], s["cmake_src_subdir"], sub,
            cmake_defs=s.get("cmake_defs"), use_ninja=use_ninja,
            verbose=verbose)

    keys = list(by_cn.keys())
    if keys:
        with cf.ThreadPoolExecutor(max_workers=min(max_workers, len(keys))) as ex:
            for cl, res in ex.map(_build, keys):
                builds[cl] = res

    # ---- Phase 2: parallel push+run per exec node -----------------------
    results = {}
    lock = threading.Lock()

    def _run(s):
        cl = s["compile_node"]["label"]
        label = s["exec_node"]["label"]
        rc, msg, bld = builds.get(cl, (1, "no build result", None))
        if rc != 0:
            # A compile-node build failure is NOT a benchmark regression: the
            # exec node is never touched (no push, no run).  Tag the message
            # SKIP: so the recorder marks the cells SKIP, not FAIL -- a node
            # we couldn't build for has an *unknown* metric, which the
            # decision matrix must not read as a regression.
            with lock:
                results[label] = ({p: None for p in s["profilers"]},
                                  f"SKIP:compile-failed: {msg}")
            return
        out, m = push_and_run_profilers(
            s["compile_node"], s["exec_node"], bld, s["bin_name"],
            s["runtime_cmd"], s["profilers"], extras=s.get("extras"),
            run_timeout=s.get("run_timeout", RUN_TIMEOUT_DEFAULT),
            verbose=verbose,
            callgrind_toggle=s.get("callgrind_toggle"))
        with lock:
            results[label] = (out, m)

    if specs:
        with cf.ThreadPoolExecutor(max_workers=min(max_workers, len(specs))) as ex:
            list(ex.map(_run, specs))
    return results


def _run_benchmark_on_exec_inner(compile_node, exec_node, cmake_src_subdir,
                          build_subdir, bin_name, runtime_cmd,
                          profilers, cmake_defs=None, extras=None,
                          run_timeout=RUN_TIMEOUT_DEFAULT, verbose=False,
                          use_ninja=None, callgrind_toggle=None):
    """Full flow for one benchmark, one exec node:
      1. stage source on compile node
      2. cmake configure+build on compile node
      3. pull binary; push to exec node
      4. run each profiler that the (already-filtered) profilers list contains

    `runtime_cmd` is a shell-quoted command STRING run from the
    exec_node's work_dir; if it includes the binary, use a relative
    path like './bin --foo 1'.

    `use_ninja` — forwarded to build_on_compile_node; None = auto-detect,
    True = force Ninja, False = use Make.

    Returns a dict { profiler -> result } where result is either
    parsed metrics or None on failure. `binary-size` reads the file
    size on the exec node (the binary may be stripped differently per
    arch)."""
    out = {}
    if verbose:
        _rlog(f"{exec_node['label']!r}: staging source on "
              f"{compile_node['label']!r}")
    rc, msg = stage_source_on_compile_node(compile_node, verbose=verbose)
    if rc != 0:
        return {p: None for p in profilers}, f"stage failed: {msg}"
    if verbose:
        _rlog(f"{exec_node['label']!r}: building")
    rc, msg, remote_bld = build_on_compile_node(
        compile_node, cmake_src_subdir, build_subdir,
        cmake_defs=cmake_defs, verbose=verbose, use_ninja=use_ninja)
    if rc != 0:
        return {p: None for p in profilers}, msg
    # Carry the compile node's system-vs-vendored verdict onto this exec
    # node so its history record stamps the libs it actually ran.
    bh.alias_libs(compile_node["label"], exec_node["label"])
    if verbose:
        _rlog(f"{exec_node['label']!r}: pushing binary")
    rc, exec_bin, msg = push_binary_to_exec(
        compile_node, exec_node, remote_bld, bin_name,
        extras=extras, verbose=verbose)
    if rc != 0:
        return {p: None for p in profilers}, msg
    # The runtime_cmd may embed ./<bin_name>; make sure we run with
    # work_dir as cwd so the relative path resolves.
    ewd = exec_node["work_dir"]
    _setup = f"cd {shlex.quote(ewd)} && chmod +x {shlex.quote(bin_name)}"
    cmd_in_work = f"{_setup} && {runtime_cmd}"
    snap_before = remote_sys_snapshot(exec_node)
    for p in profilers:
        if p == "rusage":
            out[p] = remote_time_v(exec_node, cmd_in_work, timeout=run_timeout)
        elif p == "perf-stat":
            out[p] = remote_perf_stat(exec_node, cmd_in_work, timeout=run_timeout)
        elif p == "callgrind":
            # binary run DIRECTLY under valgrind (not via sh -c).
            out[p] = remote_callgrind(exec_node, runtime_cmd, ewd,
                                      timeout=run_timeout * 30,
                                      toggle_collect=callgrind_toggle,
                                      setup_prefix=_setup)
        elif p == "binary-size":
            out[p] = remote_binary_size(exec_node, exec_bin)
        else:
            out[p] = None
    snap_after = remote_sys_snapshot(exec_node)
    out["sys_mem_frag"] = {
        "buddyinfo_before":    snap_before.get("buddyinfo"),
        "buddyinfo_after":     snap_after.get("buddyinfo"),
        "pagetypeinfo_before": snap_before.get("pagetypeinfo"),
        "pagetypeinfo_after":  snap_after.get("pagetypeinfo"),
        "interrupts_delta":    interrupts_delta(
            snap_before.get("interrupts"), snap_after.get("interrupts")),
    }
    return out, "ok"
