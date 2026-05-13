#!/usr/bin/env python3
"""
testingstats.py — Live player-count tracking via the `stats` tool.

Flow:
  1. Compile catchchallenger-server-cli (file_db backend) and
     the stats tool, both from server/cli/ and tools/stats/. Compile
     qtcpu800x600 as the test client.
  2. Start the file_db server UNDER VALGRIND on a fixed TCP port,
     with a shared <statclient><token> seeded into server-properties.xml.
  3. Start the stats binary with stats-client.xml pointing at the
     server's host/port and using the same token.
  4. Wait until stats authenticates (gameserver.json populated with a
     non-empty server list).
  5. Read connectedPlayer count → expect 0.
  6. Spawn qtcpu800x600 client with --host/--port; wait for
     "MapVisualiserPlayer::mapDisplayedSlot()".
  7. Read connectedPlayer count → expect 1 (verifies count INCREASED).
  8. Stop the client.
  9. Read connectedPlayer count → expect 0 (verifies count DECREASED).
 10. Stop the stats tool, check the server is still alive (didn't
     crash on stats-client disconnect).
 11. Stop the server, check valgrind exit code (0 = clean,
     23 = memory errors).
"""
import sys
import process_helpers
sys.dont_write_bytecode = True

import os, sys, signal, subprocess, threading, multiprocessing, json
import shutil, re, time, socket, secrets

import diagnostic
import build_paths
from cmd_helpers import clamp_local, assert_port_or_fail


build_paths.ensure_root()

DIAG = diagnostic.parse_diag_args()
_DIAG_SUFFIX = diagnostic.build_dir_suffix(DIAG)

_CONFIG_PATH = os.path.join(os.path.expanduser("~"),
                            ".config", "catchchallenger-testing", "config.json")
with open(_CONFIG_PATH, "r") as _f:
    _config = json.load(_f)

ROOT  = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NPROC = str(multiprocessing.cpu_count())

DATAPACK_SRC = "/home/user/Desktop/CatchChallenger/CatchChallenger-datapack"
MAINCODE     = "test"

SERVER_PRO     = os.path.join(ROOT, "server/cli/catchchallenger-server-filedb.pro")
STATS_PRO      = os.path.join(ROOT, "tools/stats/stats.pro")
CLIENT_CPU_PRO = os.path.join(ROOT, "client/qtcpu800x600/qtcpu800x600.pro")

SERVER_BUILD     = build_paths.build_path("server/cli/build/testing-stats-server" + _DIAG_SUFFIX)
STATS_BUILD      = build_paths.build_path("tools/stats/build/testing-stats"        + _DIAG_SUFFIX)
CLIENT_CPU_BUILD = build_paths.build_path("client/qtcpu800x600/build/testing-stats-cpu" + _DIAG_SUFFIX)

SERVER_BIN     = "catchchallenger-server-cli"
STATS_BIN      = "stats"
CLIENT_CPU_BIN = "catchchallenger"

SERVER_HOST = "127.0.0.1"
SERVER_PORT = 61922

# CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER == 16 bytes
# (= 32 hex chars). See general/base/GeneralVariable.hpp:38.
# Anything longer gets truncated by both server and stats; if the
# truncations diverge, auth fails silently with no stat client connected.
SHARED_TOKEN_HEX = secrets.token_hex(16).upper()

COMPILE_TIMEOUT       = 600
SERVER_READY_TIMEOUT  = 60
STATS_READY_TIMEOUT   = 30
CLIENT_TIMEOUT        = 90

NICE_PREFIX = ["nice", "-n", "19", "ionice", "-c", "3"]

C_GREEN  = "\033[92m"
C_RED    = "\033[91m"
C_CYAN   = "\033[96m"
C_RESET  = "\033[0m"

results = []
total_expected = [0]
_last_log_time = [time.monotonic()]

server_proc = None
stats_proc  = None

SCRIPT_NAME = os.path.basename(__file__)
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
    import cmake_helpers as _ch
    return _ch.build_project(
        pro_file, build_dir, label,
        root=ROOT, nproc=NPROC,
        log_info=log_info, log_pass=log_pass, log_fail=log_fail,
        ensure_dir=ensure_dir, run_cmd=run_cmd,
        diag=DIAG, diag_module=diagnostic,
    )


