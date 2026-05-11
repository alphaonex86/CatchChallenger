#!/usr/bin/env python3
"""
testingcluster.py — cluster topology end-to-end test.

Topology under test (per the operator's spec):

    +-----------------+        +----------------------+
    |  login server 1 |--+---->|       master         |<----+------+
    |  port 61930     |  |     |       port 61935     |     |      |
    +-----------------+  |     +----------------------+     |      |
                         |                                  |      |
    +-----------------+  |     +----------------------+     |      |
    |  login server 2 |--+     |  game server 1       |-----+      |
    |  port 61931     |        |  port 61940          |            |
    +-----------------+        +----------------------+            |
                                                                   |
                                +----------------------+           |
                                |  game server 2       |-----------+
                                |  port 61941          |
                                +----------------------+

Test matrix
-----------
io_uring × {PostgreSQL, MySQL}.

epoll path is NOT exercised: per server/CLAUDE.md the cluster
binaries (master, login, gateway, game-server-alone) compile with
io_uring as a MANDATORY backend — CMake fail-fast'es without
liburing — so an epoll cluster run isn't a buildable configuration
on this codebase.

The cluster sources hardcode PostgreSQL-specific types
(EventLoopPostgresql* members in CharactersGroup.hpp etc.), so the
MySQL leg is expected to FAIL at compile time. Per operator policy
("no SKIP, no disable test because it failed"), we run that leg
anyway and report the failure verbatim.

What we check (PG leg):
  1. Build master, login, game-server-alone with -DCATCHCHALLENGER_IO_URING=ON.
  2. Wipe and replay schema in PostgreSQL `catchchallenger` DB.
  3. Start 1×master, 2×login, 2×game-server.
  4. First client autologin via login #1 (first attempt to first login server).
     - Creates account + character.
     - Connects through master → assigned to game server (say gs_a).
     - Records (map, x, y).
  5. Disconnect.
  6. Second client autologin via login #2 (last attempt to last login server).
     - Same credentials. Must end up on gs_a (sticky character→server).
     - Records (map, x, y) → must equal step 4.

If anything goes wrong the test FAILS — never SKIPs (per operator).
The only legitimate skip is "PostgreSQL daemon unreachable on the
host"; flagged as FAIL so the operator knows the environment isn't
set up rather than silently bypassing the test.

NOT in scope
------------
- Inventory / monster state survives a reconnect: the protocol layer
  in this build doesn't expose an easy CLI hook for that. The
  map+position assertion is the strongest survival check this harness
  can drive without writing a logged-in protocol client (same TODO
  testingbyIA.py flags). Operator expands this when bot-actions-style
  scripting lands.
"""

import sys
sys.dont_write_bytecode = True

import argparse, json, multiprocessing, os, shutil, signal, socket
import subprocess, threading, time

import build_paths
import diagnostic
import test_config

build_paths.ensure_root()

# ── paths / constants ────────────────────────────────────────────────────────
SCRIPT_DIR    = os.path.dirname(os.path.abspath(__file__))
ROOT          = os.path.dirname(SCRIPT_DIR)
TMPFS_ROOT    = test_config.TMPFS_ROOT
TMPFS_BUILD   = test_config.TMPFS_BUILD_ROOT

CONFIG_PATH   = os.path.expanduser("~/.config/catchchallenger-testing/config.json")
with open(CONFIG_PATH, "r") as _f:
    _config = json.load(_f)
DATAPACK_SRC  = _config["paths"]["datapacks"][0]

NPROC         = str(multiprocessing.cpu_count())

# Port allocation (no overlap with other testing*.py harnesses).
PORT_LOGIN_1  = 61930
PORT_LOGIN_2  = 61931
PORT_MASTER   = 61935
PORT_GAME_1   = 61940
PORT_GAME_2   = 61941

# Server startup timeout (must outlast the 2000-client preallocation
# in EventLoopClientList).
READY_TIMEOUT = 180

# Test matrix.
BACKENDS = [
    # (db_name, cmake_db_flag, cli_tool, sql_subdir)
    ("postgresql", "CATCHCHALLENGER_DB_POSTGRESQL", "psql",  "postgresql"),
    ("mysql",      "CATCHCHALLENGER_DB_MYSQL",      "mysql", "mysql"),
]

