#!/usr/bin/env python3
"""security-server.py - one tool, two phases, driven by deepseek-coder:33b.

PHASE 1  scan    : agentic security audit of server/cli + its repo-local
                   includes (vendored libs excluded). Prints candidate
                   findings to stdout; project-wide facts accrue in
                   SECURITY-IA.md. The model can READ/GREP repo files for
                   context before deciding.
PHASE 2  exploit : for each memory/input-y candidate finding, an exploit-
                   engineer agent. The MODEL writes exploit.py (+ helpers)
                   into /mnt/data/perso/tmpfs/security/server/<name>/ via
                   WRITE, runs it under gdb/valgrind against the LIVE cli
                   server over TCP via a SANDBOXED RUN (bubblewrap: only the
                   exploit's own dir is writable), and emits a VERDICT. It
                   gets a 1h budget per exploit and may not bail early; if
                   the budget elapses without a confirmed crash/OOB the whole
                   exploit folder is dropped. Confirmed exploits are kept.

Everything under the temp dir /mnt/data/perso/tmpfs/security/server/ (build,
staged run, each exploit dir, REPORT.md) is produced by the model/script -
it is all server-related, hence the `server/` subfolder.

Usage:
  python3 security-server.py scan    > findings.txt 2> progress.log
  python3 security-server.py exploit                 # reads findings.txt
  python3 security-server.py all                     # scan then exploit
  (no arg defaults to `scan`)
"""

import json
import os
import re
import shutil
import subprocess
import sys
import time
import urllib.error
import urllib.request

# ===========================================================================
# Shared configuration
# ===========================================================================
REPO_ROOT = "/home/user/Desktop/CatchChallenger/git"
TARGET_DIR = os.path.join(REPO_ROOT, "server", "cli")
MODEL_NAME = "deepseek-coder:33b"
OLLAMA_API = "http://127.0.0.1:11434/api/generate"
OLLAMA_CHAT = "http://127.0.0.1:11434/api/chat"

HERE = os.path.dirname(os.path.realpath(__file__))


def _ts():
    """Wall-clock stamp for progress lines: H:m D/M/Y (e.g. 18:42 23/05/2026)."""
    return time.strftime("%H:%M %d/%m/%Y")


def _human_dur(seconds):
    """Elapsed seconds as MINmSECs (e.g. 99min09s)."""
    seconds = int(seconds)
    return "%dmin%02ds" % (seconds // 60, seconds % 60)

# All server-related generated artifacts live under this temp subfolder.
OUTPUT_ROOT = "/mnt/data/perso/tmpfs/security/server"

# ===========================================================================
# PHASE 1 - scan (audit)
# ===========================================================================

# Agent loop: max tool turns the model gets to pull extra context per file
# before it must produce findings.
SCAN_MAX_STEPS = 8
# Cap bytes returned by a single READ/GREP tool call.
TOOL_READ_BYTES = 40000
TOOL_GREP_HITS = 40

SOURCE_EXT = (".cpp", ".c", ".cc", ".cxx", ".h", ".hpp", ".hxx")

SYSTEM_PROMPT = (
    "You are a security auditor reviewing a C++ network server for "
    "EXPLOITABLE SECURITY VULNERABILITIES only. The code handles untrusted "
    "input from remote clients over sockets.\n"
    "\n"
    "Report ONLY concrete, exploitable security bugs, such as:\n"
    "- Buffer overflow / out-of-bounds read or write on attacker-controlled "
    "sizes or indices.\n"
    "- Integer overflow/underflow or signedness bugs that feed a length, "
    "index, or allocation.\n"
    "- Use-after-free, double-free, or null-deref reachable from untrusted "
    "input.\n"
    "- Missing or wrong validation of lengths/offsets read off the wire.\n"
    "- Injection (SQL, command, path traversal) from untrusted data.\n"
    "- Authentication/authorization bypass, or trusting client-supplied "
    "identifiers.\n"
    "- Race conditions with a security impact (TOCTOU, etc.).\n"
    "\n"
    "STRICT RULES:\n"
    "- This is ONE file out of a larger project. Types, classes, and methods "
    "used here are DEFINED IN OTHER FILES. A few likely-related files are "
    "shown under 'RELATED FILE' as a starting hint; fetch any others you "
    "need with the tools below.\n"
    "- DO NOT report: unused code, dead #ifdef branches, missing "
    "implementations, pure-virtual methods without a body here, default "
    "return values, enum scoping, code style, naming, maintainability, "
    "performance, or design suggestions. These are NOT security problems.\n"
    "- DO NOT report a symbol as 'undefined' or 'not implemented' just "
    "because its body is not in the MAIN FILE; assume it is defined "
    "elsewhere unless you can see it is genuinely wrong.\n"
    "- Only flag an issue if you can name the attacker input, the code path, "
    "and the concrete consequence. If unsure, do not report it.\n"
    "\n"
    "TOOLS - to inspect more code before deciding, reply with ONE line and "
    "nothing else:\n"
    "  READ <path>     - return a repo file (relative path or bare filename)\n"
    "  GREP <symbol>   - find where a symbol/string is defined or used\n"
    "Use tools when a method's behaviour matters to the verdict (e.g. how a "
    "length is parsed upstream). I will reply with the result and you may "
    "call another tool. When you have enough information, STOP calling tools "
    "and output your final answer. Do NOT mix a tool line with findings.\n"
    "\n"
    "For each real finding give: line number(s) in the MAIN FILE, the "
    "untrusted input, and the consequence. Be terse and technical. "
    "If you find no exploitable security vulnerability in the MAIN FILE, "
    "reply with exactly 'No issues identified' and nothing else.\n"
    "\n"
    "PROJECT NOTEBOOK: you are given the running notebook (SECURITY-IA.md) "
    "below. Use it as prior context. It is PROJECT-WIDE knowledge ONLY: "
    "trust boundaries, where untrusted client data enters the system, shared "
    "buffers/state, protocol/parsing entry points, cross-file invariants - "
    "facts useful when auditing OTHER files. If THIS file reveals such a "
    "project-wide fact NOT already in the notebook, append it by emitting a "
    "block delimited exactly like:\n"
    "<<<NOTES\n"
    "- short project-wide fact\n"
    "NOTES>>>\n"
    "Do NOT put single-file details in the notebook (a bug local to this "
    "file, a line specific to this file). Omit the block entirely if there "
    "is nothing project-wide and new.\n"
    "\n"
    "If you have data useful only for THIS file, do not put it in the "
    "notebook: start your reply with one short '// ' comment-header line, "
    "terse, not verbose."
)

# Where to look for a referenced class/header when it does not resolve by a
# relative include path.
SEARCH_DIRS = (
    os.path.join(REPO_ROOT, "server"),
    os.path.join(REPO_ROOT, "general", "base"),
    os.path.join(REPO_ROOT, "general", "fight"),
)

# Cap the context we attach so a single oversized file can't blow the model's
# context window. Bytes, applied per related file and to the total.
MAX_RELATED_BYTES = 48000
MAX_RELATED_FILES = 16

# Running audit notebook the model reads as context and appends to.
SECURITY_NOTES_FILE = os.path.join(HERE, "SECURITY-IA.md")
# The model wraps anything it wants persisted between these markers.
NOTE_RE = re.compile(r"<<<NOTES(.*?)NOTES>>>", re.DOTALL)

# num_ctx bounds (tokens). We size it to the actual payload, clamped here.
MIN_CTX = 8192
MAX_CTX = 32768

# Quoted local includes only; <system> includes are ignored.
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*"([^"]+)"', re.MULTILINE)

