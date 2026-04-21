#!/usr/bin/env python3
"""
testingserver.py — CatchChallenger server compilation, cache, NOXML, and datapack tests.

Phases:
  A. server-filedb + client connect
  B. server-cli-epoll (PostgreSQL) + client connect
  For each: empty DB + no cache → generate cache → with cache (NOXML off) → NOXML on
  C. Datapack: test each datapack source and each maincode
     + XML change/add/delete detection after client connect
"""

import os, sys, signal, subprocess, threading, shutil, multiprocessing, time, glob as globmod, json, re

# ── config ─────────────────────────────────────────────────────────────────
_CONFIG_PATH = os.path.join(os.path.expanduser("~"), ".config", "catchchallenger-testing", "config.json")
with open(_CONFIG_PATH, "r") as _f:
    _config = json.load(_f)

# ── paths ───────────────────────────────────────────────────────────────────
ROOT       = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
QMAKE      = _config["qmake"]
NPROC      = str(multiprocessing.cpu_count())

SERVER_FILEDB_PRO  = os.path.join(ROOT, "server/epoll/catchchallenger-server-filedb.pro")
SERVER_CLIEPO_PRO  = os.path.join(ROOT, "server/epoll/catchchallenger-server-cli-epoll.pro")
SERVER_REF_BUILD   = os.path.join(ROOT, "server/epoll/build/catchchallenger-server-filedb-llvm-Debug")
SERVER_BIN_NAME    = "catchchallenger-server-cli-epoll"

CLIENT_CPU_PRO     = os.path.join(ROOT, "client/qtcpu800x600/qtcpu800x600.pro")
CLIENT_CPU_BIN     = "catchchallenger"

PG_DB       = _config["postgresql"]["database"]
PG_USER     = _config["postgresql"]["user"]
PG_SQL_DIR  = os.path.join(ROOT, "server/databases/postgresql")

DATAPACK_SOURCES = [(dp, os.path.basename(dp)) for dp in _config["paths"]["datapacks"]]

# client's cached datapack base path
CLIENT_DATAPACK_CACHE = os.path.expanduser(
    _config["paths"]["client_datapack_cache"])
CLIENT_CACHE_DIR      = os.path.join(CLIENT_DATAPACK_CACHE, f"argument-{_config['server_host']}-{_config['server_port']}")
CLIENT_CACHE_BIN      = CLIENT_CACHE_DIR + "-cache"
CLIENT_SETTINGS       = os.path.expanduser("~/.config/CatchChallenger/client-qtcpu800x600.conf")

COMPILE_TIMEOUT      = 600
SERVER_READY_TIMEOUT = 60
CLIENT_TIMEOUT       = 5
SAVE_TIMEOUT         = 120

NICE_PREFIX = ["nice", "-n", "19", "ionice", "-c", "3"]

# ── compilers and server target flag combinations ─────────────────────────
COMPILERS = [
    ("gcc",   "linux-g++"),
    ("clang", "linux-clang"),
]

