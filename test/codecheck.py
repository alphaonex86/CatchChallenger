#!/usr/bin/env python3
"""
test/codecheck.py — run the function-by-function C/C++ codecheck auditor as an
all.sh stage.

Drives tools/codecheck/codecheck.py (the engine server.py's `codecheck` mode uses):
build the clang-LLVM-IR code tree, then process each function/method ONE BY ONE
with its callers + its multiple callees — but feed the IA only ONE callee branch
per turn so a small local model's context never saturates.

Three layers:
  1. INVARIANT CHECK (always, deterministic, no IA): for every function in scope,
     each per-turn view (headers + body + caller tree + ONE callee branch) must
     hold AT MOST ONE callee branch AND stay under CONTEXT_BUDGET_TOKENS. A view
     that saturates -> FAIL (that is exactly the thing this stage exists to catch).
  2. FILE SWEEP (deterministic clang-tidy over the scope).
  3. IA REVIEW (default, only if a backend is reachable): a REAL quality review of
     each function by the local Ollama model — real BUGS, ILLOGICAL/dead code, and
     a name that CONTRADICTS what the code does. Style/clarity/perf/duplication are
     deliberately NOT asked for, and the prompt states the project rules (C++11..23,
     no exceptions/RTTI, braceless single statements) so the model stops reporting
     them. NOT security either: security/server.py owns exploitable vulnerabilities
     and their proof. Only HIGH/CRITICAL findings gate AND reach the console — the
     engine adversarially re-checks every one of them (downgrading to MEDIUM when
     unconfirmed), so a failure means the model proved it from the code; everything
     else is written to the findings file. No backend up -> layer skipped, not a
     failure (the suite must not depend on a running Ollama/Claude).

Scope: EVERY C/C++ source we own — general/, server/, client/ AND tools/ — with the
vendored libs excluded. Override with --scope= / CC_CODECHECK_SCOPE.
Run `codecheck.py --help` for the flags (model choice, budget, gating).

Self-skips (PASS) when clang/cmake are absent or the code tree can't be built —
this is opt-in tooling, like the cross-compile stages.
"""

import sys
sys.dont_write_bytecode = True

import os
import re
import time
import shutil
import importlib.util

import diagnostic
import wall_cap
wall_cap.arm()
import build_paths
import phase_timer
from test_config import FAILED_JSON

build_paths.ensure_root()
DIAG = diagnostic.parse_diag_args()

SCRIPT_NAME = os.path.basename(__file__)
_T_START = time.monotonic()          # process start, for the wall-cap-derived budget
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SECURITY = os.path.join(ROOT, "security")
CODECHECK_DIR = os.path.join(ROOT, "tools", "codecheck")

C_GREEN = "\033[92m"; C_RED = "\033[91m"; C_CYAN = "\033[96m"; C_RESET = "\033[0m"

# A small local 30B has a small context window; each per-function turn must stay
# well under it (~4 chars/token, codecheck's own estimate).
CONTEXT_BUDGET_TOKENS = 8000
MAX_INVARIANT_FUNCS = 600     # bound the deterministic scan
# ALL our C/C++, vendor excluded (codetree.is_vendor drops blake3/hps/xxhash/zstd/
# tinyXML2/zlib/ogg/opus/opusfile/tiled). Unlike security/server.py — which only
# cares about the server-reachable attack surface — this is a GENERAL quality
# auditor, so client/ and tools/ are in scope too. Comma-separated, repo-relative.
SCOPE_REL = os.environ.get("CC_CODECHECK_SCOPE", "general,server,client,tools")
# Layer 3 (IA quality review) budget + bounds. <= 0 (the default) = NO wall budget:
# review the WHOLE scope, bounded only by the per-script wall cap. The verdict cache
# is keyed by (model, function source), so a re-run replays what it already reviewed
# for free and spends its time getting FURTHER through the scope.
IA_BUDGET_SECS = int(os.environ.get("CC_CODECHECK_IA_BUDGET", "-1"))
# Default reviewer. --model= / CC_CODECHECK_MODEL picks another one.
DEFAULT_IA_MODEL = os.environ.get("CC_CODECHECK_MODEL", "gemma4:26b")
# WHICH Ollama host serves it. Empty = whatever the out-of-repo settings resolve
# (~/.config/CatchChallenger/ia-settings.json: 'ollama_backends' can PIN
# gemma4:26b to the GPU box, else 'ollama_host', else localhost). A backend URL /
# IP is infrastructure and must NEVER be committed, so this file only ever holds
# the empty default - point it at a remote box with --ollama-host= (or the
# settings pin, which every security/ tool then honours too).
DEFAULT_IA_HOST = os.environ.get("CC_CODECHECK_OLLAMA_HOST", "").strip()
# Function cap. The real bound is --budget: a cached function replays for free, so
# each run spends its budget on functions not reviewed yet and gets FURTHER through
# the scope. Keep the cap above the scope size so it never truncates coverage.
MAX_IA_FUNCS = int(os.environ.get("CC_CODECHECK_IA_FUNCS", "2000"))
MAX_IA_PRINTED = 60          # bound the console output, not the review
# How often layer 3 says where it is. A whole-scope pass is HOURS (a hard function
# costs the model minutes), and without this the console sits silent between two
# HIGH findings, with no way to tell "still working" from "wedged".
PROGRESS_EVERY_SECS = int(os.environ.get("CC_CODECHECK_PROGRESS", "3600"))
IA_FINDINGS_MD = os.path.join(os.path.dirname(FAILED_JSON),
                              "codecheck-ia-findings.md")

