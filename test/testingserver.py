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

# Drop the .pyc cache for this process so import diagnostic / build_paths /
# remote_build never lands a __pycache__/ dir in the source tree.  Set
# before the first LOCAL import; stdlib bytecode is unaffected.
import sys
sys.dont_write_bytecode = True


import os, sys, signal, subprocess, threading, shutil, multiprocessing, time, glob as globmod, json, re
from remote_build import (start_remote_builds, collect_remote_results,
                          REMOTE_SERVERS, REMOTE_NODES, node_runs, get_node,
                          setup_remote_server_runtime, start_remote_server,
                          stop_remote_server, get_remote_server_address,
                          cleanup_remote_node, SERVER_MAKE_SCRIPTS,
                          build_server_via_make)
import diagnostic
import build_paths
from cmd_helpers import (clamp_local, assert_port_or_fail,
                         assert_port_or_fail_with_remotes)
from remote_build import all_enabled_exec_nodes


def _host_port_from_args(args):
    """Extract --host / --port from a flat client-arg list. Returns
    (host, port) defaulting to ("localhost", 61917) when missing — that
    matches the qmake-era qtcpu800x600 default and lets run_client
    callers omit the explicit flags without breaking the port probe."""
    host = "localhost"
    port = 61917
    idx = 0
    while idx < len(args) - 1:
        if args[idx] == "--host":
            host = args[idx + 1]
        elif args[idx] == "--port":
            try:
                port = int(args[idx + 1])
            except (ValueError, TypeError):
                pass
        idx += 1
    return host, port

build_paths.ensure_root()

DIAG = diagnostic.parse_diag_args()
_DIAG_SUFFIX = diagnostic.build_dir_suffix(DIAG)

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
# Reference build dir whose server-properties.xml is the patch source for
# every per-test build below. After the qmake -> CMake migration the
# directory is `testing-filedb` (this file's filedb_build also points at
# it — see Phase A below), so use the same name here. Picking the old
# qmake path silently broke setup_server_config(): src didn't exist, so
# no XML patching happened and stale mainDatapackCode="official" leaked
# into datapack-pkmn runs whose only maincodes are gen2/test.
SERVER_REF_BUILD   = build_paths.build_path("server/epoll/build/testing-filedb")
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
# Both 5s and 30s were too short for the local-client→remote-server path:
# gdb startup + Qt offscreen plugin init + IPv6 connect + datapack hash
# exchange + map handshake on a slow remote node + a debug build easily
# exceeds 30s, so the closewhenonmap marker is never seen in time. 120s
# gives the client real headroom (still well below CI per-test budgets).
CLIENT_TIMEOUT       = 120
SAVE_TIMEOUT         = 600   # generating datapack-cache.bin via --save can be slow on MIPS / valgrind / first-pass cold caches

NICE_PREFIX = ["nice", "-n", "19", "ionice", "-c", "3"]

# ── compilers and server target flag combinations ─────────────────────────
COMPILERS = [
    ("gcc",   "linux-g++"),
    ("clang", "linux-clang"),
]

CXX_VERSIONS = ["c++11", "c++23"]

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
      "CATCHCHALLENGER_HARDENED"]),
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
_last_log_time = [time.monotonic()]
total_expected = [0]
server_proc = None

SCRIPT_NAME = os.path.basename(__file__)
from test_config import FAILED_JSON
import test_config


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
    li = 0
    _ctx = diagnostic.last_cmd_lines()
    while li < len(_ctx):
        print(_ctx[li])
        li += 1


# Flags tested as standalone single-flag combos rather than being part of
# the multi-flag powerset.  In every multi-flag combination produced by
# flag_combinations(), ALWAYS_ON_IN_COMBOS flags (that are also in the
# target's optional list) are always added and ALWAYS_OFF_IN_COMBOS flags
# are never added.
ALWAYS_ON_IN_COMBOS  = ["CATCHCHALLENGER_HARDENED"]
ALWAYS_OFF_IN_COMBOS = ["CATCHCHALLENGER_NOAUDIO"]


def flag_combinations(flags):
    """Generate test flag combos.

    Strategy:
      * Empty combo () — baseline.
      * Each flag in ALWAYS_ON_IN_COMBOS or ALWAYS_OFF_IN_COMBOS that is
        present in `flags` is tested as a standalone single-flag combo.
      * The remaining ("base") flags are powerset-combined; for each
        base subset we always add the ALWAYS_ON_IN_COMBOS flags that
        appear in `flags` and never add the ALWAYS_OFF_IN_COMBOS flags.
    """
    separate = []
    idx = 0
    while idx < len(ALWAYS_ON_IN_COMBOS):
        separate.append(ALWAYS_ON_IN_COMBOS[idx])
        idx += 1
    idx = 0
    while idx < len(ALWAYS_OFF_IN_COMBOS):
        separate.append(ALWAYS_OFF_IN_COMBOS[idx])
        idx += 1

    base_flags = []
    on_in_combos = []
    idx = 0
    while idx < len(flags):
        f = flags[idx]
        if f in ALWAYS_ON_IN_COMBOS:
            on_in_combos.append(f)
        elif f in ALWAYS_OFF_IN_COMBOS:
            pass
        else:
            base_flags.append(f)
        idx += 1

    seen = set()
    result = []

    seen.add(tuple())
    result.append(tuple())

    idx = 0
    while idx < len(flags):
        f = flags[idx]
        if f in separate:
            combo = (f,)
            if combo not in seen:
                seen.add(combo)
                result.append(combo)
        idx += 1

    n = len(base_flags)
    i = 0
    while i < (1 << n):
        combo = []
        j = 0
        while j < n:
            if i & (1 << j):
                combo.append(base_flags[j])
            j += 1
        k = 0
        while k < len(on_in_combos):
            combo.append(on_in_combos[k])
            k += 1
        combo_tuple = tuple(combo)
        if combo_tuple not in seen:
            seen.add(combo_tuple)
            result.append(combo_tuple)
        i += 1

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

