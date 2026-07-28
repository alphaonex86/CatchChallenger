#!/usr/bin/env python3
"""
testingbotactions.py — MULTI-bot botactions test.

testingbots.py drives exactly ONE bot. The botactions benchmark drives N at
once, and that path carries its own failure modes that a single bot can never
show. This case pins them:

  1. onboarding — all N bots must reach the map inside MAP_TIMEOUT.
     `emit_all_player_on_map()` only fires when numberOfSelectedCharacter
     reaches the target (MultipleBotConnection.cpp), so ONE bot stuck between
     "connected" and "character selected" makes the whole run wait forever with
     no message. The bot's own --map-timeout-seconds aborts, but says nothing
     about WHERE the group stopped. Here the per-stage counters (connected /
     character selected / on map) are sampled while waiting and printed on
     timeout, so a failure names the stage instead of just "timed out".

  2. activity — reaching the map is not enough: the bots must then ACT. A
     headless run has nobody to tick BotTargetList's "Auto-select", and without
     it autoStartActionTimer never starts, every bot stands still and a
     benchmark built on this measures an idle server. Regression guard for
     MainWindow::all_player_on_map() enabling it on the mAutoConnect path.

  3. protocol — no "Protocol wrong or corrupted" while N bots onboard at once
     (datapack transfer + character creation overlapping across connections).

The binary is driven EXTERNALLY (CLI + stdout/stderr), no test hook in
tools/ — per test/CLAUDE.md. Qt runs offscreen: this is a GUI-linked binary
and a test must never open windows on the operator's display.
"""
import sys
sys.dont_write_bytecode = True

import os
import json
import re
import shutil
import signal
import subprocess
import threading
import time

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

_CONFIG_PATH = os.path.join(os.path.expanduser("~"),
                            ".config", "catchchallenger-testing", "config.json")
try:
    with open(_CONFIG_PATH, "r") as _f:
        _config = json.load(_f)
except (OSError, ValueError):
    _config = {}
DATAPACKS = _config.get("paths", {}).get("datapacks", [])

# Shared with testingbots / testingserver / testinghttp (all.sh runs the
# testing-filedb group sequentially, see test/CLAUDE.md) so this case adds no
# extra server compile. NOT registered for cleanup for the same reason.
SERVER_PRO   = os.path.join(ROOT, "server/cli/catchchallenger-server-filedb.pro")
SERVER_BUILD = build_paths.build_path("server/cli/build/testing-filedb" + _DIAG_SUFFIX)
SERVER_BIN   = "catchchallenger-server-cli"

BOT_PRO      = os.path.join(ROOT, "tools/bot-actions/bot-actions.pro")
BOT_BUILD    = build_paths.build_path("tools/bot-actions/build/testing" + _DIAG_SUFFIX)
BOT_BIN      = "bot-actions"

# Own runtime dir + own port: the build tree is shared, the RUN is not.
RUN_DIR   = build_paths.build_path("tools/bot-actions/build/run-botactions" + _DIAG_SUFFIX)
GAME_PORT = 42537

BOT_COUNT       = 8
COMPILE_TIMEOUT = 600
BIND_TIMEOUT    = 60
# First run creates BOT_COUNT accounts AND characters before anyone reaches the
# map; that is the slow path this budget is sized for.
MAP_TIMEOUT     = 240
# Once on the map, how long the AI gets to produce its first action.
# all_player_on_map() fires BEFORE BotTargetList::loadAllBotsInformation()
# preloads every map for path-finding, and only then is the action timer armed.
# On the full datapack that preload is ~800 maps and eats a 45s window whole, so
# the run looked idle when it was still loading. Sized for the preload + a real
# idle margin on top.
ACTION_WINDOW   = 180
# How often the per-stage counters are printed while waiting for the map.
PROGRESS_EVERY  = 15

NICE_PREFIX         = ["nice", "-n", "19", "ionice", "-c", "3"]
NICE_PREFIX_RUNTIME = []

C_GREEN = "\033[92m"; C_RED = "\033[91m"; C_CYAN = "\033[96m"; C_RESET = "\033[0m"

results = []
total_expected = [0]
_last_log_time = [time.monotonic()]

SCRIPT_NAME = os.path.basename(__file__)
import failed_cases as _fc
import phase_timer

_WALL_LIMIT_SEC = 1500
import faulthandler
faulthandler.enable()
faulthandler.dump_traceback_later(_WALL_LIMIT_SEC + 10, exit=False)
try:
    faulthandler.register(signal.SIGUSR1)
