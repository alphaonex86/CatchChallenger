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
BACKENDS_FULL = [
    # (db_name, cmake_db_flag, cli_tool, sql_subdir)
    ("postgresql", "CATCHCHALLENGER_DB_POSTGRESQL", "psql",  "postgresql"),
    ("mysql",      "CATCHCHALLENGER_DB_MYSQL",      "mysql", "mysql"),
]
# Allow restricting the backend matrix at the command line (defaults to
# the full list). Useful when iterating on a single backend during
# debugging — e.g. `python3 testingcluster.py --backends=postgresql`.
BACKENDS = BACKENDS_FULL

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


# ── ephemeral DB instances ───────────────────────────────────────────────────
# Each test run spins up its OWN PostgreSQL + MariaDB daemon on a
# non-default port under /mnt/data/perso/tmpfs/cluster/db/<backend>.
# No system services touched, no sudo, no preexisting role/db needed.
PG_PORT       = 5433
MARIADB_PORT  = 3307

# Shared master-auth token used by master <-> login/gateway/gsa. 32
# bytes = 64 hex chars, baked at test-startup so master.xml and the
# login/gateway/gsa xmls all carry the same value. Without this they
# each generate their own token and master rejects the auth with
# "reply to 08 return code wrong too small (abort)".
import secrets
MASTER_AUTH_TOKEN = secrets.token_hex(32)
# Per-backend data dirs requested by the operator:
#   /mnt/data/perso/tmpfs/cluster/postgresql/    — postgres data + sock
#   /mnt/data/perso/tmpfs/cluster/mysql/         — mariadb data + sock
# The cluster server work dirs live OUTSIDE these (see CLUSTER_RUN_ROOT)
# so wiping the DB at start/end doesn't take server logs with it.
DB_PG_DIR        = os.path.join(TMPFS_ROOT, "cluster", "postgresql")
DB_MARIADB_DIR   = os.path.join(TMPFS_ROOT, "cluster", "mysql")
CLUSTER_RUN_ROOT = os.path.join(TMPFS_ROOT, "cluster", "runtime")

# Ephemeral nginx for the gsa datapack mirror. gsa is built with
# CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR — it refuses to boot
# without an httpDatapackMirrorBase, and the connected client
# fetches `datapack-list/base.txt` + `pack/datapack*.tar.zst` over
# HTTP. We stand up a tiny nginx on a non-privileged port for the
# duration of the test run, populate /pack/ via datapack-archive.sh
# and /datapack-list/ via datapack-list.php, then tear it down.
#
# The base datapack contents are served direct from the source
# checkout via an nginx alias — no copy/symlink dance.
NGINX_PORT       = 18231
NGINX_ROOT       = os.path.join(TMPFS_ROOT, "cluster", "www")
NGINX_PACK_DIR   = os.path.join(NGINX_ROOT, "pack")
NGINX_LIST_DIR   = os.path.join(NGINX_ROOT, "datapack-list")
NGINX_CONF       = os.path.join(NGINX_ROOT, "nginx.conf")
NGINX_ERR_LOG    = os.path.join(NGINX_ROOT, "nginx-error.log")
NGINX_ACC_LOG    = os.path.join(NGINX_ROOT, "nginx-access.log")
NGINX_PID        = os.path.join(NGINX_ROOT, "nginx.pid")
# Scripts shipped with the production deploy
# (/home/user/Desktop/www/catchchallenger.first-world.info/) — we
# don't redistribute them; we invoke them in place. If either is
# missing the test FAILs early with a clear message.
WWW_DEPLOY_DIR   = "/home/user/Desktop/www/catchchallenger.first-world.info"
DATAPACK_ARCHIVE_SH = os.path.join(WWW_DEPLOY_DIR, "datapack-archive.sh")
DATAPACK_LIST_PHP   = os.path.join(WWW_DEPLOY_DIR, "datapack-list.php")


