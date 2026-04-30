#!/usr/bin/env python3
"""
testingcompilationmac.py — CatchChallenger macOS cross-compile + run test.

Steps:
  1. Probe the qemu mac VM (192.168.158.34) over ssh.
  2. rsync the source tree to the VM.
  3. qmake -spec macx-clang + make -j$(sysctl -n hw.ncpu) for qtcpu800x600
     and qtopengl on the VM.
  4. Optional --autosolo and multi-mode runs (latter uses the local
     server-filedb that testingserver.py / testingclient.py already built).

Self-skips when the VM is unreachable. The mac VM is shared with other
projects (per CLAUDE.md memory): never modify the VM's Qt or environment.
This script is the sibling of testingcompilationandroid.py — both used to
live inside testingclient.py; extracted so they can be invoked / scheduled
independently.
"""

import os, sys, signal, subprocess, threading, json, re, time

# ── config ─────────────────────────────────────────────────────────────────
_CONFIG_PATH = os.path.join(os.path.expanduser("~"),
                            ".config", "catchchallenger-testing", "config.json")
with open(_CONFIG_PATH, "r") as _f:
    _config = json.load(_f)

# ── paths ───────────────────────────────────────────────────────────────────
ROOT          = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATAPACKS     = _config["paths"]["datapacks"]

CLIENT_CPU_PRO = os.path.join(ROOT, "client/qtcpu800x600/qtcpu800x600.pro")
CLIENT_GL_PRO  = os.path.join(ROOT, "client/qtopengl/catchchallenger-qtopengl.pro")

#local server-filedb that the multi-mode mac client connects back to.
SERVER_BUILD    = os.path.join(ROOT, "server/epoll/build/testing-filedb")
SERVER_BIN_NAME = "catchchallenger-server-cli-epoll"

SERVER_HOST_REMOTE = "192.168.158.10"  # host LAN IP reachable from the mac VM
SERVER_PORT        = str(_config["server_port"])

# ── macOS qemu VM (ssh) ────────────────────────────────────────────────────
MAC_HOST          = "192.168.158.34"
MAC_USER          = "user"
MAC_QT            = "/Users/user/Qt/6.8.0/macos"
MAC_QMAKE         = MAC_QT + "/bin/qmake"
MAC_WORK_DIR      = "/Users/user/Desktop/catchchallenger-test"
MAC_SSH_PROBE     = 8
MAC_RUN_TIMEOUT   = 120
MAC_RSYNC_TIMEOUT = 600
MAC_APP_NAME      = "catchchallenger.app"
COMPILE_TIMEOUT   = 600
SERVER_READY_TIMEOUT = 60

# ── colours ─────────────────────────────────────────────────────────────────
C_GREEN = "\033[92m"
C_RED   = "\033[91m"
C_CYAN  = "\033[96m"
C_RESET = "\033[0m"

results = []
_last_log_time = [time.monotonic()]
total_expected = [0]
server_proc = None

SCRIPT_NAME = os.path.basename(__file__)
FAILED_JSON = "/mnt/data/perso/tmpfs/failed.json"


def load_failed_cases():
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
    if failed_cases is None:
        return True
    idx = 0
    while idx < len(failed_cases):
        if failed_cases[idx] == test_name:
            return True
        idx += 1
    return False


def save_failed_cases():
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
    if len(results) > total_expected[0]:
        total_expected[0] = len(results)
    print(f"{C_GREEN}[PASS]{C_RESET} {len(results)}/{total_expected[0]} {name}  {detail}  ({elapsed:.1f}s)")


def log_fail(name, detail=""):
    now = time.monotonic()
    elapsed = now - _last_log_time[0]
    _last_log_time[0] = now
    results.append((name, False, detail, elapsed))
    if len(results) > total_expected[0]:
        total_expected[0] = len(results)
    print(f"{C_RED}[FAIL]{C_RESET} {len(results)}/{total_expected[0]} {name}  {detail}  ({elapsed:.1f}s)")


# ── helpers ─────────────────────────────────────────────────────────────────
def detect_maincodes(datapack_src):
    map_main = os.path.join(datapack_src, "map", "main")
    if not os.path.isdir(map_main):
        return []
    out = []
    entries = sorted(os.listdir(map_main))
    idx = 0
    while idx < len(entries):
        if os.path.isdir(os.path.join(map_main, entries[idx])):
            out.append(entries[idx])
        idx += 1
    return out


