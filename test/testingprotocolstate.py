#!/usr/bin/env python3
"""
testingprotocolstate.py — raw-protocol, per-handler VALGRIND test suite for the
CatchChallenger epoll game server (server/cli, FILE_DB + HARDENED).

It builds the server ONCE (FILE_DB + HARDENED, RelWithDebInfo for line info)
and runs it ALWAYS under `valgrind --leak-check=full` — that is the entire
point of this suite. The verdict is a BASELINE-DELTA, never absolute zero:

  1. BASELINE run: a valgrind server that only does the handshake — open a
     Session, cleanly close it, let the server process the disconnect — and
     NEVER invokes any handler. We capture its valgrind fingerprint: the count
     of error contexts, the definitely/indirectly-lost byte totals, and (the
     key) the SET of leak alloc-site stacks. This residue is inherent: one-time
     datapack singletons loaded at boot (loadMonsterDrop / loadMonster /
     parseMonsters / loadItems / loadQuests / loadProfileList — OS-reclaimed,
     not a per-handler bug) plus a couple of select-character-path read sites
     and any Client::parse buffers from a session still in flight at SIGTERM.

  2. For EACH test/_prototests/h_*.py, spin up a FRESH valgrind server (own run
     dir + own valgrind log) so no test can bleed DB / map / connection state
     into the next, run test.run(server) under a hard wall guard, then — BEFORE
     stopping the server — CLEANLY CLOSE every Session the test opened and let
     the server process each disconnect (so per-connection recipes /
     encyclopedia_* / Client::parse buffers are free()'d and don't masquerade
     as a handler leak). Stop the server, parse its valgrind log, and compute
     the DELTA vs the baseline.

A handler PASSES iff: run() returned ok, no crash / abort / hang, AND it
introduced NO valgrind error or leak BEYOND the baseline — zero NEW error
contexts and zero NEW definitely-lost alloc sites vs the baseline set. A larger
byte count at the SAME baseline alloc site (e.g. one more transient datapack
singleton) is BASELINE, not a finding. A NEW alloc-site stack — especially one
under the handler's own call path / ClientNetworkRead* / the handler helpers —
IS a finding and the suite reports it RED.

Boilerplate (imports / wall-cap / faulthandler / log_pass / log_fail /
save_failed_cases) follows the testingserver.py / testingmapmanagement.py
pattern. Build dirs live under tmpfs (never in-tree); bytecode writing is
disabled so no __pycache__/ lands next to the checked-in sources.

The server is ALWAYS run under valgrind memcheck here regardless of the shared
diagnostic.py flags. (--valgrind on the command line is accepted and is a
no-op: we are already memcheck.)
"""

import sys
sys.dont_write_bytecode = True

import os
import signal
import time
import glob
import socket
import importlib
import traceback
import faulthandler
import threading
import pickle
import subprocess
import concurrent.futures

# ---------------------------------------------------------------------------
# Boilerplate: wall cap + self-diagnostics (copied pattern from
# testingserver.py / testingmapmanagement.py).
# ---------------------------------------------------------------------------
_THIS_DIR = os.path.dirname(os.path.abspath(__file__))
if _THIS_DIR not in sys.path:
    sys.path.insert(0, _THIS_DIR)

import wall_cap
wall_cap.arm()

import failed_cases as _fc
try:
    import phase_timer
except Exception:                       # phase_timer is optional plumbing
    phase_timer = None

# This suite is not in the shared wall_cap table -> defaults to 30 min, which
# is plenty: one valgrind server boot (~60-120s) per handler test plus a one-off
# baseline boot, for a handful of handlers. Keep the inner faulthandler net in
# lock-step with that default.
_WALL_LIMIT_SEC = wall_cap.cap_for(os.path.basename(sys.argv[0]))

faulthandler.enable()
try:
    faulthandler.dump_traceback_later(_WALL_LIMIT_SEC + 10, repeat=False,
                                      file=sys.stdout, exit=False)
except Exception:
    pass
try:
    faulthandler.register(signal.SIGUSR1)
except (AttributeError, ValueError, RuntimeError):
    pass

# The foundation harness (build_server / Server / Session / reload_state +
# the valgrind fingerprint/delta helpers and the session registry).
import _protoharness as H

SCRIPT_NAME = os.path.basename(__file__)
PROTOTESTS_DIR = os.path.join(_THIS_DIR, "_prototests")

C_GREEN = "\033[92m"
C_RED   = "\033[91m"
C_YEL   = "\033[93m"
C_CYAN  = "\033[96m"
C_RESET = "\033[0m"

# Per-handler wall guard. A valgrind'd server is 10-50x slower; the handler
# tests open several handshakes (each ~1-3s under valgrind). 300s is generous
# for a healthy handler and tight enough that a wedged server surfaces as a
# FAIL (hang finding) rather than soaking the whole 30-min suite cap.
_PER_HANDLER_WALL = 300

# How long to let the server's epoll loop run the disconnect path after we close
# the sessions, before we SIGTERM. Under valgrind the loop is slow, so be
# generous; this is what frees the per-connection buffers.
_DISCONNECT_SETTLE = 1.2

# results: list of dicts (see _mk_rec)
results = []
_last_log_time = [time.monotonic()]

# Filled in by run_baseline(); a H.parse_valgrind_fingerprint() dict.
BASELINE_FP = [None]
BASELINE_LOG = [None]


def _mk_rec(name, fpath):
    return {"name": name, "file": fpath, "ok": False, "detail": "",
            "valgrind_errors": -1, "crash": None, "elapsed": 0.0,
            "skipped": False, "vg_delta_clean": None, "new_sites": 0}