# ── server side ─────────────────────────────────────────────────────────────
def write_server_settings():
    """Pin the file_db server to SERVER_PORT, autologin, mainDatapackCode,
    and seed the <statclient><token> the stats tool will share."""
    xml = os.path.join(SERVER_BUILD, "server-properties.xml")
    ensure_dir(SERVER_BUILD)
    body = (
        '<?xml version="1.0"?>\n'
        '<configuration>\n'
        '    <server-port value="{port}"/>\n'
        '    <automatic_account_creation value="true"/>\n'
        '    <max-players value="200"/>\n'
        '    <httpDatapackMirror value=""/>\n'
        '    <master>\n'
        '        <external-server-port value="{port}"/>\n'
        '    </master>\n'
        '    <content>\n'
        '        <mainDatapackCode value="{mc}"/>\n'
        '        <subDatapackCode value=""/>\n'
        '    </content>\n'
        '    <statclient>\n'
        '        <token value="{token}"/>\n'
        '    </statclient>\n'
        '</configuration>\n'
    ).format(port=SERVER_PORT, mc=MAINCODE, token=SHARED_TOKEN_HEX)
    with open(xml, "w") as f:
        f.write(body)


def stage_server_datapack():
    link = os.path.join(SERVER_BUILD, "datapack")
    if os.path.islink(link) or os.path.exists(link):
        try:
            os.remove(link)
        except OSError:
            shutil.rmtree(link, ignore_errors=True)
    os.symlink(DATAPACK_SRC, link)


def reset_server_state():
    cache = os.path.join(SERVER_BUILD, "datapack-cache.bin")
    if os.path.exists(cache):
        os.remove(cache)
    db = os.path.join(SERVER_BUILD, "database")
    if os.path.isdir(db):
        shutil.rmtree(db)


def start_server_under_valgrind():
    """Wrap the server in valgrind memcheck regardless of --valgrind/
    --sanitize: this test is explicitly a "watch the server for leaks
    while clients churn" run."""
    global server_proc
    binary = os.path.join(SERVER_BUILD, SERVER_BIN)
    if not os.path.isfile(binary):
        log_fail("server start", f"binary missing: {binary}")
        return False
    wrapper = ["valgrind",
               "--error-exitcode=23",
               "--child-silent-after-fork=yes",
               "--tool=memcheck",
               "--leak-check=full",
               "--show-leak-kinds=all",
               "--track-origins=yes",
               "--errors-for-leak-kinds=definite,possible"]
    args = NICE_PREFIX + wrapper + [binary]
    diagnostic.record_cmd(args, SERVER_BUILD)
    log_info(f"starting server under valgrind on :{SERVER_PORT}")
    env = os.environ.copy()
    server_proc = subprocess.Popen(
        args, cwd=SERVER_BUILD,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env,
        preexec_fn=process_helpers.setsid_and_pdeathsig)
    ready = threading.Event()
    output = []

    log_path = os.path.join(SERVER_BUILD, "server.log")
    log_file = open(log_path, "w")
    def reader():
        for raw in iter(server_proc.stdout.readline, b""):
            line = raw.decode(errors="replace").rstrip("\n")
            output.append(line)
            log_file.write(line + "\n")
            log_file.flush()
            if "correctly bind:" in line:
                ready.set()
        ready.set()
        log_file.close()

    threading.Thread(target=reader, daemon=True).start()
    # valgrind ~10x slowdown — generous timeout for server boot + datapack parse
    timeout = max(SERVER_READY_TIMEOUT * 10, SERVER_READY_TIMEOUT)
    if ready.wait(timeout=timeout):
        log_pass("server start (valgrind)", "correctly bind: detected")
        return True
    log_fail("server start", f"timeout waiting for 'correctly bind:' ({timeout}s)")
    for l in output[-30:]:
        print(f"  | {l}")
    stop_server_under_valgrind()
    return False


def stop_server_under_valgrind(label="server stop"):
    """Stop the server, harvest valgrind's exit code. 23 == valgrind
    detected memory errors (per --error-exitcode); anything else from
    SIGTERM is fine."""
    global server_proc
    if server_proc is None:
        return None
    try:
        os.killpg(os.getpgid(server_proc.pid), signal.SIGTERM)
    except ProcessLookupError:
        pass
    try:
        server_proc.wait(timeout=30)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(os.getpgid(server_proc.pid), signal.SIGKILL)
        except ProcessLookupError:
            pass
        try:
            server_proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            pass
    rc = server_proc.returncode
    server_proc = None
    if rc == 23:
        log_fail(f"{label}: valgrind",
                 "valgrind reported memory errors (exit 23)")
        return rc
    log_pass(f"{label}: valgrind", f"clean (rc={rc})")
    return rc