def mac_ssh(cmd, timeout=COMPILE_TIMEOUT):
    args = ["ssh",
            "-o", "StrictHostKeyChecking=no",
            "-o", "BatchMode=yes",
            "-o", f"ConnectTimeout={MAC_SSH_PROBE}",
            f"{MAC_USER}@{MAC_HOST}", cmd]
    try:
        p = subprocess.run(args, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"TIMEOUT after {timeout}s"


def mac_vm_reachable():
    """Quick reachability probe; lets the script self-skip when the VM
    isn't running (no route to host, qemu off, etc.)."""
    rc, out = mac_ssh("echo lVt75gJ4sJXjq2gWxzXd8pV8",
                      timeout=MAC_SSH_PROBE + 2)
    return rc == 0 and "lVt75gJ4sJXjq2gWxzXd8pV8" in out


def rsync_to_mac():
    name = "rsync sources to mac VM"
    log_info(f"{name}: rsync -art {ROOT}/ {MAC_USER}@{MAC_HOST}:{MAC_WORK_DIR}/")
    args = ["rsync", "-art", "--delete",
            "-e", f"ssh -o StrictHostKeyChecking=no -o BatchMode=yes "
                  f"-o ConnectTimeout={MAC_SSH_PROBE}",
            "--exclude=build/", "--exclude=.git/",
            ROOT + "/", f"{MAC_USER}@{MAC_HOST}:{MAC_WORK_DIR}/"]
    try:
        p = subprocess.run(args, timeout=MAC_RSYNC_TIMEOUT,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except subprocess.TimeoutExpired:
        log_fail(name, f"TIMEOUT after {MAC_RSYNC_TIMEOUT}s")
        return False
    if p.returncode != 0:
        log_fail(name, f"rc={p.returncode}")
        out = p.stdout.decode(errors="replace")
        if out.strip():
            print(out[-1500:])
        return False
    log_pass(name)
    return True


def build_mac_client(pro_rel, label):
    """qmake -spec macx-clang + make on the Mac VM via ssh.  Returns the
    .app bundle path on the VM, or None on failure."""
    name = f"compile {label} (mac, ssh)"
    pro_dir  = MAC_WORK_DIR + "/" + os.path.dirname(pro_rel)
    pro_path = MAC_WORK_DIR + "/" + pro_rel
    log_info(f"mac-qmake {label}")
    #The VM's Qt 6.8.0 install does not ship QtWebSockets so build with
    #NOWEBSOCKET. CONFIG+=c++17 because Qt 6.8 headers require >= C++17 and
    #the macx-clang spec defaults to a pre-c++17 standard.
    rc, out = mac_ssh(f"cd {pro_dir} && /usr/bin/make distclean > /dev/null 2>&1; "
                      f"{MAC_QMAKE} -o Makefile {pro_path} -spec macx-clang "
                      f"-config debug 'CONFIG+=c++17' 'DEFINES+=NOWEBSOCKET' 2>&1",
                      timeout=COMPILE_TIMEOUT)
    if rc != 0:
        log_fail(name, f"qmake failed (rc={rc})")
        if out.strip():
            print(out[-2000:])
        return None
    log_info(f"mac-make -j$(sysctl -n hw.ncpu) {label}")
    rc, out = mac_ssh(f"cd {pro_dir} && /usr/bin/make -j$(sysctl -n hw.ncpu) 2>&1",
                      timeout=COMPILE_TIMEOUT)
    if rc != 0:
        log_fail(name, f"make failed (rc={rc})")
        if out.strip():
            print(out[-2000:])
        return None
    rc, out = mac_ssh(f"ls -d {pro_dir}/{MAC_APP_NAME} 2>/dev/null || "
                      f"ls -d {pro_dir}/*.app 2>/dev/null | head -1",
                      timeout=15)
    app_path = out.strip().splitlines()[0] if out.strip() else ""
    if rc != 0 or not app_path:
        log_fail(name, ".app bundle not found on mac")
        return None
    log_pass(name, f"-> {app_path}")
    return app_path


def setup_mac_datapack(app_path, datapack_src, maincode):
    """rsync the datapack onto the Mac VM at <parent-of-.app>/datapack/internal/
    and prune map/main/ to one maincode."""
    target_dir = os.path.dirname(app_path) + "/datapack/internal"
    log_info(f"staging datapack on mac VM at {target_dir} (maincode={maincode})")
    mac_ssh(f"rm -rf {target_dir} && mkdir -p {target_dir}", timeout=15)
    args = ["rsync", "-art", "--delete",
            "-e", f"ssh -o StrictHostKeyChecking=no -o BatchMode=yes "
                  f"-o ConnectTimeout={MAC_SSH_PROBE}",
            "--exclude=.git/",
            datapack_src + "/", f"{MAC_USER}@{MAC_HOST}:{target_dir}/"]
    try:
        p = subprocess.run(args, timeout=MAC_RSYNC_TIMEOUT,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except subprocess.TimeoutExpired:
        log_info(f"setup_mac_datapack: TIMEOUT after {MAC_RSYNC_TIMEOUT}s")
        return False
    if p.returncode != 0:
        log_info(f"setup_mac_datapack: rsync rc={p.returncode}")
        return False
    map_main = target_dir + "/map/main"
    mac_ssh(f"if [ -d {map_main} ]; then "
            f"find {map_main} -mindepth 1 -maxdepth 1 -type d "
            f"! -name {maincode} -exec rm -rf {{}} +; fi",
            timeout=30)
    return True


def run_mac_client(app_path, label, args, timeout=MAC_RUN_TIMEOUT,
                   success_marker="MapVisualiserPlayer::mapDisplayedSlot()"):
    name = f"mac run {label}"
    bin_path = app_path + "/Contents/MacOS/catchchallenger"
    cwd = os.path.dirname(app_path)
    quoted = " ".join("'" + a.replace("'", "'\\''") + "'" for a in args)
    log_info(f"ssh run cd={cwd} {bin_path} {' '.join(args)}")
    cmd = (f"cd {cwd} && export QT_QPA_PLATFORM=offscreen "
           f"DYLD_FRAMEWORK_PATH={MAC_QT}/lib && "
           f"{bin_path} {quoted} 2>&1")
    rc, out = mac_ssh(cmd, timeout=timeout)
    if success_marker in out:
        log_pass(name, f"reached map ({success_marker})")
        return True
    if rc == -1:
        log_fail(name, f"timeout {timeout}s without success_marker")
    else:
        log_fail(name, f"exit code {rc} without success_marker")
    lines = out.splitlines()
    li = max(0, len(lines) - 30)
    while li < len(lines):
        print(f"  | {lines[li]}")
        li += 1
    return False


def set_http_datapack_mirror(build_dir, value):
    xml_path = os.path.join(build_dir, "server-properties.xml")
    if not os.path.exists(xml_path):
        return
    if os.path.islink(xml_path):
        target = os.path.realpath(xml_path)
        os.remove(xml_path)
        import shutil as _sh
        _sh.copy2(target, xml_path)
    with open(xml_path, "r") as f:
        content = f.read()
    content = re.sub(r'httpDatapackMirror\s+value="[^"]*"',
                     f'httpDatapackMirror value="{value}"', content)
    with open(xml_path, "w") as f:
        f.write(content)


def start_local_server(build_dir, bin_name=SERVER_BIN_NAME):
    global server_proc
    binary = os.path.join(build_dir, bin_name)
    if not os.path.isfile(binary):
        log_fail("server start", f"{bin_name} not found in {build_dir}")
        return None
    log_info(f"starting local server: {binary}")
    server_proc = subprocess.Popen(
        ["nice", "-n", "19", "ionice", "-c", "3", binary], cwd=build_dir,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        preexec_fn=os.setsid)
    ready = threading.Event()
    def reader():
        for raw in iter(server_proc.stdout.readline, b""):
            line = raw.decode(errors="replace").rstrip("\n")
            if "correctly bind:" in line:
                ready.set()
    t = threading.Thread(target=reader, daemon=True)
    t.start()
    if ready.wait(timeout=SERVER_READY_TIMEOUT):
        return server_proc
    log_fail("server start", "timeout waiting for 'correctly bind:'")
    stop_local_server()
    return None


def stop_local_server():
    global server_proc
    if server_proc is None:
        return
    try: os.killpg(os.getpgid(server_proc.pid), signal.SIGTERM)
    except ProcessLookupError: pass
    try: server_proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        try: os.killpg(os.getpgid(server_proc.pid), signal.SIGKILL)
        except ProcessLookupError: pass
        server_proc.wait(timeout=5)
    server_proc = None


def cleanup(*_args):
    stop_local_server()
    sys.exit(1)


signal.signal(signal.SIGINT, cleanup)
signal.signal(signal.SIGTERM, cleanup)


# ── main ────────────────────────────────────────────────────────────────────
def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — macOS cross-compile testing")
    print(f"{'='*60}{C_RESET}\n")

    failed_cases = load_failed_cases()
    if failed_cases is not None and len(failed_cases) == 0:
        log_info("all previously passed, skipping (delete failed.json for full re-run)")
        return

    if not mac_vm_reachable():
        log_info(f"mac VM {MAC_USER}@{MAC_HOST} unreachable — skipping mac test "
                 f"(start qemu, then re-run)")
        save_failed_cases()
        summary()
        return

    cpu_app = None
    gl_app  = None
    rsync_ok = True
    if should_run("rsync sources to mac VM", failed_cases):
        rsync_ok = rsync_to_mac()

    if rsync_ok:
        if should_run("compile qtcpu800x600 (mac, ssh)", failed_cases):
            cpu_app = build_mac_client(os.path.relpath(CLIENT_CPU_PRO, ROOT),
                                       "qtcpu800x600")
        if should_run("compile qtopengl (mac, ssh)", failed_cases):
            gl_app = build_mac_client(os.path.relpath(CLIENT_GL_PRO, ROOT),
                                      "qtopengl")

    mac_dp_src = DATAPACKS[0] if DATAPACKS else None
    mac_mc = None
    if mac_dp_src is not None:
        mcs = detect_maincodes(mac_dp_src)
        if mcs:
            mac_mc = mcs[0]

    if cpu_app is not None and should_run("mac run qtcpu800x600 --autosolo", failed_cases):
        if mac_dp_src and mac_mc:
            setup_mac_datapack(cpu_app, mac_dp_src, mac_mc)
        run_mac_client(cpu_app, "qtcpu800x600 --autosolo",
                       ["--autosolo", "--closewhenonmap"])
    if gl_app is not None and should_run("mac run qtopengl --autosolo", failed_cases):
        if mac_dp_src and mac_mc:
            setup_mac_datapack(gl_app, mac_dp_src, mac_mc)
        run_mac_client(gl_app, "qtopengl --autosolo",
                       ["--autosolo", "--closewhenonmap"])

    mac_need_multi = ((cpu_app is not None and should_run("mac run qtcpu800x600", failed_cases)) or
                      (gl_app  is not None and should_run("mac run qtopengl", failed_cases)))
    if mac_need_multi:
        srv = None
        if os.path.isfile(os.path.join(SERVER_BUILD, SERVER_BIN_NAME)):
            set_http_datapack_mirror(SERVER_BUILD, "")
            srv = start_local_server(SERVER_BUILD)
        else:
            log_fail("mac run: server start",
                     f"{SERVER_BIN_NAME} not found in {SERVER_BUILD}")
        if srv is not None:
            if cpu_app is not None and should_run("mac run qtcpu800x600", failed_cases):
                run_mac_client(cpu_app, "qtcpu800x600",
                               ["--host", SERVER_HOST_REMOTE, "--port", SERVER_PORT,
                                "--autologin", "--character", "MacCPU",
                                "--closewhenonmap"])
            if gl_app is not None and should_run("mac run qtopengl", failed_cases):
                run_mac_client(gl_app, "qtopengl",
                               ["--host", SERVER_HOST_REMOTE, "--port", SERVER_PORT,
                                "--autologin", "--character", "MacGL",
                                "--closewhenonmap"])
            stop_local_server()

    save_failed_cases()
    summary()


def summary():
    print(f"\n{C_CYAN}{'='*60}")
    print("  Summary")
    print(f"{'='*60}{C_RESET}")
    passed = sum(1 for r in results if r[1])
    failed = sum(1 for r in results if not r[1])
    total_elapsed = sum(r[3] for r in results)
    for name, ok, detail, elapsed in results:
        tag = f"{C_GREEN}PASS{C_RESET}" if ok else f"{C_RED}FAIL{C_RESET}"
        print(f"  [{tag}] {name}  {detail}  ({elapsed:.1f}s)")
    print(f"  total elapsed: {total_elapsed:.1f}s")
    print()
    print(f"  {C_GREEN}{passed} passed{C_RESET}, {C_RED}{failed} failed{C_RESET}")
    if failed:
        sys.exit(1)


if __name__ == "__main__":
    main()