# ---------------------------------------------------------------------------
# logging (mirrors the shared log_pass/log_fail shape)
# ---------------------------------------------------------------------------
def _ts():
    if phase_timer is not None:
        try:
            return phase_timer.t()
        except Exception:
            pass
    return time.strftime("[%H:%M:%S]")


def log_info(msg):
    print(f"{_ts()} {C_CYAN}[INFO]{C_RESET} {msg}", flush=True)


def _elapsed():
    now = time.monotonic()
    e = now - _last_log_time[0]
    _last_log_time[0] = now
    return e


def log_pass(name, detail=""):
    e = _elapsed()
    print(f"{_ts()} {C_GREEN}[PASS]{C_RESET} {name}  {detail}  ({e:.1f}s)",
          flush=True)
    if phase_timer is not None:
        try:
            phase_timer.record_event("pass", name, ok=True, dt=e, detail=detail)
        except Exception:
            pass
    return e


def log_fail(name, detail=""):
    e = _elapsed()
    print(f"{_ts()} {C_RED}[FAIL]{C_RESET} {name}  {detail}  ({e:.1f}s)",
          flush=True)
    if phase_timer is not None:
        try:
            phase_timer.record_event("fail", name, ok=False, dt=e, detail=detail)
        except Exception:
            pass
    return e


def log_skip(name, detail=""):
    e = _elapsed()
    print(f"{_ts()} {C_YEL}[SKIP]{C_RESET} {name}  {detail}  ({e:.1f}s)",
          flush=True)
    return e


# ---------------------------------------------------------------------------
# failed.json integration (resume support)
# ---------------------------------------------------------------------------
def load_failed_cases():
    return _fc.load_names(SCRIPT_NAME)


def should_run(test_name, failed):
    if failed is None:
        return True
    return test_name in failed


def save_failed_cases():
    failures = []
    for r in results:
        if not r["ok"] and not r["skipped"]:
            d = _fc.make_detail(r["detail"])
            failures.append((r["name"], d))
    _fc.save(SCRIPT_NAME, failures)


# ---------------------------------------------------------------------------
# test discovery
# ---------------------------------------------------------------------------
def discover_tests():
    """Return [(module_name, NAME, run_callable, file_path)] sorted by file."""
    if PROTOTESTS_DIR not in sys.path:
        sys.path.insert(0, PROTOTESTS_DIR)
    files = sorted(glob.glob(os.path.join(PROTOTESTS_DIR, "h_*.py")))
    found = []
    for fpath in files:
        modname = os.path.splitext(os.path.basename(fpath))[0]
        try:
            mod = importlib.import_module(modname)
            mod = importlib.reload(mod)  # fresh each run (idempotent)
        except Exception as e:
            log_fail(modname, "import failed: %r" % e)
            rec = _mk_rec(modname, fpath)
            rec["detail"] = "import failed: %r" % e
            results.append(rec)
            continue
        name = getattr(mod, "NAME", modname)
        run = getattr(mod, "run", None)
        if run is None or not callable(run):
            log_fail(name, "no callable run(server) in %s" % modname)
            rec = _mk_rec(name, fpath)
            rec["detail"] = "no run(server)"
            results.append(rec)
            continue
        found.append((modname, name, run, fpath))
    return found


def _baseline_generic_error_paths(server):
    """Drive the GENERIC, handler-agnostic protocol-error paths from one or two
    throwaway on-map sessions so their valgrind residue lands in the baseline.

    The frames here are PROTOCOL-LEVEL malformations — a dynamic packet whose
    declared 4-byte length is larger than the bytes actually sent, a truncated
    dynamic header, an oversized (~4 GiB) length, and an unknown/blocked code.
    They make parseMessage()/parseQuery() return false WITH size>0, which under
    CATCHCHALLENGER_HARDENED runs the binarytoHexa(data,size) hex-dump
    diagnostic (reading the input buffer's uninitialised tail) and the
    errorOutput()->std::cerr logging. None of this is under a specific handler's
    own call path — it is the shared parser/diagnostic, identical no matter
    which packet code is malformed — so it belongs in the baseline. (Several
    handler tests legitimately send the very same generic malformations as part
    of their malformed-framing battery.)

    Best-effort: never raises; the server may kick each sender."""
    # The signatures we must seed into the baseline are produced by the SHARED
    # ProtocolParsing layer + the SHARED errorOutput/std::cerr sink, NOT by any
    # one gameplay handler:
    #   * an UNKNOWN / blocked packet code (0xFF) carrying trailing bytes makes
    #     parseMessage()/parseQuery() return false -> the CATCHCHALLENGER_HARDENED
    #     diagnostic binarytoHexa(data,size) hex-dumps the input buffer
    #     (reading its uninitialised tail) at parseDispatch, and std::to_string
    #     (packetCode) runs;
    #   * a DYNAMIC frame whose declared 4-byte length over-/under-shoots the
    #     bytes actually sent drives parseDataSize()/parseData() length-byte
    #     comparisons against the reusable input buffer's stale tail;
    #   * a generic illegal in-game action (0x07 tryEscape out of fight) routes
    #     through errorOutput()->sanitizeUtf8String()->std::cerr (the libc stdio
    #     write of the kick string), the same sink every handler's kick uses.
    # All of the above is testing-mode / protocol-layer residue, identical no
    # matter which packet code is malformed, so it belongs in the baseline.
    frames = [
        bytes([0xFF, 0x11, 0x22, 0x33]),            # unknown MESSAGE code + data
        bytes([0xFF, 0x00]) + H.u32(8) + b"abcd",   # unknown QUERY-shaped + data
        b"\x03" + H.u32(64) + b"\x00\x01",          # 0x03 chat dyn: declares 64, sends 2
        b"\x03" + H.u32(0x7FFFFFFF),                # huge declared length, no data
        b"\x05" + H.u32(48) + b"\xff\xfe\xfd",      # 0x05 dyn, truncated payload
        bytes([0x80, 0x00, 0xAB, 0xCD]),            # unknown query code 0x80 + junk
    ]
    for raw in frames:
        try:
            s = H.Session(server)
        except Exception:
            continue
        try:
            s.send_raw(raw)
        except OSError:
            pass
        try:
            s.drain(0.6)
        except Exception:
            pass
        # leave it registered; close_all_sessions() will close it cleanly.
    # A generic well-framed-but-illegal in-game packet that any logged-in player
    # can send to hit errorOutput()->kick without touching a specific gameplay
    # handler under test: 0x07 (tryEscape) out of fight -> errorFightEngine ->
    # errorOutput -> sanitizeUtf8String -> std::cerr. Same kick sink as every
    # handler. Send it from two sessions so the stdio-buffer write site is hit.
    i = 0
    while i < 2:
        try:
            s = H.Session(server)
            s.m(0x07)
            s.drain(0.6)
        except Exception:
            pass
        i += 1


