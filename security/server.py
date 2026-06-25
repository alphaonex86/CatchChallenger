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

import concurrent.futures
import functools
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import threading
import time
import urllib.error
import urllib.request

import common  # shared IA backend transport (Ollama / Claude)
from common import *  # noqa: F401,F403  (chat, settings, _ts, _Tee, ...)

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
    # Other client-facing binaries IN bounty scope (EXAMPLE-BOUNTY §2): the
    # PRE-AUTH login parser is the single highest-value remote surface, and
    # gateway/game-server-alone also face untrusted client TCP. master/ and
    # inter-node links stay OUT of scope (internal-only) and are NOT listed.
    "server/login",
    "server/gateway",
    "server/game-server-alone",
    "general/base",     # ProtocolParsing* framing + shared engine/datapack parse
    "general/fight",    # shared turn-based fight engine
))

# IA backend transport (Ollama/Claude routing, num_ctx sizing, usage
# accounting, chat() entry points) lives in common.py, imported above.
_NOTES_LOCK = threading.Lock()   # serialises SECURITY-IA.md appends

HERE = os.path.dirname(os.path.realpath(__file__))


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

# Active compile defines for the audited CLI server binary (RelWithDebInfo +
# CATCHCHALLENGER_DB_INTERNAL_VARS, as build_server() configures). Telling the
# IA which #ifdef branches are LIVE stops it auditing Qt-only / dead branches
# (NormalServerGlobal.cpp, Map_loader.cpp, the Qt admin GUI) and focuses the
# review on the no-Qt event-loop code that is the actual remote attack surface.
ACTIVE_DEFINES = (
    "=== ACTIVE COMPILE DEFINES (the CLI server binary built by this tool) ===\n"
    "Live (compiled in): CATCHCHALLENGER_SERVER, "
    "CATCHCHALLENGER_CLASS_ALLINONESERVER, CATCHCHALLANGER_DB_FILE, "
    "CATCHCHALLENGER_DB_INTERNAL_VARS.\n"
    "OFF (preprocessed out, do NOT audit those branches): "
    "CATCHCHALLENGER_CLASS_QT, CATCHCHALLENGER_SERVER_QT, "
    "CATCHCHALLENGER_DB_MYSQL/POSTGRESQL/SQLITE/BLACKHOLE, "
    "CATCHCHALLENGER_CLIENT_* .\n"
    "A branch guarded by `#ifndef CATCHCHALLENGER_SERVER` / `#ifdef "
    "CATCHCHALLENGER_CLASS_QT` etc. that resolves to OFF here is DEAD CODE in "
    "the target binary - clear it as 'No issues identified (dead branch)' "
    "without further analysis. Only the LIVE branches are in scope.\n"
    "=== END ACTIVE DEFINES ===\n"
)

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
    "- The Qt admin GUI (server/qt) and ALL Qt-only code: the audited binary is "
    "the no-Qt CLI game server (epoll EventLoop). Code compiled only into the Qt "
    "build is not in the attack surface. (A file whose Qt path is #ifdef'd out in "
    "the CLI build is in scope only for its NON-Qt, compiled-in code.)\n"
    "Even a REAL memory-corruption or injection bug (overflow, OOB, "
    "command/SQL injection) on one of these LOCAL channels is OUT OF SCOPE "
    "here - output 'No issues identified' for it UNLESS the very same sink is "
    "ALSO reachable from the remote TCP path. When unsure whether an input is "
    "remote-TCP or local, treat it as local (out of scope).\n"
    "\n"
    "STRICT RULES:\n"
    "- This is ONE file out of a larger project. Types, classes, and methods "
    "used here are DEFINED IN OTHER FILES. To judge exploitability you must "
    "read AROUND the file: the COUNTERPART .cpp/.hpp (the CALLED code - "
    "function bodies where the overflow/OOB actually lives) and the CALLERS "
    "(the TAINT SOURCE - where untrusted client bytes enter and flow in). "
    "These are attached as 'COUNTERPART', 'USED-CLASS DEFINITION', 'RELATED "
    "FILE' and 'CALLERS' hints; fetch any others with the tools below.\n"
    "- TRACE THE TAINT before deciding. A finding is real only if you can walk "
    "the path attacker-bytes (EventLoopClient::read -> ProtocolParsing* / "
    "ClientNetworkRead* -> a CALLER) into the sink in THIS file. Conversely do "
    "NOT clear a function that consumes a length/index/id/size/offset/pointer "
    "with 'No issues' until you have checked - via the CALLER hints, a GREP for "
    "its call sites, or READing its implementation - that no untrusted TCP "
    "input reaches it unchecked. A header's declarations seen alone are NEVER "
    "sufficient to clear it.\n"
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
    "  ANALYZE <path>  - Clang Static Analyzer findings for a .cpp (or a header "
    "via its .cpp), built with the CLI server's real flags: path-sensitive "
    "null-deref / use-after-free / OOB / leak / API-misuse candidates\n"
    "Use tools when a method's behaviour matters to the verdict (e.g. how a "
    "length is parsed upstream, or to ANALYZE a callee/caller). I will reply "
    "with the result and you may call another tool. When you have enough "
    "information, STOP calling tools and output your final answer. Do NOT mix a "
    "tool line with findings.\n"
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
    "terse, not verbose. COMMENT THE CODE A LITTLE: in that '// ' header NAME "
    "the cross-file location (FILE:LINE) you verified that makes each finding "
    "real - the size table, bounds-check or guard you followed - e.g. '// LINE "
    "84 reads 10B but packet 0x88 is fixed 10B in ProtocolParsingGeneral.cpp:"
    "152, so NOT OOB' (then you'd output 'No issues identified' instead), or "
    "'// LINE 135 quantity is uint32 off the wire, no cap before the multiply'. "
    "Keep it factual (FILE:LINE refs), not narrative, so it never reads as a "
    "vague 'could be' note."
)
# The IA reads the bug-bounty scope on every audit turn (appended once at load).
SYSTEM_PROMPT = SYSTEM_PROMPT + read_bounty() + "\n\n" + ACTIVE_DEFINES
# Where to look for a referenced class/header when it does not resolve by a
# relative include path.
# Scope of the class/symbol graph + caller/basename search. Mirrors
# SCAN_SEED_DIRS: the in-scope client-facing binaries (cli + login + gateway +
# game-server-alone) and the shared engine - NOT the Qt admin GUI (server/qt),
# build artefacts (server/build), or the internal-only master daemon.
SEARCH_DIRS = tuple(os.path.join(REPO_ROOT, p) for p in (
    "server/cli",
    "server/base",
    "server/crafting",
    "server/fight",
    "server/login",
    "server/gateway",
    "server/game-server-alone",
    "general/base",
    "general/fight",
))

# Cap the context we attach so a single oversized file can't blow the model's
# context window. Bytes, applied per related file and to the total.
MAX_RELATED_BYTES = 96000
MAX_RELATED_FILES = 24
# Per-attached-file byte cap so one hub header (e.g. GeneralStructures.hpp, 81
# classes) can't swallow the whole related-context budget - several dependency
# files each get a slice.
MAX_RELATED_PER_FILE = 20000
# How many class-DEFINITION files to pull for the classes THIS file uses (the
# 'file B' for 'A uses class B'). Bounded so a file touching many types stays
# within the model's context.
MAX_DEP_CLASSES = 12

# Reverse-dependency (CALLER) context. A file - above all a header - read in
# isolation hides the bug: the function BODIES live in the sibling .cpp (callees)
# and the untrusted input enters through the CALLERS (taint sources). We attach a
# bounded sample of call sites so the reviewer sees who feeds this code without
# having to guess to GREP. Kept small so a 33B model's context isn't blown; the
# model can still READ a full caller on demand.
MAX_CALLER_FILES = 8       # distinct caller files sampled
MAX_CALLER_PER_FILE = 4    # call-site lines kept per caller (spread, not one hot file)
MAX_CALLER_HITS = 24       # total call-site lines shown
MAX_CALLER_BYTES = 16000   # hard cap on the whole caller-hint block

# ---------------------------------------------------------------------------
# Local-LLM recall boosters
# ---------------------------------------------------------------------------
# Hot files: the highest-value remote surface, where a lazy 'No issues
# identified' from a small model costs the most recall. Matched against the
# repo-relative path. Packet handlers + the wire parser + the per-packet
# ClientEvents/ClientLoad/ClientHeavyLoad* dispatchers + the pre-auth login
# parser. These get an ADVERSARIAL re-audit when the first pass comes back clean.
HOT_FILE_RE = re.compile(
    r"(^|/)("
    r"ProtocolParsing(Input|General|Base)\.(cpp|hpp|h)"
    r"|ClientNetworkRead(Message|Query)\.cpp"
    r"|EventLoopClient(LoginSlave)?ProtocolParsing\.cpp"
    r"|ClientEvents/.*\.cpp"
    r"|ClientLoad/.*\.cpp"
    r"|ClientHeavyLoad.*\.cpp"
    r"|[A-Za-z]*ProtocolParsing[A-Za-z]*\.(cpp|hpp|h)"
    r")$")

# Adversarial preamble prepended to a second-pass audit of a hot file whose
# first pass returned 'No issues identified'. A 26B local model defaults to the
# safe clean-verdict; this reframe forces it to either produce a concrete bug
# OR justify the safety of every length/index/id/size it consumes - which
# itself surfaces findings the first pass dismissed.
ADVERSARIAL_PREAMBLE = (
    "ADVERSARIAL RE-AUDIT: the first pass on this file returned 'No issues "
    "identified'. This is a HOT FILE (a packet handler / wire parser / pre-auth "
    "path) where a clean verdict is the MOST likely to be a missed bug, not a "
    "real clean result. Assume there IS at least one exploitable vulnerability "
    "in the LIVE (non-Qt, non-dead-#ifdef) code and HUNT IT:\n"
    "- For EVERY length, index, id, size, offset or pointer the file reads off "
    "the wire, trace it to the sink and ask: is it bounded BEFORE use? Cite the "
    "guard FILE:LINE or report the missing guard as a finding.\n"
    "- For EVERY packet case 0xNN, compare the bytes the handler reads vs the "
    "parser's declared fixed size (ProtocolParsingGeneral.cpp:packetFixedSize) "
    "or the inner length of a 0xFE variable packet - a mismatch is an OOB.\n"
    "- For EVERY state transition (login -> character select -> in-game), ask "
    "whether a packet can be sent in the WRONG state and bypass an auth check.\n"
    "If after this you are CERTAIN the file is clean, you may output 'No issues "
    "identified' - but ONLY after naming, for each length/index/id consumed, "
    "the FILE:LINE guard that bounds it. A bare clean verdict without those "
    "guards is rejected.\n\n")

# ---------------------------------------------------------------------------
# Semantic static analysis: the Clang Static Analyzer, driven through clang-tidy
# with the CLI server's OWN compile flags (a generated compile_commands.json).
# This analyses EXACTLY the no-Qt binary - Qt #ifdef branches are preprocessed
# out, and types/macros/templates resolve for real - so it is the semantic-grade
# layer the regex graph cannot provide. Entirely best-effort: if clang-tidy is
# absent or the cmake configure fails, the analyzer layer is silently skipped and
# the LLM audit still runs.
# ---------------------------------------------------------------------------
CLANG_TIDY = shutil.which("clang-tidy")
# Compile DB generated out of the source tree (override for a shared cache).
COMPILE_DB_DIR = os.environ.get("CC_COMPILE_DB_DIR", "/tmp/cc-security-cli-compiledb")
CLI_CMAKE_SRC = os.path.join(REPO_ROOT, "server", "cli")
# Curated, LOW-NOISE, security-relevant checks: the path-sensitive Clang Static
# Analyzer (memory/null/uninit/leak/API-misuse) + a hand-picked bugprone subset.
# Style/noise checks (e.g. bugprone-narrowing-conversions, which fires on every
# uint8_t->char byte cast) are deliberately OFF - they flood the panel without
# finding remote bugs.
CLANG_TIDY_CHECKS = ",".join((
    "-*",
    "clang-analyzer-core.*",
    "clang-analyzer-cplusplus.*",
    "clang-analyzer-unix.*",
    "clang-analyzer-security.*",
    "bugprone-signed-char-misuse",
    "bugprone-misplaced-widening-cast",
    "bugprone-too-small-loop-variable",
    "bugprone-integer-division",
    "bugprone-suspicious-memory-comparison",
    "bugprone-not-null-terminated-result",
    "bugprone-undefined-memory-manipulation",
    "bugprone-sizeof-expression",
    "bugprone-misplaced-pointer-arithmetic-in-alloc",
    "bugprone-suspicious-realloc-usage",
))
TIDY_TIMEOUT = 240         # per-TU wall budget (analysis can be slow on big TUs)
TIDY_MAX_FINDINGS = 40     # cap analyzer lines fed into one audit

