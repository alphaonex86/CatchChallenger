#!/usr/bin/env python3
"""agentic.py — multi-IA agentic WORKGROUP engine, shared by codecheck.py
(general QA) and server.py (security + exploit).

A FRESH mechanism (NOT server.py's per-file panel). Per function:

  1. INDEPENDENT REVIEW (agentic, per IA): each IA reviews the function starting
     from codecheck's LIMITED one-branch view, and may REQUEST MORE DATA one step
     at a time — READ a file, GREP a symbol, pull the NEXT callee branch — so a
     small model's context stays lean. Bounded by ROUNDS and a per-function TIME
     cap (the same budget covers every IA).

  2. DISCUSSION ROUND: each IA is shown the panel's findings and declares which
     it AGREES are real (shares the vision). An IA can agree with several -> it
     joins several workgroups. A WORKGROUP = one shared finding + the IAs behind
     it (>= CONSENSUS_MIN agreeing for codecheck; >= 1 with potential for security).

  3. FINALIZE per workgroup, role-specific:
       - codecheck: the workgroup redacts a BRIEF (file:line, may reference other
         files). Output per function = the briefs; NOTHING if no workgroup speaks.
       - security:  the workgroup DEVELOPS an EXPLOIT via a caller-supplied
         callback (server.py's live-server exploit harness). A working exploit is
         the proof; otherwise nothing is emitted (no proof => no claim).

Single-IA mode (panel of one) skips steps 2-3: the lone IA's agentic findings are
returned directly (codecheck) or handed straight to the exploit callback (security).

Imports codecheck for the limited view (build_views) + the index; never imported
BY codecheck at module load (codecheck imports this lazily) to avoid a cycle.
"""

import os
import re
import sys
import time

import common
import codetree
# codecheck.py (the shared per-function engine) lives in tools/codecheck/ — put it on
# the path so `import codecheck` resolves from here.
_CC_DIR = os.path.normpath(os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "tools", "codecheck"))
if _CC_DIR not in sys.path:
    sys.path.insert(0, _CC_DIR)
import codecheck

# --- limits (sensible defaults; all env-overridable) -----------------------
ROUNDS = int(os.environ.get("CC_AGENTIC_ROUNDS", "6"))       # tool turns / IA / fn
FUNC_SECS = int(os.environ.get("CC_AGENTIC_FUNC_SECS", "180"))  # wall cap / function
# Grace window for the ONE final "conclude now" turn when ROUNDS/FUNC_SECS run out
# (see _agentic_review_run): without it a slow model's review is thrown away and
# the function is reported clean.
FINAL_GRACE_SECS = int(os.environ.get("CC_AGENTIC_FINAL_SECS", "120"))
CONSENSUS_MIN = int(os.environ.get("CC_CONSENSUS_MIN", "2"))  # IAs to form a wg (QA)
_TOOL_RESULT_CAP = 8000
# Keep the agentic conversation inside the server's context window so the prompt is
# NEVER truncated. Budget = (that window) - reply - margin. Fallback for the backends
# that expose no window (Claude); 32768 fits the common local models (gemma4,
# qwen-coder); set lower for a smaller-context model.
_CTX_BUDGET_TOKENS = int(os.environ.get("CC_AGENTIC_CTX_BUDGET", "32768"))

REPO_ROOT = codecheck.REPO_ROOT


def _convo_char_budget():
    """Max conversation size (chars) before we force a conclusion — keeps the prompt
    within the context (no truncation). ~3 chars/token.

    Budget against the window the SERVER gives us (common.assumed_ctx), NOT the
    model's trained context. We send no num_ctx — that would evict the running
    runner and reload the whole model — so the real window is the host's
    OLLAMA_CONTEXT_LENGTH, which is typically far below what the model was trained
    for. Budgeting off the trained ctx (131072 on gemma4:26b) against a 32768 window
    would let the conversation grow 4x past it and be silently truncated.

    Reserve the REAL reply cap, not a flat slab: prompt and answer share that one
    window, and nothing grows it to fit. Under-reserving cuts the reply at the same
    byte every turn — which the agent loop reads as a stuck model
    ([TRUNCATED REPEAT] -> finding skipped)."""
    ctx = common.assumed_ctx() or _CTX_BUDGET_TOKENS
    reply = common._ollama_num_predict() + 1024      # answer (thought included) + slab
    return max(0, ctx - reply) * 3

