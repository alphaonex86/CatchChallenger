#!/usr/bin/env python3
"""
testingbroadcast.py — CatchChallenger LAN "here I am" UDP announce tests.

Exercises the server-side LAN-discovery broadcast end-to-end on the wire:

  server/cli/timer/TimerBroadcastAnnounce.cpp
      builds the Qt_4_4 datagram by hand (no Qt) — quint32 byteCount +
      name as UTF-16BE + quint16 game-port — and sendto()s it to
      255.255.255.255:42490 once per 1s tick, from ONE socket, ONLY when
      <broadcastName> is non-empty.

The matching client listener is client/libqtcatchchallenger/LanBroadcastWatcher.cpp;
it deduplicates by (senderAddress, advertised-port). That dedup is correct:
a single server announces from exactly ONE source address, so it collapses
to ONE row. Two rows for the same name therefore mean two genuinely distinct
servers that happen to share a <broadcastName> (e.g. several boards left on the
default "CatchChallenger ESP32"), NOT a client bug. These tests pin the
server-side contract the dedup relies on:

  1. announce-present            — enabled server actually broadcasts.
  2. single-source-no-duplicate  — ONE server == ONE (srcIP,port) on the wire
                                    (not "multiple broadcast from multiple udp
                                    sockets"); this is what lets the client show
                                    one row per server.
  3. advertised-payload-correct  — the datagram decodes to our name + game port.
  4. rate-one-per-tick           — ~1 datagram/sec, not a per-tick flood.
  5. disabled-silent             — empty <broadcastName> => NO datagram, and no
                                    "LAN announce enabled" log line.

We observe the binary EXTERNALLY (a UDP socket on 42490), never adding any
test hook to the server — per test/CLAUDE.md. The host LAN usually carries
OTHER CatchChallenger broadcasters (leftover test servers, other boxes), so
every assertion FILTERS to this run's unique broadcast name and game port.
"""
import sys
sys.dont_write_bytecode = True

import os
import socket
import struct
import signal
import subprocess
import time
import json
import resource
import shutil

import diagnostic
import wall_cap
wall_cap.arm()
import build_paths
import cleanup_helpers
import datapack_stage
import process_helpers
from cmd_helpers import clamp_local

build_paths.ensure_root()

DIAG = diagnostic.parse_diag_args()
_DIAG_SUFFIX = diagnostic.build_dir_suffix(DIAG)

ROOT  = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NPROC = str(os.cpu_count() or 1)

# ── config (datapack source) ────────────────────────────────────────────────
_CONFIG_PATH = os.path.join(os.path.expanduser("~"),
                            ".config", "catchchallenger-testing", "config.json")
try:
    with open(_CONFIG_PATH, "r") as _f:
        _config = json.load(_f)
except (OSError, ValueError):
    _config = {}
DATAPACKS = _config.get("paths", {}).get("datapacks", [])

SERVER_PRO  = os.path.join(ROOT, "server/cli/catchchallenger-server-cli.pro")
BUILD_DIR   = build_paths.build_path("server/cli/build/testing-broadcast" + _DIAG_SUFFIX)
cleanup_helpers.register_build_dir(BUILD_DIR)
BIN_NAME    = "catchchallenger-server-cli"

# The UDP port the client LAN watcher binds to (must match the C++ constant
# CC_LAN_BROADCAST_PORT / LanBroadcastWatcher::bind(42490)).
LAN_PORT    = 42490
# This run's UNIQUE identity, used to filter our datagrams out of any other
# CatchChallenger broadcasters live on the same LAN. ASCII only (the server
# hand-encodes the name one byte -> one UTF-16 unit, i.e. Latin-1).
UNIQUE_NAME = f"CCBcastTest-{os.getpid()}"
GAME_PORT   = 42531

COMPILE_TIMEOUT = 300
BIND_TIMEOUT    = 25      # generous: datapack load + bind on a slow/busy host
SNIFF_SECONDS   = 4.0     # ~1 announce/sec => expect ~4 datagrams

NICE_PREFIX         = ["nice", "-n", "19", "ionice", "-c", "3"]
NICE_PREFIX_RUNTIME = []

C_GREEN = "\033[92m"; C_RED = "\033[91m"; C_CYAN = "\033[96m"; C_RESET = "\033[0m"

results = []
total_expected = [0]
_last_log_time = [time.monotonic()]

SCRIPT_NAME = os.path.basename(__file__)
import failed_cases as _fc
import phase_timer

