#!/usr/bin/env python3
"""
testinghttp.py — CatchChallenger HTTP mirror testing.

Steps:
  1. Check automatic_account_creation is true in server-properties.xml
  2. Update symlink  datapack -> server build's datapack/
  3. Run datapack-archive.sh   (cwd = WWW_ROOT)
  4. Run mirror-json-generator.php  (cwd = official-server/)
  5. Build & start server-filedb, wait for "correctly bind:"
  6. Build & run qtcpu800x600 client connecting via --host/--port
"""

import os, sys, signal, subprocess, threading, multiprocessing, json, shutil, re

# ── config ─────────────────────────────────────────────────────────────────
_CONFIG_PATH = os.path.join(os.path.expanduser("~"), ".config", "catchchallenger-testing", "config.json")
with open(_CONFIG_PATH, "r") as _f:
    _config = json.load(_f)

# ── paths ───────────────────────────────────────────────────────────────────
ROOT       = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
QMAKE      = _config["qmake"]
NPROC      = str(multiprocessing.cpu_count())
HTTP_DATAPACK_MIRROR = _config["httpDatapackMirror"]

SERVER_FILEDB_PRO  = os.path.join(ROOT, "server/epoll/catchchallenger-server-filedb.pro")
SERVER_BUILD       = os.path.join(ROOT, "server/epoll/build/catchchallenger-server-filedb-llvm-Debug")
SERVER_BIN_NAME    = "catchchallenger-server-cli-epoll"

CLIENT_CPU_PRO     = os.path.join(ROOT, "client/qtcpu800x600/qtcpu800x600.pro")
CLIENT_CPU_BUILD   = os.path.join(ROOT, "client/qtcpu800x600/build/testing-cpu")
CLIENT_CPU_BIN     = "catchchallenger"

CLIENT_DATAPACK_CACHE = os.path.expanduser(_config["paths"]["client_datapack_cache"])
CLIENT_CACHE_DIR      = os.path.join(CLIENT_DATAPACK_CACHE, f"argument-{_config['server_host']}-{_config['server_port']}")
CLIENT_CACHE_BIN      = CLIENT_CACHE_DIR + "-cache"
CLIENT_SETTINGS       = os.path.expanduser("~/.config/CatchChallenger/client-qtcpu800x600.conf")

WWW_ROOT           = _config["paths"]["www_root"]
DATAPACK_SYMLINK   = os.path.join(WWW_ROOT, "datapack")
DATAPACK_TARGET    = os.path.join(SERVER_BUILD, "datapack")
OFFICIAL_SERVER    = os.path.join(WWW_ROOT, "official-server")
SERVER_PROPS       = os.path.join(SERVER_BUILD, "server-properties.xml")

COMPILE_TIMEOUT      = 600
SERVER_READY_TIMEOUT = 60
CLIENT_TIMEOUT       = 60

NICE_PREFIX = ["nice", "-n", "19", "ionice", "-c", "3"]

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


# ── helpers ─────────────────────────────────────────────────────────────────
def ensure_dir(path):
    os.makedirs(path, exist_ok=True)

def clean_build_artifacts(build_dir):
    if not os.path.isdir(build_dir):
        return
    import glob as _glob
    for pattern in ("*.o", "moc_*.cpp", "qrc_*.cpp", "ui_*.h", "Makefile"):
        for f in _glob.glob(os.path.join(build_dir, pattern)):
            os.remove(f)

def set_map_visibility_minimize(build_dir, value):
    xml_path = os.path.join(build_dir, "server-properties.xml")
    if not os.path.exists(xml_path):
        return
    if os.path.islink(xml_path):
        target = os.path.realpath(xml_path)
        os.remove(xml_path)
        shutil.copy2(target, xml_path)
    with open(xml_path, "r") as f:
        content = f.read()
    content = re.sub(r'<minimize\s+value="[^"]*"',
                     f'<minimize value="{value}"', content)
    with open(xml_path, "w") as f:
        f.write(content)
    log_info(f'server-properties.xml minimize="{value}"')

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

