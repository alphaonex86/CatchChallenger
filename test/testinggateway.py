#!/usr/bin/env python3
"""
testinggateway.py — CatchChallenger gateway test.

Tests the catchchallenger-gateway binary under valgrind memcheck:

  1. Build server-cli-epoll (file-db backend), gateway, qtcpu800x600 client
     all with -std=c++11.
  2. For each datapack in (CatchChallenger-datapack, datapack-pkmn) and
     for each maincode + subcode combination found under map/main/<mc>/sub/<sc>/:
       For each (dest_http, gateway_http) in {0,1}^2:
         - Stage the datapack at the backend's build/datapack symlink.
         - Start an nginx instance (managed by this script) serving the
           datapack over HTTP.
         - Configure backend server-properties.xml (mainDatapackCode,
           subDatapackCode, httpDatapackMirror per dest_http flag).
         - Configure gateway gateway.xml (destination=backend, rewrite
           URLs per gateway_http flag).
         - Start backend, wait for "correctly bind:".
         - Start gateway UNDER VALGRIND, wait for "correctly bind:".
         - Run qtcpu800x600 client with --host/--port pointing at the
           gateway, expect success marker
           "MapVisualiserPlayer::mapDisplayedSlot()".
         - Stop client, gateway (collect valgrind report), backend, nginx.
"""
import sys
sys.dont_write_bytecode = True

import os, sys, signal, subprocess, threading, multiprocessing, json
import shutil, re, time, socket

import diagnostic
import build_paths
from cmd_helpers import clamp_local, assert_port_or_fail


build_paths.ensure_root()

# diagnostic.py is the canonical valgrind/sanitizer integration. We do
# not respect --sanitize here (gateway test is valgrind-only by design),
# but we still go through diagnostic so the per-run build dir suffix and
# command logging stay consistent with the rest of the harness.
DIAG = diagnostic.parse_diag_args()
_DIAG_SUFFIX = diagnostic.build_dir_suffix(DIAG)

# ── config ─────────────────────────────────────────────────────────────────
_CONFIG_PATH = os.path.join(os.path.expanduser("~"),
                            ".config", "catchchallenger-testing", "config.json")
with open(_CONFIG_PATH, "r") as _f:
    _config = json.load(_f)

ROOT  = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NPROC = str(multiprocessing.cpu_count())

# Two datapacks the user explicitly listed. Hard-coded paths per request;
# config.json's `paths.datapacks` may also point here but we trust the
# user-provided absolute paths.
DATAPACKS = [
    "/home/user/Desktop/CatchChallenger/CatchChallenger-datapack",
    "/home/user/Desktop/CatchChallenger/datapack-pkmn",
]

SERVER_FILEDB_PRO = os.path.join(ROOT, "server/epoll/catchchallenger-server-filedb.pro")
GATEWAY_PRO       = os.path.join(ROOT, "server/gateway/gateway.pro")
CLIENT_CPU_PRO    = os.path.join(ROOT, "client/qtcpu800x600/qtcpu800x600.pro")

SERVER_BUILD  = build_paths.build_path("server/epoll/build/testing-gateway-backend" + _DIAG_SUFFIX)
GATEWAY_BUILD = build_paths.build_path("server/gateway/build/testing-gateway" + _DIAG_SUFFIX)
CLIENT_BUILD  = build_paths.build_path("client/qtcpu800x600/build/testing-gateway-cpu" + _DIAG_SUFFIX)

SERVER_BIN  = "catchchallenger-server-cli-epoll"
GATEWAY_BIN = "catchchallenger-gateway"
CLIENT_BIN  = "catchchallenger"

BACKEND_HOST = "127.0.0.1"
BACKEND_PORT = 61918   # backend listen
GATEWAY_PORT = 61919   # gateway listen (client connects here)
NGINX_PORT   = 8765    # nginx listen (datapack mirror)

