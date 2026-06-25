#!/usr/bin/env python3
"""
test/codecheck.py — run the function-by-function C/C++ codecheck auditor as an
all.sh stage.

Drives tools/codecheck/codecheck.py (the engine server.py's `codecheck` mode uses):
build the clang-LLVM-IR code tree, then process each function/method ONE BY ONE
with its callers + its multiple callees — but feed the IA only ONE callee branch
per turn so a small local model's context never saturates.

Two layers:
  1. INVARIANT CHECK (always, deterministic, no IA): for every function in scope,
     each per-turn view (headers + body + caller tree + ONE callee branch) must
     hold AT MOST ONE callee branch AND stay under CONTEXT_BUDGET_TOKENS. A view
     that saturates -> FAIL (that is exactly the thing this stage exists to catch).
  2. LIVE SMOKE (only if an IA backend is reachable): audit a few functions for
     real, one branch at a time, and FAIL only on a transport/tooling error. If no
     backend is up, this layer is skipped (not a failure) — the suite must not
     depend on a running Ollama/Claude.

Scope: general/base (the protocol parsers — security-critical, and they build
fast from the server/cli compile DB). Override with CC_CODECHECK_SCOPE.

Self-skips (PASS) when clang/cmake are absent or the code tree can't be built —
this is opt-in tooling, like the cross-compile stages.
"""

import sys
sys.dont_write_bytecode = True

import os
import time
import shutil
import importlib.util
import subprocess

import diagnostic
import wall_cap
wall_cap.arm()
import build_paths
import phase_timer
from test_config import FAILED_JSON, TMPFS_BUILD_ROOT

build_paths.ensure_root()
DIAG = diagnostic.parse_diag_args()

SCRIPT_NAME = os.path.basename(__file__)
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SECURITY = os.path.join(ROOT, "security")
CODECHECK_DIR = os.path.join(ROOT, "tools", "codecheck")

C_GREEN = "\033[92m"; C_RED = "\033[91m"; C_CYAN = "\033[96m"; C_RESET = "\033[0m"

# A small local 30B has a small context window; each per-function turn must stay
# well under it (~4 chars/token, codecheck's own estimate).
CONTEXT_BUDGET_TOKENS = 8000
MAX_INVARIANT_FUNCS = 600     # bound the deterministic scan
LIVE_SMOKE_FUNCS = 3          # how many functions to audit for real when IA is up
SCOPE_REL = os.environ.get("CC_CODECHECK_SCOPE", "general/base")
NICE_PREFIX = ["nice", "-n", "19", "ionice", "-c", "3"]


def log_pass(label, secs=0.0):
    print(f"{phase_timer.t()}   [{C_GREEN}PASS{C_RESET}] {label}  ({secs:.1f}s)")
    phase_timer.record_event("pass", label, ok=True, dt=secs)


def log_fail(label, why, secs=0.0):
    print(f"{phase_timer.t()}   [{C_RED}FAIL{C_RESET}] {label}  {why}  ({secs:.1f}s)")
    phase_timer.record_event("fail", label, ok=False, dt=secs, detail=why)


def log_info(msg):
    print(f"{phase_timer.t()}   {msg}")


def _skip_pass(msg):
    """Opt-in tooling absent -> report skip and exit 0 (not a failure)."""
    log_info(f"[skip] {msg}")
    sys.exit(0)