_TEST_SERVER_PROPERTIES_SEED = """<?xml version="1.0"?>
<configuration>
    <server-port value="{port}"/>
    <automatic_account_creation value="true"/>
    <master>
        <external-server-port value="{port}"/>
    </master>
</configuration>
"""


def _seed_server_properties_if_missing(build_dir):
    """When server-properties.xml doesn't exist yet (very first server
    run on a fresh build dir), drop a minimal seed so subsequent regex
    patches operate on a real file. NormalServerGlobal::checkSettingsFile
    fills in every other field with its defaults but respects what we
    pre-set, so the random-port branch never fires."""
    xml = os.path.join(build_dir, "server-properties.xml")
    if not os.path.exists(xml):
        os.makedirs(build_dir, exist_ok=True)
        with open(xml, "w") as f:
            f.write(_TEST_SERVER_PROPERTIES_SEED.format(port=SERVER_PORT))


def set_automatic_account_creation(build_dir, value):
    """Flip <automatic_account_creation value="..."/> in server-properties.xml.
    Test clients use --autologin which needs the server to auto-create the
    account on first connect; the server's default is false (safe for
    production), so every test that drives an autologin client must enable
    this before starting the server, otherwise the server replies "Login
    refused" and the client never reaches the map."""
    _seed_server_properties_if_missing(build_dir)
    xml_path = os.path.join(build_dir, "server-properties.xml")
    if not os.path.exists(xml_path):
        return
    if os.path.islink(xml_path):
        target = os.path.realpath(xml_path)
        os.remove(xml_path)
        shutil.copy2(target, xml_path)
    with open(xml_path, "r") as f:
        content = f.read()
    content = re.sub(r'automatic_account_creation\s+value="[^"]*"',
                     f'automatic_account_creation value="{value}"', content)
    with open(xml_path, "w") as f:
        f.write(content)
    log_info(f'server-properties.xml automatic_account_creation="{value}"')


def set_server_port(build_dir, port):
    """Pin <server-port value="..."/> + <external-server-port> in
    server-properties.xml. NormalServerGlobal::checkSettingsFile() picks
    a RANDOM port (10000 + rand()%55535) the first time the server runs
    against a fresh config, which means the test client / TCP probe can
    never connect to a stable port. Force the configured test port
    (config.json's `server_port`, 61917 by default) on every server
    start so the probe + client both hit the same address as the bind."""
    _seed_server_properties_if_missing(build_dir)
    xml_path = os.path.join(build_dir, "server-properties.xml")
    if not os.path.exists(xml_path):
        return
    if os.path.islink(xml_path):
        target = os.path.realpath(xml_path)
        os.remove(xml_path)
        shutil.copy2(target, xml_path)
    with open(xml_path, "r") as f:
        content = f.read()
    content = re.sub(r'(?<!external-)server-port\s+value="[^"]*"',
                     f'server-port value="{port}"', content)
    content = re.sub(r'external-server-port\s+value="[^"]*"',
                     f'external-server-port value="{port}"', content)
    with open(xml_path, "w") as f:
        f.write(content)
    log_info(f'server-properties.xml server-port={port}')


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
    # clamp_local caps any caller-supplied timeout at 600s — matches the
    # cmd_helpers.LOCAL_MAX_TIMEOUT policy. A returned (-1, "TIMEOUT…")
    # tuple makes log_fail print "TIMEOUT" instead of a generic rc=-1.
    timeout = clamp_local(timeout)
    diagnostic.record_cmd(NICE_PREFIX + list(args), cwd)
    try:
        p = subprocess.run(NICE_PREFIX + list(args), cwd=cwd, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           env=env or os.environ)
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"TIMEOUT after {timeout}s"

def build_project(pro_file, build_dir, label, compiler_spec="linux-g++",
                   extra_defines=None, clean_first=False, cxx_version=None,
                   extra_qmake_args=None):
    """Phase 5 (qmake -> CMake) port. Same call shape as the qmake-era
    function but drives `cmake` + `cmake --build` via cmake_helpers."""
    import cmake_helpers as _ch
    return _ch.build_project(
        pro_file, build_dir, label,
        root=ROOT, nproc=NPROC,
        log_info=log_info, log_pass=log_pass, log_fail=log_fail,
        ensure_dir=ensure_dir, run_cmd=run_cmd,
        diag=DIAG, diag_module=diagnostic,
        compiler_spec=compiler_spec,
        extra_defines=extra_defines,
        clean_first=clean_first,
        cxx_version=cxx_version,
        extra_qmake_args=extra_qmake_args,
    )

def setup_server_datapack(build_dir, datapack_src, patch_db=False):
    """Symlink the pre-staged datapack into <build>/datapack and adapt
    server-properties.xml mainDatapackCode to match. The source tree
    was rsynced once at all.sh startup (see stage_datapacks.py /
    CLAUDE.md "Datapack staging cache"), so per-test setup is now an
    instant ln -s instead of a 5-15s shutil.copytree."""
    import datapack_stage as _ds
    staged = _ds.staged_local(datapack_src)
    dst = os.path.join(build_dir, "datapack")
    if os.path.islink(dst) or os.path.isfile(dst):
        os.remove(dst)
    elif os.path.isdir(dst):
        shutil.rmtree(dst)
    os.makedirs(build_dir, exist_ok=True)
    os.symlink(staged, dst)
    log_info(f"symlinked datapack {dst} -> {staged} (src {datapack_src})")
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


_SQL_BACKEND_DIRS = {
    "MySQL":      "mysql",
    "PostgreSQL": "postgresql",
    "SQLite":     "sqlite",
}