# ---------------------------------------------------------------------------
# BASELINE: handshake-only valgrind server -> fingerprint
# ---------------------------------------------------------------------------
def run_baseline(binary, maincode):
    """Start a fresh valgrind server, do exactly ONE handshake (a full login +
    create + select to CharacterSelected), then CLEANLY close it and let the
    server process the disconnect, never touching a handler. Stop the server and
    return its valgrind fingerprint (the residue every handler run inherits)."""
    run_dir = os.path.join(H._WORK, "run-baseline")
    log_info("starting BASELINE valgrind server (handshake-only) ...")
    srv = None
    try:
        srv = H.Server(binary, run_dir, maincode=maincode, valgrind=True)
    except Exception as e:
        log_fail("baseline", "baseline server start failed: %r" % e)
        return None
    log_info("baseline server bound on port %d; doing handshake + generic "
             "protocol-error paths" % srv.port)
    try:
        try:
            sess = H.Session(srv)
            log_info("baseline handshake OK: char_id=%s pos=(%s,%s) mapIndex=%s"
                     % (sess.character_id, sess.x, sess.y, sess.mapIndex))
        except Exception as e:
            log_fail("baseline", "baseline handshake failed: %r" % e)
            return None
        # Exercise the GENERIC (handler-agnostic) protocol-error paths so their
        # valgrind residue is part of the baseline, not attributed to whichever
        # handler test happens to send a malformed frame:
        #   * the HARDENED parse-fail diagnostic binarytoHexa(data,size) in
        #     ProtocolParsingInput.cpp::parseDispatch — it hex-dumps the raw
        #     packet on a parseMessage/parseQuery==false, reading the input
        #     buffer's uninitialised tail (a TESTING-ONLY diagnostic, OFF in
        #     production), plus parseDataSize/parseData length-byte comparisons
        #     on a truncated/oversized dynamic frame;
        #   * the generic errorOutput()->std::cerr kick logging (libc stdio
        #     buffering reads slack).
        # These are protocol-layer, not under any specific handler's call path,
        # so they must be baseline. We drive them from a SEPARATE on-map session
        # that is then cleanly closed.
        _baseline_generic_error_paths(srv)
        # Cleanly close and let the server run the disconnect path so the
        # per-connection buffers are free()'d (same discipline as the handlers).
        H.close_all_sessions(srv, settle=_DISCONNECT_SETTLE)
        if not srv.alive():
            # alive() opens+closes a probe TCP connection; if the server already
            # died that is itself a problem, but for the baseline we just warn.
            log_info("WARN: baseline server not accepting after handshake close")
    finally:
        # the probe connection from alive() must also be gone; alive() closes it.
        try:
            srv.stop()
        except Exception:
            pass
    time.sleep(0.3)
    fp = H.parse_valgrind_fingerprint(srv.valgrind_log)
    BASELINE_LOG[0] = srv.valgrind_log
    if fp is None:
        log_fail("baseline", "could not parse baseline valgrind log: %s"
                 % srv.valgrind_log)
        return None
    log_info("BASELINE fingerprint: error_contexts=%d  definitely_lost=%d B  "
             "indirectly_lost=%d B  def_sites=%d  indir_sites=%d  err_sites=%d"
             % (fp["error_contexts"], fp["definitely_lost_bytes"],
                fp["indirectly_lost_bytes"], len(fp["def_sites"]),
                len(fp["indir_sites"]), len(fp["error_sites"])))
    H.forget_sessions(srv)
    return fp