# Wall cap kept 1:1 with all.sh PER_TEST_TIMEOUT_MAP[testingbroadcast.py].
_WALL_LIMIT_SEC = 900
import faulthandler
faulthandler.enable()
faulthandler.dump_traceback_later(_WALL_LIMIT_SEC + 10, exit=False)
try:
    faulthandler.register(signal.SIGUSR1)
except (AttributeError, RuntimeError):
    pass


def load_failed_cases():
    return _fc.load_names(SCRIPT_NAME)


def should_run(test_name, failed_cases):
    return failed_cases is None or test_name in failed_cases


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
    now = time.monotonic(); elapsed = now - _last_log_time[0]; _last_log_time[0] = now
    results.append((name, True, detail, elapsed))
    if len(results) > total_expected[0]:
        total_expected[0] = len(results)
    print(f"{phase_timer.t()} {C_GREEN}[PASS]{C_RESET} {len(results)}/{total_expected[0]} "
          f"{name}  {detail}  ({elapsed:.1f}s)")
    phase_timer.record_event("pass", name, ok=True, dt=elapsed, detail=detail)


def log_fail(name, detail=""):
    now = time.monotonic(); elapsed = now - _last_log_time[0]; _last_log_time[0] = now
    results.append((name, False, detail, elapsed))
    if len(results) > total_expected[0]:
        total_expected[0] = len(results)
    print(f"{phase_timer.t()} {C_RED}[FAIL]{C_RESET} {len(results)}/{total_expected[0]} "
          f"{name}  {detail}  ({elapsed:.1f}s)")
    phase_timer.record_event("fail", name, ok=False, dt=elapsed, detail=detail)


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


def build_server():
    import cmake_helpers as _ch
    return _ch.build_project(
        SERVER_PRO, BUILD_DIR, BIN_NAME,
        root=ROOT, nproc=NPROC,
        log_info=log_info, log_pass=log_pass, log_fail=log_fail,
        ensure_dir=ensure_dir, run_cmd=run_cmd,
        diag=DIAG, diag_module=diagnostic,
    )


# ── server-properties.xml — partial; NormalServerGlobal fills the rest ───────
def write_server_properties(path, maincode, broadcast_name):
    """Pin only the keys this test needs. An EMPTY/absent <broadcastName>
    disables the announce timer (the disabled-case under test)."""
    bcast = ""
    if broadcast_name:
        bcast = f'    <broadcastName value="{broadcast_name}"/>\n'
    with open(path, "w") as f:
        f.write('<?xml version="1.0"?>\n<configuration>\n'
                f'    <server-port value="{GAME_PORT}"/>\n'
                '    <server-ip value=""/>\n'
                '    <automatic_account_creation value="true"/>\n'
                '    <max-players value="10"/>\n'
                + bcast +
                '    <content>\n'
                f'        <mainDatapackCode value="{maincode}"/>\n'
                '    </content>\n'
                '    <mapVisibility>\n'
                '        <minimize value="cpu"/>\n'
                '    </mapVisibility>\n'
                '</configuration>\n')


def pick_maincode(staged_datapack):
    map_main = os.path.join(staged_datapack, "map", "main")
    if os.path.isdir(map_main):
        codes = sorted(d for d in os.listdir(map_main)
                       if os.path.isdir(os.path.join(map_main, d)))
        if codes:
            return codes[0]
    return "test"


def _apply_child_rlimits():
    """preexec_fn for the server child. setsid + PR_SET_PDEATHSIG so the kernel
    reaps the server if this harness dies (incl. SIGKILL from all.sh's wall-cap
    timeout) — otherwise the server lingers and keeps LAN-broadcasting (exactly
    the orphan we found). Plus 15 min CPU + 128 MiB AS (per test/CLAUDE.md) to
    catch a wedged announce loop / runaway alloc; skipped under valgrind, which
    inflates both far past these caps on a legitimate workload."""
    process_helpers.setsid_and_pdeathsig()
    try:
        if not diagnostic.is_valgrind(DIAG):
            resource.setrlimit(resource.RLIMIT_CPU, (15 * 60, 15 * 60 + 60))
            resource.setrlimit(resource.RLIMIT_AS, (128 * 1024 * 1024,) * 2)
    except Exception as exc:
        try:
            os.write(2, f"[testingbroadcast] preexec rlimit failed: {exc!r}\n".encode())
        except Exception:
            pass