# Running audit notebook the model reads as context and appends to.
SECURITY_NOTES_FILE = os.path.join(HERE, "SECURITY-IA.md")
# The model wraps anything it wants persisted between these markers.
NOTE_RE = re.compile(r"<<<NOTES(.*?)NOTES>>>", re.DOTALL)


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

# Out of scope for THIS audit: the no-Qt CLI server is the target, so the Qt
# admin GUI is excluded entirely (operator-side, not the remote attack surface),
# along with in-source build artefacts and all client code. Never audited and
# never pulled in as context. (Files in scope that only touch Qt under #ifdef -
# e.g. NormalServerGlobal.cpp, Map_loader.cpp - stay IN: their Qt branch is
# compiled out of the CLI build, which is exactly what clang-tidy then analyses.)
EXCLUDED_DIRS = tuple(os.path.realpath(os.path.join(REPO_ROOT, p)) for p in (
    "server/qt",      # Qt admin GUI binary - not the CLI server
    "server/build",   # in-source CMake build artefacts
    "client",         # client side, not the server attack surface
))


def is_vendor(path):
    """True for a vendored third-party tree OR an out-of-scope tree (Qt GUI /
    build artefacts / client). Such files are never audited and never attached
    as context."""
    rp = os.path.realpath(path)
    return any(rp == d or rp.startswith(d + os.sep)
               for d in VENDOR_DIRS + EXCLUDED_DIRS)


def is_hot_file(path):
    """True for the highest-value remote-surface files (packet handlers, the
    wire parser, the per-packet dispatchers, the pre-auth login parser). A clean
    first-pass verdict on these gets an ADVERSARIAL re-audit because a small
    local LLM defaults to 'No issues identified' and that is where the most
    recall is lost."""
    rel = os.path.relpath(path, REPO_ROOT)
    return bool(HOT_FILE_RE.search(rel))


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


def related_files(path, depth=2):
    """Repo-local files referenced by `path` via its quoted #includes, followed
    TRANSITIVELY up to `depth` hops (depth=1 = direct includes only, the old
    behaviour). Deeper hops pull the dependency-of-a-dependency that a single
    hop misses - e.g. a packet handler -> Client.hpp -> the ProtocolParsing
    headers that hold the packetFixedSize table; that table is exactly what
    proves a fixed-size handler can't read OOB. Breadth-first, deduped by
    realpath, vendor trees pruned, capped at MAX_RELATED_FILES total."""
    repo_root = os.path.realpath(REPO_ROOT)
    out = []
    seen = {os.path.realpath(path)}
    frontier = [path]
    hop = 0
    while frontier and hop < depth and len(out) < MAX_RELATED_FILES:
        hop += 1
        nxt = []
        for fpath in frontier:
            base = os.path.dirname(fpath)
            try:
                text = open(fpath, "r", errors="replace").read()
            except OSError:
                continue
            for inc in INCLUDE_RE.findall(text):
                cand = os.path.realpath(os.path.join(base, inc))
                if not (cand.startswith(repo_root + os.sep) and os.path.isfile(cand)):
                    cand = find_by_basename(os.path.basename(inc))
                # Skip non-repo, missing, vendored (do-not-audit, huge) deps.
                if not cand or cand in seen or is_vendor(cand):
                    continue
                seen.add(cand)
                out.append(cand)
                nxt.append(cand)
                if len(out) >= MAX_RELATED_FILES:
                    break
            if len(out) >= MAX_RELATED_FILES:
                break
        frontier = nxt
    return out


def counterpart_files(path):
    """The sibling implementation/header of `path` (same dir + basename, other
    extension). Auditing a .hpp in isolation hides the bug - the function BODIES
    (where the overflow/OOB actually lives) sit in the .cpp, which the header does
    not #include, so related_files() misses it. Returns existing repo-local,
    non-vendor counterparts (callee bodies for a header, declarations for a .cpp)."""
    repo_root = os.path.realpath(REPO_ROOT)
    base = os.path.dirname(path)
    stem = os.path.splitext(os.path.basename(path))[0]
    if path.endswith((".h", ".hpp", ".hxx")):
        exts = (".cpp", ".cc", ".cxx", ".c")
    else:
        exts = (".hpp", ".h", ".hxx")
    out = []
    for ext in exts:
        cand = os.path.realpath(os.path.join(base, stem + ext))
        if cand != path and cand.startswith(repo_root + os.sep) \
                and os.path.isfile(cand) and not is_vendor(cand):
            out.append(cand)
    return out


# ---------------------------------------------------------------------------
# Class symbol graph: follow dependencies BY CLASS, not just by #include.
# 'A uses class B' -> pull B's defining file; 'C uses class A' -> C is a caller.
# A pure-Python index (no libclang/ctags dependency: clang.cindex is not
# importable here and the clang CLI would need the full Qt6 include flags +
# a compile DB, which this no-root-CMake repo does not provide). Good enough:
# the project names classes in PascalCase and one class is defined per 'class X
# {' / 'struct X {' site, which a regex resolves reliably.
# ---------------------------------------------------------------------------
# A class/struct DEFINITION (has a body): 'class X {' or 'class X : Base {',
# at line start (after indentation). A forward decl 'class X;' is excluded
# because [^;{]* stops at the ';' before any '{'/':' is seen. Leading ALL-CAPS
# tokens are skipped so an export macro ('class DLL_PUBLIC Foo') does not steal
# the name; the (?:...)* is greedy but backtracks, so an all-caps struct name
# ('struct DDOS {') still resolves (zero macros matched).
_CLASSDEF_RE = re.compile(
    r'^[ \t]*(?:class|struct)[ \t]+(?:[A-Z][A-Z0-9_]*[ \t]+)*([A-Za-z_]\w*)\b'
    r'[^;{]*[{:]', re.MULTILINE)

_symbol_index = None          # (def_map {class: deffile}, file_classes {file: {class}})
_SYMBOL_LOCK = threading.Lock()


def symbol_index():
    """Repo class/struct symbol graph over SEARCH_DIRS, built once and cached:
    (def_map {class_name: defining_file}, file_classes {defining_file: {class}}).
    A header definition wins over a .cpp one; forward decls are skipped. Lets us
    resolve a class NAME to the file that defines it (a class name is not always
    the file basename - GeneralStructures.hpp alone defines 81)."""
    global _symbol_index
    if _symbol_index is None:
        with _SYMBOL_LOCK:
            if _symbol_index is None:
                def_map = {}
                file_classes = {}   # keyed by realpath for O(1) lookup by callers
                for d in SEARCH_DIRS:
                    for root, _dirs, names in os.walk(d):
                        for n in sorted(names):
                            if not n.endswith(SOURCE_EXT):
                                continue
                            fp = os.path.realpath(os.path.join(root, n))
                            try:
                                text = open(fp, "r", errors="replace").read()
                            except OSError:
                                continue
                            is_header = n.endswith((".h", ".hpp", ".hxx"))
                            for m in _CLASSDEF_RE.finditer(text):
                                cls = m.group(1)
                                prev = def_map.get(cls)
                                if prev is None or (is_header and not prev.endswith(
                                        (".h", ".hpp", ".hxx"))):
                                    def_map[cls] = fp
                                file_classes.setdefault(fp, set()).add(cls)
                _symbol_index = (def_map, file_classes)
    return _symbol_index


_IDENT_RE = re.compile(r'\b[A-Za-z_]\w*\b')


def classes_used_by(path):
    """Definition files of the repo classes that `path` references - the 'file B'
    for 'A uses class B'. Intersects the file's identifiers with the class index,
    drops classes this file defines itself, and returns deduped def files in
    first-use order, capped to MAX_DEP_CLASSES."""
    def_map, file_classes = symbol_index()
    rp = os.path.realpath(path)
    own = file_classes.get(rp, set())
    try:
        content = open(path, "r", errors="replace").read()
    except OSError:
        return []
    out = []
    seen_files = set()
    seen_cls = set()
    for m in _IDENT_RE.finditer(content):
        cls = m.group(0)
        if cls in seen_cls or cls in own:
            continue
        seen_cls.add(cls)
        deffile = def_map.get(cls)
        if not deffile:
            continue
        drp = os.path.realpath(deffile)
        if drp == rp or drp in seen_files:
            continue
        seen_files.add(drp)
        out.append(deffile)
        if len(out) >= MAX_DEP_CLASSES:
            break
    return out


def classes_defined_in(path):
    """The class names `path` itself defines (for the reverse 'who uses class A'
    grep). Empty for free-function / split-implementation files."""
    _def_map, file_classes = symbol_index()
    return sorted(file_classes.get(os.path.realpath(path), set()))


# A method/function DEFINED in a .cpp: a 'Class::method(' opening at the start of
# a line (definitions sit at column 0; calls are indented inside a body). Used to
# find the real callers of split-implementation files (CC spreads Client's body
# across ClientHeavyLoad*.cpp etc., none named after the class - so the stem alone
# finds nobody).
_DEF_RE = re.compile(r'^[A-Za-z_][\w:<>,&*\s]*?\b([A-Za-z_]\w+)::([A-Za-z_]\w+)\s*\(',
                     re.MULTILINE)
MAX_CALLER_SYMBOLS = 12


def caller_snippets(path):
    """Reverse-dependency hint: a bounded sample of CALL SITES that feed `path`,
    so the reviewer can see whether untrusted TCP input reaches this code - the
    taint source lives in the CALLERS, never in the file itself. Two greps over
    the audit scope (SEARCH_DIRS):
      1. WHOLE-WORD references to the class names this file DEFINES ('who uses
         class A') - falls back to the basename stem when the file defines no
         class (free-function / split-implementation files);
      2. fixed-string references to the fully-qualified methods this file DEFINES
         ('Class::method', which finds callers of split .cpp files not named
         after their class) and to #includes of its basename.
    Returns deduped 'rel/path:line: <code>' lines, spread across callers and
    capped. The file itself and its counterpart (definitions, not callers) are
    dropped. '' when nothing references it."""
    base = os.path.basename(path)
    stem = os.path.splitext(base)[0]
    skip = {os.path.realpath(path)}
    for c in counterpart_files(path):
        skip.add(os.path.realpath(c))

    # (1) Whole-word references to the classes this file defines. -w so 'Map'
    # does not also match 'Mapper'/'MapItem'. Stem fallback when none.
    word_patterns = classes_defined_in(path)[:MAX_CALLER_SYMBOLS] or [stem]

    # (2) Fixed-string method/#include references.
    try:
        content = open(path, "r", errors="replace").read()
    except OSError:
        content = ""
    fixed_patterns = [base]
    seen_sym = set()
    for m in _DEF_RE.finditer(content):
        sym = m.group(1) + "::" + m.group(2)
        if sym not in seen_sym and sym != stem + "::" + stem:
            seen_sym.add(sym)
            fixed_patterns.append(sym)
        if len(fixed_patterns) >= MAX_CALLER_SYMBOLS:
            break

    incl = ["--include=*.cpp", "--include=*.h", "--include=*.hpp",
            "--include=*.cc", "--include=*.cxx"]
    raw = []
    for flags, pats in (("-rnIwF", word_patterns), ("-rnIF", fixed_patterns)):
        if not pats:
            continue
        args = ["grep", flags] + incl
        for pat in pats:
            args += ["-e", pat]
        args += ["--", *SEARCH_DIRS]
        try:
            raw += subprocess.run(args, capture_output=True, text=True,
                                  timeout=60).stdout.splitlines()
        except (OSError, subprocess.SubprocessError) as exc:
            sys.stderr.write("    [caller-scan] grep failed: %s\n" % exc)

    per_file = {}
    hits = []
    total_bytes = 0
    seen_lines = set()
    for line in raw:
        fp = line.split(":", 1)[0]
        rfp = os.path.realpath(fp)
        if rfp in skip:
            continue
        if rfp not in per_file and len(per_file) >= MAX_CALLER_FILES:
            continue
        if per_file.get(rfp, 0) >= MAX_CALLER_PER_FILE:
            continue
        rel_line = line[len(REPO_ROOT) + 1:] \
            if line.startswith(REPO_ROOT + os.sep) else line
        if rel_line in seen_lines:
            continue
        if total_bytes + len(rel_line) + 1 > MAX_CALLER_BYTES:
            break
        seen_lines.add(rel_line)
        per_file[rfp] = per_file.get(rfp, 0) + 1
        total_bytes += len(rel_line) + 1
        hits.append(rel_line)
        if len(hits) >= MAX_CALLER_HITS:
            break
    return "\n".join(hits)