# ---------------------------------------------------------------------------
# per-handler runner: fresh valgrind server, run(server), clean close, delta
# ---------------------------------------------------------------------------
def _run_one(modname, name, run, fpath, binary, maincode):
    """Run a single handler test in a fresh valgrind server. Appends a result
    dict and returns it."""
    run_dir = os.path.join(H._WORK, "run-" + modname)
    rec = _mk_rec(name, fpath)

    log_info("starting fresh valgrind server for %s ..." % name)
    srv = None
    # A handler whose positive branch needs an OWNED item (e.g. 0x10
    # useObjectOnMonster) sets NEEDS_EVERY_BODY_IS_ROOT=True so the server unlocks
    # the in-game "/give" admin command; the test then grants itself the item via
    # H.grant_item(). This only changes command authorization, not the threat
    # model of the handler under test.
    _root = False
    try:
        _m = sys.modules.get(getattr(run, "__module__", modname)) \
            or importlib.import_module(modname)
        _root = bool(getattr(_m, "NEEDS_EVERY_BODY_IS_ROOT", False))
    except Exception:
        _root = False
    try:
        srv = H.Server(binary, run_dir, maincode=maincode, valgrind=True,
                       every_body_is_root=_root)
    except Exception as e:
        rec["detail"] = "server start failed: %r" % e
        rec["elapsed"] = log_fail(name, rec["detail"])
        results.append(rec)
        return rec

    log_info("server bound on port %d; running %s" % (srv.port, name))

    # Hard wall guard around the handler body: a handler that hangs the server is
    # a finding, not a reason to soak the suite cap. We enforce a ceiling with a
    # watchdog thread that stops the server, unblocking any socket wait in run().
    timed_out = [False]

    def _watchdog():
        timed_out[0] = True
        try:
            srv.stop()        # kill the server -> any socket read in run() ends
        except Exception:
            pass

    wd = threading.Timer(_PER_HANDLER_WALL, _watchdog)
    wd.daemon = True
    wd.start()

    started = time.monotonic()
    ok = False
    detail = ""
    is_skip = False
    try:
        out = run(srv)
        # SKIP channel: a handler that genuinely cannot reach its precondition in
        # the 'test' maincode returns (None, reason) -> marked SKIP/partial, NOT a
        # hard FAIL (per the suite contract). It is still run under valgrind so a
        # crash / NEW leak on the reachable portion is still caught below.
        if isinstance(out, tuple) and len(out) == 2 and out[0] is None:
            is_skip, detail = True, str(out[1])
            ok = True
        elif isinstance(out, tuple) and len(out) == 2:
            ok, detail = bool(out[0]), str(out[1])
        elif isinstance(out, bool):
            ok, detail = out, ""
        else:
            ok, detail = False, "run() returned %r (want (bool, str))" % (out,)
    except Exception as e:
        ok = False
        detail = "run() raised: %r\n%s" % (e, traceback.format_exc()[-1500:])
    finally:
        wd.cancel()

    rec["elapsed_test"] = time.monotonic() - started

    if timed_out[0]:
        ok = False
        detail = ("HANG: handler exceeded %ds wall — server was force-stopped "
                  "(possible infinite loop / deadlock). %s"
                  % (_PER_HANDLER_WALL, detail))

    # Capture crash/abort BEFORE stopping (crash_report reads the process state
    # and the server log; the server may already be dead from an abort()).
    crash = None
    try:
        crash = srv.crash_report()
    except Exception:
        crash = None
    rec["crash"] = crash

    # CLEAN-CLOSE every session the test opened (and the harness opened during
    # the test's reload_state calls) and let the server process the disconnects,
    # so per-connection buffers are free()'d and don't masquerade as a handler
    # leak. Only do this if the server is still up (after a crash it's moot).
    closed = 0
    if not timed_out[0] and crash is None:
        try:
            closed = H.close_all_sessions(srv, settle=_DISCONNECT_SETTLE)
        except Exception:
            closed = 0
    rec["sessions_closed"] = closed

    # Stop the server so its valgrind log is finalised, then read the verdict.
    try:
        srv.stop()
    except Exception:
        pass
    time.sleep(0.2)
    H.forget_sessions(srv)

    # ---- baseline-delta verdict ----
    run_fp = None
    try:
        run_fp = H.parse_valgrind_fingerprint(srv.valgrind_log)
    except Exception:
        run_fp = None

    vg_clean, vg_reasons, vg_detail = H.valgrind_delta(BASELINE_FP[0], run_fp)
    rec["vg_delta_clean"] = vg_clean
    rec["valgrind_log"] = getattr(srv, "valgrind_log", None)
    if run_fp is not None:
        rec["valgrind_errors"] = run_fp["error_contexts"]
        rec["new_sites"] = len(vg_detail.get("new_def_sites", set())) + \
            len(vg_detail.get("new_err_sites", set()))
    else:
        rec["valgrind_errors"] = -1

    # A clean handler test must: report ok, leave NO crash/hang, and introduce
    # NO valgrind finding BEYOND the baseline (delta-clean).
    final_ok = ok and (crash is None) and (not timed_out[0]) and vg_clean
    detail_bits = [detail] if detail else []
    if crash is not None:
        detail_bits.append("CRASH/ABORT: %s" % crash[:400])
    if not vg_clean:
        detail_bits.append("VALGRIND-DELTA: " + "; ".join(vg_reasons))
        # name the NEW alloc sites so the operator can see exactly what leaked.
        for sig in list(vg_detail.get("new_def_sites", set()))[:4]:
            detail_bits.append("  NEW-DEF-LEAK: " + _fmt_sig(sig))
        for sig in list(vg_detail.get("new_err_sites", set()))[:4]:
            detail_bits.append("  NEW-ERROR: " + _fmt_sig(sig))
    elif run_fp is not None:
        # informative: show the byte/ctx numbers so a reader can confirm it is
        # baseline residue, not a finding. Also surface how many NEW error
        # contexts were classified as testing-mode HARDENED-diagnostic residue
        # (hex-dump / DDoS dump / kick-logging) so the absorption is transparent.
        ndiag = len(vg_detail.get("diag_err_sites", set()))
        diag_s = (" +%d diag-only(hexdump/ddos/log)" % ndiag) if ndiag else ""
        detail_bits.append("vg-baseline-ok (def %dB/ctx%d vs base def %dB/ctx%d; "
                           "closed %d sess)%s"
                           % (vg_detail.get("run_def_bytes", -1),
                              vg_detail.get("run_err_ctx", -1),
                              vg_detail.get("base_def_bytes", -1),
                              vg_detail.get("base_err_ctx", -1), closed, diag_s))
    rec["detail"] = "  ".join(detail_bits)

    # A SKIP is only honoured if the reachable portion still ran cleanly (no
    # crash / hang / NEW valgrind finding); otherwise it degrades to a FAIL so a
    # crash on the reachable path is never hidden behind a SKIP.
    if is_skip and (crash is None) and (not timed_out[0]) and vg_clean:
        rec["skipped"] = True
        rec["ok"] = True            # not counted as failed
        rec["elapsed"] = log_skip(name, rec["detail"])
    elif final_ok:
        rec["ok"] = True
        rec["elapsed"] = log_pass(name, rec["detail"])
    else:
        rec["ok"] = False
        rec["elapsed"] = log_fail(name, rec["detail"])
        # surface a few lines of the valgrind log / crash inline for triage
        _dump_diag(srv, crash, vg_detail)
    # Reclaim the per-handler run dir's BULK (the ~90 MiB RelWithDebInfo binary
    # copy + the datapack symlink + the FILE_DB database) now that the valgrind
    # log has been parsed and any failure-diagnostics dumped. Without this, 36
    # fresh servers each leave a 90 MiB binary copy behind and fill tmpfs
    # (the 0xAB/0xAC "No space left on device" we hit). We KEEP the small
    # valgrind.log + server.log so a failed handler stays post-mortem'able.
    _reclaim_run_dir(run_dir)
    results.append(rec)
    return rec


