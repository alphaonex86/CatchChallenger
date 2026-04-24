#!/usr/bin/env python3
"""
testingremote.py — Remote compilation testing on Debian 12 servers.

Opens a single SSH connection per server, uploads sources via rsync,
and compiles each .pro file with c++11, no EXTRA CatchChallenger flags.

Servers:
  - 982.vps.confiared.com:18016  (x86_64, uses mold linker)
  - [2803:1920::3:440a]          (MIPS, no mold linker)
"""

import os, sys, subprocess, json

# ── config ─────────────────────────────────────────────────────────────────
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Remote servers: (label, ssh_host, ssh_port, use_mold)
SERVERS = [
    ("debian12-x86", "root@982.vps.confiared.com", 18016, True),
    ("debian12-mips", "root@2803:1920::3:440a", 22, False),
]

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

def log_info(msg):
    print(f"{C_CYAN}[INFO]{C_RESET} {msg}")

def log_pass(name, detail=""):
    results.append((name, True, detail))
    print(f"{C_GREEN}[PASS]{C_RESET} {name}  {detail}")

def log_fail(name, detail=""):
    results.append((name, False, detail))
    print(f"{C_RED}[FAIL]{C_RESET} {name}  {detail}")


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


def rsync_to_remote(host, port):
    """Upload source tree to remote server."""
    log_info(f"rsync to {host}:{REMOTE_DIR}")
    rsync_args = [
        "rsync", "-az", "--delete",
        "--exclude=.git", "--exclude=build/", "--exclude=*.o",
        "--exclude=moc_*", "--exclude=Makefile",
        "-e", f"ssh -o StrictHostKeyChecking=no -o BatchMode=yes -p {port}",
        ROOT + "/", f"{host}:{REMOTE_DIR}/"
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


def detect_qmake(host, port):
    """Detect qmake6 or qmake on remote server."""
    rc, out = ssh_cmd(host, port, "which qmake6 2>/dev/null || which qmake 2>/dev/null", timeout=15)
    if rc == 0 and out.strip():
        qmake = out.strip().splitlines()[0]
        log_info(f"detected qmake: {qmake}")
        return qmake
    log_fail(f"detect qmake on {host}", "qmake6/qmake not found")
    return None


def build_pro_remote(host, port, qmake, pro_rel, label, use_mold):
    """Build a .pro file on the remote server."""
    pro_path = f"{REMOTE_DIR}/{pro_rel}"
    build_dir = f"{REMOTE_DIR}/build-remote/{os.path.basename(pro_rel).replace('.pro', '')}"

    mold_flags = ""
    if use_mold:
        mold_flags = '"QMAKE_LFLAGS+=-fuse-ld=mold" "LIBS+=-fuse-ld=mold"'

    cmd = (
        f"mkdir -p {build_dir} && "
        f"cd {build_dir} && "
        f"make distclean 2>/dev/null; "
        f"{qmake} -o Makefile {pro_path} "
        f"-spec linux-g++ CONFIG+=debug "
        f"\"QMAKE_CXXFLAGS+=-std={CXX_VERSION}\" "
        f"\"QMAKE_CXXFLAGS+=-g\" \"QMAKE_CFLAGS+=-g\" \"QMAKE_LFLAGS+=-g\" "
        f"{mold_flags} "
        f"2>&1 && "
        f"make -j$(nproc) 2>&1"
    )
    log_info(f"building {pro_rel} on {label}")
    rc, out = ssh_cmd(host, port, cmd)
    name = f"compile {pro_rel} ({label})"
    if rc == 0:
        log_pass(name)
        return True
    log_fail(name, f"rc={rc}")
    lines = out.splitlines()
    # print last 30 lines of output for diagnosis
    start = len(lines) - 30
    if start < 0:
        start = 0
    idx = start
    while idx < len(lines):
        print(f"  | {lines[idx]}")
        idx += 1
    return False


def test_server(label, host, port, use_mold):
    """Run all compilation tests on a single remote server."""
    print(f"\n{C_CYAN}{'='*60}")
    print(f"  Testing on: {label} ({host}:{port})")
    print(f"{'='*60}{C_RESET}\n")

    if not rsync_to_remote(host, port):
        return

    qmake = detect_qmake(host, port)
    if qmake is None:
        return

    idx = 0
    while idx < len(PRO_FILES):
        build_pro_remote(host, port, qmake, PRO_FILES[idx], label, use_mold)
        idx += 1


def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — Remote Compilation Testing (Debian 12)")
    print(f"{'='*60}{C_RESET}\n")

    idx = 0
    while idx < len(SERVERS):
        label, host, port, use_mold = SERVERS[idx]
        test_server(label, host, port, use_mold)
        idx += 1

    # summary
    print(f"\n{C_CYAN}{'='*60}")
    print("  Summary")
    print(f"{'='*60}{C_RESET}")
    passed = sum(1 for _, ok, _ in results if ok)
    failed = sum(1 for _, ok, _ in results if not ok)
    idx = 0
    while idx < len(results):
        name, ok, detail = results[idx]
        tag = f"{C_GREEN}PASS{C_RESET}" if ok else f"{C_RED}FAIL{C_RESET}"
        print(f"  [{tag}] {name}  {detail}")
        idx += 1
    print()
    print(f"  {C_GREEN}{passed} passed{C_RESET}, {C_RED}{failed} failed{C_RESET}")
    if failed:
        sys.exit(1)


if __name__ == "__main__":
    main()