def wipe_via_sql(backend):
    """Drop every table in db `catchchallenger` on localhost (default port)
    and replay every .sql under server/databases/<backend>/. ONLY called
    when an exec_node opts in via execute_sql=[...] in remote_nodes.json;
    default test runs use the file-db backend exclusively and never call
    this helper.

    Uses the host-installed SQL CLI (mysql / psql / sqlite3). The exec
    node operator is responsible for installing the daemon and creating
    an empty `catchchallenger` DB; missing CLI = backend skipped, not
    failed.

    See CLAUDE.md "Database backends in testing*.py — file_db only on
    host, SQL gated to remote_nodes.json"."""
    sub = _SQL_BACKEND_DIRS.get(backend)
    if sub is None:
        log_info(f"wipe_via_sql: unknown backend {backend!r}")
        return False
    sql_dir = os.path.join(ROOT, "server", "databases", sub)
    if not os.path.isdir(sql_dir):
        log_info(f"wipe_via_sql: {sql_dir} missing")
        return False
    db = "catchchallenger"
    # The DB and user `catchchallenger` are operator-managed (see
    # CLAUDE.md "testingserver.py + execute_sql contract:
    # pre-configured DBs only"). We only operate ON the data: drop
    # every table, replay the schema. Never touch the daemon, never
    # CREATE/DROP DATABASE, never CREATE USER. Auth is always
    # localhost + default port + catchchallenger/catchchallenger.
    SQL_USER = "catchchallenger"
    SQL_PASS = "catchchallenger"
    if backend == "PostgreSQL":
        if not shutil.which("psql"):
            log_info("wipe_via_sql: psql not installed, skipping PostgreSQL")
            return False
        env = os.environ.copy()
        env["PGPASSWORD"] = SQL_PASS
        # List → drop loop in a single PL/pgSQL block so we avoid name
        # quoting / case issues on the python side.
        drop_sql = (
            "DO $$ "
            "DECLARE r RECORD; "
            "BEGIN "
            "FOR r IN (SELECT tablename FROM pg_tables WHERE schemaname=current_schema()) LOOP "
            "  EXECUTE 'DROP TABLE IF EXISTS ' || quote_ident(r.tablename) || ' CASCADE'; "
            "END LOOP; "
            "END $$;")
        psql_base = ["psql", "-h", "localhost", "-U", SQL_USER, "-d", db]
        try:
            subprocess.run(psql_base + ["-c", drop_sql], env=env,
                           timeout=30, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT)
            for fn in sorted(os.listdir(sql_dir)):
                if fn.endswith(".sql"):
                    subprocess.run(psql_base + ["-f", os.path.join(sql_dir, fn)],
                                   env=env, timeout=60,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT)
        except subprocess.TimeoutExpired:
            log_info("wipe_via_sql: PostgreSQL operation timed out")
            return False
        log_info(f"wipe_via_sql: PostgreSQL `{db}` tables dropped + reloaded from {sql_dir}")
        return True
    if backend == "MySQL":
        if not shutil.which("mysql"):
            log_info("wipe_via_sql: mysql not installed, skipping MySQL")
            return False
        # MySQL: use information_schema to enumerate, generate a single
        # DROP TABLE statement covering every table in the DB. SET
        # FOREIGN_KEY_CHECKS=0 lets the drops succeed regardless of
        # FK ordering; the value is connection-local so it does not
        # leak after the wipe.
        drop_script = (
            "SET FOREIGN_KEY_CHECKS=0; "
            f"SET GROUP_CONCAT_MAX_LEN=1048576; "
            f"SELECT IFNULL(GROUP_CONCAT('`',table_name,'`'),'') "
            f"INTO @tbls FROM information_schema.tables "
            f"WHERE table_schema='{db}'; "
            "SET @sql := IF(@tbls='','SELECT 1', CONCAT('DROP TABLE IF EXISTS ', @tbls)); "
            "PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt; "
            "SET FOREIGN_KEY_CHECKS=1;")
        mysql_base = ["mysql", "-h", "localhost", "-u", SQL_USER,
                      f"-p{SQL_PASS}", db]
        try:
            subprocess.run(mysql_base + ["-e", drop_script],
                           timeout=30, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT)
            for fn in sorted(os.listdir(sql_dir)):
                if fn.endswith(".sql"):
                    with open(os.path.join(sql_dir, fn), "rb") as _src:
                        subprocess.run(mysql_base, stdin=_src,
                                       timeout=60, stdout=subprocess.PIPE,
                                       stderr=subprocess.STDOUT)
        except subprocess.TimeoutExpired:
            log_info("wipe_via_sql: MySQL operation timed out")
            return False
        log_info(f"wipe_via_sql: MySQL `{db}` tables dropped + reloaded from {sql_dir}")
        return True
    if backend == "SQLite":
        if not shutil.which("sqlite3"):
            log_info("wipe_via_sql: sqlite3 not installed, skipping SQLite")
            return False
        # SQLite has no daemon and no auth. The harness uses a single
        # fixed file at <tmpfs_root>/catchchallenger.sqlite3; the
        # operator just needs `sqlite3` on $PATH.
        db_file = os.path.join(test_config.TMPFS_ROOT, "catchchallenger.sqlite3")
        if os.path.exists(db_file):
            run_cmd(["bash", "-c",
                     f"sqlite3 {db_file} \""
                     f"SELECT 'DROP TABLE IF EXISTS \\\"' || name || '\\\";' "
                     f"FROM sqlite_master WHERE type='table'\" "
                     f"| sqlite3 {db_file}"],
                    ROOT, timeout=30)
        for fn in sorted(os.listdir(sql_dir)):
            if fn.endswith(".sql"):
                run_cmd(["bash", "-c",
                         f"sqlite3 {db_file} < {os.path.join(sql_dir, fn)}"],
                        ROOT, timeout=60)
        log_info(f"wipe_via_sql: SQLite `{db_file}` tables dropped + reloaded from {sql_dir}")
        return True
    log_info(f"wipe_via_sql: unsupported backend {backend!r}")
    return False


# Compatibility alias for the legacy direct-postgres path. The name is
# preserved so any caller still in flight resolves, but it now no-ops
# unless the caller is inside an opt-in execute_sql=[PostgreSQL] block;
# otherwise default runs go through the file-db only flow.
def clear_database_postgresql():
    log_info("clear_database_postgresql: skipped (file-db default; opt in via "
             "execute_sql in remote_nodes.json)")