def _probe_port(port, timeout=20.0):
    """Wait until something accepts on 127.0.0.1:<port>, up to `timeout`s.
    Returns True if the port came up, False on timeout."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.5):
                return True
        except OSError:
            time.sleep(0.2)
    return False


# Keep handles so we can stop them in the global teardown.
_pg_proc = None
_mariadb_proc = None


def start_postgresql():
    """initdb + pg_ctl start on PG_PORT. Creates role + database
    `catchchallenger`/`catchchallenger`. Returns (ok, detail)."""
    global _pg_proc
    if not shutil.which("initdb"):
        return False, "initdb not on $PATH (postgresql server package missing)"
    # Wipe the entire DB dir up front per operator policy.
    if os.path.isdir(DB_PG_DIR):
        shutil.rmtree(DB_PG_DIR, ignore_errors=True)
    data_dir = os.path.join(DB_PG_DIR, "data")
    sock_dir = os.path.join(DB_PG_DIR, "sock")
    log_path = os.path.join(DB_PG_DIR, "postgres.log")
    os.makedirs(data_dir, exist_ok=True)
    os.makedirs(sock_dir, exist_ok=True)
    p = subprocess.run(
        ["initdb", "-D", data_dir, "-U", "catchchallenger",
         "--auth-host=trust", "--auth-local=trust", "-E", "UTF8"],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if p.returncode != 0:
        return False, "initdb failed:\n" + p.stdout.decode(errors='replace')[-800:]
    # Start postgres in the foreground (we own the process).
    logf = open(log_path, "w")
    _pg_proc = subprocess.Popen(
        ["postgres", "-D", data_dir, "-p", str(PG_PORT),
         "-k", sock_dir, "-h", "127.0.0.1"],
        stdout=logf, stderr=subprocess.STDOUT)
    if not _probe_port(PG_PORT, timeout=20):
        return False, f"postgres didn't open port {PG_PORT} (log: {log_path})"
    # createdb (the role 'catchchallenger' is already a superuser via -U on initdb).
    p = subprocess.run(
        ["createdb", "-h", "127.0.0.1", "-p", str(PG_PORT),
         "-U", "catchchallenger", "catchchallenger"],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if p.returncode != 0:
        return False, "createdb failed:\n" + p.stdout.decode(errors='replace')[-800:]
    return True, f"ephemeral postgres up on :{PG_PORT}"


def stop_postgresql():
    """Stop the ephemeral PG daemon and wipe the data dir
    (DB_PG_DIR). Per operator: clean at start AND end."""
    global _pg_proc
    if _pg_proc is not None:
        try:
            _pg_proc.terminate()
            try:
                _pg_proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                _pg_proc.kill()
                _pg_proc.wait(timeout=5)
        except ProcessLookupError:
            pass
        _pg_proc = None
    if os.path.isdir(DB_PG_DIR):
        shutil.rmtree(DB_PG_DIR, ignore_errors=True)


def start_mariadb():
    """mysql_install_db + mariadbd on MARIADB_PORT. Creates user/db
    `catchchallenger`/`catchchallenger`. Returns (ok, detail)."""
    global _mariadb_proc
    if not shutil.which("mariadbd"):
        return False, "mariadbd not on $PATH"
    # Wipe the entire DB dir up front per operator policy.
    if os.path.isdir(DB_MARIADB_DIR):
        shutil.rmtree(DB_MARIADB_DIR, ignore_errors=True)
    data_dir = os.path.join(DB_MARIADB_DIR, "data")
    sock_path = os.path.join(DB_MARIADB_DIR, "mariadb.sock")
    log_path = os.path.join(DB_MARIADB_DIR, "mariadb.log")
    os.makedirs(data_dir, exist_ok=True)
    # mysql_install_db / mariadb-install-db bootstraps the data dir.
    installer = (shutil.which("mariadb-install-db") or
                 shutil.which("mysql_install_db"))
    if installer is None:
        return False, "no mariadb-install-db / mysql_install_db on $PATH"
    p = subprocess.run(
        [installer, f"--datadir={data_dir}", "--auth-root-authentication-method=normal"],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if p.returncode != 0:
        return False, "mariadb-install-db failed:\n" + p.stdout.decode(errors='replace')[-1500:]
    logf = open(log_path, "w")
    pid_path = os.path.join(DB_MARIADB_DIR, "mariadb.pid")
    _mariadb_proc = subprocess.Popen(
        ["mariadbd",
         f"--datadir={data_dir}",
         f"--port={MARIADB_PORT}",
         f"--socket={sock_path}",
         f"--pid-file={pid_path}",
         "--bind-address=127.0.0.1",
         "--skip-networking=0",
         "--skip-name-resolve",
         f"--log-error={log_path}.err",
         #--user= only allowed when running as root; unprivileged
         #mariadbd inherits the current uid.
        ],
        stdout=logf, stderr=subprocess.STDOUT)
    if not _probe_port(MARIADB_PORT, timeout=30):
        return False, f"mariadbd didn't open port {MARIADB_PORT} (log: {log_path}, {log_path}.err)"
    # Create role + db. Root has no password right after install_db.
    sqls = [
        "CREATE USER 'catchchallenger'@'localhost' IDENTIFIED BY 'catchchallenger';",
        "CREATE USER 'catchchallenger'@'127.0.0.1' IDENTIFIED BY 'catchchallenger';",
        "CREATE DATABASE catchchallenger;",
        "GRANT ALL ON catchchallenger.* TO 'catchchallenger'@'localhost';",
        "GRANT ALL ON catchchallenger.* TO 'catchchallenger'@'127.0.0.1';",
        "FLUSH PRIVILEGES;",
    ]
    for q in sqls:
        p = subprocess.run(
            ["mariadb", "--protocol=tcp", "-h", "127.0.0.1",
             "-P", str(MARIADB_PORT), "-u", "root", "-e", q],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if p.returncode != 0:
            return False, f"mariadb DDL failed ({q}):\n" + p.stdout.decode(errors='replace')[-500:]
    return True, f"ephemeral mariadb up on :{MARIADB_PORT}"


def stop_mariadb():
    """Stop the ephemeral MariaDB daemon and wipe the data dir
    (DB_MARIADB_DIR). Per operator: clean at start AND end."""
    global _mariadb_proc
    if _mariadb_proc is not None:
        try:
            _mariadb_proc.terminate()
            try:
                _mariadb_proc.wait(timeout=8)
            except subprocess.TimeoutExpired:
                _mariadb_proc.kill()
                _mariadb_proc.wait(timeout=5)
        except ProcessLookupError:
            pass
        _mariadb_proc = None
    if os.path.isdir(DB_MARIADB_DIR):
        shutil.rmtree(DB_MARIADB_DIR, ignore_errors=True)


# ── ephemeral nginx datapack mirror ──────────────────────────────────────────
_nginx_proc = None


def _stage_nginx_root():
    """Wipe and re-stage NGINX_ROOT with the bits nginx needs to serve.

    Layout when this returns:
        NGINX_ROOT/
            pack/                 ← datapack-archive.sh tarballs (.tar.zst)
            datapack-list/        ← datapack-list.php text files
            nginx.conf            ← config we'll launch nginx against
            nginx-error.log, nginx-access.log, nginx.pid (runtime)

    The raw datapack source is NOT copied — it's served straight from
    /home/user/Desktop/CatchChallenger/CatchChallenger-datapack/ via
    an nginx `alias` directive in the conf written by start_nginx().
    """
    if os.path.isdir(NGINX_ROOT):
        shutil.rmtree(NGINX_ROOT, ignore_errors=True)
    os.makedirs(NGINX_PACK_DIR, exist_ok=True)
    os.makedirs(NGINX_LIST_DIR, exist_ok=True)


def _run_datapack_archive():
    """Invoke datapack-archive.sh with (datapack_src, NGINX_PACK_DIR).
    The shipped script writes datapack[*].tar.zst into the output dir."""
    if not os.path.isfile(DATAPACK_ARCHIVE_SH):
        return False, f"datapack-archive.sh missing: {DATAPACK_ARCHIVE_SH}"
    p = subprocess.run(
        ["bash", DATAPACK_ARCHIVE_SH, DATAPACK_SRC, NGINX_PACK_DIR],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        timeout=600)
    if p.returncode != 0:
        return False, ("datapack-archive.sh rc=%d\n" % p.returncode) + \
                      p.stdout.decode(errors="replace")[-800:]
    # The script generates at least datapack.tar.zst; require that to exist.
    base_zst = os.path.join(NGINX_PACK_DIR, "datapack.tar.zst")
    if not os.path.isfile(base_zst):
        return False, "datapack.tar.zst not produced"
    return True, f"{base_zst}"


def _run_datapack_list_php():
    """Invoke datapack-list.php once for the base list and once per
    maincode (+ per sub). The script expects to find `datapack/` as a
    *relative* dir in CWD — so we cwd into NGINX_ROOT after dropping
    a temporary `datapack` symlink pointing at the source tree, run
    the PHP, capture stdout into NGINX_LIST_DIR, then remove the
    symlink."""
    if not os.path.isfile(DATAPACK_LIST_PHP):
        return False, f"datapack-list.php missing: {DATAPACK_LIST_PHP}"
    dp_link = os.path.join(NGINX_ROOT, "datapack")
    if os.path.islink(dp_link) or os.path.exists(dp_link):
        os.remove(dp_link)
    os.symlink(DATAPACK_SRC, dp_link)
    try:
        # base
        p = subprocess.run(["php", DATAPACK_LIST_PHP],
                           cwd=NGINX_ROOT,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                           timeout=300)
        if p.returncode != 0:
            return False, "datapack-list.php base failed: " + \
                          p.stderr.decode(errors="replace")[-300:]
        with open(os.path.join(NGINX_LIST_DIR, "base.txt"), "wb") as f:
            f.write(p.stdout)
        # per maincode + per subcode
        map_main = os.path.join(DATAPACK_SRC, "map", "main")
        if os.path.isdir(map_main):
            for mc in sorted(os.listdir(map_main)):
                if not os.path.isdir(os.path.join(map_main, mc)):
                    continue
                p = subprocess.run(
                    ["php", DATAPACK_LIST_PHP, f"main={mc}"],
                    cwd=NGINX_ROOT,
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                    timeout=300)
                if p.returncode == 0:
                    with open(os.path.join(NGINX_LIST_DIR,
                                           f"main-{mc}.txt"), "wb") as f:
                        f.write(p.stdout)
                sub_dir = os.path.join(map_main, mc, "sub")
                if os.path.isdir(sub_dir):
                    for sc in sorted(os.listdir(sub_dir)):
                        if not os.path.isdir(os.path.join(sub_dir, sc)):
                            continue
                        p = subprocess.run(
                            ["php", DATAPACK_LIST_PHP,
                             f"main={mc}", f"sub={sc}"],
                            cwd=NGINX_ROOT,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            timeout=300)
                        if p.returncode == 0:
                            out = os.path.join(NGINX_LIST_DIR,
                                               f"sub-{mc}-{sc}.txt")
                            with open(out, "wb") as f:
                                f.write(p.stdout)
    finally:
        if os.path.islink(dp_link):
            os.remove(dp_link)
    return True, "base.txt + per-main/sub lists generated"


def _write_nginx_conf():
    """Write a minimal nginx.conf. The two `alias` directives split
    /datapack/pack/ and /datapack/datapack-list/ off the raw
    /datapack/ tree before nginx falls through to the upstream
    checkout — order doesn't matter; nginx picks the longest-prefix
    match. `daemon off` keeps the process attached to our Popen
    handle so we can SIGTERM cleanly."""
    conf = f"""\