# nginx + www staging live in tmpfs so we leave nothing on disk
from test_config import TMPFS_ROOT
NGINX_PREFIX = os.path.join(TMPFS_ROOT, "cc-nginx-gateway")
NGINX_WWW    = os.path.join(NGINX_PREFIX, "www")
NGINX_CONF   = os.path.join(NGINX_PREFIX, "nginx.conf")
NGINX_TPL    = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                            "nginx-gateway.conf.in")

COMPILE_TIMEOUT       = 600
SERVER_READY_TIMEOUT  = 60
CLIENT_TIMEOUT        = 60
NGINX_READY_TIMEOUT   = 5

NICE_PREFIX = ["nice", "-n", "19", "ionice", "-c", "3"]

# ── colours ─────────────────────────────────────────────────────────────────
C_GREEN  = "\033[92m"
C_RED    = "\033[91m"
C_YELLOW = "\033[93m"
C_CYAN   = "\033[96m"
C_RESET  = "\033[0m"

results = []
total_expected = [0]
_last_log_time = [time.monotonic()]

backend_proc = None
gateway_proc = None
nginx_proc   = None

SCRIPT_NAME = os.path.basename(__file__)
from test_config import FAILED_JSON
import failed_cases as _fc
import phase_timer


def load_failed_cases():
    return _fc.load_names(SCRIPT_NAME)


def should_run(test_name, failed_cases):
    if failed_cases is None:
        return True
    return test_name in failed_cases


def save_failed_cases():
    failed = []
    for name, ok, detail, _elapsed in results:
        if not ok:
            d = _fc.make_detail(detail)
            d.update(_fc.pop_extras(name))
            failed.append((name, d))
    _fc.save(SCRIPT_NAME, failed)


def log_info(msg):
    print(f"{phase_timer.t()} {C_CYAN}[INFO]{C_RESET} {msg}")


def log_pass(name, detail=""):
    now = time.monotonic()
    elapsed = now - _last_log_time[0]
    _last_log_time[0] = now
    results.append((name, True, detail, elapsed))
    if len(results) > total_expected[0]:
        total_expected[0] = len(results)
    print(f"{phase_timer.t()} {C_GREEN}[PASS]{C_RESET} {len(results)}/{total_expected[0]} "
          f"{name}  {detail}  ({elapsed:.1f}s)")
    phase_timer.record_event("pass", name, ok=True, dt=elapsed, detail=detail)


def log_fail(name, detail=""):
    now = time.monotonic()
    elapsed = now - _last_log_time[0]
    _last_log_time[0] = now
    results.append((name, False, detail, elapsed))
    if len(results) > total_expected[0]:
        total_expected[0] = len(results)
    print(f"{phase_timer.t()} {C_RED}[FAIL]{C_RESET} {len(results)}/{total_expected[0]} "
          f"{name}  {detail}  ({elapsed:.1f}s)")
    phase_timer.record_event("fail", name, ok=False, dt=elapsed, detail=detail)
    li = 0
    _ctx = diagnostic.last_cmd_lines()
    while li < len(_ctx):
        print(_ctx[li])
        li += 1


def ensure_dir(path):
    os.makedirs(path, exist_ok=True)


def run_cmd(args, cwd, timeout=COMPILE_TIMEOUT, env=None):
    timeout = clamp_local(timeout)
    diagnostic.record_cmd(NICE_PREFIX + list(args), cwd)
    try:
        p = subprocess.run(NICE_PREFIX + list(args), cwd=cwd, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           env=env or os.environ)
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"TIMEOUT after {timeout}s"


def build_project(pro_file, build_dir, label):
    """Compile via cmake_helpers.build_project with cxx_version='c++11'.
    Per user request all three binaries (server, gateway, client) build
    against the C++11 standard."""
    import cmake_helpers as _ch
    return _ch.build_project(
        pro_file, build_dir, label,
        root=ROOT, nproc=NPROC,
        log_info=log_info, log_pass=log_pass, log_fail=log_fail,
        ensure_dir=ensure_dir, run_cmd=run_cmd,
        diag=DIAG, diag_module=diagnostic,
        cxx_version="c++11",
    )


