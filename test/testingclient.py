#!/usr/bin/env python3
"""
testingclient.py — CatchChallenger client compilation, solo, and multiplayer tests.

Sections:
  1. Datapack: copy CatchChallenger-datapack to <binary>/datapack/internal/
  2. Compilation: build qtcpu800x600 and qtopengl
  3. Solo: remove savegames, play, close, come back
  4. Multiplayer: start server-filedb, test both clients connect
"""

import os, sys, signal, subprocess, threading, shutil, multiprocessing, json, re

# ── config ─────────────────────────────────────────────────────────────────
_CONFIG_PATH = os.path.join(os.path.expanduser("~"), ".config", "catchchallenger-testing", "config.json")
with open(_CONFIG_PATH, "r") as _f:
    _config = json.load(_f)

# ── paths ───────────────────────────────────────────────────────────────────
ROOT       = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
QMAKE      = _config["qmake"]
NPROC      = str(multiprocessing.cpu_count())

DATAPACKS       = _config["paths"]["datapacks"]

SERVER_PRO      = os.path.join(ROOT, "server/epoll/catchchallenger-server-filedb.pro")
SERVER_BUILD    = os.path.join(ROOT, "server/epoll/build/testing-filedb")
SERVER_REF_BUILD= os.path.join(ROOT, "server/epoll/build/catchchallenger-server-filedb-llvm-Debug")
SERVER_BIN_NAME = "catchchallenger-server-cli-epoll"

CLIENT_CPU_PRO   = os.path.join(ROOT, "client/qtcpu800x600/qtcpu800x600.pro")
CLIENT_CPU_BUILD = os.path.join(ROOT, "client/qtcpu800x600/build/testing-cpu")
CLIENT_CPU_BIN   = "catchchallenger"

CLIENT_GL_PRO    = os.path.join(ROOT, "client/qtopengl/catchchallenger-qtopengl.pro")
CLIENT_GL_BUILD  = os.path.join(ROOT, "client/qtopengl/build/testing-gl")
CLIENT_GL_BIN    = "catchchallenger"

SAVEGAME_CPU = os.path.expanduser(_config["paths"]["savegame_cpu"])
SAVEGAME_GL  = os.path.expanduser(_config["paths"]["savegame_gl"])

SERVER_HOST  = _config["server_host"]
SERVER_PORT  = str(_config["server_port"])

COMPILE_TIMEOUT      = 600
SERVER_READY_TIMEOUT = 60
CLIENT_TIMEOUT       = 5

# ── colours ─────────────────────────────────────────────────────────────────
C_GREEN  = "\033[92m"
C_RED    = "\033[91m"
C_YELLOW = "\033[93m"
C_CYAN   = "\033[96m"
C_RESET  = "\033[0m"

results = []
server_proc = None


def log_info(msg):
    print(f"{C_CYAN}[INFO]{C_RESET} {msg}")

def log_pass(name, detail=""):
    results.append((name, True, detail))
    print(f"{C_GREEN}[PASS]{C_RESET} {name}  {detail}")

def log_fail(name, detail=""):
    results.append((name, False, detail))
    print(f"{C_RED}[FAIL]{C_RESET} {name}  {detail}")


# ── helpers ─────────────────────────────────────────────────────────────────
def ensure_dir(path):
    os.makedirs(path, exist_ok=True)


def clean_build_artifacts(build_dir):
    """Remove *.o, moc_*.cpp, qrc_*.cpp, ui_*.h and Makefile from a build directory."""
    if not os.path.isdir(build_dir):
        return
    import glob as _glob
    for pattern in ("*.o", "moc_*.cpp", "qrc_*.cpp", "ui_*.h", "Makefile"):
        for f in _glob.glob(os.path.join(build_dir, pattern)):
            os.remove(f)


def set_http_datapack_mirror(build_dir, value):
    xml_path = os.path.join(build_dir, "server-properties.xml")
    if not os.path.exists(xml_path):
        return
    if os.path.islink(xml_path):
        target = os.path.realpath(xml_path)
        os.remove(xml_path)
        shutil.copy2(target, xml_path)
    with open(xml_path, "r") as f:
        content = f.read()
    content = re.sub(r'httpDatapackMirror\s+value="[^"]*"',
                     f'httpDatapackMirror value="{value}"', content)
    with open(xml_path, "w") as f:
        f.write(content)
    log_info(f'server-properties.xml httpDatapackMirror="{value}"')


def run_cmd(args, cwd, timeout=COMPILE_TIMEOUT, env=None):
    try:
        p = subprocess.run(args, cwd=cwd, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           env=env or os.environ)
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"TIMEOUT after {timeout}s"