worker_processes 1;
daemon off;
error_log {NGINX_ERR_LOG} info;
pid {NGINX_PID};

events {{
    worker_connections 64;
}}

http {{
    access_log {NGINX_ACC_LOG};
    sendfile on;
    types_hash_max_size 1024;
    default_type application/octet-stream;
    types {{
        text/plain               txt;
        image/png                png;
        image/jpeg               jpg jpeg;
        image/gif                gif;
        application/xml          xml tmx tsx;
        audio/opus               opus;
        application/octet-stream tar zst tar.zst;
    }}

    server {{
        listen 127.0.0.1:{NGINX_PORT};
        server_name localhost;

        # datapack-archive.sh output (tarballs)
        location /datapack/pack/ {{
            alias {NGINX_PACK_DIR}/;
            autoindex on;
        }}
        # datapack-list.php output (file lists)
        location /datapack/datapack-list/ {{
            alias {NGINX_LIST_DIR}/;
            autoindex on;
        }}
        # Everything else under /datapack/ → raw source checkout.
        location /datapack/ {{
            alias {DATAPACK_SRC}/;
            autoindex on;
        }}
    }}
}}
"""
    with open(NGINX_CONF, "w") as f:
        f.write(conf)


def start_nginx():
    """Stage NGINX_ROOT, run the archive + list generators, then
    launch nginx in foreground on NGINX_PORT. Returns (ok, detail)."""
    global _nginx_proc
    if not shutil.which("nginx"):
        return False, "nginx not on $PATH"
    if not shutil.which("php"):
        return False, "php not on $PATH (datapack-list.php needs it)"
    if not os.path.isdir(DATAPACK_SRC):
        return False, f"datapack source missing: {DATAPACK_SRC}"
    _stage_nginx_root()
    ok, detail = _run_datapack_archive()
    if not ok:
        return False, "datapack-archive: " + detail
    ok, detail = _run_datapack_list_php()
    if not ok:
        return False, "datapack-list: " + detail
    _write_nginx_conf()
    logf = open(os.path.join(NGINX_ROOT, "nginx.stdout"), "w")
    _nginx_proc = subprocess.Popen(
        ["nginx", "-c", NGINX_CONF, "-p", NGINX_ROOT],
        stdout=logf, stderr=subprocess.STDOUT)
    if not _probe_port(NGINX_PORT, timeout=10):
        return False, (f"nginx didn't open port {NGINX_PORT} "
                       f"(see {NGINX_ERR_LOG})")
    return True, f"nginx serving /datapack/ on :{NGINX_PORT}"


def stop_nginx():
    """Stop the nginx daemon and remove its on-disk staging dir."""
    global _nginx_proc
    if _nginx_proc is not None:
        try:
            _nginx_proc.terminate()
            try:
                _nginx_proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                _nginx_proc.kill()
                _nginx_proc.wait(timeout=5)
        except ProcessLookupError:
            pass
        _nginx_proc = None
    if os.path.isdir(NGINX_ROOT):
        shutil.rmtree(NGINX_ROOT, ignore_errors=True)


def wipe_postgresql():
    """Drop every table in the ephemeral postgres on PG_PORT and
    replay every .sql under server/databases/postgresql/."""
    sql_dir = os.path.join(ROOT, "server", "databases", "postgresql")
    if not os.path.isdir(sql_dir):
        return False, f"sql dir missing: {sql_dir}"
    base_args = ["psql", "-h", "127.0.0.1", "-p", str(PG_PORT),
                 "-U", "catchchallenger", "-d", "catchchallenger"]
    # Drop everything.
    subprocess.run(base_args + ["-c", "DROP SCHEMA public CASCADE; CREATE SCHEMA public;"],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    for fn in sorted(os.listdir(sql_dir)):
        if not fn.endswith(".sql"):
            continue
        with open(os.path.join(sql_dir, fn), "rb") as fp:
            p = subprocess.run(base_args, stdin=fp,
                               stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        if p.returncode != 0:
            return False, f"psql replay of {fn} failed: {p.stderr.decode(errors='replace').strip()[:300]}"
    return True, "DB wiped + schema replayed"


def wipe_mysql():
    """Wipe + replay against the ephemeral mariadb on MARIADB_PORT."""
    sql_dir = os.path.join(ROOT, "server", "databases", "mysql")
    if not os.path.isdir(sql_dir):
        return False, f"sql dir missing: {sql_dir}"
    base_args = ["mariadb", "--protocol=tcp", "-h", "127.0.0.1",
                 "-P", str(MARIADB_PORT),
                 "-u", "catchchallenger", "-pcatchchallenger", "catchchallenger"]
    # Drop everything.
    drop_q = ("SET FOREIGN_KEY_CHECKS=0; "
              "DROP DATABASE catchchallenger; CREATE DATABASE catchchallenger;")
    p = subprocess.run(
        ["mariadb", "--protocol=tcp", "-h", "127.0.0.1",
         "-P", str(MARIADB_PORT), "-u", "root", "-e", drop_q],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if p.returncode != 0:
        return False, "drop+recreate db failed: " + p.stdout.decode(errors='replace')[-300:]
    for fn in sorted(os.listdir(sql_dir)):
        if not fn.endswith(".sql"):
            continue
        with open(os.path.join(sql_dir, fn), "rb") as fp:
            p = subprocess.run(base_args, stdin=fp,
                               stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        if p.returncode != 0:
            return False, f"mariadb replay of {fn} failed: {p.stderr.decode(errors='replace').strip()[:300]}"
    return True, "DB wiped + schema replayed"


def db_host_for(backend):
    """The XML <host> value passed to the cluster servers.

    PG: abuse libpq's space-separated key=value conninfo syntax —
        setting host to '127.0.0.1 port=5433' makes EventLoopPostgresql's
        strcat-based conninfo build resolve to a valid string targeting
        the ephemeral instance.
    MariaDB: EventLoopMySQL parses 'host:port' explicitly (patched in
        with the ephemeral-port support). Emit '127.0.0.1:3307'."""
    if backend == "postgresql":
        return f"127.0.0.1 port={PG_PORT}"
    return f"127.0.0.1:{MARIADB_PORT}"


# ── server-properties.xml templates ──────────────────────────────────────────
# Hand-built XML; production deploys would derive these from
# server/CONFIG.md. Every key the binary actually reads is set
# explicitly so an unspecified default never silently kicks in.
def _xml_master(db_kind, max_players):
    db_host = db_host_for(db_kind)
    # Master reads db-login + db-base + db-common-0 (NOT db-common —
    # the loop iterates db-common-N). Each section must explicitly
    # point at the ephemeral DB; without any one of them the master
    # falls back to a hardcoded localhost/root/catchchallenger_base
    # default and crashes at syncConnect.
    # Master expects the listen port as <port>, not <server-port>.
    # Master serializes CommonSettingsCommon into the C211 reply's
    # raw byte blob at offsets 0x00..0x0B (automatic_account_creation,
    # character_delete_time, min/max_character, max_pseudo_size,
    # maxPlayerMonsters, maxWarehousePlayerMonsters). Every one of
    # those keys MUST also be in login.xml — otherwise master's reply
    # bytes diverge from login's pre-computed blob and login aborts
    # at LinkToMasterProtocolParsingMessage.cpp:503 with
    # "C211 different CharactersGroup".
    return f"""<?xml version="1.0"?>