# ── nginx management ────────────────────────────────────────────────────────
def nginx_available():
    return shutil.which("nginx") is not None


def write_nginx_conf():
    ensure_dir(NGINX_PREFIX)
    ensure_dir(NGINX_WWW)
    with open(NGINX_TPL, "r") as f:
        tpl = f.read()
    body = (tpl
            .replace("@PREFIX@", NGINX_PREFIX)
            .replace("@WWW_ROOT@", NGINX_WWW)
            .replace("@NGINX_PORT@", str(NGINX_PORT)))
    with open(NGINX_CONF, "w") as f:
        f.write(body)


def stage_nginx_www(server_datapack):
    """Mirror the active backend datapack into NGINX_WWW so the http
    case has something to serve. Use a symlink — the datapack is read-only
    from nginx's POV.

    Always (re)create the NGINX_WWW dir first: all.sh wipes the whole
    tmpfs root between runs, so on the first case of a fresh run the
    directory is gone and `os.symlink(target, link)` fails with
    ENOENT on the parent of `link`. write_nginx_conf() also creates
    it, but that's only called *after* this function on the first
    case (start_nginx → write_nginx_conf), so we need the mkdir
    here too."""
    ensure_dir(NGINX_WWW)
    link = os.path.join(NGINX_WWW, "datapack")
    if os.path.islink(link) or os.path.exists(link):
        try:
            os.remove(link)
        except OSError:
            shutil.rmtree(link, ignore_errors=True)
    os.symlink(server_datapack, link)


def start_nginx():
    global nginx_proc
    if not nginx_available():
        log_fail("nginx start", "nginx binary not found on PATH")
        return False
    write_nginx_conf()
    # Run nginx in NON-daemon mode so we own its lifetime via Popen, and
    # so a stale pidfile from a previous crashed run can't confuse the
    # next start. -g overrides the conf-level `daemon on`.
    args = ["nginx", "-p", NGINX_PREFIX + "/", "-c", NGINX_CONF,
            "-g", "daemon off;"]
    diagnostic.record_cmd(args, NGINX_PREFIX)
    log_info(f"starting nginx on :{NGINX_PORT}")
    nginx_proc = subprocess.Popen(
        args, cwd=NGINX_PREFIX,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        preexec_fn=os.setsid)
    # Probe — nginx binds within ~50ms.
    deadline = time.monotonic() + NGINX_READY_TIMEOUT
    while time.monotonic() < deadline:
        try:
            s = socket.create_connection(("127.0.0.1", NGINX_PORT), timeout=0.5)
            s.close()
            return True
        except OSError:
            if nginx_proc.poll() is not None:
                out = nginx_proc.stdout.read().decode(errors="replace")
                log_fail("nginx start", f"died early: {out[-300:]}")
                nginx_proc = None
                return False
            time.sleep(0.1)
    log_fail("nginx start", f"port :{NGINX_PORT} not reachable in {NGINX_READY_TIMEOUT}s")
    stop_nginx()
    return False


def stop_nginx():
    global nginx_proc
    if nginx_proc is None:
        return
    try:
        os.killpg(os.getpgid(nginx_proc.pid), signal.SIGTERM)
    except ProcessLookupError:
        pass
    try:
        nginx_proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(os.getpgid(nginx_proc.pid), signal.SIGKILL)
        except ProcessLookupError:
            pass
        nginx_proc.wait(timeout=2)
    nginx_proc = None


