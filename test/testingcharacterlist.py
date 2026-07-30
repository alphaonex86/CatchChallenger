#!/usr/bin/env python3
"""
testingcharacterlist.py — the account character-list PERSISTENCE contract.

WHAT IS PINNED
--------------
With the FILE database backend and the all-in-one CLI server, every character
successfully created on ONE login must STILL be in the character list that
login gets back. Creation appends to a single hps-serialised blob:

  server/base/ClientLoad/ClientHeavyLoadLogin2.cpp  (#elif CATCHCHALLENGER_DB_FILE)
      read  database/common/accounts/<account_id>   -> characterEntryList
      push_back(newChar)
      write database/common/accounts/<account_id>

and it is read back verbatim at login by
  server/base/ClientLoad/ClientHeavyLoadLogin.cpp:317-406 (send_characterEntryList).

So the test creates N characters through one login and then asks the server how
many that login has. Three numbers must agree:

  created  — creations the protocol confirmed (bot went CREATING_CHARACTER
             then SELECTING_CHARACTER, i.e. newCharacterId returned 0x00)
  onfile   — files in <run>/database/common/characters/, one per created
             character (that file's existence is what makes a pseudo
             "already taken", ClientHeavyLoadLogin2.cpp:515)
  listed   — "CHARACTERS <n>" reported by a --list-only bot, n = size of the
             character list in the server's login reply

THE BUG THIS CAUGHT (still open at the time of writing)
-------------------------------------------------------
listed < created: 8 bots -> created 8, listed 4; 16 bots -> created 16,
listed 11. Not a lost update inside the account blob — the login ends up with
SEVERAL account records and the characters are split across them:

  Client::createAccount() (ClientHeavyLoadLogin.cpp:453) NEVER re-checks that
  database/login/<hexa-of-login> already exists in the CATCHCHALLENGER_DB_FILE
  branch. The SQL backends do (createAccount_return re-reads db_query_login and
  answers 0x02 "Login already used", line 677-679); the FILE branch's guard at
  line 578-581 stats "characters/"+hexa-of-**pseudo** — a path that is not the
  database's ("database/common/characters/"), keyed on the wrong field, and
  built from a pseudo that is still EMPTY at account-creation time. It can
  never match, so every create-account request bumps maxAccountId, writes a
  fresh EMPTY database/common/accounts/<newid> and OVERWRITES
  database/login/<hexa> to point at it.

  N bots opening the same brand-new login in one burst all get the "unknown
  login, you may create it" reply 0x07 (line 163-175) before any of them has
  written the login file, so each sends a create-account: run-8 ended up with
  accounts 1..4 for one login (login file -> 4, holding 4 characters, the other
  4 orphaned on account 1); run-16 with accounts 1..11 (login file -> 11,
  holding 11, the other 5 orphaned on account 6).

The character SHORTFALL itself is timing-dependent (a re-run of the 8-bot case
produced 8 duplicate accounts yet listed all 8 characters, because that time
every bot happened to latch the last-written account id). The duplicate ACCOUNT
records are not: they appear on every run. So the test asserts both — the count
of account records for its single login must be exactly 1, and no created
character may be missing from the list.

WHAT IS *NOT* A BUG HERE
------------------------
tools/bot-bench/ has no %NUMBER% substitution: `--login` is ONE value, so all
N bots share ONE account and bot i takes the i-th character of the list. A
character cannot be logged in twice, so when several bots race for the same
slot some of them legitimately fail — a multi-bot re-run reporting
"ON_MAP 7/16" is NOT evidence of data loss and this test deliberately does not
assert on it. That is exactly why PHASE B uses a SINGLE bot with --list-only:
one bot cannot collide with itself, and it selects nothing at all, so the count
it reports is the account's real content and nothing else.

PHASES (per bot count N)
------------------------
  A. N bots on one account create N characters (fresh database).
  B. ONE bot, --list-only, prints CHARACTERS <n>.
     assert n == created == onfile.
  C. the server never crashed and stopped cleanly.

No test hook is added to the server: everything is observed from the outside
(bot-bench over TCP + the server's own on-disk database), per test/CLAUDE.md.
"""
import sys
sys.dont_write_bytecode = True