<configuration>
    <port value="{PORT_MASTER}"/>
    <server-ip value="127.0.0.1"/>
    <max-players value="{max_players}"/>
    <max_pseudo_size value="32"/>
    <min_character value="1"/>
    <max_character value="1"/>
    <maxPlayerMonsters value="6"/>
    <maxWarehousePlayerMonsters value="50"/>
    <automatic_account_creation value="true"/>
    <character_delete_time value="3600"/>
    <token value="{MASTER_AUTH_TOKEN}"/>
    <db>
        <type value="{db_kind}"/>
        <host value="{db_host}"/>
        <login value="catchchallenger"/>
        <pass value="catchchallenger"/>
        <db value="catchchallenger"/>
    </db>
    <db-login>
        <type value="{db_kind}"/>
        <host value="{db_host}"/>
        <login value="catchchallenger"/>
        <pass value="catchchallenger"/>
        <db value="catchchallenger"/>
        <considerDownAfterNumberOfTry value="3"/>
        <tryInterval value="5"/>
    </db-login>
    <db-base>
        <type value="{db_kind}"/>
        <host value="{db_host}"/>
        <login value="catchchallenger"/>
        <pass value="catchchallenger"/>
        <db value="catchchallenger"/>
        <considerDownAfterNumberOfTry value="3"/>
        <tryInterval value="5"/>
    </db-base>
    <db-common-0>
        <type value="{db_kind}"/>
        <host value="{db_host}"/>
        <login value="catchchallenger"/>
        <pass value="catchchallenger"/>
        <db value="catchchallenger"/>
        <considerDownAfterNumberOfTry value="3"/>
        <tryInterval value="5"/>
        <charactersGroup value="default"/>
    </db-common-0>
    <chat><kick-on-flood value="false"/></chat>