# ── server / gateway config writers ─────────────────────────────────────────
def write_backend_settings(maincode, subcode, http_url):
    """Write backend server-properties.xml. Pin the listen port to
    BACKEND_PORT (otherwise NormalServerGlobal seeds a random one),
    enable autologin, set datapack codes + http mirror."""
    xml = os.path.join(SERVER_BUILD, "server-properties.xml")
    ensure_dir(SERVER_BUILD)
    body = (
        '<?xml version="1.0"?>\n'
        '<configuration>\n'
        '    <server-port value="{port}"/>\n'
        '    <automatic_account_creation value="true"/>\n'
        '    <max-players value="200"/>\n'
        '    <httpDatapackMirror value="{http}"/>\n'
        '    <master>\n'
        '        <external-server-port value="{port}"/>\n'
        '    </master>\n'
        '    <content>\n'
        '        <mainDatapackCode value="{mc}"/>\n'
        '        <subDatapackCode value="{sc}"/>\n'
        '    </content>\n'
        '</configuration>\n'
    ).format(port=BACKEND_PORT, http=http_url, mc=maincode, sc=subcode)
    with open(xml, "w") as f:
        f.write(body)


def write_gateway_settings(rewrite_base, rewrite_main_sub):
    """Gateway reads gateway.xml from FacilityLibGeneral::applicationDirPath
    (== GATEWAY_BUILD here). The rewrite URLs MUST be non-empty — gateway
    aborts otherwise (EpollServerLoginSlave.cpp:147,162). For the
    "no http via gateway" cases we therefore set unreachable placeholders
    (the client falls back to the in-protocol datapack stream)."""
    xml = os.path.join(GATEWAY_BUILD, "gateway.xml")
    ensure_dir(GATEWAY_BUILD)
    body = (
        '<?xml version="1.0"?>\n'
        '<configuration>\n'
        '    <ip value=""/>\n'
        '    <port value="{gport}"/>\n'
        '    <destination_ip value="{bhost}"/>\n'
        '    <destination_port value="{bport}"/>\n'
        '    <destination_proxy_ip value=""/>\n'
        # Gateway aborts when destination_proxy_port is 0 OR > 65535
        # (EpollServerLoginSlave.cpp:129-133), regardless of whether
        # destination_proxy_ip is empty. We don't need an upstream
        # SOCKS/HTTP proxy for the test, but the validator requires a
        # syntactically-valid port, so pin to 1 (low port, won't ever
        # be reached because destination_proxy_ip is empty).
        '    <destination_proxy_port value="1"/>\n'
        '    <httpDatapackMirrorRewriteBase value="{rb}"/>\n'
        '    <httpDatapackMirrorRewriteMainAndSub value="{rms}"/>\n'
        '    <Linux>\n'
        '        <tcpCork value="false"/>\n'
        '        <tcpNodelay value="false"/>\n'
        '    </Linux>\n'
        '    <commandUpdateDatapack>\n'
        '        <base value=""/>\n'
        '        <main value=""/>\n'
        '        <sub value=""/>\n'
        '    </commandUpdateDatapack>\n'
        '</configuration>\n'
    ).format(gport=GATEWAY_PORT, bhost=BACKEND_HOST, bport=BACKEND_PORT,
             rb=rewrite_base, rms=rewrite_main_sub)
    with open(xml, "w") as f:
        f.write(body)


def stage_backend_datapack(datapack_src):
    """Replace SERVER_BUILD/datapack with a symlink to `datapack_src`.
    The backend treats <build_dir>/datapack as its base path (see
    BaseServer2.cpp:51)."""
    link = os.path.join(SERVER_BUILD, "datapack")
    if os.path.islink(link) or os.path.exists(link):
        try:
            os.remove(link)
        except OSError:
            shutil.rmtree(link, ignore_errors=True)
    os.symlink(datapack_src, link)


def reset_backend_state():
    """Drop file-db state + datapack-cache.bin between cases so a stale
    cache from the previous (datapack, maincode) doesn't leak in."""
    cache = os.path.join(SERVER_BUILD, "datapack-cache.bin")
    if os.path.exists(cache):
        os.remove(cache)
    db = os.path.join(SERVER_BUILD, "database")
    if os.path.isdir(db):
        shutil.rmtree(db)