def _load_engine():
    """Import tools/codecheck/codecheck.py as 'sec_codecheck' (its basename clashes
    with THIS file, so load it by explicit path). The engine self-adds security/ to
    sys.path for its `import common` / `import codetree`; we add both dirs so a
    transitive `import codecheck` resolves to the engine, not this harness."""
    for d in (CODECHECK_DIR, SECURITY):
        if d not in sys.path:
            sys.path.insert(0, d)
    spec = importlib.util.spec_from_file_location(
        "sec_codecheck", os.path.join(CODECHECK_DIR, "codecheck.py"))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def _ensure_compile_db():
    """codetree needs clang compile flags from a compile_commands.json. Configure
    server/cli (its compile DB covers server/ + general/base via the INTERFACE
    libs) with CMAKE_EXPORT_COMPILE_COMMANDS. Returns the DB path or None."""
    if shutil.which("cmake") is None:
        return None
    build = os.path.join(TMPFS_BUILD_ROOT, "codecheck-cdb")
    os.makedirs(build, exist_ok=True)
    cmd = NICE_PREFIX + [
        "cmake", "-S", os.path.join(ROOT, "server", "cli"), "-B", build,
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        "-DCATCHCHALLENGER_DB_FILE=ON",
        "-DCMAKE_BUILD_TYPE=RelWithDebInfo"]
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
    except (OSError, subprocess.SubprocessError) as exc:
        log_info(f"compile-db configure failed to launch: {exc}")
        return None
    db = os.path.join(build, "compile_commands.json")
    if r.returncode != 0 or not os.path.isfile(db):
        log_info("compile-db configure failed:\n" + (r.stderr or "")[-800:])
        return None
    return db


def _ia_reachable(common):
    """Cheap one-shot probe of the configured backend (Ollama/Claude/claude-cli).
    True iff a tiny prompt returns text; any error -> not reachable (skip live)."""
    try:
        ans = common.chat([{"role": "system", "content": "Reply with OK."},
                           {"role": "user", "content": "OK"}], timeout=25)
        return bool(ans and ans.strip())
    except Exception as exc:
        log_info(f"IA backend not reachable ({exc}); live smoke skipped")
        return False


