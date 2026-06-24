#!/usr/bin/env python3
"""codecheck.py — function-by-function C/C++ auditor with a LIMITED per-function
view, for SMALL local models (gemma/qwen 30B-class, small num_ctx).

The point: never send a whole file. Each IA turn sees ONLY
  - the relevant HEADER(s) (.h/.hpp) in full — they are short (declarations), so
    the model has the signatures/types globally,
  - the ONE function (method) under audit — its body,
  - its CALLER tree (who calls it — the untrusted-input/taint sources),
  - and ONE CALLEE BRANCH at a time (the body of one thing it calls), iterated,
so a local 30B with a few-K context window can audit a huge C/C++ codebase one
leaf at a time. Functions are walked LEAVES-FIRST (callees before callers).

Scope: ALL C/C++ in the repo, EXCLUDING the vendored libs (codetree.is_vendor:
blake3/hps/xxhash/zstd/tinyXML2/ogg/opus/opusfile/tiled). Python is NOT analysed
here (that is pycodetree.py's separate job).

Shares the transport (common.chat) and the code tree (codetree.py: clang LLVM-IR
call graph) with server.py — server.py reuses audit_function() for its
small-context per-function scan mode, so the two run the SAME logic.

codetree.py is treated as READ-ONLY (a parallel session edits it): we only read
its public index (by_name / _forward_callees / source_body / TreeRender) and the
leaves-first walk + header lookup + branch iteration live here.
"""

import argparse
import os
import sys

import common
import codetree

REPO_ROOT = codetree.REPO_ROOT

# All C/C++ except vendor. codetree.is_vendor() drops the vendored subtrees;
# we list the top-level source roots. (codetree only IR-compiles a TU when it can
# resolve flags from a compile_commands.json or the common-flags fallback, so a
# file with no available flags — e.g. a Qt client TU with no DB entry — is simply
# skipped with a logged warning, not a hard error.)
DEFAULT_SCOPE = tuple(os.path.join(REPO_ROOT, p) for p in (
    "general", "server", "client", "tools"))

HEADER_EXT = (".hpp", ".h", ".hxx")
_HEADER_CAP = 8000           # a header is "short" — cap so a hub header stays small
_BODY_CAP = 9000             # one function body
_CALLEE_BODY_CAP = 6000      # one callee branch body
_MAX_BRANCHES = 10           # cap callee branches audited per function

# Crisp system prompt for a SMALL model: one function, terse structured output.
CHECK_SYSTEM = (
    "You are a meticulous C/C++ security & correctness reviewer. You are shown the "
    "relevant HEADER(s), ONE function to audit, its CALLER tree (who calls it - "
    "possible sources of untrusted/remote input), and (when present) ONE thing it "
    "calls. Audit ONLY the shown function. Look for: out-of-bounds read/write, "
    "unchecked length/index/size/offset/pointer taken from a caller, integer "
    "overflow/underflow or signed/unsigned confusion, unchecked return value, "
    "use-after-free/double-free, leak, null-deref, logic error, wrong condition, "
    "dead code, name-vs-purpose mismatch. Use the CALLER tree to judge whether bad "
    "input can actually reach it. Be terse. If the function is fine, reply with "
    "exactly: NO ISSUES. Otherwise one line per finding:\n"
    "SEVERITY(low|medium|high|critical) | function:line | one-line problem -> fix")


# ---------------------------------------------------------------------------
# Index + scope
# ---------------------------------------------------------------------------
def build_index(scope=None):
    """Build (or reuse) the codetree C/C++ index over `scope` (excl vendor)."""
    codetree.SCOPE_DIRS = tuple(scope) if scope else DEFAULT_SCOPE
    idx = codetree.Index()
    idx.build()
    return idx


def leaves_first(idx):
    """Functions leaves-first: a function appears AFTER everything it calls
    (callees before callers); cycles broken deterministically by qual_name.
    Implemented here so codetree.py stays untouched."""
    order = []
    state = {}                       # qual -> 1 in-progress / 2 done

    def visit(q):
        st = state.get(q, 0)
        if st:                       # done OR on the stack (cycle edge) -> stop
            return
        state[q] = 1
        for c in sorted(idx._forward_callees.get(q, {})):
            if c in idx.by_name:
                visit(c)
        state[q] = 2
        order.append(q)

    for q in sorted(idx.by_name):
        visit(q)
    return [idx.by_name[q] for q in order]


# ---------------------------------------------------------------------------
# The LIMITED VIEW
# ---------------------------------------------------------------------------
def headers_for(fi):
    """The function's OWN header(s) in full (short): the sibling .hpp/.h of its
    .cpp (same basename), which declares its class + signatures. Returns a list of
    (rel_path, text). Capped so it stays 'short'."""
    out = []
    base = os.path.splitext(fi.file)[0]      # /path/Foo  (from Foo.cpp)
    for ext in HEADER_EXT:
        h = base + ext
        if os.path.isfile(h):
            try:
                txt = open(h, "r", errors="replace").read()
            except OSError:
                continue
            out.append((os.path.relpath(h, REPO_ROOT), txt[:_HEADER_CAP]))
    return out


def _body(fi, cap):
    body, _ = codetree.source_body(fi.file, fi.line)
    return body[:cap] if body else "(body not found)"


def callee_branches(idx, qual):
    """Immediate callees of `qual` as (callee_qual, callee_FuncInfo) — the
    'one branch at a time' units. Skips self and unresolved/external callees,
    capped at _MAX_BRANCHES (security-relevant ones first: those whose name hints
    at a length/index/size/copy)."""
    callees = [c for c in idx._forward_callees.get(qual, {})
               if c in idx.by_name and c != qual]
    risky = ("size", "len", "copy", "read", "memcpy", "load", "get", "parse",
             "alloc", "index", "offset", "count")

    def score(c):
        n = c.lower()
        return (0 if any(r in n for r in risky) else 1, c)
    callees.sort(key=score)
    return [(c, idx.by_name[c]) for c in callees[:_MAX_BRANCHES]]