# ── start helpers ───────────────────────────────────────────────────────────
def _start_and_wait_bind(label, args, cwd, timeout, wrap_with_valgrind):
    """Spawn args + wait for "correctly bind:". Returns (Popen, output_lines)
    or (None, output_lines)."""
    env = os.environ.copy()
    if wrap_with_valgrind:
        for k, v in diagnostic.runtime_env(DIAG).items():
            env[k] = v
        # Force memcheck regardless of --valgrind flag — gateway test is
        # valgrind-by-design per user request. Mirrors diagnostic.runtime_wrapper
        # output for memcheck.
        wrapper = ["valgrind",
                   "--error-exitcode=23",
                   "--child-silent-after-fork=yes",
                   "--tool=memcheck",
                   "--leak-check=full",
                   "--show-leak-kinds=all",
                   "--track-origins=yes",
                   "--errors-for-leak-kinds=definite,possible"]
        full = NICE_PREFIX + wrapper + list(args)
        timeout = max(timeout * 10, timeout)
    else:
        full = NICE_PREFIX + list(args)
    diagnostic.record_cmd(full, cwd)
    proc = subprocess.Popen(
        full, cwd=cwd,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env,
        preexec_fn=os.setsid)
    ready = threading.Event()
    output_lines = []

    def reader():
        for raw in iter(proc.stdout.readline, b""):
            line = raw.decode(errors="replace").rstrip("\n")
            output_lines.append(line)
            if "correctly bind:" in line:
                ready.set()
        ready.set()

    t = threading.Thread(target=reader, daemon=True)
    t.start()
    if not ready.wait(timeout=timeout):
        log_fail(label, f"timeout waiting for 'correctly bind:' ({timeout}s)")
        for l in output_lines[-30:]:
            print(f"  | {l}")
        _kill(proc)
        return None, output_lines
    if proc.poll() is not None:
        log_fail(label, f"died before bind, rc={proc.returncode}")
        for l in output_lines[-30:]:
            print(f"  | {l}")
        return None, output_lines
    log_pass(label, "correctly bind: detected")
    return proc, output_lines


def start_backend():
    global backend_proc
    binary = os.path.join(SERVER_BUILD, SERVER_BIN)
    if not os.path.isfile(binary):
        log_fail("backend start", f"binary missing: {binary}")
        return False
    backend_proc, _ = _start_and_wait_bind(
        "backend start", [binary], SERVER_BUILD,
        SERVER_READY_TIMEOUT, wrap_with_valgrind=False)
    return backend_proc is not None


def start_gateway():
    global gateway_proc
    binary = os.path.join(GATEWAY_BUILD, GATEWAY_BIN)
    if not os.path.isfile(binary):
        log_fail("gateway start", f"binary missing: {binary}")
        return False
    gateway_proc, _ = _start_and_wait_bind(
        "gateway start", [binary], GATEWAY_BUILD,
        SERVER_READY_TIMEOUT, wrap_with_valgrind=True)
    return gateway_proc is not None


def _kill(proc):
    if proc is None:
        return
    try:
        os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
    except ProcessLookupError:
        return
    try:
        proc.wait(timeout=15)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
        except ProcessLookupError:
            pass
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            pass


def stop_backend():
    global backend_proc
    if backend_proc is None:
        return
    _kill(backend_proc)
    backend_proc = None


def stop_gateway(label):
    """Stop gateway and check valgrind exit code. Valgrind exits 23 when
    it detected an error (per --error-exitcode above), which we treat as
    a failure. SIGTERM produces a 0 exit when no leaks are found."""
    global gateway_proc
    if gateway_proc is None:
        return True
    _kill(gateway_proc)
    rc = gateway_proc.returncode
    gateway_proc = None
    if rc == 23:
        log_fail(f"{label}: valgrind",
                 "valgrind reported memory errors (exit 23)")
        return False
    log_pass(f"{label}: valgrind", f"clean (rc={rc})")
    return True