import os
import re
import json
import shutil
import signal
import subprocess
import resource
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

# ── config (datapack source) ────────────────────────────────────────────────
_CONFIG_PATH = os.path.join(os.path.expanduser("~"),
                            ".config", "catchchallenger-testing", "config.json")
try:
    with open(_CONFIG_PATH, "r") as _f:
        _config = json.load(_f)
except (OSError, ValueError):
    _config = {}
DATAPACKS = _config.get("paths", {}).get("datapacks", [])

SERVER_PRO  = os.path.join(ROOT, "server/cli/catchchallenger-server-filedb.pro")
SERVER_BUILD = build_paths.build_path("server/cli/build/testing-characterlist" + _DIAG_SUFFIX)
SERVER_BIN  = "catchchallenger-server-cli"
cleanup_helpers.register_build_dir(SERVER_BUILD)

BOT_PRO   = os.path.join(ROOT, "tools/bot-bench/bot-bench.pro")
BOT_BUILD = build_paths.build_path("tools/bot-bench/build/testing-characterlist" + _DIAG_SUFFIX)
BOT_BIN   = "bot-bench"
cleanup_helpers.register_build_dir(BOT_BUILD)

# Bot counts to exercise. 8 is the smallest count the earlier report claimed a
# shortfall at; 16 is where it claimed a big one.
BOT_COUNTS = (8, 16)

# One port per bot count so a leftover server from the previous case can never
# be mistaken for this one. Kept away from 61917 (testing-filedb) and 42531
# (testingbroadcast).
BASE_PORT = 42541

# max_character is the per-account creation cap (uint8_t, server/base/
# NormalServerGlobal.cpp default 3, enforced in ClientHeavyLoadLogin2.cpp:268).
# Pin it well above max(BOT_COUNTS) so the cap is provably not what limits the
# result.
MAX_CHARACTER = 200

COMPILE_TIMEOUT = 600
BIND_TIMEOUT    = 40      # datapack load + bind on a slow/busy host
BOT_TIMEOUT     = 240     # N bots: account creation + datapack parse + N maps
LIST_TIMEOUT    = 60      # one bot, login only, no datapack load

NICE_PREFIX         = ["nice", "-n", "19", "ionice", "-c", "3"]
NICE_PREFIX_RUNTIME = []

C_GREEN = "\033[92m"; C_RED = "\033[91m"; C_CYAN = "\033[96m"; C_RESET = "\033[0m"

results = []
total_expected = [0]
_last_log_time = [time.monotonic()]

SCRIPT_NAME = os.path.basename(__file__)
import failed_cases as _fc
import phase_timer

# Wall cap kept 1:1 with all.sh PER_TEST_TIMEOUT_MAP[testingcharacterlist.py].
_WALL_LIMIT_SEC = 20 * 60
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


def build_one(pro, build_dir, binname):
    import cmake_helpers as _ch
    return _ch.build_project(
        pro, build_dir, binname,
        root=ROOT, nproc=NPROC,
        log_info=log_info, log_pass=log_pass, log_fail=log_fail,
        ensure_dir=ensure_dir, run_cmd=run_cmd,
        diag=DIAG, diag_module=diagnostic,
    )