def get_allowed_extensions():
    hpp = os.path.join(ROOT, "general", "base", "GeneralVariable.hpp")
    with open(hpp, "r") as f:
        content = f.read()
    m = re.search(r'CATCHCHALLENGER_EXTENSION_ALLOWED\s+"([^"]+)"', content)
    if not m:
        return set()
    return set(m.group(1).split(";"))

DATAPACK_FILE_RE = re.compile(r'^[0-9a-z_./-]*[0-9a-z]\.[a-z]{2,4}$')

def datapack_checksum(directory, allowed_ext):
    """Compute a checksum of all allowed files in directory, excluding map/main/."""
    import hashlib
    h = hashlib.sha256()
    files = []
    for dirpath, _dirs, filenames in os.walk(directory):
        rel_dir = os.path.relpath(dirpath, directory)
        if rel_dir.startswith("map" + os.sep + "main") or rel_dir.startswith("map/main"):
            continue
        for fn in filenames:
            ext = fn.rsplit(".", 1)[-1] if "." in fn else ""
            rel_path = os.path.relpath(os.path.join(dirpath, fn), directory)
            if ext in allowed_ext and DATAPACK_FILE_RE.match(rel_path):
                files.append(os.path.join(dirpath, fn))
    files.sort()
    idx = 0
    while idx < len(files):
        with open(files[idx], "rb") as f:
            h.update(f.read())
        idx += 1
    return h.hexdigest()

def dir_checksum(directory, allowed_ext):
    """Compute checksum of all allowed files in directory recursively."""
    import hashlib
    h = hashlib.sha256()
    files = []
    for dirpath, _dirs, filenames in os.walk(directory):
        for fn in filenames:
            ext = fn.rsplit(".", 1)[-1] if "." in fn else ""
            if ext in allowed_ext:
                files.append(os.path.join(dirpath, fn))
    files.sort()
    idx = 0
    while idx < len(files):
        with open(files[idx], "rb") as f:
            h.update(f.read())
        idx += 1
    return h.hexdigest()

def check_datapack_match(label, server_datapack, client_cache_dir):
    allowed = get_allowed_extensions()
    if not os.path.isdir(client_cache_dir):
        log_fail(f"{label}: datapack match", "client cache dir not found")
        return
    server_hash = datapack_checksum(server_datapack, allowed)
    client_hash = datapack_checksum(client_cache_dir, allowed)
    if server_hash == client_hash:
        log_pass(f"{label}: datapack base match", f"checksums equal ({server_hash[:16]}...)")
    else:
        log_fail(f"{label}: datapack base match",
                 f"server={server_hash[:16]}... client={client_hash[:16]}...")

def dir_checksum_exclude_sub(directory, allowed_ext):
    """Compute checksum excluding sub/ subdirectories."""
    import hashlib
    h = hashlib.sha256()
    files = []
    for dirpath, _dirs, filenames in os.walk(directory):
        rel_dir = os.path.relpath(dirpath, directory)
        if rel_dir.startswith("sub" + os.sep) or rel_dir.startswith("sub/") or rel_dir == "sub":
            continue
        for fn in filenames:
            ext = fn.rsplit(".", 1)[-1] if "." in fn else ""
            rel_path = os.path.relpath(os.path.join(dirpath, fn), directory)
            if ext in allowed_ext and DATAPACK_FILE_RE.match(rel_path):
                files.append(os.path.join(dirpath, fn))
    files.sort()
    idx = 0
    while idx < len(files):
        with open(files[idx], "rb") as f:
            h.update(f.read())
        idx += 1
    return h.hexdigest()