_TOOL_HELP = (
    "\n\nBefore concluding you MAY request more data — ONE request per reply, on "
    "its own first line:\n"
    "  READ <repo-relative-path>[:start[:end]]   read numbered source lines; "
    "use ranges to inspect truncated bodies or guards\n"
    "  GREP <symbol>               find where a symbol is defined / used\n"
    "  BRANCH                      show the NEXT thing this function calls\n"
    "  DONE                        finish — then give your findings\n"
    "Keep each request minimal (small context). When you have enough, reply with "
    "your findings directly (no tool line). If the function is clean, reply NO "
    "ISSUES. If required evidence is unavailable, reply INCONCLUSIVE: <reason>. "
    "Never treat truncated code or a failed tool request as evidence of safety.")

_DISCUSS_SYS = (
    "You are in a review panel. You are shown the other reviewers' findings for one "
    "function. Reply with ONLY the item numbers you AGREE are real and worth raising "
    "(comma-separated), or NONE. Agreement means you share that assessment.")

_TOOL_RE = re.compile(r"^\s*(READ|GREP|BRANCH|DONE)\b[:\s]*(.*)$")
_NUM_RE = re.compile(r"\d+")


# ---------------------------------------------------------------------------
# Panel
# ---------------------------------------------------------------------------
def parse_llms(raw):
    """Parse a comma-separated LLM-spec list into chat_with specs. Each entry:
      <model>[@<ollama-url>]  an Ollama model, optionally pinned to a backend host
                              (e.g. gemma4:12b, or gemma4:12b@http://gpu1:11434)
      claude[:<id>]           the OFFICIAL `claude` CLI (the ToS-safe path) —
                              normalized to claude-cli[:<id>]
    Registers any @url pin so chat_with routes the bare model name to it. NEVER
    auto-discovers anything — the list is EXACTLY what was given (the LLM is never
    chosen for the user)."""
    out = []
    for s in (raw or "").split(","):
        s = s.strip()
        if not s:
            continue
        if s == "claude":
            out.append("claude-cli")
        elif s.startswith("claude:"):
            out.append("claude-cli:" + s.split(":", 1)[1].strip())
        elif s == "claude-cli" or s.startswith("claude-cli:"):
            out.append(s)
        else:
            out.append(common._register_model_spec(s))   # @url pin -> bare name
    return out


def resolve_llms():
    """The EXPLICIT LLM list from CC_IA_PANEL — NEVER auto-chosen. Returns [] when
    unset; the caller MUST require >= 1 (an LLM is mandatory for codecheck.py /
    server.py; multiple => panel/workgroup)."""
    return parse_llms(os.environ.get("CC_IA_PANEL", ""))


def _label(spec):
    return spec if spec else (common.CLAUDE_MODEL if common.USE_CLAUDE
                              else common.MODEL_NAME)


def _chat(spec, messages, deadline):
    common.check_cancelled()
    timeout = max(10, int(deadline - time.time()))
    answer = common.chat_with(spec, messages, timeout=timeout)
    common.check_cancelled()
    if common.last_reply_truncated():
        raise ReviewIncomplete("model reply was truncated; no complete verdict: %s"
                               % (answer or "(empty response)")[:1000])
    return answer