_USAGE = """\
usage: codecheck.py [--model=NAME] [--budget=SECS] [--scope=REL]
                    [--no-ia] [--no-gate] [--help]

all.sh stage driving the function-by-function C/C++ quality auditor
(tools/codecheck/codecheck.py), one callee branch per turn. Three layers:

  1 invariant  deterministic: every per-function IA view holds exactly ONE callee
               branch and stays under the context budget            [gates]
  2 sweep      deterministic clang-tidy sweep over the scope        [gates]
  3 IA review  local-Ollama review of each function: real BUGS, ILLOGICAL/dead
               code, and names that CONTRADICT the code. Style, clarity, perf and
               duplication are NOT asked for; SECURITY IS NOT REVIEWED HERE -
               security/server.py owns vulns + proof.  [gates on HIGH/CRITICAL]

Every HIGH/CRITICAL finding is adversarially re-checked by the same model and
downgraded to MEDIUM when the check cannot confirm it, so only findings the model
is SURE of can fail the stage. The console shows those; LOW/MEDIUM (and anything
downgraded) go to the findings file only.

Options:
  --model=NAME   Ollama model for layer 3 (default %s, CC_CODECHECK_MODEL).
                 Append '@host:port' to pin a backend, or use 'claude-cli'.
                 --llm= is accepted as an alias.
  --ollama-host=URL   Ollama backend serving that model - a full URL or a bare
                 host:port (http assumed); BRACKET an IPv6 address, e.g.
                 --ollama-host='[2803:1920::4:100]:11434'. Same as
                 CC_CODECHECK_OLLAMA_HOST, and as the '@host' suffix of --model.
                 Default: the out-of-repo settings
                 (~/.config/CatchChallenger/ia-settings.json 'ollama_backends'
                 pin / 'ollama_host'), else localhost - no backend IP is ever
                 stored in the repo. --host= is accepted as an alias.
  --budget=SECS  wall budget for layer 3 (default %d, CC_CODECHECK_IA_BUDGET).
                 Functions are reviewed leaves-first until it runs out;
                 -1 (or any value <= 0) means NO limit - review the whole
                 scope, bounded only by the script's own wall cap.
  --scope=REL    repo-relative scope (default %s, CC_CODECHECK_SCOPE)
  --no-ia        deterministic layers only, no IA call (CC_CODECHECK_NO_IA=1)
  --no-gate      report IA findings without ever failing (CC_CODECHECK_NO_GATE=1)
  --help, -h     this help

The shared --sanitize / --valgrind / --profile diagnostic flags also apply.
"""


def _parse_args(argv):
    """Flags for this stage. Unknown ones are ignored on purpose: all.sh passes
    the shared diagnostic flags (--sanitize/--valgrind/--profile/--node), which
    diagnostic.parse_diag_args() owns."""
    opt = {"model": DEFAULT_IA_MODEL, "host": DEFAULT_IA_HOST,
           "budget": IA_BUDGET_SECS, "scope": SCOPE_REL,
           "ia": not os.environ.get("CC_CODECHECK_NO_IA"),
           "gate": not os.environ.get("CC_CODECHECK_NO_GATE")}
    for a in argv:
        if a in ("--help", "-h"):
            print(_USAGE % (DEFAULT_IA_MODEL, IA_BUDGET_SECS, SCOPE_REL))
            sys.exit(0)
        elif a.startswith("--model=") or a.startswith("--llm="):
            opt["model"] = a.split("=", 1)[1].strip()
        elif a.startswith("--ollama-host=") or a.startswith("--host="):
            opt["host"] = a.split("=", 1)[1].strip()
        elif a.startswith("--budget="):
            try:
                opt["budget"] = int(a.split("=", 1)[1])
            except ValueError:
                print("error: --budget expects seconds, e.g. --budget=900 "
                      "(-1 = no limit)")
                sys.exit(2)
        elif a.startswith("--scope="):
            opt["scope"] = a.split("=", 1)[1].strip()
        elif a == "--no-ia":
            opt["ia"] = False
        elif a == "--no-gate":
            opt["gate"] = False
    return opt


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


