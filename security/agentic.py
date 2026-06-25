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
import codecheck

# --- limits (sensible defaults; all env-overridable) -----------------------
ROUNDS = int(os.environ.get("CC_AGENTIC_ROUNDS", "6"))       # tool turns / IA / fn
FUNC_SECS = int(os.environ.get("CC_AGENTIC_FUNC_SECS", "180"))  # wall cap / function
CONSENSUS_MIN = int(os.environ.get("CC_CONSENSUS_MIN", "2"))  # IAs to form a wg (QA)
_TOOL_RESULT_CAP = 8000

REPO_ROOT = codecheck.REPO_ROOT

_TOOL_HELP = (
    "\n\nBefore concluding you MAY request more data — ONE request per reply, on "
    "its own first line:\n"
    "  READ <repo-relative-path>   read a file (follow a caller / a type)\n"
    "  GREP <symbol>               find where a symbol is defined / used\n"
    "  BRANCH                      show the NEXT thing this function calls\n"
    "  DONE                        finish — then give your findings\n"
    "Keep each request minimal (small context). When you have enough, reply with "
    "your findings directly (no tool line). If the function is clean, reply NO "
    "ISSUES.")

_DISCUSS_SYS = (
    "You are in a review panel. You are shown the other reviewers' findings for one "
    "function. Reply with ONLY the item numbers you AGREE are real and worth raising "
    "(comma-separated), or NONE. Agreement means you share that assessment.")

_TOOL_RE = re.compile(r"^\s*(READ|GREP|BRANCH|DONE)\b[:\s]*(.*)$")
_NUM_RE = re.compile(r"\d+")


# ---------------------------------------------------------------------------
# Panel
# ---------------------------------------------------------------------------
def resolve_panel():
    """The list of IA 'specs' (model identifiers passed to common.chat_with).
    CC_IA_PANEL=a,b,c  (comma-separated; '@host:port' pins a backend per model).
    'auto' = every installed Ollama model under 35B + 'claude' when a credential
    is set. Unset / single entry => single-IA mode. Falls back to the default
    backend (one IA) when nothing is configured."""
    spec = os.environ.get("CC_IA_PANEL", "").strip()
    if not spec:
        return [None]                      # the configured default backend, 1 IA
    if spec.lower() == "auto":
        return _auto_panel() or [None]
    return [s.strip() for s in spec.split(",") if s.strip()] or [None]


def _auto_panel():
    out = []
    try:
        import json
        import urllib.request
        req = urllib.request.Request(common.OLLAMA_DEFAULT_URL + "/api/tags")
        with urllib.request.urlopen(req, timeout=5) as r:
            tags = json.load(r)
        for m in tags.get("models", []):
            name = m.get("name", "")
            b = (m.get("details", {}) or {}).get("parameter_size", "") or ""
            mb = re.search(r"([\d.]+)\s*B", b)
            if name and (not mb or float(mb.group(1)) < 35):
                out.append(name)
    except Exception:
        pass
    if common._claude_has_creds():
        out.append("claude")
    return out


def _label(spec):
    return spec if spec else (common.CLAUDE_MODEL if common.USE_CLAUDE
                              else common.MODEL_NAME)


def _chat(spec, messages, deadline):
    timeout = max(10, int(deadline - time.time()))
    return common.chat_with(spec, messages, timeout=timeout)


# ---------------------------------------------------------------------------
# Agentic tools (pull more data, one step at a time)
# ---------------------------------------------------------------------------
def _tool_read(arg):
    p = arg.strip().strip("`'\"")
    full = p if os.path.isabs(p) else os.path.join(REPO_ROOT, p)
    if codetree.is_vendor(full):
        return "(refused: vendored library, out of scope)"
    try:
        return open(full, "r", errors="replace").read()
    except OSError as exc:
        return "(read error: %s)" % exc


