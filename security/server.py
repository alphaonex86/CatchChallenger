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
# In-scope code reachable from the game TCP port, per EXAMPLE-BOUNTY-BUG-SERVER.md
# (§2 Scope + §4 attack-surface zones). The audit seeds from ALL of these, not
# just server/cli: otherwise the real per-packet handlers (ClientEvents/), the
# crafting/fight game logic, and the wire parser (ProtocolParsing*,
# ClientNetworkRead*) are never looked at - they live in server/base & general/,
# not under server/cli, so #include-following alone misses them.
SCAN_SEED_DIRS = tuple(os.path.join(REPO_ROOT, p) for p in (
    "server/cli",       # the standalone game server binary (primary target)
    "server/base",      # ClientNetworkRead*, ClientEvents/, ClientLoad/, handlers
    "server/crafting",  # recipe / plant handlers
    "server/fight",     # server-side battle handlers
    "general/base",     # ProtocolParsing* framing + shared engine/datapack parse
    "general/fight",    # shared turn-based fight engine
))
MODEL_NAME = "opencode-fast"
OLLAMA_API = "http://127.0.0.1:11434/api/generate"
OLLAMA_CHAT = "http://127.0.0.1:11434/api/chat"

HERE = os.path.dirname(os.path.realpath(__file__))


def _ts():
    """Wall-clock stamp for progress lines: H:m D/M/Y (e.g. 18:42 23/05/2026)."""
    return time.strftime("%H:%M %d/%m/%Y")


class _Tee:
    """Write to multiple streams at once (used to tee stdout + findings file)."""
    def __init__(self, *streams):
        self._streams = streams

    def write(self, s):
        for st in self._streams:
            st.write(s)

    def flush(self):
        for st in self._streams:
            st.flush()


