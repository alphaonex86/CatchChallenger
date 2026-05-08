"""
remote_build.py — Shared module for remote compilation and testing.

Per-node configuration lives in the operator's remote_nodes.json file
(path resolved via test_config.REMOTE_NODES_JSON). It's the single source of
truth).  This module loads it once at import time and exposes:

  REMOTE_NODES     list of dicts (full per-node config straight from JSON)
  REMOTE_SERVERS   list of legacy 8-tuples derived from REMOTE_NODES, kept
                   so old call sites (testingbots.py / testingtools.py read
                   index [5] = has_gui) keep working.
  node_runs(label, test_name)   helper: True iff node opts into that test
  get_node(label)               look up node config by label

Remote builds run in background threads so they can be parallelized
with local compilation.  Call start_remote_builds() at the beginning
of the compilation phase and collect_remote_results() at the end.

For functional testing: start a server on a remote machine and connect
a local client to it via start_remote_server() / stop_remote_server().
Always call cleanup_remote_node() when done so the work dir does not
linger on the remote node.
"""

import os, json, re, subprocess, threading, signal
import diagnostic as _diag_mod
import cmd_helpers as _ch
import build_paths
import test_config

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Path to the per-operator nodes file. Lives outside the repo so the
# operator's SSH hosts and rootfs paths don't leak into git history;
# resolved through ~/.config/catchchallenger-testing/config.json
# (paths.remote_nodes_json key).
NODES_JSON = test_config.REMOTE_NODES_JSON

with open(NODES_JSON, "r") as _f:
    REMOTE_NODES = json.load(_f)["nodes"]


# All keys in remote_nodes.json are MANDATORY — there are no defaults.
# A missing key on any node aborts at module load with a precise error
# pointing at the offending node + key, so a typo never silently
# regresses a remote phase to "default behaviour" weeks later. See
# CLAUDE.md "All remote_nodes.json fields are mandatory".
#
# Adding a new field: bump both lists below AND the schema doc inside
# remote_nodes.json's `_doc` block AND the section in CLAUDE.md.
_REQUIRED_NODE_KEYS = (
    "label", "comment", "enabled", "ssh", "work_dir",
    "arch", "os", "linker",
    "has_gui", "has_ccache", "has_ninja",
    "cxx_version", "compilers", "compile_sql",
    "extra_defines", "tests",
    "save_timeout", "server_ready_timeout",
    "cmake_configure_timeout", "cmake_compile_timeout",
    "execution_nodes",
)
_REQUIRED_SSH_KEYS = ("host", "user", "port")
_REQUIRED_EXEC_NODE_KEYS = (
    "label", "enabled", "user", "host", "port",
    "work_dir", "datapack_cache",
    "real_hardware", "has_gui", "gpu_works",
    "client_run_mode", "x11", "execute_sql",
    "gdb_attach_timeout",
    # Per-exec-node opt-in for diagnostic runs.
    # sanitizer_gcc=true   → exec node will run --valgrind passes
    # sanitizer_clang=true → exec node will run --sanitize passes
    # Both default to false on a freshly-added node so the operator
    # opts in only after verifying the runtime has the required
    # tooling (valgrind installed for gcc; lib*san runtime libs for
    # clang). Setting either to true on a node that lacks the
    # tooling will surface a clear failure instead of silently
    # skipping.
    "sanitizer_gcc", "sanitizer_clang",
)


def _validate_node(n, idx):
    """Verify every required key is present on a compile node."""
    if not isinstance(n, dict):
        raise RuntimeError(
            f"remote_nodes.json node #{idx} is not a JSON object")
    label = n.get("label", f"<idx={idx}>")
    ki = 0
    while ki < len(_REQUIRED_NODE_KEYS):
        key = _REQUIRED_NODE_KEYS[ki]
        ki += 1
        if key not in n:
            raise RuntimeError(
                f"remote_nodes.json node {label!r}: missing required key {key!r}. "
                f"Every key listed in remote_build._REQUIRED_NODE_KEYS is "
                f"mandatory — see CLAUDE.md "
                f"\"All remote_nodes.json fields are mandatory\".")
    if not isinstance(n["ssh"], dict):
        raise RuntimeError(
            f"remote_nodes.json node {label!r}: 'ssh' must be a JSON object")
    si = 0
    while si < len(_REQUIRED_SSH_KEYS):
        key = _REQUIRED_SSH_KEYS[si]
        si += 1
        if key not in n["ssh"]:
            raise RuntimeError(
                f"remote_nodes.json node {label!r}: missing required key "
                f"ssh.{key}.")
    if not isinstance(n["execution_nodes"], list):
        raise RuntimeError(
            f"remote_nodes.json node {label!r}: 'execution_nodes' must be a list")
    ei = 0
    while ei < len(n["execution_nodes"]):
        en = n["execution_nodes"][ei]
        if not isinstance(en, dict):
            raise RuntimeError(
                f"remote_nodes.json node {label!r}: execution_nodes[{ei}] "
                f"is not a JSON object")
        en_label = en.get("label", f"<exec idx={ei}>")
        eki = 0
        while eki < len(_REQUIRED_EXEC_NODE_KEYS):
            key = _REQUIRED_EXEC_NODE_KEYS[eki]
            eki += 1
            if key not in en:
                raise RuntimeError(
                    f"remote_nodes.json node {label!r} execution_nodes[{ei}] "
                    f"({en_label!r}): missing required key {key!r}.")
        ei += 1


_ni = 0
while _ni < len(REMOTE_NODES):
    _validate_node(REMOTE_NODES[_ni], _ni)
    _ni += 1


_VALID_LINKERS = ("", "mold", "lld", "bfd", "gold", "wild")


def _node_linker(n):
    """Read the `linker` field from a node dict, validate it.

    Empty string = system default. Any other value must be one of
    mold/lld/bfd/gold; bad values fall back to "" with a warning so a
    typo doesn't poison every build with a bogus -fuse-ld flag.
    Back-compat: the old boolean `has_mold` (true) is mapped to "mold".
    """
    if "linker" in n:
        v = n.get("linker") or ""
        if v not in _VALID_LINKERS:
            print(f"warning: remote_nodes.json node {n.get('label','?')!r}: "
                  f"linker={v!r} not in {_VALID_LINKERS}; using \"\".")
            return ""
        return v
    if n.get("has_mold"):
        return "mold"
    return ""


def _node_to_tuple(n):
    """Build the legacy (label, ssh_host, port, linker, extra_defines,
    has_gui, cxx_versions, remote_dir) tuple from a JSON node dict.

    Index 3 was historically `use_mold` (bool); it is now the linker
    name string ("" / "mold" / "lld" / "bfd" / "gold"). Callers test it
    truthily for "any non-default linker" — the same shape works.

    Index 5 (has_gui) MUST stay stable — testingbots.py/testingtools.py
    read REMOTE_SERVERS[i][5] by position.

    Index 6 (cxx_versions) is a list of std flags; [] means qmake default
    (one build pass, no -std injected). Older single-value entries are
    coerced to a one-element list so callers never see a bare string.
    """
    raw = n["cxx_version"]
    if raw is None:
        cxx_versions = []
    elif isinstance(raw, list):
        cxx_versions = list(raw)
    else:
        cxx_versions = [raw]
    return (
        n["label"],
        f"{n['ssh']['user']}@{n['ssh']['host']}",
        n["ssh"]["port"],
        _node_linker(n),
        n["extra_defines"],
        n["has_gui"],
        cxx_versions,
        n["work_dir"],
    )


REMOTE_SERVERS = [_node_to_tuple(n) for n in REMOTE_NODES]


# Optional ad-hoc node filter, set via the CC_NODE_FILTER env var
# (comma-separated list of labels).  When set, ONLY listed nodes run, and
# the per-node `enabled` flag is bypassed — handy for debugging a node
# that is currently disabled in remote_nodes.json without flipping it.
_NODE_FILTER = None
_filter_env = os.environ.get("CC_NODE_FILTER", "").strip()
if _filter_env:
    requested = [p.strip() for p in _filter_env.split(",") if p.strip()]
    known_labels = {n["label"] for n in REMOTE_NODES}
    unknown = [r for r in requested if r not in known_labels]
    if unknown:
        import sys as _sys
        print(f"warning: CC_NODE_FILTER mentions unknown node label(s): "
              f"{', '.join(unknown)} — known: {', '.join(sorted(known_labels))}",
              file=_sys.stderr)
    _NODE_FILTER = set(requested)


def get_node(label):
    """Return the full per-node dict, or None if not found."""
    i = 0
    while i < len(REMOTE_NODES):
        if REMOTE_NODES[i]["label"] == label:
            return REMOTE_NODES[i]
        i += 1
    return None


def all_enabled_exec_nodes(diag=None):
    """Flatten every `execution_nodes` entry from every enabled compile
    node in remote_nodes.json that itself is enabled. Each returned dict
    is the exec_node verbatim (label, enabled, user, host, port, work_dir,
    real_hardware, has_gui, gpu_works, client_run_mode, x11). Used by
    test/testing*.py to feed cmd_helpers.assert_port_or_fail_with_remotes
    when probing 'is the server reachable from every machine that may
    later try to connect?'.

    When `diag` is set (--sanitize / --valgrind), additionally drop any
    exec node that has not opted in to the matching diagnostic mode via
    its `sanitizer_clang` / `sanitizer_gcc` flag in remote_nodes.json.
    Caller passes `diag=DIAG` to honour the per-exec-node opt-in; passing
    None preserves the legacy "all enabled nodes" behaviour for tests
    that don't care about the diag matrix.
    """
    out = []
    ni = 0
    while ni < len(REMOTE_NODES):
        node = REMOTE_NODES[ni]
        ni += 1
        if not bool(node.get("enabled", True)):
            continue
        execs = node.get("execution_nodes") or []
        ei = 0
        while ei < len(execs):
            ent = execs[ei]
            ei += 1
            if not bool(ent.get("enabled", True)):
                continue
            if diag is not None and not _diag_mod.exec_node_supports(
                    ent, diag, parent_compile_node=node):
                continue
            out.append(ent)
    return out