def _tool_grep(arg):
    sym = arg.strip().strip("`'\"")
    if not sym:
        return "(empty symbol)"
    import subprocess
    try:
        r = subprocess.run(["grep", "-rnI", "--include=*.cpp", "--include=*.h",
                            "--include=*.hpp", "--include=*.cc", "--include=*.cxx",
                            sym] + list(codecheck.DEFAULT_SCOPE),
                           capture_output=True, text=True, timeout=30)
        return r.stdout or "(no matches)"
    except (OSError, subprocess.SubprocessError) as exc:
        return "(grep error: %s)" % exc


def _parse_tool(answer):
    for line in answer.splitlines():
        m = _TOOL_RE.match(line)
        if m:
            return (m.group(1).upper(), m.group(2).strip())
    return None


def _agentic_review(spec, idx, fi, sysprompt, deadline):
    """One IA reviews `fi` agentically, starting from the limited one-branch view
    and pulling more on request. Returns its findings text (or 'NO ISSUES')."""
    views = codecheck.build_views(idx, fi)
    first = next(views, None)
    if first is None:
        return "NO ISSUES"
    convo = [{"role": "system", "content": sysprompt},
             {"role": "user", "content": first[1] + _TOOL_HELP}]
    rounds = 0
    while rounds < ROUNDS and time.time() < deadline:
        rounds += 1
        try:
            ans = _chat(spec, convo, deadline)
        except Exception as exc:
            return "NO ISSUES"          # transport error: this IA abstains
        if not ans:
            return "NO ISSUES"
        convo.append({"role": "assistant", "content": ans})
        tool = _parse_tool(ans)
        if tool is None:
            return ans                  # no tool line -> these are the findings
        name, arg = tool
        if name == "DONE":
            # strip the DONE line; the rest (if any) are findings
            rest = "\n".join(l for l in ans.splitlines() if not _TOOL_RE.match(l))
            return rest.strip() or "NO ISSUES"
        if name == "BRANCH":
            nxt = next(views, None)
            result = nxt[1] if nxt else "(no more callee branches — conclude now)"
        elif name == "READ":
            result = _tool_read(arg)
        elif name == "GREP":
            result = _tool_grep(arg)
        else:
            result = "(unknown tool)"
        convo.append({"role": "user",
                      "content": "TOOL RESULT:\n" + result[:_TOOL_RESULT_CAP]})
    return "NO ISSUES"                   # ran out of rounds/time without concluding


# ---------------------------------------------------------------------------
# Findings helpers + discussion round -> workgroups
# ---------------------------------------------------------------------------
def _finding_lines(text):
    """Non-trivial finding lines from an IA reply ('NO ISSUES' -> none)."""
    if not text or "NO ISSUES" in text.upper():
        return []
    out = []
    for ln in text.splitlines():
        s = ln.strip()
        if s and not s.startswith(("#", "//", "===")) and len(s) > 8:
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
    "quality, NOT security). Write ONE terse brief: 'file:line: <what is wrong and "
    "why; the fix>'. You MAY reference other files (their path:line). No preamble. "
    "If on reflection there is nothing worth raising, reply exactly: NOTHING.")


def _redact_brief(lead_spec, idx, fi, finding, deadline):
    """The workgroup (via its lead) redacts the consensus brief for one finding."""
    body, _ = codetree.source_body(fi.file, fi.line)
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
    Returns a list of output strings (briefs or confirmed exploits); EMPTY when the
    panel has nothing to say."""
    deadline = time.time() + FUNC_SECS
    if sysprompt is None:
        sysprompt = codecheck.CHECK_SYSTEM
    # 1. independent agentic review (shared budget)
    per_ia = {}
    for spec in specs:
        if time.time() >= deadline:
            break
        per_ia[_label(spec) + ":" + str(id(spec))] = (
            spec, _agentic_review(spec, idx, fi, sysprompt, deadline))

    # single-IA: no workgroups
    if len(specs) < 2:
        spec, txt = next(iter(per_ia.values()), (specs[0], "NO ISSUES"))
        flines = _finding_lines(txt)
        if not flines:
            return []
        if role == "security" and exploit_cb:
            res = exploit_cb(fi, "\n".join(flines), [spec])
            return [res] if res else []
        return ["%s:%d (%s)\n%s" % (os.path.relpath(fi.file, REPO_ROOT), fi.line,
                                    fi.qual_name, "\n".join(flines))]

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