def _human_dur(seconds):
    """Elapsed seconds as MINmSECs (e.g. 99min09s)."""
    seconds = int(seconds)
    return "%dmin%02ds" % (seconds // 60, seconds % 60)

# All server-related generated artifacts live under this temp subfolder.
OUTPUT_ROOT = "/mnt/data/perso/tmpfs/security/server"

# Official bug-bounty scope/methodology doc. The IA (auditor + exploit engineer)
# MUST read it: it defines the in-scope code, the rewarded vulnerability classes,
# and the attack-surface zones to probe. We append it to BOTH system prompts so
# every turn carries the authoritative scope (TCP-reachable defects only).
BOUNTY_DOC = os.path.join(HERE, "EXAMPLE-BOUNTY-BUG-SERVER.md")


def read_bounty():
    """The bounty scope doc wrapped as an authoritative reference block, or ''
    if absent/empty. Best-effort - a missing doc is never fatal, the audit just
    runs without it (a startup warning is emitted by the phase entry points)."""
    try:
        txt = open(BOUNTY_DOC, "r", errors="replace").read().strip()
    except OSError:
        return ""
    if not txt:
        return ""
    return ("\n\n=== AUTHORITATIVE BUG-BOUNTY SCOPE (EXAMPLE-BOUNTY-BUG-SERVER.md) "
            "- this defines what is IN SCOPE, what is REWARDED, and WHERE TO LOOK. "
            "Obey it; it overrides any conflicting instinct. Only a defect reachable "
            "from a remote game-client TCP socket counts. ===\n%s\n"
            "=== END BUG-BOUNTY SCOPE ===" % txt)


def check_tooling():
    """Verify the IA has the context + tools the bounty doc's methodology needs,
    and log any gap. Best-effort (warn only): the phase code fails closed where a
    tool is truly mandatory (LiveServer/do_run refuse to run unsandboxed without
    bwrap). The doc requires: the scope doc itself (so the IA reads it), a
    sandbox (bwrap), a memory-safety prover (valgrind, §3.1), crash catch + live
    state inspection (gdb, §3.2/3.4-7), a C/C++ toolchain to compile the
    reproducer ELF (§6.2), and cmake to build the target (§1)."""
    if os.path.isfile(BOUNTY_DOC):
        sys.stderr.write("%s [tooling] bounty scope doc loaded (%s); IA audits to "
                         "its scope + vuln classes\n"
                         % (_ts(), os.path.basename(BOUNTY_DOC)))
    else:
        sys.stderr.write("%s [tooling] WARNING: bounty scope doc missing (%s) - "
                         "auditing WITHOUT it; scope/vuln-class guidance reduced\n"
                         % (_ts(), BOUNTY_DOC))
    needed = (
        ("bwrap", "exploit/server sandbox (MANDATORY - runs fail closed without it)"),
        ("valgrind", "Memcheck OOB/UAF/bad-free prover (MODE valgrind, reward §3.1)"),
        ("gdb", "crash catch + read-only live state inspection (MODE gdb, §3.2-7)"),
        ("gcc", "compile the C reproducer ELF (§6.2)"),
        ("g++", "compile C++ reproducers"),
        ("cmake", "build the target server (§1)"),
    )
    for tool, why in needed:
        if shutil.which(tool) is None:
            sys.stderr.write("%s [tooling] WARNING: '%s' not found - %s\n"
                             % (_ts(), tool, why))

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
    "SCOPE - THE ONLY ACCEPTED ATTACK IS A REMOTE ATTACK OVER THE TCP "
    "SOCKET/SERVER. The remote attack surface is bytes arriving from an "
    "untrusted client over a TCP connection (EventLoopClient::read() -> "
    "ProtocolParsing* / ClientNetworkRead*). A finding is valid ONLY if the "
    "attacker input reaches the sink over that TCP path.\n"
    "OUT OF SCOPE - do NOT report anything reachable only through a LOCAL / "
    "OPERATOR-controlled channel; those are trusted, not remote:\n"
    "- Unix domain sockets (AF_UNIX / sockaddr_un): UnixSocketServer, "
    "UnixSocketClient, UnixSocketClientFinal - local IPC / admin control "
    "channel, not the network.\n"
    "- The stdin admin console (EventLoopStdin) or any interactive/local "
    "console input.\n"
    "- Config / deploy inputs set by the operator: server-properties.xml, "
    "datapack files, command-line arguments, environment variables, on-disk "
    "file paths.\n"
    "- ALL XML PARSING, without exception. EVERY XML input in this server "
    "(TinyXML2 / TinyXMLSettings, the settings/server-properties.xml, and "
    "every datapack XML loader - items, monsters, skills, maps/tmx, quests, "
    "crafting, plants, shops, profiles, etc.) is loaded from the OPERATOR'S "
    "DISK at startup. There is NO remote XML: the client never sends XML over "
    "TCP. So ANY bug in XML parsing or in consuming parsed-XML values (bad "
    "length, missing/duplicate attribute, malformed number, OOB on a datapack "
    "id table, etc.) is OUT OF SCOPE - it needs a malicious datapack/config the "
    "attacker does not control. Do NOT report ANY XML-sourced finding. The only "
    "in-scope nuance: the SAME id table, once INDEXED by a client-supplied id "
    "arriving over TCP, is in scope - but that is an index/bounds bug in the "
    "HANDLER, never in the XML loader.\n"
    "- The test/automation channel (QLocalServer) and any other local-only "
    "IPC.\n"
    "Even a REAL memory-corruption or injection bug (overflow, OOB, "
    "command/SQL injection) on one of these LOCAL channels is OUT OF SCOPE "
    "here - output 'No issues identified' for it UNLESS the very same sink is "
    "ALSO reachable from the remote TCP path. When unsure whether an input is "
    "remote-TCP or local, treat it as local (out of scope).\n"
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
    "MANDATORY OUTPUT FORMAT - your final answer MUST be one of exactly two "
    "forms, nothing else:\n"
    "  (A) If you found at least one exploitable vulnerability:\n"
    "      LINE <n>: <attacker-controlled input> -> <concrete consequence>\n"
    "      One line per finding. No prose, no summary, no preamble.\n"
    "  (B) If you found nothing exploitable:\n"
    "      No issues identified\n"
    "      That exact phrase, alone, nothing before or after it.\n"
    "\n"
    "FORBIDDEN in your final answer: narrative descriptions of the code, "
    "generic summaries ('the code handles...', 'this class is part of...'), "
    "opinions ('could potentially', 'might be'), requests for clarification "
    "('be more specific', 'without more context'), design observations, or "
    "any text that is not a LINE finding or the exact phrase above. When in "
    "doubt, output 'No issues identified'.\n"
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
# The IA reads the bug-bounty scope on every audit turn (appended once at load).
SYSTEM_PROMPT = SYSTEM_PROMPT + read_bounty()

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
# Floor is 16K: smaller contexts truncate the audited file + related files and
# the model loses the bug. Ceiling is 1M so a large file + its related files
# can fit when the model/host support it. ensure_context() checks at runtime
# what the model can actually honour and warns/caps accordingly.
MIN_CTX = 16384
MAX_CTX = 1024 * 1024
# Model's actual trained context, discovered at runtime by ensure_context();
# fit_ctx caps to it so we never ask Ollama for more than the model can honour.
MODEL_CTX = None

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
    """Return an ordered list of repo files to audit: every source file in the
    in-scope dirs (SCAN_SEED_DIRS - the bounty's TCP-reachable code) plus,
    transitively, every local header/source they #include that still lives under
    REPO_ROOT (vendored libs excluded)."""
    repo_root = os.path.realpath(REPO_ROOT)
    seen = set()
    ordered = []
    pending = []

    # Seed with every in-scope dir's own sources (recursive) so the per-packet
    # handlers + wire parser are audited, not just the cli plumbing.
    for seed in SCAN_SEED_DIRS:
        if os.path.isdir(seed):
            for root, _dirs, names in os.walk(seed):
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
    # Ceiling: MAX_CTX, but never beyond what the model actually supports.
    ceiling = MAX_CTX if MODEL_CTX is None else min(MAX_CTX, MODEL_CTX)
    if tokens > ceiling:
        tokens = ceiling
    if tokens < MIN_CTX:
        tokens = MIN_CTX
    return tokens


def ensure_context():
    """Verify at runtime that the model can actually honour a >=16K context.
    Asks Ollama (/api/show) for the model's trained context_length and warns if
    it is below MIN_CTX (a too-small context silently truncates our prompt, so
    the audit/exploit reasoning degrades). Best-effort: never fatal - if Ollama
    is unreachable or the field is absent we just note it and continue. The
    discovered length is stored in MODEL_CTX so fit_ctx caps num_ctx to it."""
    global MODEL_CTX
    try:
        req = urllib.request.Request(
            OLLAMA_API.replace("/api/generate", "/api/show"),
            data=json.dumps({"model": MODEL_NAME}).encode("utf-8"),
            headers={"Content-Type": "application/json"},
        )
        with urllib.request.urlopen(req, timeout=30) as resp:
            info = json.loads(resp.read().decode("utf-8", "replace"))
    except (urllib.error.URLError, OSError, ValueError) as exc:
        sys.stderr.write("%s [ctx] could not query model context length (%s); "
                         "requesting num_ctx in [%d,%d] anyway\n"
                         % (_ts(), exc, MIN_CTX, MAX_CTX))
        return None
    # model_info keys are arch-prefixed, e.g. "qwen2.context_length".
    ctx = None
    for key, val in (info.get("model_info") or {}).items():
        if key.endswith(".context_length"):
            try:
                ctx = int(val)
            except (TypeError, ValueError):
                ctx = None
            break
    if ctx is None:
        sys.stderr.write("%s [ctx] model %s context length unknown; requesting "
                         "num_ctx in [%d,%d]\n"
                         % (_ts(), MODEL_NAME, MIN_CTX, MAX_CTX))
    elif ctx < MIN_CTX:
        sys.stderr.write("%s [ctx] WARNING: model %s max context %d < required "
                         "%d - prompts will be TRUNCATED. Use a model with a "
                         "bigger context.\n" % (_ts(), MODEL_NAME, ctx, MIN_CTX))
    else:
        sys.stderr.write("%s [ctx] model %s context %d >= %d OK (num_ctx capped "
                         "to min(%d, %d))\n"
                         % (_ts(), MODEL_NAME, ctx, MIN_CTX, MAX_CTX, ctx))
    MODEL_CTX = ctx
    return ctx


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
            sock_to = 1200
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
            sys.stderr.write("    [chat retry %d/5 after %ds %s]\n" % (attempt, sock_to, exc))
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


def _finalize_answer(rel, answer):
    """Persist + strip the NOTES block and fix repo paths in a model answer.
    Shared by analyze() and its degenerate-output retry."""
    for block in NOTE_RE.findall(answer):
        append_notes(rel, block)
    answer = NOTE_RE.sub("", answer).strip()
    return fix_paths(answer)


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

    answer = _finalize_answer(rel, answer)
    if looks_degenerate(answer):
        # A repetition loop usually means the model lost the thread, not that the
        # file is clean. Give it ONE corrective retry for a terse final answer
        # before giving up - feed back only a truncated copy of the junk so we
        # don't re-prime the same loop. Salvages files we used to discard wholesale.
        sys.stderr.write("    [degenerate output: repetition loop -> retry once terse]\n")
        messages.append({"role": "assistant", "content": answer[:2000]})
        messages.append({"role": "user", "content":
            "Your last reply was a repetition loop / junk. STOP repeating and do "
            "NOT call tools. Output your final answer ONCE in the mandatory "
            "format: either one 'LINE <n>: <input> -> <consequence>' per real "
            "finding for the MAIN FILE, or the exact phrase 'No issues "
            "identified'. Nothing else."})
        answer = _finalize_answer(rel, chat(messages))
        if looks_degenerate(answer):
            sys.stderr.write("    [discarded degenerate output after retry]\n")
            return ""
    # Validate cited line numbers against the actual file length. A finding line
    # citing a LINE past the end of the MAIN FILE was reasoning about an included
    # file, not this one - drop ONLY that line and keep the valid findings,
    # instead of discarding the whole audit over one bad citation.
    nlines = len(content.splitlines())
    kept, dropped = [], []
    for ln in answer.splitlines():
        cited = [int(m.group(1)) for m in re.finditer(r'\bLINE\s+(\d+)\b', ln, re.I)]
        if cited and max(cited) > nlines:
            dropped.append(max(cited))
        else:
            kept.append(ln)
    if dropped:
        sys.stderr.write(
            "    [dropped %d out-of-range finding line(s): %s cited but file has "
            "only %d lines; kept the rest]\n"
            % (len(dropped), ", ".join(str(n) for n in dropped), nlines))
    return "\n".join(kept).strip()


# Repo-relative source paths the model might cite inside finding text.
# Matches things like "server/base/Foo.cpp" or "general/fight/Bar.hpp".
_REPOPATH_RE = re.compile(
    r'\b((?:server|general|client|tools)/[a-zA-Z0-9_/.-]+\.(?:cpp|hpp|h|cc|cxx))\b'
)


def fix_paths(text):
    """Replace every repo-relative path in `text` that does not exist on disk
    with the real path found via find_by_basename().  When the model writes
    'server/base/ProtocolParsingInput.cpp' but the real file is
    'general/base/ProtocolParsingInput.cpp', the corrected path is substituted
    so the exploit agent (and the human reader) can locate the file."""
    def _sub(m):
        p = m.group(1)
        if os.path.isfile(os.path.join(REPO_ROOT, p)):
            return p
        real = find_by_basename(os.path.basename(p))
        if real and os.path.isfile(real):
            corrected = os.path.relpath(real, REPO_ROOT)
            if corrected != p:
                sys.stderr.write("    [path fix] %s -> %s\n" % (p, corrected))
                return corrected
        return p
    return _REPOPATH_RE.sub(_sub, text)


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


# Phrases that indicate a vague/narrative non-finding response from the model.
# These are NOT security findings; they trigger false exploit runs.
_VAGUE_RE = re.compile(
    r"be more (?:specific|clear)|without (?:specific|more) details?|"
    r"without more context|hard(?:er)? to (?:say|provide|give|determine)|"
    r"it(?:'s| is) (?:difficult|hard|unclear)|could potentially|might be "
    r"vulnerable|seems? (?:like|to)|the code (?:you|handles|appears|is a)|"
    r"this (?:class|code|file|snippet) (?:is|appears|seems|handles)|"
    r"asking about potential|without (?:specific|more) (?:info|details|"
    r"context)|no concrete|as a (?:starting|general)",
    re.I,
)

# A real finding produced by the mandatory format must cite a line number.
_LINE_NUM_RE = re.compile(r"\bLINE\s+\d+\b|\bline[s]?\s+\d+\b|\b\d+:\d+\b", re.I)


def has_findings(answer):
    """True only when the model produced at least one concrete LINE finding.

    The mandatory output format is either 'No issues identified' (clean) or
    one or more 'LINE <n>: ...' entries. Anything else - vague narratives,
    generic code descriptions, hedged 'could potentially' text - is treated
    as no-finding so it never feeds the exploit phase."""
    if not answer:
        return False
    normalized = re.sub(r"[^a-z ]", " ", answer.lower())
    normalized = " ".join(normalized.split())
    # Explicit clean-file answer.
    if "no issues identified" in normalized:
        return False
    # Vague narrative - model did not follow the mandatory format.
    if _VAGUE_RE.search(answer):
        return False
    # A real finding must cite at least one line number (the format requires it).
    if not _LINE_NUM_RE.search(answer):
        return False
    return True


def run_scan(out=sys.stdout):
    """Phase 1. Findings are written to `out` AND always persisted to FINDINGS
    on disk so the exploit phase can read them without a stdout redirect."""
    if not os.path.isdir(TARGET_DIR):
        sys.stderr.write("[ERROR] Target directory not found: %s\n" % TARGET_DIR)
        return 1

    # Tee every print() into FINDINGS so `python3 server.py scan` (no redirect)
    # still produces the file the exploit phase needs.
    findings_fh = open(FINDINGS, "w")
    out = _Tee(out, findings_fh)
    try:
        return _run_scan_inner(out)
    finally:
        findings_fh.close()


def _run_scan_inner(out):
    check_tooling()
    ensure_context()
    files = collect_files()
    scan_start = time.time()
    sys.stderr.write(
        "%s [INIT] %d files to audit (in-scope dirs + repo-local includes, vendor "
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
        os.makedirs(os.path.dirname(SCAN_TIME_FILE), exist_ok=True)
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
BUILD_DIR = os.path.join(OUTPUT_ROOT, "build")
SERVER_BIN = os.path.join(BUILD_DIR, "catchchallenger-server-cli")
STAGED_RUN = os.path.join(OUTPUT_ROOT, "run")
SERVER_PORT = 61997
# The live gdb-driven inspection session (LiveServer) runs its OWN server copy on
# a distinct port so it never clashes with a server the model started via RUN.
GDB_PORT = SERVER_PORT + 1

# Datapack the staged server boots against. We symlink a read-only STAGED copy
# into the run dir (never the source tree - datapack-source-safety rule), exactly
# like test/_protoharness.py.
DATAPACK_SRC = "/home/user/Desktop/CatchChallenger/CatchChallenger-datapack"
TEST_DIR = os.path.join(REPO_ROOT, "test")
# Minimal server-properties.xml matching the bounty doc's run recipe (§1):
# automatic_account_creation on (so the exploit can reach a selected character),
# compression none (packets readable on the wire), maincode "test". The server
# fills every other field with its defaults. Port is seeded as SERVER_PORT;
# LiveServer.start() rewrites it to GDB_PORT for its private instance.
STAGED_PROPERTIES = """<?xml version="1.0"?>
<configuration>
    <server-port value="{port}"/>
    <server-ip value="127.0.0.1"/>
    <automatic_account_creation value="true"/>
    <compression value="none"/>
    <max-players value="200"/>
    <content>
        <mainDatapackCode value="test"/>
        <subDatapackCode value=""/>
    </content>
    <master>
        <external-server-port value="{port}"/>
    </master>
</configuration>
"""


def resolve_run_under():
    """STARTING supervisor for the target server. The exploit model OWNS this
    choice at runtime - it switches with the MODE action (see EXPLOIT_SYSTEM) -
    so this only sets which tool the FIRST instance boots under:
      - gdb      : catch fatal signals (SIGSEGV/SIGABRT/...) + read-only live
                   inspection of game state. Misses memory errors that don't
                   actually crash.
      - valgrind : run under Memcheck. ANY memory error it reports (invalid
                   read/write, OOB, bad free, fatal signal) = exploit SUCCESS,
                   even a SILENT out-of-range access that never segfaults. Live
                   gdb inspection is off in this mode (valgrind owns the process).
    Default gdb; SECSERVER_MODE / --mode= only nudge the starting point."""
    m = os.environ.get("SECSERVER_MODE", "gdb").strip().lower()
    return "valgrind" if m in ("valgrind", "vg", "memcheck") else "gdb"


# Starting supervisor (the exploit model switches at will via the MODE action).
RUN_UNDER = resolve_run_under()
VALGRIND = shutil.which("valgrind")

# Per-exploit WALL-CLOCK budget. This is a CAP, not a floor: the model may
# keep rewriting/retrying up to this long, but it ends EARLY on a CONFIRMED
# proof OR once it is sure it cannot be exploited (VERDICT FALSEPOSITIVE).
# Budget reached without a confirmed crash/OOB -> not-exploitable.
# Either way a non-confirmed finding's exploit folder is dropped.
# Per-exploit budgets are DERIVED from the measured total scan time (written to
# SCAN_TIME_FILE by the scan phase): hard cap = total scan time (100%), soft cap
# = 75% of it. Env vars override either. Fallback if no scan time is on disk
# (e.g. running `exploit` standalone before any scan): 1h hard / 75% soft.
SCAN_TIME_FILE = os.path.join(os.path.dirname(OUTPUT_ROOT), "scan-seconds.txt")
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
RUN_TIMEOUT = int(os.environ.get("SECSERVER_RUNTIMEOUT", "900"))  # s wall per RUN
RUN_OUT_CAP = 8000      # chars of RUN output fed back to the model
# Hard caps the compiled exploit ELF runs under (the sandbox enforces these).
SANDBOX_CPU_SECONDS = int(os.environ.get("SECSERVER_CPU", "10"))   # RLIMIT_CPU
SANDBOX_MEM_BYTES = int(os.environ.get("SECSERVER_MEM", str(128 * 1024 * 1024)))
EXPLOIT_BIN_NAME = "exploit"   # fixed path of the compiled ELF (in the chroot: /exploit)
# Attempt an exploit for any finding smelling like a REWARDED bounty class
# (EXAMPLE-BOUNTY-BUG-SERVER.md §3). The exploit engine can PROVE every one of
# them: memory errors + crashes under gdb/valgrind (case a), hang/CPU under the
# event-loop ping (case b), and economy/ownership/world-rule/state-corruption via
# gdb read-only state inspection (case c). So all classes flow into the prove
# loop, not just memory-safety ones.
CANDIDATE_RE = re.compile(
    # §3.1 memory safety
    r"overflow|out.?of.?bounds|oob|use.?after.?free|double.?free|"
    r"injection|memcpy|strcpy|index|underflow|race|uninitial....?|"
    r"buffer|null.?deref|"
    # §3.2 crash / unhandled exception
    r"crash|abort|assert|segfault|sigsegv|sigabrt|throw|exception|terminate|"
    # §3.3 hang / infinite loop / CPU amplification
    r"hang|infinite.?loop|deadlock|spin|amplif|\bcpu\b|\bdos\b|"
    # §3.4 economy integrity
    r"duplicat|\bdupe?\b|econom|price|quantity|\bcount\b|negative|"
    r"free (?:item|cash|gold|money|monster|xp)|"
    # §3.5 ownership / authorization bypass
    r"ownership|authoriz|authentication|\bauth\b|bypass|impersonat|"
    # §3.6 world-rule bypass
    r"collision|ledge|teleport|walkthrough|world.?rule|"
    # §3.7 persisted-state corruption
    r"corrupt|persist|cross.?player|phantom", re.I)

EXPLOIT_SYSTEM = (
    "You are an exploit engineer doing AUTHORIZED security testing of the "
    "CatchChallenger game server (the operator's own code). For ONE candidate "
    "finding you must produce a runnable proof and a verdict.\n"
    "\n"
    "TARGET (already running for you):\n"
    "- The HARNESS runs the server under a SUPERVISOR on 127.0.0.1:" +
    str(GDB_PORT) + " and OWNS its lifecycle - you do NOT start/stop/continue/"
    "step it. Your job is to write a client that connects to it and breaks it. "
    "You MAY ask for a FRESH instance with RESTARTSERVER (e.g. to reset "
    "game-state to a clean baseline between probes); it reboots on the same "
    "port.\n"
    "- YOU CHOOSE THE SUPERVISOR with the MODE action - pick whichever exposes "
    "the bug you are chasing, and switch any time:\n"
    "    MODE gdb       crashes (SIGSEGV/SIGABRT/...) are caught AND you get "
    "read-only live inspection of game state (the GDB action). Best for proving "
    "an UNINTENDED state change (case c) or grabbing a crash backtrace.\n"
    "    MODE valgrind  the server runs under Memcheck: ANY memory error it "
    "reports - 'Invalid read/write of size N', out-of-bounds, invalid/mismatched "
    "free, or a fatal signal - is an AUTOMATIC SUCCESS, even a SILENT "
    "out-of-range access that NEVER crashes. The GDB inspection action is OFF in "
    "this mode (valgrind owns the process). Best for hunting OOB/use-after-free/"
    "bad-free reads & writes. Switching MODE reboots the server fresh.\n"
    "- Protocol: server is ALLINONESERVER. Handshake magic (PROTOCOL_HEADER_"
    "LOGIN) = bytes 9c d6 49 8d 14. A query frame on the wire is "
    "[packetCode][queryNumber][data...]. Fixed packet sizes: 0xA0=5, "
    "0xA8=64, 0xA9=64. IMPORTANT: ProtocolParsingBase::parseData() reassembles "
    "a FIXED-size packet until ALL declared bytes arrive before dispatching, "
    "so short fixed-size packets are NOT delivered short. Real OOBs need a "
    "DYNAMIC-size (0xFE) handler that trusts an inner length and reads past "
    "`size`.\n"
    "\n"
    "HOW YOU ATTACK - write a C/C++ TCP client, then RUN it:\n"
    "- WRITE one or more .c/.cpp files into your exploit dir. RUN compiles ALL "
    "of them together into a single STATIC ELF at the fixed path /" +
    EXPLOIT_BIN_NAME + " and executes it. Your code must open a TCP socket to "
    "127.0.0.1:" + str(GDB_PORT) + ", do the handshake, and send the crafted "
    "packets that trigger the bug.\n"
    "- The ELF runs in a HARD SANDBOX: chrooted to your exploit dir (read-only - "
    "it cannot see or touch any other path, and cannot write files), in its own "
    "pid namespace, CPU-time capped (" + str(SANDBOX_CPU_SECONDS) + "s) and "
    "memory capped (" + str(SANDBOX_MEM_BYTES // (1024 * 1024)) + " MB). A "
    "seccomp allowlist KILLS any syscall other than what a TCP client needs "
    "(socket/connect/send/recv/... + memory + basic IO). So: NO opening files, "
    "NO fork/exec, NO ptrace - just connect to the server and talk. A SIGSYS "
    "kill in the output means your code used a forbidden syscall; rewrite it to "
    "only do networking.\n"
    "- After each RUN the harness checks the supervised server: if your packets "
    "CRASHED it (or, in MODE valgrind, tripped a memcheck memory error) that is "
    "reported back (with the backtrace / memcheck report) and the exploit is an "
    "automatic SUCCESS.\n"
    "\n"
    "INSPECTION - GDB (read-only, MODE gdb ONLY): pauses the server, runs your "
    "READ-ONLY gdb "
    "commands (print/x/info/bt/ptype/break/watch/...), then resumes it. You may "
    "read ANY variable/memory/register but you CANNOT write it, call functions, "
    "step/continue it, or run a shell ('(' and '=' are blocked) - so what you "
    "observe is the server's REAL state and cannot be forged into a fake win. "
    "Use it to confirm an UNINTENDED state change: RUN your exploit, then GDB "
    "read the relevant player/item/monster fields and compare to what the rules "
    "allow.\n"
    "\n"
    "EACH TURN, do exactly ONE of: take an ACTION (write/run/inspect), REQUEST "
    "something you need, or give the final VERDICT - SUCCESS or FAIL.\n"
    "\n"
    "TOOL PROTOCOL - reply with EXACTLY ONE action per message, nothing else:\n"
    "  READ <repo-or-abs path>\n"
    "  GREP <symbol>\n"
    "  WRITE <relpath-in-your-exploit-dir>\n"
    "  ```\n  <full file content (e.g. exploit.c)>\n  ```\n"
    "  RUN                        (compile your C/C++ -> /" + EXPLOIT_BIN_NAME +
    " and run it sandboxed)\n"
    "  RESTARTSERVER              (reboot the supervised server fresh on the "
    "same port - resets game-state)\n"
    "  MODE gdb | MODE valgrind   (choose/switch the supervisor; reboots fresh)\n"
    "  GDB\n  ```\n  <read-only gdb commands, one per line>\n  ```"
    "  (gdb mode only)\n"
    "  VERDICT CONFIRMED <one-line reason>     (= exploit SUCCESS)\n"
    "  VERDICT FALSEPOSITIVE <one-line reason> (= exploit FAIL)\n"
    "\n"
    "WHAT COUNTS AS SUCCESS (VERDICT CONFIRMED):\n"
    "  (a) the server CRASHES under your exploit (gdb catches SIGSEGV/SIGABRT/"
    "...), OR - in MODE valgrind - valgrind reports ANY memory error (invalid "
    "read/write, OOB, bad free) even with no crash;\n"
    "  (b) the server HANGS - an infinite loop or an event loop that stops "
    "responding (a DoS). After each RUN the harness pings the event loop (TCP "
    "connect + handshake) and samples its CPU; a wedged/spinning loop is "
    "detected;\n"
    "  (c) any UNINTENDED game-state result you DEMONSTRATE via GDB read-only "
    "inspection (MODE gdb) - more of an item than the rules allow, an automatic "
    "battle win, a NEW monster outside the legitimate catch/warehouse path, a "
    "corrupted/overwritten value, an auth/ownership bypass.\n"
    "The harness AUTO-CONFIRMS (a) and (b) the moment it sees them; for (c) show "
    "it concretely (the gdb-observed memory before vs after your RUN). "
    "Speculation is not SUCCESS. TIP: a memory-corruption finding is usually "
    "fastest to prove under MODE valgrind (it flags the bad access directly); "
    "switch to MODE gdb when you need to read game state for case (c).\n"
    "WHAT COUNTS AS FAIL (VERDICT FALSEPOSITIVE): you are SURE it cannot be "
    "exploited. End as soon as you are certain - you need NOT use the whole "
    "budget - but you MUST give the concrete LOGIC PATH: trace the untrusted "
    "input to the sink and name the exact guard / bounds-check / size "
    "validation / line(s) that make it safe (e.g. the parser-reassembly "
    "invariant above). A bare 'not exploitable' with no such reasoning will be "
    "rejected and you will be asked for the path. Keep files small and "
    "self-contained."
)
# The exploit engineer reads the same bug-bounty scope: it spells out the
# rewarded vuln classes (§3) and the attack-surface zones/packets (§4), which
# tell it concretely what an "unintended game-state" win (case c) looks like
# (economy dup, ownership bypass, world-rule bypass) and where to aim.
EXPLOIT_SYSTEM = EXPLOIT_SYSTEM + read_bounty()

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
        return ("RUN", "", None)
    # Ask the harness for a fresh server-under-gdb (reset game-state / recover a
    # wedge). The harness still owns the lifecycle; this is the one exception.
    if up == "RESTARTSERVER" or up.startswith("RESTARTSERVER "):
        return ("RESTARTSERVER", "", None)
    # Pick which supervisor the target runs under (gdb | valgrind). The model
    # chooses; the harness reboots the server under that tool on the same port.
    if up.startswith("MODE "):
        return ("MODE", first[5:].strip(), None)
    # Read-only inspection of the harness-managed server-under-gdb. The gdb
    # instance lifecycle is the harness's, not the model's.
    if up == "GDB" or up.startswith("GDB "):
        return ("GDB", "", first_codeblock(answer))
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


# ---------------------------------------------------------------------------
# Compile + sandboxed execution of the model's C/C++ exploit ELF
# ---------------------------------------------------------------------------
# Source files (top level of the exploit dir) compiled into the ELF.
EXPLOIT_SRC_EXT = (".c", ".cc", ".cpp", ".cxx")

# Syscalls the sandboxed exploit ELF may make: just what a STATIC client needs
# to start up, manage its own memory, do basic I/O, and CONNECT TO THE SERVER
# OVER TCP. Anything else (open/openat a file, fork/clone a process, ptrace,
# mount, ...) is NOT listed and is KILLED by the seccomp filter. x86_64 numbers.
SECCOMP_ALLOW_X86_64 = (
    0, 1, 3, 5, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 22, 23,
    24, 25, 28, 32, 33, 35, 39, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
    52, 53, 54, 55, 59, 60, 61, 63, 72, 79, 96, 97, 102, 104, 107, 108, 131,
    138, 158, 186, 202, 204, 218, 219, 228, 230, 231, 232, 233, 262, 267, 270,
    271, 273, 281, 288, 290, 291, 292, 293, 302, 318, 324, 332, 334,
)
# 267=readlinkat, 61=wait4 and the other non-obvious entries are needed by
# glibc's static startup/exit (verified empirically). open/openat are NOT in the
# list on purpose: the exploit cannot open any file (a SIGSYS kill on open() is a
# clear "you used a forbidden syscall" signal). It can only do networking + its
# own memory + basic IO on the socket/stdio it already holds.


def _seccomp_filter_bytes():
    """Build a classic-BPF seccomp program (array of struct sock_filter) that
    ALLOWS only SECCOMP_ALLOW_X86_64 and KILLS the process on anything else.
    Byte layout is what `bwrap --seccomp FD` expects."""
    import struct
    AUDIT_ARCH_X86_64 = 0xC000003E
    KILL = 0x80000000          # SECCOMP_RET_KILL_PROCESS
    ALLOW = 0x7FFF0000         # SECCOMP_RET_ALLOW
    LD_W_ABS = 0x20            # BPF_LD|BPF_W|BPF_ABS
    JEQ_K = 0x15               # BPF_JMP|BPF_JEQ|BPF_K
    RET_K = 0x06               # BPF_RET|BPF_K
    allow = SECCOMP_ALLOW_X86_64
    n = len(allow)
    kill_idx = 3 + n           # index of RET KILL
    allow_idx = 4 + n          # index of RET ALLOW
    ins = [
        (LD_W_ABS, 0, 0, 4),                            # 0: A = arch
        (JEQ_K, 0, kill_idx - 2, AUDIT_ARCH_X86_64),    # 1: wrong arch -> kill
        (LD_W_ABS, 0, 0, 0),                            # 2: A = syscall nr
    ]
    for i, nr in enumerate(allow):                      # 3..: match -> ALLOW
        j = 3 + i
        ins.append((JEQ_K, allow_idx - j - 1, 0, nr))
    ins.append((RET_K, 0, 0, KILL))                     # kill_idx
    ins.append((RET_K, 0, 0, ALLOW))                    # allow_idx
    return b"".join(struct.pack("<HBBI", *t) for t in ins)


def _compile_exploit(outdir):
    """Compile every C/C++ source in `outdir` into a single STATIC ELF at the
    fixed path outdir/EXPLOIT_BIN_NAME. Returns (binpath, None) or (None, err)."""
    srcs = sorted(
        os.path.join(outdir, f) for f in os.listdir(outdir)
        if f.endswith(EXPLOIT_SRC_EXT) and os.path.isfile(os.path.join(outdir, f)))
    if not srcs:
        return None, ("RUN error: no C/C++ source (%s) in your exploit dir - "
                      "WRITE exploit.c first." % "/".join(EXPLOIT_SRC_EXT))
    binpath = os.path.join(outdir, EXPLOIT_BIN_NAME)
    cxx = any(s.endswith((".cc", ".cpp", ".cxx")) for s in srcs)
    # Static link so the chroot needs no shared libs / dynamic loader.
    cmd = [("g++" if cxx else "gcc"), "-static", "-O1", "-g", "-o", binpath] + srcs
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
    except (OSError, subprocess.SubprocessError) as exc:
        return None, "RUN error: compiler failed to launch: %s" % exc
    if r.returncode != 0 or not os.path.isfile(binpath):
        return None, "RUN: COMPILE FAILED\n%s" % ((r.stderr or r.stdout or "")[:RUN_OUT_CAP])
    return binpath, None


def _set_sandbox_rlimits():
    """preexec_fn: cap CPU time and address space of the sandboxed child."""
    import resource
    resource.setrlimit(resource.RLIMIT_CPU,
                       (SANDBOX_CPU_SECONDS, SANDBOX_CPU_SECONDS))
    resource.setrlimit(resource.RLIMIT_AS, (SANDBOX_MEM_BYTES, SANDBOX_MEM_BYTES))
    resource.setrlimit(resource.RLIMIT_NOFILE, (64, 64))


def do_run(outdir, _block=None, timeout=RUN_TIMEOUT):
    """Compile the model's C/C++ exploit and RUN the resulting ELF under a hard
    sandbox: bubblewrap chroots it to a READ-ONLY view of the exploit dir (it
    cannot see or touch anything outside that folder) in its own pid/ipc/uts
    namespaces; a seccomp allowlist KILLS any syscall beyond what a TCP client
    needs; RLIMIT_CPU and a 128 MB RLIMIT_AS cap it. Host network is shared on
    purpose so it can reach the gdb-server on 127.0.0.1. Fail CLOSED if bwrap is
    missing - never run the binary unsandboxed."""
    if not BWRAP:
        return ("RUN error: bubblewrap (bwrap) not found - refusing to run the "
                "exploit unsandboxed. Install bwrap.")
    binpath, err = _compile_exploit(outdir)
    if err:
        return err
    seccomp_path = os.path.join(outdir, ".seccomp.bpf")
    try:
        open(seccomp_path, "wb").write(_seccomp_filter_bytes())
        sfd = os.open(seccomp_path, os.O_RDONLY)
    except OSError as exc:
        return "RUN error: cannot stage seccomp filter: %s" % exc
    wrapped = [
        BWRAP,
        "--tmpfs", "/",                    # fresh empty root - the chroot
        "--ro-bind", binpath, "/" + EXPLOIT_BIN_NAME,   # only the ELF is visible
        "--dev", "/dev",
        "--tmpfs", "/tmp",
        "--unshare-pid", "--unshare-ipc", "--unshare-uts", "--unshare-cgroup",
        "--die-with-parent", "--chdir", "/",
        "--seccomp", str(sfd),
        "/" + EXPLOIT_BIN_NAME,
    ]
    try:
        r = subprocess.run(wrapped, capture_output=True, text=True,
                           timeout=timeout, preexec_fn=_set_sandbox_rlimits,
                           pass_fds=(sfd,))
        out = (r.stdout or "") + (r.stderr or "")
        out += "\n[exploit exit code %d]" % r.returncode
        if r.returncode < 0:   # killed by a signal (seccomp SIGSYS, CPU SIGXCPU, ...)
            out += " (terminated by signal %d)" % (-r.returncode)
    except subprocess.TimeoutExpired as e:
        out = (e.stdout or "") + (e.stderr or "") + \
              "\n[exploit wall-timeout after %ds]" % timeout
    except Exception as e:  # noqa
        out = "RUN error: %s" % e
    finally:
        try:
            os.close(sfd)
        except OSError:
            pass
    if len(out) > RUN_OUT_CAP:
        out = out[:RUN_OUT_CAP // 2] + "\n...[truncated]...\n" + out[-RUN_OUT_CAP // 2:]
    return out or "[no output]"


# ---------------------------------------------------------------------------
# Live gdb-driven inspection session (the server-under-gdb the exploit attacks)
# ---------------------------------------------------------------------------
# Strips terminal escapes (colour + bracketed-paste) gdb emits over a pty.
_ANSI_RE = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]")

# Fatal signals that mean the SERVER CRASHED (an exploit SUCCESS). SIGINT is NOT
# here on purpose: that is OUR own `interrupt` to pause for a read, not a crash.
GDB_CRASH_SIGNALS = ("SIGSEGV", "SIGABRT", "SIGBUS", "SIGFPE", "SIGILL",
                     "SIGSYS", "stack smashing detected", "*** stack smashing")

# Memcheck report lines that mean the server touched memory it must not have:
# an out-of-range read/write, a bad/double free, or a fatal signal valgrind
# intercepted. Seeing any of these AFTER the attack packets = the exploit
# corrupted memory = SUCCESS, even when the process never actually crashes (the
# whole reason VALGRIND mode catches what gdb cannot). Leak summaries and
# "uninitialised value" warnings are intentionally NOT here: library noise, not
# attacker-controlled corruption.
VALGRIND_ERROR_MARKERS = (
    "Invalid read of size",
    "Invalid write of size",
    "Invalid free",
    "Mismatched free",
    "Process terminating with default action of signal",
)

# gdb commands the model may run against the LIVE server: an ALLOWLIST by first
# token, inspection-only. We drive run/continue ourselves so the inferior stays
# READ-ONLY - the model observes memory/registers/state but can never write the
# inferior, call functions, resume/step it, touch files, or spawn a shell.
GDB_ALLOWED = {
    "print", "p", "output", "x", "info", "i", "bt", "backtrace", "where",
    "frame", "f", "up", "down", "list", "l", "ptype", "whatis", "display",
    "undisplay", "thread", "break", "b", "tbreak", "hbreak", "watch", "rwatch",
    "awatch", "delete", "d", "clear", "disable", "enable", "ignore",
}
# Reject a single '=' (assignment to the inferior); '==', '!=', '>=', '<=' are
# comparisons and fine.
_ASSIGN_RE = re.compile(r"(?<![=!<>])=(?!=)")


def gdb_cmd_ok(line):
    """True if `line` is a read-only inspection command we permit. We must
    GUARANTEE the model cannot mutate the inferior (else it could just poke a
    'win' flag and fake a SUCCESS). So beyond the allowlist we reject:
      - assignment '='            (set var / print x=...)
      - '(' anywhere              (a function CALL like `print give_item()` or
                                   `p memset(p,0,n)` would execute inferior code
                                   and write memory; casts use '(' too, so we
                                   drop both - reads use `p var`, `p s->f`,
                                   `x/8xb 0xADDR`, `info`, `bt`, ...)."""
    line = line.strip()
    if not line or line.startswith("#"):
        return True
    base = re.split(r"[/\s]", line, 1)[0].lower()   # 'x/4xw' -> 'x'
    if base not in GDB_ALLOWED:
        return False
    if "(" in line or _ASSIGN_RE.search(line):
        return False
    return True


class LiveServer:
    """ONE server instance started under gdb (CLI, on a pty) inside the bwrap
    sandbox, managed entirely by the harness (the model never starts/stops/
    continues it). The model's compiled exploit ELF connects to it over TCP on
    127.0.0.1:GDB_PORT and attacks it; after each RUN the harness polls gdb and
    if the server CRASHED that is an exploit SUCCESS. The model may also inspect
    it via the GDB tool: strictly READ-ONLY (no writes/calls/flow-control), so
    what it observes is the server's REAL state and cannot be forged into a fake
    win. Sandboxed because a confirmed exploit can get code-exec inside the
    server; that must not touch the host. Best-effort research aid."""

    def __init__(self, outdir, mode=None):
        self.outdir = outdir
        self.mode = mode or RUN_UNDER          # 'gdb' or 'valgrind'
        self.rundir = os.path.join(outdir, "gdb-run")
        self.proc = None
        self.mfd = -1
        self.alive = False     # inferior is running (vs stopped/exited)
        self.crashed = False   # a fatal signal / memcheck error was seen = SUCCESS
        self.crash_info = ""   # captured stop banner + backtrace / valgrind report
        self.hung = False      # event loop wedged (no crash) = a DoS / liveness fail
        self._pid = None       # inferior host pid, for /proc CPU-time sampling
        self._baseline_reply = False  # did a healthy server reply to the ping at boot?
        self._closing = False  # in stop(): ignore the kill/exit as a "crash"
        self._booting = False  # during start(): ignore startup memcheck noise

    # -- pty plumbing -------------------------------------------------------
    def _w(self, cmd):
        os.write(self.mfd, (cmd + "\n").encode())

    def _drain(self, idle=0.6, hard=8):
        """Read pty output until it goes idle `idle`s or `hard`s elapse, then
        scan it for a crash (so a fault is caught no matter which call drained
        it - a TCP poke, an interrupt, anything)."""
        import select
        buf = b""
        last = time.time()
        end = time.time() + hard
        while time.time() < end:
            r, _, _ = select.select([self.mfd], [], [], 0.2)
            if r:
                try:
                    d = os.read(self.mfd, 65536)
                except OSError:
                    break
                if d:
                    buf += d
                    last = time.time()
                else:
                    break
            elif time.time() - last > idle:
                break
        text = _ANSI_RE.sub("", buf.decode(errors="replace")).replace("\r", "")
        self._scan_crash(text)
        return text

    def _scan_crash(self, text):
        """Latch a crash/memory-error the first time one shows up in the output.
        gdb mode: a fatal signal. valgrind mode: a fatal signal OR any Memcheck
        error marker (an OOB/invalid access counts even with no crash). Startup
        memcheck noise is ignored (see self._booting)."""
        if self._closing or self.crashed:
            return
        markers = GDB_CRASH_SIGNALS
        if self.mode == "valgrind":
            # Don't let library/startup memcheck chatter before the attack count
            # as an exploit; only post-boot errors are attacker-induced.
            if self._booting:
                return
            markers = markers + VALGRIND_ERROR_MARKERS
        if any(m in text for m in markers):
            self.crashed = True
            self.alive = False
            if self.mode == "gdb":
                # Inferior is stopped at the fault now - grab a backtrace as proof.
                try:
                    self._w("bt")
                    bt = self._drain(idle=0.5, hard=6)  # crashed already latched
                except OSError:
                    bt = ""
                self.crash_info = (text + "\n" + bt).strip()
            else:
                # Valgrind already printed its own stack trace inline with the
                # error; the captured text IS the proof (no gdb prompt to query).
                self.crash_info = text.strip()

    # -- lifecycle ----------------------------------------------------------
    def start(self):
        if self.proc is not None:
            return "[%s session already running on port %d]" % (self.mode, GDB_PORT)
        if not BWRAP:
            return ("%s error: bubblewrap (bwrap) not found - refusing to run "
                    "the server unsandboxed. Install bwrap." % self.mode.upper())
        if self.mode == "valgrind" and not VALGRIND:
            return ("VALGRIND error: valgrind not found - install it or run with "
                    "SECSERVER_MODE=gdb.")
        if not os.path.isfile(SERVER_BIN) or not os.path.isdir(STAGED_RUN):
            return ("%s error: server kit missing (%s / %s) - build it first."
                    % (self.mode.upper(), SERVER_BIN, STAGED_RUN))
        # Private run dir on a distinct port (keep symlinks, e.g. datapack).
        if os.path.isdir(self.rundir):
            shutil.rmtree(self.rundir, ignore_errors=True)
        shutil.copytree(STAGED_RUN, self.rundir, symlinks=True)
        props = os.path.join(self.rundir, "server-properties.xml")
        try:
            txt = open(props).read()
            txt = txt.replace('value="%d"' % SERVER_PORT, 'value="%d"' % GDB_PORT)
            open(props, "w").write(txt)
        except OSError as exc:
            return "%s error: cannot set port in server-properties.xml: %s" % (
                self.mode.upper(), exc)
        binpath = os.path.join(self.rundir, "catchchallenger-server-cli")
        if not os.path.isfile(binpath):
            binpath = SERVER_BIN
        # The supervisor (gdb or valgrind) + the server run inside bwrap: whole
        # fs read-only except this exploit's outdir; network shared so the TCP
        # attack reaches it. (pid namespace is NOT unshared, so gdb's ptrace of
        # its child / valgrind's /proc pid both work.)
        if self.mode == "valgrind":
            # Memcheck with no leak/uninit noise: we only care about OOB / bad
            # frees / fatal signals (the markers in VALGRIND_ERROR_MARKERS).
            # --error-exitcode=0 so a reported error never changes the exit code
            # (we latch on the printed marker, not the exit status).
            tool = [VALGRIND, "--tool=memcheck", "--error-exitcode=0",
                    "--leak-check=no", "--track-origins=no", "--num-callers=20",
                    "--child-silent-after-fork=yes", binpath]
        else:
            tool = ["gdb", "-q", "-nx", binpath]
        wrapped = [
            BWRAP,
            "--ro-bind", "/", "/",
            "--dev", "/dev", "--proc", "/proc", "--tmpfs", "/tmp",
            "--bind", self.outdir, self.outdir,
            "--die-with-parent", "--chdir", self.rundir,
            "--setenv", "TMPDIR", self.outdir,
        ] + tool
        import pty
        mfd, sfd = pty.openpty()
        self.proc = subprocess.Popen(
            wrapped, stdin=sfd, stdout=sfd, stderr=sfd, close_fds=True)
        os.close(sfd)
        self.mfd = mfd
        self._booting = True
        if self.mode == "gdb":
            for c in ("set pagination off", "set confirm off",
                      "set target-async on", "set width 0", "set height 0"):
                self._w(c)
            self._drain(idle=0.4, hard=3)
            self._w("run &")
            out = self._drain(idle=0.6, hard=30)
        else:
            # Valgrind instruments the whole Qt/SQL startup -> much slower and it
            # boots on its own (no "run" to send). Wait longer, and give it a
            # bigger idle gap before concluding it has settled at the listen loop.
            out = self._drain(idle=3.0, hard=240)
        self._booting = False
        self.alive = "Waiting connection on port" in out and not self.crashed
        if self.alive:
            self._capture_pid(out)
            # Baseline: does a HEALTHY server reply to the event-loop ping? If so,
            # later silence under an exploit is a hang. If not, we lean on the
            # CPU-spin signal instead.
            self._baseline_reply = self._protocol_ping()[1]
        status = "listening" if self.alive else "NOT confirmed listening"
        return ("[%s session started (sandboxed): server under %s on "
                "127.0.0.1:%d (%s)]\n%s"
                % (self.mode, self.mode, GDB_PORT, status, out[-2000:]))

    def _capture_pid(self, boot_text=None):
        """Cache the inferior's (host) pid for /proc-based liveness sampling.
        bwrap does NOT unshare the pid namespace, so the reported pid is a real
        host pid we can read under /proc. gdb: ask it. valgrind: parse its own
        '==PID==' line prefix from the boot output."""
        if self.mode == "valgrind":
            m = re.search(r"==(\d+)==", boot_text or "")
            self._pid = int(m.group(1)) if m else None
            return
        self._w("interrupt")
        out = self._drain(idle=0.4, hard=5)
        m = re.search(r"process (\d+)", out)
        if not m:
            self._w("info inferiors")
            m = re.search(r"process (\d+)", self._drain(idle=0.4, hard=4))
        self._pid = int(m.group(1)) if m else None
        self._continue()

    def _interrupt(self):
        """Stop the inferior so memory can be read (our own SIGINT - not a
        crash). Returns the stop banner. No-op under valgrind (no gdb prompt)."""
        if self.mode != "gdb":
            return ""
        self._w("interrupt")
        return self._drain(idle=0.5, hard=6)

    def _continue(self):
        if self.mode != "gdb":
            return
        if self.alive and not self.crashed:
            self._w("continue &")
            self._drain(idle=0.3, hard=3)

    def _crash_suffix(self):
        if self.crashed:
            if self.mode == "valgrind":
                return ("\n\n*** VALGRIND FLAGGED A MEMORY ERROR - this is an "
                        "exploit SUCCESS. Memcheck report:\n%s"
                        % (self.crash_info or "(no report)"))
            return ("\n\n*** SERVER CRASHED - this is an exploit SUCCESS. "
                    "Backtrace:\n%s" % (self.crash_info or "(no backtrace)"))
        return ""

    def gdb(self, block):
        """Run the model's read-only gdb command(s) against the live inferior.
        Unavailable under valgrind: valgrind owns the process, so we cannot
        attach a read-only gdb to it. The model doesn't need it there - any
        memory error is auto-detected from valgrind's output after each RUN."""
        if self.mode != "gdb":
            return ("[live GDB inspection is OFF in VALGRIND mode] The server "
                    "runs under valgrind/memcheck; after each RUN the harness "
                    "scans valgrind's output and AUTO-CONFIRMS any memory error "
                    "it reports (invalid read/write, OOB, bad free, fatal "
                    "signal). Just RUN your attack packets - no inspection "
                    "needed." + self._crash_suffix())
        if self.proc is None:
            start_out = self.start()
            if start_out.startswith("GDB error"):
                return start_out
        if self.crashed:
            return "[server already crashed]" + self._crash_suffix()
        cmds = [ln for ln in (block or "").splitlines() if ln.strip()]
        if not cmds:
            return "GDB error: no commands (put them in a ``` code block)."
        bad = [c for c in cmds if not gdb_cmd_ok(c)]
        if bad:
            return ("GDB refused (read-only inspection ONLY - no inferior "
                    "writes/calls/flow-control/shell; '(' and '=' are blocked): "
                    "%s" % "; ".join(bad))
        banner = self._interrupt()
        chunks = []
        for c in cmds:
            if self.crashed:
                break
            self._w(c)
            chunks.append("(gdb) %s\n%s" % (c, self._drain(idle=0.5, hard=8)))
        self._continue()
        return ((banner + "\n" if banner.strip() else "")
                + "\n".join(chunks) + self._crash_suffix())

    # CPU time (clock ticks) the server can burn over the sample window before we
    # call it spinning. A healthy event loop blocks in epoll/recv (≈0 CPU); a
    # wedged one (infinite loop / busy deadlock) burns a full core.
    _HANG_SAMPLE_S = 0.6
    _HANG_TICKS = 15            # >0.15s CPU in 0.6s wall = spinning (CLK_TCK=100)

    def _cpu_ticks(self):
        """utime+stime of the inferior from /proc, or None."""
        if not self._pid:
            return None
        try:
            fields = open("/proc/%d/stat" % self._pid).read().rsplit(")", 1)[1].split()
            # after "comm)": state is fields[0]; utime=fields[11], stime=fields[12]
            return int(fields[11]) + int(fields[12])
        except (OSError, IndexError, ValueError):
            return None

    # Handshake the client sends first (PROTOCOL_HEADER_LOGIN) - used to ping the
    # event loop: a healthy loop accepts the connection and processes these bytes.
    PROTOCOL_HEADER_LOGIN = b"\x9c\xd6\x49\x8d\x14"

    def _protocol_ping(self, timeout=2.0):
        """Ping the event loop: open a FRESH TCP connection, send the login
        handshake, wait for a reply. Returns (connected, got_reply). A wedged
        loop won't process the connection -> no reply."""
        import socket
        try:
            c = socket.create_connection(("127.0.0.1", GDB_PORT), timeout=timeout)
        except OSError:
            return (False, False)
        got = False
        try:
            c.sendall(self.PROTOCOL_HEADER_LOGIN)
            c.settimeout(timeout)
            try:
                got = bool(c.recv(4096))
            except socket.timeout:
                got = False
        except OSError:
            got = False
        finally:
            try:
                c.close()
            except OSError:
                pass
        return (True, got)

    def check_responsive(self):
        """Liveness check so the harness NEVER hangs on a wedged server AND so a
        hang counts as a validated exploit. Two independent signals:
          1. CPU spin: sample the inferior's CPU time over a short window; a full
             core burned while idle of input = an INFINITE LOOP.
          2. Event-loop ping: open a fresh TCP connection and send the login
             handshake; a healthy loop replies, a wedged/deadlocked one does not
             (judged against the baseline reply captured at startup).
        Either signal sets self.hung. Symbol-independent."""
        if self.proc is None or self.crashed or not self.alive:
            return ""
        t0 = self._cpu_ticks()
        time.sleep(self._HANG_SAMPLE_S)
        t1 = self._cpu_ticks()
        connected, got_reply = self._protocol_ping()
        # A crash during the window would have closed the port; re-check gdb.
        self._drain(idle=0.3, hard=2)
        if self.crashed:
            return self._crash_suffix()
        burned = (t1 - t0) if (t0 is not None and t1 is not None) else None
        spinning = burned is not None and burned >= self._HANG_TICKS
        # Unresponsive: can't connect at all, OR it answered the same ping at
        # startup (baseline) but now stays silent.
        unresponsive = (not connected) or (self._baseline_reply and not got_reply)
        if spinning or unresponsive:
            self.hung = True
            why = []
            if spinning:
                why.append("infinite loop: server burned %dms CPU in %.1fs while "
                           "idle of input" % (burned * 10, self._HANG_SAMPLE_S))
            if unresponsive:
                why.append("event loop did not answer a TCP+handshake ping "
                           "(connect=%s, reply=%s)"
                           % ("ok" if connected else "FAILED",
                              "yes" if got_reply else "none"))
            return ("\n\n*** SERVER HANG (DoS) - this is an exploit SUCCESS. %s. "
                    "Confirm with GDB (interrupt + bt shows the stuck loop)."
                    % "; ".join(why))
        return ("[responsive: ~0 CPU while idle, event-loop ping reply=%s]"
                % ("yes" if got_reply else "n/a"))

    def poll_crash(self, hard=4):
        """After a RUN: did the exploit CRASH the server / trip a memcheck error
        (= SUCCESS) or HANG its event loop (= DoS / liveness failure)? Returns a
        short status string."""
        if self.proc is None:
            return "[no live server]"
        if not self.crashed:
            # Valgrind is slow and prints its error report a beat after the bad
            # access; give the pty a little longer to surface it.
            self._drain(idle=0.5 if self.mode == "valgrind" else 0.4,
                        hard=hard * 3 if self.mode == "valgrind" else hard)
        if self.crashed:
            tag = ("valgrind flagged a memory error" if self.mode == "valgrind"
                   else "live server CRASHED")
            return tag + self._crash_suffix()
        # No crash - make sure the event loop is still alive (never hangs).
        return "[live server did not crash]" + self.check_responsive()

    def restart(self):
        """Tear down the current server-under-gdb and boot a FRESH one on the
        same port. Lets the model reset to a clean baseline between probes (a
        game-state mutation in case (c), or a wedged loop) without ever giving
        it control of the inferior's flow. Clears every per-instance latch so a
        later poll reflects ONLY the new server, then re-starts."""
        self.stop()
        # stop() left proc=None and _closing=True; reset the whole instance
        # state so the new server starts from a clean slate.
        self._closing = False
        self.crashed = False
        self.crash_info = ""
        self.hung = False
        self.alive = False
        self._pid = None
        self._baseline_reply = False
        out = self.start()
        return "[server restarted on a fresh instance]\n" + out

    def set_mode(self, new_mode):
        """Switch the SUPERVISOR the target runs under, on the model's request.
        gdb (signals + read-only live inspection) <-> valgrind (Memcheck: catches
        SILENT out-of-bounds / bad-free that never crash, no live inspection).
        Reboots the server under the chosen tool on the same port. The model
        drives this so it can pick whichever exposes the bug it is chasing."""
        want = "valgrind" if new_mode.strip().lower() in (
            "valgrind", "vg", "memcheck") else "gdb"
        if want == "valgrind" and not VALGRIND:
            return ("MODE error: valgrind is not installed on this host; staying "
                    "under %s." % self.mode)
        if want == self.mode and self.proc is not None:
            return "[already supervising under %s on 127.0.0.1:%d]" % (
                self.mode, GDB_PORT)
        self.mode = want
        return ("[supervisor switched to %s]\n" % want) + self.restart()

    def stop(self):
        self._closing = True
        if self.proc is not None:
            try:
                self._w("kill")
                self._w("quit")
            except OSError:
                pass
            try:
                self.proc.kill()
            except OSError:
                pass
            try:
                os.close(self.mfd)
            except OSError:
                pass
            self.proc = None


# The live gdb/TCP session for the exploit currently in progress. Held at module
# level so run_exploit() can guarantee it is torn down (freeing GDB_PORT) even if
# a turn raises or the run aborts.
_CURRENT_LIVE = None


def exploit_one(rel, finding, idx, hard_budget, soft_budget):
    """Run the exploit agent for one candidate. Returns (verdict, reason, dir)."""
    global _CURRENT_LIVE
    outdir = os.path.join(OUTPUT_ROOT, slugify(rel, idx))
    os.makedirs(outdir, exist_ok=True)
    sys.stderr.write("\n%s [exploit %02d] %s -> %s\n" % (_ts(), idx, rel, outdir))
    # server.py OWNS the supervisor instance: we boot the server under gdb (or
    # valgrind) up front and tear it down at the end. The model never starts/
    # stops it - it only pokes the already-running instance via TCP (and, in gdb
    # mode, inspects it read-only via GDB).
    live = LiveServer(outdir)
    _CURRENT_LIVE = live
    boot = live.start()
    sys.stderr.write("%s     [%s-instance] %s\n"
                     % (_ts(), live.mode,
                        boot.splitlines()[0] if boot else "(no output)"))

    # Correct any wrong repo paths in the scanner finding before showing it to
    # the exploit agent (e.g. "server/base/X.cpp" -> "general/base/X.cpp").
    finding = fix_paths(finding)
    # Include the actual source file so the exploit agent can verify line
    # numbers and understand the exact code it is attacking.
    src_path = os.path.join(REPO_ROOT, rel)
    try:
        src_content = open(src_path, "r", errors="replace").read()
    except OSError:
        src_content = "(source not readable)"
    src_lines = src_content.splitlines()
    src_block = (
        "\n\n=== SOURCE FILE (%s, %d lines) ===\n%s"
        % (rel, len(src_lines),
           "\n".join("%d\t%s" % (i + 1, ln) for i, ln in enumerate(src_lines)))
    )
    user = (
        "Candidate finding to prove or refute:\n\nFILE: %s\n\nSCANNER NOTE:\n%s"
        "%s\n\n"
        "Your exploit dir is: %s\nWRITE your C/C++ exploit there (paths relative "
        "to it); RUN compiles it and runs the ELF sandboxed against the server "
        "ALREADY running on 127.0.0.1:%d (managed by the harness). It starts "
        "under MODE %s; switch with 'MODE gdb' / 'MODE valgrind' whenever the "
        "other supervisor better exposes this bug (valgrind for OOB/bad-free, "
        "gdb for live game-state inspection). Begin."
        % (rel, finding, src_block, outdir, GDB_PORT, live.mode))
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
    last_answer = None      # previous reply, to detect verbatim repetition
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
        # Hard repetition guard: if the model emits the EXACT same reply twice
        # in a row, it is stuck (the 33B sometimes apologises in a tight loop,
        # e.g. "can't find EventLoop.h ..."). Something is wrong - stop this
        # exploit and abort the WHOLE run with exit code 255 (per operator
        # policy: a repeated sentence is a fatal signal, not a soft skip).
        if last_answer is not None and answer.strip() == last_answer.strip():
            sys.stderr.write("%s     [REPEAT ABORT] identical reply twice on "
                             "exploit %02d (%s) -> aborting whole run (exit 255)\n"
                             % (_ts(), idx, rel))
            transcript.append("### REPEAT ABORT (identical reply twice; exit 255)")
            verdict, reason = ("ABORTED",
                               "model repeated the same reply twice; run aborted")
            open(os.path.join(outdir, "transcript.md"), "w").write(
                "# Exploit transcript: %s\n\nVERDICT: %s - %s\n\n%s\n"
                % (rel, verdict, reason, "\n\n".join(transcript)))
            live.stop()
            sys.exit(255)
        last_answer = answer
        act = parse_action(answer)
        # A non-action reply (that is NOT a verbatim repeat) just gets nudged
        # back into the one-action protocol.
        if act is None:
            messages.append({"role": "assistant", "content": answer})
            messages.append({"role": "user", "content":
                "Reply with exactly ONE action (READ/GREP/WRITE/RUN/GDB/VERDICT) "
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
            # time left (min 1s) so a long probe cannot overrun the budget.
            run_to = max(1, min(RUN_TIMEOUT, int(deadline - time.time())))
            sys.stderr.write("%s     [RUN] compile+run exploit ELF (step %d, "
                             "%ds left, timeout %ds)\n"
                             % (_ts(), step, remaining, run_to))
            result = do_run(outdir, timeout=run_to)
            # The exploit just hammered the live server - did it fault it?
            result += "\n\n--- live server status ---\n" + live.poll_crash()
            ran_something = True
        elif kind == "GDB":
            sys.stderr.write("%s     [GDB] (step %d)\n" % (_ts(), step))
            result = live.gdb(block)
            ran_something = True
        elif kind == "RESTARTSERVER":
            sys.stderr.write("%s     [RESTARTSERVER] (step %d)\n" % (_ts(), step))
            result = live.restart()
        elif kind == "MODE":
            sys.stderr.write("%s     [MODE] -> %s (step %d)\n"
                             % (_ts(), arg, step))
            result = live.set_mode(arg)
        else:
            result = "unknown action"
        transcript.append("### tool result (%s %s)\n%s" % (kind, arg, result))
        # The live server CRASHED / valgrind flagged a memory error under the
        # exploit - that IS the proof. End the exploit NOW as SUCCESS; do not
        # keep poking (nothing left to confirm).
        if live.crashed:
            if live.mode == "valgrind":
                tag, why = "MEMERR", "valgrind detected a memory error"
            else:
                tag, why = "CRASH", "live server crashed"
            sys.stderr.write("%s     [%s] %s -> SUCCESS, ending exploit %02d "
                             "(%s)\n" % (_ts(), tag, why, idx, rel))
            verdict = "CONFIRMED"
            reason = "%s under crafted packets: %s" % (
                why, " ".join((live.crash_info or "").split())[:160])
            transcript.append("### %s DETECTED (auto-SUCCESS)\n%s"
                              % (tag, live.crash_info))
            break
        # The live server HUNG (infinite loop / unresponsive event loop) under the
        # exploit - a DoS. Per policy this also counts as a VALIDATED exploit.
        if live.hung:
            sys.stderr.write("%s     [HANG] live server event loop wedged -> "
                             "SUCCESS, ending exploit %02d (%s)\n"
                             % (_ts(), idx, rel))
            verdict = "CONFIRMED"
            reason = ("live server event loop hung (DoS) under crafted packets - "
                      "no crash, server stopped responding")
            transcript.append("### HANG DETECTED (auto-SUCCESS): event loop "
                              "unresponsive / infinite loop")
            break
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

    live.stop()
    _CURRENT_LIVE = None
    open(os.path.join(outdir, "transcript.md"), "w").write(
        "# Exploit transcript: %s\n\nVERDICT: %s - %s\n\n%s\n"
        % (rel, verdict, reason, "\n\n".join(transcript)))
    return verdict, reason, outdir


def build_server():
    """Configure + compile the gdb-able cli server into BUILD_DIR, producing
    SERVER_BIN. Built with internal-vars DB (no external SQL needed) and
    RelWithDebInfo so gdb/valgrind get symbols + line numbers for the exploit
    phase. Returns True on success. Best-effort ccache flush + one retry on a
    compile failure (a stale ccache entry can wedge a build); never fatal if
    ccache is absent. cmake build dir is OUT of the source tree (under the
    tmpfs OUTPUT_ROOT), per the repo's separate-artifacts rule."""
    src = os.path.join(REPO_ROOT, "server", "cli")
    configure = ["cmake", "-S", src, "-B", BUILD_DIR,
                 "-DCATCHCHALLENGER_DB_INTERNAL_VARS=ON",
                 "-DCMAKE_BUILD_TYPE=RelWithDebInfo"]
    compile_ = ["cmake", "--build", BUILD_DIR, "-j"]
    try:
        os.makedirs(BUILD_DIR, exist_ok=True)
    except OSError as exc:
        sys.stderr.write("%s [build] cannot create %s: %s\n"
                         % (_ts(), BUILD_DIR, exc))
        return False

    sys.stderr.write("%s [build] configuring server: %s\n"
                     % (_ts(), " ".join(configure)))
    try:
        r = subprocess.run(configure, capture_output=True, text=True)
    except (OSError, subprocess.SubprocessError) as exc:
        sys.stderr.write("%s [build] cmake configure failed to launch: %s\n"
                         % (_ts(), exc))
        return False
    if r.returncode != 0:
        sys.stderr.write("%s [build] CONFIGURE FAILED:\n%s\n"
                         % (_ts(), (r.stderr or r.stdout or "")[-4000:]))
        return False

    # Compile, then on failure flush ccache once and retry (a poisoned ccache
    # entry can fail a build that is otherwise fine).
    attempt = 0
    while attempt < 2:
        attempt += 1
        sys.stderr.write("%s [build] compiling (attempt %d): %s\n"
                         % (_ts(), attempt, " ".join(compile_)))
        try:
            r = subprocess.run(compile_, capture_output=True, text=True)
        except (OSError, subprocess.SubprocessError) as exc:
            sys.stderr.write("%s [build] cmake build failed to launch: %s\n"
                             % (_ts(), exc))
            return False
        if r.returncode == 0 and os.path.isfile(SERVER_BIN):
            sys.stderr.write("%s [build] OK -> %s\n" % (_ts(), SERVER_BIN))
            return True
        sys.stderr.write("%s [build] COMPILE FAILED:\n%s\n"
                         % (_ts(), (r.stderr or r.stdout or "")[-4000:]))
        ccache = shutil.which("ccache")
        if ccache and attempt < 2:
            sys.stderr.write("%s [build] flushing ccache and retrying once\n"
                             % _ts())
            subprocess.run([ccache, "-C"], capture_output=True, text=True)
        else:
            break
    return False


def staged_datapack():
    """A read-only STAGED copy of the datapack (never the source tree, per the
    datapack-source-safety rule). Prefers the test harness's staged slot, then a
    known tmpfs cache, and only as a last resort the source itself (read-only)."""
    try:
        if TEST_DIR not in sys.path:
            sys.path.insert(0, TEST_DIR)
        import datapack_stage
        p = datapack_stage.staged_local(DATAPACK_SRC, server=False)
        if os.path.isdir(p):
            return p
    except Exception:
        pass
    cand = "/mnt/data/perso/tmpfs/cc-datapack/CatchChallenger-datapack"
    if os.path.isdir(cand):
        return cand
    return DATAPACK_SRC


def stage_run():
    """Build STAGED_RUN: the boot kit LiveServer copies per-exploit. Contains a
    binary copy (the server resolves its datapack relative to argv[0], so the
    binary must sit next to the datapack), a `datapack` symlink to a staged
    read-only copy, and server-properties.xml from the bounty doc's run recipe.
    Idempotent (rewritten each call). Returns True when the kit is ready."""
    if not os.path.isfile(SERVER_BIN):
        sys.stderr.write("%s [stage] no server binary to stage: %s\n"
                         % (_ts(), SERVER_BIN))
        return False
    dp = staged_datapack()
    if not os.path.isdir(os.path.join(dp, "map", "main", "test")):
        sys.stderr.write("%s [stage] WARNING: datapack %s has no map/main/test "
                         "(maincode 'test') - server may fail to load\n"
                         % (_ts(), dp))
    try:
        os.makedirs(STAGED_RUN, exist_ok=True)
        # binary copy: the server looks for datapack/ next to argv[0].
        local_bin = os.path.join(STAGED_RUN, "catchchallenger-server-cli")
        shutil.copy2(SERVER_BIN, local_bin)
        os.chmod(local_bin, 0o755)
        # datapack symlink -> staged read-only copy (never the source tree).
        link = os.path.join(STAGED_RUN, "datapack")
        if os.path.islink(link) or os.path.exists(link):
            os.remove(link)
        os.symlink(dp, link)
        # server-properties.xml (auto-create, compression none, maincode test).
        with open(os.path.join(STAGED_RUN, "server-properties.xml"), "w") as f:
            f.write(STAGED_PROPERTIES.format(port=SERVER_PORT))
    except OSError as exc:
        sys.stderr.write("%s [stage] cannot build run kit: %s\n" % (_ts(), exc))
        return False
    sys.stderr.write("%s [stage] run kit ready: %s (datapack -> %s)\n"
                     % (_ts(), STAGED_RUN, dp))
    return True


def run_exploit():
    """Phase 2. Returns 0/1."""
    check_tooling()
    if not os.path.isfile(SERVER_BIN):
        sys.stderr.write("%s [!] prebuilt server not found: %s - building it now\n"
                         % (_ts(), SERVER_BIN))
        if not build_server():
            sys.stderr.write(
                "[!] automatic build failed. Build it manually:\n"
                "    cmake -S %s/server/cli -B %s "
                "-DCATCHCHALLENGER_DB_INTERNAL_VARS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo "
                "&& cmake --build %s -j\n"
                % (REPO_ROOT, BUILD_DIR, BUILD_DIR))
            return 1
    # Stage the run kit (datapack + server-properties.xml + binary copy) the
    # LiveServer boots from. Without it every exploit aborts with "server kit
    # missing". Rebuilt each run so a fresh binary is always staged.
    if not stage_run():
        sys.stderr.write("[!] could not stage the server run kit (%s); the "
                         "exploit phase cannot boot the target.\n" % STAGED_RUN)
        return 1
    ensure_context()
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
        finally:
            # Guarantee the gdb child + its server (holding GDB_PORT) are gone
            # before the next candidate, even if the turn raised mid-session.
            if _CURRENT_LIVE is not None:
                _CURRENT_LIVE.stop()
        # Keep EVERY exploit folder (confirmed or not). A non-confirmed one is
        # not deleted - we just drop a fail.md marker into it recording the
        # verdict and the logic-path explanation of WHY it is not exploitable.
        # The aggregate not-exploitable.md is still appended for a quick index.
        kept = os.path.basename(outdir)
        if verdict != "CONFIRMED" and outdir != "-" and os.path.isdir(outdir):
            with open(os.path.join(OUTPUT_ROOT, "not-exploitable.md"), "a") as fh:
                fh.write("## %s\n**%s.** logic path why it cannot be "
                         "exploited:\n\n%s\n\n---\n\n"
                         % (rel, verdict, reason or "(no explanation given)"))
            with open(os.path.join(outdir, "fail.md"), "w") as fh:
                fh.write("# FAIL: %s\n\nVERDICT: %s\n\nlogic path why it cannot "
                         "be exploited:\n\n%s\n"
                         % (rel, verdict, reason or "(no explanation given)"))
            kept = "%s (fail.md)" % os.path.basename(outdir)
            sys.stderr.write("    [fail] %s -> not exploitable; marked with "
                             "%s/fail.md\n" % (rel, os.path.basename(outdir)))
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
def _print_help(prog):
    sys.stderr.write(
        "usage: %s [--model=NAME] [scan|exploit|all] [2> progress.log]\n"
        "\n"
        "Modes (default: all):\n"
        "  scan     Audit server/cli; findings printed to stdout AND written\n"
        "           to %s\n"
        "  exploit  Generate+run exploits for %s\n"
        "  all      scan then exploit  [DEFAULT]\n"
        "\n"
        "Options:\n"
        "  --model=NAME   Ollama model to use (default: %s)\n"
        "                 e.g. --model=qwen3:30b-a3b or --model=qwen2.5-coder:32b\n"
        "  --mode=MODE    STARTING supervisor: gdb (default) or valgrind. The\n"
        "                 exploit model switches at runtime via its MODE action;\n"
        "                 this only sets which tool the first instance boots\n"
        "                 under. Same as SECSERVER_MODE.\n"
        "  --help, -h     Show this help\n"
        "\n"
        "Environment overrides:\n"
        "  SECSERVER_MODE        gdb (default) | valgrind  starting supervisor;\n"
        "                        the exploit model switches at runtime (see --mode)\n"
        "  SECSERVER_BUDGET      Hard per-exploit budget in seconds\n"
        "  SECSERVER_SOFT_BUDGET Soft per-exploit budget in seconds (default 75%%)\n"
        "  SECSERVER_STEPS       Max exploit turns (default 400)\n"
        "  SECSERVER_RUNTIMEOUT  RUN wall timeout in seconds (default 900)\n"
        "  SECSERVER_CPU         Sandbox RLIMIT_CPU seconds (default 10)\n"
        "  SECSERVER_MEM         Sandbox RLIMIT_AS bytes (default 128M)\n"
        "  SECSERVER_MAX         Max exploit candidates to attempt\n"
        % (prog, FINDINGS, FINDINGS, MODEL_NAME))


def main(argv):
    global MODEL_NAME, RUN_UNDER

    # scan always writes FINDINGS to disk (tee), so the exploit phase works
    # regardless of whether stdout was redirected.
    args = argv[1:]

    # Strip --model=NAME / --mode=gdb|valgrind / --help/-h before the phase word.
    filtered = []
    for a in args:
        if a.startswith("--model="):
            MODEL_NAME = a[len("--model="):]
        elif a.startswith("--mode="):
            m = a[len("--mode="):].strip().lower()
            RUN_UNDER = "valgrind" if m in ("valgrind", "vg", "memcheck") else "gdb"
        elif a in ("--help", "-h"):
            _print_help(argv[0])
            return 0
        else:
            filtered.append(a)
    args = filtered

    mode = args[0] if args else "all"
    if mode in ("scan", "-s", "--scan"):
        return run_scan(sys.stdout)
    if mode in ("exploit", "-e", "--exploit"):
        return run_exploit()
    if mode in ("all", "-a", "--all"):
        os.makedirs(OUTPUT_ROOT, exist_ok=True)
        rc = run_scan(sys.stdout)
        sys.stdout.flush()
        if rc != 0:
            return rc
        return run_exploit()
    sys.stderr.write("error: unknown mode %r\n" % mode)
    _print_help(argv[0])
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