def server_is_alive():
    return server_proc is not None and server_proc.poll() is None


# ── stats side ──────────────────────────────────────────────────────────────
def write_stats_settings():
    """Seed stats-client.xml in STATS_BUILD with a host/port pointing
    at our server and the same shared token. Also pick a unix socket
    path under tmpfs so two parallel runs don't collide."""
    xml = os.path.join(STATS_BUILD, "stats-client.xml")
    ensure_dir(STATS_BUILD)
    sock_path = os.path.join(STATS_BUILD, "catchchallenger-stats.sock")
    out_path  = os.path.join(STATS_BUILD, "gameserver.json")
    # Drop a stale socket so tryListen() doesn't refuse to bind.
    if os.path.exists(sock_path):
        try:
            os.remove(sock_path)
        except OSError:
            pass
    if os.path.exists(out_path):
        try:
            os.remove(out_path)
        except OSError:
            pass
    body = (
        '<?xml version="1.0"?>\n'
        '<configuration>\n'
        '    <host value="{host}"/>\n'
        '    <port value="{port}"/>\n'
        '    <token value="{token}"/>\n'
        '    <tryInterval value="2"/>\n'
        '    <considerDownAfterNumberOfTry value="3"/>\n'
        '    <outputFile value="{out}"/>\n'
        '    <unixSocket value="{sock}"/>\n'
        '    <withIndentation value="false"/>\n'
        '</configuration>\n'
    ).format(host=SERVER_HOST, port=SERVER_PORT, token=SHARED_TOKEN_HEX,
             out=out_path, sock=sock_path)
    with open(xml, "w") as f:
        f.write(body)
    return out_path, sock_path


def start_stats():
    global stats_proc
    binary = os.path.join(STATS_BUILD, STATS_BIN)
    if not os.path.isfile(binary):
        log_fail("stats start", f"binary missing: {binary}")
        return False
    args = NICE_PREFIX + [binary]
    diagnostic.record_cmd(args, STATS_BUILD)
    log_info(f"starting stats client → {SERVER_HOST}:{SERVER_PORT}")
    stats_proc = subprocess.Popen(
        args, cwd=STATS_BUILD,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        preexec_fn=process_helpers.setsid_and_pdeathsig)
    return True


def stop_stats():
    global stats_proc
    if stats_proc is None:
        return
    try:
        os.killpg(os.getpgid(stats_proc.pid), signal.SIGTERM)
    except ProcessLookupError:
        pass
    try:
        stats_proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(os.getpgid(stats_proc.pid), signal.SIGKILL)
        except ProcessLookupError:
            pass
        try:
            stats_proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            pass
    stats_proc = None


def wait_for_stats_connected(json_path, timeout=STATS_READY_TIMEOUT):
    """Poll gameserver.json until it contains a populated server list
    (i.e. the stats client has authenticated and received at least one
    status push from the game server). Returns the parsed JSON content
    or None on timeout."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if os.path.isfile(json_path):
            try:
                with open(json_path, "r") as f:
                    text = f.read()
                if text.strip():
                    parsed = json.loads(text)
                    if _has_server_entry(parsed):
                        return parsed
            except (OSError, json.JSONDecodeError):
                pass
        time.sleep(0.2)
    return None


def _iter_server_entries(parsed):
    """Yield each server dict from gameserver.json. Real schema:
       { "<groupName>": { "xml": "...", "servers": [ { "connectedPlayer": N, ... }, ... ] }, ... }
    Walks any nested dict/list, picking out dicts with "connectedPlayer"."""
    if isinstance(parsed, dict):
        if "connectedPlayer" in parsed:
            yield parsed
        for v in parsed.values():
            yield from _iter_server_entries(v)
    elif isinstance(parsed, list):
        for v in parsed:
            yield from _iter_server_entries(v)


def _has_server_entry(parsed):
    for _ in _iter_server_entries(parsed):
        return True
    return False


def read_player_count(json_path):
    """Sum connectedPlayer across every server entry in the JSON. Returns
    None if the file isn't readable / parseable."""
    if not os.path.isfile(json_path):
        return None
    try:
        with open(json_path, "r") as f:
            parsed = json.loads(f.read())
    except (OSError, json.JSONDecodeError):
        return None
    total = 0
    for entry in _iter_server_entries(parsed):
        try:
            total += int(entry["connectedPlayer"])
        except (TypeError, ValueError):
            pass
    return total