except (AttributeError, RuntimeError):
    pass


# ── per-stage markers, straight off the bot's own logging ───────────────────
# Api_protocol_Qt::stateChanged(3)  -> socket up + protocol good, one per bot
# "selected character: "            -> MultipleBotConnection::haveCharacter(),
#                                      one per bot; THIS is what gates
#                                      emit_all_player_on_map()
# all_player_connected()/on_map()   -> the group-level events
STAGE_CONNECTED = "stateChanged(3)"
STAGE_SELECTED  = "selected character: "
EVENT_ALL_CONN  = "all_player_connected()"
EVENT_ALL_MAP   = "all_player_on_map()"

# Proof the AI actually ran: BotTargetList picks a target ("set blockObject")
# and/or the bot performs a game action. Any one of these means the action
# timer is ticking; zero of them across the whole window means it never armed.
ACTIVITY_PATTERNS = [
    # The one that matters for a load benchmark: a real move packet on the wire.
    # Api_protocol::send_player_move_internal() is logged once per step, so its
    # absence means the bots stand still and the server is idle whatever else
    # the run reports. Bot-side "step" logging is commented out upstream and the
    # target markers below only fire when a global target is reachable, so this
    # is the reliable witness.
    "send_player_move_internal",
    "set blockObject",
    "Start this: Try capture",
    "Start this: Try skill",
    "Start this: In fight",
    "is now in fight",
    "Seed correctly planted",
    "Plant collected",
    "monsterCatch(",
    "haveBeatBot",
    "normal tp",
    "tp after loose",
]

# Which libbot subsystem each marker proves was actually exercised. Movement
# alone says nothing about fight/heal/plant/shop, and those are most of the
# library: a load benchmark that only walks leaves them all untested.
# chat is listed but cannot fire headlessly today -- randomText and
# globalChatRandomReply default to false in ActionsBotInterface and are only
# settable from the GUI, so a CLI run never sends a chat packet.
COVERAGE_PATTERNS = {
    "move":      ["send_player_move_internal"],
    "target":    ["set blockObject"],
    "wildfight": ["is now in fight", "Start this: In fight", "Start this: Try skill"],
    "botfight":  ["haveBeatBot"],
    "catch":     ["Start this: Try capture", "monsterCatch("],
    "plant":     ["Seed correctly planted", "Seed cannot be planted", "Plant collected"],
    "heal":      ["heal point", "tp after loose"],
    "shop":      ["GlobalTarget::Shop", "Start this: Shop"],
    "item":      ["GlobalTarget::ItemOnMap"],
    "teleport":  ["normal tp", "teleportTo"],
    "inventory": ["have_inventory", "add_to_inventory"],
    "chat":      ["new_chat_text", "Qtnew_chat_text"],
}

CORRUPTION_PATTERNS = [
    "Protocol wrong or corrupted",
    "extension not allowed",
]

_CRASH_RE = re.compile(r"SIGSEGV|SIGABRT|Segmentation fault|std::bad_alloc|"
                       r"terminate called", re.IGNORECASE)


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


def build(pro, build_dir, label):
    import cmake_helpers as _ch
    return _ch.build_project(
        pro, build_dir, label,
        root=ROOT, nproc=NPROC,
        log_info=log_info, log_pass=log_pass, log_fail=log_fail,
        ensure_dir=ensure_dir, run_cmd=run_cmd,
        diag=DIAG, diag_module=diagnostic,
    )


def pick_maincode(staged_datapack):
    map_main = os.path.join(staged_datapack, "map", "main")
    if os.path.isdir(map_main):
        codes = sorted(d for d in os.listdir(map_main)
                       if os.path.isdir(os.path.join(map_main, d)))
        if codes:
            return codes[0]
    return "test"


def write_server_properties(path, maincode):
    """Only the keys this case needs; NormalServerGlobal fills the rest.
    Accounts are auto-created because the bots log in with fresh names."""
    with open(path, "w") as f:
        f.write('<?xml version="1.0"?>\n<configuration>\n'
                f'    <server-port value="{GAME_PORT}"/>\n'
                '    <server-ip value=""/>\n'
                '    <automatic_account_creation value="true"/>\n'
                f'    <max-players value="{BOT_COUNT * 4}"/>\n'
                '    <content>\n'
                f'        <mainDatapackCode value="{maincode}"/>\n'
                '    </content>\n'
                '    <mapVisibility>\n'
                '        <minimize value="cpu"/>\n'
                '    </mapVisibility>\n'
                '</configuration>\n')