# ---------------------------------------------------------------------------
# Agentic tools (pull more data, one step at a time)
# ---------------------------------------------------------------------------
def _tool_read(arg):
    p = arg.strip().strip("`'\"")
    start, end = 1, None
    match = re.fullmatch(r"(.+?):(\d+)(?::(\d+))?", p)
    if match:
        p, start = match.group(1), int(match.group(2))
        end = int(match.group(3)) if match.group(3) else None
    if start < 1 or (end is not None and end < start):
        return "(read error: expected 1-based start <= end)"
    full = p if os.path.isabs(p) else os.path.join(REPO_ROOT, p)
    full = os.path.realpath(full)
    if not full.startswith(os.path.realpath(REPO_ROOT) + os.sep):
        return "(refused: file outside repository)"
    if codetree.is_vendor(full):
        return "(refused: vendored library, out of scope)"
    try:
        with open(full, "r", errors="replace") as source:
            lines = source.readlines()
    except OSError as exc:
        return "(read error: %s)" % exc
    if start > len(lines):
        return "(read error: start exceeds file length %d)" % len(lines)
    end = min(end or len(lines), len(lines))
    out, size = [], 0
    for number in range(start, end + 1):
        line = "%d: %s" % (number, lines[number - 1])
        if size + len(line) > _TOOL_RESULT_CAP - 256:
            if not out:
                return codecheck._cap(line, _TOOL_RESULT_CAP - 256, "source line")
            out.append("\n[truncated; continue with READ %s:%d:%d]\n"
                       % (p, number, end))
            break
        out.append(line)
        size += len(line)
    return "".join(out)


def _tool_grep(arg):
    sym = arg.strip().strip("`'\"")
    if not sym:
        return "(empty symbol)"
    import subprocess
    try:
        r = subprocess.run(["grep", "-rnI", "--include=*.cpp", "--include=*.h",
                            "--include=*.hpp", "--include=*.cc", "--include=*.cxx",
                            "-F", "--", sym] + list(codecheck.DEFAULT_SCOPE),
                           capture_output=True, text=True, timeout=30)
        if r.returncode not in (0, 1):
            return "(grep error: %s)" % (r.stderr.strip() or r.returncode)
        return r.stdout or "(no matches)"
    except (OSError, subprocess.SubprocessError) as exc:
        return "(grep error: %s)" % exc


def _parse_tool(answer):
    for line in common.unwrap_tool_call(answer).splitlines():
        m = _TOOL_RE.match(line)
        if m:
            return (m.group(1).upper(), m.group(2).strip())
    return None


class ReviewIncomplete(RuntimeError):
    """The reviewer did not produce a usable conclusion; never a clean verdict."""


def _review_result(answer):
    """Accept explicit clean verdicts or structured findings, never stray prose."""
    answer = (answer or "").strip()
    findings = []
    for line in answer.splitlines():
        # Local models sometimes emit "high | file:line" despite the template.
        # Normalize this equivalent form rather than losing a security candidate.
        line = re.sub(r"^\s*(low|medium|high|critical)\s*\|",
                      lambda m: "SEVERITY(%s) |" % m.group(1).upper(), line,
                      count=1, flags=re.I)
        if (codecheck._SEVERITY_RE.match(line.strip())
                and re.search(r'\|\s*[^\s|]+:\d+(?:\s*\||:)', line)):
            findings.append(line.strip())
    if findings:
        return "\n".join(findings)
    # Accept an explicit terminal verdict with an explanation, not a mention of
    # the required format inside reasoning. Findings always take precedence.
    if (re.search(r"\bINCONCLUSIVE\b", answer, re.I)
            or "[reply truncated at " in answer):
        return None
    lines = answer.splitlines()
    if lines and (re.fullmatch(r"NO ISSUES\.?\s*(?:\[done\])?", lines[-1], re.I)
                  or (len(lines) == 1
                      and re.match(r"^NO ISSUES\s+[—–-]\s+\S", answer, re.I))):
        return "NO ISSUES"
    return None