# (pro_relative_path, name, build_base_relative, [optional_flags])
# Flags are commented-out DEFINES in each .pro (excluding SERVERSSL,
# SERVERBENCHMARK, PROTOCOLPARSINGDEBUG, USING_PCH, and DB-backend
# variants that require library changes).
# Excluded flags (broken C++ code, not test-script issues):
#   CATCHCHALLENGERSERVERDROPIFCLENT — incompatible with CATCHCHALLENGER_EXTRA_CHECK
#       (hardcoded in filedb) and has ProtocolParsing constructor bugs in all epoll targets
#   EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION — CompressionProtocol references not guarded
#       in login/gateway LinkToGameServerProtocolParsing.cpp
# Excluded targets:
#   server-test — c++20 build errors in ClientHeavyLoadLogin.cpp
#   server-gui / server-cli — relative include path resolution fails from build dir
#       (../../general/base/cpp11addition.hpp not found)
SERVER_TARGETS = [
    ("server/epoll/catchchallenger-server-filedb.pro",
     "server-filedb", "server/epoll/build",
     ["CATCHCHALLENGER_NOXML", "CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR",
      "CATCHCHALLENGER_HARDENED", "CATCHCHALLENGER_LIMIT_254CONNECTED",
      "CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK",
      "HPS_VECTOR_RAW_BINARY"]),
    ("server/epoll/catchchallenger-server-cli-epoll.pro",
     "server-cli-epoll", "server/epoll/build",
     ["CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR",
      "CATCHCHALLENGER_NOXML", "CATCHCHALLENGER_HARDENED"]),
    ("server/login/login.pro",
     "server-login", "server/login/build",
     []),
    ("server/master/master.pro",
     "server-master", "server/master/build",
     []),
    ("server/gateway/gateway.pro",
     "server-gateway", "server/gateway/build",
     []),
    ("server/game-server-alone/game-server-alone.pro",
     "game-server-alone", "server/game-server-alone/build",
     ["CATCHCHALLENGER_HARDENED"]),
    ("server/epoll/filedb-converter/filedb-converter.pro",
     "filedb-converter", "server/epoll/filedb-converter/build",
     []),
]

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


def flag_short(flag):
    """Shorten a flag name for directory paths."""
    s = flag.lower()
    s = s.replace("epollcatchchallengerserver", "epoll-")
    s = s.replace("catchchallenger_", "")
    if s.startswith("catchchallenger"):
        s = s[len("catchchallenger"):]
    return s


def combo_suffix(compiler_name, flags):
    """Build a unique directory/label suffix for a compiler+flags combination."""
    parts = [compiler_name]
    if flags:
        idx = 0
        while idx < len(flags):
            parts.append(flag_short(flags[idx]))
            idx += 1
    else:
        parts.append("default")
    return "-".join(parts)


# ── helpers ─────────────────────────────────────────────────────────────────
def ensure_dir(path):
    os.makedirs(path, exist_ok=True)

def clean_build_artifacts(build_dir):
    if not os.path.isdir(build_dir):
        return
    for pattern in ("*.o", "moc_*.cpp", "qrc_*.cpp", "ui_*.h", "Makefile"):
        for f in globmod.glob(os.path.join(build_dir, pattern)):
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

def run_cmd(args, cwd, timeout=COMPILE_TIMEOUT, env=None):
    try:
        p = subprocess.run(NICE_PREFIX + list(args), cwd=cwd, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           env=env or os.environ)
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"TIMEOUT after {timeout}s"

def build_project(pro_file, build_dir, label, compiler_spec="linux-g++",
                   extra_defines=None, clean_first=False):
    ensure_dir(build_dir)
    clean_build_artifacts(build_dir)
    log_info(f"make distclean {label}")
    run_cmd(["make", "distclean"], build_dir, timeout=60)
    name = f"compile {label}"
    qmake_args = [QMAKE, "-o", "Makefile", pro_file,
                  "-spec", compiler_spec, "CONFIG+=debug", "CONFIG+=qml_debug"]
    if extra_defines:
        for d in extra_defines:
            qmake_args.append(f"DEFINES+={d}")
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

def setup_server_datapack(build_dir, datapack_src, patch_db=False):
    """Copy datapack and adapt server-properties.xml mainDatapackCode to match."""
    dst = os.path.join(build_dir, "datapack")
    if os.path.islink(dst):
        os.remove(dst)
    elif os.path.isdir(dst):
        shutil.rmtree(dst)
    shutil.copytree(datapack_src, dst, ignore=shutil.ignore_patterns(".git"))
    log_info(f"copied datapack from {datapack_src}")
    # adapt mainDatapackCode in server-properties.xml to first available maincode
    map_main = os.path.join(dst, "map", "main")
    if os.path.isdir(map_main):
        maincodes = sorted([d for d in os.listdir(map_main)
                            if os.path.isdir(os.path.join(map_main, d))])
        if maincodes:
            setup_server_config(build_dir, maincode=maincodes[0], patch_db=patch_db)