</configuration>
"""


def _xml_login(port, db_kind, max_players):
    db_host = db_host_for(db_kind)
    # Login expects its listen port as <port> (NOT <server-port>).
    # httpDatapackMirror is required by EventLoopServerLoginSlave —
    # it aborts on empty (the "not coded for now" branch). The login
    # server uses its OWN <httpDatapackMirror> to build the protocol
    # reply sent to clients, so this URL is what the client actually
    # hits (NOT the value in gsa's XML). Always point at the
    # ephemeral nginx; the test varies gsa's mirror separately.
    return f"""<?xml version="1.0"?>
<configuration>
    <port value="{port}"/>
    <server-ip value="127.0.0.1"/>
    <max-players value="{max_players}"/>
    <httpDatapackMirror value="{NGINX_URL_BASE}"/>
    <max_pseudo_size value="32"/>
    <min_character value="1"/>
    <max_character value="1"/>
    <max_pseudo_size value="32"/>
    <maxPlayerMonsters value="6"/>
    <maxWarehousePlayerMonsters value="50"/>
    <automatic_account_creation value="true"/>
    <character_delete_time value="3600"/>
    <tolerantMode value="true"/>
    <compression value="zstd"/>
    <compressionLevel value="1"/>
    <db>
        <type value="{db_kind}"/>
        <host value="{db_host}"/>
        <login value="catchchallenger"/>
        <pass value="catchchallenger"/>
        <db value="catchchallenger"/>
    </db>
    <db-login>
        <type value="{db_kind}"/>
        <host value="{db_host}"/>
        <login value="catchchallenger"/>
        <pass value="catchchallenger"/>
        <db value="catchchallenger"/>
        <considerDownAfterNumberOfTry value="3"/>
        <tryInterval value="5"/>
    </db-login>
    <db-base>
        <type value="{db_kind}"/>
        <host value="{db_host}"/>
        <login value="catchchallenger"/>
        <pass value="catchchallenger"/>
        <db value="catchchallenger"/>
        <considerDownAfterNumberOfTry value="3"/>
        <tryInterval value="5"/>
    </db-base>
    <db-common-0>
        <type value="{db_kind}"/>
        <host value="{db_host}"/>
        <login value="catchchallenger"/>
        <pass value="catchchallenger"/>
        <db value="catchchallenger"/>
        <considerDownAfterNumberOfTry value="3"/>
        <tryInterval value="5"/>
        <charactersGroup value="default"/>
    </db-common-0>
    <master>
        <host value="127.0.0.1"/>
        <port value="{PORT_MASTER}"/>
        <token value="{MASTER_AUTH_TOKEN}"/>
    </master>
</configuration>
"""


def _xml_gameserver(port, db_kind, max_players, mirror):
    db_host = db_host_for(db_kind)
    # gsa is OPTIONALLY built with CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR.
    # When ON  → mirror MUST be non-empty (gsa exits otherwise) and the
    #            client fetches `datapack-list/base.txt` and
    #            `pack/datapack*.tar.zst` from it.
    # When OFF → mirror MAY be empty; gsa pushes the datapack to the
    #            client over the gameplay protocol (in-band push path).
    # testingcluster.py exercises both modes, see MIRROR_VARIANTS.
    return f"""<?xml version="1.0"?>