# ── wire decode (mirror of TimerBroadcastAnnounce's hand-built datagram) ─────
def parse_datagram(data):
    """Return (name, port) or None. Qt_4_4 QString = quint32 byteCount(=2*len)
    + UTF-16BE chars; then quint16 BE port."""
    if len(data) < 6:
        return None
    byte_count = struct.unpack(">I", data[:4])[0]
    if byte_count == 0xFFFFFFFF:        # Qt null string
        name = ""
        off = 4
    else:
        if byte_count % 2 != 0 or len(data) < 4 + byte_count + 2:
            return None
        try:
            name = data[4:4 + byte_count].decode("utf-16-be")
        except UnicodeDecodeError:
            return None
        off = 4 + byte_count
    port = struct.unpack(">H", data[off:off + 2])[0]
    return name, port


def open_sniffer():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
    except (AttributeError, OSError):
        pass
    s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    s.bind(("", LAN_PORT))   # all interfaces; local 255.255.255.255 loops back
    s.settimeout(0.5)
    return s


def sniff(sock, duration, name_filter=None, port_filter=None):
    """Aggregate matching datagrams into {(srcIP, name, port): count}."""
    agg = {}
    end = time.monotonic() + duration
    while time.monotonic() < end:
        try:
            data, addr = sock.recvfrom(4096)
        except socket.timeout:
            continue
        parsed = parse_datagram(data)
        if parsed is None:
            continue
        name, port = parsed
        if name_filter is not None and name != name_filter:
            continue
        if port_filter is not None and port != port_filter:
            continue
        key = (addr[0], name, port)
        agg[key] = agg.get(key, 0) + 1
    return agg


def launch_server(work, broadcast_name, staged_datapack):
    """Stage a run dir, launch the server, wait until it binds.
    Returns (proc, logpath, log_text) or (None, logpath, log_text)."""
    if os.path.isdir(work):
        shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work, exist_ok=True)
    shutil.copy2(os.path.join(BUILD_DIR, BIN_NAME), os.path.join(work, BIN_NAME))
    link = os.path.join(work, "datapack")
    if os.path.islink(link) or os.path.exists(link):
        os.remove(link)
    os.symlink(staged_datapack, link)
    write_server_properties(os.path.join(work, "server-properties.xml"),
                            pick_maincode(staged_datapack), broadcast_name)
    logpath = os.path.join(work, "server.log")
    cmd = NICE_PREFIX_RUNTIME + diagnostic.runtime_wrapper(DIAG) \
        + [os.path.join(work, BIN_NAME)]
    diagnostic.record_cmd(cmd, work)
    lf = open(logpath, "wb")
    proc = subprocess.Popen(cmd, cwd=work, stdout=lf, stderr=subprocess.STDOUT,
                            preexec_fn=_apply_child_rlimits)
    deadline = time.monotonic() + clamp_local(BIND_TIMEOUT)
    bound = False
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            break
        try:
            with open(logpath, "rb") as f:
                if b"correctly bind" in f.read():
                    bound = True
                    break
        except OSError:
            pass
        time.sleep(0.2)
    try:
        with open(logpath, "rb") as f:
            log_text = f.read().decode(errors="replace")
    except OSError:
        log_text = ""
    if not bound:
        kill_server(proc)
        return None, logpath, log_text
    return proc, logpath, log_text


def kill_server(proc):
    if proc is None:
        return
    try:
        proc.terminate()
        proc.wait(timeout=5)
    except Exception:
        try:
            proc.kill()
            proc.wait(timeout=5)
        except Exception:
            pass