def check_datapack_main_match(label, server_datapack, client_cache_dir, maincode):
    allowed = get_allowed_extensions()
    server_main = os.path.join(server_datapack, "map", "main", maincode)
    client_main = os.path.join(client_cache_dir, "map", "main", maincode)
    if not os.path.isdir(server_main):
        log_fail(f"{label}: datapack main match", f"server map/main/{maincode} not found")
        return
    if not os.path.isdir(client_main):
        log_fail(f"{label}: datapack main match", f"client map/main/{maincode} not found")
        return
    server_hash = dir_checksum_exclude_sub(server_main, allowed)
    client_hash = dir_checksum_exclude_sub(client_main, allowed)
    if server_hash == client_hash:
        log_pass(f"{label}: datapack main match", f"map/main/{maincode} checksums equal ({server_hash[:16]}...)")
    else:
        log_fail(f"{label}: datapack main match",
                 f"map/main/{maincode} server={server_hash[:16]}... client={client_hash[:16]}...")

def path_to_settings_key(path):
    return path.lstrip('/').rstrip('/').replace('/', '\\')

def set_client_hashes_random():
    if not os.path.isfile(CLIENT_SETTINGS):
        return
    key_prefix = path_to_settings_key(CLIENT_CACHE_DIR)
    random_hash = os.urandom(32).hex()
    with open(CLIENT_SETTINGS, 'r') as f:
        lines = f.readlines()
    new_lines = []
    for line in lines:
        if key_prefix in line and '=' in line:
            key = line.split('=', 1)[0]
            new_lines.append(f"{key}={random_hash}\n")
        else:
            new_lines.append(line)
    with open(CLIENT_SETTINGS, 'w') as f:
        f.writelines(new_lines)
    log_info("client settings: all hashes set to random")

def setup_client_cache_partial(server_datapack):
    if os.path.exists(CLIENT_CACHE_DIR):
        shutil.rmtree(CLIENT_CACHE_DIR)
    shutil.copytree(server_datapack, CLIENT_CACHE_DIR,
                    ignore=shutil.ignore_patterns(".git"))
    all_files = []
    for dirpath, _dirs, files in os.walk(CLIENT_CACHE_DIR):
        for f in files:
            all_files.append(os.path.join(dirpath, f))
    kept = all_files[0] if all_files else None
    idx = 1
    while idx < len(all_files):
        os.remove(all_files[idx])
        idx += 1
    for dirpath, _dirs, _files in os.walk(CLIENT_CACHE_DIR, topdown=False):
        if not os.listdir(dirpath) and dirpath != CLIENT_CACHE_DIR:
            os.rmdir(dirpath)
    with open(os.path.join(CLIENT_CACHE_DIR, "toremove.xml"), 'w') as f:
        f.write('<?xml version="1.0" encoding="UTF-8"?>\n<test>toremove</test>\n')
    if kept:
        log_info(f"client cache: kept {os.path.relpath(kept, CLIENT_CACHE_DIR)} + toremove.xml")

def setup_client_cache_empty():
    if os.path.exists(CLIENT_CACHE_DIR):
        shutil.rmtree(CLIENT_CACHE_DIR)
    os.makedirs(CLIENT_CACHE_DIR, exist_ok=True)
    log_info("client cache: emptied")

def clear_client_bin_cache():
    if os.path.isdir(CLIENT_CACHE_BIN):
        shutil.rmtree(CLIENT_CACHE_BIN)
        log_info(f"cleared client binary cache: {CLIENT_CACHE_BIN}")