# ---------------------------------------------------------------------------
# Clang Static Analyzer integration (semantic, path-sensitive).
# ---------------------------------------------------------------------------
_compile_db = None          # path to compile_commands.json, or "" if unavailable
_cli_tus = None             # frozenset of realpath TUs in the CLI server build
_compile_lock = threading.Lock()
_tidy_cache = {}            # {target_tu: findings_block}
_tidy_locks = {}            # {target_tu: Lock} so the same TU is analysed once
_tidy_master = threading.Lock()


def compile_db():
    """Path to a compile_commands.json for the CLI server, generated once via
    cmake (out of the source tree) and cached on disk. '' if clang-tidy is
    missing or the configure fails -> the analyzer layer is then skipped."""
    global _compile_db
    if _compile_db is None:
        with _compile_lock:
            if _compile_db is None:
                _compile_db = ""
                cdb = os.path.join(COMPILE_DB_DIR, "compile_commands.json")
                if CLANG_TIDY and not os.path.isfile(cdb):
                    sys.stderr.write("%s [analyzer] generating CLI compile DB "
                                     "(cmake configure %s)...\n"
                                     % (_ts(), CLI_CMAKE_SRC))
                    try:
                        r = subprocess.run(
                            ["cmake", "-S", CLI_CMAKE_SRC, "-B", COMPILE_DB_DIR,
                             "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"],
                            capture_output=True, text=True, timeout=600)
                        if r.returncode != 0:
                            sys.stderr.write("%s [analyzer] cmake configure failed "
                                             "(rc=%d); static analysis disabled\n"
                                             % (_ts(), r.returncode))
                    except (OSError, subprocess.SubprocessError) as exc:
                        sys.stderr.write("%s [analyzer] cmake configure error: %s; "
                                         "static analysis disabled\n" % (_ts(), exc))
                if CLANG_TIDY and os.path.isfile(cdb):
                    _compile_db = cdb
    return _compile_db


def cli_translation_units():
    """Realpaths of the .cpp TUs compiled into the CLI server (from the compile
    DB) - the authoritative in-scope set for the no-Qt binary. Empty if there is
    no compile DB."""
    global _cli_tus
    if _cli_tus is None:
        tus = set()
        cdb = compile_db()
        if cdb:
            try:
                for e in json.load(open(cdb)):
                    f = e.get("file")
                    if f:
                        tus.add(os.path.realpath(f))
            except (OSError, ValueError) as exc:
                sys.stderr.write("%s [analyzer] could not read compile DB: %s\n"
                                 % (_ts(), exc))
        _cli_tus = frozenset(tus)
    return _cli_tus


def _tidy_target(path):
    """The .cpp TU to analyse for `path`: the file itself if it is a CLI TU, else
    its .cpp counterpart if THAT is a TU (so a header is checked via its
    implementation). None if neither is in the CLI build."""
    tus = cli_translation_units()
    rp = os.path.realpath(path)
    if rp in tus:
        return rp
    for c in counterpart_files(path):
        rc = os.path.realpath(c)
        if rc in tus:
            return rc
    return None


def _run_clang_tidy(target):
    """Run clang-tidy on one TU; return its MAIN-FILE findings as a capped block."""
    try:
        res = subprocess.run(
            [CLANG_TIDY, "-p", COMPILE_DB_DIR, "--quiet",
             "--header-filter=$^",          # display only main-file diagnostics
             "--checks=" + CLANG_TIDY_CHECKS, target],
            capture_output=True, text=True, timeout=TIDY_TIMEOUT)
    except subprocess.TimeoutExpired:
        sys.stderr.write("    [analyzer] clang-tidy timed out on %s\n"
                         % os.path.relpath(target, REPO_ROOT))
        return ""
    except (OSError, subprocess.SubprocessError) as exc:
        sys.stderr.write("    [analyzer] clang-tidy error: %s\n" % exc)
        return ""
    prefix = target + ":"
    rel = os.path.relpath(target, REPO_ROOT)
    lines = []
    for ln in res.stdout.splitlines():
        if ln.startswith(prefix) and (": warning:" in ln or ": error:" in ln):
            lines.append(rel + ln[len(target):])
            if len(lines) >= TIDY_MAX_FINDINGS:
                lines.append("... [more analyzer findings omitted]")
                break
    return "\n".join(lines)


def clang_tidy_findings(path):
    """Clang Static Analyzer findings located IN the audited file (for a header,
    surfaced from its .cpp counterpart), as a capped text block. Cached per
    target TU; thread-safe with per-TU locking so the panel's concurrent members
    never analyse the same TU twice. '' when clang-tidy/DB is absent, the file is
    out of CLI scope, or it is clean."""
    if not CLANG_TIDY:
        return ""
    target = _tidy_target(path)
    if not target:
        return ""
    with _tidy_master:
        if target in _tidy_cache:
            return _tidy_cache[target]
        lk = _tidy_locks.setdefault(target, threading.Lock())
    with lk:                                   # serialises only the SAME TU
        with _tidy_master:
            if target in _tidy_cache:
                return _tidy_cache[target]
        block = _run_clang_tidy(target)
        with _tidy_master:
            _tidy_cache[target] = block
        return block


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
        with _NOTES_LOCK:
            header = "" if os.path.exists(SECURITY_NOTES_FILE) else (
                "# SECURITY-IA.md - project-wide security audit notebook\n"
                "# Trust boundaries, untrusted-input entry points, shared state, "
                "cross-file invariants.\n\n"
            )
            with open(SECURITY_NOTES_FILE, "a") as fh:
                fh.write("%s%s\n" % (header, block))


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


def tool_analyze(arg):
    """ANALYZE <path>: Clang Static Analyzer findings for a repo .cpp (or a
    header, via its .cpp counterpart), built with the CLI server's own no-Qt
    flags. Lets the model pull semantic analysis for a callee/caller on demand."""
    if not CLANG_TIDY:
        return "ANALYZE: clang-tidy not available on this host"
    repo_root = os.path.realpath(REPO_ROOT)
    cand = os.path.realpath(os.path.join(REPO_ROOT, arg))
    if not (cand.startswith(repo_root + os.sep) and os.path.isfile(cand)):
        cand = find_by_basename(os.path.basename(arg))
    if not cand or not os.path.isfile(cand):
        return "ANALYZE: not found in repo: %s" % arg
    out = clang_tidy_findings(cand)
    return out if out else ("ANALYZE: no static-analyzer findings (clean, or "
                            "not part of the CLI-server build) for %s" % arg)


def parse_tool(answer):
    """If the reply is a single READ/GREP/ANALYZE tool call, return (name, arg)."""
    stripped = answer.strip()
    first = stripped.splitlines()[0].strip() if stripped else ""
    for name in ("READ", "GREP", "ANALYZE"):
        prefix = name + " "
        if first.startswith(prefix) and len(stripped) - len(first) < 40:
            return name, first[len(prefix):].strip()
    return None


def _finalize_answer(rel, answer, persist=True):
    """Strip the NOTES block (persisting it unless persist=False) and fix repo
    paths in a model answer. Shared by analyze() and its degenerate retry. Panel
    round-1/discussion members pass persist=False so only the synthesised
    consensus writes the shared notebook (and threads don't race on it)."""
    if persist:
        for block in NOTE_RE.findall(answer):
            append_notes(rel, block)
    answer = NOTE_RE.sub("", answer).strip()
    return fix_paths(answer)


def analyze(path, spec=None, persist_notes=True, preamble="", adversarial=False):
    """Agentic audit of one file by ONE model (spec; None = configured backend).
    `preamble` is prepended to the prompt (the panel uses it to announce the
    reviewer + round). The NOTES block is stripped, and persisted unless
    persist_notes=False (panel members defer the notebook write to synthesis).
    `adversarial` prepends ADVERSARIAL_PREAMBLE (red-team re-audit of a hot
    file whose first pass came back clean)."""
    rel = os.path.relpath(path, REPO_ROOT)
    try:
        content = open(path, "r", errors="replace").read()
    except OSError as exc:
        return "[read error] %s" % exc

    if adversarial:
        preamble = ADVERSARIAL_PREAMBLE + preamble

    context = []
    budget = MAX_RELATED_BYTES
    # Build the related-file list by PRIORITY, deduped by realpath:
    #   1. COUNTERPART  - this file's own sibling .cpp/.hpp (the called code /
    #      declarations); matters most, so it gets the budget first.
    #   2. USED-CLASS DEFINITION - for every repo class THIS file uses, the file
    #      that DEFINES that class (the class graph: 'A uses class B' -> file B).
    #      A class name is not always the basename, so this catches deps that
    #      #include-following alone misses.
    #   3. RELATED FILE - the forward #includes (extra: enum/typedef/free-function
    #      headers that define no class).
    ordered = []
    seen_ctx = set()
    for cpath in counterpart_files(path):
        rp = os.path.realpath(cpath)
        if rp not in seen_ctx:
            seen_ctx.add(rp)
            ordered.append((
                "COUNTERPART (implementation/header of the MAIN FILE - the "
                "CALLED code; read to judge behaviour, do not audit)", cpath))
    for dpath in classes_used_by(path):
        rp = os.path.realpath(dpath)
        if rp not in seen_ctx:
            seen_ctx.add(rp)
            ordered.append((
                "USED-CLASS DEFINITION (a class this file uses is defined here; "
                "read to judge how it is used, do not audit)", dpath))
    for rpath in related_files(path):
        rp = os.path.realpath(rpath)
        if rp not in seen_ctx:
            seen_ctx.add(rp)
            ordered.append(("RELATED FILE (dependency hint, do not audit)", rpath))
    for label, rpath in ordered:
        if budget <= 0:
            break
        try:
            rtext = open(rpath, "r", errors="replace").read()
        except OSError:
            rtext = ""
        # Per-file cap so one hub header can't eat the whole budget.
        rtext = rtext[:min(budget, MAX_RELATED_PER_FILE)]
        budget -= len(rtext)
        rrel = os.path.relpath(rpath, REPO_ROOT)
        context.append("=== %s: %s ===\n%s" % (label, rrel, rtext))
    # CALLERS: a bounded sample of who feeds this file (the taint source - where
    # untrusted TCP input may reach it). Own byte budget; never an audit target.
    callers = caller_snippets(path)
    if callers:
        context.append(
            "=== CALLERS of %s (taint-source hint - where remote input may "
            "reach this code; do NOT audit, READ/GREP a caller for the full "
            "path) ===\n%s" % (rel, callers))
    # Semantic static-analyzer findings (Clang Static Analyzer, compiled with the
    # CLI server's own no-Qt flags). High recall, NOT pre-filtered for remote
    # reachability - the model must verify each against the TCP taint path.
    analyzer = clang_tidy_findings(path)
    if analyzer:
        context.append(
            "=== STATIC ANALYZER (clang, CLI-server build) findings in this "
            "file - each is a CANDIDATE only: confirm it is reachable from "
            "untrusted TCP input AND exploitable before reporting; many will "
            "not be. Do not blindly echo them ===\n%s" % analyzer)
    context_block = ("\n\n".join(context) + "\n\n") if context else ""

    notes = read_notes()
    notes_block = (
        "=== SECURITY-IA.md (project notebook so far) ===\n%s\n\n" % notes
        if notes.strip()
        else ""
    )

    user = (
        "%s%s%sAudit the MAIN FILE below for exploitable security "
        "vulnerabilities. Report findings only for the MAIN FILE. The "
        "COUNTERPART (.cpp/.hpp), USED-CLASS DEFINITION and CALLER hints above "
        "show the called code, the types it uses and where remote input may "
        "enter - USE them, and confirm or dismiss every STATIC ANALYZER "
        "candidate against the TCP taint path. Before concluding 'No "
        "issues identified' on any function that consumes a length, index, id, "
        "size, offset or pointer, TRACE whether untrusted TCP input can reach "
        "it: follow a CALLER, GREP for more call sites, or READ the "
        "implementation. A header's declarations alone are NOT enough to clear "
        "it.\n\n"
        "=== MAIN FILE: %s ===\n\n%s"
        % (preamble, notes_block, context_block, rel, content)
    )
    messages = [
        {"role": "system", "content": SYSTEM_PROMPT},
        {"role": "user", "content": user},
    ]

    answer = ""
    step = 0
    while step < SCAN_MAX_STEPS:
        step += 1
        answer = chat_with(spec, messages)
        tool = parse_tool(answer)
        if tool is None:
            break
        name, arg = tool
        if name == "READ":
            result = tool_read(arg)
        elif name == "GREP":
            result = tool_grep(arg)
        else:
            result = tool_analyze(arg)
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
        answer = chat_with(spec, messages)

    answer = _finalize_answer(rel, answer, persist_notes)
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
        answer = _finalize_answer(rel, chat_with(spec, messages), persist_notes)
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