<configuration>
    <server-port value="{port}"/>
    <server-ip value="127.0.0.1"/>
    <max-players value="{max_players}"/>
    <mainDatapackCode value="test"/>
    <httpDatapackMirror value="{mirror}"/>
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
    <compression value="zstd"/>
    <compressionLevel value="1"/>
    <db>
        <type value="{db_kind}"/>
        <host value="{db_host}"/>
        <login value="catchchallenger"/>
        <pass value="catchchallenger"/>
        <db value="catchchallenger"/>
    </db>
    <db-server>
        <type value="{db_kind}"/>
        <host value="{db_host}"/>
        <login value="catchchallenger"/>
        <pass value="catchchallenger"/>
        <db value="catchchallenger"/>
    </db-server>
    <db-common>
        <type value="{db_kind}"/>
        <host value="{db_host}"/>
        <login value="catchchallenger"/>
        <pass value="catchchallenger"/>
        <db value="catchchallenger"/>
    </db-common>
    <master>
        <!-- where master is, so gsa can connect TO master -->
        <host value="127.0.0.1"/>
        <port value="{PORT_MASTER}"/>
        <tryInterval value="5"/>
        <considerDownAfterNumberOfTry value="3"/>
        <!-- gsa's OWN public address, advertised back to master -->
        <external-server-ip value="127.0.0.1"/>
        <external-server-port value="{port}"/>
        <token value="{MASTER_AUTH_TOKEN}"/>
        <charactersGroup value="default"/>
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
    here is an io_uring build by construction.

    game-server-alone is built TWICE — once with
    CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR=ON (the production
    default, http-mirror path) and once with =OFF (in-protocol push
    path). testingcluster.py exercises both at runtime via
    MIRROR_VARIANTS. Build dirs:
        <base>/server_game-server-alone/        ← ONLYBYMIRROR=ON
        <base>/server_game-server-alone-push/   ← ONLYBYMIRROR=OFF
    Master and login are unaffected by this toggle.

    Returns (ok, fail_detail)."""
    cmake_flags = [f"-D{db_define}=ON"]
    base = os.path.join(TMPFS_BUILD, "cluster",
                        db_define.replace("CATCHCHALLENGER_DB_", "").lower())
    # Master + login: single build each.
    for sub in ["server/master", "server/login"]:
        bd = os.path.join(base, sub.replace("/", "_"))
        ok, detail = cmake_configure_and_build(sub, bd, cmake_flags)
        if not ok:
            return False, f"{sub}: {detail}"
    # gsa http-mirror build (the default — ONLYBYMIRROR is ON in CMakeLists).
    gsa_mirror_dir = os.path.join(base, "server_game-server-alone")
    ok, detail = cmake_configure_and_build(
        "server/game-server-alone", gsa_mirror_dir,
        cmake_flags + ["-DCATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR=ON"])
    if not ok:
        return False, "server/game-server-alone (mirror): " + detail
    # gsa in-protocol-push build (ONLYBYMIRROR off → empty mirror allowed).
    gsa_push_dir = os.path.join(base, "server_game-server-alone-push")
    ok, detail = cmake_configure_and_build(
        "server/game-server-alone", gsa_push_dir,
        cmake_flags + ["-DCATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR=OFF"])
    if not ok:
        return False, "server/game-server-alone (push): " + detail
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
        ("server/master",                 "catchchallenger-server-master"),
        ("server/login",                  "catchchallenger-server-login"),
        # Both gsa builds (mirror + push) must satisfy the
        # exactly-one-DB-client rule. The build dirs share the same
        # cmake source, only the ONLYBYMIRROR define differs.
        ("server/game-server-alone",      "catchchallenger-game-server-alone"),
        ("server/game-server-alone-push", "catchchallenger-game-server-alone"),
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
        # Each binary reads its config XML from
        # getFolderFromFile(argv[0]) — i.e. relative to the exe path.
        # We exec via a symlink in the work_dir so argv[0] is
        # './<exe>' and the config dir resolves to cwd (= work_dir).
        # Each binary expects a different XML filename:
        #     master  -> master.xml
        #     login   -> login.xml
        #     gateway -> gateway.xml
        #     gsa     -> server-properties.xml
        # (per server/{master,login,gateway,game-server-alone}/*.cpp)
        xml_name = {
            "master": "master.xml",
            "login1": "login.xml",
            "login2": "login.xml",
            "game1":  "server-properties.xml",
            "game2":  "server-properties.xml",
            "gateway": "gateway.xml",
        }.get(self.label, "server-properties.xml")
        with open(os.path.join(self.work_dir, xml_name), "w") as fp:
            fp.write(self.xml_text)
        # Each server needs its own datapack. Symlink to avoid copying ~100 MB.
        dp_dst = os.path.join(self.work_dir, "datapack")
        if not os.path.exists(dp_dst):
            os.symlink(DATAPACK_SRC, dp_dst)
        # Same for database/ scratch dir (gsa expects it for FILE backend
        # diagnostics even when DB is PG/MySQL).
        os.makedirs(os.path.join(self.work_dir, "database"), exist_ok=True)
        # Symlink the binary so argv[0] becomes './<exe>' and
        # getFolderFromFile(argv[0]) resolves to the work_dir.
        exe_name = os.path.basename(self.exe)
        local_exe = os.path.join(self.work_dir, exe_name)
        if not os.path.exists(local_exe):
            os.symlink(self.exe, local_exe)
        logf = open(self.log_path, "w")
        self.proc = subprocess.Popen(
            ["./" + exe_name], cwd=self.work_dir,
            stdout=logf, stderr=subprocess.STDOUT)
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
# Run each backend with two <max-players> values so the EventLoopClientList
# constructor is exercised both at the small (50) and the realistic large
# (2000) end of its allocation range. Helps catch ctor-time hangs that
# only appear under high pre-allocation.
MAX_PLAYERS_VARIANTS = (50, 2000)

# Run each backend with both datapack-delivery modes:
#   * mirror_url == NGINX_URL_BASE → gsa built with ONLYBYMIRROR; client
#     hits nginx at NGINX_PORT for tar.zst + file lists.
#   * mirror_url == ""             → gsa built WITHOUT ONLYBYMIRROR;
#     the client receives the datapack pushed inline over the
#     gameplay socket. No HTTP needed for that variant.
NGINX_URL_BASE = f"http://localhost:{NGINX_PORT}/datapack/"
MIRROR_VARIANTS = (
    ("mirror", NGINX_URL_BASE),
    ("push",   ""),
)


def test_one_backend(db_name, db_define, db_cli, sql_subdir):
    """Runs the per-backend test. Tracks every spawned server +
    ephemeral DB so the finally: block can guarantee teardown even
    on exception. DB spawn + build + ldd are done once per backend;
    the cluster startup + client tests are repeated for every
    <max-players> variant."""
    log_info(f"--- backend: {db_name} ---")
    t_start = time.monotonic()
    spawned_servers = []
    try:
        return _test_one_backend_inner(db_name, db_define, db_cli, sql_subdir,
                                       t_start, spawned_servers)
    finally:
        for s in spawned_servers:
            s.stop()
        (stop_postgresql if db_name == "postgresql" else stop_mariadb)()


def _test_one_backend_inner(db_name, db_define, db_cli, sql_subdir,
                             t_start, spawned_servers):

    # 1a) Spin up an ephemeral DB daemon on a non-default port. No
    #     system service touched, no sudo, no preexisting role/db
    #     needed. The data dir lives under
    #     /mnt/data/perso/tmpfs/cluster/db/<backend>/ and is wiped
    #     at the start of every run.
    log_info(f"  spawn ephemeral {db_name}")
    t_db = time.monotonic()
    start_fn = start_postgresql if db_name == "postgresql" else start_mariadb
    ok, detail = start_fn()
    if not ok:
        log_fail(f"{db_name}/db-spawn", detail, time.monotonic() - t_db)
        return
    log_pass(f"{db_name}/db-spawn", detail, time.monotonic() - t_db)

    # 1b) Schema replay.
    log_info(f"  wipe + reload schema ({db_name})")
    t_wipe = time.monotonic()
    wipe_fn = wipe_postgresql if db_name == "postgresql" else wipe_mysql
    ok, detail = wipe_fn()
    if not ok:
        log_fail(f"{db_name}/db-setup", detail, time.monotonic() - t_wipe)
        (stop_postgresql if db_name == "postgresql" else stop_mariadb)()
        return
    log_pass(f"{db_name}/db-setup", detail, time.monotonic() - t_wipe)

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

    # 3) Per-(max_players × mirror_mode): start cluster, run client
    #    tests, stop cluster. DB + build are shared, so the only thing
    #    that changes per variant is the on-disk server-properties.xml
    #    (re-rendered from the templates) and which gsa binary is
    #    spawned (mirror-build vs push-build).
    for max_players in MAX_PLAYERS_VARIANTS:
        for mirror_label, mirror_url in MIRROR_VARIANTS:
            _run_cluster_variant(db_name, base, max_players,
                                 mirror_label, mirror_url,
                                 spawned_servers, t_start)


def _gsa_subdir(mirror_label):
    """The build dir suffix matches the gsa binary built in
    build_cluster() for this mirror mode."""
    return ("server/game-server-alone"
            if mirror_label == "mirror"
            else "server/game-server-alone-push")


def _run_cluster_variant(db_name, base, max_players,
                         mirror_label, mirror_url,
                         spawned_servers, t_start):
    """One run of master + 2×login + 2×game-server for the requested
    (max_players, mirror_mode). Cluster servers are scoped under a
    per-variant work_root so concurrent or stacked runs don't share
    state."""
    log_info(f"  variant: max-players={max_players} mirror={mirror_label}")
    work_root = os.path.join(CLUSTER_RUN_ROOT, db_name,
                             f"mp{max_players}-{mirror_label}")
    if os.path.isdir(work_root):
        shutil.rmtree(work_root, ignore_errors=True)
    os.makedirs(work_root, exist_ok=True)

    tag = f"{db_name}/mp{max_players}-{mirror_label}"
    # Each variant gets its own server set; remember where the variant
    # started in the global spawned_servers list so we can stop only
    # this variant's processes at the end.
    variant_start = len(spawned_servers)

    try:
        master = ServerProc(
            "master",
            bin_path(base, "server/master", "catchchallenger-server-master"),
            os.path.join(work_root, "master"),
            _xml_master(db_name, max_players)).start()
        spawned_servers.append(master)
        ok, detail = master.wait_ready()
        if not ok:
            log_fail(f"{tag}/master-start", detail, time.monotonic() - t_start)
            return
        log_pass(f"{tag}/master-start", f"port {PORT_MASTER}", 0.0)

        login1 = ServerProc(
            "login1",
            bin_path(base, "server/login", "catchchallenger-server-login"),
            os.path.join(work_root, "login1"),
            _xml_login(PORT_LOGIN_1, db_name, max_players)).start()
        spawned_servers.append(login1)
        login2 = ServerProc(
            "login2",
            bin_path(base, "server/login", "catchchallenger-server-login"),
            os.path.join(work_root, "login2"),
            _xml_login(PORT_LOGIN_2, db_name, max_players)).start()
        spawned_servers.append(login2)
        for ln in (login1, login2):
            ok, detail = ln.wait_ready()
            if not ok:
                log_fail(f"{tag}/{ln.label}-start", detail, 0.0)
                return
            log_pass(f"{tag}/{ln.label}-start",
                     f"port {PORT_LOGIN_1 if ln is login1 else PORT_LOGIN_2}",
                     0.0)

        # Start game1 first, wait until it's bound (which means
        # preload_X has populated the shared dictionary_map table),
        # THEN start game2. Two gsa instances sharing one DB race
        # on the dictionary_map(id,map) INSERTs at startup — the
        # loser hits "duplicate key value violates unique constraint
        # dictionary_map_pkey" and aborts. Sequential boot lets the
        # second gsa see the rows committed by the first via the
        # SELECT pre-pass and skip the conflicting INSERT.
        gsa_bin_subdir = _gsa_subdir(mirror_label)
        gsa_bin = bin_path(base, gsa_bin_subdir,
                           "catchchallenger-game-server-alone")
        game1 = ServerProc(
            "game1",
            gsa_bin,
            os.path.join(work_root, "game1"),
            _xml_gameserver(PORT_GAME_1, db_name, max_players,
                            mirror_url)).start()
        spawned_servers.append(game1)
        ok, detail = game1.wait_ready()
        if not ok:
            log_fail(f"{tag}/game1-start", detail, 0.0)
            return
        log_pass(f"{tag}/game1-start", f"port {PORT_GAME_1}", 0.0)

        game2 = ServerProc(
            "game2",
            gsa_bin,
            os.path.join(work_root, "game2"),
            _xml_gameserver(PORT_GAME_2, db_name, max_players,
                            mirror_url)).start()
        spawned_servers.append(game2)
        ok, detail = game2.wait_ready()
        if not ok:
            log_fail(f"{tag}/game2-start", detail, 0.0)
            return
        log_pass(f"{tag}/game2-start", f"port {PORT_GAME_2}", 0.0)

        # 4) First client autologin via login #1.
        log_info("  client #1 → login server #1")
        cred = "cluster_pinned_user"
        char = "Player"
        t_c1 = time.monotonic()
        ok1, det1 = client_connect_via(
            PORT_LOGIN_1, cred, cred, char, "first-via-login1", timeout=120)
        if not ok1:
            log_fail(f"{tag}/client1-via-login1", det1,
                     time.monotonic() - t_c1)
            return
        log_pass(f"{tag}/client1-via-login1", det1, time.monotonic() - t_c1)

        # 5) Second client autologin via login #2 (sticky check).
        log_info("  client #2 → login server #2 (sticky check)")
        t_c2 = time.monotonic()
        ok2, det2 = client_connect_via(
            PORT_LOGIN_2, cred, cred, char, "second-via-login2", timeout=120)
        if not ok2:
            log_fail(f"{tag}/client2-via-login2", det2,
                     time.monotonic() - t_c2)
            return

        if det1.startswith("map=") and det2.startswith("map=") and det1 != det2:
            log_fail(f"{tag}/client2-via-login2",
                     f"state divergence: c1={det1!r} c2={det2!r}",
                     time.monotonic() - t_c2)
        else:
            same = "exact match" if det1 == det2 else "both reached map"
            log_pass(f"{tag}/client2-via-login2",
                     f"sticky verified ({same}): {det2}",
                     time.monotonic() - t_c2)
    finally:
        # Stop only the servers spawned by THIS variant; leave any
        # outer setup intact for the next variant.
        while len(spawned_servers) > variant_start:
            spawned_servers.pop().stop()


# ── main ─────────────────────────────────────────────────────────────────────
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--continue", dest="cont", action="store_true",
                    help="ignored (kept for all.sh compatibility)")
    ap.add_argument("--backends", default="",
                    help="comma-separated subset of backends to run "
                         "(postgresql, mysql); empty = all")
    args = ap.parse_args()

    global BACKENDS
    if args.backends:
        wanted = {b.strip() for b in args.backends.split(",") if b.strip()}
        BACKENDS = [b for b in BACKENDS_FULL if b[0] in wanted]
        if not BACKENDS:
            log_fail("main", f"no matching backends in '{args.backends}'")
            sys.exit(2)

    log_info("testingcluster: io_uring × {"
             + ", ".join(b[0] for b in BACKENDS) + "}")
    log_info(f"topology: 2×login (61930/61931) + 1×master (61935) + 2×game (61940/61941)")
    log_info(f"nginx mirror: http://localhost:{NGINX_PORT}/datapack/ "
             f"(staged from {DATAPACK_SRC})")

    # 0) Spin up the ephemeral nginx that fronts the datapack mirror.
    #    The "mirror" leg of MIRROR_VARIANTS needs it; the "push" leg
    #    doesn't, but starting it once for the whole run keeps the
    #    teardown simple. If nginx can't start, the http leg will
    #    correctly FAIL — we don't pretend it would have worked.
    t_nginx = time.monotonic()
    nginx_ok, nginx_detail = start_nginx()
    if nginx_ok:
        log_pass("nginx-start", nginx_detail, time.monotonic() - t_nginx)
    else:
        # Don't bail out — the push variant still runs without HTTP.
        # Each mirror=mirror variant will hit a transferring error
        # and FAIL legitimately.
        log_fail("nginx-start", nginx_detail, time.monotonic() - t_nginx)

    try:
        for db_name, db_define, db_cli, sql_subdir in BACKENDS:
            test_one_backend(db_name, db_define, db_cli, sql_subdir)
    finally:
        stop_nginx()

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