def _ia_reachable(common, model=None):
    """Cheap one-shot probe of the model the review will actually use (an Ollama
    model, or the configured backend when model is None). True iff a tiny prompt
    returns text; any error -> not reachable, so the IA layer is skipped, not
    failed (an absent local model is not a code regression)."""
    msgs = [{"role": "system", "content": "Reply with OK."},
            {"role": "user", "content": "OK"}]
    try:
        ans = common.chat_with(model, msgs, 60) if model else common.chat(msgs, 60)
        return bool(ans and ans.strip())
    except Exception as exc:
        log_info(f"IA backend/model not reachable ({exc}); IA review skipped")
        return False


_BARE_SEV_RE = re.compile(r"\|\s*(LOW|MEDIUM|HIGH|CRITICAL)\s*\|", re.I)


def _severity_of(eng, finding):
    """Severity a finding gates on.

    A real model writes its verdict as the bare pipe column
    ('logic | MEDIUM | fn:35 | ...'), then keeps talking. The engine's
    finding_severity() takes the HIGHEST SEVERITY(...) tag anywhere in the block,
    so one 'SEVERITY(HIGH)' echoed from the instruction template while the model
    argues with itself outranked the MEDIUM it had just declared. Every false gate
    observed so far was exactly that shape - a block whose column says MEDIUM and
    whose prose wanders into HIGH, including one that concluded "there is no risk
    of leak because you delete before" and still gated HIGH.

    So the DECLARED column wins when there is one (max across columns, so a block
    holding several findings still gates on its worst). The tagged form stays the
    fallback for a block written exactly as instructed - 'CATEGORY(bug) |
    SEVERITY(HIGH) | ...' has no bare column (the pipes wrap SEVERITY(HIGH), not a
    bare HIGH), so a properly formatted HIGH still gates.

    Note the adversarial re-check is NOT a substitute here: it confirmed all four
    of these, which is why the parse itself had to stop over-reading them."""
    declared = []
    for m in _BARE_SEV_RE.finditer(finding):
        declared.append(m.group(1).upper())
    if declared:
        sev = max(declared, key=lambda s: eng._SEV_ORDER[s])
        # A block that rates the SAME function both MEDIUM and HIGH has not
        # proved anything: HIGH/CRITICAL is reserved for "the shown code PROVES
        # it - you must be SURE", so contradicting itself is the definition of
        # not sure. Cap it at MEDIUM instead of gating on its loudest sentence.
        # A block that says HIGH consistently still gates. The finding is NOT
        # dropped - it stays in the findings file for a human to read.
        if len(set(declared)) > 1 and eng._SEV_ORDER[sev] >= eng._SEV_ORDER["HIGH"]:
            sev = "MEDIUM"
        return sev
    return eng.finding_severity(finding)


