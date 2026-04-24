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
from remote_build import (start_remote_builds, collect_remote_results,
                          REMOTE_SERVERS, REMOTE_DIR,
                          setup_remote_server_runtime, start_remote_server,
                          stop_remote_server, get_remote_server_address)

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
CLIENT_SOLO_TIMEOUT  = 30

NICE_PREFIX = ["nice", "-n", "19", "ionice", "-c", "3"]

# ── compilers and optional flag combinations ───────────────────────────────
COMPILERS = [
    ("gcc",   "linux-g++"),
    ("clang", "linux-clang"),
]

CXX_VERSIONS = ["c++11", "c++23"]

# CATCHCHALLENGER_NOAUDIO is commented out in qtcpu800x600.pro but the build
# requires it (Qt multimedia audio path has issues), so always add it as a
# base define.  The remaining flags are independently toggleable.
CPU_BASE_DEFINES = ["CATCHCHALLENGER_NOAUDIO"]
CPU_OPTIONAL_FLAGS = ["NOSINGLEPLAYER", "CATCHCHALLENGER_EXTRA_CHECK", "NOWEBSOCKET"]

# qtopengl default build has audio enabled (Qt multimedia works).
# Only CATCHCHALLENGER_EXTRA_CHECK and NOWEBSOCKET are safe independent
# toggles; NOSINGLEPLAYER/CATCHCHALLENGER_NOAUDIO/NOTCPSOCKET/NOTHREADS
# are WASM-only flags with interdependencies that break native builds.
GL_BASE_DEFINES = []
GL_OPTIONAL_FLAGS = ["NOSINGLEPLAYER", "CATCHCHALLENGER_NOAUDIO",
                     "CATCHCHALLENGER_EXTRA_CHECK", "NOWEBSOCKET"]

# ── colours ─────────────────────────────────────────────────────────────────
C_GREEN  = "\033[92m"
C_RED    = "\033[91m"
C_YELLOW = "\033[93m"
C_CYAN   = "\033[96m"
C_RESET  = "\033[0m"

results = []
total_expected = [0]
server_proc = None


def log_info(msg):
    print(f"{C_CYAN}[INFO]{C_RESET} {msg}")

def log_pass(name, detail=""):
    results.append((name, True, detail))
    print(f"{C_GREEN}[PASS]{C_RESET} {len(results)}/{total_expected[0]} {name}  {detail}")

def log_fail(name, detail=""):
    results.append((name, False, detail))
    print(f"{C_RED}[FAIL]{C_RESET} {len(results)}/{total_expected[0]} {name}  {detail}")


def flag_combinations(flags):
    """Generate all subsets (powerset) of the given flags."""
    result = []
    count = len(flags)
    idx = 0
    while idx < (1 << count):
        combo = []
        j = 0
        while j < count:
            if idx & (1 << j):
                combo.append(flags[j])
            j += 1
        result.append(tuple(combo))
        idx += 1
    return result


def combo_suffix(compiler_name, flags):
    """Build a unique directory/label suffix for a compiler+flags combination."""
    parts = [compiler_name]
    if flags:
        idx = 0
        while idx < len(flags):
            parts.append(flags[idx].lower())
            idx += 1
    else:
        parts.append("default")
    return "-".join(parts)


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
        p = subprocess.run(NICE_PREFIX + list(args), cwd=cwd, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           env=env or os.environ)
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"TIMEOUT after {timeout}s"


def build_project(pro_file, build_dir, label, compiler_spec="linux-g++",
                   extra_defines=None, cxx_version=None):
    ensure_dir(build_dir)
    clean_build_artifacts(build_dir)
    log_info(f"make distclean {label}")
    run_cmd(["make", "distclean"], build_dir, timeout=60)
    name = f"compile {label}"
    qmake_args = [QMAKE, "-o", "Makefile", pro_file,
                  "-spec", compiler_spec, "CONFIG+=debug", "CONFIG+=qml_debug",
                  "QMAKE_CXXFLAGS+=-g", "QMAKE_CFLAGS+=-g", "QMAKE_LFLAGS+=-g",
                  "QMAKE_LFLAGS+=-fuse-ld=mold", "LIBS+=-fuse-ld=mold"]
    if cxx_version:
        qmake_args.append(f"QMAKE_CXXFLAGS+=-std={cxx_version}")
    if extra_defines:
        qmake_args.append("DEFINES+=" + " ".join(extra_defines))
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


def detect_maincodes(datapack_src):
    """Return sorted list of maincode subdirectories in map/main/."""
    map_main = os.path.join(datapack_src, "map", "main")
    if not os.path.isdir(map_main):
        return []
    return sorted([d for d in os.listdir(map_main)
                   if os.path.isdir(os.path.join(map_main, d))])