def _server_preexec():
    process_helpers.setsid_and_pdeathsig()


def launch_server(staged_datapack):
    """Stage a clean run dir and start the server. Returns (proc, logpath)."""
    if os.path.isdir(RUN_DIR):
        shutil.rmtree(RUN_DIR, ignore_errors=True)
    os.makedirs(RUN_DIR, exist_ok=True)
    shutil.copy2(os.path.join(SERVER_BUILD, SERVER_BIN),
                 os.path.join(RUN_DIR, SERVER_BIN))
    link = os.path.join(RUN_DIR, "datapack")
    if os.path.islink(link) or os.path.exists(link):
        os.remove(link)
    os.symlink(staged_datapack, link)
    write_server_properties(os.path.join(RUN_DIR, "server-properties.xml"),
                            pick_maincode(staged_datapack))
    logpath = os.path.join(RUN_DIR, "server.log")
    cmd = NICE_PREFIX_RUNTIME + diagnostic.runtime_wrapper(DIAG) \
        + [os.path.join(RUN_DIR, SERVER_BIN)]
    diagnostic.record_cmd(cmd, RUN_DIR)
    lf = open(logpath, "wb")
    proc = subprocess.Popen(cmd, cwd=RUN_DIR, stdout=lf, stderr=subprocess.STDOUT,
                            preexec_fn=_server_preexec)
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
    if not bound:
        kill_proc(proc)
        return None, logpath
    return proc, logpath


def kill_proc(proc):
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


