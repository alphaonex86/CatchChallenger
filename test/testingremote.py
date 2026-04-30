#!/usr/bin/env python3
"""
testingremote.py — Remote compilation testing on the CC test nodes.

Opens a single SSH connection per node, uploads sources via rsync,
compiles each .pro file with c++11 / no EXTRA CatchChallenger flags,
then removes the work dir on the remote node.

Per-node config is loaded from test/remote_nodes.json via remote_build.
Only nodes with "compile" in their `tests` list are exercised here.
"""

import os, sys, subprocess, json, time
from remote_build import REMOTE_NODES
import diagnostic

DIAG = diagnostic.parse_diag_args()

# ── config ─────────────────────────────────────────────────────────────────
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Build the (label, ssh_host, ssh_port, use_mold, remote_dir) tuple list
# from the shared JSON, filtering on the "compile" test opt-in.
SERVERS = []
for _n in REMOTE_NODES:
    if "compile" not in _n.get("tests", []):
        continue
    SERVERS.append((
        _n["label"],
        f"{_n['ssh']['user']}@{_n['ssh']['host']}",
        _n["ssh"]["port"],
        _n.get("has_mold", False),
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

# Per-node remote_dir is stored in SERVERS; the constant below is kept only
# as a fallback for ad-hoc callers — it is not used in the loops below.
REMOTE_DIR = "/tmp/catchchallenger-test"
CXX_VERSION = "c++11"
COMPILE_TIMEOUT = 600
RSYNC_TIMEOUT = 300
NICE_PREFIX = ["nice", "-n", "19"]

# ── colours ─────────────────────────────────────────────────────────────────
C_GREEN = "\033[92m"
C_RED = "\033[91m"
C_CYAN = "\033[96m"
C_RESET = "\033[0m"

results = []
_last_log_time = [time.monotonic()]

SCRIPT_NAME = os.path.basename(__file__)
FAILED_JSON = "/mnt/data/perso/tmpfs/failed.json"


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


def ssh_cmd(host, port, command, timeout=COMPILE_TIMEOUT):
    """Run a command over SSH, return (returncode, output)."""
    ssh_args = ["ssh", "-o", "StrictHostKeyChecking=no",
                "-o", "BatchMode=yes", "-p", str(port),
                host, command]
    try:
        p = subprocess.run(ssh_args, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"TIMEOUT after {timeout}s"


def _rsync_host(host):
    """Wrap IPv6 addresses in brackets for rsync (e.g. user@[2803::1])."""
    if "@" in host:
        u, addr = host.split("@", 1)
        if ":" in addr and not addr.startswith("["):
            return f"{u}@[{addr}]"
    return host


def rsync_to_remote(host, port, remote_dir):
    """Upload source tree to remote server."""
    log_info(f"rsync to {host}:{remote_dir}")
    # ssh user accounts may not have the parent dir yet
    ssh_cmd(host, port, f"mkdir -p {remote_dir}", timeout=15)
    rsync_args = [
        "rsync", "-az", "--delete",
        "--exclude=.git", "--exclude=build/", "--exclude=*.o",
        "--exclude=moc_*", "--exclude=Makefile",
        "-e", f"ssh -o StrictHostKeyChecking=no -o BatchMode=yes -p {port}",
        ROOT + "/", f"{_rsync_host(host)}:{remote_dir}/"
    ]
    try:
        p = subprocess.run(rsync_args, timeout=RSYNC_TIMEOUT,
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
    cmake_helpers.py."""
    import cmake_helpers as _ch
    suffix = diagnostic.build_dir_suffix(DIAG)
    name = f"compile {pro_rel} ({label})"
    try:
        target, configure_flags, _output_subdir = _ch.pro_to_cmake_target(pro_rel)
    except KeyError:
        log_fail(name, f"no cmake target mapping for {pro_rel}")
        return False
    build_dir = f"{remote_dir}/build-remote{suffix}/{target}"
    mold_args = ""
    if use_mold:
        mold_args = "-DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=mold"
    flag_args = " ".join(configure_flags)
    cmd = (
        f"mkdir -p {build_dir} && "
        f"{cmake} -S {remote_dir} -B {build_dir} "
        f"-DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD={CXX_VERSION.replace('c++','')} "
        f"{flag_args} {mold_args} "
        f"2>&1 && "
        f"{cmake} --build {build_dir} --target {target} -j$(nproc) 2>&1"
    )
    log_info(f"building {target} on {label}")
    rc, out = ssh_cmd(host, port, cmd)
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
    """Remove the work dir on a remote node after tests."""
    if not remote_dir or len(remote_dir) < 6 or remote_dir == "/":
        return
    ssh_cmd(host, port, f"rm -rf {remote_dir}", timeout=60)


def test_server(label, host, port, use_mold, remote_dir, failed_cases=None):
    """Run all compilation tests on a single remote server then clean up."""
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
    finally:
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
        test_server(label, host, port, use_mold, remote_dir, failed_cases)
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