# Vendored / embedded third-party libs (see root CLAUDE.md do-not-modify
# list). We audit OUR code only, never vendor forks - fixes belong in our
# call sites, not in these trees. Excluded from the audited set; they may
# still be pulled in as read-only CONTEXT by related_files()/tool_read.
VENDOR_DIRS = tuple(os.path.realpath(os.path.join(REPO_ROOT, p)) for p in (
    "general/blake3",
    "general/hps",
    "general/libxxhash",
    "general/libzstd",
    "general/tinyXML2",
    "client/libqtcatchchallenger/libogg",
    "client/libqtcatchchallenger/libopus",
    "client/libqtcatchchallenger/libopusfile",
    "client/libqtcatchchallenger/libtiled",
))


def is_vendor(path):
    rp = os.path.realpath(path)
    return any(rp == d or rp.startswith(d + os.sep) for d in VENDOR_DIRS)


def collect_files():
    """Return an ordered list of repo files to audit: every source file in
    the cli project plus, transitively, every local header/source it
    #includes that still lives under REPO_ROOT (vendored libs excluded)."""
    repo_root = os.path.realpath(REPO_ROOT)
    seen = set()
    ordered = []
    pending = []

    # Seed with the cli project's own sources (recursive).
    for root, _dirs, names in os.walk(TARGET_DIR):
        for name in sorted(names):
            if name.endswith(SOURCE_EXT):
                pending.append(os.path.realpath(os.path.join(root, name)))

    pending.sort()

    while pending:
        path = pending.pop(0)
        if path in seen:
            pass
        else:
            seen.add(path)
            # Keep only files that live inside the repo, and never audit or
            # descend into vendored third-party code.
            if path.startswith(repo_root + os.sep) and os.path.isfile(path) \
                    and not is_vendor(path):
                ordered.append(path)
                try:
                    text = open(path, "r", errors="replace").read()
                except OSError:
                    text = ""
                base = os.path.dirname(path)
                for inc in INCLUDE_RE.findall(text):
                    cand = os.path.realpath(os.path.join(base, inc))
                    if cand not in seen and not is_vendor(cand):
                        pending.append(cand)

    return ordered


_basename_index = None


def find_by_basename(name):
    """Locate a header/source by basename under SEARCH_DIRS. Built once and
    cached. Returns an absolute path or None."""
    global _basename_index
    if _basename_index is None:
        _basename_index = {}
        for d in SEARCH_DIRS:
            for root, _dirs, names in os.walk(d):
                for n in names:
                    if n.endswith(SOURCE_EXT):
                        _basename_index.setdefault(n, os.path.join(root, n))
    return _basename_index.get(name)


def related_files(path):
    """Repo-local files referenced by `path` (its quoted #includes)."""
    repo_root = os.path.realpath(REPO_ROOT)
    base = os.path.dirname(path)
    out = []
    seen = set()
    try:
        text = open(path, "r", errors="replace").read()
    except OSError:
        return out
    for inc in INCLUDE_RE.findall(text):
        cand = os.path.realpath(os.path.join(base, inc))
        if not (cand.startswith(repo_root + os.sep) and os.path.isfile(cand)):
            cand = find_by_basename(os.path.basename(inc))
        if cand and cand != path and cand not in seen:
            seen.add(cand)
            out.append(cand)
        if len(out) >= MAX_RELATED_FILES:
            break
    return out


def read_notes():
    """Current SECURITY-IA.md content (empty string if absent)."""
    try:
        return open(SECURITY_NOTES_FILE, "r", errors="replace").read()
    except OSError:
        return ""


def append_notes(rel, block):
    """Append model-emitted project-wide facts to SECURITY-IA.md (flat list)."""
    block = block.strip()
    if block:
        header = "" if os.path.exists(SECURITY_NOTES_FILE) else (
            "# SECURITY-IA.md - project-wide security audit notebook\n"
            "# Trust boundaries, untrusted-input entry points, shared state, "
            "cross-file invariants.\n\n"
        )
        with open(SECURITY_NOTES_FILE, "a") as fh:
            fh.write("%s%s\n" % (header, block))