def setup_server_config(build_dir, maincode=None, patch_db=False):
    """Copy server-properties.xml from reference build, patching mainDatapackCode.
    If maincode is None and a config already exists, preserve its maincode.
    If patch_db is True, set all DB sections to use catchchallenger/postgres/localhost."""
    import re
    dst = os.path.join(build_dir, "server-properties.xml")
    src = os.path.join(SERVER_REF_BUILD, "server-properties.xml")
    if not os.path.exists(src):
        return
    # if config already exists and no explicit changes, keep it
    if maincode is None and not patch_db and os.path.isfile(dst):
        return
    # read from existing dst to preserve current maincode, else from reference
    if os.path.isfile(dst) and maincode is None:
        with open(dst, "r") as f:
            content = f.read()
    else:
        with open(src, "r") as f:
            content = f.read()
    if maincode:
        content = re.sub(r'mainDatapackCode\s+value="[^"]*"',
                         f'mainDatapackCode value="{maincode}"', content)
        log_info(f"server-properties.xml mainDatapackCode={maincode}")
    if patch_db:
        # all 4 db sections: use db=catchchallenger, login=postgres, pass empty
        content = re.sub(r'(<db-(?:server|common|base|login)>.*?</db-(?:server|common|base|login)>)',
                         lambda m: re.sub(r'<db\s+value="[^"]*"', '<db value="catchchallenger"',
                                  re.sub(r'<login\s+value="[^"]*"', '<login value="postgres"',
                                  re.sub(r'<pass\s+value="[^"]*"', '<pass value=""', m.group(0)))),
                         content, flags=re.DOTALL)
        log_info("server-properties.xml patched DB: catchchallenger/postgres/localhost")
    if os.path.exists(dst) or os.path.islink(dst):
        os.remove(dst)
    with open(dst, "w") as f:
        f.write(content)

def clear_database_filedb(build_dir):
    db = os.path.join(build_dir, "database")
    if os.path.isdir(db):
        shutil.rmtree(db)
        log_info("removed database/")


def clear_database_postgresql():
    """Drop all tables and re-create from SQL schema files."""
    # drop all tables
    rc, out = run_cmd(
        ["psql", PG_DB, PG_USER, "-c",
         "DROP SCHEMA public CASCADE; CREATE SCHEMA public;"],
        ROOT, timeout=30)
    if rc != 0:
        log_info(f"warning: DROP SCHEMA failed: {out.strip()}")
    # re-create from SQL files in order: login, common, server
    for sql_file in ("catchchallenger-postgresql-login.sql",
                     "catchchallenger-postgresql-common.sql",
                     "catchchallenger-postgresql-server.sql"):
        path = os.path.join(PG_SQL_DIR, sql_file)
        if not os.path.isfile(path):
            log_info(f"warning: {path} not found")
            continue
        rc, out = run_cmd(["psql", PG_DB, PG_USER, "-f", path], ROOT, timeout=30)
        if rc != 0:
            log_info(f"warning: {sql_file}: {out.strip()[-200:]}")
    log_info("PostgreSQL tables re-created")

def remove_cache(build_dir):
    cache = os.path.join(build_dir, "datapack-cache.bin")
    if os.path.exists(cache):
        os.remove(cache)
        log_info("removed datapack-cache.bin")

def generate_cache(build_dir, bin_name=SERVER_BIN_NAME):
    binary = os.path.join(build_dir, bin_name)
    if not os.path.isfile(binary):
        log_fail("generate cache", "binary not found")
        return False
    log_info(f"generating cache: {binary} --save")
    rc, out = run_cmd([binary, "--save"], build_dir, timeout=SAVE_TIMEOUT)
    cache = os.path.join(build_dir, "datapack-cache.bin")
    if rc == 0 and os.path.isfile(cache):
        log_pass("generate cache", f"{os.path.getsize(cache)} bytes")
        return True
    log_fail("generate cache", f"rc={rc}")
    if out.strip():
        for line in out.splitlines()[-20:]:
            print(f"  | {line}")
    return False