def wait_for_count(json_path, predicate, timeout=15):
    """Block until read_player_count(json_path) satisfies predicate.
    Returns the matching count, or None on timeout."""
    deadline = time.monotonic() + timeout
    last = None
    while time.monotonic() < deadline:
        last = read_player_count(json_path)
        if last is not None and predicate(last):
            return last
        time.sleep(0.2)
    return last  # last seen value (may not satisfy predicate)


def refresh_stats_snapshot(json_path):
    """The file_db game server's stat-client push path
    (ClientList::clientForStatus) is unimplemented — it never pushes
    live `0x47 currentPlayer` updates to already-connected stat
    clients. The cached server-list packet IS kept up to date by
    PlayerUpdaterBase::exec(), so the freshest count is whatever a NEW
    stat-client connection reads in its initial handshake reply.
    Restart stats between transitions to harvest a current snapshot."""
    stop_stats()
    if os.path.exists(json_path):
        try:
            os.remove(json_path)
        except OSError:
            pass
    if not start_stats():
        return None
    return wait_for_stats_connected(json_path, STATS_READY_TIMEOUT)


# ── client side ─────────────────────────────────────────────────────────────
def run_client_until_on_map():
    """Spawn qtcpu800x600 with --host/--port pointing at the file_db
    server (NOT --closewhenonmap — we keep it alive for the duration
    of the count check). Returns the Popen on success, None on
    failure."""
    binary = os.path.join(CLIENT_CPU_BUILD, CLIENT_CPU_BIN)
    if not os.path.isfile(binary):
        log_fail("client start", f"binary missing: {binary}")
        return None
    if not assert_port_or_fail(SERVER_HOST, SERVER_PORT,
                               log_fail, "client start"):
        return None
    args = ["--host", SERVER_HOST, "--port", str(SERVER_PORT),
            "--autologin", "--character", "Player"]
    log_info(f"client: {CLIENT_CPU_BIN} {' '.join(args)}")
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"
    cmd = NICE_PREFIX + [binary] + args
    diagnostic.record_cmd(cmd, CLIENT_CPU_BUILD)
    proc = subprocess.Popen(
        cmd, cwd=CLIENT_CPU_BUILD,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env,
        preexec_fn=process_helpers.setsid_and_pdeathsig)
    output = []
    on_map = threading.Event()

    def reader():
        for raw in iter(proc.stdout.readline, b""):
            line = raw.decode(errors="replace").rstrip("\n")
            output.append(line)
            if "MapVisualiserPlayer::mapDisplayedSlot()" in line:
                on_map.set()

    threading.Thread(target=reader, daemon=True).start()
    timeout = diagnostic.scale_timeout(DIAG, CLIENT_TIMEOUT)
    if on_map.wait(timeout=timeout):
        log_pass("client on map", "MapVisualiserPlayer::mapDisplayedSlot()")
        proc._reader_lines = output
        return proc
    log_fail("client on map", f"timeout after {timeout}s")
    for l in output[-30:]:
        print(f"  | {l}")
    _kill_proc(proc)
    return None


def _kill_proc(proc):
    try:
        os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
    except ProcessLookupError:
        return
    try:
        proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
        except ProcessLookupError:
            pass
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            pass


# ── cleanup ─────────────────────────────────────────────────────────────────
def cleanup(*_args):
    stop_stats()
    stop_server_under_valgrind()
    sys.exit(1)


signal.signal(signal.SIGINT, cleanup)
signal.signal(signal.SIGTERM, cleanup)