# ── server-properties.xml — partial; NormalServerGlobal fills the rest ───────
def write_server_properties(path, maincode, port, max_players):
    """Only the keys this test depends on. `max_character` is the per-account
    creation cap read by server/cli/main-unix2.cpp:48 (top-level key, NOT
    inside a group); `max-players` must exceed the bot count or the server
    refuses the extra connections and the shortfall would be OUR fault."""
    with open(path, "w") as f:
        f.write('<?xml version="1.0"?>\n<configuration>\n'
                f'    <server-port value="{port}"/>\n'
                '    <server-ip value=""/>\n'
                '    <automatic_account_creation value="true"/>\n'
                f'    <max-players value="{max_players}"/>\n'
                f'    <max_character value="{MAX_CHARACTER}"/>\n'
                '    <min_character value="1"/>\n'
                '    <max_pseudo_size value="20"/>\n'
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
    """preexec_fn for the server child: setsid + PR_SET_PDEATHSIG so the kernel
    reaps it when this harness dies (incl. SIGKILL from all.sh's wall cap),
    plus the 15 min CPU / 128 MiB AS caps from test/CLAUDE.md to catch a wedged
    loop or a runaway alloc. Skipped under valgrind, which blows past both on a
    perfectly legitimate workload."""
    process_helpers.setsid_and_pdeathsig()
    try:
        if not diagnostic.is_valgrind(DIAG):
            resource.setrlimit(resource.RLIMIT_CPU, (15 * 60, 15 * 60 + 60))
            resource.setrlimit(resource.RLIMIT_AS, (128 * 1024 * 1024,) * 2)
    except Exception as exc:
        try:
            os.write(2, f"[testingcharacterlist] preexec rlimit failed: {exc!r}\n".encode())
        except Exception:
            pass


def stage_run_dir(work, staged_datapack, port, max_players):
    """A FRESH run dir per case: the FILE backend keeps its whole database in
    ./database/ relative to the CWD, so a new dir == a guaranteed-empty
    database (the server mkdir()s the tree itself, see main-unix.cpp:354)."""
    if os.path.isdir(work):
        shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work, exist_ok=True)
    shutil.copy2(os.path.join(SERVER_BUILD, SERVER_BIN), os.path.join(work, SERVER_BIN))
    link = os.path.join(work, "datapack")
    if os.path.islink(link) or os.path.exists(link):
        os.remove(link)
    os.symlink(staged_datapack, link)
    write_server_properties(os.path.join(work, "server-properties.xml"),
                            pick_maincode(staged_datapack), port, max_players)


def launch_server(work):
    """Start the server and wait for its bind line. Returns (proc, logpath)."""
    logpath = os.path.join(work, "server.log")
    cmd = NICE_PREFIX_RUNTIME + diagnostic.runtime_wrapper(DIAG) \
        + [os.path.join(work, SERVER_BIN)]
    diagnostic.record_cmd(cmd, work)
    lf = open(logpath, "wb")
    proc = subprocess.Popen(cmd, cwd=work, stdout=lf, stderr=subprocess.STDOUT,
                            preexec_fn=_apply_child_rlimits)
    deadline = time.monotonic() + clamp_local(diagnostic.scale_timeout(DIAG, BIND_TIMEOUT))
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
        stop_server(proc)
        return None, logpath
    return proc, logpath


def stop_server(proc):
    """SIGTERM then SIGKILL. Returns the child's returncode (None if it was
    already reaped)."""
    if proc is None:
        return None
    if proc.poll() is not None:
        return proc.returncode
    try:
        proc.terminate()
        proc.wait(timeout=15)
    except Exception:
        try:
            proc.kill()
            proc.wait(timeout=10)
        except Exception:
            pass
    return proc.returncode


def read_log(logpath):
    try:
        with open(logpath, "rb") as f:
            return f.read().decode(errors="replace")
    except OSError:
        return ""


# ── bot-bench driving ───────────────────────────────────────────────────────
_STATE_RE = re.compile(r"^\[(bot\d+)\]\s+([A-Z_]+)")


def run_bots(work, port, login, bots, list_only, timeout):
    """Run bot-bench from `work` (so it finds the datapack symlink) and return
    (returncode, combined_output)."""
    args = [os.path.join(BOT_BUILD, BOT_BIN),
            "--host", "localhost", "--port", str(port),
            "--login", login, "--pass", login,
            "--bots", str(bots),
            "--datapack", os.path.join(work, "datapack"),
            "--timeout", str(int(clamp_local(diagnostic.scale_timeout(DIAG, timeout)) * 1000))]
    if list_only:
        args.append("--list-only")
    cmd = NICE_PREFIX_RUNTIME + args
    diagnostic.record_cmd(cmd, work)
    wall = clamp_local(diagnostic.scale_timeout(DIAG, timeout) + 60)
    try:
        p = subprocess.run(cmd, cwd=work, timeout=wall,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           preexec_fn=process_helpers.setsid_and_pdeathsig)
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired as exc:
        out = exc.output.decode(errors="replace") if exc.output else ""
        return -1, out + f"\n[harness] bot-bench exceeded {wall}s\n"