SERVER_HOST = _config["server_host"]
SERVER_PORT = _config["server_port"]

def wait_for_port_free(port=SERVER_PORT, timeout=10):
    """Kill any process using the port, then wait until the port is free."""
    import socket, time as _time
    # kill any process on the port
    rc, out = run_cmd(["fuser", "-k", f"{port}/tcp"], ROOT, timeout=5)
    deadline = _time.monotonic() + timeout
    while _time.monotonic() < deadline:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            try:
                s.bind(("0.0.0.0", port))
                return True
            except OSError:
                _time.sleep(0.2)
    log_info(f"warning: port {port} still not free after {timeout}s")
    return False


def start_server(build_dir, bin_name=SERVER_BIN_NAME):
    wait_for_port_free()
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
        log_fail(label, "binary not found")
        return False, ""
    log_info(f"running: {bin_name} {' '.join(args)}")
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"
    try:
        p = subprocess.run(NICE_PREFIX + [binary] + args, cwd=build_dir, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env)
        out = p.stdout.decode(errors="replace")
        if p.returncode == 0:
            log_pass(label, "exit code 0")
            return True, out
        log_fail(label, f"exit code {p.returncode}")
        for line in out.splitlines()[-20:]:
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

def client_connect(client_build, label):
    """Shorthand: connect qtcpu800x600 to server."""
    ok, out = run_client(client_build, CLIENT_CPU_BIN,
                         ["--host", SERVER_HOST, "--port", str(SERVER_PORT),
                          "--autologin", "--character", "Player",
                          "--closewhenonmap"],
                         label,
                         success_marker="MapVisualiserPlayer::mapDisplayedSlot()")
    return ok, out


# ── per-server cache / NOXML cycle ─────────────────────────────────────────
def test_server_cache_cycle(server_pro, build_dir, server_label, client_build,
                            is_filedb, patch_db=False):
    tag = f"[{server_label}]"

    # 1. empty DB + no cache
    log_info(f"{tag} phase 1: empty database, no cache")
    if is_filedb:
        clear_database_filedb(build_dir)
    else:
        clear_database_postgresql()
    remove_cache(build_dir)
    set_http_datapack_mirror(build_dir, "")
    srv = start_server(build_dir)
    if srv:
        client_connect(client_build, f"{tag} client (no cache, empty DB)")
        stop_server()

    # 2. generate cache
    log_info(f"{tag} phase 2: generate cache")
    generate_cache(build_dir)

    # 3. with cache, NOXML off (recompile)
    log_info(f"{tag} phase 3: with cache, NOXML off (recompile)")
    if build_project(server_pro, build_dir,
                     f"{server_label} (NOXML off)", clean_first=True):
        setup_server_config(build_dir, patch_db=patch_db)
        generate_cache(build_dir)
        if not is_filedb:
            clear_database_postgresql()
        set_http_datapack_mirror(build_dir, "")
        srv = start_server(build_dir)
        if srv:
            client_connect(client_build, f"{tag} client (with cache, NOXML off)")
            stop_server()

    # 4. with cache, NOXML on (recompile with CATCHCHALLENGER_NOXML)
    log_info(f"{tag} phase 4: with cache, NOXML on (recompile)")
    if build_project(server_pro, build_dir,
                     f"{server_label} (NOXML on)",
                     extra_defines=["CATCHCHALLENGER_NOXML"],
                     clean_first=True):
        setup_server_config(build_dir, patch_db=patch_db)
        if is_filedb:
            clear_database_filedb(build_dir)
        else:
            clear_database_postgresql()
        set_http_datapack_mirror(build_dir, "")
        srv = start_server(build_dir)
        if srv:
            client_connect(client_build, f"{tag} client (with cache, NOXML on)")
            stop_server()

    # restore
    build_project(server_pro, build_dir,
                  f"{server_label} (restore)", clean_first=True)
    setup_server_config(build_dir, patch_db=patch_db)