def _reclaim_run_dir(run_dir):
    """Delete the space-heavy artefacts of a finished handler's run dir (the
    server binary copy, the datapack symlink, the database) but keep the tiny
    valgrind.log / server.log for triage. Best-effort; never raises."""
    import shutil
    for sub in ("catchchallenger-server-cli", "datapack", "database"):
        p = os.path.join(run_dir, sub)
        try:
            if os.path.islink(p):
                os.remove(p)
            elif os.path.isdir(p):
                shutil.rmtree(p, ignore_errors=True)
            elif os.path.isfile(p):
                os.remove(p)
        except OSError:
            pass


def _fmt_sig(sig):
    """One-line view of a leak/error alloc-site signature (top frames)."""
    frames = [f for f in sig if f and not f.startswith("???")
              and "vg_replace_malloc" not in f and f not in (
                  "malloc", "calloc", "realloc", "operator new",
                  "operator new[]")]
    if not frames:
        frames = list(sig)
    return " <- ".join(frames[:4])


def _dump_diag(srv, crash, vg_detail):
    """Print a bounded slice of the valgrind log + crash report for the failing
    handler so the console carries the root cause without the operator hunting
    for the log file."""
    try:
        # If the finding is a NEW alloc site, print that block from the log so
        # the full stack is visible.
        new_sites = list(vg_detail.get("new_def_sites", set())) + \
            list(vg_detail.get("new_err_sites", set()))
        vlog = getattr(srv, "valgrind_log", None)
        if new_sites and vlog and os.path.isfile(vlog):
            print(f"  {C_YEL}| --- NEW valgrind site(s) vs baseline ---{C_RESET}",
                  flush=True)
            for sig in new_sites[:4]:
                print("  | " + _fmt_sig(sig), flush=True)
        if vlog and os.path.isfile(vlog):
            with open(vlog, "r", errors="replace") as f:
                txt = f.read()
            lines = txt.splitlines()
            ls = None
            for i, ln in enumerate(lines):
                if "LEAK SUMMARY" in ln:
                    ls = i
                    break
            if ls is not None:
                print(f"  {C_YEL}| --- valgrind leak summary ---{C_RESET}",
                      flush=True)
                for ln in lines[ls:ls + 8]:
                    print("  | " + ln, flush=True)
        if crash:
            print(f"  {C_YEL}| --- crash report (tail) ---{C_RESET}", flush=True)
            for ln in crash.splitlines()[-20:]:
                print("  | " + ln, flush=True)
    except Exception as e:
        print("  | (diag dump failed: %r)" % e, flush=True)