def fit_ctx(*texts):
    """Pick a num_ctx that fits the payload (~3 chars/token + reply headroom),
    clamped to [MIN_CTX, MAX_CTX] and rounded up to a 2048 multiple."""
    chars = 0
    for t in texts:
        chars += len(t)
    tokens = chars // 3 + 2048
    tokens = ((tokens + 2047) // 2048) * 2048
    if tokens < MIN_CTX:
        tokens = MIN_CTX
    if tokens > MAX_CTX:
        tokens = MAX_CTX
    return tokens


def tool_read(arg):
    """READ <path>: return a repo file, resolved by relative path or bare
    basename under SEARCH_DIRS. Capped to TOOL_READ_BYTES."""
    repo_root = os.path.realpath(REPO_ROOT)
    cand = os.path.realpath(os.path.join(REPO_ROOT, arg))
    if not (cand.startswith(repo_root + os.sep) and os.path.isfile(cand)):
        cand = find_by_basename(os.path.basename(arg))
    if not cand or not os.path.isfile(cand):
        return "READ: not found in repo: %s" % arg
    try:
        text = open(cand, "r", errors="replace").read()
    except OSError as exc:
        return "READ: error: %s" % exc
    rrel = os.path.relpath(cand, REPO_ROOT)
    clipped = text[:TOOL_READ_BYTES]
    if len(text) > TOOL_READ_BYTES:
        clipped += "\n... [truncated]"
    return "=== %s ===\n%s" % (rrel, clipped)


def tool_grep(symbol):
    """GREP <symbol>: file:line matches under SEARCH_DIRS, capped."""
    symbol = symbol.strip()
    if not symbol:
        return "GREP: empty query"
    try:
        out = subprocess.run(
            ["grep", "-rnI", "--include=*.cpp", "--include=*.h",
             "--include=*.hpp", "--include=*.cc", "--include=*.cxx",
             "-e", symbol, "--", *SEARCH_DIRS],
            capture_output=True, text=True, timeout=60,
        ).stdout
    except (OSError, subprocess.SubprocessError) as exc:
        return "GREP: error: %s" % exc
    hits = []
    for line in out.splitlines():
        if line.startswith(REPO_ROOT + os.sep):
            line = line[len(REPO_ROOT) + 1:]
        hits.append(line)
        if len(hits) >= TOOL_GREP_HITS:
            hits.append("... [more hits omitted]")
            break
    return "\n".join(hits) if hits else "GREP: no matches for %s" % symbol


def chat(messages, timeout=None):
    """One streamed /api/chat turn. Returns the assistant message text.
    Retries transient Ollama outages (ECONNREFUSED) up to 5x with backoff.

    `timeout` (seconds, optional) is a HARD wall-clock cap on the whole call,
    used by the exploit phase to clamp each turn to the remaining per-exploit
    budget so a single streaming reply cannot overrun the deadline. When set:
    the socket read timeout is bounded by it, the stream stops early and
    returns whatever has arrived once the cap is hit, and we never retry/back-
    off past it. None = unbounded (scan phase)."""
    deadline = (time.time() + timeout) if timeout else None
    body = "".join(m["content"] for m in messages)
    payload = {
        "model": MODEL_NAME,
        "messages": messages,
        "stream": True,
        "options": {
            "temperature": 0.1,
            "num_ctx": fit_ctx(body),
            # Bound a single reply and discourage the 33B's repetition loops
            # (it sometimes spews the same READ/GREP line thousands of times).
            "num_predict": 2048,
            "repeat_penalty": 1.3,
        },
    }
    data = json.dumps(payload).encode("utf-8")
    attempt = 0
    while True:
        attempt += 1
        # Socket-level timeout: bound a stalled read by the time left (so a hung
        # connection can't block past the budget), else a sane default.
        if deadline:
            sock_to = max(1, int(deadline - time.time()))
        else:
            sock_to = 120
        try:
            req = urllib.request.Request(
                OLLAMA_CHAT, data=data,
                headers={"Content-Type": "application/json"},
            )
            chunks = []
            with urllib.request.urlopen(req, timeout=sock_to) as resp:
                for raw in resp:
                    # Hard wall-clock cap: stop reading and return the partial
                    # reply; the caller's deadline check then ends the exploit.
                    if deadline and time.time() >= deadline:
                        sys.stderr.write("    [chat cut: budget reached mid-stream]\n")
                        break
                    raw = raw.strip()
                    if raw:
                        try:
                            obj = json.loads(raw)
                        except ValueError:
                            pass
                        else:
                            msg = obj.get("message") or {}
                            piece = msg.get("content")
                            if piece:
                                chunks.append(piece)
            return "".join(chunks).strip()
        except (urllib.error.URLError, ConnectionError, OSError) as exc:
            # Don't retry/backoff past the wall-clock cap - return empty so the
            # caller's deadline check fires.
            if deadline and time.time() >= deadline:
                sys.stderr.write("    [chat give up: budget reached after %s]\n" % exc)
                return ""
            if attempt >= 5:
                raise
            sys.stderr.write("    [chat retry %d/5 after %s]\n" % (attempt, exc))
            # Don't sleep past the deadline either.
            nap = 3 * attempt
            if deadline:
                nap = min(nap, max(0, int(deadline - time.time())))
            time.sleep(nap)


def parse_tool(answer):
    """If the reply is a single READ/GREP tool call, return (name, arg)."""
    stripped = answer.strip()
    first = stripped.splitlines()[0].strip() if stripped else ""
    for name in ("READ", "GREP"):
        prefix = name + " "
        if first.startswith(prefix) and len(stripped) - len(first) < 40:
            return name, first[len(prefix):].strip()
    return None


def analyze(path):
    """Agentic audit of one file. The NOTES block is stripped + persisted."""
    rel = os.path.relpath(path, REPO_ROOT)
    try:
        content = open(path, "r", errors="replace").read()
    except OSError as exc:
        return "[read error] %s" % exc

    context = []
    budget = MAX_RELATED_BYTES
    for rpath in related_files(path):
        if budget <= 0:
            break
        try:
            rtext = open(rpath, "r", errors="replace").read()
        except OSError:
            rtext = ""
        rtext = rtext[:budget]
        budget -= len(rtext)
        rrel = os.path.relpath(rpath, REPO_ROOT)
        context.append(
            "=== RELATED FILE (starting hint, do not audit): %s ===\n%s"
            % (rrel, rtext)
        )
    context_block = ("\n\n".join(context) + "\n\n") if context else ""

    notes = read_notes()
    notes_block = (
        "=== SECURITY-IA.md (project notebook so far) ===\n%s\n\n" % notes
        if notes.strip()
        else ""
    )

    user = (
        "%s%sAudit the MAIN FILE below for exploitable security "
        "vulnerabilities. Report findings only for the MAIN FILE. Call "
        "READ/GREP first if you need to see how something is defined.\n\n"
        "=== MAIN FILE: %s ===\n\n%s"
        % (notes_block, context_block, rel, content)
    )
    messages = [
        {"role": "system", "content": SYSTEM_PROMPT},
        {"role": "user", "content": user},
    ]

    answer = ""
    step = 0
    while step < SCAN_MAX_STEPS:
        step += 1
        answer = chat(messages)
        tool = parse_tool(answer)
        if tool is None:
            break
        name, arg = tool
        result = tool_read(arg) if name == "READ" else tool_grep(arg)
        sys.stderr.write("    [tool] %s %s\n" % (name, arg))
        messages.append({"role": "assistant", "content": answer})
        messages.append({
            "role": "user",
            "content": "Tool result for `%s %s`:\n%s\n\nContinue: call "
            "another tool or give your final answer." % (name, arg, result),
        })
    else:
        messages.append({
            "role": "user",
            "content": "No more tool calls. Give your final answer now "
            "based on what you have.",
        })
        answer = chat(messages)

    for block in NOTE_RE.findall(answer):
        append_notes(rel, block)
    answer = NOTE_RE.sub("", answer).strip()
    if looks_degenerate(answer):
        sys.stderr.write("    [discarded degenerate output: repetition loop]\n")
        return ""
    return answer


def looks_degenerate(text):
    """True if the model fell into a repetition loop (same line many times) or
    just spewed tool-call lines as its 'answer'. Such output is junk, not a
    finding - drop it rather than dumping thousands of repeated lines."""
    lines = [ln.strip() for ln in text.splitlines() if ln.strip()]
    if len(lines) < 8:
        return False
    counts = {}
    toolish = 0
    maxc = 0
    for ln in lines:
        counts[ln] = counts.get(ln, 0) + 1
        if counts[ln] > maxc:
            maxc = counts[ln]
        if ln.startswith("READ ") or ln.startswith("GREP "):
            toolish += 1
    # same line repeated 5+ times = loop; or a majority of tool-call lines.
    return maxc > 4 or toolish * 2 > len(lines)


def has_findings(answer):
    """A clean file answers (only) 'No issues identified'."""
    if not answer:
        return False
    normalized = re.sub(r"[^a-z ]", " ", answer.lower())
    normalized = " ".join(normalized.split())
    return "no issues identified" not in normalized


def run_scan(out=sys.stdout):
    """Phase 1. Findings blocks are written to `out` (default stdout, so the
    caller can redirect to findings.txt). Returns 0/1."""
    if not os.path.isdir(TARGET_DIR):
        sys.stderr.write("[ERROR] Target directory not found: %s\n" % TARGET_DIR)
        return 1

    files = collect_files()
    scan_start = time.time()
    sys.stderr.write(
        "%s [INIT] %d files to audit (cli project + repo-local includes, vendor "
        "excluded) via %s\n" % (_ts(), len(files), MODEL_NAME)
    )

    flagged = []
    for i, path in enumerate(files, 1):
        rel = os.path.relpath(path, REPO_ROOT)
        sys.stderr.write("%s [%d/%d] %s\n" % (_ts(), i, len(files), rel))
        sys.stderr.flush()
        try:
            answer = analyze(path)
        except Exception as exc:  # network / ollama failure - report, continue
            sys.stderr.write("    [skip] %s\n" % exc)
            continue
        if has_findings(answer):
            flagged.append(rel)
            print("=" * 70, file=out)
            print("FINDINGS: %s" % rel, file=out)
            print("=" * 70, file=out)
            print(answer, file=out)
            print(file=out)
            out.flush()

    print("=" * 70, file=out)
    print("SUMMARY: %d/%d files flagged" % (len(flagged), len(files)), file=out)
    print("=" * 70, file=out)
    if flagged:
        for rel in flagged:
            print("  - %s" % rel, file=out)
    else:
        print("  No issues identified across the scanned project.", file=out)
    out.flush()
    scan_secs = time.time() - scan_start
    try:
        open(SCAN_TIME_FILE, "w").write("%d\n" % int(scan_secs))
    except OSError as exc:
        sys.stderr.write("    [warn] could not record scan time: %s\n" % exc)
    sys.stderr.write("%s [DONE] TOTAL time to scan all files: %s\n"
                     % (_ts(), _human_dur(scan_secs)))
    sys.stderr.flush()
    return 0


# ===========================================================================
# PHASE 2 - exploit (model-generated proofs)
# ===========================================================================
FINDINGS = os.path.join(HERE, "findings.txt")
# Prebuilt, gdb-able server + a staged run dir the model reuses.
SERVER_BIN = os.path.join(OUTPUT_ROOT, "build", "catchchallenger-server-cli")
STAGED_RUN = os.path.join(OUTPUT_ROOT, "run")
SERVER_PORT = 61997

# Per-exploit WALL-CLOCK budget. This is a CAP, not a floor: the model may
# keep rewriting/retrying up to this long, but it ends EARLY on a CONFIRMED
# proof OR once it is sure it cannot be exploited (VERDICT FALSEPOSITIVE).
# Budget reached without a confirmed crash/OOB -> not-exploitable.
# Either way a non-confirmed finding's exploit folder is dropped.
# Per-exploit budgets are DERIVED from the measured total scan time (written to
# SCAN_TIME_FILE by the scan phase): hard cap = total scan time (100%), soft cap
# = 75% of it. Env vars override either. Fallback if no scan time is on disk
# (e.g. running `exploit` standalone before any scan): 1h hard / 75% soft.
SCAN_TIME_FILE = os.path.join(HERE, "scan-seconds.txt")
FALLBACK_BUDGET = 3600


def read_scan_seconds():
    """Total scan seconds recorded by the last scan, or None if absent/bad."""
    try:
        return int(float(open(SCAN_TIME_FILE).read().strip()))
    except (OSError, ValueError):
        return None


def resolve_budgets():
    """(hard, soft) per-exploit budgets in seconds. Env overrides win; else
    hard = measured total scan time, soft = 75% of hard."""
    env_hard = os.environ.get("SECSERVER_BUDGET")
    hard = int(env_hard) if env_hard else (read_scan_seconds() or FALLBACK_BUDGET)
    env_soft = os.environ.get("SECSERVER_SOFT_BUDGET")
    soft = int(env_soft) if env_soft else int(hard * 0.75)
    return hard, soft
EXPLOIT_MAX_STEPS = int(os.environ.get("SECSERVER_STEPS", "400"))  # safety cap
RUN_TIMEOUT = int(os.environ.get("SECSERVER_RUNTIMEOUT", "900"))  # s per RUN
RUN_OUT_CAP = 8000      # chars of RUN output fed back to the model
# Only attempt exploits for findings that smell memory/inputy.
CANDIDATE_RE = re.compile(
    r"overflow|out.?of.?bounds|oob|use.?after.?free|double.?free|"
    r"injection|memcpy|strcpy|index|underflow|race|uninitial....?|"
    r"buffer|null.?deref", re.I)

EXPLOIT_SYSTEM = (
    "You are an exploit engineer doing AUTHORIZED security testing of the "
    "CatchChallenger game server (the operator's own code). For ONE candidate "
    "finding you must produce a runnable proof and a verdict.\n"
    "\n"
    "TARGET KIT (already prepared for you):\n"
    "- Prebuilt gdb-able server binary (CATCHCHALLENGER_DB_INTERNAL_VARS, "
    "RelWithDebInfo): " + SERVER_BIN + "\n"
    "- A staged run dir you can copy into your own exploit dir: " + STAGED_RUN +
    " (contains server-properties.xml, a `datapack` symlink, and "
    "datapack-cache.bin). The server prints 'Waiting connection on port' once "
    "listening. Use a UNIQUE port (e.g. " + str(SERVER_PORT) + ") in your copy "
    "of server-properties.xml so you do not clash with other runs.\n"
    "- Protocol: server is ALLINONESERVER. Handshake magic (PROTOCOL_HEADER_"
    "LOGIN) = bytes 9c d6 49 8d 14. A query frame on the wire is "
    "[packetCode][queryNumber][data...]. Fixed packet sizes: 0xA0=5, "
    "0xA8=64, 0xA9=64. IMPORTANT: ProtocolParsingBase::parseData() reassembles "
    "a FIXED-size packet until ALL declared bytes arrive before dispatching, "
    "so short fixed-size packets are NOT delivered short. Real OOBs need a "
    "DYNAMIC-size (0xFE) handler that trusts an inner length and reads past "
    "`size`.\n"
    "\n"
    "SANDBOX: every RUN executes with the whole filesystem READ-ONLY except "
    "your own exploit dir (the only writable path). So COPY the staged binary "
    "and config into your exploit dir first (e.g. `cp -r " + STAGED_RUN +
    "/. ./run/` and `cp " + SERVER_BIN + " ./run/`), then run everything from "
    "there. Writes/deletes outside your dir will fail - do not attempt them.\n"
    "\n"
    "DELIVERABLE: write exploit.py (and any helper files) into your exploit "
    "dir. exploit.py must: start the server (over TCP) under gdb OR valgrind "
    "(NEVER both - project policy), connect as an unauthenticated TCP client, "
    "send crafted packets, and capture proof (gdb: crash / corrupted variable "
    "/ overwritten return address; valgrind: 'Invalid read/write'). Then RUN "
    "it. Decide the verdict from the ACTUAL output, not speculation.\n"
    "\n"
    "TOOL PROTOCOL - reply with EXACTLY ONE action per message, nothing else:\n"
    "  READ <repo-or-abs path>\n"
    "  GREP <symbol>\n"
    "  WRITE <relpath-in-your-exploit-dir>\n"
    "  ```\n  <full file content>\n  ```\n"
    "  RUN\n  ```\n  <shell commands, run in your exploit dir>\n  ```\n"
    "  VERDICT CONFIRMED <one-line reason>\n"
    "  VERDICT FALSEPOSITIVE <one-line reason>\n"
    "Use WRITE then RUN to build and execute your proof. Emit VERDICT "
    "CONFIRMED the moment you have a working crash/OOB. Emit VERDICT "
    "FALSEPOSITIVE as soon as you are SURE it cannot be exploited - you do "
    "NOT need to use the whole time budget, end as soon as you are certain - "
    "but you MUST explain the concrete LOGIC PATH: trace the untrusted input "
    "to the sink and name the exact guard / bounds-check / size validation / "
    "line(s) that make it safe (e.g. the parser-reassembly invariant above). "
    "A bare 'not exploitable' with no such reasoning will be rejected and you "
    "will be asked for the path. Keep files small and self-contained."
)

CODEBLOCK_RE = re.compile(r"```[a-zA-Z0-9_]*\n(.*?)```", re.DOTALL)


def slugify(rel, idx):
    base = re.sub(r"[^a-zA-Z0-9]+", "-", rel).strip("-").lower()
    return "%02d-%s" % (idx, base[:60])


def parse_findings(path):
    """Split findings.txt into (rel, text) blocks."""
    try:
        txt = open(path, errors="replace").read()
    except OSError:
        return []
    out = []
    parts = re.split(r"={60,}\nFINDINGS: (.+?)\n={60,}\n", txt)
    i = 1
    while i < len(parts):
        rel = parts[i].strip()
        body = parts[i + 1] if i + 1 < len(parts) else ""
        out.append((rel, body.strip()))
        i += 2
    return out


def first_codeblock(answer):
    m = CODEBLOCK_RE.search(answer)
    return m.group(1) if m else None


def parse_action(answer):
    """Return (kind, arg, block) for the model's single action, or None."""
    s = answer.strip()
    if not s:
        return None
    first = s.splitlines()[0].strip()
    up = first.upper()
    if up.startswith("READ "):
        return ("READ", first[5:].strip(), None)
    if up.startswith("GREP "):
        return ("GREP", first[5:].strip(), None)
    if up.startswith("WRITE "):
        return ("WRITE", first[6:].strip(), first_codeblock(answer))
    if up == "RUN" or up.startswith("RUN "):
        return ("RUN", "", first_codeblock(answer))
    # Capture the FULL (possibly multi-line) reason after the keyword - the
    # FALSEPOSITIVE logic-path explanation usually spans several lines.
    if up.startswith("VERDICT CONFIRMED"):
        return ("VERDICT", "CONFIRMED", s[len("VERDICT CONFIRMED"):].strip())
    if up.startswith("VERDICT FALSEPOSITIVE"):
        return ("VERDICT", "FALSEPOSITIVE", s[len("VERDICT FALSEPOSITIVE"):].strip())
    return None


def do_write(outdir, rel, content):
    if content is None:
        return "WRITE error: no ``` code block found in your message."
    dest = os.path.realpath(os.path.join(outdir, rel))
    if not dest.startswith(os.path.realpath(outdir) + os.sep):
        return "WRITE error: path escapes the exploit dir: %s" % rel
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    open(dest, "w").write(content)
    if dest.endswith((".py", ".sh")):
        os.chmod(dest, 0o755)
    return "WROTE %s (%d bytes)" % (rel, len(content))


BWRAP = shutil.which("bwrap")


def do_run(outdir, cmd, timeout=RUN_TIMEOUT):
    """Execute model-authored shell SANDBOXED with bubblewrap: the whole
    filesystem is read-only EXCEPT the exploit's own outdir, so a stray
    `rm -rf` (or any write/delete) cannot touch the repo, the security root,
    other tmpfs runs, or the prebuilt server. Server binary, datapack and
    system libs stay readable; network is shared so the TCP attack works.
    Fail CLOSED if bwrap is unavailable - never run the model's bash raw."""
    if not cmd:
        return "RUN error: no ``` code block with commands found."
    if not BWRAP:
        return ("RUN error: bubblewrap (bwrap) not found - refusing to run "
                "model-authored shell unsandboxed. Install bwrap.")
    wrapped = [
        BWRAP,
        "--ro-bind", "/", "/",          # everything read-only ...
        "--dev", "/dev",
        "--proc", "/proc",
        "--tmpfs", "/tmp",              # fresh throwaway writable /tmp
        "--bind", outdir, outdir,       # ... except this exploit's own dir
        "--die-with-parent",
        "--chdir", outdir,
        "--setenv", "TMPDIR", outdir,
        "bash", "-c", cmd,
    ]
    try:
        r = subprocess.run(wrapped, capture_output=True, text=True,
                           timeout=timeout)
        out = (r.stdout or "") + (r.stderr or "")
    except subprocess.TimeoutExpired as e:
        out = (e.stdout or "") + (e.stderr or "") + \
              "\n[RUN timed out after %ds]" % timeout
    except Exception as e:  # noqa
        out = "RUN error: %s" % e
    if len(out) > RUN_OUT_CAP:
        out = out[:RUN_OUT_CAP // 2] + "\n...[truncated]...\n" + out[-RUN_OUT_CAP // 2:]
    return out or "[no output]"


def exploit_one(rel, finding, idx, hard_budget, soft_budget):
    """Run the exploit agent for one candidate. Returns (verdict, reason, dir)."""
    outdir = os.path.join(OUTPUT_ROOT, slugify(rel, idx))
    os.makedirs(outdir, exist_ok=True)
    sys.stderr.write("\n%s [exploit %02d] %s -> %s\n" % (_ts(), idx, rel, outdir))

    user = (
        "Candidate finding to prove or refute:\n\nFILE: %s\n\nSCANNER NOTE:\n%s\n\n"
        "Your exploit dir is: %s\nWrite files with paths relative to it. "
        "RUN commands execute there. Begin." % (rel, finding, outdir))
    messages = [
        {"role": "system", "content": EXPLOIT_SYSTEM},
        {"role": "user", "content": user},
    ]
    transcript = []
    verdict, reason = "NOT-EXPLOITABLE", "budget exhausted without a confirmed proof"
    start = time.time()
    deadline = start + hard_budget                            # HARD cap (100%)
    soft_deadline = start + min(soft_budget, hard_budget)     # soft cap (~75%)
    ran_something = False   # did the model actually RUN a test yet?
    nudged = False          # already nudged once on a premature give-up?
    soft_warned = False     # already demanded the wrap-up verdict at soft cap?
    step = 0
    while step < EXPLOIT_MAX_STEPS:
        # HARD 1h cap: force-stop this exploit and move on to the next finding,
        # whatever the model is doing. No more chat()/RUN can be started.
        if time.time() >= deadline:
            sys.stderr.write("%s     [HARD STOP] %s budget reached -> dropping "
                             "exploit %02d (%s)\n"
                             % (_ts(), _human_dur(hard_budget), idx, rel))
            verdict, reason = ("NOT-EXPLOITABLE",
                               "hard %s budget reached without a confirmed proof"
                               % _human_dur(hard_budget))
            transcript.append("### HARD STOP (%s budget reached)"
                              % _human_dur(hard_budget))
            break
        step += 1
        answer = chat(messages, timeout=max(1, int(deadline - time.time())))
        remaining = int(deadline - time.time())
        transcript.append("### assistant (step %d, %ds left)\n%s"
                          % (step, remaining, answer))
        act = parse_action(answer)
        if act is None:
            messages.append({"role": "assistant", "content": answer})
            messages.append({"role": "user", "content":
                "Reply with exactly ONE action (READ/GREP/WRITE/RUN/VERDICT) "
                "as specified, nothing else."})
            continue
        kind, arg, block = act
        if kind == "VERDICT":
            # CONFIRMED ends immediately (exploit works).
            if arg == "CONFIRMED":
                verdict, reason = arg, block
                break
            # FALSEPOSITIVE = the model is SURE it can't be exploited -> END
            # EARLY, no matter how much time is left (budget is a CAP). But it
            # MUST explain the concrete logic path. Nudge ONCE if the
            # explanation is thin or it gave up before running any test.
            thin = len((block or "").strip()) < 40
            if not nudged and remaining > 30 and (thin or not ran_something):
                nudged = True
                transcript.append("### nudge (need logic-path justification)")
                messages.append({"role": "assistant", "content": answer})
                messages.append({"role": "user", "content":
                    "Before we stop: give the CONCRETE LOGIC PATH why it "
                    "cannot be exploited - trace the untrusted input to the "
                    "sink and name the exact guard/bounds-check/size-"
                    "validation and line(s) that make it safe (or RUN a probe "
                    "if you are not yet sure). Then repeat 'VERDICT "
                    "FALSEPOSITIVE <that explanation>'. We end as soon as you "
                    "are certain - you need not use the remaining time."})
                continue
            verdict, reason = "NOT-EXPLOITABLE", block
            break
        if kind == "READ":
            result = tool_read(arg)
        elif kind == "GREP":
            result = tool_grep(arg)
        elif kind == "WRITE":
            result = do_write(outdir, arg, block)
        elif kind == "RUN":
            # Never let a RUN outlive the hard cap: clamp its timeout to the
            # time left (min 1s) so a long probe cannot overrun the 1h budget.
            run_to = max(1, min(RUN_TIMEOUT, int(deadline - time.time())))
            sys.stderr.write("%s     [RUN] (step %d, %ds left, timeout %ds)\n"
                             % (_ts(), step, remaining, run_to))
            result = do_run(outdir, block, timeout=run_to)
            ran_something = True
        else:
            result = "unknown action"
        transcript.append("### tool result (%s %s)\n%s" % (kind, arg, result))
        # SOFT cap (~75%): stop starting new probes, ask once for the final
        # verdict. The hard cap at the top of the loop is the safety net if the
        # model keeps going. One forced final chat here still leaves headroom
        # before the 1h hard stop.
        if not soft_warned and time.time() >= soft_deadline:
            soft_warned = True
            sys.stderr.write("%s     [SOFT STOP] %s soft cap reached -> demanding "
                             "verdict (step %d)\n"
                             % (_ts(), _human_dur(time.time() - start), step))
            transcript.append("### SOFT STOP (45min soft cap, verdict demanded)")
            messages.append({"role": "assistant", "content": answer})
            messages.append({"role": "user", "content":
                "Soft time budget reached - do NOT start new probes. If you "
                "have a working CONFIRMED proof emit 'VERDICT CONFIRMED "
                "<reason>', otherwise emit 'VERDICT FALSEPOSITIVE <one-line "
                "reason it is not exploitable>' now."})
            final = chat(messages, timeout=max(1, int(deadline - time.time())))
            transcript.append("### assistant (final, soft budget)\n%s" % final)
            act = parse_action(final)
            if act and act[0] == "VERDICT":
                verdict = "CONFIRMED" if act[1] == "CONFIRMED" else "NOT-EXPLOITABLE"
                reason = act[2]
                break
            # Model didn't give a verdict; let the loop continue - the hard cap
            # will force-stop it.
            continue
        messages.append({"role": "assistant", "content": answer})
        messages.append({"role": "user", "content":
            "Result of `%s %s`:\n%s\n\n(~%ds of your budget left.) "
            "Next single action." % (kind, arg, result, remaining)})

    open(os.path.join(outdir, "transcript.md"), "w").write(
        "# Exploit transcript: %s\n\nVERDICT: %s - %s\n\n%s\n"
        % (rel, verdict, reason, "\n\n".join(transcript)))
    return verdict, reason, outdir


def run_exploit():
    """Phase 2. Returns 0/1."""
    if not os.path.isfile(SERVER_BIN):
        sys.stderr.write(
            "[!] prebuilt server not found: %s\n    Build it first:\n"
            "    cmake -S %s/server/cli -B %s/build "
            "-DCATCHCHALLENGER_DB_INTERNAL_VARS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo "
            "&& cmake --build %s/build -j\n"
            % (SERVER_BIN, REPO_ROOT, OUTPUT_ROOT, OUTPUT_ROOT))
        return 1
    findings = parse_findings(FINDINGS)
    if not findings:
        sys.stderr.write("[!] no findings in %s; run `%s scan > findings.txt` "
                         "first.\n" % (FINDINGS, sys.argv[0]))
        return 1

    candidates = [(rel, body) for rel, body in findings if CANDIDATE_RE.search(body)]
    _cap = os.environ.get("SECSERVER_MAX")
    if _cap:
        candidates = candidates[:int(_cap)]
    os.makedirs(OUTPUT_ROOT, exist_ok=True)
    hard_budget, soft_budget = resolve_budgets()
    sys.stderr.write("%s [INIT] %d findings, %d look exploitable; generating "
                     "exploits via %s (budget per exploit: hard %s / soft %s)\n"
                     % (_ts(), len(findings), len(candidates), MODEL_NAME,
                        _human_dur(hard_budget), _human_dur(soft_budget)))

    results = []
    idx = 0
    for rel, body in candidates:
        idx += 1
        try:
            verdict, reason, outdir = exploit_one(rel, body, idx,
                                                  hard_budget, soft_budget)
        except Exception as e:  # noqa - one bad exploit shouldn't abort the run
            sys.stderr.write("    [error] %s\n" % e)
            verdict, reason, outdir = "ERROR", str(e), "-"
        # Keep only CONFIRMED exploit folders; drop the rest - BUT first
        # preserve the logic-path explanation of WHY it is not exploitable in
        # a kept file, AND keep the full IA chat / thinking transcript as an
        # individual file (the rest of the folder is then removed).
        kept = os.path.basename(outdir)
        if verdict != "CONFIRMED" and outdir != "-" and os.path.isdir(outdir):
            with open(os.path.join(OUTPUT_ROOT, "not-exploitable.md"), "a") as fh:
                fh.write("## %s\n**%s.** logic path why it cannot be "
                         "exploited:\n\n%s\n\n---\n\n"
                         % (rel, verdict, reason or "(no explanation given)"))
            # Rescue the IA chat/thinking transcript before dropping the dir so
            # the full model conversation survives as its own file.
            transcripts_dir = os.path.join(OUTPUT_ROOT, "transcripts")
            os.makedirs(transcripts_dir, exist_ok=True)
            src = os.path.join(outdir, "transcript.md")
            saved = ""
            if os.path.isfile(src):
                dst = os.path.join(transcripts_dir,
                                   "%s.md" % os.path.basename(outdir))
                shutil.move(src, dst)
                saved = " (chat -> transcripts/%s)" % os.path.basename(dst)
            shutil.rmtree(outdir, ignore_errors=True)
            kept = "(dropped; why -> not-exploitable.md)"
            sys.stderr.write("    [drop] %s -> not exploitable; why kept in "
                             "not-exploitable.md%s\n" % (rel, saved))
        results.append((rel, verdict, reason, kept))
        # Notebook stays project-wide + short; the full path is in the file above.
        short = " ".join((reason or "").split())[:120]
        append_notes(rel, "<<<NOTES\n- exploit verdict: %s (%s) [%s]\nNOTES>>>"
                     % (verdict, short, kept))

    lines = ["# security-server.py exploit run\n",
             "Model: %s  |  candidates: %d\n" % (MODEL_NAME, len(candidates)),
             "\n| # | finding | verdict | reason | dir |",
             "|---|---|---|---|---|"]
    for i, (rel, verdict, reason, kept) in enumerate(results, 1):
        lines.append("| %d | %s | %s | %s | %s |"
                     % (i, rel, verdict, (reason or "")[:80], kept))
    open(os.path.join(OUTPUT_ROOT, "REPORT.md"), "w").write("\n".join(lines) + "\n")
    sys.stderr.write("%s [DONE] wrote %s/REPORT.md\n" % (_ts(), OUTPUT_ROOT))
    # To stderr (not stdout) so it never pollutes a findings.txt that stdout
    # may be redirected to in the default `all` pipeline.
    for rel, verdict, reason, _ in results:
        sys.stderr.write("%-14s %s  (%s)\n" % (verdict, rel, reason))
    return 0


# ===========================================================================
# CLI dispatch
# ===========================================================================
def main(argv):
    # No subcommand = full pipeline (scan -> exploit), matching:
    #   python3 server.py > findings.txt 2> progress.log
    # scan findings go to stdout (the user's redirect); the exploit phase then
    # reads them back from FINDINGS and emits only to stderr + REPORT.md, so
    # findings.txt stays clean.
    mode = argv[1] if len(argv) > 1 else "all"
    if mode in ("scan", "-s", "--scan"):
        return run_scan(sys.stdout)
    if mode in ("exploit", "-e", "--exploit"):
        return run_exploit()
    if mode in ("all", "-a", "--all"):
        os.makedirs(OUTPUT_ROOT, exist_ok=True)
        rc = run_scan(sys.stdout)      # -> findings.txt via the user's redirect
        sys.stdout.flush()
        if rc != 0:
            return rc
        if not os.path.isfile(FINDINGS):
            sys.stderr.write(
                "[!] %s not written - redirect stdout to it for the exploit "
                "phase: `%s > findings.txt 2> progress.log`\n"
                % (FINDINGS, argv[0]))
            return 1
        return run_exploit()
    sys.stderr.write(
        "usage: %s [scan|exploit|all]   (no arg = all)\n"
        "  scan     audit server/cli, print findings to stdout "
        "(redirect to findings.txt)\n"
        "  exploit  model-generate+run exploits for findings.txt under %s\n"
        "  all      scan (stdout->findings.txt) then exploit  [DEFAULT]\n"
        % (argv[0], OUTPUT_ROOT))
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