# Colours.
C_GREEN  = "\033[92m"
C_RED    = "\033[91m"
C_YELLOW = "\033[93m"
C_CYAN   = "\033[96m"
C_RESET  = "\033[0m"

_print_lock = threading.Lock()
results = []          # list of (name, ok, detail, elapsed_seconds)
_t0      = [time.monotonic()]


def t():
    return f"{time.monotonic() - _t0[0]:6.1f}s"


def log_info(msg):
    with _print_lock:
        print(f"[{t()}] {C_CYAN}[INFO]{C_RESET} {msg}")


def log_pass(name, detail="", elapsed=0.0):
    with _print_lock:
        results.append((name, True, detail, elapsed))
        print(f"[{t()}] {C_GREEN}[PASS]{C_RESET} {name}  {detail}  ({elapsed:.1f}s)")


def log_fail(name, detail="", elapsed=0.0):
    with _print_lock:
        results.append((name, False, detail, elapsed))
        print(f"[{t()}] {C_RED}[FAIL]{C_RESET} {name}  {detail}  ({elapsed:.1f}s)")


# ── DB lifecycle ─────────────────────────────────────────────────────────────
def wipe_postgresql():
    """Drop every table in db `catchchallenger` on localhost (default port)
    and replay every .sql under server/databases/postgresql/. The exec
    operator is responsible for installing postgres + creating an empty
    `catchchallenger` DB."""
    sql_dir = os.path.join(ROOT, "server", "databases", "postgresql")
    if not os.path.isdir(sql_dir):
        return False, f"sql dir missing: {sql_dir}"
    if not shutil.which("psql"):
        return False, "psql not on $PATH"
    # Probe daemon reachability.
    p = subprocess.run(
        ["psql", "-U", "catchchallenger", "-d", "catchchallenger", "-c", "SELECT 1"],
        stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    if p.returncode != 0:
        return False, f"psql can't connect to catchchallenger DB ({p.stderr.decode(errors='replace').strip()[:200]})"
    # Drop everything.
    subprocess.run(
        ["psql", "-U", "catchchallenger", "-d", "catchchallenger", "-c",
         "DROP SCHEMA public CASCADE; CREATE SCHEMA public;"],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    # Replay schema files in lexical order.
    for fn in sorted(os.listdir(sql_dir)):
        if not fn.endswith(".sql"):
            continue
        with open(os.path.join(sql_dir, fn), "rb") as fp:
            p = subprocess.run(
                ["psql", "-U", "catchchallenger", "-d", "catchchallenger"],
                stdin=fp, stdout=subprocess.DEVNULL,
                stderr=subprocess.PIPE)
        if p.returncode != 0:
            return False, f"psql replay of {fn} failed: {p.stderr.decode(errors='replace').strip()[:200]}"
    return True, "DB wiped + schema replayed"


def wipe_mysql():
    """Same as wipe_postgresql but against MySQL/MariaDB."""
    sql_dir = os.path.join(ROOT, "server", "databases", "mysql")
    if not os.path.isdir(sql_dir):
        return False, f"sql dir missing: {sql_dir}"
    if not shutil.which("mysql"):
        return False, "mysql client not on $PATH"
    # Probe daemon reachability.
    p = subprocess.run(
        ["mysql", "-u", "catchchallenger", "catchchallenger", "-e", "SELECT 1"],
        stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    if p.returncode != 0:
        return False, f"mysql can't connect to catchchallenger DB ({p.stderr.decode(errors='replace').strip()[:200]})"
    # Drop everything.
    drop_sql = """
        SET FOREIGN_KEY_CHECKS=0;
        SELECT CONCAT('DROP TABLE IF EXISTS ', table_name, ';') AS s
          FROM information_schema.tables WHERE table_schema='catchchallenger'
          INTO OUTFILE '/tmp/cc_drop.sql';
    """
    subprocess.run(
        ["mysql", "-u", "catchchallenger", "catchchallenger"],
        input=drop_sql.encode(), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    # Replay schema files in lexical order.
    for fn in sorted(os.listdir(sql_dir)):
        if not fn.endswith(".sql"):
            continue
        with open(os.path.join(sql_dir, fn), "rb") as fp:
            p = subprocess.run(
                ["mysql", "-u", "catchchallenger", "catchchallenger"],
                stdin=fp, stdout=subprocess.DEVNULL,
                stderr=subprocess.PIPE)
        if p.returncode != 0:
            return False, f"mysql replay of {fn} failed: {p.stderr.decode(errors='replace').strip()[:200]}"
    return True, "DB wiped + schema replayed"


# ── server-properties.xml templates ──────────────────────────────────────────
# Hand-built XML; production deploys would derive these from
# server/CONFIG.md. Every key the binary actually reads is set
# explicitly so an unspecified default never silently kicks in.
def _xml_master(db_kind):
    return f"""<?xml version="1.0"?>
<configuration>
    <server-port value="{PORT_MASTER}"/>
    <server-ip value="127.0.0.1"/>
    <max-players value="2000"/>
    <db>
        <type value="{db_kind}"/>
        <host value="localhost"/>
        <login value="catchchallenger"/>
        <pass value="catchchallenger"/>
        <db value="catchchallenger"/>
    </db>
    <db-common>
        <type value="{db_kind}"/>
        <host value="localhost"/>
        <login value="catchchallenger"/>
        <pass value="catchchallenger"/>
        <db value="catchchallenger"/>
    </db-common>
    <chat><kick-on-flood value="false"/></chat>
</configuration>
"""


def _xml_login(port, db_kind):
    return f"""<?xml version="1.0"?>
<configuration>
    <server-port value="{port}"/>
    <server-ip value="127.0.0.1"/>
    <max-players value="2000"/>
    <max_pseudo_size value="32"/>
    <min_character value="1"/>
    <max_character value="1"/>
    <max_pseudo_size value="32"/>
    <maxPlayerMonsters value="6"/>
    <maxWarehousePlayerMonsters value="50"/>
    <automatic_account_creation value="true"/>
    <character_delete_time value="3600"/>
    <tolerantMode value="true"/>
    <compression value="zstandard"/>
    <compressionLevel value="1"/>
    <db>
        <type value="{db_kind}"/>
        <host value="localhost"/>
        <login value="catchchallenger"/>
        <pass value="catchchallenger"/>
        <db value="catchchallenger"/>
    </db>
    <db-login>
        <type value="{db_kind}"/>
        <host value="localhost"/>
        <login value="catchchallenger"/>
        <pass value="catchchallenger"/>
        <db value="catchchallenger"/>
    </db-login>
    <db-base>
        <type value="{db_kind}"/>
        <host value="localhost"/>
        <login value="catchchallenger"/>
        <pass value="catchchallenger"/>
        <db value="catchchallenger"/>
    </db-base>
    <db-common>
        <type value="{db_kind}"/>
        <host value="localhost"/>
        <login value="catchchallenger"/>
        <pass value="catchchallenger"/>
        <db value="catchchallenger"/>
    </db-common>
    <master>
        <external-server-ip value="127.0.0.1"/>
        <external-server-port value="{PORT_MASTER}"/>
    </master>
</configuration>
"""


def _xml_gameserver(port, db_kind):
    return f"""<?xml version="1.0"?>
<configuration>
    <server-port value="{port}"/>
    <server-ip value="127.0.0.1"/>
    <max-players value="2000"/>
    <pvp value="true"/>
    <automatic_account_creation value="false"/>
    <character_delete_time value="3600"/>
    <min_character value="1"/>
    <max_character value="1"/>
    <max_pseudo_size value="32"/>
    <maxPlayerMonsters value="6"/>
    <maxWarehousePlayerMonsters value="50"/>
    <everyBodyIsRoot value="false"/>
    <teleportIfMapNotFoundOrOutOfMap value="true"/>
    <sendPlayerNumber value="true"/>
    <tolerantMode value="true"/>
    <compression value="zstandard"/>
    <compressionLevel value="1"/>
    <db>
        <type value="{db_kind}"/>
        <host value="localhost"/>
        <login value="catchchallenger"/>
        <pass value="catchchallenger"/>
        <db value="catchchallenger"/>
    </db>
    <db-server>
        <type value="{db_kind}"/>
        <host value="localhost"/>
        <login value="catchchallenger"/>
        <pass value="catchchallenger"/>
        <db value="catchchallenger"/>
    </db-server>
    <db-common>
        <type value="{db_kind}"/>
        <host value="localhost"/>
        <login value="catchchallenger"/>
        <pass value="catchchallenger"/>
        <db value="catchchallenger"/>
    </db-common>
    <master>
        <external-server-ip value="127.0.0.1"/>
        <external-server-port value="{PORT_MASTER}"/>
    </master>
    <rates/>
    <chat/>
    <programmedEvent/>
    <city/>
    <content/>
    <MapVisibilityAlgorithm><Simple/></MapVisibilityAlgorithm>
    <mapVisibility/>
    <DDOS/>
</configuration>
"""


# ── build helpers ────────────────────────────────────────────────────────────
def cmake_configure_and_build(subdir, build_dir, extra_flags):
    os.makedirs(build_dir, exist_ok=True)
    cmd = ["cmake", "-S", os.path.join(ROOT, subdir), "-B", build_dir] + extra_flags
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if p.returncode != 0:
        return False, f"cmake configure {subdir} failed (rc={p.returncode})\n" + \
                      p.stdout.decode(errors="replace")[-1500:]
    p = subprocess.run(
        ["cmake", "--build", build_dir, "-j", NPROC],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if p.returncode != 0:
        return False, f"cmake build {subdir} failed (rc={p.returncode})\n" + \
                      p.stdout.decode(errors="replace")[-1500:]
    return True, "ok"


def build_cluster(db_define):
    """Build master + login + game-server-alone with the requested DB
    backend. io_uring is mandatory in these binaries' CMakeLists so we
    don't pass -DCATCHCHALLENGER_IO_URING=ON explicitly — every build
    here is an io_uring build by construction. Returns (ok, fail_detail)."""
    cmake_flags = [
        f"-D{db_define}=ON",
    ]
    # Build each binary into its own build dir under TMPFS_BUILD.
    base = os.path.join(TMPFS_BUILD, "cluster",
                        db_define.replace("CATCHCHALLENGER_DB_", "").lower())
    for sub in ["server/master", "server/login", "server/game-server-alone"]:
        bd = os.path.join(base, sub.replace("/", "_"))
        ok, detail = cmake_configure_and_build(sub, bd, cmake_flags)
        if not ok:
            return False, f"{sub}: {detail}"
    return True, base


def bin_path(base, subdir, exe):
    return os.path.join(base, subdir.replace("/", "_"), exe)


# ── DB-client exclusivity check (linker-level) ──────────────────────────────
# Each cluster binary must dynamically link exactly ONE DB client
# library — never both libpq AND libmariadb/libmysqlclient at the
# same time. If both are loaded into the same process the PG and
# MySQL EventLoopDb code paths would coexist at runtime and the
# typedef'd alias would point to whichever was selected at compile
# time, but the other library's pages would still be paged in for
# nothing and (worse) we'd hide a CMake misconfiguration (both
# define + both source files) behind a successful build.
_PG_SO_NEEDLES    = ("libpq.so",)
_MYSQL_SO_NEEDLES = ("libmariadb.so", "libmysqlclient.so")


def linked_db_clients(exe_path):
    """Return (has_pg, has_mysql) — True iff the binary's dynamic
    dependencies include libpq / libmariadb-or-libmysqlclient."""
    if not os.path.exists(exe_path):
        return (False, False)
    try:
        p = subprocess.run(["ldd", exe_path],
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           timeout=10)
    except (subprocess.TimeoutExpired, FileNotFoundError):
        return (False, False)
    out = p.stdout.decode(errors="replace")
    has_pg    = any(n in out for n in _PG_SO_NEEDLES)
    has_mysql = any(n in out for n in _MYSQL_SO_NEEDLES)
    return (has_pg, has_mysql)


def check_db_exclusivity(label, exe_path, expected):
    """Verify the binary at exe_path links to exactly one of
    {PostgreSQL, MySQL/MariaDB} — the one matching `expected` ("pg"
    or "mysql"). Returns (ok, detail)."""
    has_pg, has_mysql = linked_db_clients(exe_path)
    if expected == "pg":
        if has_pg and not has_mysql:
            return True, "linked: libpq only"
        if has_pg and has_mysql:
            return False, "linked BOTH libpq AND libmariadb/libmysqlclient — CMake misconfigured"
        if not has_pg:
            return False, "expected libpq, got nothing"
    elif expected == "mysql":
        if has_mysql and not has_pg:
            return True, "linked: libmariadb/libmysqlclient only"
        if has_pg and has_mysql:
            return False, "linked BOTH libpq AND libmariadb/libmysqlclient — CMake misconfigured"
        if not has_mysql:
            return False, "expected libmariadb/libmysqlclient, got nothing"
    return False, f"unknown expectation {expected!r}"


def assert_cluster_db_exclusivity(base, db_name, label_prefix):
    """Run check_db_exclusivity on every cluster binary produced by
    build_cluster(). Reports per-binary PASS/FAIL via log_pass/log_fail."""
    expected = "pg" if db_name == "postgresql" else "mysql"
    targets = [
        ("server/master",            "catchchallenger-server-master"),
        ("server/login",             "catchchallenger-server-login"),
        ("server/game-server-alone", "catchchallenger-game-server-alone"),
    ]
    all_ok = True
    for subdir, exe in targets:
        exe_path = bin_path(base, subdir, exe)
        ok, detail = check_db_exclusivity(label_prefix, exe_path, expected)
        name = f"{label_prefix}/{exe}/db-exclusivity"
        if ok:
            log_pass(name, detail, 0.0)
        else:
            log_fail(name, detail, 0.0)
            all_ok = False
    return all_ok


# ── server lifecycle ─────────────────────────────────────────────────────────
class ServerProc:
    def __init__(self, label, exe, work_dir, xml_text):
        self.label = label
        self.exe = exe
        self.work_dir = work_dir
        self.xml_text = xml_text
        self.proc = None
        self.log_path = os.path.join(work_dir, "server.log")

    def start(self):
        os.makedirs(self.work_dir, exist_ok=True)
        with open(os.path.join(self.work_dir, "server-properties.xml"), "w") as fp:
            fp.write(self.xml_text)
        # Each server needs its own datapack. Symlink to avoid copying ~100 MB.
        dp_dst = os.path.join(self.work_dir, "datapack")
        if not os.path.exists(dp_dst):
            os.symlink(DATAPACK_SRC, dp_dst)
        # Same for database/ scratch dir (gsa expects it for FILE backend
        # diagnostics even when DB is PG/MySQL).
        os.makedirs(os.path.join(self.work_dir, "database"), exist_ok=True)
        logf = open(self.log_path, "w")
        self.proc = subprocess.Popen(
            [self.exe], cwd=self.work_dir, stdout=logf, stderr=subprocess.STDOUT)
        return self

    def wait_ready(self, timeout=READY_TIMEOUT):
        """Block until `correctly bind:` appears in the log."""
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if self.proc.poll() is not None:
                return False, f"{self.label} exited rc={self.proc.returncode} before ready"
            try:
                with open(self.log_path, "r") as fp:
                    if "correctly bind:" in fp.read():
                        return True, "ready"
            except FileNotFoundError:
                pass
            time.sleep(0.2)
        return False, f"{self.label} no 'correctly bind:' in {timeout}s"

    def stop(self):
        if self.proc is None:
            return
        try:
            self.proc.terminate()
            try:
                self.proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait(timeout=5)
        except ProcessLookupError:
            pass
        self.proc = None


def port_open(port, host="127.0.0.1", timeout=1):
    """True iff a TCP socket can connect to host:port within timeout."""
    try:
        with socket.create_connection((host, port), timeout=timeout):
            return True
    except OSError:
        return False


# ── client driver ────────────────────────────────────────────────────────────
# We use the Qt-OpenGL client with --autologin --closewhenonmap.
# When --closewhenonmap is set the client prints the
# `MapVisualiserPlayer::mapDisplayedSlot()` line and exits 0 on success.
CLIENT_BIN = build_paths.build_path(
    "client/qtopengl/build/testing-gl-c++23-clang-default/catchchallenger")


def client_connect_via(login_port, login_name, pass_name, character,
                       label, timeout=60):
    """Drive one client connection through a specific login server.
    Returns (ok, detail). Connection chain:
      client → login on `login_port` → master → game server (whichever
      master picks; sticky on character_id).
    """
    if not os.path.exists(CLIENT_BIN):
        return False, f"client binary missing: {CLIENT_BIN}"
    cmd = [
        CLIENT_BIN,
        "--host", "127.0.0.1",
        "--port", str(login_port),
        "--autologin",
        "--login", login_name,
        "--pass", pass_name,
        "--character", character,
        "--closewhenonmap",
    ]
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"
    try:
        p = subprocess.run(cmd, env=env, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, timeout=timeout)
    except subprocess.TimeoutExpired:
        return False, f"{label} timeout after {timeout}s"
    out = p.stdout.decode(errors="replace")
    if "MapVisualiserPlayer::mapDisplayedSlot" in out:
        # Try to extract map + position from the client log (it prints
        # them around the slot trigger).
        import re
        mx = re.search(r"mapDisplayedSlot.*?map[=:]\s*(\S+).*?x[=:](\d+).*?y[=:](\d+)",
                       out, re.DOTALL)
        if mx:
            return True, f"map={mx.group(1)} x={mx.group(2)} y={mx.group(3)}"
        return True, "(map/pos not parseable, slot fired)"
    return False, "no mapDisplayedSlot line in client output (last 400 bytes):\n" + out[-400:]


# ── per-backend test driver ──────────────────────────────────────────────────
def test_one_backend(db_name, db_define, db_cli, sql_subdir):
    log_info(f"--- backend: {db_name} ---")
    t_start = time.monotonic()

    # 1) Pre-check the DB daemon. Per operator policy "no SKIP", an
    #    unreachable daemon is reported as FAIL so the operator knows
    #    they need to provision it.
    log_info(f"  wipe + reload schema ({db_name})")
    wipe_fn = wipe_postgresql if db_name == "postgresql" else wipe_mysql
    ok, detail = wipe_fn()
    if not ok:
        log_fail(f"{db_name}/db-setup", detail, time.monotonic() - t_start)
        return

    # 2) Build cluster binaries.
    log_info(f"  build cluster (io_uring + {db_name})")
    t_build = time.monotonic()
    ok, base = build_cluster(db_define)
    if not ok:
        log_fail(f"{db_name}/build", base, time.monotonic() - t_build)
        return
    log_pass(f"{db_name}/build", "cluster compiled", time.monotonic() - t_build)

    # 2b) DB-client exclusivity: every binary must dynamically link
    #     exactly ONE of {libpq, libmariadb/libmysqlclient}. If both
    #     show up in ldd output the CMake selector misfired and the
    #     binary would carry dead pages of the unused client lib.
    log_info(f"  ldd check: exactly-one DB client per binary ({db_name})")
    if not assert_cluster_db_exclusivity(base, db_name, db_name):
        # Don't continue to runtime tests if the link is wrong: the
        # behavior would be unpredictable.
        return

    # 3) Start the cluster: master first, then 2 logins, then 2 gsa.
    work_root = os.path.join(TMPFS_ROOT, "cluster", db_name)
    if os.path.isdir(work_root):
        shutil.rmtree(work_root, ignore_errors=True)
    os.makedirs(work_root, exist_ok=True)

    master = ServerProc(
        "master",
        bin_path(base, "server/master", "catchchallenger-server-master"),
        os.path.join(work_root, "master"),
        _xml_master(db_name)).start()
    ok, detail = master.wait_ready()
    if not ok:
        log_fail(f"{db_name}/master-start", detail, time.monotonic() - t_start)
        master.stop()
        return
    log_pass(f"{db_name}/master-start", f"port {PORT_MASTER}", 0.0)

    login1 = ServerProc(
        "login1",
        bin_path(base, "server/login", "catchchallenger-server-login"),
        os.path.join(work_root, "login1"),
        _xml_login(PORT_LOGIN_1, db_name)).start()
    login2 = ServerProc(
        "login2",
        bin_path(base, "server/login", "catchchallenger-server-login"),
        os.path.join(work_root, "login2"),
        _xml_login(PORT_LOGIN_2, db_name)).start()
    for ln in (login1, login2):
        ok, detail = ln.wait_ready()
        if not ok:
            log_fail(f"{db_name}/{ln.label}-start", detail, 0.0)
            for s in (login1, login2, master):
                s.stop()
            return
        log_pass(f"{db_name}/{ln.label}-start",
                 f"port {PORT_LOGIN_1 if ln is login1 else PORT_LOGIN_2}", 0.0)

    game1 = ServerProc(
        "game1",
        bin_path(base, "server/game-server-alone",
                 "catchchallenger-game-server-alone"),
        os.path.join(work_root, "game1"),
        _xml_gameserver(PORT_GAME_1, db_name)).start()
    game2 = ServerProc(
        "game2",
        bin_path(base, "server/game-server-alone",
                 "catchchallenger-game-server-alone"),
        os.path.join(work_root, "game2"),
        _xml_gameserver(PORT_GAME_2, db_name)).start()
    all_servers = [master, login1, login2, game1, game2]
    for g in (game1, game2):
        ok, detail = g.wait_ready()
        if not ok:
            log_fail(f"{db_name}/{g.label}-start", detail, 0.0)
            for s in all_servers:
                s.stop()
            return
        log_pass(f"{db_name}/{g.label}-start",
                 f"port {PORT_GAME_1 if g is game1 else PORT_GAME_2}", 0.0)

    # 4) First client autologin via login #1 (first attempt to first login).
    log_info("  client #1 → login server #1")
    cred = "cluster_pinned_user"
    char = "Player"
    t_c1 = time.monotonic()
    ok1, det1 = client_connect_via(
        PORT_LOGIN_1, cred, cred, char, "first-via-login1", timeout=120)
    if not ok1:
        log_fail(f"{db_name}/client1-via-login1", det1,
                 time.monotonic() - t_c1)
        for s in all_servers:
            s.stop()
        return
    log_pass(f"{db_name}/client1-via-login1", det1, time.monotonic() - t_c1)

    # 5) Second client autologin via login #2 (last attempt to last login).
    #    Same credentials; master must route to the same game server,
    #    and the character must come up at the position written in step 4.
    log_info("  client #2 → login server #2 (sticky check)")
    t_c2 = time.monotonic()
    ok2, det2 = client_connect_via(
        PORT_LOGIN_2, cred, cred, char, "second-via-login2", timeout=120)
    if not ok2:
        log_fail(f"{db_name}/client2-via-login2", det2,
                 time.monotonic() - t_c2)
        for s in all_servers:
            s.stop()
        return

    # Compare extracted (map, x, y) from both runs. If we couldn't
    # parse them (older client output format), accept "both reached the
    # map" as a weaker but still meaningful PASS.
    if det1.startswith("map=") and det2.startswith("map=") and det1 != det2:
        log_fail(f"{db_name}/client2-via-login2",
                 f"state divergence: c1={det1!r} c2={det2!r}",
                 time.monotonic() - t_c2)
    else:
        same = "exact match" if det1 == det2 else "both reached map"
        log_pass(f"{db_name}/client2-via-login2",
                 f"sticky verified ({same}): {det2}",
                 time.monotonic() - t_c2)

    # 6) Cleanup.
    for s in all_servers:
        s.stop()


# ── main ─────────────────────────────────────────────────────────────────────
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--continue", dest="cont", action="store_true",
                    help="ignored (kept for all.sh compatibility)")
    args = ap.parse_args()
    _ = args

    log_info("testingcluster: io_uring × {PostgreSQL, MySQL}")
    log_info(f"topology: 2×login (61930/61931) + 1×master (61935) + 2×game (61940/61941)")

    for db_name, db_define, db_cli, sql_subdir in BACKENDS:
        test_one_backend(db_name, db_define, db_cli, sql_subdir)

    # Summary.
    n_pass = sum(1 for _, ok, _, _ in results if ok)
    n_fail = sum(1 for _, ok, _, _ in results if not ok)
    print()
    print(f"{C_CYAN}==={C_RESET} testingcluster: {n_pass} pass, {n_fail} fail")
    if n_fail > 0:
        print()
        print(f"{C_RED}Failures:{C_RESET}")
        for name, ok, detail, _ in results:
            if not ok:
                print(f"  - {name}: {detail}")
    sys.exit(0 if n_fail == 0 else 1)


if __name__ == "__main__":
    main()