# ── datapack testing ────────────────────────────────────────────────────────
def find_client_cache_dir():
    """Find the client datapack cache subdir used by --host/--port connections."""
    if not os.path.isdir(CLIENT_DATAPACK_CACHE):
        return None
    for name in (f"argument-{SERVER_HOST}-{SERVER_PORT}", f"{SERVER_HOST}:{SERVER_PORT}", SERVER_HOST):
        p = os.path.join(CLIENT_DATAPACK_CACHE, name)
        if os.path.isdir(p):
            return p
    return None


def test_xml_change_detection(build_dir, client_build):
    """
    Test that modifying/adding/deleting an XML in the server datapack
    is reflected in the client's local cache after reconnecting.
    """
    print(f"\n{C_CYAN}  -- XML change detection --{C_RESET}\n")

    server_dp = os.path.join(build_dir, "datapack")
    if not os.path.isdir(server_dp):
        log_fail("xml change", "no server datapack")
        return

    # 1. initial connect to establish baseline
    clear_database_filedb(build_dir)
    remove_cache(build_dir)
    set_http_datapack_mirror(build_dir, "")
    srv = start_server(build_dir)
    if not srv:
        return
    client_connect(client_build, "xml change: baseline connect")
    stop_server()

    cache_dir = find_client_cache_dir()
    if not cache_dir:
        log_fail("xml change", "client cache dir not found")
        return

    # pick an existing xml to modify
    xml_files = globmod.glob(os.path.join(server_dp, "**", "*.xml"), recursive=True)
    xml_files = [f for f in xml_files if os.path.isfile(f)]
    if not xml_files:
        log_fail("xml change", "no xml files in server datapack")
        return

    target_xml = xml_files[0]
    rel_path = os.path.relpath(target_xml, server_dp)
    client_file = os.path.join(cache_dir, rel_path)

    # 2. modify: append a comment to the xml
    log_info(f"modifying: {rel_path}")
    with open(target_xml, "a") as f:
        f.write("\n<!-- test-modification -->\n")

    # 3. add: create a new xml
    added_xml = os.path.join(server_dp, "test_added_file.xml")
    with open(added_xml, "w") as f:
        f.write('<?xml version="1.0" encoding="UTF-8"?>\n<test>added</test>\n')
    log_info("added: test_added_file.xml")

    # 4. delete: pick a second xml if available
    deleted_rel = None
    if len(xml_files) > 1:
        deleted_xml = xml_files[1]
        deleted_rel = os.path.relpath(deleted_xml, server_dp)
        os.remove(deleted_xml)
        log_info(f"deleted: {deleted_rel}")

    # 5. reconnect client
    clear_database_filedb(build_dir)
    remove_cache(build_dir)
    set_http_datapack_mirror(build_dir, "")
    srv = start_server(build_dir)
    if not srv:
        return
    client_connect(client_build, "xml change: reconnect after changes")
    stop_server()

    # 6. verify changes in client cache
    # check modified file has the marker
    if os.path.isfile(client_file):
        with open(client_file, "r") as f:
            content = f.read()
        if "test-modification" in content:
            log_pass("xml change: modified file synced")
        else:
            log_fail("xml change: modified file synced",
                     "marker not found in client cache")
    else:
        log_fail("xml change: modified file synced",
                 f"file not in client cache: {rel_path}")

    # check added file
    client_added = os.path.join(cache_dir, "test_added_file.xml")
    if os.path.isfile(client_added):
        log_pass("xml change: added file synced")
    else:
        log_fail("xml change: added file synced",
                 "test_added_file.xml not in client cache")

    # check deleted file removed from client
    if deleted_rel:
        client_deleted = os.path.join(cache_dir, deleted_rel)
        if not os.path.isfile(client_deleted):
            log_pass("xml change: deleted file removed")
        else:
            log_fail("xml change: deleted file removed",
                     f"{deleted_rel} still in client cache")