def build_views(idx, fi):
    """Yield (label, context_text) — one SMALL per-turn view for the function:
    a base view (headers + callers + body), then ONE callee branch per turn."""
    rel = os.path.relpath(fi.file, REPO_ROOT)
    hdrs = headers_for(fi)
    hdr_block = "".join("=== HEADER %s ===\n%s\n\n" % (r, t) for r, t in hdrs)
    caller_tree = codetree.TreeRender.caller_tree(idx, fi.qual_name, depth=4)
    body = _body(fi, _BODY_CAP)
    base = (
        "%s=== CALLERS (taint sources — who reaches this function) ===\n%s\n\n"
        "=== AUDIT THIS FUNCTION: %s  (%s:%d) ===\n%s\n"
        % (hdr_block, caller_tree, fi.qual_name, rel, fi.line, body))

    branches = callee_branches(idx, fi.qual_name)
    if not branches:
        yield ("solo", base)
        return
    # One callee branch at a time: the model audits the SAME function, seeing one
    # thing it calls per turn (so it can judge correct usage) — minimal context.
    i = 0
    for cq, cfi in branches:
        i += 1
        crel = os.path.relpath(cfi.file, REPO_ROOT)
        branch = ("\n=== ONE THING IT CALLS (branch %d/%d): %s  (%s:%d) ===\n%s\n"
                  % (i, len(branches), cq, crel, cfi.line, _body(cfi, _CALLEE_BODY_CAP)))
        yield ("branch:%s" % cq, base + branch)


# ---------------------------------------------------------------------------
# Audit one function (SHARED by server.py's per-function mode)
# ---------------------------------------------------------------------------
def audit_function(idx, fi, model=None, system=CHECK_SYSTEM, dry=False):
    """Audit ONE function with the limited view, one callee branch per turn.
    Returns a list of finding lines (model replies that aren't 'NO ISSUES').
    dry=True: don't call the IA — return the per-turn view SIZES (verification)."""
    findings = []
    for label, ctx in build_views(idx, fi):
        if dry:
            findings.append("[dry %s] view=%d chars (~%d tok)"
                            % (label, len(ctx), len(ctx) // 4))
            continue
        messages = [{"role": "system", "content": system},
                    {"role": "user", "content": ctx}]
        try:
            answer = common.chat_with(model, messages) if model else common.chat(messages)
        except Exception as exc:                       # transport error: note + go on
            findings.append("[chat error %s: %s]" % (label, exc))
            continue
        a = (answer or "").strip()
        if a and "NO ISSUES" not in a.upper():
            findings.append("[%s]\n%s" % (label, a))
    return findings


# ---------------------------------------------------------------------------
# Driver
# ---------------------------------------------------------------------------
def run(scope=None, only_file=None, only_func=None, limit=None,
        model=None, dry=False, out=sys.stdout):
    # Model override: set the Ollama served model the same way server.py --model
    # does (common.MODEL_NAME drives common.chat), so `--model qwen3-coder:30b`
    # is reliable regardless of chat_with spec parsing. Then audit via common.chat.
    if model:
        common.MODEL_NAME = model
        model = None
    idx = build_index(scope)
    if only_func:
        funcs = [idx.by_name[only_func]] if only_func in idx.by_name else []
        if not funcs:
            out.write("[not in index: %s]\n" % only_func)
            return 2
    elif only_file:
        funcs = idx.functions_in(only_file)
    else:
        funcs = leaves_first(idx)
    if limit:
        funcs = funcs[:limit]
    out.write("[codecheck] %s: %d function(s) %s\n"
              % (_ts_safe(), len(funcs), "(dry)" if dry else ""))
    total = 0
    for fi in funcs:
        fl = audit_function(idx, fi, model=model, dry=dry)
        if fl:
            out.write("\n#### %s  (%s:%d)\n"
                      % (fi.qual_name, os.path.relpath(fi.file, REPO_ROOT), fi.line))
            for line in fl:
                out.write(line + "\n")
            total += 0 if dry else len(fl)
    out.write("\n[codecheck] done; %d finding-block(s)\n" % total)
    return 0


def _ts_safe():
    try:
        return common._ts()
    except Exception:
        import time
        return time.strftime("%H:%M %d/%m/%Y")


def main(argv):
    p = argparse.ArgumentParser(
        description="Function-by-function C/C++ auditor (limited per-function view, "
                    "for small local models). Excludes Python and vendored libs.")
    p.add_argument("--scope", help="comma-separated source roots (default: all C/C++ "
                                    "excl vendor: general,server,client,tools)")
    p.add_argument("--file", help="audit only functions defined in this .cpp")
    p.add_argument("--func", help="audit only this qualified function (e.g. "
                                  "CatchChallenger::Client::parseQuery)")
    p.add_argument("--limit", type=int, help="cap number of functions")
    p.add_argument("--model", help="model override (Ollama served name, e.g. "
                                   "qwen3-coder:30b); else the configured backend")
    p.add_argument("--dry", action="store_true",
                   help="don't call the IA; print per-turn view sizes (verify the "
                        "context stays small for a 30B model)")
    args = p.parse_args(argv[1:])
    scope = ([os.path.join(REPO_ROOT, s) if not os.path.isabs(s) else s
              for s in args.scope.split(",")] if args.scope else None)
    return run(scope=scope, only_file=args.file, only_func=args.func,
               limit=args.limit, model=args.model, dry=args.dry)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