# ── client ──────────────────────────────────────────────────────────────────
def run_client(label):
    binary = os.path.join(CLIENT_BUILD, CLIENT_BIN)
    if not os.path.isfile(binary):
        log_fail(label, f"binary missing: {binary}")
        return False
    args = ["--host", BACKEND_HOST, "--port", str(GATEWAY_PORT),
            "--autologin", "--character", "Player", "--closewhenonmap"]
    if not assert_port_or_fail(BACKEND_HOST, GATEWAY_PORT, log_fail, label):
        return False
    log_info(f"client: {CLIENT_BIN} {' '.join(args)}")
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"
    timeout = clamp_local(CLIENT_TIMEOUT * 10)  # gateway under valgrind is slow
    cmd = NICE_PREFIX + [binary] + args
    diagnostic.record_cmd(cmd, CLIENT_BUILD)
    proc = subprocess.Popen(
        cmd, cwd=CLIENT_BUILD,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env,
        preexec_fn=os.setsid)
    output = []
    done = threading.Event()
    outcome = [None]
    seen_state1 = [False]

    def reader():
        for raw in iter(proc.stdout.readline, b""):
            line = raw.decode(errors="replace").rstrip("\n")
            output.append(line)
            if "MapVisualiserPlayer::mapDisplayedSlot()" in line:
                outcome[0] = ("pass", "reached map")
                done.set()
                return
            if "Connection refused by the server" in line:
                outcome[0] = ("fail", "Connection refused")
                done.set()
                return
            if "stateChanged(1)" in line:
                seen_state1[0] = True
            elif seen_state1[0] and "stateChanged(0)" in line:
                outcome[0] = ("fail",
                              "stateChanged(0) after stateChanged(1) — "
                              "client connected then dropped before map")
                done.set()
                return
        done.set()

    threading.Thread(target=reader, daemon=True).start()
    triggered = done.wait(timeout=timeout)
    if not triggered:
        _kill(proc)
        log_fail(label, f"timeout after {timeout}s")
        for l in output[-20:]:
            print(f"  | {l}")
        return False
    _kill(proc)
    if outcome[0] is not None:
        kind, detail = outcome[0]
        if kind == "pass":
            log_pass(label, detail)
            return True
        log_fail(label, detail)
        for l in output[-20:]:
            print(f"  | {l}")
        return False
    log_fail(label, "client exited without reaching map")
    for l in output[-20:]:
        print(f"  | {l}")
    return False


# ── enumeration ─────────────────────────────────────────────────────────────
_NAME_RE = re.compile(r"^[a-z0-9_-]+$")


def enumerate_main_sub(datapack):
    """Yield (maincode, subcode) pairs from <datapack>/map/main/<mc>/[sub/<sc>/].
    `subcode=""` covers the "no subcode" case for every maincode."""
    main_dir = os.path.join(datapack, "map", "main")
    if not os.path.isdir(main_dir):
        return []
    out = []
    mi = 0
    mains = sorted(os.listdir(main_dir))
    while mi < len(mains):
        mc = mains[mi]
        mi += 1
        mc_path = os.path.join(main_dir, mc)
        if not os.path.isdir(mc_path) or not _NAME_RE.match(mc):
            continue
        out.append((mc, ""))
        sub_dir = os.path.join(mc_path, "sub")
        if not os.path.isdir(sub_dir):
            continue
        si = 0
        subs = sorted(os.listdir(sub_dir))
        while si < len(subs):
            sc = subs[si]
            si += 1
            if (os.path.isdir(os.path.join(sub_dir, sc))
                    and _NAME_RE.match(sc)):
                out.append((mc, sc))
    return out


# ── cleanup ─────────────────────────────────────────────────────────────────
def cleanup(*_args):
    stop_gateway("cleanup")
    stop_backend()
    stop_nginx()
    sys.exit(1)


signal.signal(signal.SIGINT, cleanup)
signal.signal(signal.SIGTERM, cleanup)