def check_mirror_empty(label, client_out):
    """Check that mirror markers show empty value (no HTTP mirror)."""
    base_empty = "CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase is now: \n"
    server_empty = "CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer is now: \n"
    if base_empty in client_out:
        log_pass(f"{label}: mirror base empty", "httpDatapackMirrorBase is empty")
    else:
        log_fail(f"{label}: mirror base empty",
                 "expected empty httpDatapackMirrorBase in client output")
        for line in client_out.splitlines()[-30:]:
            print(f"  | {line}")
    if server_empty in client_out:
        log_pass(f"{label}: mirror server empty", "httpDatapackMirrorServer is empty")
    else:
        log_fail(f"{label}: mirror server empty",
                 "expected empty httpDatapackMirrorServer in client output")
        for line in client_out.splitlines()[-30:]:
            print(f"  | {line}")

def test_client_datapack_mechanism(build_dir, client_build):
    """Test 3 client datapack cache scenarios."""
    server_datapack = os.path.join(build_dir, "datapack")
    if not os.path.isdir(server_datapack):
        log_fail("client datapack mechanism", "no server datapack")
        return
    # detect maincode from server-properties.xml
    sp_path = os.path.join(build_dir, "server-properties.xml")
    maincode = "test"
    if os.path.isfile(sp_path):
        with open(sp_path, "r") as f:
            sp = f.read()
        mc_m = re.search(r'mainDatapackCode\s+value="([^"]*)"', sp)
        if mc_m:
            maincode = mc_m.group(1)

    # case 3: empty client cache (full download)
    print(f"\n{C_CYAN}  -- Case 3: empty client cache --{C_RESET}\n")
    clear_database_filedb(build_dir)
    remove_cache(build_dir)
    setup_client_cache_empty()
    clear_client_bin_cache()
    set_client_hashes_random()
    set_http_datapack_mirror(build_dir, "")
    srv = start_server(build_dir)
    if srv:
        ok, out = client_connect(client_build, "case3: empty cache")
        if ok:
            check_mirror_empty("case3", out)
        check_datapack_match("case3", server_datapack, CLIENT_CACHE_DIR)
        check_datapack_main_match("case3", server_datapack, CLIENT_CACHE_DIR, maincode)
        stop_server()

    # case 2: partial client cache (diff download)
    print(f"\n{C_CYAN}  -- Case 2: partial client cache --{C_RESET}\n")
    clear_database_filedb(build_dir)
    remove_cache(build_dir)
    setup_client_cache_partial(server_datapack)
    clear_client_bin_cache()
    set_client_hashes_random()
    set_http_datapack_mirror(build_dir, "")
    srv = start_server(build_dir)
    if srv:
        ok, out = client_connect(client_build, "case2: partial cache")
        if ok:
            check_mirror_empty("case2", out)
        check_datapack_match("case2", server_datapack, CLIENT_CACHE_DIR)
        check_datapack_main_match("case2", server_datapack, CLIENT_CACHE_DIR, maincode)
        stop_server()

    # case 1: same datapack (hash match, no download)
    print(f"\n{C_CYAN}  -- Case 1: same datapack --{C_RESET}\n")
    clear_database_filedb(build_dir)
    set_http_datapack_mirror(build_dir, "")
    srv = start_server(build_dir)
    if srv:
        ok, out = client_connect(client_build, "case1: same datapack")
        if ok:
            check_mirror_empty("case1", out)
        check_datapack_match("case1", server_datapack, CLIENT_CACHE_DIR)
        check_datapack_main_match("case1", server_datapack, CLIENT_CACHE_DIR, maincode)
        stop_server()