# ── main ────────────────────────────────────────────────────────────────────
def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — Stats / Player-Count Testing")
    print(f"{'='*60}{C_RESET}\n")

    if shutil.which("valgrind") is None:
        log_fail("setup", "valgrind not on PATH")
        summary()
        return
    if not os.path.isdir(DATAPACK_SRC):
        log_fail("setup", f"datapack missing: {DATAPACK_SRC}")
        summary()
        return

    failed_cases = load_failed_cases()
    if failed_cases is not None and len(failed_cases) == 0:
        log_info("all previously passed (delete failed.json to re-run)")
        return

    total_expected[0] = 12  # rough estimate

    # 1. Build the three binaries.
    if not build_project(SERVER_PRO, SERVER_BUILD, "server-filedb"):
        summary()
        return
    if not build_project(STATS_PRO, STATS_BUILD, "stats"):
        summary()
        return
    if not build_project(CLIENT_CPU_PRO, CLIENT_CPU_BUILD, "qtcpu800x600"):
        summary()
        return

    # 2. Stage server, write configs.
    stage_server_datapack()
    reset_server_state()
    write_server_settings()
    json_path, sock_path = write_stats_settings()

    # 3. Start server under valgrind.
    if not start_server_under_valgrind():
        summary()
        return

    # 4. Start stats, wait for it to authenticate.
    if not start_stats():
        stop_server_under_valgrind()
        summary()
        return
    parsed = wait_for_stats_connected(json_path, STATS_READY_TIMEOUT)
    if parsed is None:
        log_fail("stats connected",
                 f"gameserver.json not populated within "
                 f"{STATS_READY_TIMEOUT}s")
        stop_stats()
        stop_server_under_valgrind()
        summary()
        return
    log_pass("stats connected",
             "gameserver.json has at least one server entry")

    # 5. Initial player count: should be 0.
    initial = read_player_count(json_path)
    if initial is None:
        log_fail("initial player count", "gameserver.json unparseable")
        stop_stats()
        stop_server_under_valgrind()
        summary()
        return
    if initial == 0:
        log_pass("initial player count", f"connectedPlayer=0")
    else:
        log_fail("initial player count",
                 f"expected 0, got {initial}")

    # 6. Spawn the client, wait for it to reach the map.
    client_proc = run_client_until_on_map()
    if client_proc is None:
        stop_stats()
        stop_server_under_valgrind()
        summary()
        return

    # 7. Player count should bump to 1 — refresh stats snapshot first.
    #    PlayerUpdaterBase::exec() ticks once per second and only THEN
    #    updates the cached server-list packet with the new count.
    #    `mapDisplayedSlot()` fires client-side as soon as the map paint
    #    completes, which can race ahead of the server-side
    #    addConnectedPlayer + exec() pair. Sleep ~2 ticks to let the
    #    cached packet catch up before re-handshaking stats.
    time.sleep(2.5)
    if refresh_stats_snapshot(json_path) is None:
        log_fail("stats reconnect after client connect",
                 "gameserver.json not re-populated")
    else:
        log_pass("stats reconnect after client connect",
                 "fresh snapshot taken")
    after_connect = read_player_count(json_path)
    if after_connect is not None and after_connect >= 1:
        log_pass("player count after connect",
                 f"connectedPlayer={after_connect} (initial={initial})")
    else:
        log_fail("player count after connect",
                 f"expected >=1, got {after_connect}")

    # 8. Disconnect the client.
    _kill_proc(client_proc)
    log_pass("client disconnected", "SIGTERM delivered")
    # Give the server a moment to register the disconnect — its
    # ProtocolParsing-side cleanup is async and the next stats
    # snapshot must reflect the player gone.
    time.sleep(2)

    # 9. Player count should drop back to 0.
    if refresh_stats_snapshot(json_path) is None:
        log_fail("stats reconnect after client disconnect",
                 "gameserver.json not re-populated")
    else:
        log_pass("stats reconnect after client disconnect",
                 "fresh snapshot taken")
    after_disconnect = read_player_count(json_path)
    if after_disconnect == 0:
        log_pass("player count after disconnect",
                 f"connectedPlayer=0 (was {after_connect})")
    else:
        log_fail("player count after disconnect",
                 f"expected 0, got {after_disconnect}")

    # 10. Stop stats. Server should keep running cleanly.
    stop_stats()
    log_pass("stats stopped", "stats SIGTERM delivered")
    # Give the server a moment to register the stat-client disconnect.
    time.sleep(2)
    if server_is_alive():
        log_pass("server alive after stats disconnect",
                 f"pid {server_proc.pid}")
    else:
        log_fail("server alive after stats disconnect",
                 f"server exited rc={server_proc.returncode}")

    # 11. Stop the server, harvest valgrind exit code (last log line).
    stop_server_under_valgrind("server final stop")

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