# ===========================================================================
# Collaborative panel: several local models (< CC_PANEL_MAX_B params) audit the
# SAME file at once, then discuss for CC_PANEL_ROUNDS rounds - each sees the
# others' findings and may CHANGE ITS VIEW - and a lead synthesises the team
# consensus. Improves recall + precision over any single small model.
# ===========================================================================

# Comma-separated model specs, or "auto"/"all" to use every installed Ollama
# model under CC_PANEL_MAX_B billion params. Empty -> single-model (unchanged).
CC_IA_PANEL = os.environ.get("CC_IA_PANEL", "").strip()
try:
    CC_PANEL_MAX_B = float(os.environ.get("CC_PANEL_MAX_B", "35") or "35")
except ValueError:
    CC_PANEL_MAX_B = 35.0
try:
    CC_PANEL_MAX_MEMBERS = int(os.environ.get("CC_PANEL_MAX_MEMBERS", "4") or "4")
except ValueError:
    CC_PANEL_MAX_MEMBERS = 4
# Total speaking rounds: round 1 = independent findings, rounds 2..N = team
# discussion (each model is told "round X/N" and may revise). Default 10.
try:
    CC_PANEL_ROUNDS = int(os.environ.get("CC_PANEL_ROUNDS", "10") or "10")
except ValueError:
    CC_PANEL_ROUNDS = 10
if CC_PANEL_ROUNDS < 1:
    CC_PANEL_ROUNDS = 1
# Which member synthesises consensus (model spec); default claude-if-present-
# else-first. May carry an '@host:port' backend pin like any panel member.
CC_PANEL_LEAD = _register_model_spec(os.environ.get("CC_PANEL_LEAD", "").strip())

# Shared framing so every model behaves as a fair, constructive teammate.
TEAM_FRAMING = (
    "You are one reviewer in a PANEL auditing the same file together as a TEAM. "
    "Speak constructively and fairly: build on good points, concede when a peer "
    "is right, and rebut only with concrete reasoning grounded in the code. The "
    "goal is the best SHARED diagnosis, not winning.")
# Cap on MAIN FILE bytes echoed into each discussion / synthesis turn. Must stay
# >= the largest audited file: round 1 audits the FULL file, so a smaller cap
# would hide a real tail-line finding from the peers/lead and they would drop it.
PANEL_FILE_CAP = 200000


def _param_billions(model):
    """Billions of parameters for an /api/tags model entry, from
    details.parameter_size (e.g. '32.8B'), else a ':NNb' tag suffix. None if
    unknown."""
    det = model.get("details") or {}
    ps = det.get("parameter_size")
    if ps:
        m = re.search(r"([\d.]+)\s*([BM])", ps, re.I)
        if m:
            v = float(m.group(1))
            return v / 1000.0 if m.group(2).upper() == "M" else v
    m = re.search(r":(\d+(?:\.\d+)?)b\b", model.get("name", ""), re.I)
    return float(m.group(1)) if m else None


def ollama_models(max_b=None):
    """Installed Ollama models across ALL configured backends, as [(name,
    size_b)] (deduped, first backend reporting a name wins). When max_b is set,
    keep only models whose KNOWN parameter size is < max_b (unknown-size models
    are excluded - conservative). [] if no backend is reachable."""
    seen = {}
    order = []
    for url, _pins in ollama_backends():
        try:
            with urllib.request.urlopen(url + "/api/tags", timeout=10) as resp:
                info = json.loads(resp.read().decode("utf-8", "replace"))
        except (urllib.error.URLError, OSError, ValueError) as exc:
            sys.stderr.write("%s [panel] backend %s not listable: %s\n"
                             % (_ts(), url, exc))
            continue
        for m in info.get("models") or []:
            name = m.get("name") or m.get("model")
            if not name or name in seen:
                continue
            size_b = _param_billions(m)
            if max_b is not None and (size_b is None or size_b >= max_b):
                continue
            seen[name] = size_b
            order.append(name)
    return [(n, seen[n]) for n in order]


def resolve_panel():
    """Ordered, de-duplicated list of model specs in the collaborative panel.
    'auto'/'all' expands to installed Ollama models < CC_PANEL_MAX_B (capped to
    CC_PANEL_MAX_MEMBERS), plus 'claude' when a credential is present. Claude
    specs are dropped (with a warning) if no credential is set."""
    specs = []
    for tok in CC_IA_PANEL.split(","):
        tok = tok.strip()
        if not tok:
            continue
        if tok.lower() in ("auto", "all"):
            picked = [name for name, _ in ollama_models(CC_PANEL_MAX_B)]
            if 0 < CC_PANEL_MAX_MEMBERS < len(picked):
                dropped = picked[CC_PANEL_MAX_MEMBERS:]
                picked = picked[:CC_PANEL_MAX_MEMBERS]
                sys.stderr.write("%s [panel] %d models < %gB; using %d, skipping: "
                                 "%s\n" % (_ts(), len(picked) + len(dropped),
                                           CC_PANEL_MAX_B, len(picked),
                                           ", ".join(dropped)))
            specs.extend(picked)
            if _claude_has_creds():
                specs.append("claude")
        else:
            # Register an optional '@host:port' backend pin and keep the bare
            # name (so it stays a clean dict key / display label downstream).
            specs.append(_register_model_spec(tok))
    seen = set()
    out = []
    for s in specs:
        if s not in seen:
            seen.add(s)
            out.append(s)
    if not _claude_has_creds():
        kept = [s for s in out if not (s == "claude" or s.startswith("claude:"))]
        if len(kept) != len(out):
            sys.stderr.write("%s [panel] dropping claude member(s): no credential "
                             "set\n" % _ts())
        out = kept
    return out


def _panel_lead(specs):
    """Pick the synthesiser: explicit CC_PANEL_LEAD, else claude if present, else
    the first member."""
    if CC_PANEL_LEAD:
        return CC_PANEL_LEAD
    for s in specs:
        if s == "claude" or s.startswith("claude:"):
            return s
    return specs[0] if specs else None


def _panel_map(fn, specs):
    """Run fn(spec) for every spec AT THE SAME TIME (one thread each) and return
    {spec: result} in the original spec order. A spec whose call raises
    contributes '' so the panel carries on with the rest."""
    results = {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, len(specs))) as ex:
        futs = {ex.submit(fn, s): s for s in specs}
        for fut in concurrent.futures.as_completed(futs):
            s = futs[fut]
            try:
                results[s] = fut.result()
            except Exception as exc:
                sys.stderr.write("    [panel] %s errored: %s\n" % (s, exc))
                results[s] = ""
    return {s: results.get(s, "") for s in specs}


def _panel_digest(positions, exclude=None):
    """Labelled concatenation of each member's current findings, for sharing."""
    parts = []
    for spec, text in positions.items():
        if spec == exclude:
            continue
        body = (text or "").strip() or "(no findings reported)"
        parts.append("----- reviewer %s says:\n%s" % (spec, body))
    return "\n\n".join(parts)


def _line_signature(text):
    """The set of MAIN-FILE line numbers a position flags (empty = 'no issues').
    Reviewers that share a signature share a 'vision' -> the same work group."""
    nums = set()
    for m in re.finditer(r'\bLINE\s+(\d+)\b', text or "", re.I):
        nums.add(int(m.group(1)))
    return frozenset(nums)


def _work_groups(positions):
    """Cluster reviewers by shared signature into work groups, largest first.
    A group of one = a reviewer standing alone on a unique view."""
    groups = {}
    for spec, text in positions.items():
        groups.setdefault(_line_signature(text), []).append(spec)
    return sorted(groups.items(), key=lambda kv: (-len(kv[1]), sorted(kv[1])))


def _groups_blurb(positions):
    """Model/operator-readable description of the current work groups."""
    out = []
    i = 0
    for sig, members in _work_groups(positions):
        i += 1
        where = ("lines " + ", ".join(str(n) for n in sorted(sig))) if sig \
            else "no issues"
        alone = " (alone)" if len(members) == 1 else ""
        out.append("work-group %d%s: {%s} -> %s"
                   % (i, alone, ", ".join(members), where))
    return "\n".join(out)


def _panel_signatures(positions):
    """{spec: signature} for change detection between rounds."""
    return {spec: _line_signature(text) for spec, text in positions.items()}


def _panel_revise_one(path, content, round_no, total, positions, spec):
    """One discussion turn: member `spec` re-examines the file plus the panel's
    current work groups and may change its view (switch group / stand alone).
    Single turn - context is in the prompt."""
    rel = os.path.relpath(path, REPO_ROOT)
    mine = (positions.get(spec) or "").strip() or "(you reported nothing last round)"
    peers = _panel_digest(positions, exclude=spec)
    blurb = _groups_blurb(positions)
    user = (
        "%s\n\nRound %d/%d. The panel currently splits into these WORK GROUPS "
        "(reviewers who share a view):\n%s\n\nRe-examine %s (below) and the "
        "findings, then decide where you stand: STAY in your group, SWITCH to "
        "another group if its reasoning convinced you, or STAND ALONE with your "
        "own view - whatever the code actually supports. React constructively: "
        "KEEP what holds up, DROP what a peer convincingly rebutted or the file "
        "does not support, ADD a real issue a peer surfaced that you verified, "
        "and FIX wrong line numbers. If you have nothing to change and agree "
        "with your group, simply restate your position. Output the mandatory "
        "format only (one 'LINE <n>: <input> -> <consequence>' per real finding, "
        "or 'No issues identified').\n\n"
        "=== MAIN FILE: %s ===\n%s\n\n=== YOUR FINDINGS ===\n%s\n\n"
        "=== PEER FINDINGS ===\n%s"
        % (TEAM_FRAMING, round_no, total, blurb, rel, rel,
           content[:PANEL_FILE_CAP], mine, peers))
    messages = [{"role": "system", "content": SYSTEM_PROMPT},
                {"role": "user", "content": user}]
    return _finalize_answer(rel, chat_with(spec, messages), persist=False)