def count_confirmed_creations(output):
    """Count bots the SERVER confirmed a creation to.

    bot-bench prints one line per state change. A bot that asked for a
    character enters CREATING_CHARACTER and only leaves it for
    SELECTING_CHARACTER when newCharacterId() carried the success code 0x00
    (CliApiClient.cpp:481 turns every other code into a failure). Both lines
    are always emitted: the transition needs a server round-trip, so the event
    loop passes through reportStateChanges() in between."""
    order = {}
    for line in output.splitlines():
        m = _STATE_RE.match(line.strip())
        if m:
            order.setdefault(m.group(1), []).append(m.group(2))
    created = 0
    for _bot, states in order.items():
        if "CREATING_CHARACTER" in states and "SELECTING_CHARACTER" in states:
            if states.index("CREATING_CHARACTER") < states.index("SELECTING_CHARACTER"):
                created += 1
    return created


def parse_characters_line(output):
    """Read back the "CHARACTERS <n>" of a --list-only run. Returns None when
    the bot never got a login reply."""
    found = None
    for line in output.splitlines():
        m = re.match(r"^CHARACTERS\s+(\d+)\s*$", line.strip())
        if m:
            found = int(m.group(1))
    return found


def _count_files(*parts):
    d = os.path.join(*parts)
    try:
        return len([n for n in os.listdir(d)
                    if os.path.isfile(os.path.join(d, n))])
    except OSError:
        return -1


def count_character_files(work):
    """Files under database/common/characters/ — the server writes exactly one
    per created character, keyed by the hexa of the pseudo (that file is what
    ClientHeavyLoadLogin2.cpp:515 stats to reject a duplicate pseudo)."""
    return _count_files(work, "database", "common", "characters")


def count_account_files(work):
    """Files under database/common/accounts/ — ONE per account record. The test
    uses a single login, so anything but 1 means create-account produced
    duplicate accounts for it (see the module docstring)."""
    return _count_files(work, "database", "common", "accounts")