def _agentic_review(spec, idx, fi, sysprompt, deadline):
    """One IA reviews `fi` agentically. INCREMENTAL: a CONCLUDED (spec, function-
    source, sysprompt) result is cached, so a re-run skips unchanged functions for
    this IA. A transport error / budget timeout is inconclusive and is NOT cached,
    so a transient outage can't poison the cache."""
    body, _ = codetree.function_body(fi)
    if not body:
        raise ReviewIncomplete("source body unavailable at %s:%d; no model request sent"
                               % (os.path.relpath(fi.file, REPO_ROOT), fi.line))
    views = list(codecheck.build_views(idx, fi))   # materialize: cache key + the loop
    if not views:
        raise ReviewIncomplete("no source views available")
    identity = _label(spec)
    if not common.USE_CLAUDE and not identity.startswith("claude"):
        identity += repr((common._ollama_api_kind(), common.backend_for_model(identity),
                          common._ollama_think()))
    material = ["agentic-v3:" + identity, sysprompt, _TOOL_HELP, codecheck.TIDY_CHECKS,
                "".join(c for _, c in views)]
    cached = codecheck.verdict_get(material)
    if cached is not None:
        return cached
    codecheck._CACHE_STATS["miss"] += 1
    answer = _agentic_review_run(spec, views, fi, sysprompt, deadline)
    result = _review_result(answer)
    if result is None:
        raise ReviewIncomplete("review lacks an explicit clean verdict or structured finding: %s"
                               % ((answer or "(empty response)")[:1000]))
    codecheck.verdict_put(material, result)
    return result


def _agentic_review_run(spec, views, fi, sysprompt, deadline):
    """The agentic loop over the materialized `views`: start from the base view,
    pull the next callee BRANCH (or READ/GREP) on request. Returns the concluded
    findings text (or 'NO ISSUES'), or None when it could NOT conclude (transport
    error, empty reply, ran out of rounds/time)."""
    # Keep fixed instructions before variable source so llama.cpp can reuse their
    # KV prefix between functions. Content and verdict-cache inputs are unchanged.
    convo = [{"role": "system", "content": sysprompt + _TOOL_HELP},
             {"role": "user", "content": views[0][1]}]
    budget = _convo_char_budget()
    vi = 1                              # next callee-branch index into `views`
    rounds = 0
    format_retried = False
    while rounds < ROUNDS and time.time() < deadline:
        rounds += 1
        # CONTEXT GUARD: near the model's limit -> force a final answer this turn so
        # the accumulated prompt is NEVER truncated.
        if sum(len(m["content"]) for m in convo) > budget:
            raise ReviewIncomplete("conversation exceeds context budget; no oversized request sent")
        try:
            ans = _chat(spec, convo, deadline)
        except Exception as exc:
            raise ReviewIncomplete("model request failed: %s" % exc) from exc
        if not ans:
            return None
        ans = codecheck.collapse_repetition(ans)       # defang an LLM text-loop
        convo.append({"role": "assistant", "content": ans})
        tool = _parse_tool(ans)
        if tool is None:
            # A verbose conclusion otherwise discards the completed review and
            # repeats all inference on the next run. Repair once, within budget,
            # retaining the evidence and every suspected finding in context.
            if (_review_result(ans) is None and not format_retried
                    and not ans.lstrip().upper().startswith("INCONCLUSIVE")
                    and rounds < ROUNDS and time.time() < deadline):
                format_retried = True
                convo.append({"role": "user", "content":
                    "Reformat your conclusion using the required output format. "
                    "Preserve every suspected issue and its evidence; do not "
                    "restart the analysis or invent evidence. Use one "
                    "SEVERITY(low|medium|high|critical) | file:line | evidence "
                    "line per finding. Only if your review concluded clean, "
                    "reply exactly NO ISSUES. If evidence is missing, reply "
                    "INCONCLUSIVE: <reason>. No preamble or explanation outside "
                    "the finding lines."})
                continue
            return ans                  # no tool line -> these are the findings
        name, arg = tool
        if name == "DONE":
            # strip the DONE line; the rest (if any) are findings
            rest = "\n".join(l for l in ans.splitlines() if not _TOOL_RE.match(l))
            return rest.strip() or None
        if name == "BRANCH":
            if vi < len(views):
                result = views[vi][1]
                # The base view is already in the conversation. Repeating it can
                # exhaust the tool-result cap before the new callee body appears.
                branch_start = result.find("\n=== ONE THING IT CALLS")
                if branch_start >= 0:
                    result = result[branch_start:]
                vi += 1
            else:
                result = "(no more callee branches — conclude now)"
        elif name == "READ":
            result = _tool_read(arg)
        elif name == "GREP":
            result = _tool_grep(arg)
        else:
            result = "(unknown tool)"
        convo.append({"role": "user",
                      "content": "TOOL RESULT:\n" + codecheck._cap(
                          result, _TOOL_RESULT_CAP, "tool result")})
    # Rounds/time exhausted: give the model one final chance to conclude.
    # Ask ONCE, on a small extra grace window, for the findings it already has;
    # only a failed/empty final turn stays inconclusive (and uncached).
    convo.append({"role": "user", "content":
        "TIME/STEP BUDGET REACHED - request no more data. Give your FINAL "
        "findings now (file:line). Reply NO ISSUES only if the available evidence "
        "is sufficient; otherwise reply INCONCLUSIVE: <missing evidence>."})
    if sum(len(m["content"]) for m in convo) > budget:
        raise ReviewIncomplete("conversation exceeds context budget before final verdict")
    try:
        ans = _chat(spec, convo, time.time() + FINAL_GRACE_SECS)
    except Exception as exc:
        raise ReviewIncomplete("final model request failed: %s" % exc) from exc
    if not ans:
        return None
    ans = codecheck.collapse_repetition(ans)
    rest = "\n".join(l for l in ans.splitlines() if not _TOOL_RE.match(l)).strip()
    return rest or None