def build_project(pro_file, build_dir, label, clean_first=False):
    ensure_dir(build_dir)
    clean_build_artifacts(build_dir)
    if clean_first:
        run_cmd(["make", "distclean"], build_dir, timeout=60)
    name = f"compile {label}"
    qmake_args = [QMAKE, "-o", "Makefile", pro_file,
                  "-spec", "linux-g++", "CONFIG+=debug", "CONFIG+=qml_debug"]
    log_info(f"qmake {label}")
    rc, out = run_cmd(qmake_args, build_dir)
    if rc != 0:
        log_fail(name, f"qmake failed (rc={rc})")
        if out.strip(): print(out[-2000:])
        return False
    log_info(f"make -j{NPROC} {label}")
    rc, out = run_cmd(["make", f"-j{NPROC}"], build_dir)
    if rc != 0:
        log_fail(name, f"make failed (rc={rc})")
        if out.strip(): print(out[-3000:])
        return False
    log_pass(name)
    return True


def setup_datapack_client(build_dir, datapack_src, label):
    """Copy datapack to <build_dir>/datapack/internal/, keep only test in map/main/."""
    dst = os.path.join(build_dir, "datapack", "internal")
    if os.path.exists(dst):
        shutil.rmtree(dst)
    if not os.path.isdir(datapack_src):
        log_fail(f"datapack {label}", f"source not found: {datapack_src}")
        return False
    log_info(f"copying datapack to {dst}")
    shutil.copytree(datapack_src, dst, ignore=shutil.ignore_patterns(".git"))
    # keep only test in map/main/
    map_main = os.path.join(dst, "map", "main")
    if os.path.isdir(map_main):
        for entry in os.listdir(map_main):
            if entry == "test":
                continue
            full = os.path.join(map_main, entry)
            if os.path.isdir(full):
                shutil.rmtree(full)
                log_info(f"removed map/main/{entry}")
    log_pass(f"datapack {label}")
    return True


def setup_server_runtime(build_dir, ref_dir=None):
    if ref_dir is None:
        ref_dir = SERVER_REF_BUILD
    for name in ("datapack", "server-properties.xml", "database"):
        dst_path = os.path.join(build_dir, name)
        src_path = os.path.join(ref_dir, name)
        if os.path.exists(dst_path) or os.path.islink(dst_path):
            continue
        if os.path.exists(src_path):
            os.symlink(src_path, dst_path)
            log_info(f"symlinked {name} -> {src_path}")


def start_server(build_dir, bin_name=SERVER_BIN_NAME):
    global server_proc
    binary = os.path.join(build_dir, bin_name)
    if not os.path.isfile(binary):
        log_fail("start server", f"binary not found: {binary}")
        return None
    log_info(f"starting server: {binary}")
    server_proc = subprocess.Popen(
        [binary], cwd=build_dir,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        preexec_fn=os.setsid)
    ready = threading.Event()
    output_lines = []
    def reader():
        for raw in iter(server_proc.stdout.readline, b""):
            line = raw.decode(errors="replace").rstrip("\n")
            output_lines.append(line)
            if "correctly bind:" in line:
                ready.set()
    t = threading.Thread(target=reader, daemon=True)
    t.start()
    if ready.wait(timeout=SERVER_READY_TIMEOUT):
        log_pass("server start", "correctly bind: detected")
        return server_proc
    else:
        log_fail("server start", "timeout waiting for 'correctly bind:'")
        for l in output_lines[-30:]:
            print(f"  | {l}")
        stop_server()
        return None


def stop_server():
    global server_proc
    if server_proc is None:
        return
    log_info("stopping server")
    try:
        os.killpg(os.getpgid(server_proc.pid), signal.SIGTERM)
    except ProcessLookupError:
        pass
    try:
        server_proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        try: os.killpg(os.getpgid(server_proc.pid), signal.SIGKILL)
        except ProcessLookupError: pass
        server_proc.wait(timeout=5)
    server_proc = None


def run_client(build_dir, bin_name, args, label, timeout=CLIENT_TIMEOUT,
               success_marker=None, use_offscreen=True):
    binary = os.path.join(build_dir, bin_name)
    if not os.path.isfile(binary):
        log_fail(label, f"binary not found: {binary}")
        return False
    log_info(f"running: {bin_name} {' '.join(args)}")
    env = os.environ.copy()
    if use_offscreen:
        env["QT_QPA_PLATFORM"] = "offscreen"
    try:
        p = subprocess.run([binary] + args, cwd=build_dir, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env)
        out = p.stdout.decode(errors="replace")
        if p.returncode == 0:
            log_pass(label, "exit code 0")
            return True
        else:
            log_fail(label, f"exit code {p.returncode}")
            for line in out.splitlines()[-20:]:
                print(f"  | {line}")
            return False
    except subprocess.TimeoutExpired as e:
        out = e.stdout.decode(errors="replace") if e.stdout else ""
        if success_marker and success_marker in out:
            log_pass(label, f"timeout but connected (found '{success_marker}')")
            return True
        log_fail(label, f"timeout after {timeout}s")
        for line in out.splitlines()[-20:]:
            print(f"  | {line}")
        return False


def remove_savegames(path, label):
    if os.path.isdir(path):
        shutil.rmtree(path)
        log_info(f"removed savegames: {path}")
    else:
        log_info(f"no savegames to remove: {path}")


# ── cleanup ─────────────────────────────────────────────────────────────────
def cleanup(*_args):
    stop_server()
    sys.exit(1)

signal.signal(signal.SIGINT, cleanup)
signal.signal(signal.SIGTERM, cleanup)