def _ia_review(eng, idx, funcs, opt, failures):
    """Layer 3: real quality review of each function by the local IA - bugs,
    illogical/dead code, and names that CONTRADICT the code. Style, clarity, perf
    and duplication are not asked for; NOT security either (security/server.py owns
    exploitable vulnerabilities and their proof).

    Only HIGH/CRITICAL findings gate: the engine adversarially re-checks every one
    of them with the same model and downgrades it to MEDIUM when the check cannot
    confirm it, so a gating finding is one the model proved from the shown code.
    A transport error never gates (backend health is not a code regression)."""
    model = opt["model"] or None
    t0 = time.monotonic()
    targets = eng.audit_targets(funcs)
    # A cached verdict replays for free, so a second run over an unchanged tree
    # finishes in milliseconds having asked the model NOTHING. That is correct
    # (nothing is silently dropped - every finding still reaches IA_FINDINGS_MD) but
    # it must never read as a fresh audit: report recomputed vs reused, and how far
    # into the scope the budget got.
    eng.reset_cache_stats()
    # <= 0 = no budget of our own: review the WHOLE scope. It is still bounded by
    # the script's own wall cap (wall_cap.arm()), and stopping ~2 min short of it
    # matters: past the cap the process is SIGALRM'd to exit 124, all.sh reports
    # [TIMEOUT] and the findings/tally are never printed. So an unlimited budget
    # means "run to the wall", not "run past it and lose the report".
    if opt["budget"] > 0:
        budget = opt["budget"]
        budget_note = f"budget {budget}s"
    else:
        budget = max(60, int(wall_cap.cap_for(SCRIPT_NAME)
                             - (time.monotonic() - _T_START) - 120))
        budget_note = f"no budget, whole scope ({budget}s left before the wall cap)"
    log_info(f"IA review: bugs/illogic/naming via {model or 'configured backend'} "
             f"over {len(targets)} function(s) of {len(funcs)} indexed "
             f"({budget_note}; security NOT reviewed here)")
    reviewed = 0
    printed = 0
    errors = 0
    counts = {"LOW": 0, "MEDIUM": 0, "HIGH": 0, "CRITICAL": 0}
    hard = []
    md = ["# codecheck IA review (bugs / illogic / naming - NOT security)\n"]
    total = len(targets[:MAX_IA_FUNCS])
    next_progress = t0 + PROGRESS_EVERY_SECS
    # Only REAL logic: audit_targets drops destructors, getters/setters, thin
    # forwarders and compiler-generated members. Layer 1 deliberately keeps the
    # unfiltered list (it checks the view SIZE of every function), but sending
    # those to the model wastes the budget and invites nonsense findings about a
    # body the function does not have.
    for fi in targets[:MAX_IA_FUNCS]:
        if time.monotonic() - t0 >= budget:
            break
        try:
            found = eng.audit_function(idx, fi, model=model, verify=True)
        except Exception as exc:      # a bad function must not kill the stage
            errors += 1
            log_info(f"IA review: {fi.qual_name} raised {exc}")
            continue
        reviewed += 1
        rel = os.path.relpath(fi.file, ROOT)
        now = time.monotonic()
        if now >= next_progress:
            next_progress = now + PROGRESS_EVERY_SECS
            spent = now - t0
            per = spent / reviewed
            fits = int((budget - spent) / per) if per > 0 else 0
            log_info(f"IA review: {reviewed}/{total} reviewed "
                     f"({100.0 * reviewed / total:.1f}%) in {spent / 60:.0f} min, "
                     f"{per:.1f}s/function -> ~{fits} more fit in the remaining "
                     f"{(budget - spent) / 60:.0f} min; "
                     f"{counts['CRITICAL']}C/{counts['HIGH']}H/{counts['MEDIUM']}M/"
                     f"{counts['LOW']}L so far ({eng.cache_summary()}); at {rel}")
        for f in found:
            if f.startswith("[chat error"):
                errors += 1
            else:
                sev = _severity_of(eng, f)
                if sev:
                    counts[sev] += 1
                one = " ".join(f.split())
                md.append("- **%s** `%s:%d` %s — %s\n"
                          % (sev or "INFO", rel, fi.line, fi.qual_name, one))
                # The console shows ONLY what can gate (a CONFIRMED HIGH/CRITICAL —
                # "[severity downgraded" means the adversarial check did not confirm
                # it). Everything else is still recorded, in IA_FINDINGS_MD: over a
                # 1144-function scope the LOW/MEDIUM tier is hundreds of lines and
                # buries the findings that matter.
                if sev in ("HIGH", "CRITICAL") and "[severity downgraded" not in f:
                    hard.append(("ia %s" % fi.qual_name, "%s:%d %s"
                                 % (rel, fi.line, one[:400])))
                    if printed < MAX_IA_PRINTED:
                        printed += 1
                        log_info(f"  {C_RED}{sev}{C_RESET} {rel}:{fi.line} "
                                 f"{fi.qual_name}\n        {one[:400]}")
    secs = time.monotonic() - t0
    try:
        with open(IA_FINDINGS_MD, "w") as fh:
            fh.write("".join(md))
    except OSError as exc:
        log_info(f"could not write {IA_FINDINGS_MD}: {exc}")
    left = max(0, total - reviewed)
    tally = (f"{reviewed}/{len(targets)} function(s) "
             f"({eng.cache_summary()}"
             f"{f'; {left} not reached in the budget' if left else '; whole scope covered'}), "
             f"{counts['CRITICAL']} critical / {counts['HIGH']} high / "
             f"{counts['MEDIUM']} medium / {counts['LOW']} low"
             f"{f', {errors} transport error(s)' if errors else ''}")
    if reviewed == 0:
        log_info(f"IA review: nothing reviewed inside {budget}s "
                 "— raise --budget / the wall cap, or use a faster model")
    elif hard and opt["gate"]:
        failures.extend(hard)
        log_fail("IA review", f"{len(hard)} confirmed HIGH/CRITICAL finding(s); "
                 f"{tally}; details {IA_FINDINGS_MD}", secs)
    elif hard:
        log_info(f"IA review: {len(hard)} HIGH/CRITICAL finding(s) NOT gating "
                 f"(--no-gate); {tally}; details {IA_FINDINGS_MD}")
        log_pass(f"IA review (report-only): {tally}", secs)
    else:
        log_pass(f"IA review: {tally}; LOW/MEDIUM + every finding in "
                 f"{IA_FINDINGS_MD}", secs)