def test_datapacks(build_dir, client_build):
    """Copy each datapack source to server, test each maincode in map/main/."""
    for dp_src, dp_name in DATAPACK_SOURCES:
        if not os.path.isdir(dp_src):
            log_fail(f"datapack {dp_name}", f"not found: {dp_src}")
            continue
        print(f"\n{C_CYAN}  -- Datapack: {dp_name} --{C_RESET}\n")
        setup_server_datapack(build_dir, dp_src)

        map_main = os.path.join(build_dir, "datapack", "map", "main")
        if not os.path.isdir(map_main):
            log_fail(f"datapack {dp_name}", "map/main/ not found")
            continue

        maincodes = sorted([d for d in os.listdir(map_main)
                            if os.path.isdir(os.path.join(map_main, d))])
        if not maincodes:
            log_fail(f"datapack {dp_name}", "no maincodes in map/main/")
            continue
        log_info(f"maincodes found: {', '.join(maincodes)}")

        for mc in maincodes:
            setup_server_config(build_dir, maincode=mc)
            clear_database_filedb(build_dir)
            remove_cache(build_dir)
            set_http_datapack_mirror(build_dir, "")
            srv = start_server(build_dir)
            if srv:
                client_connect(client_build,
                               f"datapack {dp_name} maincode={mc}")
                stop_server()
            else:
                log_fail(f"datapack {dp_name} maincode={mc}",
                         "server failed to start")

    # XML change detection test (uses last datapack)
    test_xml_change_detection(build_dir, client_build)


# ── cleanup ─────────────────────────────────────────────────────────────────
def cleanup(*_args):
    stop_server()
    sys.exit(1)

signal.signal(signal.SIGINT, cleanup)
signal.signal(signal.SIGTERM, cleanup)