# ── the test ────────────────────────────────────────────────────────────────
def run_case(bots, staged_datapack, failed):
    case_persist = f"persist-{bots}bots"
    case_clean   = f"server-clean-{bots}bots"
    if not should_run(case_persist, failed) and not should_run(case_clean, failed):
        return

    port  = BASE_PORT + BOT_COUNTS.index(bots)
    login = f"cclist{bots}"
    work  = os.path.join(SERVER_BUILD, f"run-{bots}")
    # max-players must clear the bot count with room for the login socket and
    # the game socket of a bot being handed over between the two stages.
    stage_run_dir(work, staged_datapack, port, bots * 2 + 8)

    proc, logpath = launch_server(work)
    if proc is None:
        log_fail(case_persist, f"server failed to bind; see {logpath}")
        print(read_log(logpath)[-2000:])
        return

    created = 0
    listed = None
    onfile = -1
    accounts = -1
    bots_rc = None
    list_rc = None
    bots_out = ""
    list_out = ""
    onmap_ok = False
    try:
        # ── PHASE 0: ONE bot creates the ACCOUNT ────────────────────────
        # Deliberately serialised. Letting N bots race to create the same
        # brand-new login does NOT test character persistence -- it tests
        # createAccount() concurrency, where being told "Login already used"
        # is the CORRECT answer (that is what the SQL backends do), so all
        # but one bot legitimately fail and the run says nothing about the
        # character list. Creating the account first isolates the thing this
        # test is actually about: N concurrent addCharacter on ONE account.
        with phase_timer.phase(f"account-{bots}bots"):
            acc_rc, acc_out = run_bots(work, port, login, 1,
                                       False, BOT_TIMEOUT)
        log_info(f"phase 0: account create rc={acc_rc}, "
                 f"{count_confirmed_creations(acc_out)} creation(s)")

        # ── PHASE A: N bots on that EXISTING account ────────────────────
        with phase_timer.phase(f"create-{bots}bots"):
            bots_rc, bots_out = run_bots(work, port, login, bots,
                                         False, BOT_TIMEOUT)
        created = count_confirmed_creations(bots_out)
        onmap = re.search(r"^ON_MAP\s+(\d+)/(\d+)\s*$", bots_out, re.M)
        onmap_txt = onmap.group(0) if onmap else "ON_MAP <absent>"
        # Every bot must get on the map. Without this the case passes
        # trivially when a regression leaves only ONE bot working: the
        # remaining invariants below are all self-consistent at n=1.
        onmap_ok = bool(onmap) and onmap.group(1) == onmap.group(2) == str(bots)
        log_info(f"phase A: {bots} bots, rc={bots_rc}, {onmap_txt}, "
                 f"{created} creation(s) confirmed by the server")
        onfile = count_character_files(work)
        accounts = count_account_files(work)

        # ── PHASE B: ONE bot, login only, count what the account holds ──
        with phase_timer.phase(f"list-{bots}bots"):
            list_rc, list_out = run_bots(work, port, login, 1,
                                         True, LIST_TIMEOUT)
        listed = parse_characters_line(list_out)
        log_info(f"phase B: --list-only rc={list_rc}, CHARACTERS={listed}")
    finally:
        rc = stop_server(proc)

    log_text = read_log(logpath)

    # ── assertion: nothing was lost ─────────────────────────────────────
    if should_run(case_persist, failed):
        if created == 0:
            _fc.set_extras(case_persist,
                           run_output=bots_out + "\n=== server log ===\n" + log_text)
            log_fail(case_persist,
                     f"phase A created no character at all (rc={bots_rc}); "
                     f"the test cannot say anything about persistence")
            print(bots_out[-3000:])
        elif listed is None:
            _fc.set_extras(case_persist,
                           run_output=list_out + "\n=== server log ===\n" + log_text)
            log_fail(case_persist,
                     f"phase B printed no 'CHARACTERS <n>' line (rc={list_rc}); "
                     f"the --list-only bot never received a login reply")
            print(list_out[-3000:])
        else:
            numbers = (f"{bots} bots: created={created} onfile={onfile} "
                       f"listed={listed} accounts={accounts}")
            problems = []
            if not onmap_ok:
                problems.append(f"not every bot reached the map ({onmap_txt}); "
                                f"the concurrent-addCharacter scenario did not "
                                f"actually run, so the counts below prove nothing")
            # The account's character list must match what is on disk. This is
            # the real persistence invariant, and unlike a created-vs-listed
            # comparison it holds however the bots split between selecting an
            # existing character and creating a new one.
            if listed != onfile:
                problems.append(f"login {login!r} lists {listed} character(s) "
                                f"but {onfile} exist on disk — "
                                f"{onfile - listed} unreachable")
            if accounts != 1:
                problems.append(f"{accounts} account record(s) exist for that "
                                f"ONE login (expected 1)")
            if not problems:
                log_pass(case_persist, numbers)
            else:
                _fc.set_extras(case_persist,
                               run_output=bots_out + "\n=== --list-only ===\n" + list_out
                                          + "\n=== server log ===\n" + log_text)
                log_fail(case_persist, numbers + " — " + "; ".join(problems) +
                         ". Client::createAccount() does not re-check "
                         "database/login/<hexa> in the CATCHCHALLENGER_DB_FILE "
                         "branch, see the module docstring")
                print(list_out[-2000:])

    # ── assertion: the server survived and stopped cleanly ──────────────
    if should_run(case_clean, failed):
        crashed = process_helpers.is_crash(rc)
        marker = ""
        if "Segmentation fault" in log_text:
            marker = "'Segmentation fault' in the server log"
        if crashed:
            marker = f"returncode {rc}"
        if marker:
            if process_helpers.is_sigabrt(rc) and \
               process_helpers.looks_like_protocol_parse_failure(log_text):
                bt = process_helpers.rerun_under_gdb(
                    [os.path.join(work, SERVER_BIN)], cwd=work, timeout=120)
                _fc.set_extras(case_clean,
                               run_output=log_text + "\n=== gdb ===\n" + bt)
                log_fail(case_clean, f"PROTOCOL-PARSE-FAILURE ({marker})")
                print(bt[-4000:])
            else:
                _fc.set_extras(case_clean, run_output=log_text)
                log_fail(case_clean, f"server crashed: {marker}")
                print(log_text[-3000:])
        else:
            log_pass(case_clean, f"no crash, stopped with returncode {rc}")


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