def _panel_synthesize(path, specs, positions):
    """Lead member merges the panel's final positions into one consensus answer
    (the only step that writes the shared notebook)."""
    rel = os.path.relpath(path, REPO_ROOT)
    try:
        content = open(path, "r", errors="replace").read()
    except OSError:
        content = ""
    lead = _panel_lead(specs)
    digest = _panel_digest(positions)
    user = (
        "You are the lead of a security review panel for %s. The team has "
        "finished discussing. Below is the file and every reviewer's FINAL "
        "position. Produce the CONSENSUS for the MAIN FILE: include a finding "
        "only if it is real and survived scrutiny (a clear majority, or one "
        "well-argued reviewer no one rebutted, verifiable in the code); drop "
        "rebutted / out-of-scope / duplicate ones; merge duplicates and fix line "
        "numbers. Output the mandatory format only (one 'LINE <n>: <input> -> "
        "<consequence>' per consensus finding, or 'No issues identified').\n\n"
        "=== MAIN FILE: %s ===\n%s\n\n=== PANEL FINAL POSITIONS ===\n%s"
        % (rel, rel, content[:PANEL_FILE_CAP], digest))
    messages = [{"role": "system", "content": SYSTEM_PROMPT},
                {"role": "user", "content": user}]
    return _finalize_answer(rel, chat_with(lead, messages))