# ── main ────────────────────────────────────────────────────────────────────
def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — Server Testing")
    print(f"{'='*60}{C_RESET}\n")

    # ═══════════════════════════════════════════════════════════════
    # Phase 0: COMPILATION — all flag + compiler combinations
    # ═══════════════════════════════════════════════════════════════
    print(f"\n{C_CYAN}--- Phase 0: Compilation (all combinations) ---{C_RESET}\n")

    total_builds = 0
    ti = 0
    while ti < len(SERVER_TARGETS):
        _pro, _name, _base, opt_flags = SERVER_TARGETS[ti]
        total_builds += len(COMPILERS) * len(flag_combinations(opt_flags))
        ti += 1
    log_info(f"total server compilation combinations: {total_builds}")

    ti = 0
    while ti < len(SERVER_TARGETS):
        pro_rel, target_name, build_base_rel, opt_flags = SERVER_TARGETS[ti]
        pro_path = os.path.join(ROOT, pro_rel)
        build_base = os.path.join(ROOT, build_base_rel)
        combos = flag_combinations(opt_flags)
        n_combos = len(COMPILERS) * len(combos)
        log_info(f"{target_name}: {n_combos} combinations "
                 f"({len(COMPILERS)} compilers x {len(combos)} flag sets)")

        ci = 0
        while ci < len(COMPILERS):
            compiler_name, compiler_spec = COMPILERS[ci]
            fi = 0
            while fi < len(combos):
                flags = combos[fi]
                suffix = combo_suffix(compiler_name, flags)
                build_dir = os.path.join(build_base,
                                         f"testing-combo-{target_name}-{suffix}")
                label = f"{target_name} {compiler_name}"
                if flags:
                    label += " " + "+".join(flags)
                build_project(pro_path, build_dir, label, compiler_spec,
                              extra_defines=list(flags) if flags else None)
                fi += 1
            ci += 1
        ti += 1

    # ═══════════════════════════════════════════════════════════════
    # Phases A–D: functional tests (use default gcc builds)
    # ═══════════════════════════════════════════════════════════════

    filedb_build = os.path.join(ROOT, "server/epoll/build/testing-filedb")
    cliepo_build = os.path.join(ROOT, "server/epoll/build/testing-cli-epoll")
    client_build = os.path.join(ROOT, "client/qtcpu800x600/build/testing-cpu")

    # build client once
    if not build_project(CLIENT_CPU_PRO, client_build, "qtcpu800x600"):
        summary()
        return

    # ── Phase A: server-filedb ──────────────────────────────────────
    print(f"\n{C_CYAN}--- Phase A: server-filedb ---{C_RESET}\n")
    if build_project(SERVER_FILEDB_PRO, filedb_build, "server-filedb"):
        for dp_index, dp_path in enumerate(DATAPACK_SOURCES):
            dp_src, dp_name = dp_path
            setup_server_datapack(filedb_build, dp_src)
            map_main = os.path.join(filedb_build, "datapack", "map", "main")
            maincodes = sorted([d for d in os.listdir(map_main)
                                if os.path.isdir(os.path.join(map_main, d))]) if os.path.isdir(map_main) else []
            if not maincodes:
                log_fail(f"filedb {dp_name}", "no maincodes in map/main/")
                continue
            log_info(f"{dp_name}: maincodes found: {', '.join(maincodes)}")
            for mc in maincodes:
                print(f"\n{C_CYAN}  -- filedb {dp_name} maincode={mc} --{C_RESET}\n")
                setup_server_config(filedb_build, maincode=mc)
                clear_database_filedb(filedb_build)
                remove_cache(filedb_build)
                set_http_datapack_mirror(filedb_build, "")
                set_map_visibility_minimize(filedb_build, "network")
                srv = start_server(filedb_build)
                if srv:
                    client_connect(client_build, f"filedb: {dp_name} maincode={mc}")
                    stop_server()

        test_server_cache_cycle(SERVER_FILEDB_PRO, filedb_build,
                                "filedb", client_build, is_filedb=True)

    # ── Phase B: server-cli-epoll (PostgreSQL) ──────────────────────
    print(f"\n{C_CYAN}--- Phase B: server-cli-epoll ---{C_RESET}\n")
    if build_project(SERVER_CLIEPO_PRO, cliepo_build, "server-cli-epoll"):
        for dp_index, dp_path in enumerate(DATAPACK_SOURCES):
            dp_src, dp_name = dp_path
            setup_server_datapack(cliepo_build, dp_src, patch_db=True)
            map_main = os.path.join(cliepo_build, "datapack", "map", "main")
            maincodes = sorted([d for d in os.listdir(map_main)
                                if os.path.isdir(os.path.join(map_main, d))]) if os.path.isdir(map_main) else []
            if not maincodes:
                log_fail(f"cli-epoll {dp_name}", "no maincodes in map/main/")
                continue
            log_info(f"{dp_name}: maincodes found: {', '.join(maincodes)}")
            for mc in maincodes:
                print(f"\n{C_CYAN}  -- cli-epoll {dp_name} maincode={mc} --{C_RESET}\n")
                setup_server_config(cliepo_build, maincode=mc, patch_db=True)
                clear_database_postgresql()
                set_http_datapack_mirror(cliepo_build, "")
                set_map_visibility_minimize(cliepo_build, "network")
                srv = start_server(cliepo_build)
                if srv:
                    client_connect(client_build, f"cli-epoll: {dp_name} maincode={mc}")
                    stop_server()
                else:
                    log_fail(f"cli-epoll {dp_name} maincode={mc}",
                             "server failed to start (PostgreSQL may not be available)")
                    break
        test_server_cache_cycle(SERVER_CLIEPO_PRO, cliepo_build,
                                "cli-epoll", client_build, is_filedb=False,
                                patch_db=True)
    else:
        log_fail("server-cli-epoll tests",
                 "compilation failed (libpq may not be available)")

    # ── Phase C: Datapack testing ───────────────────────────────────
    print(f"\n{C_CYAN}--- Datapack testing ---{C_RESET}\n")
    if os.path.isfile(os.path.join(filedb_build, SERVER_BIN_NAME)):
        test_datapacks(filedb_build, client_build)
    else:
        log_fail("datapack tests", "server-filedb binary not available")

    # ── Phase D: Client datapack mechanism ─────────────────────────
    print(f"\n{C_CYAN}--- Client datapack mechanism ---{C_RESET}\n")
    if os.path.isfile(os.path.join(filedb_build, SERVER_BIN_NAME)):
        test_client_datapack_mechanism(filedb_build, client_build)
    else:
        log_fail("client datapack mechanism", "server-filedb binary not available")

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