# ── test: enabled => announces, exactly one source, correct payload, ~1/s ────
def test_enabled(staged_datapack):
    work = os.path.join(BUILD_DIR, "run-enabled")
    proc, logpath, log_text = launch_server(work, UNIQUE_NAME, staged_datapack)
    if proc is None:
        log_fail("announce-present", f"server failed to bind; see {logpath}")
        print(log_text[-1500:])
        return
    try:
        if "LAN announce enabled:" not in log_text:
            # log was captured at bind time; re-read in case the line is late
            try:
                with open(logpath, "rb") as f:
                    log_text = f.read().decode(errors="replace")
            except OSError:
                pass
        sock = open_sniffer()
        dur = diagnostic.scale_timeout(DIAG, SNIFF_SECONDS)
        log_info(f"sniffing UDP {LAN_PORT} for {dur:.0f}s (name={UNIQUE_NAME!r})")
        agg = sniff(sock, dur, name_filter=UNIQUE_NAME)
        sock.close()
    finally:
        kill_server(proc)

    total = sum(agg.values())
    sources = sorted(agg.keys())

    # 1. announce-present
    if total >= 2:
        log_pass("announce-present", f"{total} datagram(s) named {UNIQUE_NAME!r}")
    else:
        log_fail("announce-present",
                 f"expected >=2 datagrams, got {total}; 'LAN announce enabled:' "
                 f"{'present' if 'LAN announce enabled:' in log_text else 'MISSING'} in log")
        return

    # 2. single-source-no-duplicate — the heart of "not multiple broadcast from
    #    multiple udp sockets": one server must appear as exactly ONE (srcIP,port).
    if len(sources) == 1:
        log_pass("single-source-no-duplicate",
                 f"one (srcIP,port): {sources[0][0]}:{sources[0][2]}")
    else:
        log_fail("single-source-no-duplicate",
                 f"expected 1 (srcIP,name,port), got {len(sources)}: {sources}")

    # 3. advertised-payload-correct — decodes to our name + game port
    (_ip, gname, gport) = sources[0]
    if gname == UNIQUE_NAME and gport == GAME_PORT:
        log_pass("advertised-payload-correct", f"name={gname!r} port={gport}")
    else:
        log_fail("advertised-payload-correct",
                 f"name={gname!r} (want {UNIQUE_NAME!r}) port={gport} (want {GAME_PORT})")

    # 4. rate-one-per-tick — ~1/s, not a per-tick flood. Under valgrind the
    #    event-loop timer drifts, so only assert "not a flood" there.
    cap = int(dur) * 4 + 4 if diagnostic.is_valgrind(DIAG) else int(dur) + 2
    if total <= cap:
        log_pass("rate-one-per-tick", f"{total} datagrams over {dur:.0f}s (<= {cap})")
    else:
        log_fail("rate-one-per-tick",
                 f"{total} datagrams over {dur:.0f}s exceeds {cap} — emitting "
                 f"multiple per tick / from multiple sockets")


# ── test: disabled => no datagram, no announce log line ──────────────────────
def test_disabled(staged_datapack):
    work = os.path.join(BUILD_DIR, "run-disabled")
    proc, logpath, log_text = launch_server(work, "", staged_datapack)
    if proc is None:
        log_fail("disabled-silent", f"server failed to bind; see {logpath}")
        print(log_text[-1500:])
        return
    try:
        sock = open_sniffer()
        dur = diagnostic.scale_timeout(DIAG, SNIFF_SECONDS)
        log_info(f"sniffing UDP {LAN_PORT} for {dur:.0f}s (expect NO port {GAME_PORT})")
        agg = sniff(sock, dur, port_filter=GAME_PORT)
        sock.close()
        try:
            with open(logpath, "rb") as f:
                log_text = f.read().decode(errors="replace")
        except OSError:
            pass
    finally:
        kill_server(proc)

    total = sum(agg.values())
    announce_line = "LAN announce enabled:" in log_text
    if total == 0 and not announce_line:
        log_pass("disabled-silent",
                 f"no datagram for port {GAME_PORT}, no 'LAN announce enabled:' line")
    else:
        log_fail("disabled-silent",
                 f"broadcast leaked while disabled: datagrams={total} "
                 f"announce_log_line={announce_line}; {sorted(agg.keys())}")


def cleanup(*_args):
    sys.exit(1)


signal.signal(signal.SIGINT, cleanup)
signal.signal(signal.SIGTERM, cleanup)


def summary():
    print(f"\n{C_CYAN}{'='*60}")
    print("  Summary")
    print(f"{'='*60}{C_RESET}")
    passed = sum(1 for r in results if r[1])
    failed = sum(1 for r in results if not r[1])
    for name, ok, detail, elapsed in results:
        tag = f"{C_GREEN}PASS{C_RESET}" if ok else f"{C_RED}FAIL{C_RESET}"
        print(f"  [{tag}] {name}  {detail}  ({elapsed:.1f}s)")
    print()
    print(f"  {C_GREEN}{passed} passed{C_RESET}, {C_RED}{failed} failed{C_RESET}")
    save_failed_cases()
    if failed:
        sys.exit(1)


def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — LAN broadcast announce tests")
    print(f"{'='*60}{C_RESET}\n")

    failed = load_failed_cases()
    if failed is not None and len(failed) == 0:
        log_info("all previously passed (delete failed.json to re-run)")
        return

    if not DATAPACKS:
        log_fail("datapack", "no datapack configured (config paths.datapacks)")
        summary(); return
    staged = datapack_stage.staged_local(DATAPACKS[0], server=True)
    if not os.path.isdir(staged):
        log_fail("datapack", f"staged datapack missing: {staged}")
        summary(); return

    if not build_server():
        summary(); return

    test_enabled(staged)
    test_disabled(staged)
    summary()


if __name__ == "__main__":
    main()