def remove_cache(build_dir):
    cache = os.path.join(build_dir, "datapack-cache.bin")
    if os.path.exists(cache):
        os.remove(cache)
        log_info("removed datapack-cache.bin")

def sync_cache_mtime(build_dir):
    """Set datapack-cache.bin mtime to match server-properties.xml mtime.
    The server only loads the cache when both mtimes match."""
    cache_path = os.path.join(build_dir, "datapack-cache.bin")
    xml_path = os.path.join(build_dir, "server-properties.xml")
    if os.path.isfile(cache_path) and os.path.isfile(xml_path):
        xml_stat = os.stat(xml_path)
        os.utime(cache_path, (xml_stat.st_atime, xml_stat.st_mtime))
        log_info("synced datapack-cache.bin mtime to match server-properties.xml")

def generate_cache(build_dir, bin_name=SERVER_BIN_NAME):
    binary = os.path.join(build_dir, bin_name)
    if not os.path.isfile(binary):
        log_fail("generate cache", "binary not found")
        return False
    log_info(f"generating cache: {binary} --save")
    wrapper = diagnostic.runtime_wrapper(DIAG)
    cache_env = os.environ.copy()
    for k, v in diagnostic.runtime_env(DIAG).items():
        cache_env[k] = v
    rc, out = run_cmd(wrapper + [binary, "--save"], build_dir,
                      timeout=diagnostic.scale_timeout(DIAG, SAVE_TIMEOUT),
                      env=cache_env)
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


_server_output_lines = []

def start_server(build_dir, bin_name=SERVER_BIN_NAME):
    wait_for_port_free()
    global server_proc, _server_output_lines
    binary = os.path.join(build_dir, bin_name)
    if not os.path.isfile(binary):
        log_fail("start server", f"binary not found: {binary}")
        return None
    # Test clients all use --autologin (auto-create + auto-login on first
    # connect). The server's default is automatic_account_creation=false
    # (safe for production); flip it on right before launch so the test
    # client isn't refused with "Login failed: account does not exist".
    set_automatic_account_creation(build_dir, "true")
    # Pin the listen port. checkSettingsFile picks a random port on
    # first run; the test infra and TCP probe expect a stable port.
    set_server_port(build_dir, SERVER_PORT)
    log_info(f"starting server: {binary}")
    env = os.environ.copy()
    for k, v in diagnostic.runtime_env(DIAG).items():
        env[k] = v
    wrapper = diagnostic.runtime_wrapper(DIAG)
    srv_args = NICE_PREFIX + wrapper + [binary]
    diagnostic.record_cmd(srv_args, build_dir)
    server_proc = subprocess.Popen(
        srv_args, cwd=build_dir,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env,
        preexec_fn=os.setsid)
    ready = threading.Event()
    _server_output_lines = []
    def reader():
        for raw in iter(server_proc.stdout.readline, b""):
            line = raw.decode(errors="replace").rstrip("\n")
            _server_output_lines.append(line)
            if "correctly bind:" in line:
                ready.set()
    t = threading.Thread(target=reader, daemon=True)
    t.start()
    if ready.wait(timeout=diagnostic.scale_timeout(DIAG, SERVER_READY_TIMEOUT)):
        log_pass("server start", "correctly bind: detected")
        return server_proc
    log_fail("server start", "timeout waiting for 'correctly bind:'")
    for l in _server_output_lines[-30:]:
        print(f"  | {l}")
    stop_server()
    return None