def main():
    t0 = time.monotonic()
    opt = _parse_args(sys.argv[1:])
    global SCOPE_REL
    SCOPE_REL = opt["scope"]
    # Thinking models (gemma4) spend the whole num_predict on thought and return
    # empty; the per-function views need no reasoning, so default think OFF
    # (operator can override CC_OLLAMA_THINK).
    os.environ.setdefault("CC_OLLAMA_THINK", "false")
    if shutil.which("clang") is None:
        _skip_pass("clang not found — codecheck needs clang for the LLVM-IR tree")

    eng = _load_engine()
    if opt["model"]:
        # '@host:port' pins which Ollama backend serves this model; the bare name
        # is what chat_with() then routes. --ollama-host= is the same pin spelled
        # separately, so fold it in (an explicit '@' in --model always wins).
        spec = opt["model"]
        if opt["host"] and "@" not in spec:
            spec = "%s@%s" % (spec, opt["host"])
        opt["model"] = eng.common._register_model_spec(spec)
        log_info(f"IA model {opt['model']} -> "
                 f"{eng.common.backend_for_model(opt['model'])}")
    # codecheck.build_index() auto-finds/generates the compile DB and redirects the
    # clang-IR + type caches to the persistent SSD cache — nothing to wire here.
    scope = [os.path.join(ROOT, s.strip()) for s in SCOPE_REL.split(",") if s.strip()]
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

    # ---- layer 1b: deterministic FILE-LEVEL sweep --------------------------------
    # The comprehensive clang-tidy pass that covers EVERY function (not just the
    # codetree-indexed few). Deterministic, so it GATES: a crash, a malformed finding
    # tuple, or 0 findings (a broken compile DB / parse silently yields 0, but a
    # tree this size always has findings incl. intentional false-positives) is a real
    # regression. Skips when clang-tidy is absent.
    if shutil.which("clang-tidy") is None:
        log_info("sweep check skipped (clang-tidy not on PATH)")
    else:
        ts = time.monotonic()
        try:
            sweep = eng.file_sweep(scope)
        except Exception as exc:
            log_fail("sweep", f"file_sweep raised: {exc}", time.monotonic() - ts)
            failures.append(("sweep", f"file_sweep raised: {exc}"))
            sweep = None
        if sweep is not None:
            total = sum(len(v) for v in sweep.values())
            bad = next((f"{rel}:{it}" for rel, items in sweep.items() for it in items
                        if not (isinstance(it, (list, tuple)) and len(it) == 3
                                and isinstance(it[0], int))), None)
            if bad is not None:
                log_fail("sweep", f"malformed finding tuple: {bad}", time.monotonic() - ts)
                failures.append(("sweep", f"malformed finding tuple: {bad}"))
            elif total == 0:
                log_fail("sweep", f"0 findings over {SCOPE_REL} — compile DB or "
                         "parse likely broken", time.monotonic() - ts)
                failures.append(("sweep", f"0 findings over {SCOPE_REL}"))
            else:
                log_pass(f"sweep: {total} well-formed finding(s) over {SCOPE_REL}",
                         time.monotonic() - ts)

    # ---- layer 3: IA quality review (bugs / illogic / perf - NOT security) -----
    if not opt["ia"]:
        log_info("IA review skipped (--no-ia / CC_CODECHECK_NO_IA)")
    elif _ia_reachable(eng.common, opt["model"] or None):
        _ia_review(eng, idx, funcs, opt, failures)
    else:
        log_info("IA review skipped (model/backend not up) — deterministic layers ran")

    # (The old one-call "live smoke" is gone: layer 3 exercises the same per-turn
    # pipeline for real, so a separate probe call was pure duplication.)
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