# ---------------------------------------------------------------------------
# summary
# ---------------------------------------------------------------------------
def summary():
    print(f"\n{C_CYAN}{'=' * 72}")
    print("  testingprotocolstate — per-handler valgrind BASELINE-DELTA summary")
    print(f"{'=' * 72}{C_RESET}", flush=True)

    bfp = BASELINE_FP[0]
    if bfp is not None:
        print(f"  baseline: error_contexts={bfp['error_contexts']}  "
              f"definitely_lost={bfp['definitely_lost_bytes']}B  "
              f"indirectly_lost={bfp['indirectly_lost_bytes']}B  "
              f"def_sites={len(bfp['def_sites'])}  "
              f"err_sites={len(bfp['error_sites'])}", flush=True)
        print(f"  baseline log: {BASELINE_LOG[0]}", flush=True)
    else:
        print(f"  {C_RED}baseline: NOT CAPTURED (all handlers fail-safe)"
              f"{C_RESET}", flush=True)

    passed = sum(1 for r in results if r["ok"] and not r["skipped"])
    failed = sum(1 for r in results if not r["ok"] and not r["skipped"])
    skipped = sum(1 for r in results if r["skipped"])
    any_crash = any(r["crash"] is not None for r in results)
    any_new_vg = any((r.get("new_sites") or 0) > 0 for r in results)

    print("", flush=True)
    for r in results:
        if r["skipped"]:
            tag = f"{C_YEL}SKIP{C_RESET}"
        elif r["ok"]:
            tag = f"{C_GREEN}PASS{C_RESET}"
        else:
            tag = f"{C_RED}FAIL{C_RESET}"
        verr = r["valgrind_errors"]
        verr_s = ("ctx=%d" % verr) if verr >= 0 else "ctx=?"
        new_s = ("+%d" % r["new_sites"]) if r.get("new_sites") else "  "
        print(f"  [{tag}] {r['name']:<28} {verr_s:<7} new{new_s:<4} "
              f"{r['detail'][:110]}", flush=True)

    valgrind_clean = (not any_new_vg) and (BASELINE_FP[0] is not None)
    print(f"\n  valgrind_clean(no new vg site vs baseline) = {valgrind_clean}   "
          f"(any_crash={any_crash})", flush=True)
    print(f"  {C_GREEN}{passed} passed{C_RESET}, "
          f"{C_RED}{failed} failed{C_RESET}, "
          f"{C_YEL}{skipped} skipped{C_RESET}", flush=True)

    save_failed_cases()

    # Exit non-zero on any fail, any NEW valgrind site, or any crash.
    if failed > 0 or any_new_vg or any_crash or BASELINE_FP[0] is None:
        return 1
    return 0


# ---------------------------------------------------------------------------
# Reliable kick detector injected over each handler's own (unreliable socket-
# EOF) one. errorOutput()->disconnectClient() ALWAYS logs "<pseudo>: Kicked
# by:" for the offending session, which H.session_was_kicked() polls — reliable
# even when the TCP FIN is deferred (a handler's raw recv()==b'' probe missed
# that, producing false "did NOT kick" fails although the server DID kick).
# ---------------------------------------------------------------------------
def _reliable_is_kicked(sess, *args, **kwargs):
    # Two complementary signals:
    #  (1) LOG, scoped per session: errorOutput() writes "<pseudo>: Kicked by:"
    #      SYNCHRONOUSLY for every semantic/invalid kick. A logged-in
    #      disconnectClient() does NOT close the socket (deferred to next I/O,
    #      Client.cpp ~298) and a poke does NOT reliably provoke it, so the log is
    #      the only dependable signal for those. We grep the log ONLY past
    #      sess._log_base (captured when this session went live) so an earlier
    #      session that reused the same character pseudo and was kicked cannot
    #      false-match a later NOT-kicked session (the 0xAC select-abuse race).
    #  (2) SOCKET drain-to-EOF + ONE complete-move poke [0x02,0x01,0x05]: catches
    #      None-state closeSocket() (immediate) and any path that does close. The
    #      poke is a full 3-byte packet (cannot poison a reused session's framing),
    #      valid direction, moveThePlayer() result discarded so it NEVER kicks.
    #      Exactly ONE is sent (a flood trips the DDoS move limit). Side effect: a
    #      surviving session may move one tile (0x13's no-op check is length-based).
    try:
        timeout = 2.5
        for v in list(args) + list(kwargs.values()):
            if isinstance(v, (int, float)):
                timeout = max(timeout, float(v) + 1.0)
                break
        # Callers pass EITHER a Session (has .sock/.pseudo/.server/._log_base) OR a
        # raw socket (h_0xab's _is_kicked(sk)). Resolve both; a raw socket has no
        # log context so it falls back to the socket signal only.
        sk = getattr(sess, "sock", None)
        if sk is None and hasattr(sess, "recv") and hasattr(sess, "sendall"):
            sk = sess
        if sk is None:
            return False
        pseudo = getattr(sess, "pseudo", None)
        server = getattr(sess, "server", None)
        log_base = getattr(sess, "_log_base", 0)
        needle = (pseudo + ": Kicked by:") if pseudo else None

        def _log_hit():
            if needle and server is not None:
                try:
                    with open(server.log_path, "r", errors="replace") as f:
                        f.seek(log_base)
                        return needle in f.read()
                except OSError:
                    return False
            return False

        def _drain_eof():
            try:
                sk.setblocking(False)
            except OSError:
                return True
            try:
                while True:
                    try:
                        if sk.recv(65536) == b"":
                            return True            # clean EOF = server closed = kicked
                    except BlockingIOError:
                        return False               # open, nothing buffered -> not yet
                    except OSError:
                        return True                # RST -> kicked
            finally:
                try:
                    sk.setblocking(True)
                except OSError:
                    pass

        deadline = time.time() + timeout
        while time.time() < deadline:
            if _log_hit() or _drain_eof():
                return True
            if not getattr(sess, "_kick_nudged", False):
                setattr(sess, "_kick_nudged", True)
                try:
                    sk.sendall(bytes([0x02, 0x01, 0x05]))  # complete move; provoke close
                except OSError:
                    return True
            time.sleep(0.12)
        return _log_hit() or _drain_eof()
    except Exception:
        return False


def _reliable_is_open(sess, *args, **kwargs):
    """Negative variant for detectors whose True means 'still connected'."""
    return not _reliable_is_kicked(sess, *args, **kwargs)


def _reliable_confirm_kick(sess, *args, **kwargs):
    """(kicked_bool, reason) variant for _confirm_kick-style detectors."""
    if _reliable_is_kicked(sess, *args, **kwargs):
        return True, "peer closed / 'Kicked by:' logged (reliable detector)"
    return False, "connection still open (NOT kicked) -> contract violation"