def run_bots(bot_count, map_timeout, action_window):
    """Run bot_count bots headless and watch them onboard.

    Returns a dict with the per-stage counters, when the group reached the map,
    and when the AI first acted. Nothing here fails the test — main() decides.
    """
    binary = os.path.join(BOT_BUILD, BOT_BIN)
    if not os.path.isfile(binary):
        return {"error": f"binary not found: {binary}"}

    env = os.environ.copy()
    # A GUI-linked Qt binary: never open a window on the operator's display.
    env["QT_QPA_PLATFORM"] = "offscreen"
    env["DISPLAY"] = ""
    for k, v in diagnostic.runtime_env(DIAG).items():
        env[k] = v

    args = [binary,
            "--host", "localhost", "--port", str(GAME_PORT),
            "--bots", str(bot_count),
            "--login", "botact_%NUMBER%", "--pass", "botact_%NUMBER%",
            # Let THIS harness own the timeout so it can report the stage the
            # group stopped at; the bot's own abort would just exit.
            "--map-timeout-seconds", str(int(map_timeout) + 60)]
    cmd = NICE_PREFIX_RUNTIME + diagnostic.runtime_wrapper(DIAG) + args
    diagnostic.record_cmd(cmd, BOT_BUILD)
    log_info(f"running {bot_count} bots headless (offscreen), map timeout {map_timeout}s")

    proc = subprocess.Popen(cmd, cwd=BOT_BUILD,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            env=env, preexec_fn=process_helpers.setsid_and_pdeathsig)

    lock = threading.Lock()
    state = {
        "connected": 0, "selected": 0,
        "all_connected": False, "on_map": False,
        "activity": 0, "first_activity_line": "",
        "corruption": [], "crash": [],
        "coverage": {},
        "lines": [],
    }
    on_map_evt = threading.Event()
    activity_evt = threading.Event()

    def consume(line):
        with lock:
            state["lines"].append(line)
            if len(state["lines"]) > 4000:
                del state["lines"][:1000]
            if STAGE_CONNECTED in line:
                state["connected"] += 1
            if STAGE_SELECTED in line:
                state["selected"] += 1
            if EVENT_ALL_CONN in line:
                state["all_connected"] = True
            if EVENT_ALL_MAP in line:
                state["on_map"] = True
                on_map_evt.set()
            idx = 0
            while idx < len(ACTIVITY_PATTERNS):
                if ACTIVITY_PATTERNS[idx] in line:
                    state["activity"] += 1
                    if not state["first_activity_line"]:
                        state["first_activity_line"] = line.strip()[:120]
                    activity_evt.set()
                    break
                idx += 1
            for subsystem, markers in COVERAGE_PATTERNS.items():
                mi = 0
                while mi < len(markers):
                    if markers[mi] in line:
                        state["coverage"][subsystem] = state["coverage"].get(subsystem, 0) + 1
                        break
                    mi += 1
            idx = 0
            while idx < len(CORRUPTION_PATTERNS):
                if CORRUPTION_PATTERNS[idx] in line:
                    state["corruption"].append(line.strip()[:200])
                    break
                idx += 1
            if _CRASH_RE.search(line):
                state["crash"].append(line.strip()[:200])

    def reader(stream):
        while True:
            raw = stream.readline()
            if not raw:
                break
            consume(raw.decode(errors="replace").rstrip("\n"))

    t_out = threading.Thread(target=reader, args=(proc.stdout,), daemon=True)
    t_err = threading.Thread(target=reader, args=(proc.stderr,), daemon=True)
    t_out.start()
    t_err.start()

    # ── phase A: wait for the whole group to reach the map ──────────────────
    start = time.monotonic()
    deadline = start + clamp_local(map_timeout)
    next_progress = start + PROGRESS_EVERY
    while time.monotonic() < deadline:
        if on_map_evt.wait(timeout=0.5):
            break
        if proc.poll() is not None:
            break
        if time.monotonic() >= next_progress:
            with lock:
                log_info(f"  onboarding: connected={state['connected']}/{bot_count} "
                         f"character-selected={state['selected']}/{bot_count} "
                         f"all_connected={state['all_connected']}")
            next_progress = time.monotonic() + PROGRESS_EVERY
    state["map_seconds"] = time.monotonic() - start
    state["exited_early"] = proc.poll() is not None
    state["exit_code"] = proc.poll()

    # ── phase B: on the map, the AI must act ────────────────────────────────
    if state["on_map"]:
        a_start = time.monotonic()
        a_deadline = a_start + clamp_local(action_window)
        while time.monotonic() < a_deadline:
            if activity_evt.wait(timeout=0.5):
                break
            if proc.poll() is not None:
                break
        state["action_seconds"] = time.monotonic() - a_start

    # Read the real widget values while the bot is STILL RUNNING -- this is the
    # only moment the automation channel is alive.
    if state["on_map"] and proc.poll() is None:
        state["widgets"] = query_widgets([
            ("GETSTATE", "STATE"),
            ("GETWIDGET bots", "WIDGET"),
            ("GETWIDGET globalTargets", "WIDGET"),
            ("GETWIDGET autoSelectTarget", "WIDGET"),
            ("GETWIDGET comboBoxStep", "WIDGET"),
        ])

    kill_proc(proc)
    t_out.join(timeout=5)
    t_err.join(timeout=5)
    with lock:
        # Keep the bot output on disk: a "bots stand still" or "exited early"
        # verdict is untriageable from the summary line alone, and re-running to
        # reproduce costs a full onboarding cycle.
        try:
            with open(os.path.join(RUN_DIR, "bot-actions.log"), "w") as bf:
                idx = 0
                while idx < len(state["lines"]):
                    bf.write(state["lines"][idx] + "\n")
                    idx += 1
        except OSError as exc:
            log_info(f"could not save bot log: {exc}")
        return dict(state)


def _automation_socket_paths(prefix="CatchChallenger-BotActions-", slots=4):
    """QLocalServer names on unix are ExtraSocket::pathSocket(prefix+N), i.e.
    "<prefix><N>-<uid>", created under the Qt runtime dir or /tmp."""
    uid = os.getuid()
    roots = [os.environ.get("XDG_RUNTIME_DIR") or f"/run/user/{uid}", "/tmp"]
    out = []
    for root in roots:
        n = 0
        while n < slots:
            out.append(os.path.join(root, f"{prefix}{n}-{uid}"))
            n += 1
    return out


def query_widgets(commands, connect_timeout=20):
    """Ask the running bot for widget state over its automation channel.

    Returns {command: [reply lines]}. This reads the REAL widget values (list
    row counts, checkbox states, combo selection) instead of inferring them
    from stdout, which is the only way to catch "the list looks populated in
    the GUI but is empty" style defects.
    """
    import socket as _socket
    sock = None
    deadline = time.monotonic() + clamp_local(connect_timeout)
    while sock is None and time.monotonic() < deadline:
        for path in _automation_socket_paths():
            if os.path.exists(path):
                candidate = _socket.socket(_socket.AF_UNIX, _socket.SOCK_STREAM)
                try:
                    candidate.connect(path)
                    candidate.settimeout(5)
                    sock = candidate
                    break
                except OSError:
                    candidate.close()
        if sock is None:
            time.sleep(0.5)
    if sock is None:
        return None
    out = {}
    try:
        for cmd, end_marker in commands:
            sock.sendall((cmd + "\n").encode())
            buf = b""
            lines = []
            stop = time.monotonic() + 5
            while time.monotonic() < stop:
                try:
                    chunk = sock.recv(4096)
                except (OSError, _socket.timeout):
                    break
                if not chunk:
                    break
                buf += chunk
                while b"\n" in buf:
                    raw, buf = buf.split(b"\n", 1)
                    text = raw.decode(errors="replace").strip()
                    if text:
                        lines.append(text)
                if lines and (end_marker is None or end_marker in lines[-1]):
                    break
            out[cmd] = lines
    finally:
        sock.close()
    return out