def setup_datapack_client(build_dir, datapack_src, maincode, label):
    """Copy datapack to <build_dir>/datapack/internal/, keep only maincode in map/main/."""
    dst = os.path.join(build_dir, "datapack", "internal")
    if os.path.exists(dst):
        shutil.rmtree(dst)
    if not os.path.isdir(datapack_src):
        log_fail(f"datapack {label}", f"source not found: {datapack_src}")
        return False
    log_info(f"copying datapack to {dst}")
    shutil.copytree(datapack_src, dst, ignore=shutil.ignore_patterns(".git"))
    map_main = os.path.join(dst, "map", "main")
    if os.path.isdir(map_main):
        for entry in os.listdir(map_main):
            if entry == maincode:
                continue
            full = os.path.join(map_main, entry)
            if os.path.isdir(full):
                shutil.rmtree(full)
                log_info(f"removed map/main/{entry}")
    log_info(f"maincode: {maincode}")
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
        NICE_PREFIX + [binary], cwd=build_dir,
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
    gdb_commands = ("handle SIGPIPE nostop noprint pass\n"
                    "break __throw_out_of_range\n"
                    "run\n"
                    "bt\n"
                    "quit\n")
    gdb_args = ["gdb", "-batch",
                "-ex", "set breakpoint pending on",
                "-ex", "handle SIGPIPE nostop noprint pass",
                "-ex", "break __throw_out_of_range",
                "-ex", "run", "-ex", "bt", "-ex", "quit",
                "--args", binary] + args
    try:
        p = subprocess.run(NICE_PREFIX + gdb_args, cwd=build_dir, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env)
        out = p.stdout.decode(errors="replace")
        if p.returncode == 0:
            log_pass(label, "exit code 0")
            return True
        else:
            log_fail(label, f"exit code {p.returncode}")
            # print backtrace from gdb output
            for line in out.splitlines()[-40:]:
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

    # ═══════════════════════════════════════════════════════════════
    # 1. COMPILATION — all flag + compiler combinations
    # ═══════════════════════════════════════════════════════════════
    print(f"\n{C_CYAN}--- Compilation (all combinations) ---{C_RESET}\n")

    # start remote builds in parallel with local builds
    remote_pro_rels = [
        os.path.relpath(CLIENT_CPU_PRO, ROOT),
        os.path.relpath(CLIENT_GL_PRO, ROOT),
    ]
    remote_threads, remote_results, remote_lock = start_remote_builds(remote_pro_rels)
    log_info("remote builds started in background")

    cpu_combos = flag_combinations(CPU_OPTIONAL_FLAGS)
    gl_combos = flag_combinations(GL_OPTIONAL_FLAGS)

    total_cpu = len(CXX_VERSIONS) * len(COMPILERS) * len(cpu_combos)
    total_gl = len(CXX_VERSIONS) * len(COMPILERS) * len(gl_combos)

    # compute total expected results
    n_maincodes = 0
    idx = 0
    while idx < len(DATAPACKS):
        mc = detect_maincodes(DATAPACKS[idx])
        n_maincodes += len(mc) if mc else 0
        idx += 1
    from remote_build import REMOTE_SERVERS as _RS, GUI_PRO_FILES as _GF
    remote_count = 0
    ri = 0
    while ri < len(_RS):
        remote_count += 1  # rsync
        si = 0
        while si < len(remote_pro_rels):
            if _RS[ri][5] or remote_pro_rels[si] not in _GF:
                remote_count += 1
            si += 1
        client_builds_list = [CLIENT_CPU_PRO, CLIENT_GL_PRO]
        remote_count += 1 + len(client_builds_list)  # server_start + client per client type
        ri += 1
    client_types_list = [CLIENT_CPU_BUILD, CLIENT_GL_BUILD]
    solo_runs_list = ["first run", "come back"]
    # per maincode: datapack setups + solo runs per client
    per_maincode = len(client_types_list) + len(client_types_list) * len(solo_runs_list)
    # multiplayer: server build + server start + client connects
    multiplayer_parts = ["server_build", "server_start"] + client_types_list
    multiplayer = len(multiplayer_parts)
    # remote: already counted in remote_count (rsync + builds + server_start + client_connects)
    total_expected[0] = total_cpu + total_gl + remote_count + n_maincodes * per_maincode + multiplayer
    log_info(f"qtcpu800x600: {total_cpu} combinations "
             f"({len(CXX_VERSIONS)} c++ versions x {len(COMPILERS)} compilers x {len(cpu_combos)} flag sets)")
    log_info(f"qtopengl:     {total_gl} combinations "
             f"({len(CXX_VERSIONS)} c++ versions x {len(COMPILERS)} compilers x {len(gl_combos)} flag sets)")
    log_info(f"total expected results: {total_expected[0]}")

    cpu_ok = False
    gl_ok = False

    # qtcpu800x600 — all combinations
    vi = 0
    while vi < len(CXX_VERSIONS):
        cxx_ver = CXX_VERSIONS[vi]
        ci = 0
        while ci < len(COMPILERS):
            compiler_name, compiler_spec = COMPILERS[ci]
            fi = 0
            while fi < len(cpu_combos):
                flags = cpu_combos[fi]
                all_defines = list(CPU_BASE_DEFINES) + list(flags)
                suffix = combo_suffix(compiler_name, flags)
                if compiler_name == "gcc" and not flags and cxx_ver == CXX_VERSIONS[0]:
                    build_dir = CLIENT_CPU_BUILD
                else:
                    build_dir = os.path.join(ROOT, "client", "qtcpu800x600", "build",
                                             f"testing-cpu-{cxx_ver}-{suffix}")
                label = f"qtcpu800x600 {cxx_ver} {compiler_name}"
                if flags:
                    label += " " + "+".join(flags)
                ok = build_project(CLIENT_CPU_PRO, build_dir, label, compiler_spec,
                                   all_defines if all_defines else None,
                                   cxx_version=cxx_ver)
                if compiler_name == "gcc" and not flags and cxx_ver == CXX_VERSIONS[0]:
                    cpu_ok = ok
                fi += 1
            ci += 1
        vi += 1

    # qtopengl — all combinations
    vi = 0
    while vi < len(CXX_VERSIONS):
        cxx_ver = CXX_VERSIONS[vi]
        ci = 0
        while ci < len(COMPILERS):
            compiler_name, compiler_spec = COMPILERS[ci]
            fi = 0
            while fi < len(gl_combos):
                flags = gl_combos[fi]
                all_defines = list(GL_BASE_DEFINES) + list(flags)
                suffix = combo_suffix(compiler_name, flags)
                if compiler_name == "gcc" and not flags and cxx_ver == CXX_VERSIONS[0]:
                    build_dir = CLIENT_GL_BUILD
                else:
                    build_dir = os.path.join(ROOT, "client", "qtopengl", "build",
                                             f"testing-gl-{cxx_ver}-{suffix}")
                label = f"qtopengl {cxx_ver} {compiler_name}"
                if flags:
                    label += " " + "+".join(flags)
                ok = build_project(CLIENT_GL_PRO, build_dir, label, compiler_spec,
                                   all_defines if all_defines else None,
                                   cxx_version=cxx_ver)
                if compiler_name == "gcc" and not flags and cxx_ver == CXX_VERSIONS[0]:
                    gl_ok = ok
                fi += 1
            ci += 1
        vi += 1

    # collect remote build results
    log_info("waiting for remote builds to finish...")
    remote = collect_remote_results(remote_threads, remote_results, remote_lock)
    idx = 0
    while idx < len(remote):
        name, ok, detail = remote[idx]
        if ok:
            log_pass(name, detail)
        else:
            log_fail(name, detail)
        idx += 1

    # ═══════════════════════════════════════════════════════════════
    # 2 + 3. DATAPACK SETUP + SOLO (iterate each datapack + maincode)
    # ═══════════════════════════════════════════════════════════════
    for dp_path in DATAPACKS:
        dp_name = os.path.basename(dp_path)
        maincodes = detect_maincodes(dp_path)
        if not maincodes:
            log_fail(f"datapack {dp_name}", "no maincodes in map/main/")
            continue
        log_info(f"{dp_name}: maincodes found: {', '.join(maincodes)}")

        for mc in maincodes:
            print(f"\n{C_CYAN}--- Datapack + Solo: {dp_name} maincode={mc} ---{C_RESET}\n")

            dp_cpu = False
            dp_gl  = False
            if cpu_ok:
                dp_cpu = setup_datapack_client(CLIENT_CPU_BUILD, dp_path, mc,
                                               f"qtcpu800x600 ({dp_name} {mc})")
            if gl_ok:
                dp_gl = setup_datapack_client(CLIENT_GL_BUILD, dp_path, mc,
                                              f"qtopengl ({dp_name} {mc})")

            if cpu_ok and dp_cpu:
                remove_savegames(SAVEGAME_CPU, "qtcpu800x600")
                run_client(CLIENT_CPU_BUILD, CLIENT_CPU_BIN,
                           ["--autosolo", "--closewhenonmap"],
                           f"qtcpu800x600 solo first run ({dp_name} {mc})",
                           timeout=CLIENT_SOLO_TIMEOUT,
                           success_marker="MapVisualiserPlayer::mapDisplayedSlot()")
                run_client(CLIENT_CPU_BUILD, CLIENT_CPU_BIN,
                           ["--autosolo", "--closewhenonmap"],
                           f"qtcpu800x600 solo come back ({dp_name} {mc})",
                           timeout=CLIENT_SOLO_TIMEOUT,
                           success_marker="MapVisualiserPlayer::mapDisplayedSlot()")

            if gl_ok and dp_gl:
                remove_savegames(SAVEGAME_GL, "qtopengl")
                run_client(CLIENT_GL_BUILD, CLIENT_GL_BIN,
                           ["--autosolo", "--closewhenonmap"],
                           f"qtopengl solo first run ({dp_name} {mc})",
                           timeout=CLIENT_SOLO_TIMEOUT,
                           success_marker="MapVisualiserPlayer::mapDisplayedSlot()")
                run_client(CLIENT_GL_BUILD, CLIENT_GL_BIN,
                           ["--autosolo", "--closewhenonmap"],
                           f"qtopengl solo come back ({dp_name} {mc})",
                           timeout=CLIENT_SOLO_TIMEOUT,
                           success_marker="MapVisualiserPlayer::mapDisplayedSlot()")

    # ═══════════════════════════════════════════════════════════════
    # 4. MULTIPLAYER ON SERVER
    # ═══════════════════════════════════════════════════════════════
    print(f"\n{C_CYAN}--- Multiplayer on server ---{C_RESET}\n")

    if not build_project(SERVER_PRO, SERVER_BUILD, "server-filedb"):
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
                    "--autologin", "--character", "PlayerCPU",
                    "--closewhenonmap"],
                   "qtcpu800x600 connect to server",
                   timeout=30,
                   success_marker="MapVisualiserPlayer::mapDisplayedSlot()")

    if gl_ok:
        run_client(CLIENT_GL_BUILD, CLIENT_GL_BIN,
                   ["--host", SERVER_HOST, "--port", SERVER_PORT,
                    "--autologin", "--character", "PlayerGL",
                    "--closewhenonmap"],
                   "qtopengl connect to server",
                   timeout=30,
                   success_marker="MapVisualiserPlayer::mapDisplayedSlot()")

    stop_server()

    # ═══════════════════════════════════════════════════════════════
    # 5. REMOTE SERVER + LOCAL CLIENT
    # ═══════════════════════════════════════════════════════════════
    print(f"\n{C_CYAN}--- Remote server + local client ---{C_RESET}\n")

    remote_filedb_bin = f"{REMOTE_DIR}/build-remote/catchchallenger-server-filedb/catchchallenger-server-cli-epoll"
    remote_filedb_build = f"{REMOTE_DIR}/build-remote/catchchallenger-server-filedb"
    dp_src = DATAPACKS[0] if DATAPACKS else None

    ri = 0
    while ri < len(REMOTE_SERVERS):
        label, host, ssh_port, _use_mold, _extra_defines, _has_gui = REMOTE_SERVERS[ri]
        server_addr = get_remote_server_address(host)
        print(f"\n{C_CYAN}  -- Remote: {label} ({server_addr}) --{C_RESET}\n")

        if dp_src is None:
            log_fail(f"remote {label}", "no datapack source")
            ri += 1
            continue
        if not setup_remote_server_runtime(host, ssh_port, remote_filedb_build, dp_src):
            log_fail(f"remote {label} setup", "failed to rsync datapack")
            ri += 1
            continue

        ssh_proc, actual_port = start_remote_server(host, ssh_port,
                                                    remote_filedb_bin,
                                                    remote_filedb_build)
        if actual_port == 0:
            log_fail(f"remote {label} server start", "timeout or binary not found")
            ri += 1
            continue
        log_pass(f"remote {label} server start", f"port {actual_port}")

        if cpu_ok:
            run_client(CLIENT_CPU_BUILD, CLIENT_CPU_BIN,
                       ["--host", server_addr, "--port", str(actual_port),
                        "--autologin", "--character", "PlayerCPU",
                        "--closewhenonmap"],
                       f"qtcpu800x600 -> remote {label}",
                       timeout=30,
                       success_marker="MapVisualiserPlayer::mapDisplayedSlot()")

        if gl_ok:
            run_client(CLIENT_GL_BUILD, CLIENT_GL_BIN,
                       ["--host", server_addr, "--port", str(actual_port),
                        "--autologin", "--character", "PlayerGL",
                        "--closewhenonmap"],
                       f"qtopengl -> remote {label}",
                       timeout=30,
                       success_marker="MapVisualiserPlayer::mapDisplayedSlot()")

        stop_remote_server(ssh_proc, host, ssh_port)
        ri += 1

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