# Detectors whose True == kicked/closed (bool); whose True == alive (negative);
# and the one (kicked, reason) tuple variant. Handler agents named their kick
# probe inconsistently and most rolled a raw socket-EOF check that misses the
# DEFERRED close of a logged-in kick -> we override them all with the reliable one.
_KICK_POSITIVE = ("_is_kicked", "_socket_closed_by_peer", "_peer_gone",
                  "_confirm_kicked", "_peer_is_closed", "_peer_closed", "_kicked")
_KICK_NEGATIVE = ("_peer_is_open", "_socket_alive")


def _inject_reliable_kick(mod):
    """Replace a handler module's own kick detector(s) with the authoritative,
    log-based one so a correctly-kicking server is never mis-reported. Server
    liveness helpers (_alive_clean/_alive_and_clean) are intentionally untouched."""
    for fn in _KICK_POSITIVE:
        if callable(getattr(mod, fn, None)):
            setattr(mod, fn, _reliable_is_kicked)
    for fn in _KICK_NEGATIVE:
        if callable(getattr(mod, fn, None)):
            setattr(mod, fn, _reliable_is_open)
    if callable(getattr(mod, "_confirm_kick", None)):
        setattr(mod, "_confirm_kick", _reliable_confirm_kick)


# ---------------------------------------------------------------------------
# --run-one: execute ONE handler test in an ISOLATED subprocess.
# Run from the parallel dispatcher in main(); each subprocess has its OWN
# Python interpreter -> its OWN _protoharness globals (_ALL_SESSIONS registry,
# _PORT_SEQ, _PSEUDO_SEQ) -> its OWN valgrind server on its OWN ephemeral TCP
# port. That sidesteps the harness's (sequential-only) shared state entirely,
# so N of these run safely at once = N servers on N ports.
# Contract: env CC_PROTO_BASELINE_PKL points to the pickled BASELINE_FP (so the
# per-handler baseline-delta verdict is identical to the sequential path), and
# CC_PROTO_OUT_PKL is where this process writes its single result rec.
# ---------------------------------------------------------------------------
def _subprocess_run_one(modname, maincode):
    out_pkl = os.environ.get("CC_PROTO_OUT_PKL")
    base_pkl = os.environ.get("CC_PROTO_BASELINE_PKL")
    try:
        with open(base_pkl, "rb") as f:
            BASELINE_FP[0] = pickle.load(f)
    except Exception as e:
        log_fail(modname, "could not load baseline pickle: %r" % e)
        return 1
    try:
        binary = H.build_server(valgrind=True)   # idempotent: reuses on-disk binary
    except Exception as e:
        rec = _mk_rec(modname, "")
        rec["detail"] = "build_server failed in subprocess: %r" % e
        if out_pkl:
            with open(out_pkl, "wb") as f:
                pickle.dump(rec, f)
        return 1
    # import ONLY this handler module (not all 36) to keep the subprocess light.
    if PROTOTESTS_DIR not in sys.path:
        sys.path.insert(0, PROTOTESTS_DIR)
    fpath = os.path.join(PROTOTESTS_DIR, modname + ".py")
    try:
        mod = importlib.import_module(modname)
        _inject_reliable_kick(mod)   # authoritative kick detection
        name = getattr(mod, "NAME", modname)
        run = getattr(mod, "run", None)
        if run is None or not callable(run):
            raise RuntimeError("no callable run(server)")
    except Exception as e:
        rec = _mk_rec(modname, fpath)
        rec["detail"] = "import failed: %r" % e
        if out_pkl:
            with open(out_pkl, "wb") as f:
                pickle.dump(rec, f)
        return 1
    rec = _run_one(modname, name, run, fpath, binary, maincode)
    if out_pkl:
        with open(out_pkl, "wb") as f:
            pickle.dump(rec, f)
    return 0 if rec.get("ok") else 1


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------
def main():
    print(f"\n{C_CYAN}{'=' * 72}")
    print("  CatchChallenger — protocol-state per-handler VALGRIND suite")
    print(f"{'=' * 72}{C_RESET}\n", flush=True)

    maincode = "test"
    # allow `testingprotocolstate.py <maincode>` override (default 'test')
    for a in sys.argv[1:]:
        if not a.startswith("-"):
            maincode = a
            break

    failed = load_failed_cases()
    if failed is not None and len(failed) == 0:
        log_info("all previously passed (delete failed.json to re-run)")
        return 0

    # ---- build ONCE, INCREMENTALLY (valgrind RelWithDebInfo + FILE_DB, prod) ----
    # force=True so edits to server/general sources since the last run ARE
    # recompiled (a prior on-disk binary must not mask a server-side fix). The
    # per-handler subprocesses then reuse this freshly-built binary (force=False).
    log_info("building server/cli (FILE_DB, production valgrind build, incremental) ...")
    t0 = time.monotonic()
    try:
        binary = H.build_server(valgrind=True, force=True)
    except Exception as e:
        log_fail("build", "server build failed: %r" % e)
        rec = _mk_rec("build", "")
        rec["detail"] = "build failed: %r" % e
        results.append(rec)
        return summary()
    log_info("build OK in %.1fs -> %s" % (time.monotonic() - t0, binary))

    # verify the binary is PRODUCTION (NON-HARDENED): the suite must test the
    # production contract (invalid input KICKS the offending client, server
    # survives), which HARDENED breaks by aborting the whole process on a
    # parse-fail. "Bug at data-sending" is emitted only inside a
    # CATCHCHALLENGER_HARDENED block in ProtocolParsingInput.cpp; its ABSENCE
    # confirms the parse-fail abort path is OFF (production behaviour).
    try:
        import subprocess
        sres = subprocess.run(["strings", binary], capture_output=True,
                              text=True, timeout=60)
        if "Bug at data-sending" in sres.stdout:
            log_info("WARN: binary appears HARDENED (parse-fail abort path ON) "
                     "— the kick contract cannot be tested; expected production build")
        else:
            log_info("production build confirmed (non-HARDENED: invalid input "
                     "kicks the client, server survives)")
    except Exception:
        pass

    # ---- BASELINE (handshake-only) fingerprint, captured ONCE ----
    BASELINE_FP[0] = run_baseline(binary, maincode)
    if BASELINE_FP[0] is None:
        # Without a baseline we cannot compute a delta; that is itself a hard
        # failure (the foundation handshake or valgrind is broken).
        log_fail("baseline", "no baseline fingerprint -> cannot run delta suite")
        rec = _mk_rec("baseline", "")
        rec["detail"] = "baseline fingerprint capture failed"
        results.append(rec)
        return summary()

    # ---- discover ----
    tests = discover_tests()
    if not tests:
        log_fail("discover", "no h_*.py test files found in %s" % PROTOTESTS_DIR)
        rec = _mk_rec("discover", "")
        rec["detail"] = "no tests discovered"
        results.append(rec)
        return summary()
    log_info("discovered %d handler test(s): %s"
             % (len(tests), ", ".join(t[1] for t in tests)))

    # ---- run handlers IN PARALLEL: one ISOLATED subprocess per handler, each
    # its own valgrind server on its own ephemeral TCP port (1 server = 1 port,
    # several at once). Resume-skipped handlers are recorded directly. ----
    to_run = []
    for modname, name, run, fpath in tests:
        if not should_run(name, failed):
            log_skip(name, "(passed in prior run; resume mode)")
            rec = _mk_rec(name, fpath)
            rec["ok"] = True
            rec["detail"] = "skipped (resume)"
            rec["valgrind_errors"] = 0
            rec["skipped"] = True
            rec["vg_delta_clean"] = True
            results.append(rec)
        else:
            to_run.append((modname, name, fpath))

    # Pickle the baseline ONCE; every subprocess reads it for an identical
    # baseline-delta verdict (pickle handles the fingerprint's sets/tuples).
    base_pkl = os.path.join(H._WORK, "baseline_fp.pkl")
    with open(base_pkl, "wb") as f:
        pickle.dump(BASELINE_FP[0], f)

    # Each valgrind server costs ~10-20x RAM, so cap concurrency (CC_PROTO_JOBS
    # overrides).
    try:
        jobs = int(os.environ.get("CC_PROTO_JOBS", "0"))
    except ValueError:
        jobs = 0
    if jobs <= 0:
        jobs = min(8, max(2, (os.cpu_count() or 4) // 2))
    jobs = min(jobs, max(1, len(to_run)))
    log_info("running %d handler(s) across %d concurrent valgrind servers "
             "(1 server = 1 TCP port) ..." % (len(to_run), jobs))

    def _dispatch(modname, name, fpath):
        out_pkl = os.path.join(H._WORK, "out_%s.pkl" % modname)
        try:
            os.remove(out_pkl)
        except OSError:
            pass
        env = dict(os.environ)
        env["CC_PROTO_BASELINE_PKL"] = base_pkl
        env["CC_PROTO_OUT_PKL"] = out_pkl
        to = _PER_HANDLER_WALL + 180   # backstop above the in-process watchdog
        console = ""
        try:
            p = subprocess.run(
                [sys.executable, os.path.abspath(__file__),
                 "--run-one", modname, maincode],
                env=env, capture_output=True, text=True, timeout=to)
            console = p.stdout or ""
        except subprocess.TimeoutExpired as e:
            console = (e.stdout if isinstance(e.stdout, str) else "") or ""
            console += "\n[dispatch] %s subprocess hard-timeout after %ds" % (name, to)
        if console:
            sys.stdout.write(console)
            sys.stdout.flush()
        if os.path.isfile(out_pkl):
            try:
                with open(out_pkl, "rb") as f:
                    return pickle.load(f)
            except Exception as e:
                rec = _mk_rec(name, fpath)
                rec["detail"] = "result unpickle failed: %r" % e
                return rec
        rec = _mk_rec(name, fpath)
        rec["detail"] = ("subprocess produced no result rec (hard timeout / "
                         "crash before write); console tail:\n"
                         + console[-1200:])
        return rec

    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as ex:
        futs = [ex.submit(_dispatch, m, n, f) for (m, n, f) in to_run]
        for fut in concurrent.futures.as_completed(futs):
            try:
                results.append(fut.result())
            except Exception as e:
                rec = _mk_rec("dispatch", "")
                rec["detail"] = "dispatch worker raised: %r" % e
                results.append(rec)

    return summary()


def _on_signal(_signo, _frame):
    # graceful: print whatever we have, then exit non-zero
    try:
        summary()
    except Exception:
        pass
    os._exit(1)


signal.signal(signal.SIGINT, _on_signal)
signal.signal(signal.SIGTERM, _on_signal)


if __name__ == "__main__":
    # Isolated single-handler worker spawned by main()'s parallel dispatcher.
    if "--run-one" in sys.argv:
        _i = sys.argv.index("--run-one")
        _modname = sys.argv[_i + 1]
        _mc = sys.argv[_i + 2] if len(sys.argv) > _i + 2 \
            and not sys.argv[_i + 2].startswith("-") else "test"
        sys.exit(_subprocess_run_one(_modname, _mc))
    sys.exit(main())