# ---------------------------------------------------------------------------
# Findings helpers + discussion round -> workgroups
# ---------------------------------------------------------------------------
def _finding_lines(text):
    """Non-trivial finding lines from an IA reply ('NO ISSUES' -> none)."""
    if not text:
        return []
    out = []
    for ln in text.splitlines():
        s = ln.strip()
        if (s and s.upper().rstrip(".") != "NO ISSUES"
                and not s.startswith(("#", "//", "===")) and len(s) > 8):
            out.append(s)
    return out


def _sig(line):
    """Cluster signature for a finding line: a file:line token if present, else a
    normalized word-set, so near-identical findings from different IAs merge."""
    m = re.search(r"([\w./]+:\d+)", line)
    if m:
        return m.group(1)
    return " ".join(sorted(set(re.findall(r"[a-z]{4,}", line.lower())))[:8])


def _dedup(lines):
    seen = {}
    for ln in lines:
        seen.setdefault(_sig(ln), ln)
    return list(seen.values())


def _discuss(specs, per_ia, fi, deadline):
    """Show every IA the panel's deduped findings; each replies which numbers it
    AGREES are real. Returns workgroups: [(finding_text, [member_specs])]."""
    allf = _dedup([ln for _spec, txt in per_ia.values() for ln in _finding_lines(txt)])
    if not allf:
        return []
    numbered = "\n".join("%d. %s" % (i + 1, t) for i, t in enumerate(allf))
    groups = [[] for _ in allf]
    head = "Function under review: %s (%s:%d)\n\nPanel findings:\n%s\n\n" % (
        fi.qual_name, os.path.relpath(fi.file, REPO_ROOT), fi.line, numbered)
    for spec in specs:
        if time.time() >= deadline:
            break
        try:
            ans = _chat(spec, [{"role": "system", "content": _DISCUSS_SYS},
                               {"role": "user", "content": head}], deadline)
        except Exception:
            continue
        if not ans or "NONE" in ans.upper().split("\n")[0]:
            continue
        for n in {int(x) for x in _NUM_RE.findall(ans)}:
            if 1 <= n <= len(allf):
                groups[n - 1].append(spec)
    return [(allf[i], members) for i, members in enumerate(groups) if members]