def run_race_case(bots, staged_datapack, failed):
    """N bots open the SAME BRAND-NEW login at once -> exactly ONE account.

    This is the case persist-<N>bots deliberately cannot cover: that one
    pre-creates the account, so createAccount() is never called concurrently.
    Here it is, and the invariant is narrow on purpose:

      * accounts == 1. One login owns one account record, always.

    What is NOT asserted: that every bot succeeds. All but one being told
    "Login already used" (reply 0x02) is the CORRECT answer and is exactly
    what the SQL backends do -- so requiring created == N would demand the
    broken behaviour back. The losers are expected to fail; what must never
    happen is a second account record appearing for the same login, because
    the login file then points at the last writer and every character
    created against the earlier records becomes unreachable.

    Deterministic: the duplicate records appeared on every pre-fix run,
    whereas the resulting character shortfall was timing-dependent -- which
    is why the account count, not the character count, is the oracle here.
    """
    case_race  = f"account-race-{bots}bots"
    case_clean = f"race-clean-{bots}bots"
    if not should_run(case_race, failed) and not should_run(case_clean, failed):
        return

    # own port + own run dir + own login: must not touch persist-<N>bots
    port  = BASE_PORT + len(BOT_COUNTS) + BOT_COUNTS.index(bots)
    login = f"ccrace{bots}"
    work  = os.path.join(SERVER_BUILD, f"race-{bots}")
    stage_run_dir(work, staged_datapack, port, bots * 2 + 8)

    proc, logpath = launch_server(work)
    if proc is None:
        log_fail(case_race, f"server failed to bind; see {logpath}")
        print(read_log(logpath)[-2000:])
        return

    accounts = -1
    onfile = -1
    bots_rc = None
    bots_out = ""
    try:
        with phase_timer.phase(f"race-{bots}bots"):
            bots_rc, bots_out = run_bots(work, port, login, bots,
                                         False, BOT_TIMEOUT)
        onmap = re.search(r"^ON_MAP\s+(\d+)/(\d+)\s*$", bots_out, re.M)
        onmap_txt = onmap.group(0) if onmap else "ON_MAP <absent>"
        accounts = count_account_files(work)
        onfile = count_character_files(work)
        log_info(f"race: {bots} bots on a fresh login, rc={bots_rc}, "
                 f"{onmap_txt}, accounts={accounts}, onfile={onfile}")
    finally:
        rc = stop_server(proc)

    log_text = read_log(logpath)

    if should_run(case_race, failed):
        numbers = f"{bots} bots on one fresh login: accounts={accounts} onfile={onfile}"
        if accounts == 1:
            log_pass(case_race, numbers)
        else:
            _fc.set_extras(case_race,
                           run_output=bots_out + "\n=== server log ===\n" + log_text)
            log_fail(case_race,
                     f"{numbers} — {accounts} account record(s) for ONE login "
                     f"(expected 1): the losers of the create-account race got "
                     f"their own account instead of 'Login already used', so the "
                     f"login file points at the last writer and the characters on "
                     f"the earlier records are unreachable")
            print(bots_out[-2000:])

    if should_run(case_clean, failed):
        crashed = process_helpers.is_crash(rc)
        if crashed:
            _fc.set_extras(case_clean, run_output=log_text)
            log_fail(case_clean, f"server crashed (returncode {rc})")
            print(log_text[-2000:])
        else:
            log_pass(case_clean, f"no crash, stopped with returncode {rc}")


def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — account character-list persistence")
    print(f"{'='*60}{C_RESET}\n")

    failed = load_failed_cases()
    if failed is not None and len(failed) == 0:
        log_info("all previously passed (delete failed.json to re-run)")
        return

    if not DATAPACKS:
        log_fail("datapack", "no datapack configured (config paths.datapacks)")
        summary(); return
    staged = datapack_stage.staged_local(DATAPACKS[0])
    if not os.path.isdir(staged):
        log_fail("datapack", f"staged datapack missing: {staged}")
        summary(); return

    if not build_one(SERVER_PRO, SERVER_BUILD, SERVER_BIN):
        summary(); return
    if not build_one(BOT_PRO, BOT_BUILD, BOT_BIN):
        summary(); return

    for bots in BOT_COUNTS:
        run_case(bots, staged, failed)
        run_race_case(bots, staged, failed)
    summary()


if __name__ == "__main__":
    main()