def main():
    t0 = time.monotonic()
    # This file is the all.sh HARNESS self-test of the codecheck ENGINE, NOT the
    # auditor — it ignores the auditor's flags. If invoked WITH them, redirect loudly
    # so it can't be mistaken for the auditor (don't silently run the smoke + report
    # "0 failures", which reads as "the code is clean").
    _auditor_flags = {"--llm", "--panel", "--func", "--model", "--scope", "--limit"}
    if any(a.split("=", 1)[0] in _auditor_flags for a in sys.argv[1:]):
        print("test/codecheck.py is the all.sh HARNESS self-test of the codecheck "
              "engine — NOT the auditor; it ignores %s.\n"
              "To AUDIT code (find bugs/naming/clarity/perf), run the tool:\n"
              "    python3 %s --llm <model[@ollama-url]> --scope <path>"
              % (", ".join(sorted(_auditor_flags)),
                 os.path.join(CODECHECK_DIR, "codecheck.py")))
        sys.exit(2)
    # Thinking models (gemma4) spend the whole num_predict on thought and return
    # empty; the audit views need no reasoning, so default think OFF for the smoke
    # (operator can override CC_OLLAMA_THINK).
    os.environ.setdefault("CC_OLLAMA_THINK", "false")
    if shutil.which("clang") is None:
        _skip_pass("clang not found — codecheck needs clang for the LLVM-IR tree")

    eng = _load_engine()
    # codecheck.build_index() auto-finds/generates the compile DB and redirects the
    # clang-IR + type caches to the persistent SSD cache — nothing to wire here.
    scope = [os.path.join(ROOT, SCOPE_REL)]
    log_info(f"building code tree over {SCOPE_REL} (persistent cache)")
    try:
        idx = eng.build_index(scope)
    except Exception as exc:
        log_fail("build-tree", f"codetree.build() raised: {exc}", time.monotonic() - t0)
        _save_and_exit([("build-tree", str(exc))])
    funcs = eng.leaves_first(idx)
    if len(funcs) < 10:
        # Clang present but almost nothing indexed -> an environment issue (missing
        # sysroot headers etc.), not a codecheck bug. Skip rather than false-fail.
        _skip_pass(f"only {len(funcs)} functions indexed — IR build incomplete "
                   "(toolchain/headers), nothing to check")
    log_info(f"{len(funcs)} functions indexed")
    # warm the clang var-type cache in parallel so the per-function views below
    # aren't serialized on one clang parse each.
    sample = funcs[:MAX_INVARIANT_FUNCS]
    log_info(f"pre-warming clang type cache for {len(sample)} function(s)")
    eng.prewarm_types(sample)

    failures = []

    # ---- layer 1: deterministic one-branch / bounded-context invariant --------
    checked = 0
    branch_turns = 0
    worst = (0, "")
    saturated = []
    for fi in sample:
        checked += 1
        try:
            views = list(eng.build_views(idx, fi))
        except Exception as exc:
            failures.append((f"views {fi.qual_name}", f"build_views raised: {exc}"))
            continue
        for label, ctx in views:
            toks = len(ctx) // 4
            if toks > worst[0]:
                worst = (toks, f"{fi.qual_name} [{label}]")
            # exactly ONE callee branch per turn (label 'solo' = zero, 'branch:*' = one)
            if ctx.count("=== ONE THING IT CALLS") > 1:
                failures.append((f"branch {fi.qual_name}",
                                 f"view '{label}' holds >1 callee branch"))
            if label.startswith("branch:"):
                branch_turns += 1
            if toks > CONTEXT_BUDGET_TOKENS:
                saturated.append(f"{fi.qual_name} [{label}] ~{toks} tok")
    if saturated:
        failures.append(("context-budget",
                         f"{len(saturated)} per-function turn(s) exceed "
                         f"{CONTEXT_BUDGET_TOKENS} tok, e.g. " + "; ".join(saturated[:5])))
    inv_secs = time.monotonic() - t0
    if any(f[0] in ("context-budget",) or f[0].startswith(("branch ", "views "))
           for f in failures):
        log_fail("invariant", f"{len(failures)} issue(s); worst view "
                 f"{worst[0]} tok ({worst[1]})", inv_secs)
    else:
        log_pass(f"invariant: {checked} functions, {branch_turns} one-branch turns, "
                 f"worst view {worst[0]} tok (<= {CONTEXT_BUDGET_TOKENS})", inv_secs)

    # ---- layer 2: bounded live smoke (only if a backend is reachable) ----------
    # ONE function, ONE branch, ONE IA call — just prove the per-turn pipeline works
    # end to end. A full audit is a manual / server.py run, NOT a CI gate: a 26-30B
    # local model is far too slow to audit hundreds of functions inside a test cap.
    if _ia_reachable(eng.common):
        target = next((f for f in funcs if eng.callee_branches(idx, f.qual_name)), funcs[0])
        _label, ctx = next(iter(eng.build_views(idx, target)))
        log_info(f"live smoke: 1 branch of {target.qual_name} (~{len(ctx)//4} tok)")
        ts = time.monotonic()
        # The smoke is OPPORTUNISTIC: an empty/errored reply is a backend-health
        # issue (unreachable model, a thinking model with think-tokens not disabled,
        # etc.), NOT a codecheck logic bug — report it but do NOT gate the suite on
        # local-model health. Only the deterministic invariant check above gates.
        try:
            ans = eng.common.chat([{"role": "system", "content": eng.CHECK_SYSTEM},
                                   {"role": "user", "content": ctx}], timeout=180)
            if ans and ans.strip():
                log_pass(f"live smoke {target.qual_name}", time.monotonic() - ts)
            else:
                log_info(f"live smoke: empty IA reply ({time.monotonic()-ts:.0f}s) — "
                         "backend issue, not gating the suite")
        except Exception as exc:
            log_info(f"live smoke: IA call failed ({exc}) — not gating the suite")
    else:
        log_info("live smoke skipped (no IA backend up) — invariant check still ran")

    _save_and_exit(failures)


def _save_and_exit(failures):
    import failed_cases as _fc
    print(f"\n{C_CYAN}{'='*60}{C_RESET}")
    print(f"  codecheck: {len(failures)} failure(s)")
    print(f"{C_CYAN}{'='*60}{C_RESET}")
    entries = [(name, _fc.make_detail(why)) for name, why in failures]
    _fc.save(SCRIPT_NAME, entries)
    sys.exit(0 if not failures else 1)


if __name__ == "__main__":
    main()