# ---------------------------------------------------------------------------
# Finalize: codecheck BRIEF (consensus) / security EXPLOIT (callback)
# ---------------------------------------------------------------------------
_BRIEF_SYS = (
    "You speak for a review workgroup that agreed on a code-quality issue (general "
    "quality, NOT security). Write ONE terse brief: "
    "'SEVERITY(LOW|MEDIUM|HIGH|CRITICAL) | file:line: <what is wrong and why; the "
    "fix>'. LOW = style/naming/clarity nit; MEDIUM = real improvement or probable "
    "minor bug; HIGH = definite bug producing wrong behaviour; CRITICAL = definite "
    "crash/data-loss bug. Keep HIGH or CRITICAL ONLY when the shown code PROVES it "
    "- you must be SURE; when in doubt, MEDIUM. You MAY reference other files "
    "(their path:line). No preamble. If on reflection there is nothing worth "
    "raising, reply exactly: NOTHING.")


def _redact_brief(lead_spec, idx, fi, finding, deadline):
    """The workgroup (via its lead) redacts the consensus brief for one finding."""
    body, _ = codetree.function_body(fi)
    try:
        ans = _chat(lead_spec, [
            {"role": "system", "content": _BRIEF_SYS},
            {"role": "user", "content":
                "The agreed finding:\n%s\n\nThe function (%s:%d):\n%s\n\nWrite the brief."
                % (finding, os.path.relpath(fi.file, REPO_ROOT), fi.line, body[:6000])}],
            deadline)
    except Exception:
        return ""
    a = (ans or "").strip()
    return "" if (not a or "NOTHING" in a.upper()) else a


# ---------------------------------------------------------------------------
# Orchestrator (one function)
# ---------------------------------------------------------------------------
def audit_function(idx, fi, specs, role="codecheck", exploit_cb=None, sysprompt=None):
    """Multi-IA agentic review of ONE function. role='codecheck' -> consensus
    briefs; role='security' -> exploit_cb(fi, finding, members) develops a proof.
    Returns a list of output strings (deterministic static-analysis findings +
    briefs/confirmed exploits); EMPTY when nothing (algorithmic or IA) was found."""
    deadline = time.time() + FUNC_SECS
    if sysprompt is None:
        sysprompt = codecheck.CHECK_SYSTEM
    rel = os.path.relpath(fi.file, REPO_ROOT)
    # Deterministic clang-tidy findings come from codecheck.file_sweep (whole-file,
    # every function) — NOT re-emitted here; this is pure IA judgment.
    # 1. independent agentic review (shared budget)
    per_ia = {}
    if not specs:
        raise ReviewIncomplete("no reviewers configured")
    for spec in specs:
        if time.time() >= deadline:
            raise ReviewIncomplete("review budget exhausted before every reviewer ran")
        per_ia[_label(spec) + ":" + str(id(spec))] = (
            spec, _agentic_review(spec, idx, fi, sysprompt, deadline))

    # single-IA: no workgroups
    if len(specs) < 2:
        spec, txt = next(iter(per_ia.values()))
        flines = _finding_lines(txt)
        if not flines:
            return []
        if role == "security" and exploit_cb:
            res = exploit_cb(fi, "\n".join(flines), [spec])
            return [res] if res else []
        return ["%s:%d (%s)\n%s"
                % (rel, fi.line, fi.qual_name, "\n".join(flines))]

    # 2. discussion -> workgroups
    wgs = _discuss(specs, per_ia, fi, deadline)

    # 3. finalize per workgroup
    out = []
    for finding, members in wgs:
        if role == "security":
            # a security workgroup pursues a proof if ANY member sees potential
            if exploit_cb and members:
                res = exploit_cb(fi, finding, members)
                if res:
                    out.append(res)
            elif members:
                # no proof harness wired here: emit the workgroup-vetted candidate;
                # the caller (server.py) drives exploit development on it.
                out.append("%s:%d (%s)\n%s" % (os.path.relpath(fi.file, REPO_ROOT),
                           fi.line, fi.qual_name, finding))
        else:
            # codecheck: CONSENSUS required (>= CONSENSUS_MIN agreeing IAs)
            if len(members) >= CONSENSUS_MIN:
                brief = _redact_brief(members[0], idx, fi, finding, deadline)
                if brief:
                    out.append(brief)
    return out