def panel_analyze(path, specs, adversarial=False):
    """Collaborative multi-model audit of one file. Returns the consensus answer
    in the SAME format as analyze() so the scan pipeline is unchanged.
    Round 1: every member audits independently, at the same time. Rounds 2..N:
    each member sees its peers and may change its view (constructive team work).
    Finally the lead synthesises the consensus.
    `adversarial` prepends ADVERSARIAL_PREAMBLE to round 1 (red-team re-audit
    of a hot file that came back clean on the first pass)."""
    total = CC_PANEL_ROUNDS
    try:
        content = open(path, "r", errors="replace").read()
    except OSError:
        content = ""
    sys.stderr.write("    [panel] %d reviewers x up to %d rounds: %s\n"
                     % (len(specs), total, ", ".join(specs)))

    # Round 1/N: independent audits, concurrently; defer notebook writes.
    sys.stderr.write("    [panel] round 1/%d (independent findings)\n" % total)
    r1_preamble = "%s\n\nRound 1/%d: present your initial independent findings.\n\n" \
        % (TEAM_FRAMING, total)
    if adversarial:
        r1_preamble = ADVERSARIAL_PREAMBLE + r1_preamble
    preamble = r1_preamble
    positions = _panel_map(
        functools.partial(analyze, path, persist_notes=False, preamble=preamble),
        specs)
    sys.stderr.write("    [panel] work groups after round 1:\n%s\n"
                     % _groups_blurb(positions))
    prev_sig = _panel_signatures(positions)

    # Rounds 2..N: team discussion; a member may switch group or stand alone.
    # Stop early on consensus (one work group) or when nobody changes view.
    r = 1
    while r < total:
        r += 1
        sys.stderr.write("    [panel] round %d/%d (discussion)\n" % (r, total))
        positions = _panel_map(
            functools.partial(_panel_revise_one, path, content, r, total, positions),
            specs)
        groups = _work_groups(positions)
        sys.stderr.write("    [panel] work groups:\n%s\n" % _groups_blurb(positions))
        sig = _panel_signatures(positions)
        if len(groups) <= 1:
            sys.stderr.write("    [panel] consensus (one work group) at round "
                             "%d/%d -> stopping early\n" % (r, total))
            break
        if sig == prev_sig:
            sys.stderr.write("    [panel] no reviewer changed view at round %d/%d "
                             "-> nothing more to say, stopping early\n" % (r, total))
            break
        prev_sig = sig

    # Synthesis: lead merges into the team consensus (persists notes once). If
    # the lead model is unreachable (Ollama down / Claude 4xx / retries spent),
    # fall back to the largest work group's view instead of letting the error
    # propagate and discard the whole file's multi-round panel work.
    try:
        return _panel_synthesize(path, specs, positions)
    except Exception as exc:
        sys.stderr.write("    [panel] synthesis failed (%s); falling back to the "
                         "largest work group\n" % exc)
        rel = os.path.relpath(path, REPO_ROOT)
        for _sig, members in _work_groups(positions):
            for spec in members:
                ans = (positions.get(spec) or "").strip()
                if ans:
                    return _finalize_answer(rel, ans, persist=True)
        return ""


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
    as no-finding so it never feeds the exploit phase.

    A small local LLM hedges EVERY finding ('could potentially', 'might be').
    The old behaviour dropped any hedged answer wholesale, which silently
    killed real findings that cited a concrete line + code path. Now a
    concrete 'LINE <n>:' entry OVERRIDES the vague-text filter: hedging on a
    real finding is fine, only a vague answer WITHOUT any concrete LINE entry
    is rejected."""
    if not answer:
        return False
    normalized = re.sub(r"[^a-z ]", " ", answer.lower())
    normalized = " ".join(normalized.split())
    # Explicit clean-file answer.
    if "no issues identified" in normalized:
        return False
    # Scan only the NON-comment lines: the mandatory '// ' code-comment header
    # (cross-file 'check XXXX.cpp' refs) is free text and may legitimately
    # contain a word the vague-regex matches; judging it would wrongly discard
    # the real LINE finding.
    body = "\n".join(ln for ln in answer.splitlines()
                     if not ln.lstrip().startswith("//"))
    # A concrete finding is a line in the mandatory 'LINE <n>:' shape. One such
    # line is enough to keep the whole answer: the hedging around it is the
    # small model's wording, not a lack of a finding.
    if re.search(r"^\s*LINE\s+\d+\s*:", body, re.MULTILINE | re.IGNORECASE):
        return True
    # No concrete LINE entry -> vague narrative / generic description. Reject
    # so it does not trigger a false exploit run.
    if _VAGUE_RE.search(body):
        return False
    # A real finding must cite at least one line number (the format requires it).
    if not _LINE_NUM_RE.search(answer):
        return False
    return True


# ---------------------------------------------------------------------------
# Protocol-handler sweep (improvement #3). The per-file audit cannot see
# cross-packet inconsistencies: a handler reading more bytes than the parser
# declared for its code, a packet accepted in the wrong auth state, a query
# reply code that bypasses the per-player lock. This is a single CROSS-FILE
# turn that feeds the dispatch table (ProtocolParsingGeneral.cpp) + the
# per-packet handler bodies (ClientNetworkRead*, ProtocolParsingInput) to the
# model in one prompt, so it can compare parser-declared sizes vs handler-read
# sizes and auth-state guards across the whole protocol at once. Env-gated
# (CC_NO_PROTOCOL_SWEEP=1 disables); default on because it is the highest-
# value remote surface and the per-file sweep structurally cannot cover it.
# ---------------------------------------------------------------------------
PROTOCOL_SWEEP_FILES = (
    "general/base/ProtocolParsingGeneral.cpp",
    "general/base/ProtocolParsingInput.cpp",
    "general/base/ProtocolParsingBase.cpp",
    "server/base/ClientNetworkReadMessage.cpp",
    "server/base/ClientNetworkReadQuery.cpp",
)
SWEEP_FILE_CAP = 26000
SWEEP_TOTAL_CAP = 90000

PROTOCOL_SWEEP_SYSTEM = (
    "You are a security auditor doing ONE CROSS-FILE protocol sweep of the "
    "CatchChallenger game server. The files below contain the WIRE PARSER "
    "(packetFixedSize[] table: the byte size declared for each packet code; "
    "0xFE = variable-size, parsed via an inner length) and the PER-PACKET "
    "HANDLERS (the `case 0xNN:` switch bodies that consume the bytes). A bug "
    "here is the single highest-value remote defect: the input is untrusted "
    "TCP from a game client.\n"
    "\n"
    "Hunt SPECIFICALLY for these cross-file defects the per-file audit misses:\n"
    "- SIZE MISMATCH: a handler reads MORE bytes than packetFixedSize[code] "
    "declares (OOB read), or reads an inner length off the wire for a 0xFE "
    "packet and trusts it without bounding the subsequent reads.\n"
    "- STATE/AUTH BYPASS: a packet handler runnable BEFORE login completes, "
    "or one that skips the character-selected check, or trusts a client-supplied "
    "character/player id instead of the session's own.\n"
    "- INDEX MISMATCH: a handler uses a client-supplied id to index a table "
    "(item, monster, skill, plant, clan) without a bounds check against that "
    "table's real size.\n"
    "- REPLY-CODE ABUSE: a query reply (replyTo) routed to a handler that "
    "writes shared state without the per-player lock.\n"
    "\n"
    "The LIVE compile defines are: CATCHCHALLENGER_SERVER, "
    "CATCHCHALLENGER_CLASS_ALLINONESERVER, CATCHCHALLENGER_DB_FILE, "
    "CATCHCHALLENGER_DB_INTERNAL_VARS. Qt / dead-#ifdef branches are out.\n"
    "\n"
    "OUTPUT FORMAT - one line per real finding, nothing else:\n"
    "  LINE <n> of <relpath>: <packet code or symbol> -> <concrete consequence "
    "with the cross-file mismatch>\n"
    "If genuinely nothing exploitable across the whole protocol, output exactly:\n"
    "  No issues identified\n"
    "Do NOT report style, naming, or non-security issues. Cite real line numbers."
)


def _sweep_slice(text, cap):
    """Return up to `cap` bytes of `text`, keeping the dispatch-relevant lines
    (packetFixedSize assignments, case 0xNN: handlers, and a few lines of
    context around each) so the model sees the wire contract, not the file's
    full boilerplate."""
    lines = text.splitlines()
    keep_idx = set()
    for i, ln in enumerate(lines):
        if (re.search(r"packetFixedSize\d*\[0x", ln)
                or re.match(r"\s*case\s+0x[0-9A-Fa-f]+\s*:", ln)
                or re.match(r"\s*case\s+\d+\s*:", ln)):
            for j in range(max(0, i - 2), min(len(lines), i + 14)):
                keep_idx.add(j)
    out = [lines[j] for j in sorted(keep_idx)]
    return "\n".join(out)[:cap]


def protocol_sweep(out, spec=None):
    """One cross-file audit turn over the wire parser + per-packet handlers.
    Writes any finding to `out` in the same FINDINGS block shape as the per-
    file sweep so the exploit phase picks it up. Returns 0 always (best-effort:
    a backend failure is logged, never fatal)."""
    if os.environ.get("CC_NO_PROTOCOL_SWEEP"):
        return 0
    sys.stderr.write("%s [protocol-sweep] cross-file audit of the wire parser + "
                     "packet handlers\n" % _ts())
    blocks = []
    total = 0
    for rel in PROTOCOL_SWEEP_FILES:
        fp = os.path.join(REPO_ROOT, rel)
        try:
            text = open(fp, "r", errors="replace").read()
        except OSError as exc:
            sys.stderr.write("    [protocol-sweep] skip %s: %s\n" % (rel, exc))
            continue
        sliced = _sweep_slice(text, SWEEP_FILE_CAP)
        if total + len(sliced) > SWEEP_TOTAL_CAP:
            sliced = sliced[:SWEEP_TOTAL_CAP - total]
        if not sliced.strip():
            continue
        blocks.append("=== %s (dispatch-relevant slice) ===\n%s" % (rel, sliced))
        total += len(sliced)
        if total >= SWEEP_TOTAL_CAP:
            break
    if not blocks:
        sys.stderr.write("    [protocol-sweep] no dispatch code found; skipping\n")
        return 0
    notes = read_notes()
    notes_block = ("=== SECURITY-IA.md (project notebook) ===\n%s\n\n"
                   % notes if notes.strip() else "")
    user = (
        "%sPerform ONE cross-file protocol sweep. For EACH packet code that has "
        "both a packetFixedSize[] entry AND a `case 0xNN:` handler, verify the "
        "handler reads AT MOST the declared size and bounds every inner length "
        "/ id / index. Report only real exploitable mismatches in the mandatory "
        "LINE format.\n\n%s\n=== ALL DISPATCH FILES ===\n%s"
        % (notes_block, PROTOCOL_SWEEP_SYSTEM, "\n\n".join(blocks)))
    messages = [{"role": "system", "content": PROTOCOL_SWEEP_SYSTEM},
                {"role": "user", "content": user}]
    try:
        answer = chat_with(spec, messages)
    except Exception as exc:
        sys.stderr.write("    [protocol-sweep] backend error: %s\n" % exc)
        return 0
    answer = _finalize_answer("protocol-sweep", answer)
    if has_findings(answer):
        # Group finding lines by the cited source file so each becomes its own
        # FINDINGS block keyed by the REAL repo-relative path -> the exploit
        # phase reads the right source file. A line that doesn't cite a path
        # goes under a generic 'protocol-sweep' bucket the exploit phase will
        # skip (no source to read) but the human still sees.
        sweep_line_re = re.compile(
            r"^\s*LINE\s+\d+\s+of\s+(\S+)\s*:", re.IGNORECASE)
        groups = {}
        order = []
        generic = []
        for ln in answer.splitlines():
            m = sweep_line_re.match(ln)
            if m:
                rp = m.group(1)
                if rp not in groups:
                    groups[rp] = []
                    order.append(rp)
                groups[rp].append(ln)
            elif ln.strip() and not ln.lstrip().startswith("//"):
                generic.append(ln)
        for rp in order:
            print("=" * 70, file=out)
            print("FINDINGS: %s" % rp, file=out)
            print("=" * 70, file=out)
            print("\n".join(groups[rp]), file=out)
            print(file=out)
        if generic:
            print("=" * 70, file=out)
            print("FINDINGS: protocol-sweep", file=out)
            print("=" * 70, file=out)
            print("\n".join(generic), file=out)
            print(file=out)
        out.flush()
        sys.stderr.write("    [protocol-sweep] %d finding line(s) across %d "
                         "file(s)\n" % (sum(len(v) for v in groups.values()),
                                        len(order)))
    else:
        sys.stderr.write("    [protocol-sweep] no exploitable cross-file "
                         "mismatch found\n")
    return 0


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
    # Log + fast-probe the backend; the LLM is the whole audit, so a dead backend
    # means abort now rather than hang 1200s x retries on every one of 270+ files.
    if not preflight_backend()[0]:
        sys.stderr.write("%s [ERROR] IA backend unreachable - aborting scan "
                         "(set OLLAMA_HOST / fix ~/.config/CatchChallenger/"
                         "ia-settings.json or start the Ollama daemon)\n" % _ts())
        return 1
    ensure_context()
    files = collect_files()
    # Optional cap: scan only the first N files. Lets a Claude run be bounded to
    # a cheap, quick subset before committing to the full (276-file) sweep.
    scan_max = os.environ.get("SECSERVER_SCAN_MAX")
    if scan_max:
        try:
            n = int(scan_max)
        except ValueError:
            n = 0
        if 0 < n < len(files):
            sys.stderr.write("%s [INIT] SECSERVER_SCAN_MAX=%d: limiting scan to "
                             "first %d of %d files\n"
                             % (_ts(), n, n, len(files)))
            files = files[:n]
    panel = resolve_panel()
    if len(panel) > 1:
        # Build the shared caches now, single-threaded, so the panel worker
        # threads don't race on their lazy init.
        find_by_basename("")
        if len(ollama_backends()) > 1:
            _model_locations()
    scan_start = time.time()
    sys.stderr.write(
        "%s [INIT] %d files to audit (in-scope dirs + repo-local includes, vendor "
        "excluded) via %s\n" % (_ts(), len(files), common.IA_LABEL)
    )
    if len(panel) > 1:
        sys.stderr.write("%s [INIT] collaborative panel of %d models, %d rounds "
                         "each: %s\n"
                         % (_ts(), len(panel), CC_PANEL_ROUNDS, ", ".join(panel)))

    flagged = []
    for i, path in enumerate(files, 1):
        rel = os.path.relpath(path, REPO_ROOT)
        sys.stderr.write("%s [%d/%d] %s\n" % (_ts(), i, len(files), rel))
        sys.stderr.flush()
        try:
            if len(panel) > 1:
                answer = panel_analyze(path, panel)
            elif panel:
                answer = analyze(path, spec=panel[0])
            else:
                answer = analyze(path)
        except Exception as exc:  # network / ollama failure - report, continue
            sys.stderr.write("    [skip] %s\n" % exc)
            continue
        # Adversarial re-audit of HOT files whose first pass came back clean.
        # A small local LLM defaults to 'No issues identified'; the reframe
        # forces it to either surface a bug or justify every length/index it
        # consumes. Skipped when the first pass already found something, when
        # the operator disables it (CC_NO_ADVERSARIAL=1), or on non-hot files.
        if (not has_findings(answer) and is_hot_file(path)
                and not os.environ.get("CC_NO_ADVERSARIAL")):
            sys.stderr.write("    [adversarial] hot file came back clean -> "
                             "re-auditing with red-team prompt\n")
            try:
                if len(panel) > 1:
                    answer = panel_analyze(path, panel, adversarial=True)
                elif panel:
                    answer = analyze(path, spec=panel[0], adversarial=True)
                else:
                    answer = analyze(path, adversarial=True)
            except Exception as exc:
                sys.stderr.write("    [adversarial skip] %s\n" % exc)
        if has_findings(answer):
            flagged.append(rel)
            print("=" * 70, file=out)
            print("FINDINGS: %s" % rel, file=out)
            print("=" * 70, file=out)
            print(answer, file=out)
            print(file=out)
            out.flush()

    # Cross-file protocol sweep (improvement #3): a single turn over the wire
    # parser + per-packet handlers, after the per-file sweep. Catches the size
    # / auth-state / index mismatches the per-file audit cannot see. Uses the
    # same panel spec as the per-file sweep when one is configured.
    sweep_spec = None
    if len(panel) == 1:
        sweep_spec = panel[0]
    elif not panel:
        sweep_spec = None
    else:
        sweep_spec = _panel_lead(panel)
    try:
        protocol_sweep(out, spec=sweep_spec)
    except Exception as exc:
        sys.stderr.write("    [protocol-sweep error] %s\n" % exc)

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
    summary = claude_usage_summary()
    if summary:
        sys.stderr.write(summary)
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
    "  RUNHOST                    (POST-sandbox escalation: compile + run the "
    "SAME ELF on the host WITHOUT the chroot/seccomp jail - rlimits still cap "
    "CPU/mem. Use ONLY if the sandbox's seccomp KILLS a syscall your exploit "
    "genuinely needs: a 'terminated by signal 31'/SIGSYS under RUN is that jail, "
    "NOT a refutation. Re-RUN sandboxed first; RUNHOST is the fallback.)\n"
    "  SEEDDB item=<id>:<qty>[,<id>:<qty>...] cash=<n>\n"
    "                             (boundary-value DB: set the STARTING inventory/"
    "cash a NEWLY CREATED character gets from the profile, then reboots. Use it "
    "to put a count AT the 16/32-bit limit - e.g. 'SEEDDB item=5:4294967295' - "
    "then connect, create an account, ADD a character (0xAA) + select it (the "
    "profile is applied at CHARACTER creation, not just account auto-creation), "
    "and drive ONE more add/buy/sell/trade to test whether the item count or "
    "cash integer-overflows. item qty caps at uint32 max; cash is exact <2^53.)\n"
    "  RESTARTSERVER              (reboot the supervised server fresh on the "
    "same port - resets game-state)\n"
    "  MODE gdb | MODE valgrind   (choose/switch the supervisor; reboots fresh)\n"
    "  GDB\n  ```\n  <read-only gdb commands, one per line>\n  ```"
    "  (gdb mode only)\n"
    "  VERDICT CONFIRMED <one-line reason>     (= exploit SUCCESS)\n"
    "  VERDICT FALSEPOSITIVE <one-line reason> (= exploit FAIL)\n"
    "WITH EITHER VERDICT, append ONE terse C++ '// ' comment line meant to be "
    "pasted at the cited source line - documenting the code a little: for "
    "CONFIRMED, what the bug is (e.g. '// BUG: realprice*quantity wraps in 32 "
    "bits before the uint64 cash compare'); for FALSEPOSITIVE, the cross-file "
    "guard that makes it safe naming the FILE:LINE (e.g. '// SAFE: packet "
    "0x88 is fixed 10B (ProtocolParsingGeneral.cpp:152), parser delivers full "
    "size'). Keep it factual and short.\n"
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
    # POST-sandbox escalation: run the SAME exploit ELF on the host without the
    # bwrap chroot/seccomp (rlimits still apply). Checked before RUN ("RUNHOST"
    # has no space so RUN's prefix test would miss it anyway, but be explicit).
    if up == "RUNHOST" or up.startswith("RUNHOST "):
        return ("RUNHOST", "", None)
    # Seed boundary-value starting state (item count / cash at the 16/32-bit
    # limit) into the staged datapack profile, then reboot.
    if up.startswith("SEEDDB "):
        return ("SEEDDB", first[7:].strip(), None)
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


def do_run_host(outdir, timeout=RUN_TIMEOUT):
    """POST-sandbox escalation (the 'try exploit OUT of sandbox' step): compile
    the model's exploit and run the SAME ELF on the HOST WITHOUT bwrap's chroot
    + seccomp allowlist. The sandbox in do_run() is a deliberately TIGHT
    bare-TCP-client jail; some real exploits legitimately need a syscall it
    KILLS (a SIGSYS there is a false dead-end, not a refutation). This is the
    less-isolated retry: ONLY RLIMIT_CPU/AS/NOFILE still apply (preexec_fn) -
    there is NO filesystem (chroot) and NO syscall (seccomp) confinement, so the
    model-authored ELF runs with the harness's own host access. It is the
    deliberate "out of sandbox" attempt; use ONLY after a sandboxed RUN proved
    insufficient, on an authorized host you accept running attacker code on."""
    binpath, err = _compile_exploit(outdir)
    if err:
        return err
    try:
        r = subprocess.run([binpath], capture_output=True, text=True,
                           timeout=timeout, preexec_fn=_set_sandbox_rlimits,
                           cwd=outdir)
        out = (r.stdout or "") + (r.stderr or "")
        out += "\n[exploit (UNSANDBOXED, host) exit code %d]" % r.returncode
        if r.returncode < 0:   # killed by a signal (CPU SIGXCPU, segfault, ...)
            out += " (terminated by signal %d)" % (-r.returncode)
    except subprocess.TimeoutExpired as e:
        out = (e.stdout or "") + (e.stderr or "") + \
              "\n[exploit (UNSANDBOXED, host) wall-timeout after %ds]" % timeout
    except Exception as e:  # noqa
        out = "RUNHOST error: %s" % e
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
    base = re.split(r"[/\s]", line, maxsplit=1)[0].lower()   # 'x/4xw' -> 'x'
    if base not in GDB_ALLOWED:
        return False
    if "(" in line or _ASSIGN_RE.search(line):
        return False
    return True


def _parse_seed_spec(arg):
    """Parse a SEEDDB argument into {'items': {id: qty}, 'cash': int|None}.
    Syntax (either part optional):  item=<id>:<qty>[,<id>:<qty>...]  cash=<n>
    e.g. 'SEEDDB item=5:4294967295,1:65535 cash=1000000'. Non-numeric tokens are
    dropped. Lets the exploit IA stand up a FRESH character whose starting
    inventory/cash already sits AT the 16/32-bit limit, so it can drive one more
    add/buy/sell/trade and observe whether the count integer-overflows."""
    items = {}
    cash = None
    for tok in arg.replace(",", " ").split():
        low = tok.lower()
        if low.startswith("item="):
            tok = tok[5:]
            low = tok.lower()
        if low.startswith("cash="):
            try:
                cash = int(tok[5:])
            except ValueError:
                pass
            continue
        if ":" in tok:
            a, b = tok.split(":", 1)
            try:
                iid, qty = int(a), int(b)
            except ValueError:
                continue
            # The engine parses start.xml item quantity as uint32 and DROPS a
            # qty<=0 item ("quantity is null"); a qty>UINT32_MAX silently
            # truncates (stoul cast). Clamp to [1, 0xFFFFFFFF] so the seed loads.
            if qty > 0xFFFFFFFF:
                qty = 0xFFFFFFFF
            if qty >= 1:
                items[iid] = qty
    return {"items": items, "cash": cash}


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
        # Boundary-value DB seed set by the SEEDDB action ({'items':..,'cash':..}
        # or None). Re-applied on every (re)boot to the staged datapack profile
        # so a freshly auto-created character starts AT the chosen limit.
        self.seed = None
        self._seed_note = ""   # result of the last _apply_seed(), shown at boot
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

    # -- boundary-value DB seeding -----------------------------------------
    def _apply_seed(self):
        """If a SEEDDB seed is set, edit the run dir's datapack PROFILE
        (player/start.xml) so every freshly auto-created character starts with
        the requested inventory / cash. New-character creation seeds the player
        from this profile (server/base/ClientLoad/ClientHeavyLoadLogin2.cpp:
        `playerForProfile.items[item.id]=item.quantity`), so this is how the
        exploit reaches a count ALREADY at the 16/32-bit limit WITHOUT a real
        DB. The source datapack stays read-only (datapack-source-safety): the
        whole-dir `datapack` symlink is replaced by a shallow mirror that
        symlinks every entry EXCEPT a tiny WRITABLE copy of player/. No-op
        (returns "") when no seed is set."""
        if not self.seed:
            return ""
        import xml.etree.ElementTree as ET
        link = os.path.join(self.rundir, "datapack")
        try:
            target = os.path.realpath(link)
            if os.path.islink(link):
                os.remove(link)
                os.makedirs(link)
                for name in sorted(os.listdir(target)):
                    src = os.path.join(target, name)
                    dst = os.path.join(link, name)
                    if name == "player":
                        shutil.copytree(src, dst)   # writable (player/ is tiny)
                    else:
                        os.symlink(src, dst)        # source stays read-only
            startxml = os.path.join(link, "player", "start.xml")
            tree = ET.parse(startxml)
            start = tree.getroot().find("start")
            if start is None:
                return "[SEEDDB warning: no <start> in start.xml; not applied]"
            if self.seed.get("cash") is not None:
                cash = start.find("cash")
                if cash is None:
                    cash = ET.SubElement(start, "cash")
                cash.set("value", str(self.seed["cash"]))
            for item_id, qty in self.seed.get("items", {}).items():
                el = None
                for it in start.findall("item"):
                    if it.get("id") == str(item_id):
                        el = it
                        break
                if el is None:
                    el = ET.SubElement(start, "item")
                    el.set("id", str(item_id))
                el.set("quantity", str(qty))
            tree.write(startxml)
            # Drop any stale binary datapack cache so the edited XML is re-read.
            cache = os.path.join(link, "datapack-cache.bin")
            if os.path.isfile(cache):
                os.remove(cache)
            return "[SEEDDB applied to start.xml: %s]" % self.seed
        except (OSError, ET.ParseError) as exc:
            return "[SEEDDB error applying seed: %s]" % exc

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
        # Apply any boundary-value DB seed to the staged datapack profile BEFORE
        # boot (no-op when unset). Captured so the model sees it took effect.
        self._seed_note = self._apply_seed()
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
        seed = ("\n" + self._seed_note) if self._seed_note else ""
        return ("[%s session started (sandboxed): server under %s on "
                "127.0.0.1:%d (%s)]%s\n%s"
                % (self.mode, self.mode, GDB_PORT, status, seed, out[-2000:]))

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
        if (last_answer is not None and answer.strip()
                and answer.strip() == last_answer.strip()):
            # A reply cut at the output-token cap is DETERMINISTIC at our low
            # temperature, so it re-truncates to the same bytes and repeats
            # verbatim. That is an infrastructure limit, NOT a stuck model: don't
            # nuke the whole run - skip just THIS finding and carry on. Raise
            # ollama_num_predict / set ollama_think:false, or CC_IA_BACKEND=claude
            # to stop the truncation.
            if last_reply_truncated():
                sys.stderr.write("%s     [TRUNCATED REPEAT] reply cut at the "
                                 "output-token cap twice on exploit %02d (%s) -> "
                                 "skipping this finding (raise ollama_num_predict / "
                                 "set ollama_think:false, or CC_IA_BACKEND=claude)\n"
                                 % (_ts(), idx, rel))
                transcript.append("### TRUNCATED REPEAT (output-token cap; "
                                  "finding skipped, run continues)")
                verdict, reason = ("NOT-EXPLOITABLE",
                                   "reply truncated at the output-token cap "
                                   "(num_predict); finding skipped")
                break
            # Genuine stuck-model loop: fatal per operator policy.
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
        elif kind == "RUNHOST":
            # POST-sandbox: run the same ELF without bwrap (rlimits kept). For
            # exploits the tight seccomp jail blocks; same crash/hang check after.
            run_to = max(1, min(RUN_TIMEOUT, int(deadline - time.time())))
            sys.stderr.write("%s     [RUNHOST] compile+run exploit ELF "
                             "UNSANDBOXED on host (step %d, %ds left, timeout %ds)\n"
                             % (_ts(), step, remaining, run_to))
            result = do_run_host(outdir, timeout=run_to)
            result += "\n\n--- live server status ---\n" + live.poll_crash()
            ran_something = True
        elif kind == "SEEDDB":
            spec = _parse_seed_spec(arg)
            if not spec.get("items") and spec.get("cash") is None:
                result = ("SEEDDB error: nothing to seed. Syntax: "
                          "'SEEDDB item=<id>:<qty>[,<id>:<qty>...] cash=<n>' "
                          "(e.g. 'SEEDDB item=5:4294967295 cash=1000000').")
            else:
                live.seed = spec
                sys.stderr.write("%s     [SEEDDB] %s (step %d)\n"
                                 % (_ts(), spec, step))
                result = ("[seed set; rebooting so a newly CREATED character "
                          "starts with it]\n" + live.restart() +
                          "\n[The profile (player/start.xml) is applied when a "
                          "CHARACTER is created, not merely on account "
                          "auto-creation: connect, auto-create/login the "
                          "account, then ADD a character (0xAA) and select it - "
                          "that character's starter inventory/cash IS the seeded "
                          "one. Then drive one more add/buy/sell/trade to push "
                          "the count past the limit and check (valgrind / GDB "
                          "read-only) whether it wraps. NOTE: item qty caps at "
                          "uint32 max (4294967295); cash loads via double so is "
                          "integer-exact only below 2^53.]")
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
    if not preflight_backend()[0]:
        sys.stderr.write("%s [ERROR] IA backend unreachable - aborting exploit "
                         "phase (set OLLAMA_HOST / fix ~/.config/CatchChallenger/"
                         "ia-settings.json or start the Ollama daemon)\n" % _ts())
        return 1
    ensure_context()
    findings = parse_findings(FINDINGS)
    if not findings:
        sys.stderr.write("[!] no findings in %s; run `%s scan > findings.txt` "
                         "first.\n" % (FINDINGS, sys.argv[0]))
        return 1

    # CANDIDATE_RE is a PRIORITY ORDER, not a hard filter: a real bug worded
    # without a trigger keyword must NOT be silently dropped (recall loss that
    # trusts the model's phrasing over the code). Keyword-matched findings go
    # first (most likely a rewarded class), the rest follow and are logged so
    # the operator sees what was deprioritized. CC_EXPLOIT_MATCHED_ONLY=1
    # restores the old matched-only behaviour if a run must stay tight.
    matched = [(rel, body) for rel, body in findings if CANDIDATE_RE.search(body)]
    unmatched = [(rel, body) for rel, body in findings if not CANDIDATE_RE.search(body)]
    if unmatched:
        sys.stderr.write("%s [INIT] %d finding(s) did NOT match CANDIDATE_RE "
                         "(deprioritized, %s):\n"
                         % (_ts(), len(unmatched),
                            "DROPPED" if os.environ.get("CC_EXPLOIT_MATCHED_ONLY")
                            else "still queued after matched"))
        for rel, body in unmatched:
            sys.stderr.write("    - %s: %s\n"
                             % (rel, " ".join(body.split())[:120]))
    if os.environ.get("CC_EXPLOIT_MATCHED_ONLY"):
        candidates = matched
    else:
        candidates = matched + unmatched
    _cap = os.environ.get("SECSERVER_MAX")
    if _cap:
        candidates = candidates[:int(_cap)]
    os.makedirs(OUTPUT_ROOT, exist_ok=True)
    hard_budget, soft_budget = resolve_budgets()
    sys.stderr.write("%s [INIT] %d findings, %d look exploitable; generating "
                     "exploits via %s (budget per exploit: hard %s / soft %s)\n"
                     % (_ts(), len(findings), len(candidates), common.IA_LABEL,
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
             "Model: %s  |  candidates: %d\n" % (common.IA_LABEL, len(candidates)),
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
    summary = claude_usage_summary()
    if summary:
        sys.stderr.write(summary)
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
        "                 e.g. --model=gemma4:26b or --model=qwen2.5-coder:32b\n"
        "                 append '@host:port' to pin a specific Ollama backend,\n"
        "                 e.g. --model=qwen3:30b-a3b@gpu1:11434\n"
        "  --collaborate=A,B,C   collaborative panel of models (or 'auto' for the\n"
        "                 installed Ollama set <35B); they discuss up to 10 rounds,\n"
        "                 form work groups, and converge on a consensus diagnosis.\n"
        "                 Pin a per-model backend with '@host:port', e.g.\n"
        "                 --collaborate=qwen3:30b@gpu1:11434,deepseek:33b@gpu2:11434\n"
        "  --mode=MODE    STARTING supervisor: gdb (default) or valgrind. The\n"
        "                 exploit model switches at runtime via its MODE action;\n"
        "                 this only sets which tool the first instance boots\n"
        "                 under. Same as SECSERVER_MODE.\n"
        "  --help, -h     Show this help\n"
        "\n"
        "Examples:\n"
        "  # Default (backend + model come from ~/.config/CatchChallenger/\n"
        "  # ia-settings.json: 'ia_backend' + 'claude_api_key' OR 'ollama_host').\n"
        "  # Scan (detection) then exploit, both via the configured backend.\n"
        "  python3 server.py all 2> progress.log\n"
        "\n"
        "  # Claude (Anthropic API) - detection + exploit on Claude Opus 4.8.\n"
        "  # Key + ia_backend='claude' live in the out-of-repo settings; no env.\n"
        "  CC_IA_BACKEND=claude CC_CLAUDE_MODEL=claude-opus-4-8 \\\n"
        "      python3 server.py all 2> progress.log\n"
        "\n"
        "  # Claude via the OFFICIAL CLI - the ONLY ToS-safe way to use a Pro/Max\n"
        "  # SUBSCRIPTION (replaying its OAuth token from the API path is a Terms\n"
        "  # violation). Just `claude` logged in once; no token/key here at all.\n"
        "  CC_IA_BACKEND=claude-cli python3 server.py all 2> progress.log\n"
        "\n"
        "  # LOCAL Ollama with a SPECIFIC model (override the settings default).\n"
        "  # Hit 127.0.0.1:11434 directly; --model picks the served model.\n"
        "  OLLAMA_HOST=http://127.0.0.1:11434 \\\n"
        "      python3 server.py --model=qwen2.5-coder:32b all 2> progress.log\n"
        "\n"
        "  # REMOTE Ollama daemon (or the PHP router) + a specific model.\n"
        "  # The URL embeds infra IP - keep it in the out-of-repo settings, NOT\n"
        "  # on the command line; append '@host:port' to pin a backend per model.\n"
        "  python3 server.py --model=gemma4:26b@rtx5090:11434 all 2> progress.log\n"
        "\n"
        "  # ALL models work TOGETHER (collaborative panel): several local Ollama\n"
        "  # models + Claude audit each file at once, discuss up to 10 rounds,\n"
        "  form work groups, and a lead synthesises the consensus. 'auto' picks\n"
        "  every installed Ollama model <35B; add Claude explicitly if a key is\n"
        "  set. Detection (scan) uses the panel; exploit uses the single lead.\n"
        "  CC_IA_PANEL=auto,claude python3 server.py scan 2> progress.log\n"
        "  # Explicit panel across multiple backends (each pinned to its host):\n"
        "  python3 server.py --collaborate=qwen3:30b@gpu1:11434,deepseek:33b@gpu2:11434,claude all 2> progress.log\n"
        "\n"
        "Environment overrides:\n"
        "  CC_IA_BACKEND         ollama (default) | claude (API) | claude-cli\n"
        "                        (official `claude -p`; ToS-safe for a Pro/Max\n"
        "                        subscription - no token replay)\n"
        "  OLLAMA_HOST           default Ollama backend URL; selects the access\n"
        "                        mode by URL alone (local / a remote daemon / the\n"
        "                        Ollama-compatible PHP router). The remote/router\n"
        "                        URL embeds infra IP - keep it OUT of the repo in\n"
        "                        ~/.config/CatchChallenger/ia-settings.json as\n"
        "                        {\"ollama_host\": \"...\"}; never commit it.\n"
        "  CC_IA_MODEL           default Ollama model (default gemma4:26b)\n"
        "  CC_CLAUDE_MODEL       Claude model id (default claude-opus-4-8)\n"
        "  CC_IA_SETTINGS        JSON file declaring multiple Ollama backends\n"
        "                        (default ./ia-settings.json; see .example).\n"
        "                        When set it overrides OLLAMA_HOST.\n"
        "  CC_IA_PANEL           model list, or 'auto' (<35B Ollama set) -> a\n"
        "                        collaborative multi-model team audit (scan phase)\n"
        "  CC_PANEL_ROUNDS       team discussion rounds (default 10)\n"
        "  CC_PANEL_MAX_B        'auto' size cap, billions of params (default 35)\n"
        "  CC_PANEL_MAX_MEMBERS  cap on 'auto' panel size (default 4)\n"
        "  CC_PANEL_LEAD         model spec that synthesises consensus\n"
        "  ANTHROPIC_API_KEY     console API key (sk-ant-api...) for CC_IA_BACKEND=claude\n"
        "  CLAUDE_CODE_OAUTH_TOKEN  alt: a Claude Code token (`claude setup-token`)\n"
        "  SECSERVER_MODE        gdb (default) | valgrind  starting supervisor;\n"
        "                        the exploit model switches at runtime (see --mode)\n"
        "  SECSERVER_BUDGET      Hard per-exploit budget in seconds\n"
        "  SECSERVER_SOFT_BUDGET Soft per-exploit budget in seconds (default 75%%)\n"
        "  SECSERVER_STEPS       Max exploit turns (default 400)\n"
        "  SECSERVER_RUNTIMEOUT  RUN wall timeout in seconds (default 900)\n"
        "  SECSERVER_CPU         Sandbox RLIMIT_CPU seconds (default 10)\n"
        "  SECSERVER_MEM         Sandbox RLIMIT_AS bytes (default 128M)\n"
        "  SECSERVER_SCAN_MAX    Max files to scan (bound a cheap subset run)\n"
        "  SECSERVER_MAX         Max exploit candidates to attempt\n"
        "  CC_NO_ADVERSARIAL     Set to 1 to skip the adversarial re-audit of\n"
        "                        hot files (packet handlers / wire parser) whose\n"
        "                        first pass came back clean (recall booster)\n"
        "  CC_NO_PROTOCOL_SWEEP  Set to 1 to skip the cross-file protocol sweep\n"
        "                        (parser-declared size vs handler-read size)\n"
        % (prog, FINDINGS, FINDINGS, common.MODEL_NAME))


# One-function SECURITY prompt for the codecheck-engine scan. server.py is the
# security auditor (not a general code checker like codecheck.py), so this asks
# ONLY for EXPLOITABLE, remote-reachable memory-safety bugs, judged via the
# caller tree (the path untrusted TCP bytes take to the function).
CODECHECK_SECURITY_SYSTEM = (
    "You are a security auditor of a TCP-reachable C/C++ server. You are shown the "
    "relevant HEADER(s) in full, ONE function to audit, its CALLER tree (who reaches "
    "it - the path untrusted REMOTE bytes take to get here), and ONE thing it calls "
    "at a time. Audit ONLY the shown function. Report ONLY EXPLOITABLE memory-safety "
    "bugs reachable from untrusted network input: out-of-bounds read/write, an "
    "unchecked length/index/size/offset/pointer taken from a caller, integer "
    "overflow/underflow or signed/unsigned confusion driving a read/write/alloc, "
    "use-after-free / double-free, or a NULL deref on attacker-controlled input. Use "
    "the CALLER tree to confirm remote reachability - if untrusted input cannot reach "
    "the bad value it is NOT a finding. Be terse. If nothing exploitable, reply "
    "exactly: NO ISSUES. Otherwise one line per finding: "
    "SEVERITY(low|medium|high|critical) | function:line | the exploitable bug + how "
    "remote input triggers it.")


def run_codecheck():
    """SECURITY scan via the multi-IA agentic WORKGROUP engine (agentic.py): each
    function is reviewed independently by every IA (agentic — one callee branch per
    turn, may pull more data); the IAs discuss and form consensus WORKGROUPS around
    shared findings. The workgroup-vetted candidates are written to FINDINGS and
    run_exploit() DEVELOPS the proof. So server.py = multi-IA security audit + exploit
    generation, small-model friendly. Single-IA (no CC_IA_PANEL) = the lone agentic
    reviewer; no proof => no claim."""
    import codecheck
    import agentic
    os.makedirs(OUTPUT_ROOT, exist_ok=True)
    idx = codecheck.build_index()                  # all C/C++ excl vendor
    funcs = codecheck.leaves_first(idx)            # callees before callers
    specs = agentic.resolve_panel()
    sys.stderr.write("[codecheck] %d functions, %d IA(s); agentic per-function "
                     "security review (one branch at a time)\n"
                     % (len(funcs), len(specs)))
    codecheck.prewarm_types(funcs)
    by_file = {}
    i = 0
    while i < len(funcs):
        fi = funcs[i]
        i += 1
        blocks = agentic.audit_function(idx, fi, specs, role="security",
                                        sysprompt=CODECHECK_SECURITY_SYSTEM)
        if blocks:
            rel = os.path.relpath(fi.file, REPO_ROOT)
            by_file.setdefault(rel, []).append(
                "## %s (%s:%d)\n%s" % (fi.qual_name, rel, fi.line, "\n\n".join(blocks)))
    # Same FINDINGS block shape as run_scan(), so run_exploit() parses it unchanged.
    with open(FINDINGS, "w") as fh:
        for rel in by_file:
            for sink in (sys.stdout, fh):
                print("=" * 70, file=sink)
                print("FINDINGS: %s" % rel, file=sink)
                print("=" * 70, file=sink)
                print("\n\n".join(by_file[rel]), file=sink)
                print(file=sink)
    sys.stderr.write("[codecheck] %d file(s) with findings -> exploit phase\n"
                     % len(by_file))
    return run_exploit()


def main(argv):
    global RUN_UNDER, CC_IA_PANEL

    # scan always writes FINDINGS to disk (tee), so the exploit phase works
    # regardless of whether stdout was redirected.
    args = argv[1:]

    # Strip --model / --collaborate / --mode / --help before the phase word.
    filtered = []
    for a in args:
        if a.startswith("--model="):
            # Override whichever backend is active (Claude when CC_IA_BACKEND
            # =claude, else Ollama).
            if USE_CLAUDE:
                common.CLAUDE_MODEL = a[len("--model="):]
            else:
                # An optional '@host:port' suffix pins which Ollama backend
                # serves this model; keep the bare name as MODEL_NAME.
                common.MODEL_NAME = _register_model_spec(a[len("--model="):])
        elif a.startswith("--collaborate="):
            # Collaborative panel: comma-separated model specs, or 'auto' for the
            # installed Ollama models under CC_PANEL_MAX_B. Same as CC_IA_PANEL.
            CC_IA_PANEL = a[len("--collaborate="):].strip()
        elif a.startswith("--mode="):
            m = a[len("--mode="):].strip().lower()
            RUN_UNDER = "valgrind" if m in ("valgrind", "vg", "memcheck") else "gdb"
        elif a in ("--help", "-h"):
            _print_help(argv[0])
            return 0
        else:
            filtered.append(a)
    args = filtered
    # Recompute after --model so INIT/DONE logs name the model actually used.
    common.IA_LABEL = common.CLAUDE_MODEL if USE_CLAUDE else common.MODEL_NAME

    # Fail fast: without a credential, chat_claude raises per file and run_scan's
    # per-file `except` swallows it -> a misleading empty, exit-0 "clean" audit.
    # The claude-cli backend is exempt: it holds NO token of ours; the official
    # `claude` binary authenticates itself, so its preflight below checks instead.
    if USE_CLAUDE and not CLAUDE_VIA_CLI and not _claude_has_creds():
        sys.stderr.write("error: CC_IA_BACKEND=claude but no credential found - "
                         "export ANTHROPIC_API_KEY (console key) or "
                         "CLAUDE_CODE_OAUTH_TOKEN (e.g. `claude setup-token`), "
                         "or put 'claude_api_key' in the out-of-repo settings "
                         "(%s). For a Pro/Max SUBSCRIPTION use CC_IA_BACKEND="
                         "claude-cli instead (ToS-safe).\n" % common.IA_SETTINGS_FILE)
        return 2

    # Presence != validity: an expired OAuth token (rotates ~hourly), an empty
    # -balance console key, or a not-logged-in CLI all pass any presence check yet
    # fail every real call, and run_scan's per-file `except` would swallow it into
    # a misleading empty, exit-0 "clean" audit. One tiny live ping up front fails
    # fast instead - via the official CLI when that is the backend, else the API.
    if USE_CLAUDE:
        if CLAUDE_VIA_CLI:
            ok, msg = common.claude_cli_preflight()
        else:
            ok, msg = common.claude_preflight()
        if not ok:
            sys.stderr.write("error: Claude backend preflight failed - %s\n" % msg)
            return 2

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
    if mode in ("codecheck", "per-function", "perfunc"):
        # SHARED engine with codecheck.py: audit the C/C++ tree FUNCTION-BY-FUNCTION
        # with a LIMITED per-function view (headers in full + the one body + its
        # caller tree + ONE callee branch at a time). Each IA turn stays a few-K
        # tokens, so a SMALL local model (CC_IA_MODEL=qwen3-coder:30b, small num_ctx)
        # can do the audit where the per-file scan above needs a large-context model.
        # Unlike codecheck.py this stays SECURITY-focused and then GENERATES EXPLOITS
        # for what it finds (run_codecheck -> run_exploit).
        return run_codecheck()
    sys.stderr.write("error: unknown mode %r\n" % mode)
    _print_help(argv[0])
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