def node_runs(label, test_name):
    """True iff the node should run the named test phase ('compile',
    'server-client', 'solo').

    Decision order:
      1. Unknown label → False.
      2. CC_NODE_FILTER set (via env var or all.sh --node) → only labels
         in that filter run.  The per-node `enabled` flag is BYPASSED in
         this mode, so you can target a temporarily-disabled node for
         debugging without editing remote_nodes.json.
      3. No filter → respect the `enabled` field (default: true).
      4. Always require `test_name in tests`.
    """
    n = get_node(label)
    if n is None:
        return False
    if _NODE_FILTER is not None:
        if label not in _NODE_FILTER:
            return False
        # filter is explicit — bypass enabled flag
    elif not n.get("enabled", True):
        return False
    return test_name in n.get("tests", [])

# .pro files that need Qt GUI modules — skip on servers without GUI.
# Anything pulling libqtcatchchallenger directly (clients, GUI-bot tools)
# transitively requires Qt6 Core+Gui+Network+Widgets+OpenGL+OpenGLWidgets+Xml,
# so the build hard-fails on Qt-less nodes. Add new entries here when
# wiring a new GUI-touching .pro into the test matrix.
GUI_PRO_FILES = {
    "client/qtcpu800x600/qtcpu800x600.pro",
    "client/qtopengl/catchchallenger-qtopengl.pro",
    "tools/map2png/map2png.pro",
    "tools/datapack-explorer-generator-cli/datapack-explorer-generator.pro",
    # bot-actions is a Qt6 GUI bot debugger — find_package(Qt6 ... Widgets
    # WebSockets REQUIRED) at the top of its CMakeLists.txt aborts configure
    # on a node without Qt6 (mips-lxc).
    "tools/bot-actions/bot-actions.pro",
    # datapack-downloader-cli pulls libqtcatchchallenger transitively for
    # the QtDatapackClientLoader path; same Qt6 requirement.
    "tools/datapack-downloader-cli/datapack-downloader.pro",
    # bot-test-connect-to-gameserver builds against libqtcatchchallenger
    # for the protocol parser → Qt6 required.
    "tools/bot-test-connect-to-gameserver-cli/bot-test-connect-to-gameserver.pro",
}

# Default work dir on a remote node when the per-node value is unknown
# (e.g. external callers that import REMOTE_DIR).  The authoritative path
# for each configured node lives in REMOTE_SERVERS[i][7].
REMOTE_DIR = "/tmp/catchchallenger-test"
# Hard fallbacks for callers that don't pass a node label. Per-node
# overrides live in remote_nodes.json:
#   cmake_configure_timeout (default 180s) — `cmake -S … -B …` phase
#   cmake_compile_timeout   (default 600s) — `cmake --build` phase
# Slow nodes (qemu-mips, real i686 hardware) bump these in the JSON.
COMPILE_TIMEOUT = 600
CONFIGURE_TIMEOUT_DEFAULT = 180
COMPILE_TIMEOUT_DEFAULT = 600
RSYNC_TIMEOUT = 300
SERVER_READY_TIMEOUT = 60
REMOTE_GAME_PORT = 61917


def configure_timeout_for(label):
    """Per-node cmake configure timeout. Read from `cmake_configure_timeout`
    in remote_nodes.json; defaults to CONFIGURE_TIMEOUT_DEFAULT (180s)."""
    node = get_node(label)
    if node is not None:
        v = node.get("cmake_configure_timeout")
        if v is not None:
            return int(v)
    return CONFIGURE_TIMEOUT_DEFAULT


def compile_timeout_for(label):
    """Per-node cmake compile timeout. Read from `cmake_compile_timeout`
    in remote_nodes.json; defaults to COMPILE_TIMEOUT_DEFAULT (600s)."""
    node = get_node(label)
    if node is not None:
        v = node.get("cmake_compile_timeout")
        if v is not None:
            return int(v)
    return COMPILE_TIMEOUT_DEFAULT

# Mapping from .pro files to their make*.py companion script (relative to ROOT).
# Used both for server targets (SERVER_TARGETS in test scripts) and tool
# targets that have a make.py companion. Each make.py accepts the same env
# vars: EXTRA_DEFINES, COMPILER (gcc|clang), USE_MOLD, BUILD_DIR.
SERVER_MAKE_SCRIPTS = {
    "server/epoll/catchchallenger-server-filedb.pro":
        "server/epoll/make-file.py",
    "server/epoll/catchchallenger-server-cli-epoll.pro":
        "server/epoll/make-catchchallenger-server-cli-epoll.py",
    "server/epoll/catchchallenger-server-test.pro":
        "server/epoll/make-test.py",
    "server/login/login.pro":
        "server/login/make.py",
    "server/master/master.pro":
        "server/master/make.py",
    "server/gateway/gateway.pro":
        "server/gateway/make.py",
    "server/game-server-alone/game-server-alone.pro":
        "server/game-server-alone/make-game-server-alone.py",
    "tools/stats/stats.pro":
        "tools/stats/make.py",
}

# ── colours ─────────────────────────────────────────────────────────────────
C_GREEN  = "\033[92m"
C_RED    = "\033[91m"
C_CYAN   = "\033[96m"
C_RESET  = "\033[0m"


def _control_path_for(host, port):
    """Short Unix-domain socket path for the per-host SSH ControlMaster.

    Path stays under the 104-byte sun_path limit by hashing the host:port
    rather than embedding it raw (IPv6 addresses are long)."""
    import hashlib
    h = hashlib.sha1(f"{host}:{port}".encode()).hexdigest()[:10]
    return f"/tmp/cc-ssh-{os.getpid()}-{h}.sock"