def dump_server_output(label):
    """Print last 30 lines of the server's stdout — used after a client
    `stateChanged(1)→0` drop to surface the kick reason
    (`characterSelectionIsWrong`, profile mismatch, hexa-parse fail,
    etc.) that's emitted only on the SERVER side."""
    print(f"  | -- server output (last 30 lines): --")
    li = max(0, len(_server_output_lines) - 30)
    while li < len(_server_output_lines):
        print(f"  | SRV: {_server_output_lines[li]}")
        li += 1

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
    """Run a client and watch its stdout/stderr in real time. Returns
    (ok, output).

    Early-FAIL triggers (kill the client immediately, no need to wait
    out the timeout):
      * "Connection refused by the server" — server actively rejected.
      * stateChanged(0) AFTER stateChanged(1) — the client briefly
        connected (or tried) and then dropped before reaching the map,
        which always means the test will fail; killing now saves
        ~30-120s of pointless wall time.

    Success marker (if given) — a line containing it lets the run end
    early as PASS. Without a marker, exit-code-0 is success.
    """
    binary = os.path.join(build_dir, bin_name)
    if not os.path.isfile(binary):
        log_fail(label, "binary not found")
        return False, ""
    # TCP probe: server reports "correctly bind: detected" but bind only
    # proves listen() ran. Local-only probe — see CLAUDE.md "Network —
    # test box ↔ remote / execution nodes": the firewall blocks NEW
    # connections from any 2803:1920::2:/112 node back to the test box,
    # so probing from an exec_node would always FAIL by design when the
    # server is running on the test box (which is the case here).
    host, port = _host_port_from_args(args)
    if not assert_port_or_fail(host, port, log_fail, label):
        return False, ""
    log_info(f"running: {bin_name} {' '.join(args)}")
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"
    for k, v in diagnostic.runtime_env(DIAG).items():
        env[k] = v
    timeout = clamp_local(diagnostic.scale_timeout(DIAG, timeout))
    wrapper = diagnostic.runtime_wrapper(DIAG)
    if wrapper:
        run_args = wrapper + [binary] + args
    else:
        run_args = [binary] + args
    diagnostic.record_cmd(NICE_PREFIX + run_args, build_dir)
    proc = subprocess.Popen(
        NICE_PREFIX + run_args, cwd=build_dir,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env,
        preexec_fn=os.setsid)
    output_lines = []
    done = threading.Event()
    # `outcome` is set by the reader thread to one of:
    #   ("pass", marker_string)
    #   ("fail", reason_string)
    # (None) means the loop ended without hitting any pattern — falls
    # back to exit-code-based judgement after the process drains.
    outcome = [None]
    seen_state1 = [False]

    def reader():
        for raw in iter(proc.stdout.readline, b""):
            line = raw.decode(errors="replace").rstrip("\n")
            output_lines.append(line)
            # Early PASS: success marker hit
            if success_marker and success_marker in line:
                outcome[0] = ("pass", success_marker)
                done.set()
                return
            # Early FAIL: explicit refusal message (qtcpu800x600 prints
            # via qWarning; qtopengl via ScreenTransition::errorString).
            if "Connection refused by the server" in line:
                outcome[0] = ("fail", "Connection refused by the server")
                done.set()
                return
            # Track stateChanged(1) → stateChanged(0) transitions: client
            # opened socket then dropped before reaching the map, which
            # is always a FAIL. Match Api_protocol_Qt::stateChanged(N)
            # and MainWindow::stateChanged(N) — both emit the same shape.
            if "stateChanged(1)" in line:
                seen_state1[0] = True
            elif seen_state1[0] and "stateChanged(0)" in line:
                outcome[0] = (
                    "fail",
                    "stateChanged(0) after stateChanged(1) — "
                    "client connected then dropped before map")
                done.set()
                return
        done.set()  # process exited on its own; main thread judges via rc

    t = threading.Thread(target=reader, daemon=True)
    t.start()
    triggered = done.wait(timeout=timeout)
    out = "\n".join(output_lines)

    def _kill():
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        except ProcessLookupError:
            return
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            except ProcessLookupError:
                pass
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                pass

    if not triggered:
        _kill()
        log_fail(label, f"timeout after {timeout}s")
        for line in output_lines[-20:]:
            print(f"  | {line}")
        return False, out

    if outcome[0] is not None:
        kind, detail = outcome[0]
        # Always kill the proc — even on success_marker we don't need to
        # wait for clean shutdown; --closewhenonmap usually exits within
        # a second but a hung post-success exit shouldn't block us.
        _kill()
        if kind == "pass":
            log_pass(label, f"early pass ({detail})")
            return True, out
        log_fail(label, detail)
        for line in output_lines[-20:]:
            print(f"  | {line}")
        # Surface the SERVER's view too — for "client connected then
        # dropped" the kick reason (characterSelectionIsWrong / profile
        # mismatch / hexa-parse fail) is only logged on the server side.
        if "client connected then dropped" in detail:
            dump_server_output(label)
        return False, out

    # Reader returned without a verdict — process exited on its own.
    rc = proc.wait()
    if rc == 0:
        log_pass(label, "exit code 0")
        return True, out
    log_fail(label, f"exit code {rc}")
    for line in output_lines[-40:]:
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

    # Both back-ends keep some state on disk (filedb:
    # <build_dir>/database/, postgres: catchchallenger DB) AND cli-epoll
    # additionally serialises per-character map state on the local
    # filesystem via Client::serialize/parse, so a maincode switch
    # leaves stale map_file_database_id values in BOTH stores. Clear
    # both back-ends on every phase boundary; whichever we're not
    # actually using is a cheap no-op.
    def _wipe_state():
        clear_database_filedb(build_dir)
        if not is_filedb:
            clear_database_postgresql()

    # 1. empty DB + no cache
    log_info(f"{tag} phase 1: empty database, no cache")
    _wipe_state()
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
        _wipe_state()
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
        _wipe_state()
        set_http_datapack_mirror(build_dir, "")
        sync_cache_mtime(build_dir)
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
    # The XML-change detection test MUTATES files inside server_dp
    # (append-on-modify, create-new, delete). server_dp is normally a
    # symlink to the shared staged datapack at <LOCAL_CACHE_ROOT>/<id>/,
    # so mutating it would corrupt the cache for every subsequent test
    # (see CLAUDE.md "When you must NOT symlink").
    #
    # Hardening invariant: regardless of what shape server_dp is in on
    # entry (symlink, real dir, missing, partial), we ALWAYS materialise
    # a fresh private copy at <build_dir>/datapack-private and re-point
    # server_dp at it. The private copy lives OUTSIDE the staged cache
    # tree, so an os.remove() / open(..,"a") / etc. path inside this test
    # cannot reach the cache via symlink resolution. The finally block
    # restores server_dp to the original staged symlink. Even if the
    # interpreter is killed mid-test, the cache is untouched because we
    # never had a writable path into it.
    private_dir = os.path.join(build_dir, "datapack-private")
    # always start from a clean private copy of the staged source
    if os.path.islink(private_dir) or os.path.isfile(private_dir):
        os.remove(private_dir)
    elif os.path.isdir(private_dir):
        shutil.rmtree(private_dir)
    target = os.path.realpath(server_dp)
    log_info(f"xml change: building private copy at {private_dir} (from {target})")
    shutil.copytree(target, private_dir, ignore=shutil.ignore_patterns(".git"))
    # repoint server_dp at the private copy
    if os.path.islink(server_dp) or os.path.isfile(server_dp):
        os.remove(server_dp)
    elif os.path.isdir(server_dp):
        shutil.rmtree(server_dp)
    os.symlink(private_dir, server_dp)

    try:
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

        # Pick deterministic, NON-CRITICAL xml files to mutate. Earlier
        # versions of this test grabbed `xml_files[0]` and `xml_files[1]`
        # from an unsorted glob; depending on filesystem order that
        # often deleted `crafting/recipes.xml`, `monsters/skill.xml`,
        # or `items/items.xml`, which makes
        # `BaseServer::preload_finish()` (HARDENED build) abort with
        # "CommonDatapack::commonDatapack.crafingRecipesMaxId==0".
        # The point of this test is to exercise mod/add/delete sync,
        # NOT to break the server's preload sanity checks — pick
        # files we know aren't required for preload.
        #   modify: informations.xml — datapack version manifest, the
        #     server reads it but missing/changed content does not
        #     trip the HARDENED asserts.
        #   delete: a single quest text file — quest text is loaded
        #     per-quest and the server tolerates it being missing
        #     (logs a warning, skips that quest's narrative). Pick the
        #     LAST one alphabetically so the chosen path is stable
        #     across both maincodes.
        target_xml = os.path.join(server_dp, "informations.xml")
        if not os.path.isfile(target_xml):
            log_fail("xml change", "informations.xml not in server datapack")
            return
        rel_path = "informations.xml"
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

        # 4. delete: a known-noncritical quest text file.
        deleted_rel = None
        quest_texts = sorted(globmod.glob(
            os.path.join(server_dp, "map", "main", "*", "quests", "*", "text.xml")))
        if quest_texts:
            deleted_xml = quest_texts[-1]
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
    finally:
        # Always restore the datapack symlink to the shared staged cache,
        # even on early return / exception. Drop the private copy too —
        # it was mutated and downstream tests might pick it up if we
        # left it behind.
        try:
            if os.path.islink(server_dp) or os.path.isfile(server_dp):
                os.remove(server_dp)
            elif os.path.isdir(server_dp):
                shutil.rmtree(server_dp)
            if DATAPACK_SOURCES:
                import datapack_stage as _ds
                last_src = DATAPACK_SOURCES[-1][0]
                if os.path.isdir(last_src):
                    staged = _ds.staged_local(last_src)
                    os.symlink(staged, server_dp)
                    log_info(f"xml change: restored server datapack symlink → {staged}")
            if os.path.isdir(private_dir):
                shutil.rmtree(private_dir, ignore_errors=True)
        except Exception as e:
            log_info(f"xml change: cleanup error: {e}")


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

    failed_cases = load_failed_cases()
    if failed_cases is not None and len(failed_cases) == 0:
        log_info("all previously passed, skipping (delete test/failed.json for full re-run)")
        return

    # ═══════════════════════════════════════════════════════════════
    # Phase 0: COMPILATION — all flag + compiler combinations
    # ═══════════════════════════════════════════════════════════════
    print(f"\n{C_CYAN}--- Phase 0: Compilation (all combinations) ---{C_RESET}\n")

    if diagnostic.is_active(DIAG):
        log_info(f"diagnostic mode: {diagnostic.describe(DIAG)}")
    # start remote builds in parallel with local builds
    remote_pro_rels = [pro_rel for pro_rel, _, _, _ in SERVER_TARGETS]
    if failed_cases is None:
        remote_threads, remote_results, remote_lock = start_remote_builds(
            remote_pro_rels, diag=DIAG)
        log_info("remote builds started in background")
    else:
        remote_threads, remote_results, remote_lock = [], [], None
        log_info("resume mode: skipping remote builds")

    total_builds = 0
    ti = 0
    while ti < len(SERVER_TARGETS):
        _pro, _name, _base, opt_flags = SERVER_TARGETS[ti]
        total_builds += len(CXX_VERSIONS) * len(COMPILERS) * len(flag_combinations(opt_flags))
        ti += 1
    # compute total expected results
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
        remote_count += len(["server_start", "client_connect"])  # per remote
        ri += 1
    # count maincodes across all datapacks
    n_maincodes = 0
    di = 0
    while di < len(DATAPACK_SOURCES):
        dp_src = DATAPACK_SOURCES[di][0]
        mm = os.path.join(dp_src, "map", "main")
        if os.path.isdir(mm):
            n_maincodes += len([d for d in os.listdir(mm)
                                if os.path.isdir(os.path.join(mm, d))])
        di += 1
    # functional tests computation
    server_variants = [SERVER_FILEDB_PRO, SERVER_CLIEPO_PRO]
    cache_cycle_phases = ["empty_db", "generate_cache", "noxml_off", "noxml_on"]
    cache_cycle_checks = ["build", "server_start", "client"]
    datapack_scenarios = ["empty_cache", "partial_cache", "same_datapack"]
    checks_per_scenario = ["server_start", "client", "datapack_base_match", "datapack_main_match"]
    xml_change_checks = ["baseline_connect", "reconnect", "modified_synced", "added_synced", "deleted_removed", "extra"]
    mc_checks = ["server_start", "client"]
    remote_phase_checks = ["setup", "server_start", "client"]
    client_build = 1
    per_variant = 1 + n_maincodes * len(mc_checks) + len(cache_cycle_phases) * len(cache_cycle_checks)
    phase_c = n_maincodes * len(mc_checks) + len(xml_change_checks)
    phase_d = len(datapack_scenarios) * len(checks_per_scenario)
    phase_e = len(_RS) * len(remote_phase_checks)
    # Phase A2: 2 IO backends (poll, io_uring) × {build, server_start, client}
    phase_a2 = 2 * 3
    functional = client_build + len(server_variants) * per_variant + phase_a2 + phase_c + phase_d + phase_e
    total_expected[0] = total_builds + remote_count + functional
    log_info(f"total server compilation combinations: {total_builds}")
    log_info(f"total expected results: {total_expected[0]}")

    ti = 0
    while ti < len(SERVER_TARGETS):
        pro_rel, target_name, build_base_rel, opt_flags = SERVER_TARGETS[ti]
        pro_path = os.path.join(ROOT, pro_rel)
        build_base = build_paths.build_path(build_base_rel)
        combos = flag_combinations(opt_flags)
        n_combos = len(CXX_VERSIONS) * len(COMPILERS) * len(combos)
        log_info(f"{target_name}: {n_combos} combinations "
                 f"({len(CXX_VERSIONS)} c++ versions x {len(COMPILERS)} compilers x {len(combos)} flag sets)")

        vi = 0
        while vi < len(CXX_VERSIONS):
            cxx_ver = CXX_VERSIONS[vi]
            ci = 0
            while ci < len(COMPILERS):
                compiler_name, compiler_spec = COMPILERS[ci]
                fi = 0
                while fi < len(combos):
                    flags = combos[fi]
                    suffix = combo_suffix(compiler_name, flags)
                    build_dir = os.path.join(build_base,
                                             f"testing-combo-{target_name}-{cxx_ver}-{suffix}")
                    label = f"{target_name} {cxx_ver} {compiler_name}"
                    if flags:
                        label += " " + "+".join(flags)
                    if should_run(f"compile {label}", failed_cases):
                        build_project(pro_path, build_dir, label, compiler_spec,
                                      extra_defines=list(flags) if flags else None,
                                      cxx_version=cxx_ver)
                    fi += 1
                ci += 1
            vi += 1
        ti += 1

    # ── Phase 0c (was: qmake-vs-make.py parity check) — REMOVED with the
    # cmake migration. The qmake build path no longer exists, so there's
    # nothing to compare make.py output against. Phase 0b below now
    # exercises both compilers + flag combos via the make.py wrappers,
    # which themselves drive cmake.

    # ── Phase 0b: same matrix via make*.py (gcc + clang, all flag combos)
    # The make scripts hardcode their C++ standard; we don't iterate cxx_ver here.
    log_info("Phase 0b: compiling each server target via its make*.py companion")
    ti = 0
    while ti < len(SERVER_TARGETS):
        pro_rel, target_name, build_base_rel, opt_flags = SERVER_TARGETS[ti]
        if pro_rel not in SERVER_MAKE_SCRIPTS:
            ti += 1
            continue
        build_base = build_paths.build_path(build_base_rel)
        combos = flag_combinations(opt_flags)
        ci = 0
        while ci < len(COMPILERS):
            compiler_name = COMPILERS[ci][0]
            fi = 0
            while fi < len(combos):
                flags = combos[fi]
                suffix = combo_suffix(compiler_name, flags)
                build_dir = os.path.join(build_base,
                                         f"testing-make-{target_name}-{suffix}")
                label = f"{target_name} make.py {compiler_name}"
                if flags:
                    label += " " + "+".join(flags)
                test_name = f"compile {label}"
                if should_run(test_name, failed_cases):
                    log_info(f"make.py {label}")
                    rc, out = build_server_via_make(
                        pro_rel, build_dir, label,
                        extra_defines=list(flags) if flags else None,
                        compiler=compiler_name, diag=DIAG)
                    if rc == 0:
                        log_pass(test_name)
                    else:
                        log_fail(test_name, f"rc={rc}")
                        if out.strip():
                            for line in out.splitlines()[-25:]:
                                print(f"  | {line}")
                fi += 1
            ci += 1
        ti += 1

    # collect remote build results
    if remote_threads:
        log_info("waiting for remote builds to finish...")
    remote = collect_remote_results(remote_threads, remote_results, remote_lock) if remote_threads else []
    idx = 0
    while idx < len(remote):
        name, ok, detail = remote[idx]
        if ok:
            log_pass(name, detail)
        else:
            log_fail(name, detail)
        idx += 1

    # ═══════════════════════════════════════════════════════════════
    # Phases A–D: functional tests (use default gcc builds)
    # ═══════════════════════════════════════════════════════════════

    filedb_build = build_paths.build_path("server/epoll/build/testing-filedb" + _DIAG_SUFFIX)
    cliepo_build = build_paths.build_path("server/epoll/build/testing-cli-epoll" + _DIAG_SUFFIX)
    client_build = build_paths.build_path("client/qtcpu800x600/build/testing-cpu" + _DIAG_SUFFIX)

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

    # ── Phase A2: server-filedb alternative IO backends ─────────────
    # Build server-filedb against each of the three IO multiplexers
    # supported by server/epoll/Epoll.cpp:
    #   * select(2)  — most portable, FD_SETSIZE-bound; targets old hardware
    #   * epoll(7)   — Linux native, default backend (no flag = empty CONFIG+)
    #   * io_uring   — kernel >=5.13, async via liburing
    # Each backend gets one full client run that connects, logs in, and
    # walks onto the map. The three flags are mutually exclusive (the
    # root CMakeLists / Epoll.hpp #error if two are set together) and
    # are wired via CONFIG+= in catchchallenger-server-epoll.pri (the
    # io_uring path also pulls in -luring), so we pass them through
    # extra_qmake_args. The "epoll" entry passes "" to keep the matrix
    # symmetrical; build_project() ignores empty extra_qmake_args.
    print(f"\n{C_CYAN}--- Phase A2: server-filedb IO backends (select, epoll, io_uring) ---{C_RESET}\n")
    # IO_URING defaults ON on Linux (root CMakeLists.txt), so the
    # "epoll" matrix entry must explicitly disable it via the synthetic
    # `catchchallenger_force_epoll` CONFIG flag — empty extra_args
    # would silently pick the default (io_uring) and hide the epoll
    # path from coverage. The select/iouring entries set their own
    # backend ON; the helper turns the others OFF for them.
    backends = [
        ("select",  "CONFIG+=catchchallenger_select"),
        ("epoll",   "CONFIG+=catchchallenger_force_epoll"),
        ("iouring", "CONFIG+=catchchallenger_io_uring"),
    ]
    bi = 0
    while bi < len(backends):
        backend_name, qarg = backends[bi]
        backend_build = build_paths.build_path(
            "server/epoll/build/testing-filedb-" + backend_name + _DIAG_SUFFIX)
        print(f"\n{C_CYAN}  -- server-filedb backend: {backend_name} --{C_RESET}\n")
        # Empty qarg (e.g. the "epoll" default backend) means no extra
        # CONFIG+= flag — the matrix entry is just "build the default
        # backend". Pass an empty list rather than [""] so build_project
        # doesn't end up with a stray empty argv element on the qmake/cmake
        # command line.
        extra_args = [qarg] if qarg else []
        if build_project(SERVER_FILEDB_PRO, backend_build,
                         "server-filedb " + backend_name,
                         extra_qmake_args=extra_args):
            dp_src = DATAPACK_SOURCES[0][0] if DATAPACK_SOURCES else None
            if dp_src is not None and os.path.isdir(dp_src):
                setup_server_datapack(backend_build, dp_src)
                map_main = os.path.join(backend_build, "datapack", "map", "main")
                if os.path.isdir(map_main):
                    mcs = sorted([d for d in os.listdir(map_main)
                                  if os.path.isdir(os.path.join(map_main, d))])
                    if mcs:
                        setup_server_config(backend_build, maincode=mcs[0])
                clear_database_filedb(backend_build)
                remove_cache(backend_build)
                set_http_datapack_mirror(backend_build, "")
                set_map_visibility_minimize(backend_build, "network")
                srv = start_server(backend_build)
                if srv:
                    client_connect(client_build,
                                   "filedb " + backend_name + ": client to map")
                    stop_server()
            else:
                log_fail("filedb " + backend_name, "no datapack source")
        bi += 1

    # ── Phase B: server-cli-epoll (PostgreSQL) ──────────────────────
    # Gated on an opt-in `execute_sql` containing "PostgreSQL" somewhere
    # in remote_nodes.json — see CLAUDE.md "Database backends in
    # testing*.py — file_db only on host, SQL gated to remote_nodes.json".
    # No execute_sql opt-in anywhere → skip the whole phase. The default
    # all.sh run on a clean checkout does not require a running
    # PostgreSQL daemon.
    _need_postgresql = any(
        "PostgreSQL" in (en.get("execute_sql") or [])
        for n in REMOTE_NODES
        for en in (n.get("execution_nodes") or []))
    if not _need_postgresql:
        log_info("Phase B: skipped — no execute_sql=[PostgreSQL] opt-in "
                 "in remote_nodes.json (file-db default)")
    elif build_project(SERVER_CLIEPO_PRO, cliepo_build, "server-cli-epoll"):
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
                # Wipe the PostgreSQL backend AND the file-db side; this
                # whole branch only runs when execute_sql=[PostgreSQL]
                # opt-in is set on some exec_node.
                wipe_via_sql("PostgreSQL")
                # cli-epoll keeps player state on the filesystem too
                # (server/base/Client.cpp's serialise/parse path writes
                # under <build_dir>/database/). When we switch
                # mainDatapackCode the cached map_file_database_ids no
                # longer point at valid entries in the new maincode's
                # dictionary; the server protected itself by kicking
                # the client at startup before this fix landed. Clear
                # the filedb tree alongside postgres so each
                # (datapack, maincode) starts from a truly empty state.
                clear_database_filedb(cliepo_build)
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

    # ── Phase E: Remote server + local client ────────────────────
    if failed_cases is not None:
        log_info("resume mode: skipping Phase E remote server tests")
        stop_server()
        summary()
        return

    print(f"\n{C_CYAN}--- Phase E: Remote server + local client ---{C_RESET}\n")

    ri = 0
    while ri < len(REMOTE_SERVERS):
        (label, host, ssh_port, _use_mold, _extra_defines, _has_gui, _cxx_ver,
         remote_dir) = REMOTE_SERVERS[ri]
        if not node_runs(label, "server-client"):
            log_info(f"skipping remote {label}: not opted into 'server-client'")
            ri += 1
            continue
        node = get_node(label)
        if node is not None and not diagnostic.node_supports(node, DIAG):
            log_info(f"skipping remote {label}: no "
                     f"{diagnostic.compiler_name(DIAG)} for diagnostic mode")
            ri += 1
            continue
        server_addr = get_remote_server_address(host)
        print(f"\n{C_CYAN}  -- Remote server: {label} ({server_addr}) --{C_RESET}\n")

        remote_build_root = f"{remote_dir}/build-remote{_DIAG_SUFFIX}"
        # Server targets now iterate cxx_versions like every other target,
        # so the filedb build dir carries a -<cxx> suffix when the node
        # specified one. Pick the last cxx version configured (the most
        # modern standard the node opts into) to match the binary that
        # was just built.
        _cxx_suffix = f"-{_cxx_ver[-1]}" if _cxx_ver else ""
        remote_filedb_build = f"{remote_build_root}/catchchallenger-server-filedb{_cxx_suffix}"
        remote_filedb_bin = f"{remote_filedb_build}/catchchallenger-server-cli-epoll"

        try:
            # setup datapack on remote
            dp_src = DATAPACK_SOURCES[0][0] if DATAPACK_SOURCES else None
            if dp_src is None:
                log_fail(f"remote {label}", "no datapack source")
                ri += 1
                continue
            if not setup_remote_server_runtime(host, ssh_port, remote_filedb_build, dp_src):
                log_fail(f"remote {label} setup", "failed to rsync datapack")
                ri += 1
                continue
            log_pass(f"remote {label} setup", "datapack synced")

            # start server on remote
            node_ready_timeout = node.get("server_ready_timeout") if node else None
            ssh_proc, actual_port = start_remote_server(host, ssh_port,
                                                        remote_filedb_bin,
                                                        remote_filedb_build,
                                                        diag=DIAG,
                                                        ready_timeout=node_ready_timeout)
            if actual_port == 0:
                if ssh_proc == "BINARY_MISSING":
                    log_fail(f"remote {label} server start",
                             f"binary missing at {remote_filedb_bin}")
                else:
                    log_fail(f"remote {label} server start",
                             "timeout waiting for 'correctly bind:'")
                ri += 1
                continue
            log_pass(f"remote {label} server start", f"port {actual_port}")

            # connect local client to remote server
            ok, out = run_client(client_build, CLIENT_CPU_BIN,
                                 ["--host", server_addr, "--port", str(actual_port),
                                  "--autologin", "--character", "Player",
                                  "--closewhenonmap"],
                                 f"local client -> remote {label}",
                                 success_marker="MapVisualiserPlayer::mapDisplayedSlot()")

            stop_remote_server(ssh_proc, host, ssh_port)
        finally:
            cleanup_remote_node(host, ssh_port, remote_dir)
        ri += 1

    stop_server()
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
    save_failed_cases()
    if failed:
        sys.exit(1)


if __name__ == "__main__":
    main()