def _widget_int(lines, key):
    """Pull `key=<int>` out of a WIDGET/STATE reply line."""
    if not lines:
        return None
    for line in lines:
        for token in line.split():
            if token.startswith(key + "="):
                try:
                    return int(token.split("=", 1)[1])
                except ValueError:
                    return None
    return None


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


def main():
    print(f"\n{C_CYAN}{'='*60}")
    print(f"  CatchChallenger — botactions multi-bot ({BOT_COUNT} bots)")
    print(f"{'='*60}{C_RESET}\n")

    failed_cases = load_failed_cases()
    if failed_cases is not None and len(failed_cases) == 0:
        log_info("all previously passed, skipping (delete failed.json for full re-run)")
        return

    # compile server + compile bot + on-map + activity + protocol
    total_expected[0] = 6

    print(f"\n{C_CYAN}--- Compilation ---{C_RESET}\n")
    # Resume mode may skip a compile, but the all.sh post-success sweep removes
    # build dirs — so "it passed last time" can leave nothing to run at all.
    # That bail-out reached summary() with an EMPTY results list, which saved an
    # empty failure set, erased the real failure and made every later run skip
    # the case entirely (a silent pass). Build whenever the binary is missing,
    # whatever the resume state says.
    if should_run("compile server-filedb", failed_cases) \
            or not os.path.isfile(os.path.join(SERVER_BUILD, SERVER_BIN)):
        server_ok = build(SERVER_PRO, SERVER_BUILD, "compile server-filedb")
    else:
        server_ok = True
    if should_run("compile bot-actions", failed_cases) \
            or not os.path.isfile(os.path.join(BOT_BUILD, BOT_BIN)):
        bot_ok = build(BOT_PRO, BOT_BUILD, "compile bot-actions")
    else:
        bot_ok = True
    if not server_ok or not bot_ok:
        # build() already logged the FAIL, so summary() has a result to save.
        summary()
        return

    # ── datapack: server and bot both point at the staged copy ──────────────
    dp_src = DATAPACKS[0] if DATAPACKS else None
    if not dp_src or not os.path.isdir(dp_src):
        log_fail("bots-all-on-map", "no datapack configured in config.json")
        summary()
        return
    staged = datapack_stage.staged_local(dp_src)
    bot_dp = os.path.join(BOT_BUILD, "datapack")
    if os.path.islink(bot_dp) or os.path.isfile(bot_dp):
        os.remove(bot_dp)
    elif os.path.isdir(bot_dp):
        shutil.rmtree(bot_dp)
    os.symlink(staged, bot_dp)

    print(f"\n{C_CYAN}--- {BOT_COUNT} bots onboarding ---{C_RESET}\n")
    srv, srv_log = launch_server(staged)
    if srv is None:
        log_fail("bots-all-on-map", f"server failed to bind; see {srv_log}")
        summary()
        return

    try:
        st = run_bots(BOT_COUNT,
                      diagnostic.scale_timeout(DIAG, MAP_TIMEOUT),
                      diagnostic.scale_timeout(DIAG, ACTION_WINDOW))
    finally:
        kill_proc(srv)

    if "error" in st:
        log_fail("bots-all-on-map", st["error"])
        summary()
        return

    # 1. every bot on the map before the timeout
    stages = (f"connected={st['connected']}/{BOT_COUNT} "
              f"character-selected={st['selected']}/{BOT_COUNT} "
              f"all_connected={st['all_connected']}")
    if st["on_map"]:
        log_pass("bots-all-on-map",
                 f"{BOT_COUNT} bots on map in {st['map_seconds']:.0f}s ({stages})")
    else:
        extra = ""
        if st["selected"] < BOT_COUNT and st["all_connected"]:
            # the exact shape of the hang: the group event needs every bot
            extra = (f" — {BOT_COUNT - st['selected']} bot(s) connected but never "
                     f"selected a character, so emit_all_player_on_map() cannot fire")
        if st["exited_early"]:
            # A bot that dies before the map takes its reason with it, so print a
            # long tail: BOT_ABORT()/assert lines land well above the last few.
            extra += (f" — bot process exited before the map "
                      f"(exit code {st.get('exit_code')})")
        log_fail("bots-all-on-map",
                 f"timeout after {st['map_seconds']:.0f}s: {stages}{extra}")
        tail = 60 if st["exited_early"] else 15
        idx = max(0, len(st["lines"]) - tail)
        while idx < len(st["lines"]):
            print(f"  | {st['lines'][idx]}")
            idx += 1

    # 2. once on the map they must act (headless auto-select regression)
    if st["on_map"]:
        if st["activity"] > 0:
            log_pass("bots-act-after-map",
                     f"{st['activity']} action(s) in {st.get('action_seconds', 0):.0f}s: "
                     f"{st['first_activity_line']}")
        else:
            log_fail("bots-act-after-map",
                     f"no move packet and no action in {st.get('action_seconds', 0):.0f}s "
                     f"on the map — the bots stand still, so a benchmark on this measures "
                     f"an idle server. Check that the bot registered its OWN player: the "
                     f"spawn position only arrives with QthaveCharacter(mapId,x,y,dir); "
                     f"drop it and the placeholder entry keeps canMoveOnMap=false and "
                     f"moveTimer never starts")
    else:
        log_fail("bots-act-after-map", "not evaluated: bots never reached the map")

    # 2b. which libbot subsystems the run actually exercised. Reported, not
    #     gated: what the AI reaches depends on the map content it spawns near,
    #     so a hard gate here would be flaky. The table is what tells us whether
    #     a "green" benchmark really covered fight/heal/plant or only walked.
    if st["on_map"]:
        cov = st.get("coverage", {})
        hit = []
        miss = []
        for subsystem in COVERAGE_PATTERNS:
            if cov.get(subsystem):
                hit.append(f"{subsystem}={cov[subsystem]}")
            else:
                miss.append(subsystem)
        log_info(f"libbot coverage HIT : {', '.join(hit) if hit else '(none)'}")
        log_info(f"libbot coverage MISS: {', '.join(miss) if miss else '(none)'}")

    # 2c. Qt widget state, read over the automation channel. The GUI is what a
    #     human looks at when the bot misbehaves, and it can disagree with the
    #     internal state (an empty bot list while N bots are connected).
    if st["on_map"]:
        w = st.get("widgets")
        if not w:
            log_fail("bots-widget-state",
                     "no reply on the QLocalServer automation channel "
                     "(bot-actions should listen on CatchChallenger-BotActions-N)")
        else:
            for cmd in sorted(w):
                log_info(f"  {cmd} -> {'; '.join(w[cmd]) if w[cmd] else '(no reply)'}")
            internal = _widget_int(w.get("GETSTATE"), "bots")
            listed = _widget_int(w.get("GETWIDGET bots"), "count")
            checked = _widget_int(w.get("GETWIDGET autoSelectTarget"), "checked")
            steps = _widget_int(w.get("GETWIDGET comboBoxStep"), "count")
            problems = []
            if checked != 1:
                # regression guard: a headless run must have armed the AI
                problems.append(f"autoSelectTarget checked={checked}, expected 1")
            if not steps:
                problems.append(f"comboBoxStep count={steps}, expected >0")
            if listed is None or internal is None:
                problems.append("could not read bot list / internal bot count")
            elif listed != internal:
                problems.append(f"bot list shows {listed} row(s) but {internal} "
                                f"bot(s) are connected")
            if problems:
                log_fail("bots-widget-state", "; ".join(problems))
            else:
                log_pass("bots-widget-state",
                         f"bot list {listed} row(s) == {internal} connected, "
                         f"auto-select on, {steps} step(s)")
    else:
        log_fail("bots-widget-state", "not evaluated: bots never reached the map")

    # 3. no protocol desync while N bots onboard together
    if st["corruption"]:
        log_fail("bots-no-protocol-corruption",
                 f"{len(st['corruption'])} corrupted-protocol line(s): "
                 f"{st['corruption'][0]}")
    elif st["crash"]:
        log_fail("bots-no-protocol-corruption",
                 f"bot crashed: {st['crash'][0]}")
    else:
        log_pass("bots-no-protocol-corruption", "no corrupted packet, no crash")

    summary()


if __name__ == "__main__":
    main()