def _open_ssh_master(host, port, reason):
    """Open a persistent SSH ControlMaster to (host, port) and return its
    ControlPath.  Logs an INFO line stating where we connect and why so
    later commands traceably reuse the same TCP connection.

    Returns the control_path string on success, or None on failure (the
    caller should then fall back to per-command SSH).

    ConnectTimeout=5 is enforced via cmd_helpers.SSH_OPTS_LIST so a dead
    remote fails the master-open in 5s rather than ~2 min."""
    cp = _control_path_for(host, port)
    print(f"{C_CYAN}[INFO]{C_RESET} ssh-open {host}:{port} — {reason}")
    args = (["ssh"] + _ch.SSH_OPTS_LIST +
            ["-o", "ControlMaster=auto",
             "-o", f"ControlPath={cp}",
             "-o", "ControlPersist=60",
             "-o", "ServerAliveInterval=30",
             "-fN",
             "-p", str(port), host])
    try:
        p = subprocess.run(args, timeout=20,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if p.returncode == 0:
            return cp
        tail = p.stdout.decode(errors="replace").strip().splitlines()[-1:]
        msg = tail[0] if tail else f"rc={p.returncode}"
        print(f"{C_CYAN}[INFO]{C_RESET} ssh-open {host}:{port} FAILED ({msg}); "
              f"falling back to per-command ssh")
    except subprocess.TimeoutExpired:
        print(f"{C_CYAN}[INFO]{C_RESET} ssh-open {host}:{port} TIMEOUT; "
              f"falling back to per-command ssh")
    return None


def _close_ssh_master(host, port, control_path, reason=""):
    """Close the SSH ControlMaster opened by _open_ssh_master().  Safe to
    call with control_path=None."""
    if control_path is None:
        return
    suffix = f" — {reason}" if reason else ""
    print(f"{C_CYAN}[INFO]{C_RESET} ssh-close {host}:{port}{suffix}")
    try:
        subprocess.run((["ssh", "-O", "exit"] + _ch.SSH_OPTS_LIST +
                        ["-o", f"ControlPath={control_path}",
                         "-p", str(port), host]),
                       timeout=10,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except subprocess.TimeoutExpired:
        pass
    try:
        os.unlink(control_path)
    except OSError:
        pass


def _ssh_cmd(host, port, command, timeout=COMPILE_TIMEOUT, control_path=None):
    """Run `command` over SSH. ConnectTimeout=5 forced; per-call timeout
    clamped to SSH_MAX_TIMEOUT (1200s). On TimeoutExpired returns
    (-1, "<SSH_TIMEOUT_MARKER>: Ns") so callers can use
    cmd_helpers.is_ssh_timeout() to surface "TIMEOUT" in log_fail."""
    timeout = _ch.clamp_ssh(timeout)
    ssh_args = ["ssh"] + _ch.SSH_OPTS_LIST
    if control_path is not None:
        ssh_args += ["-o", f"ControlPath={control_path}"]
    ssh_args += ["-p", str(port), host, command]
    try:
        p = subprocess.run(ssh_args, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"{_ch.SSH_TIMEOUT_MARKER}: {timeout}s"


def _rsync_host(host):
    """Wrap IPv6 in brackets for rsync destination (e.g. root@[::1])."""
    if "@" in host:
        user, addr = host.split("@", 1)
        if ":" in addr and not addr.startswith("["):
            return f"{user}@[{addr}]"
    return host


def _host_address(host):
    """Extract just the address from user@host for network connections."""
    if "@" in host:
        return host.split("@", 1)[1]
    return host


def _rsync_to_remote(host, port, remote_dir, control_path=None):
    rsync_dest = _rsync_host(host)
    # Ensure parent dir + sources/ leaf exist (rsync creates the leaf
    # but not parents) and that previous test artefacts do not linger.
    # Sources go into <remote_dir>/sources/ so build outputs (build-remote/,
    # ccache/, tmp/) live alongside the source tree, not inside it.
    # Nuke any leftover loose files from an older layout where sources
    # lived at the workdir root, plus stale build-remote* dirs (cmake's
    # cache pins -S to the absolute source path; if a previous run used
    # a different source root, every configure aborts with "source ...
    # does not match the source ... used to generate cache"). Keep
    # sources/, ccache/, tmp/ — these are the persistent caches.
    _ssh_cmd(host, port,
             f"mkdir -p {remote_dir}/sources && "
             f"find {remote_dir} -mindepth 1 -maxdepth 1 "
             f"-not -name sources -not -name ccache -not -name tmp "
             f"-exec rm -rf {{}} + 2>/dev/null; true",
             timeout=30, control_path=control_path)
    ssh_e = f"{_ch.RSYNC_SSH_E} -p {port}"
    if control_path is not None:
        ssh_e += f" -o ControlPath={control_path}"
    rsync_args = [
        "rsync", "-art", "--delete",
        "--exclude=.git", "--exclude=build/", "--exclude=*.o",
        "--exclude=moc_*", "--exclude=Makefile",
        "-e", ssh_e,
        ROOT + "/", f"{rsync_dest}:{remote_dir}/sources/"
    ]
    try:
        p = subprocess.run(rsync_args, timeout=RSYNC_TIMEOUT,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        return p.returncode == 0, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return False, f"TIMEOUT after {RSYNC_TIMEOUT}s"


def _cleanup_remote(host, port, remote_dir, control_path=None):
    """Remove only the build artefacts on a remote node, leaving the
    rsynced source tree and the ccache slot in place.

    Wiping the source between phases forced every rsync to recreate
    every file (new inode → new ctime), which in turn invalidated the
    ccache (file_stat_matches sloppiness aside, the manifest still
    indexes the rsynced absolute paths). Keeping the source persistent
    means the next rsync only carries diffs and the warm ccache hits.

    Refuses to delete paths shorter than 6 characters or '/' as a
    safety net.
    """
    if not remote_dir or len(remote_dir) < 6 or remote_dir == "/":
        return -1, "refused: suspicious remote_dir"
    # Wipe everything at the workdir top level EXCEPT the persistent
    # state we want to amortise across runs:
    #   sources/ — incremental rsync target (diffs only on next run)
    #   ccache/  — warm compiler cache
    #   tmp/     — large $TMPDIR that gcc -g spills to (recreated cheaply
    #              but no need to nuke it either)
    # Anything else (build-remote*, loose files left by an older layout
    # where sources lived at the workdir root, etc.) gets removed.
    cmd = (f"find {remote_dir} -mindepth 1 -maxdepth 1 "
           f"-not -name sources -not -name ccache -not -name tmp "
           f"-exec rm -rf {{}} + 2>/dev/null; true")
    return _ssh_cmd(host, port, cmd, timeout=60, control_path=control_path)


def _detect_qmake(host, port, control_path=None):
    """Phase 5 (qmake -> CMake) — same name kept for source compat;
    now returns the path to the remote `cmake` binary, used by
    _build_pro_remote."""
    rc, out = _ssh_cmd(host, port,
                       "which cmake 2>/dev/null",
                       timeout=15, control_path=control_path)
    if rc == 0 and out.strip():
        return out.strip().splitlines()[0]
    return None


def _need_qmake_stash(host, port, control_path=None):
    """Phase 5 — was a workaround for a remote qmake6 toolchain.prf bug;
    no longer relevant under cmake. Always returns False so the call
    site is a no-op."""
    return False


def _build_pro_remote(host, port, cmake, pro_rel, label, use_mold,
                      extra_defines, need_stash, cxx_version, remote_dir,
                      diag=None, control_path=None):
    """Phase 5 (qmake -> CMake) remote build. The `qmake` arg name is
    kept for source compatibility — _detect_qmake now returns a path
    to cmake on the remote node. `need_stash` is no longer used (it
    was a workaround for a qmake6 toolchain.prf bug)."""
    import cmake_helpers as _ch
    base_name = os.path.basename(pro_rel).replace('.pro', '')
    suffix = _diag_mod.build_dir_suffix(diag)
    if cxx_version:
        build_dir = f"{remote_dir}/build-remote{suffix}/{base_name}-{cxx_version}"
    else:
        build_dir = f"{remote_dir}/build-remote{suffix}/{base_name}"
    if cxx_version:
        name = f"compile {pro_rel} ({label} {cxx_version})"
    else:
        name = f"compile {pro_rel} ({label})"
    try:
        target, configure_flags, source_subdir = _ch.pro_to_cmake_target(pro_rel)
    except KeyError:
        return (name, False, f"no cmake target mapping for {pro_rel}")
    flag_args = " ".join(configure_flags)
    define_opts = ""
    if extra_defines:
        cmake_opts, _ = _ch.classify_extra_defines(extra_defines)
        define_opts = " ".join(cmake_opts)
    cxx_arg = ""
    if cxx_version:
        m = re.match(r"(?:c\+\+|gnu\+\+)?(\d+)", cxx_version)
        if m:
            cxx_arg = f"-DCMAKE_CXX_STANDARD={m.group(1)}"
    # `use_mold` is now a linker name string ("" / "mold" / "lld" /
    # "bfd" / "gold"). Empty = system default; otherwise pass
    # -fuse-ld=<linker>. Old bool callers (True) are coerced to "mold"
    # for back-compat; bool False is "" same as before.
    linker_name = use_mold if isinstance(use_mold, str) else ("mold" if use_mold else "")
    mold_arg = ""
    # When linker=lld and ld.lld lives outside $PATH (e.g. Gentoo's
    # llvm-core/lld at /usr/lib/llvm/<ver>/bin/ld.lld), gcc's
    # -fuse-ld=lld driver fails with `cannot find 'ld'`. Prepend any
    # /usr/lib/llvm/*/bin dirs that ship ld.lld so the driver finds it.
    linker_path_prefix = ""
    if linker_name:
        # `-Wl,-z,notext` is needed for lld on MIPS: cmake's internal
        # compiler-feature probes link a tiny .o that wasn't built with
        # -fPIC, and lld refuses text relocations by default. GNU ld /
        # mold accept the same input silently. The flag is harmless on
        # other architectures and on other linkers, so apply it whenever
        # a non-default linker is selected.
        # Quote the value so the embedded space (between -fuse-ld=… and
        # -Wl,-z,notext) survives the remote shell word-split that
        # happens when the configure command is exec'd via /bin/sh -c.
        # Without the quotes cmake would see a stray "-Wl,-z,notext" as
        # its own positional argument and silently drop it.
        link_flags = f"-fuse-ld={linker_name} -Wl,-z,notext"
        mold_arg = f"\"-DCMAKE_EXE_LINKER_FLAGS={link_flags}\""
        if linker_name == "lld":
            # ld.lld lives directly in $PATH on most distros (Debian/
            # Ubuntu's `lld` package symlinks /usr/bin/ld.lld), but
            # versioned installs put it under a per-version prefix:
            #   Gentoo:        /usr/lib/llvm/<ver>/bin/ld.lld
            #   Debian/Ubuntu: /usr/lib/llvm-<ver>/bin/ld.lld
            #   Fedora/RHEL:   /usr/bin/ld.lld (already in $PATH)
            # Probe both layouts and prepend any matches; the trailing
            # $PATH catches the Fedora-style case where it's already on
            # the path so the prefix is a no-op there.
            linker_path_prefix = (
                "PATH=\"$(for d in /usr/lib/llvm/*/bin /usr/lib/llvm-*/bin; "
                "do [ -x \"$d/ld.lld\" ] && printf '%s:' \"$d\"; done)$PATH\" "
            )
    nice_prefix = (
        "NICE=\"\"; command -v nice >/dev/null && NICE=\"nice -n 19\"; "
        "command -v ionice >/dev/null && NICE=\"$NICE ionice -c 3\"; "
    )
    # Some nodes (x86-lxc) ship a small (2 GB) tmpfs at /tmp that fills up
    # under -j32 parallel debug compiles — cc1plus writes per-TU .s
    # spills there and aborts with "No space left on device".  Redirect
    # gcc's temp output into the persistent work_dir/tmp instead, which
    # lives on the LXC's main disk and survives the cleanup_remote
    # phase.  Created on demand so we don't poison nodes that don't need
    # it.
    tmp_prefix = (f"mkdir -p {remote_dir}/tmp && "
                  f"TMPDIR={remote_dir}/tmp ")
    # Per-node ccache + ninja switches:
    #   has_ccache (default true)  → CCACHE_DIR=<work_dir>/ccache so the
    #                                 cache persists across runs;
    #                                 cleanup_remote_node() preserves it.
    #   has_ccache=false           → CCACHE_DISABLE=1 (ccache's official
    #                                 no-op switch). Used when ccache is
    #                                 missing or causes trouble on a node.
    #   has_ninja (default true)   → -G Ninja for faster incremental builds.
    #   has_ninja=false            → empty -G, remote cmake picks Make.
    node = get_node(label) if label else None
    use_ccache = bool(node.get("has_ccache", True)) if node else True
    use_ninja = bool(node.get("has_ninja", True)) if node else True
    if use_ccache:
        # CCACHE_BASEDIR: ccache rewrites every absolute path that starts
        # with the basedir to a path relative to it before hashing, so the
        # same source tree compiled under /tmp/catchchallenger-test/...
        # produces identical hashes across runs even if a future change
        # moves the work_dir.
        # CCACHE_NOHASHDIR=1: stop hashing the compile CWD, otherwise the
        # build_dir's per-target subpath leaks into the cache key and a
        # second `cmake --build` of the SAME source still misses.
        # CCACHE_SLOPPINESS=time_macros,include_file_mtime,include_file_ctime,
        # file_stat_matches: tolerate __TIME__/__DATE__ macros and let
        # ccache trust file content over inode-stat changes — rsync's
        # `-a` recreates inodes (new ctime each run) even when content
        # is identical, which under default sloppiness invalidates every
        # entry. With these flags the warm cache survives across runs
        # (0% → ~100% hit rate after the first build).
        ccache_env = (f"CCACHE_DIR={remote_dir}/ccache "
                      f"CCACHE_BASEDIR={remote_dir} "
                      f"CCACHE_NOHASHDIR=1 "
                      f"CCACHE_SLOPPINESS=time_macros,include_file_mtime,"
                      f"include_file_ctime,file_stat_matches ")
        mkdir_ccache = f"mkdir -p {remote_dir}/ccache && "
    else:
        ccache_env = "CCACHE_DISABLE=1 "
        mkdir_ccache = ""
    # Pin to the system ninja (/usr/bin/ninja). Some nodes have a
    # pip-installed ~/.local/bin/ninja shim that intercepts cmake's
    # default lookup — see cmake_helpers._ninja_or_default_generator_args
    # for the same rationale on the local box.
    gen_args = ("-G Ninja -DCMAKE_MAKE_PROGRAM=/usr/bin/ninja "
                if use_ninja else "")
    # Two separate ssh invocations so we can bound configure and build
    # independently from per-node JSON (cmake_configure_timeout /
    # cmake_compile_timeout, defaults 180s / 600s). Configure rarely
    # exceeds a few seconds; isolating it means a hung build doesn't
    # eat into the configure budget on retry, and a hung configure
    # fails fast instead of stealing the whole compile-budget window.
    # Force PIC on every translation unit so cmake's compiler-feature
    # probes link cleanly under lld on MIPS — its R_MIPS_32 / TEXTREL
    # rejection only triggers on non-PIC code. PIC is the default in
    # most distros' gcc anyway, so this is a no-op everywhere else.
    # Also pin -fno-lto here (see cmake_helpers.py for the rationale —
    # tests don't need LTO and the lto-wrapper invocations slow down the
    # remote build cycle without buying anything). The init-flags string
    # is shared between the lld and non-lld branches so the merge keeps
    # both knobs.
    init_cflags = "-fno-lto"
    init_cxxflags = "-fno-lto"
    if linker_name == "lld":
        init_cflags = "-fPIC " + init_cflags
        init_cxxflags = "-fPIC " + init_cxxflags
    pic_args = (f"-DCMAKE_C_FLAGS_INIT='{init_cflags}' "
                f"-DCMAKE_CXX_FLAGS_INIT='{init_cxxflags}' "
                f"-DCMAKE_EXE_LINKER_FLAGS=-fno-lto "
                f"-DCMAKE_SHARED_LINKER_FLAGS=-fno-lto "
                f"-DCMAKE_MODULE_LINKER_FLAGS=-fno-lto ")
    # Per "one binary per CMakeLists.txt" refactor, point -S at the
    # specific subdir's CMakeLists.txt (each binary is its own project).
    cmake_source = f"{remote_dir}/sources/{source_subdir}"
    configure_cmd = (
        f"mkdir -p {build_dir} && {mkdir_ccache}{tmp_prefix}{nice_prefix}"
        f"{linker_path_prefix}{ccache_env}$NICE {cmake} {gen_args}-S {cmake_source} -B {build_dir} "
        f"-DCMAKE_BUILD_TYPE=Debug {pic_args}{flag_args} {define_opts} {cxx_arg} {mold_arg} "
        f"2>&1"
    )
    rc, out = _ssh_cmd(host, port, configure_cmd,
                       timeout=configure_timeout_for(label),
                       control_path=control_path)
    if rc != 0:
        # Persist FULL remote output via failed_cases side channel — the
        # detail string stays a one-liner summary; failed.json captures
        # the whole compile_output so a human can read what configure
        # printed.  Operator policy: never truncate.
        import failed_cases as _fc
        _fc.set_extras(name,
                       host=f"{host}:{port}",
                       cmd=f"ssh -p {port} {host} {configure_cmd!r}",
                       compile_output=(out or ""))
        return (name, False, f"configure rc={rc} on {host}:{port}")
    build_cmd = (
        f"{tmp_prefix}{nice_prefix}"
        f"{linker_path_prefix}{ccache_env}$NICE {cmake} --build {build_dir} --target {target} -j$(nproc) 2>&1"
    )
    rc, out = _ssh_cmd(host, port, build_cmd,
                       timeout=compile_timeout_for(label),
                       control_path=control_path)
    if rc == 0:
        # Self-contained per-binary CMakeLists.txt: the produced binary
        # lands at <build>/<target> directly. No copy needed.
        return (name, True, "")
    # ccache on the remote may be poisoned (e.g. inode reuse after a
    # previous build dir got rsync'd over).  Per operator policy: flush
    # it once and retry — only mark as failed if it fails again.
    print(f"{C_CYAN}[INFO]{C_RESET} {label} build failed on {host}:{port}; "
          f"flushing remote ccache and retrying once")
    _ssh_cmd(host, port, "ccache -C 2>/dev/null || true",
             timeout=120, control_path=control_path)
    rc, out = _ssh_cmd(host, port, build_cmd,
                       timeout=compile_timeout_for(label),
                       control_path=control_path)
    if rc == 0:
        return (name, True, "")
    import failed_cases as _fc
    _fc.set_extras(name,
                   host=f"{host}:{port}",
                   cmd=f"ssh -p {port} {host} {build_cmd!r}",
                   compile_output=(out or ""))
    return (name, False,
            f"build rc={rc} on {host}:{port} [retried after remote ccache -C]")


def _run_server(label, host, port, use_mold, extra_defines, has_gui,
                cxx_versions, remote_dir, pro_files, results_list, lock,
                diag=None):
    """Thread target: rsync + compile all pro_files on one remote server.

    cxx_versions is the per-node list of -std flags. An empty list means
    one build pass with qmake's default standard. Otherwise each element
    triggers its own build pass (so the same .pro is compiled once per
    requested standard).

    The remote work dir (remote_dir) is removed at the end via try/finally
    so a failed build still cleans up after itself.
    """
    cp = _open_ssh_master(host, port, f"compile phase on {label} ({len(pro_files)} pro file(s))")
    try:
        ok, out = _rsync_to_remote(host, port, remote_dir, control_path=cp)
        if not ok:
            with lock:
                results_list.append((f"rsync to {host}", False, out.strip()[:200]))
            return
        with lock:
            results_list.append((f"rsync to {host}", True, ""))

        # Server .pro files use their make*.py companion remotely; client
        # and tool .pro files still go through qmake (no make*.py).
        qmake = None
        need_stash = False

        # Empty list → one build pass with qmake default (cxx_version=None).
        passes = cxx_versions if cxx_versions else [None]

        cmake_unavailable = False
        idx = 0
        while idx < len(pro_files):
            pro_rel = pro_files[idx]
            if not has_gui and pro_rel in GUI_PRO_FILES:
                idx += 1
                continue
            if cmake_unavailable:
                idx += 1
                continue
            if qmake is None:
                qmake = _detect_qmake(host, port, control_path=cp)
                if qmake is None:
                    cmake_unavailable = True
                    print(f"{C_CYAN}[INFO]{C_RESET} no cmake on {host}: "
                          f"skipping remote builds")
                    idx += 1
                    continue
            pi = 0
            while pi < len(passes):
                cxx_ver = passes[pi]
                result = _build_pro_remote(host, port, qmake, pro_rel,
                                           label, use_mold, extra_defines,
                                           need_stash, cxx_ver, remote_dir,
                                           diag=diag, control_path=cp)
                with lock:
                    results_list.append(result)
                pi += 1
            idx += 1
    finally:
        # Do NOT _cleanup_remote here.  The server-client and autosolo
        # phases later in the test scripts re-use the binaries we just
        # built, so the work dir must persist across phases.  Each
        # caller (testingclient.py / testingserver.py phase E) runs
        # `cleanup_remote_node()` once it's truly done with the node;
        # testingbots.py / testingtools.py leak the dir, which is
        # acceptable for the local LXC setup.
        _close_ssh_master(host, port, cp,
                          reason=f"compile phase on {label} done")


# ── local make*.py invocation ──────────────────────────────────────────────

def get_make_script(pro_rel):
    """Return the make*.py script path (relative to ROOT) for a server .pro file,
    or None if the .pro has no make*.py companion."""
    return SERVER_MAKE_SCRIPTS.get(pro_rel)


def build_server_via_make(pro_rel, build_dir, label,
                          extra_defines=None, compiler="gcc",
                          use_mold=False, timeout=COMPILE_TIMEOUT,
                          diag=None):
    """Phase 5: was "run the make*.py wrapper"; the wrapper layer is
    gone, this drives cmake directly. The function name and
    (rc, output) return shape are preserved so testingserver.py /
    testingtools.py call sites need no change.
    """
    import cmake_helpers as _ch
    if SERVER_MAKE_SCRIPTS.get(pro_rel) is None:
        return -2, f"no cmake mapping for {pro_rel}"
    try:
        target, configure_flags, source_subdir = _ch.pro_to_cmake_target(pro_rel)
    except KeyError:
        return -2, f"no cmake mapping for {pro_rel}"
    build_dir = os.path.abspath(build_dir)
    os.makedirs(build_dir, exist_ok=True)
    cmake_source = os.path.join(ROOT, source_subdir)
    cc, cxx = ("gcc", "g++") if compiler == "gcc" else ("clang", "clang++")
    args = [
        "cmake", "-S", cmake_source, "-B", build_dir,
        f"-DCMAKE_C_COMPILER={cc}",
        f"-DCMAKE_CXX_COMPILER={cxx}",
        "-DCMAKE_BUILD_TYPE=Debug",
    ]
    args.extend(configure_flags)
    args.extend(_ch.cmake_extra_args(extra_defines))
    # use_mold is now a linker name string ("" / "mold" / "lld" /
    # "bfd" / "gold"); bool True is back-compat-mapped to "mold".
    linker_name = use_mold if isinstance(use_mold, str) else ("mold" if use_mold else "")
    if linker_name:
        args.append(f"-DCMAKE_EXE_LINKER_FLAGS=-fuse-ld={linker_name}")
    diag_args = _diag_mod.cmake_extra_args(diag) if hasattr(_diag_mod, "cmake_extra_args") else []
    args.extend(diag_args)
    try:
        p = subprocess.run(args,
                           timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if p.returncode != 0:
            return p.returncode, p.stdout.decode(errors="replace")
        nproc = os.cpu_count() or 4
        p = subprocess.run(["cmake", "--build", build_dir,
                            "--target", target, "-j", str(nproc + 1)],
                           timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except subprocess.TimeoutExpired:
        return -1, f"TIMEOUT after {timeout}s"
    out = p.stdout.decode(errors="replace")
    # Self-contained per-binary CMakeLists.txt: binary is at
    # build_dir/<target> directly. No copy step needed.
    return p.returncode, out


# ── qmake-vs-make comparator removed in Phase 5 (no qmake) ──


def count_remote_tests(pro_files, diag=None):
    """Predict the number of result tuples start_remote_builds(pro_files, diag)
    will produce, by replaying the same per-node iteration without doing any
    work.

    Per opted-in node:
      * 1 entry if the node lacks the compiler required by `diag` (diag-skip).
      * Otherwise 1 (rsync) plus len(cxx_versions or [None]) per .pro file
        (filtered by has_gui + GUI_PRO_FILES).

    Nodes without cmake installed emit fewer results at runtime, so on those
    machines the predicted total is an upper bound.
    """
    total = 0
    idx = 0
    while idx < len(REMOTE_SERVERS):
        srv = REMOTE_SERVERS[idx]
        idx += 1
        label = srv[0]
        if not node_runs(label, "compile"):
            continue
        node = get_node(label)
        if node is not None and not _diag_mod.node_supports(node, diag):
            total += 1
            continue
        has_gui = srv[5]
        cxx_versions = srv[6]
        passes = cxx_versions if cxx_versions else [None]
        total += 1  # rsync
        pi = 0
        while pi < len(pro_files):
            pro_rel = pro_files[pi]
            pi += 1
            if (not has_gui) and pro_rel in GUI_PRO_FILES:
                pass
            else:
                total += len(passes)
    return total


def start_remote_builds(pro_files, diag=None):
    """Start remote compilation threads for the given .pro files.

    Returns (threads, results_list, lock).
    Call collect_remote_results() after local builds are done.

    When `diag` is set, only nodes whose `compilers` list includes the
    required compiler (clang for --sanitize, gcc for --valgrind) are run;
    the others are skipped silently. The diagnostic flags propagate into
    the per-target build commands.
    """
    results_list = []
    lock = threading.Lock()
    threads = []
    idx = 0
    while idx < len(REMOTE_SERVERS):
        (label, host, port, use_mold, extra_defines, has_gui, cxx_versions,
         remote_dir) = REMOTE_SERVERS[idx]
        if not node_runs(label, "compile"):
            idx += 1
            continue
        node = get_node(label)
        if node is not None and not _diag_mod.node_supports(node, diag):
            with lock:
                results_list.append(
                    (f"diag skip {label}", True,
                     f"node has no {_diag_mod.compiler_name(diag)}, skipping"))
            idx += 1
            continue
        t = threading.Thread(target=_run_server,
                             args=(label, host, port, use_mold, extra_defines,
                                   has_gui, cxx_versions, remote_dir, pro_files,
                                   results_list, lock),
                             kwargs={"diag": diag},
                             daemon=True)
        t.start()
        threads.append(t)
        idx += 1
    return threads, results_list, lock


def collect_remote_results(threads, results_list, lock):
    """Wait for remote threads and return list of (name, ok, detail) tuples."""
    idx = 0
    while idx < len(threads):
        threads[idx].join()
        idx += 1
    with lock:
        return list(results_list)


# ── remote server testing (local client → remote server) ──────────────────

REF_SERVER_PROPS = build_paths.build_path(
    "server/epoll/build/testing-filedb/server-properties.xml")


def _detect_maincode(datapack_src):
    """Pick a maincode from the datapack's map/main/ subdirectories.

    Prefer "test" when present — it has ~17 maps vs ~400 for "official", so
    a remote MIPS server can finish parsing in a reasonable timeout. Falls
    back to the first alphabetical maincode otherwise."""
    map_main = os.path.join(datapack_src, "map", "main")
    if not os.path.isdir(map_main):
        return "test"
    entries = sorted(os.listdir(map_main))
    idx = 0
    while idx < len(entries):
        if entries[idx] == "test" and os.path.isdir(os.path.join(map_main, entries[idx])):
            return "test"
        idx += 1
    idx = 0
    while idx < len(entries):
        if os.path.isdir(os.path.join(map_main, entries[idx])):
            return entries[idx]
        idx += 1
    return "test"


def _patch_server_xml(xml, maincode, game_port):
    """Patch key values in server-properties.xml content."""
    import re as _re
    xml = _re.sub(r'mainDatapackCode\s+value="[^"]*"',
                  f'mainDatapackCode value="{maincode}"', xml)
    xml = _re.sub(r'server-port\s+value="[^"]*"',
                  f'server-port value="{game_port}"', xml)
    xml = _re.sub(r'httpDatapackMirror\s+value="[^"]*"',
                  'httpDatapackMirror value=""', xml)
    xml = _re.sub(r'automatic_account_creation\s+value="[^"]*"',
                  'automatic_account_creation value="true"', xml)
    xml = _re.sub(r'<minimize\s+value="[^"]*"',
                  '<minimize value="network"', xml)
    return xml


def setup_remote_server_runtime(host, ssh_port, build_dir, datapack_src,
                                game_port=REMOTE_GAME_PORT):
    """Stage datapack + push patched server-properties.xml to the remote server.

    The datapack is already pre-staged to <work_dir>/../datapack-cache/<id>
    on every LXC at all.sh start (stage_datapacks.py). Instead of re-rsyncing
    ~200 MB host→LXC for every Phase E iteration (which left the host idle
    for ~2 min on each call), we copy locally on the LXC from that cache
    into the build dir. cp from local tmpfs is roughly 100x faster than the
    host→LXC SSH rsync and frees the host CPU to do other work in parallel.
    A copy (not a symlink) is required because we mutate <build_dir>/datapack
    afterwards (prune map/main/<other-maincode>); a symlink would corrupt
    the shared cache for every subsequent test."""
    import datapack_stage as _ds
    # Find the matching node entry so we can resolve its datapack_cache
    # (configured via the per-exec-node `datapack_cache` field; defaults
    # to /home/catchchallenger/datapack-cache).
    bracketed_host = host.split("@", 1)[1] if "@" in host else host
    node = None
    ni = 0
    while ni < len(REMOTE_NODES):
        if REMOTE_NODES[ni].get("ssh", {}).get("host") == bracketed_host:
            node = REMOTE_NODES[ni]
            break
        ni += 1
    cache_root = _ds.DEFAULT_REMOTE_CACHE
    if node is not None:
        # Mirror remote_cache_for() for the node's first execution_node — they
        # all share the same path convention for the local LXC layout.
        exec_nodes = node.get("execution_nodes", [])
        if exec_nodes:
            cache_root = _ds.remote_cache_for(exec_nodes[0])
    cache_path = cache_root.rstrip("/") + "/" + _ds.datapack_id(datapack_src)
    # Wipe any leftover datapack/, then cp -a from the staged cache.
    # `cp -a --reflink=auto` falls back to a regular copy when the FS
    # doesn't support reflinks; on btrfs / xfs the copy is O(metadata).
    cp_cmd = (f"mkdir -p {build_dir} && rm -rf {build_dir}/datapack && "
              f"cp -a --reflink=auto {cache_path} {build_dir}/datapack")
    rc, out = _ssh_cmd(host, ssh_port, cp_cmd,
                       timeout=_ch.clamp_local(RSYNC_TIMEOUT))
    if rc != 0:
        # Fall back to the legacy host→LXC rsync if the cache path is missing
        # (e.g. operator skipped stage_datapacks.py or the cache layout
        # differs on this node). Slow but correct.
        rsync_dest = _rsync_host(host)
        _ssh_cmd(host, ssh_port, f"mkdir -p {build_dir}/datapack", timeout=15)
        rsync_args = [
            "rsync", "-art", "--delete", "--exclude=.git",
            "-e", f"{_ch.RSYNC_SSH_E} -p {ssh_port}",
            datapack_src + "/", f"{rsync_dest}:{build_dir}/datapack/"
        ]
        try:
            p = subprocess.run(rsync_args, timeout=_ch.clamp_local(RSYNC_TIMEOUT),
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        except subprocess.TimeoutExpired:
            return False
        if p.returncode != 0:
            return False

    # read reference server-properties.xml and patch it
    if not os.path.isfile(REF_SERVER_PROPS):
        return False
    with open(REF_SERVER_PROPS, "r") as f:
        xml = f.read()
    maincode = _detect_maincode(datapack_src)
    xml = _patch_server_xml(xml, maincode, game_port)
    #prune map/main/<other_maincode> on the remote so the server only parses
    #the chosen maincode's maps. CatchChallenger-datapack ships ~400 maps
    #under map/main/official; on slow nodes (MIPS) parsing them all blows
    #through the SERVER_READY_TIMEOUT before "correctly bind:" is reached.
    map_main = f"{build_dir}/datapack/map/main"
    prune_cmd = (
        f"if [ -d {map_main} ]; then "
        f"find {map_main} -mindepth 1 -maxdepth 1 -type d "
        f"! -name {maincode} -exec rm -rf {{}} +; fi")
    _ssh_cmd(host, ssh_port, prune_cmd, timeout=60)

    # upload patched XML to remote
    rc, _ = _ssh_cmd(host, ssh_port,
                     f"mkdir -p {build_dir} && cat > {build_dir}/server-properties.xml << 'XMLEOF'\n{xml}XMLEOF",
                     timeout=15)
    if rc != 0:
        return False

    # clear database dir for filedb
    _ssh_cmd(host, ssh_port, f"rm -rf {build_dir}/database", timeout=10)
    return True


def start_remote_server(host, ssh_port, bin_path, build_dir,
                        game_port=REMOTE_GAME_PORT, diag=None,
                        ready_timeout=None):
    """Start a server binary on a remote machine via SSH.

    Returns (ssh_proc, actual_port) — ssh_proc keeps the SSH session alive.
    actual_port is parsed from the "correctly bind:" output, or 0 on failure.

    When `diag` is in valgrind mode, the binary is wrapped via the
    appropriate `valgrind ...` invocation; sanitizer env vars are set
    inline ahead of the binary.
    """
    import re as _re

    # quick existence probe: if the build phase silently dropped the binary
    # (e.g. partial cleanup, build dir on tmpfs that was wiped), report that
    # specifically instead of waiting the full ready_timeout for nothing.
    rc, probe_out = _ssh_cmd(host, ssh_port,
                             f"test -x {bin_path} && echo BINARY_OK || echo BINARY_MISSING",
                             timeout=30)
    if rc != 0 or "BINARY_MISSING" in probe_out:
        import sys
        print(f"remote server binary missing on host: {bin_path}", file=sys.stderr)
        #return a sentinel string instead of None so callers can distinguish
        #binary-missing from ready-timeout (see testingserver.py / testingclient.py).
        return "BINARY_MISSING", 0

    # kill any previous server on that port
    _ssh_cmd(host, ssh_port,
             f"fuser -k {game_port}/tcp 2>/dev/null; "
             f"ss -tlnp 'sport = :{game_port}' 2>/dev/null | grep -oP 'pid=\\K[0-9]+' | xargs -r kill -9 2>/dev/null; "
             f"sleep 1", timeout=10)

    if ready_timeout is None:
        ready_timeout = _diag_mod.scale_timeout(diag, SERVER_READY_TIMEOUT)
    diag_env = _diag_mod.runtime_env(diag)
    env_prefix = " ".join(f"{k}='{v}'" for k, v in diag_env.items())
    if env_prefix:
        env_prefix += " "
    wrapper = " ".join(_diag_mod.runtime_wrapper(diag))
    if wrapper:
        wrapper += " "
    # When CC_GDB=1 is exported on the test box, wrap the remote server in
    # gdb so a SIGSEGV / abort produces a stack trace on stdout (which the
    # SSH session captures into our log) before the process exits. Without
    # this, the only signal of a server crash is the next probe failing
    # with "Connection refused" — useless for root-causing.
    if os.environ.get("CC_GDB") == "1":
        cmd = (f"cd {build_dir} && {env_prefix}{wrapper}gdb -batch "
               f"-ex 'set pagination off' "
               f"-ex 'handle SIGPIPE nostop noprint pass' "
               f"-ex 'run' "
               f"-ex 'thread apply all bt 60' "
               f"-ex 'quit' "
               f"--args {bin_path}")
    else:
        cmd = f"cd {build_dir} && {env_prefix}{wrapper}{bin_path}"
    ssh_args = ["ssh"] + _ch.SSH_OPTS_LIST + ["-p", str(ssh_port), host, cmd]
    ssh_proc = subprocess.Popen(
        ssh_args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        preexec_fn=os.setsid)

    ready = threading.Event()
    output_lines = []
    actual_port = [0]
    # Crash-signature detection.  Without this, a server that SIGBUS'd /
    # SIGSEGV'd mid-handshake (qemu-user MIPS32-BE alignment fault,
    # uninstrumented sanitizer-style crash, glibc abort) would only
    # appear to the test harness as "stateChanged(0) after (1)" — the
    # client's view of a TCP RST.  The actual diagnostic ("qemu:
    # uncaught target signal 10 (Bus error) - core dumped") was buried
    # in the SSH session's stdout that nobody surfaced.  Now: every
    # captured line is checked against CRASH_SIGNATURES and the first
    # match is stashed on the proc object for callers (testingclient.py
    # / testingserver.py) to append into log_fail's detail.
    crash_signature = [None]

    def reader():
        while True:
            raw = ssh_proc.stdout.readline()
            if not raw:
                break
            line = raw.decode(errors="replace").rstrip("\n")
            output_lines.append(line)
            if crash_signature[0] is None:
                idx = 0
                while idx < len(CRASH_SIGNATURES):
                    if CRASH_SIGNATURES[idx] in line:
                        crash_signature[0] = line
                        break
                    idx += 1
            if "correctly bind:" in line:
                m = _re.search(r"port:\s*(\d+)", line)
                if m:
                    actual_port[0] = int(m.group(1))
                else:
                    actual_port[0] = game_port
                ready.set()

    t = threading.Thread(target=reader, daemon=True)
    t.start()

    # Stash output + crash-signature refs on the proc so detect_remote_crash()
    # can read them post-hoc once the in-band test (e.g. local client
    # connect) reports a failure.  The reader thread keeps appending until
    # ssh_proc.stdout closes, so callers see live state.
    ssh_proc._cc_output_lines = output_lines
    ssh_proc._cc_crash_signature = crash_signature

    if ready.wait(timeout=ready_timeout):
        return ssh_proc, actual_port[0]
    # failed — dump what we got (last 60 lines so we capture the part where
    # the server stalled, not just the boot banner)
    import sys
    print(f"remote server start failed, got {len(output_lines)} lines (showing last 60):", file=sys.stderr)
    start = len(output_lines) - 60
    if start < 0:
        start = 0
    idx = start
    while idx < len(output_lines):
        print(f"  | {output_lines[idx]}", file=sys.stderr)
        idx += 1
    # Before killing, try to attach gdb on the remote to capture the
    # call stack of every thread. Per CLAUDE.md "Diagnosing a hung /
    # infinite-looping process — gdb attach, then decide", the bt is
    # what tells us whether the server is wedged on a real bug or
    # just slow to boot. The exec_node's `gdb_attach_timeout` controls
    # how long the runtime waits before giving up on the bt; passing
    # the full per-node value here means bt capture itself is bounded.
    # We resolve via the binary basename so this works whether the
    # remote process is the cli-epoll or any other catchchallenger
    # binary launched from the same SSH session.
    bin_basename = os.path.basename(bin_path)
    remote_gdb_bt_then_kill(host, ssh_port, bin_basename,
                             label=f"start_remote_server timeout @{host}")
    try:
        os.killpg(os.getpgid(ssh_proc.pid), signal.SIGTERM)
    except ProcessLookupError:
        pass
    try:
        ssh_proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(os.getpgid(ssh_proc.pid), signal.SIGKILL)
        except ProcessLookupError:
            pass
    return None, 0


def remote_gdb_bt(host, ssh_port, pid_or_pattern, label="<unknown>",
                  bt_depth=60):
    """SSH into `host` and run `gdb -batch -p <pid> -ex 'thread apply all
    bt <depth>' -ex detach`, returning (rc, captured_text). Used after a
    deadline passes on a process that's still alive — the backtrace is
    the single most useful clue for distinguishing an idle blocked-on-
    syscall process from a genuine hang. See CLAUDE.md "Diagnosing a
    hung / infinite-looping process — gdb attach, then decide".

    `pid_or_pattern` is either an integer-string pid or a `pgrep -f`
    pattern (the helper resolves the pattern to the first matching pid
    on the remote). Returns ("",-1) if gdb is not installed remotely
    or no matching pid is found, so callers stay safe even when the
    backtrace can't be captured."""
    # Resolve pid via pgrep when the caller hands us a pattern.
    pid = pid_or_pattern
    if not str(pid).isdigit():
        rc, out = _ssh_cmd(host, ssh_port,
                           f"pgrep -f {pid_or_pattern!s} 2>/dev/null | head -1",
                           timeout=10)
        pid = (out or "").strip().splitlines()[0] if out else ""
        if not pid:
            return ("", -1)
    rc, out = _ssh_cmd(host, ssh_port,
                       f"command -v gdb >/dev/null || {{ echo '(gdb not installed on remote)'; exit 0; }}; "
                       f"gdb -batch -p {pid} "
                       f"-ex 'set pagination off' "
                       f"-ex 'thread apply all bt {bt_depth}' "
                       f"-ex 'detach' -ex 'quit' 2>&1",
                       timeout=60)
    return (out or "", rc)


def remote_gdb_bt_then_kill(host, ssh_port, pid_or_pattern, label,
                             log_func=None, bt_depth=60):
    """Capture a backtrace, log it under "GDB-BT" lines so it's easy to
    grep out of test output, then SIGKILL the process. Used when a
    deadline-on-exec_node fires; CLAUDE.md
    `gdb_attach_timeout` tunes how long to wait before this fires."""
    out, rc = remote_gdb_bt(host, ssh_port, pid_or_pattern, label, bt_depth)
    if log_func is None:
        import sys
        def _p(line):
            print(line, file=sys.stderr)
        log_func = _p
    log_func(f"  | -- gdb backtrace ({label} @ {host}) rc={rc} --")
    li = 0
    lines = (out or "").splitlines()
    # Trim the gdb chrome but keep the actual backtrace lines.
    while li < len(lines):
        log_func(f"  | GDB-BT: {lines[li]}")
        li += 1
    log_func(f"  | -- end gdb backtrace --")
    # Hard-kill: this fires AFTER the bt capture, so a real hang dies
    # but we still have the diagnostic.
    _ssh_cmd(host, ssh_port,
             f"pkill -KILL -f {pid_or_pattern!s} 2>/dev/null; true",
             timeout=10)


# Substrings whose presence on the remote server's stdout/stderr means
# the server died of a fatal signal / uninstrumented abort, not a clean
# protocol error.  When the in-band client test then reports
# "stateChanged(0)" / "Connection refused" / "TCP probe FAIL", the test
# harness picks one of these lines up via detect_remote_crash() and
# appends it to the failure detail so the operator sees the *real*
# cause (e.g. "qemu: uncaught target signal 10 (Bus error)") instead
# of having to ssh into the node and dig through the SSH session log.
#
# Add new entries when a new strict-alignment / sanitizer / qemu-user
# variant surfaces a different banner.  Order doesn't matter — first
# substring match wins.
CRASH_SIGNATURES = (
    "qemu: uncaught target signal",        # qemu-user (mips, aarch64, ...)
    "Bus error",                            # SIGBUS (alignment, mmap)
    "Segmentation fault",                   # SIGSEGV
    "core dumped",                          # generic shell crash banner
    "AddressSanitizer:",                    # asan
    "LeakSanitizer:",                       # lsan
    "MemorySanitizer:",                     # msan
    "UndefinedBehaviorSanitizer:",          # ubsan
    "ThreadSanitizer:",                     # tsan
    "==ERROR:",                             # generic *san banner
    "*** stack smashing detected",          # libssp
    "double free or corruption",            # glibc malloc
    "free(): invalid pointer",              # glibc malloc
    "munmap_chunk(): invalid pointer",      # glibc malloc
    "*** buffer overflow detected",         # glibc _FORTIFY_SOURCE
    "terminate called",                     # uncaught C++ exception
    "Aborted (core dumped)",                # SIGABRT shell banner
)


def detect_remote_crash(ssh_proc):
    """Return the first crash-signature line seen on the remote server's
    stdout/stderr stream, or None.  Called by testingclient.py /
    testingserver.py after their in-band TCP probe / state-machine check
    fails — turns "stateChanged(0) after (1)" into "stateChanged(0) after
    (1); remote crash: qemu: uncaught target signal 10 (Bus error) -
    core dumped".  Safe on any proc object; returns None when the proc
    isn't one of ours or the reader thread hasn't observed a crash yet.

    No new SSH session is opened — the reader thread launched by
    start_remote_server() is the one populating the buffer this reads.
    Callers should give the SSH stream a small grace period (e.g. 1 s)
    after seeing the client-side disconnect before reading, so any
    last-line crash banner has time to land in the pipe."""
    sig = getattr(ssh_proc, "_cc_crash_signature", None)
    if sig is None:
        return None
    return sig[0]


def stop_remote_server(ssh_proc, host, ssh_port, game_port=REMOTE_GAME_PORT):
    """Stop a remote server started with start_remote_server()."""
    if ssh_proc is not None:
        try:
            os.killpg(os.getpgid(ssh_proc.pid), signal.SIGTERM)
        except ProcessLookupError:
            pass
        try:
            ssh_proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(os.getpgid(ssh_proc.pid), signal.SIGKILL)
            except ProcessLookupError:
                pass
    # also kill on remote side
    _ssh_cmd(host, ssh_port,
             f"fuser -k {game_port}/tcp 2>/dev/null; "
             f"ss -tlnp 'sport = :{game_port}' 2>/dev/null | grep -oP 'pid=\\K[0-9]+' | xargs -r kill -9 2>/dev/null",
             timeout=10)


def get_remote_server_address(host):
    """Get the network address to connect a local client to a remote server."""
    return _host_address(host)


def cleanup_remote_node(host, ssh_port, remote_dir):
    """Public wrapper around _cleanup_remote.

    Test scripts that spin up a server with start_remote_server() should call
    this once they are done, so the work dir does not linger on the node.
    """
    return _cleanup_remote(host, ssh_port, remote_dir)


# ── remote --autosolo testing (build + run client on the node) ─────────────

# .pro files for the two clients tested in the solo phase. Build dir is the
# basename without ".pro", same convention as _build_pro_remote.
#both .pro files set TARGET=catchchallenger, so the produced binary is named
#"catchchallenger" regardless of the .pro name. The label in the test name is
#derived from the .pro name elsewhere, so each entry just records the .pro path.
SOLO_CLIENT_PROS = [
    ("client/qtcpu800x600/qtcpu800x600.pro", "catchchallenger"),
    ("client/qtopengl/catchchallenger-qtopengl.pro", "catchchallenger"),
]


def _setup_remote_client_datapack(host, ssh_port, build_dir, datapack_src,
                                   maincode, control_path=None):
    """Rsync datapack to {build_dir}/datapack/internal/, then prune map/main/
    to keep only the requested maincode (matches setup_datapack_client in
    testingclient.py)."""
    rsync_dest = _rsync_host(host)
    _ssh_cmd(host, ssh_port, f"mkdir -p {build_dir}/datapack/internal",
             timeout=15, control_path=control_path)
    ssh_e = f"{_ch.RSYNC_SSH_E} -p {ssh_port}"
    if control_path is not None:
        ssh_e += f" -o ControlPath={control_path}"
    rsync_args = [
        "rsync", "-az", "--delete", "--exclude=.git",
        "-e", ssh_e,
        datapack_src + "/", f"{rsync_dest}:{build_dir}/datapack/internal/"
    ]
    try:
        p = subprocess.run(rsync_args,
                           timeout=_ch.clamp_local(RSYNC_TIMEOUT),
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if p.returncode != 0:
            return False, p.stdout.decode(errors="replace")[-300:]
    except subprocess.TimeoutExpired:
        return False, f"TIMEOUT after {RSYNC_TIMEOUT}s"
    map_main = f"{build_dir}/datapack/internal/map/main"
    prune_cmd = (
        f"if [ -d {map_main} ]; then "
        f"find {map_main} -mindepth 1 -maxdepth 1 -type d "
        f"! -name {maincode} -exec rm -rf {{}} +; fi")
    _ssh_cmd(host, ssh_port, prune_cmd, timeout=30, control_path=control_path)
    return True, ""


def _run_remote_autosolo(host, ssh_port, bin_path, build_dir, x11_config,
                          client_run_mode, timeout=60,
                          success_marker="MapVisualiserPlayer::mapDisplayedSlot()",
                          diag=None, control_path=None):
    """Launch one client binary with --autosolo on the remote node.

    client_run_mode='x11'       → run on the node with DISPLAY from x11_config
    client_run_mode='offscreen' → run on the node with QT_QPA_PLATFORM=offscreen
    Anything else returns a clear "skipped" detail.

    Returns (ok, detail) where ok is True on success_marker hit + clean exit.
    """
    timeout = _diag_mod.scale_timeout(diag, timeout)
    diag_env = _diag_mod.runtime_env(diag)
    diag_env_prefix = " ".join(f"{k}={v}" for k, v in diag_env.items())
    if diag_env_prefix:
        diag_env_prefix += " "
    wrapper = " ".join(_diag_mod.runtime_wrapper(diag))
    if wrapper:
        wrapper += " "
    # Wrap the binary in `timeout(1)` on the remote side so that when the
    # SSH-side timeout below fires, `setsid + timeout --kill-after` ensures
    # the remote process actually dies. Without this, Qt clients ignore
    # SIGHUP from the closing SSH session and pile up as zombies (we hit
    # this: 2h-old leaked autosolos on x86-lxc).
    # Add a few seconds of slack so the local SSH timeout fires AFTER the
    # remote `timeout` has had a chance to produce a clean exit code.
    remote_kill_grace = 5
    remote_timeout_secs = max(int(timeout) - remote_kill_grace, 5)
    remote_timeout = f"timeout --kill-after=3s {remote_timeout_secs}s "
    # Build the env prefix and the bare-binary invocation; we'll wrap with
    # gdb either upfront (CC_GDB=1) or only on the rerun-after-failure path.
    #
    # Core dumps: every shell snippet starts with `ulimit -c unlimited`
    # plus a per-test core directory and `kernel.core_pattern` set to drop
    # cores there. This is the only way to capture a SEGV that won't
    # reproduce under gdb (the qtopengl c++23 heisenbug). `sysctl` needs
    # root — fine on every node we own. On non-root nodes the sysctl call
    # silently fails and ulimit alone still puts the core in $PWD.
    core_dir = "/tmp/cc-cores"
    core_setup = (
        f"mkdir -p {core_dir} 2>/dev/null && rm -f {core_dir}/core.* 2>/dev/null && "
        f"(sysctl -w kernel.core_pattern={core_dir}/core.%e.%p 2>/dev/null || "
        f"echo '{core_dir}/core.%e.%p' > /proc/sys/kernel/core_pattern 2>/dev/null || true) && "
        f"(sysctl -w kernel.core_uses_pid=1 2>/dev/null || true) && "
        f"ulimit -c unlimited && ")
    if client_run_mode == "x11":
        if not x11_config or not x11_config.get("display"):
            return False, "x11 mode but no x11.display configured"
        env_prefix = (f"DISPLAY={x11_config['display']} "
                      f"XDG_RUNTIME_DIR=/tmp/runtime-autosolo "
                      f"mkdir -p /tmp/runtime-autosolo 2>/dev/null; "
                      f"DISPLAY={x11_config['display']} {diag_env_prefix}")
    elif client_run_mode == "offscreen":
        qpa = os.environ.get("CC_QPA", "offscreen")
        env_prefix = f"QT_QPA_PLATFORM={qpa} {diag_env_prefix}"
    else:
        return False, f"client_run_mode={client_run_mode!r} cannot run a client"
    # Prepend the core-dump setup once for every command-string we
    # emit on this node — easier than weaving it into env_prefix below.
    env_prefix = core_setup + env_prefix

    bare_run = f"{wrapper}{bin_path} --autosolo --closewhenonmap 2>&1"
    gdb_run  = (f"{wrapper}gdb -batch "
                f"-ex 'set pagination off' "
                f"-ex 'handle SIGPIPE nostop noprint pass' "
                f"-ex 'run --autosolo --closewhenonmap' "
                f"-ex 'thread apply all bt 60' "
                f"-ex 'quit' "
                f"--args {bin_path} 2>&1")

    force_gdb = os.environ.get("CC_GDB") == "1"
    first_run = gdb_run if force_gdb else bare_run
    run_cmd = f"cd {build_dir} && {env_prefix}{remote_timeout}{first_run}"
    rc, out = _ssh_cmd(host, ssh_port, run_cmd, timeout=timeout,
                       control_path=control_path)
    failed_marker = bool(success_marker and success_marker not in out)

    # ── core-dump post-mortem (preferred path for non-deterministic SEGV) ───
    # Heisenbugs that vanish under gdb (different scheduling, libc malloc
    # debug, ptrace overhead) still leave a core file with `ulimit -c
    # unlimited` set above. Try that FIRST — if a core exists, dump its
    # bt and we're done; the bt is far more reliable than re-running
    # under gdb and hoping the bug resurfaces.
    core_bt = ""
    if rc != 0 or failed_marker:
        bin_basename = os.path.basename(bin_path)
        # Find the most recent core file matching this binary; print
        # `gdb -batch -ex 'thread apply all bt 60' <bin> <core>`.
        core_glob = f"{core_dir}/core.{bin_basename}*"
        core_cmd = (
            f"latest=$(ls -1t {core_glob} 2>/dev/null | head -1); "
            f"if [ -n \"$latest\" ] && [ -s \"$latest\" ]; then "
            f"  echo \"__CORE_FOUND__:$latest\"; "
            f"  gdb -batch -ex 'set pagination off' "
            f"      -ex 'thread apply all bt 60' "
            f"      -ex 'quit' {bin_path} \"$latest\" 2>&1 | tail -200; "
            f"else echo '__NO_CORE__'; fi")
        rc_core, out_core = _ssh_cmd(host, ssh_port, core_cmd, timeout=120,
                                     control_path=control_path)
        if "__CORE_FOUND__" in out_core:
            core_bt = f"\n--- core dump bt (post-mortem from core file) ---\n{out_core}"

    # Auto-rerun under gdb when the bare run failed AND we don't already have
    # a core-file bt. SEGV (rc=139), abort (rc=134), missing-marker, or any
    # non-zero exit. Captures a real backtrace from the dying process —
    # `remote_gdb_bt_then_kill` below only helps a still-alive wedged
    # process, useless for SEGV.
    bt_tail = core_bt
    if (failed_marker or rc != 0) and not force_gdb and not core_bt:
        # Verify gdb is on the remote $PATH before re-running. Without it
        # the rerun would just produce a "gdb: command not found" line and
        # the operator would think the test produced no diagnostic at all.
        rc_probe, out_probe = _ssh_cmd(host, ssh_port,
                                       "command -v gdb || echo __NO_GDB__",
                                       timeout=15, control_path=control_path)
        if "__NO_GDB__" in out_probe or rc_probe != 0:
            bt_tail = (f"\n--- gdb rerun skipped: gdb not found on {host} "
                       f"(install gdb on the remote node to capture a backtrace) ---")
        else:
            rerun_cmd = f"cd {build_dir} && {env_prefix}{remote_timeout}{gdb_run}"
            rc2, out2 = _ssh_cmd(host, ssh_port, rerun_cmd, timeout=timeout,
                                 control_path=control_path)
            # Carve out the actually-useful slice of gdb's output: from the
            # first SIGSEGV / SIGABRT / "Program received signal" line to
            # end-of-output. Falls back to the last 80 lines when the rerun
            # didn't crash at all (heisenbug — surfaces that for the operator).
            lines = out2.splitlines()
            seg_idx = -1
            i = 0
            while i < len(lines):
                low = lines[i].lower()
                if ("received signal" in low or "segmentation fault" in low
                        or "sigabrt" in low or "sigsegv" in low):
                    seg_idx = i
                    break
                i += 1
            if seg_idx >= 0:
                gdb_tail = "\n".join(lines[seg_idx:seg_idx + 200])
                bt_tail = f"\n--- gdb rerun (rc={rc2}, signal at line {seg_idx}) ---\n{gdb_tail}"
            else:
                # Heisenbug — didn't reproduce under gdb. Burn a few BARE
                # retries (no gdb) to grab a real core file. Each retry
                # gets the same env/ulimit/core_pattern as the original
                # run, so a SEGV will land in {core_dir}.
                gdb_tail_msg = "\n".join(lines[-40:])
                heisen_attempts = int(os.environ.get("CC_HEISEN_RETRIES", "5"))
                heisen_caught = False
                attempt = 0
                while attempt < heisen_attempts:
                    attempt += 1
                    rc_h, _ = _ssh_cmd(host, ssh_port,
                                       f"cd {build_dir} && {env_prefix}{remote_timeout}{bare_run}",
                                       timeout=timeout, control_path=control_path)
                    if rc_h == 139 or rc_h == 134 or rc_h == 6:
                        heisen_caught = True
                        break
                if heisen_caught:
                    core_glob2 = f"{core_dir}/core.{os.path.basename(bin_path)}*"
                    core_cmd2 = (
                        f"latest=$(ls -1t {core_glob2} 2>/dev/null | head -1); "
                        f"if [ -n \"$latest\" ] && [ -s \"$latest\" ]; then "
                        f"  echo \"__CORE_FOUND__:$latest\"; "
                        f"  gdb -batch -ex 'set pagination off' "
                        f"      -ex 'thread apply all bt 60' "
                        f"      -ex 'quit' {bin_path} \"$latest\" 2>&1 | tail -200; "
                        f"else echo '__NO_CORE__'; fi")
                    _, out_c2 = _ssh_cmd(host, ssh_port, core_cmd2, timeout=120,
                                         control_path=control_path)
                    if "__CORE_FOUND__" in out_c2:
                        bt_tail = (f"\n--- gdb rerun (rc={rc2}) did not reproduce; "
                                   f"caught SEGV after {attempt}/{heisen_attempts} bare retries; "
                                   f"core-dump bt below ---\n{out_c2}")
                    else:
                        bt_tail = (f"\n--- gdb rerun (rc={rc2}) clean; "
                                   f"caught SEGV (rc={rc_h}) on attempt {attempt}/{heisen_attempts} "
                                   f"but no core file produced (check ulimit/permissions) ---\n"
                                   f"{gdb_tail_msg}")
                else:
                    bt_tail = (f"\n--- gdb rerun (rc={rc2}, no signal); also did "
                               f"{heisen_attempts} bare retries, none crashed — failure is "
                               f"highly flaky (1-in-many) or only triggers under "
                               f"the original test pacing. Set CC_HEISEN_RETRIES=N "
                               f"to extend; consider --sanitize asan ---\n"
                               f"{gdb_tail_msg}")
        # Some runs wedge instead of crashing. After the gdb rerun, also
        # try attaching to any stragglers (the original wedge, not the
        # rerun's clean-exited gdb child).
        bin_basename = os.path.basename(bin_path)
        remote_gdb_bt_then_kill(host, ssh_port, bin_basename,
                                 label=f"_run_remote_autosolo @{host}")
    elif failed_marker or rc != 0:
        # CC_GDB=1: first run already had gdb; just kill stragglers.
        bin_basename = os.path.basename(bin_path)
        remote_gdb_bt_then_kill(host, ssh_port, bin_basename,
                                 label=f"_run_remote_autosolo @{host}")

    if failed_marker:
        tail = "\n".join(out.splitlines()[-15:])
        return False, f"missing success marker (rc={rc})\n{tail}{bt_tail}"
    if rc != 0:
        tail = "\n".join(out.splitlines()[-15:])
        return False, f"exit rc={rc}\n{tail}{bt_tail}"
    return True, "marker hit, exit 0"


def run_remote_autosolo_phase(node, datapack_src, maincode=None, diag=None):
    """End-to-end solo phase for one node: rsync source, build both clients,
    rsync datapack, run each with --autosolo. Returns a list of
    (test_name, ok, detail) tuples. Cleans up the work dir on exit.

    Does nothing (returns a single skip entry) when the node does not opt
    into 'solo', or when its client_run_mode is 'none' / has_gui is false
    (Qt GUI modules are required to even build the clients).

    When `diag` is set, the build uses the diagnostic compiler/flags and
    the autosolo run is wrapped (sanitizer env or valgrind). Nodes whose
    `compilers` list does not include the required compiler are skipped.
    """
    label = node["label"]
    host = f"{node['ssh']['user']}@{node['ssh']['host']}"
    ssh_port = node["ssh"]["port"]
    remote_dir = node["work_dir"]
    use_mold = _node_linker(node)
    extra_defines = node.get("extra_defines", [])
    # Compile-side has_gui still gates whether the Qt GUI .pro files are
    # buildable here (find_package(Qt6 REQUIRED) needs the headers).
    compile_has_gui = node.get("has_gui", True)
    # Runtime flags moved to execution_nodes — pick the first enabled
    # entry. Fall back to None when nothing is enabled, which causes the
    # `client_run_mode == "none"` branch below to skip the phase.
    runtime_node = None
    enodes = node.get("execution_nodes", []) or []
    eidx = 0
    while eidx < len(enodes):
        if enodes[eidx].get("enabled", True):
            runtime_node = enodes[eidx]
            break
        eidx += 1
    if runtime_node is None:
        run_has_gui = False
        client_run_mode = "none"
        x11_config = None
    else:
        run_has_gui = runtime_node.get("has_gui", True)
        client_run_mode = runtime_node.get("client_run_mode", "none")
        x11_config = runtime_node.get("x11")
    has_gui = compile_has_gui and run_has_gui
    raw_cxx = node.get("cxx_version", [])
    if raw_cxx is None:
        cxx_versions = []
    elif isinstance(raw_cxx, list):
        cxx_versions = list(raw_cxx)
    else:
        cxx_versions = [raw_cxx]
    # one build pass with qmake default when the list is empty
    passes = cxx_versions if cxx_versions else [None]

    results = []
    if not has_gui:
        # Expected skip — Qt GUI modules are required to build the client,
        # not a real failure.  Reporting as PASS keeps it out of failed.json.
        results.append((f"solo {label}", True,
                        "skipped: node has no GUI (Qt widgets/opengl absent)"))
        return results
    if client_run_mode == "none":
        # Expected skip — node deliberately opts out of running clients.
        results.append((f"solo {label}", True,
                        "skipped: client_run_mode='none' on this node"))
        return results
    if not _diag_mod.node_supports(node, diag):
        results.append((f"solo {label}", True,
                        f"node has no {_diag_mod.compiler_name(diag)}, "
                        f"skipping diagnostic run"))
        return results

    if maincode is None:
        maincode = _detect_maincode(datapack_src)

    cp = _open_ssh_master(host, ssh_port,
                          f"solo phase on {label} (autosolo for both clients)")
    try:
        ok, out = _rsync_to_remote(host, ssh_port, remote_dir, control_path=cp)
        if not ok:
            results.append((f"solo {label}: rsync source", False,
                            out.strip()[:300]))
            return results
        results.append((f"solo {label}: rsync source", True, ""))

        qmake = _detect_qmake(host, ssh_port, control_path=cp)
        if qmake is None:
            results.append((f"solo {label}: detect qmake", False,
                            "qmake6/qmake not found"))
            return results
        need_stash = _need_qmake_stash(host, ssh_port, control_path=cp)

        ok, out = _setup_remote_client_datapack(host, ssh_port, remote_dir,
                                                 datapack_src, maincode,
                                                 control_path=cp)
        if not ok:
            results.append((f"solo {label}: rsync datapack", False, out))
            return results
        results.append((f"solo {label}: rsync datapack", True,
                        f"maincode={maincode}"))

        pi = 0
        while pi < len(passes):
            cxx_ver = passes[pi]
            ci = 0
            while ci < len(SOLO_CLIENT_PROS):
                pro_rel, bin_name = SOLO_CLIENT_PROS[ci]
                # Pass the raw label; _build_pro_remote already appends
                # the cxx_version when set (avoids "x86-lxc c++11 c++11").
                build_res = _build_pro_remote(host, ssh_port, qmake, pro_rel,
                                              label, use_mold,
                                              extra_defines, need_stash,
                                              cxx_ver, remote_dir, diag=diag,
                                              control_path=cp)
                results.append(build_res)
                if not build_res[1]:
                    ci += 1
                    continue
                base_name = os.path.basename(pro_rel).replace('.pro', '')
                _diag_suffix = _diag_mod.build_dir_suffix(diag)
                if cxx_ver:
                    build_dir = f"{remote_dir}/build-remote{_diag_suffix}/{base_name}-{cxx_ver}"
                else:
                    build_dir = f"{remote_dir}/build-remote{_diag_suffix}/{base_name}"
                # the client binary expects datapack at <cwd>/datapack/internal,
                # and we set cwd to the build dir, so symlink it from there.
                _ssh_cmd(host, ssh_port,
                         f"ln -sfn {remote_dir}/datapack {build_dir}/datapack",
                         timeout=10, control_path=cp)
                # Self-contained per-binary CMakeLists.txt: the executable
                # lands at <build_dir>/<binary> directly (no nested output
                # subdir). `bin_name` is the CMake OUTPUT_NAME — qtcpu800x600's
                # target is `catchchallenger800x600` but its
                # set_target_properties(... OUTPUT_NAME catchchallenger)
                # makes the on-disk file just `catchchallenger`.
                _bin_rel = f"./{bin_name}"
                run_label = f"solo {label} --autosolo {bin_name}"
                if cxx_ver:
                    run_label += f" ({cxx_ver})"
                run_ok, detail = _run_remote_autosolo(
                    host, ssh_port, _bin_rel, build_dir,
                    x11_config, client_run_mode, diag=diag, control_path=cp)
                results.append((run_label, run_ok, detail))
                ci += 1
            pi += 1
    finally:
        try:
            _cleanup_remote(host, ssh_port, remote_dir, control_path=cp)
        finally:
            _close_ssh_master(host, ssh_port, cp,
                              reason=f"solo phase on {label} done")
    return results