def run_cmd_raw(args, cwd, timeout=COMPILE_TIMEOUT):
    """Run command and return (returncode, raw bytes)."""
    try:
        p = subprocess.run(NICE_PREFIX + list(args), cwd=cwd, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return p.returncode, p.stdout
    except subprocess.TimeoutExpired:
        return -1, b""

def generate_datapack_lists(www_root):
    """Generate base.txt and main-*/sub-* using datapack-list.php."""
    datapack_list_php = os.path.join(www_root, "datapack-list.php")
    if not os.path.isfile(datapack_list_php):
        log_fail("generate datapack lists", f"not found: {datapack_list_php}")
        return
    datapack_list_dir = os.path.join(www_root, "datapack-list")
    ensure_dir(datapack_list_dir)
    # base
    rc, raw = run_cmd_raw(["php", datapack_list_php], www_root, timeout=120)
    if rc != 0:
        log_fail("generate base.txt", f"rc={rc}")
        return
    with open(os.path.join(datapack_list_dir, "base.txt"), "wb") as f:
        f.write(raw)
    log_info("generated base.txt")
    # main-* and sub-*
    map_main = os.path.join(www_root, "datapack", "map", "main")
    if os.path.isdir(map_main):
        for mc in sorted(os.listdir(map_main)):
            if not os.path.isdir(os.path.join(map_main, mc)):
                continue
            rc, raw = run_cmd_raw(["php", datapack_list_php, f"main={mc}"], www_root, timeout=120)
            if rc == 0:
                with open(os.path.join(datapack_list_dir, f"main-{mc}.txt"), "wb") as f:
                    f.write(raw)
                log_info(f"generated main-{mc}.txt")
            sub_dir = os.path.join(map_main, mc, "sub")
            if os.path.isdir(sub_dir):
                for sc in sorted(os.listdir(sub_dir)):
                    if not os.path.isdir(os.path.join(sub_dir, sc)):
                        continue
                    rc, raw = run_cmd_raw(["php", datapack_list_php, f"main={mc}", f"sub={sc}"], www_root, timeout=120)
                    if rc == 0:
                        with open(os.path.join(datapack_list_dir, f"sub-{mc}-{sc}.txt"), "wb") as f:
                            f.write(raw)
                        log_info(f"generated sub-{mc}-{sc}.txt")

def run_cmd(args, cwd, timeout=COMPILE_TIMEOUT, env=None):
    try:
        p = subprocess.run(NICE_PREFIX + list(args), cwd=cwd, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           env=env or os.environ)
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"TIMEOUT after {timeout}s"

def build_project(pro_file, build_dir, label):
    ensure_dir(build_dir)
    clean_build_artifacts(build_dir)
    log_info(f"make distclean {label}")
    run_cmd(["make", "distclean"], build_dir, timeout=60)
    name = f"compile {label}"
    qmake_args = [QMAKE, "-o", "Makefile", pro_file,
                  "-spec", "linux-g++", "CONFIG+=debug", "CONFIG+=qml_debug",
                  "QMAKE_CXXFLAGS+=-g", "QMAKE_CFLAGS+=-g", "QMAKE_LFLAGS+=-g",
                  "QMAKE_LFLAGS+=-fuse-ld=mold", "LIBS+=-fuse-ld=mold"]
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
    try: os.killpg(os.getpgid(server_proc.pid), signal.SIGTERM)
    except ProcessLookupError: pass
    try: server_proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        try: os.killpg(os.getpgid(server_proc.pid), signal.SIGKILL)
        except ProcessLookupError: pass
        server_proc.wait(timeout=5)
    server_proc = None

def run_client(build_dir, bin_name, args, label, timeout=CLIENT_TIMEOUT,
               success_marker=None):
    binary = os.path.join(build_dir, bin_name)
    if not os.path.isfile(binary):
        log_fail(label, f"binary not found")
        return False, ""
    log_info(f"running: {bin_name} {' '.join(args)}")
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"
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
            return True, out
        else:
            log_fail(label, f"exit code {p.returncode}")
            for line in out.splitlines()[-40:]:
                print(f"  | {line}")
            return False, out
    except subprocess.TimeoutExpired as e:
        out = e.stdout.decode(errors="replace") if e.stdout else ""
        if success_marker and success_marker in out:
            log_pass(label, f"timeout but connected (found '{success_marker}')")
            return True, out
        log_fail(label, f"timeout after {timeout}s")
        for line in out.splitlines()[-20:]:
            print(f"  | {line}")
        return False, out


# ── cleanup ─────────────────────────────────────────────────────────────────
def cleanup(*_args):
    stop_server()
    sys.exit(1)

signal.signal(signal.SIGINT, cleanup)
signal.signal(signal.SIGTERM, cleanup)


# ── main ────────────────────────────────────────────────────────────────────
def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — HTTP Mirror Testing")
    print(f"{'='*60}{C_RESET}\n")

    # clean all testing build dirs upfront
    for d in (SERVER_BUILD, CLIENT_CPU_BUILD):
        clean_build_artifacts(d)

    SETUP_CHECKS = ["automatic_account_creation", "http_symlink",
                     "datapack_archive", "mirror_json_generator"]
    COMPILES = ["server", "client"]
    CACHE_CASES = ["empty", "partial", "same"]
    checks_per_case = ["server_start", "client",
                       "mirror_base", "mirror_server",
                       "datapack_base_match", "datapack_main_match"]
    extra_same_case = ["initial_populate"]
    total_expected[0] = (len(SETUP_CHECKS) + len(COMPILES)
                         + len(CACHE_CASES) * len(checks_per_case)
                         + len(extra_same_case))

    # ── 1. check automatic_account_creation ─────────────────────────
    if os.path.isfile(SERVER_PROPS):
        with open(SERVER_PROPS, "r") as f:
            content = f.read()
        if 'automatic_account_creation' in content and 'value="true"' in content:
            log_pass("automatic_account_creation", "is true")
        else:
            log_fail("automatic_account_creation",
                     "not set to true in server-properties.xml")
    else:
        log_fail("automatic_account_creation",
                 f"server-properties.xml not found: {SERVER_PROPS}")

    # ── 2. update symlink ───────────────────────────────────────────
    log_info(f"symlink: {DATAPACK_SYMLINK} -> {DATAPACK_TARGET}")
    if os.path.islink(DATAPACK_SYMLINK):
        os.remove(DATAPACK_SYMLINK)
    elif os.path.exists(DATAPACK_SYMLINK):
        log_fail("http symlink",
                 f"{DATAPACK_SYMLINK} exists but is not a symlink")
        summary()
        return

    if os.path.isdir(DATAPACK_TARGET):
        os.symlink(DATAPACK_TARGET, DATAPACK_SYMLINK)
        log_pass("http symlink", f"-> {DATAPACK_TARGET}")
    else:
        log_fail("http symlink", f"datapack dir not found: {DATAPACK_TARGET}")
        summary()
        return

    # ── 3. datapack-archive.sh (cwd = WWW_ROOT) ────────────────────
    archive_sh = os.path.join(WWW_ROOT, "datapack-archive.sh")
    if os.path.isfile(archive_sh):
        log_info(f"running datapack-archive.sh  (cwd={WWW_ROOT})")
        rc, out = run_cmd(["bash", archive_sh], WWW_ROOT, timeout=300)
        if rc == 0:
            log_pass("datapack-archive.sh")
            # restore read permissions after datapack-archive.sh sets 600/700
            run_cmd(["chmod", "-R", "a+rX", DATAPACK_SYMLINK], WWW_ROOT, timeout=30)
            log_info("restored read permissions on datapack")
            # copy generated tar files from /tmp/ to pack/
            pack_dir = os.path.join(WWW_ROOT, "pack")
            ensure_dir(pack_dir)
            import glob as _glob
            copied = 0
            for tar in _glob.glob("/tmp/datapack*.tar.zst"):
                dst = os.path.join(pack_dir, os.path.basename(tar))
                shutil.copy2(tar, dst)
                copied += 1
            log_info(f"copied {copied} tar.zst file(s) to pack/")
            # create mirror/ directory with symlinks to datapack files + pack + datapack-list
            mirror_dir = os.path.join(WWW_ROOT, "mirror")
            if os.path.exists(mirror_dir):
                shutil.rmtree(mirror_dir)
            os.makedirs(mirror_dir)
            for entry in os.listdir(DATAPACK_TARGET):
                os.symlink(os.path.join(DATAPACK_TARGET, entry),
                           os.path.join(mirror_dir, entry))
            for name in ("pack", "datapack-list"):
                src = os.path.join(WWW_ROOT, name)
                if os.path.isdir(src):
                    os.symlink(src, os.path.join(mirror_dir, name))
            log_info(f"created mirror/ directory for HTTP file serving")
            # regenerate datapack-list/ from current datapack
            generate_datapack_lists(WWW_ROOT)
        else:
            log_fail("datapack-archive.sh", f"rc={rc}")
            if out.strip():
                for line in out.splitlines()[-20:]:
                    print(f"  | {line}")
    else:
        log_fail("datapack-archive.sh", f"not found: {archive_sh}")

    # ── 4. mirror-json-generator.php (cwd = official-server/) ──────
    mirror_php = os.path.join(OFFICIAL_SERVER, "mirror-json-generator.php")
    if os.path.isfile(mirror_php):
        log_info(f"running mirror-json-generator.php  (cwd={OFFICIAL_SERVER})")
        rc, out = run_cmd(["php", mirror_php, "all", "--no-mirror"],
                          OFFICIAL_SERVER, timeout=60)
        if rc == 0:
            log_pass("mirror-json-generator.php")
        else:
            log_fail("mirror-json-generator.php", f"rc={rc}")
            if out.strip():
                for line in out.splitlines()[-20:]:
                    print(f"  | {line}")
    else:
        log_fail("mirror-json-generator.php", f"not found: {mirror_php}")

    # ── 5. build server & client ──────────────────────────────────
    if not build_project(SERVER_FILEDB_PRO, SERVER_BUILD, "server-filedb"):
        stop_server()
        summary()
        return
    if not build_project(CLIENT_CPU_PRO, CLIENT_CPU_BUILD, "qtcpu800x600"):
        summary()
        return

    client_args = ["--host", _config["server_host"],
                   "--port", str(_config["server_port"]),
                   "--autologin", "--character", "Player",
                   "--closewhenonmap"]
    server_datapack = os.path.join(SERVER_BUILD, "datapack")
    mirror_base_empty = "CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase is now: \n"
    mirror_server_empty = "CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer is now: \n"

    def check_http_markers(label, client_out):
        if mirror_base_empty in client_out:
            log_fail(f"{label}: mirror base",
                     "httpDatapackMirrorBase is empty, expected URL")
            for line in client_out.splitlines()[-30:]:
                print(f"  | {line}")
        else:
            log_pass(f"{label}: mirror base", "httpDatapackMirrorBase is not empty")
        if mirror_server_empty in client_out:
            log_fail(f"{label}: mirror server",
                     "httpDatapackMirrorServer is empty, expected URL")
            for line in client_out.splitlines()[-30:]:
                print(f"  | {line}")
        else:
            log_pass(f"{label}: mirror server", "httpDatapackMirrorServer is not empty")

    # detect datapack name and maincode from server build
    dp_name = os.path.basename(os.path.realpath(server_datapack))
    with open(os.path.join(SERVER_BUILD, "server-properties.xml"), "r") as f:
        sp_content = f.read()
    mc_match = re.search(r'mainDatapackCode\s+value="([^"]*)"', sp_content)
    mc_name = mc_match.group(1) if mc_match else "unknown"
    log_info(f"server datapack: {dp_name}, maincode: {mc_name}")

    # ── 6. case 3: empty client cache (full download) ──────────────
    print(f"\n{C_CYAN}--- Case 3: empty client cache ({dp_name} {mc_name}) ---{C_RESET}\n")
    cache_file = os.path.join(SERVER_BUILD, "datapack-cache.bin")
    if os.path.exists(cache_file):
        os.remove(cache_file)
        log_info("removed datapack-cache.bin")
    db_dir = os.path.join(SERVER_BUILD, "database")
    if os.path.isdir(db_dir):
        shutil.rmtree(db_dir)
        log_info("cleared database/")
    set_http_datapack_mirror(SERVER_BUILD, HTTP_DATAPACK_MIRROR)
    set_map_visibility_minimize(SERVER_BUILD, "network")
    srv = start_server(SERVER_BUILD)
    if srv is None:
        summary()
        return

    setup_client_cache_empty()
    clear_client_bin_cache()
    set_client_hashes_random()
    ok, client_out = run_client(CLIENT_CPU_BUILD, CLIENT_CPU_BIN, client_args,
                                "case3: empty cache",
                                success_marker="MapVisualiserPlayer::mapDisplayedSlot()")
    check_http_markers("case3", client_out)
    check_datapack_match("case3", server_datapack, CLIENT_CACHE_DIR)
    check_datapack_main_match("case3", server_datapack, CLIENT_CACHE_DIR, mc_name)
    stop_server()

    # ── 7. case 2: partial client cache (diff download) ────────────
    print(f"\n{C_CYAN}--- Case 2: partial client cache ({dp_name} {mc_name}) ---{C_RESET}\n")
    if os.path.exists(cache_file):
        os.remove(cache_file)
        log_info("removed datapack-cache.bin")
    db_dir = os.path.join(SERVER_BUILD, "database")
    if os.path.isdir(db_dir):
        shutil.rmtree(db_dir)
        log_info("cleared database/")
    set_http_datapack_mirror(SERVER_BUILD, HTTP_DATAPACK_MIRROR)
    srv = start_server(SERVER_BUILD)
    if srv is None:
        summary()
        return

    setup_client_cache_partial(server_datapack)
    clear_client_bin_cache()
    set_client_hashes_random()
    ok, client_out = run_client(CLIENT_CPU_BUILD, CLIENT_CPU_BIN, client_args,
                                "case2: partial cache",
                                success_marker="MapVisualiserPlayer::mapDisplayedSlot()")
    check_http_markers("case2", client_out)
    check_datapack_match("case2", server_datapack, CLIENT_CACHE_DIR)
    check_datapack_main_match("case2", server_datapack, CLIENT_CACHE_DIR, mc_name)
    stop_server()

    # ── 8. case 1: same datapack (hash match, no download) ─────────
    #  run initial connect to populate cache + correct hash, then test
    print(f"\n{C_CYAN}--- Case 1: same datapack ({dp_name} {mc_name}) ---{C_RESET}\n")
    db_dir = os.path.join(SERVER_BUILD, "database")
    if os.path.isdir(db_dir):
        shutil.rmtree(db_dir)
        log_info("cleared database/")
    set_http_datapack_mirror(SERVER_BUILD, HTTP_DATAPACK_MIRROR)
    srv = start_server(SERVER_BUILD)
    if srv is None:
        summary()
        return

    # the cache + hash are already correct from case 3 or case 2
    # but they set random hashes, so re-populate with a fresh connect first
    setup_client_cache_empty()
    clear_client_bin_cache()
    ok, _ = run_client(CLIENT_CPU_BUILD, CLIENT_CPU_BIN, client_args,
                       "case1: initial populate",
                       success_marker="MapVisualiserPlayer::mapDisplayedSlot()")
    # now cache + hash match the server; connect again
    ok, client_out = run_client(CLIENT_CPU_BUILD, CLIENT_CPU_BIN, client_args,
                                "case1: same datapack",
                                success_marker="MapVisualiserPlayer::mapDisplayedSlot()")
    check_http_markers("case1", client_out)
    check_datapack_match("case1", server_datapack, CLIENT_CACHE_DIR)
    check_datapack_main_match("case1", server_datapack, CLIENT_CACHE_DIR, mc_name)
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