# ── main ────────────────────────────────────────────────────────────────────
def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — Gateway Testing (valgrind, c++11)")
    print(f"{'='*60}{C_RESET}\n")

    if not nginx_available():
        log_fail("nginx availability",
                 "nginx not on PATH; the gateway test requires nginx")
        summary()
        return

    failed_cases = load_failed_cases()
    if failed_cases is not None and len(failed_cases) == 0:
        log_info("all previously passed (delete failed.json to re-run)")
        return

    # 1. Compile the three binaries with c++11.
    if not build_project(SERVER_FILEDB_PRO, SERVER_BUILD, "server-filedb"):
        summary()
        return
    if not build_project(GATEWAY_PRO, GATEWAY_BUILD, "gateway"):
        summary()
        return
    if not build_project(CLIENT_CPU_PRO, CLIENT_BUILD, "qtcpu800x600"):
        summary()
        return

    # 2. Build the test plan: for every datapack × maincode × subcode ×
    #    (dest_http, gateway_http) ∈ {0,1}^2.
    plan = []
    di = 0
    while di < len(DATAPACKS):
        dp = DATAPACKS[di]
        di += 1
        if not os.path.isdir(dp):
            log_fail(f"datapack {os.path.basename(dp)}",
                     f"not found: {dp}")
            continue
        combos = enumerate_main_sub(dp)
        if not combos:
            log_fail(f"datapack {os.path.basename(dp)}",
                     "no maincode found under map/main/")
            continue
        ci = 0
        while ci < len(combos):
            mc, sc = combos[ci]
            ci += 1
            for dest_http in (0, 1):
                for gw_http in (0, 1):
                    plan.append((dp, mc, sc, dest_http, gw_http))

    if not plan:
        summary()
        return

    log_info(f"plan: {len(plan)} run(s)")
    # 3 sub-checks per run (backend start, gateway start+valgrind, client),
    # plus the upfront 3 builds.
    total_expected[0] = 3 + len(plan) * 4

    # 3. Execute every (datapack, mc, sc, dest_http, gw_http).
    pi = 0
    while pi < len(plan):
        dp, mc, sc, dest_http, gw_http = plan[pi]
        pi += 1
        dp_name = os.path.basename(dp)
        case = (f"{dp_name}/{mc}"
                + (f"/{sc}" if sc else "")
                + f" dest_http={dest_http} gw_http={gw_http}")
        if not should_run(case, failed_cases):
            continue

        print(f"\n{C_CYAN}--- case: {case} ---{C_RESET}")

        # Stage datapack into backend's build/datapack.
        stage_backend_datapack(dp)
        reset_backend_state()

        # nginx www = symlink to the active datapack.
        stage_nginx_www(os.path.join(SERVER_BUILD, "datapack"))
        if not start_nginx():
            continue

        # Build the http URL pointing at our nginx instance. The path
        # /datapack/ matches the symlink we staged above, which is what
        # the client appends ?file=<…>& to when fetching mirror entries.
        nginx_base = f"http://127.0.0.1:{NGINX_PORT}/datapack/"
        unreachable = "http://127.0.0.1:1/disabled/"

        backend_http_url = nginx_base if dest_http else ""
        if gw_http:
            gw_rewrite_base = nginx_base
            gw_rewrite_main = nginx_base
        else:
            # Gateway aborts if rewrite is empty — provide an unreachable
            # placeholder so the client falls back to in-protocol download.
            gw_rewrite_base = unreachable
            gw_rewrite_main = unreachable

        write_backend_settings(mc, sc, backend_http_url)
        write_gateway_settings(gw_rewrite_base, gw_rewrite_main)

        # Start backend, then gateway (under valgrind), then client.
        if not start_backend():
            stop_nginx()
            continue
        if not start_gateway():
            stop_backend()
            stop_nginx()
            continue
        run_client(f"{case}: client→map")
        # Stop gateway first so valgrind's report appears in the FAIL
        # context window for THIS case, not the next one.
        stop_gateway(case)
        stop_backend()
        stop_nginx()

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