# ── main ────────────────────────────────────────────────────────────────────
def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — Client Testing")
    print(f"{'='*60}{C_RESET}\n")

    # clean all testing build dirs upfront
    for d in (CLIENT_CPU_BUILD, CLIENT_GL_BUILD, SERVER_BUILD):
        clean_build_artifacts(d)

    # ═══════════════════════════════════════════════════════════════
    # 1. COMPILATION
    # ═══════════════════════════════════════════════════════════════
    print(f"\n{C_CYAN}--- Compilation ---{C_RESET}\n")

    cpu_ok = build_project(CLIENT_CPU_PRO, CLIENT_CPU_BUILD, "qtcpu800x600")
    gl_ok  = build_project(CLIENT_GL_PRO, CLIENT_GL_BUILD, "qtopengl")

    # ═══════════════════════════════════════════════════════════════
    # 2 + 3. DATAPACK SETUP + SOLO (iterate each datapack)
    # ═══════════════════════════════════════════════════════════════
    for dp_path in DATAPACKS:
        dp_name = os.path.basename(dp_path)
        print(f"\n{C_CYAN}--- Datapack + Solo: {dp_name} ---{C_RESET}\n")

        dp_cpu = False
        dp_gl  = False
        if cpu_ok:
            dp_cpu = setup_datapack_client(CLIENT_CPU_BUILD, dp_path, f"qtcpu800x600 ({dp_name})")
        if gl_ok:
            dp_gl = setup_datapack_client(CLIENT_GL_BUILD, dp_path, f"qtopengl ({dp_name})")

        if cpu_ok and dp_cpu:
            remove_savegames(SAVEGAME_CPU, "qtcpu800x600")
            run_client(CLIENT_CPU_BUILD, CLIENT_CPU_BIN,
                       ["--autosolo", "--closewhenonmap"],
                       f"qtcpu800x600 solo first run ({dp_name})")
            run_client(CLIENT_CPU_BUILD, CLIENT_CPU_BIN,
                       ["--autosolo", "--closewhenonmap"],
                       f"qtcpu800x600 solo come back ({dp_name})")

        if gl_ok and dp_gl:
            remove_savegames(SAVEGAME_GL, "qtopengl")
            run_client(CLIENT_GL_BUILD, CLIENT_GL_BIN,
                       ["--autosolo", "--closewhenonmap"],
                       f"qtopengl solo first run ({dp_name})",
                       use_offscreen=False,
                       success_marker="MapVisualiserPlayer::mapDisplayedSlot()")
            run_client(CLIENT_GL_BUILD, CLIENT_GL_BIN,
                       ["--autosolo", "--closewhenonmap"],
                       f"qtopengl solo come back ({dp_name})",
                       use_offscreen=False,
                       success_marker="MapVisualiserPlayer::mapDisplayedSlot()")

    # ═══════════════════════════════════════════════════════════════
    # 4. MULTIPLAYER ON SERVER
    # ═══════════════════════════════════════════════════════════════
    print(f"\n{C_CYAN}--- Multiplayer on server ---{C_RESET}\n")

    if not build_project(SERVER_PRO, SERVER_BUILD, "server-filedb", clean_first=True):
        stop_server()
        summary()
        return

    setup_server_runtime(SERVER_BUILD)
    db_dir = os.path.join(SERVER_BUILD, "database")
    if os.path.isdir(db_dir):
        shutil.rmtree(db_dir)
        log_info("cleared database/ for clean multiplayer test")
    set_http_datapack_mirror(SERVER_BUILD, "")
    srv = start_server(SERVER_BUILD)
    if srv is None:
        summary()
        return

    if cpu_ok:
        run_client(CLIENT_CPU_BUILD, CLIENT_CPU_BIN,
                   ["--host", SERVER_HOST, "--port", SERVER_PORT,
                    "--autologin", "--character", "Player",
                    "--closewhenonmap"],
                   "qtcpu800x600 connect to server",
                   success_marker="MapVisualiserPlayer::mapDisplayedSlot()")

    if gl_ok:
        run_client(CLIENT_GL_BUILD, CLIENT_GL_BIN,
                   ["--host", SERVER_HOST, "--port", SERVER_PORT,
                    "--autologin", "--character", "Player",
                    "--closewhenonmap"],
                   "qtopengl connect to server",
                   success_marker="MapVisualiserPlayer::mapDisplayedSlot()",
                   use_offscreen=False)

    stop_server()
    summary()


def summary():
    print(f"\n{C_CYAN}{'='*60}")
    print("  Summary")
    print(f"{'='*60}{C_RESET}")
    passed = sum(1 for _, ok, _ in results if ok)
    failed = sum(1 for _, ok, _ in results if not ok)
    for name, ok, detail in results:
        tag = f"{C_GREEN}PASS{C_RESET}" if ok else f"{C_RED}FAIL{C_RESET}"
        print(f"  [{tag}] {name}  {detail}")
    print()
    print(f"  {C_GREEN}{passed} passed{C_RESET}, {C_RED}{failed} failed{C_RESET}")
    if failed:
        sys.exit(1)


if __name__ == "__main__":
    main()
