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
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys

# codecheck.py is a GENERAL code-quality tool and lives in tools/codecheck/, but its
# IA core (common / codetree / agentic) is the shared infra under security/. Put that
# dir on sys.path so the imports resolve from here.
_SECURITY_DIR = os.path.normpath(os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "..", "security"))
if _SECURITY_DIR not in sys.path:
    sys.path.insert(0, _SECURITY_DIR)

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
_CALLER_TREE_CAP = 4000      # the caller tree text (a hub fn has many callers)
_LEADING_COMMENT_CAP = 1500  # doc-comment block right above the signature
_MAX_BRANCHES = 10           # cap callee branches audited per function
_MAX_TYPES = 16              # cap the param/local types listed (don't saturate)
_TYPE_CACHE_DIR = os.path.join(codetree.OUTPUT_ROOT, "types-cache")
_TIDY_CDB_DIR = None         # dir of the merged compile DB clang-tidy is pointed at
_TYPE_RE = re.compile(r"\b(?:Parm)?VarDecl\b.*?\b([A-Za-z_]\w*)\s+'([^']+)'")

# Triviality pre-filter: skip functions with <= this many real body lines.
_TRIVIAL_MAX_LINES = int(os.environ.get("CC_TRIVIAL_MAX_LINES", "4"))
# A "body" that is really a TYPE declaration = a compiler-generated special member
# (its debug info points at the class, not at a function body).
_TYPE_DECL_RE = re.compile(r"^(?:template\s*<[^>]*>\s*)?(?:class|struct|union)\b")
# clang-tidy check sets — GENERAL (codecheck default) vs SECURITY (server sets it).
TIDY_CHECKS = os.environ.get(
    "CC_TIDY_CHECKS",
    # performance-avoid-endl excluded: this project deliberately uses std::endl
    # (root CLAUDE.md logging rule), so it would flag every log line.
    "-*,bugprone-*,performance-*,-performance-avoid-endl,"
    "readability-identifier-naming,readability-misleading-indentation,"
    "misc-redundant-expression")
# The two dropped bugprone checks fire on this project's DELIBERATE style (the
# wire protocol assigns uint8_t<->char everywhere, and packet handlers take
# adjacent x/y/map coords), so they flood the security scan with non-security
# noise - and each such "finding" then costs the exploit phase an hour.
SECURITY_TIDY_CHECKS = ("-*,clang-analyzer-*,bugprone-*,cert-*,"
                        "-bugprone-narrowing-conversions,"
                        "-bugprone-easily-swappable-parameters")
# Comprehensive FILE-LEVEL sweep checks: HIGH-SIGNAL, tuned to THIS project's style.
# Drops the deliberate style (std::endl, snake_case_, short names, no-auto) and the
# noisy ones (include-cleaner, narrowing-conversions, swappable-params). Covers EVERY
# function in a file, not just the codetree-indexed few. CC_TIDY_DEEP adds the slow
# clang static analyzer. Override with CC_SWEEP_CHECKS.
SWEEP_CHECKS = os.environ.get(
    "CC_SWEEP_CHECKS",
    "-*,bugprone-*,-bugprone-easily-swappable-parameters,"
    "-bugprone-narrowing-conversions,performance-*,-performance-avoid-endl,"
    "misc-const-correctness,misc-redundant-expression,misc-unused-parameters,"
    "misc-unused-using-decls,misc-definitions-in-headers,"
    "readability-misleading-indentation,readability-redundant-control-flow,"
    "readability-redundant-string-cstr,readability-redundant-smartptr-get,"
    "readability-simplify-boolean-expr,readability-container-size-empty,"
    "readability-string-compare,readability-delete-null-pointer,"
    "readability-non-const-parameter")
if os.environ.get("CC_TIDY_DEEP", "").strip():
    SWEEP_CHECKS += ",clang-analyzer-*"
_TIDY_RE = re.compile(
    r"^(.+?):(\d+):\d+:\s+(?:warning|error):\s+(.*?)\s*\[([\w.,\-]+)\]\s*$")
# Persistent caches (clang-tidy output, per-(model,function) verdicts); set in
# build_index() to the SSD cache root.
_TIDY_CACHE_DIR = os.path.join(codetree.OUTPUT_ROOT, "tidy-cache")
_VERDICT_CACHE_DIR = os.path.join(codetree.OUTPUT_ROOT, "verdict-cache")

# Crisp system prompt for a SMALL model: ONE function, terse structured output.
# codecheck.py is the GENERAL code-quality reviewer — SECURITY is NOT its job
# (server.py owns exploitable-vuln finding + exploit generation). Review what a
# careful dev/QA reviewer would, EXCEPT memory-safety/security.
# Every finding carries a SEVERITY; HIGH/CRITICAL are reserved for findings the
# model is SURE of (the shown code proves them) — anything uncertain is MEDIUM,
# and audit_function() additionally adversarially verifies every HIGH/CRITICAL.
CHECK_SYSTEM = (
    "You are a meticulous C/C++ code reviewer for general quality - NOT security "
    "(a separate tool handles vulnerabilities; do not duplicate it). You are shown "
    "the relevant HEADER(s), the param/local TYPES, ONE function to review, its "
    "CALLER tree, and (when present) ONE thing it calls. Review ONLY the shown "
    "function for: real BUGS (wrong condition, off-by-one, wrong variable, a return "
    "value not checked, resource leak, logic error); ILLOGICAL or dead/unreachable "
    "code; poor VARIABLE and FUNCTION NAMES (unclear, misleading, name-vs-purpose "
    "mismatch); CLARITY problems (over-complex, confusing flow, needs a comment); "
    "OPTIMIZATION (needless copy/allocation, redundant work, a better "
    "algorithm/container); and DUPLICATION. Do NOT report memory-safety or security "
    "issues - out of scope here. Rate every finding: LOW = style/naming/clarity "
    "nit; MEDIUM = real improvement, wasted work, or a probable minor bug; HIGH = "
    "a definite bug producing wrong behaviour; CRITICAL = a definite bug causing a "
    "crash, data loss/corruption, or a badly wrong result. Use HIGH or CRITICAL "
    "ONLY when the shown code PROVES it - you must be SURE; if anything is "
    "uncertain or outside the shown code (including truncated text), use MEDIUM. "
    "Be terse. If the function is clean, reply exactly: NO ISSUES. Otherwise one "
    "line per finding:\n"
    "CATEGORY(bug|logic|naming|clarity|perf|duplication|deadcode) | "
    "SEVERITY(LOW|MEDIUM|HIGH|CRITICAL) | function:line | the problem -> the fix")

# Severity plumbing: parse the SEVERITY(...) tags out of a finding, order them,
# downgrade unconfirmed HIGH/CRITICAL to MEDIUM (never silently keep a "sure"
# level the adversarial check could not confirm).
_SEVERITY_RE = re.compile(r"\bSEVERITY\((LOW|MEDIUM|HIGH|CRITICAL)\)", re.I)
_SEV_ORDER = {"LOW": 1, "MEDIUM": 2, "HIGH": 3, "CRITICAL": 4}


def finding_severity(text):
    """Highest SEVERITY(...) tag in a finding block; '' when untagged."""
    best = ""
    for m in _SEVERITY_RE.finditer(text):
        s = m.group(1).upper()
        if _SEV_ORDER[s] > _SEV_ORDER.get(best, 0):
            best = s
    return best


def _sev_downgrade_tag(m):
    s = m.group(1).upper()
    return "SEVERITY(MEDIUM)" if _SEV_ORDER[s] >= _SEV_ORDER["HIGH"] else m.group(0)


def downgrade_severity(text, note):
    """Rewrite every SEVERITY(HIGH|CRITICAL) tag in `text` to SEVERITY(MEDIUM),
    with a visible note saying why. HIGH/CRITICAL means SURE; when the
    adversarial check cannot confirm that, the finding survives at MEDIUM."""
    return (_SEVERITY_RE.sub(_sev_downgrade_tag, text)
            + "\n[severity downgraded to MEDIUM: %s]" % note)


def count_severities(text, counts):
    """Add `text`'s SEVERITY tags into `counts` (LOW/MEDIUM/HIGH/CRITICAL keys);
    returns how many tags were seen (0 = an unrated block)."""
    n = 0
    for m in _SEVERITY_RE.finditer(text):
        counts[m.group(1).upper()] += 1
        n += 1
    return n


# ---------------------------------------------------------------------------
# Index + scope
# ---------------------------------------------------------------------------
def _cache_root():
    """Persistent on-SSD cache dir for the clang-IR cache + type cache + the
    auto-generated compile DB. write-once-per-file (few writes), survives across
    runs so a re-audit is fast. Override with CC_CODECHECK_CACHE."""
    return os.environ.get("CC_CODECHECK_CACHE",
                          "/mnt/data/perso/tmp/codecheck").rstrip("/")


# One compile DB per BINARY. Each binary compiles the SHARED sources with its
# OWN -D set (CATCHCHALLENGER_CLASS_*, DB backend), so a TU that belongs to another
# binary but is compiled with server/cli's flags fails ("no member named 'MasterLink'",
# "unknown type name 'DatabaseBaseCallBack'") and its functions never enter the index.
# With server/cli alone, 40 of 236 in-scope TUs failed - including ALL 16 of
# server/login/, the pre-auth parser, i.e. the highest-value remote surface.
# server/cli comes FIRST: _load_cdb() merges with setdefault, so its flags win for a
# source shared by several binaries. cmake CONFIGURE only (no build), cached forever.
_CDB_TARGETS = (
    ("cdb", ("server", "cli")),              # primary target; legacy dir name (cached)
    ("cdb-login", ("server", "login")),
    ("cdb-gateway", ("server", "gateway")),
    ("cdb-game-server-alone", ("server", "game-server-alone")),
    ("cdb-master", ("server", "master")),
)

# The client + tools binaries. Their TUs need Qt's -I set and each tool's own -D set,
# which no server DB carries. Registered ONLY when the audited scope reaches into
# client/ or tools/ (see _targets_for): a server-only audit — security/server.py pins
# CODECHECK_SCOPE_DIRS to general+server — must keep EXACTLY the flags it had, and
# _common_flags() (the fallback for a TU in no DB) is a majority vote over every
# merged DB entry, so silently adding ~700 client/tools entries would shift it.
_CDB_TARGETS_SCOPED = (
    ("cdb-client", ("client",)),             # qtopengl (+ embedded server for solo)
    ("cdb-client800x600", ("client", "qtcpu800x600")),
    ("cdb-bot-actions", ("tools", "bot-actions")),
    ("cdb-bot-cli", ("tools", "bot-test-connect-to-gameserver-cli")),
    ("cdb-datapack-downloader", ("tools", "datapack-downloader-cli")),
    ("cdb-datapack-explorer", ("tools", "datapack-explorer-generator-cli")),
    ("cdb-tileset-dedup", ("tools", "datapack-tileset-deduplicate")),
    ("cdb-gba2cc", ("tools", "gba2catchchallenger")),
    ("cdb-map2png", ("tools", "map2png")),
    ("cdb-mapgen", ("tools", "map-procedural-generation")),
    ("cdb-mapgen-terrain", ("tools", "map-procedural-generation-terrain")),
    ("cdb-stats", ("tools", "stats")),
    ("cdb-tileset-tagger", ("tools", "tileset-tagger")),
    ("cdb-tmx2zstd", ("tools", "tmxTileLayerConverterToZstd")),
    ("cdb-tuxemon2cc", ("tools", "tuxemon2catchchallenger")),
)


def _targets_for(scope):
    """Which binaries need a compile DB for `scope`: always the server set (general/
    + server/ is every scope's core, and server/cli's flags must stay first for the
    sources several binaries share), plus each client/tools binary whose own sources
    are inside the scope. scope=None means the DEFAULT_SCOPE (everything)."""
    out = list(_CDB_TARGETS)
    roots = [os.path.realpath(s) for s in (scope if scope else DEFAULT_SCOPE)]
    for name, parts in _CDB_TARGETS_SCOPED:
        src = os.path.realpath(os.path.join(REPO_ROOT, *parts))
        if any(src == r or src.startswith(r + os.sep) for r in roots):
            out.append((name, parts))
    return out


def _ensure_compile_db(root, scope=None):
    """Find or auto-generate the compile_commands.json set so clang resolves REAL
    flags (codetree's default /tmp path is absent on a fresh checkout). CC_COMPILE_DB
    wins (single DB); else configure each in-scope binary (_targets_for) ONCE into
    <root>/<dir> and reuse forever. Returns the list of DB paths, primary target
    first; empty when cmake is unavailable."""
    db = os.environ.get("CC_COMPILE_DB", "").strip()
    if db and os.path.isfile(db):
        return [db]
    if shutil.which("cmake") is None:
        return []
    out = []
    for name, parts in _targets_for(scope):
        build = os.path.join(root, name)
        js = os.path.join(build, "compile_commands.json")
        why = ""
        if not os.path.isfile(js):
            src = os.path.join(REPO_ROOT, *parts)
            if not os.path.isfile(os.path.join(src, "CMakeLists.txt")):
                why = "no CMakeLists.txt"
            else:
                os.makedirs(build, exist_ok=True)
                cmd = ["nice", "-n", "19", "ionice", "-c", "3", "cmake",
                       "-S", src, "-B", build,
                       "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
                       "-DCATCHCHALLENGER_DB_FILE=ON",
                       "-DCMAKE_BUILD_TYPE=RelWithDebInfo"]
                if (name, parts) not in _CDB_TARGETS:
                    # Describe the binary with the compiler we ANALYSE with. Qt6
                    # hands every Qt target GCC's -mno-direct-extern-access, which
                    # clang rejects outright -> the whole TU never compiles to IR
                    # (~300 client/tools TUs). Configuring these with clang keeps
                    # the DB flags clang-valid. The server DBs stay on the default
                    # compiler: they carry no Qt flags, they already work, and
                    # security/server.py depends on them exactly as they are.
                    cmd += ["-DCMAKE_C_COMPILER=clang",
                            "-DCMAKE_CXX_COMPILER=clang++"]
                try:
                    r = subprocess.run(cmd, capture_output=True, text=True,
                                       timeout=600)
                    if r.returncode != 0:
                        why = (r.stderr or r.stdout or "").strip().replace(
                            "\n", " ")[-160:]
                except (OSError, subprocess.SubprocessError) as exc:
                    why = str(exc)
        if os.path.isfile(js):
            out.append(js)
        else:
            # A missing DB is not fatal (flags fall back to the common set) but it
            # means that binary's TUs are audited with someone else's -D set, so say
            # so instead of silently narrowing the scope.
            sys.stderr.write("[codecheck] no compile DB for %s: %s\n"
                             % (os.path.join(*parts), why or "cmake unavailable"))
    return out


def _write_merged_cdb(root, dbs):
    """Merge every registered compile DB into ONE compile_commands.json and return
    its dir. clang-tidy takes a single `-p <dir>`, so with the raw list it saw only
    the FIRST DB (server/cli) and every client/tools/login file was swept with NO
    flags — i.e. it did not compile, and the sweep was blind exactly where the code
    is. Merge order follows _CDB_PATHS (first wins), same precedence as _load_cdb."""
    merged, seen = [], set()
    for db in dbs:
        try:
            entries = json.load(open(db))
        except (OSError, ValueError) as exc:
            sys.stderr.write("[codecheck] unreadable compile DB %s: %s\n" % (db, exc))
            continue
        for e in entries:
            f = e.get("file")
            if f and f not in seen:
                seen.add(f)
                merged.append(e)
    # One dir per DB SET (a server-only scope merges 5 DBs, the full scope 20), and
    # an atomic rename: a security audit and the all.sh stage can run at the same
    # time, and neither may read the other's half-written file.
    key = hashlib.sha256("|".join(dbs).encode()).hexdigest()[:12]
    out = os.path.join(root, "cdb-merged-" + key)
    dst = os.path.join(out, "compile_commands.json")
    try:
        os.makedirs(out, exist_ok=True)
        tmp = dst + ".%d.tmp" % os.getpid()
        with open(tmp, "w") as fh:
            json.dump(merged, fh)
        os.replace(tmp, dst)
    except OSError as exc:
        sys.stderr.write("[codecheck] cannot write the merged compile DB: %s\n" % exc)
        return None
    return out


def setup_caches(scope=None):
    """Point codetree's clang-IR cache + our type/tidy/verdict caches at the
    PERSISTENT SSD cache (CC_CODECHECK_CACHE) and auto-find/generate the compile DB
    for `scope` (so clang-tidy resolves real flags). Shared by build_index AND the
    deterministic --sweep, which needs the compile DB but NOT the IR index."""
    global _TYPE_CACHE_DIR, _TIDY_CACHE_DIR, _VERDICT_CACHE_DIR
    root = _cache_root()
    codetree.OUTPUT_ROOT = root
    codetree._IR_CACHE_DIR = os.path.join(root, "ast-cache", "ir")
    os.makedirs(codetree._IR_CACHE_DIR, exist_ok=True)
    _TYPE_CACHE_DIR = os.path.join(root, "types-cache")
    _TIDY_CACHE_DIR = os.path.join(root, "tidy-cache")
    _VERDICT_CACHE_DIR = os.path.join(root, "verdict-cache")
    # Insert in REVERSE so the first DB returned (server/cli, the primary target)
    # ends up first in _CDB_PATHS: _load_cdb() merges with setdefault, so the
    # earliest path wins for a source shared by several binaries (general/base/*).
    global _TIDY_CDB_DIR
    dbs = _ensure_compile_db(root, scope)
    for db in reversed(dbs):
        if db not in codetree._CDB_PATHS:
            codetree._CDB_PATHS.insert(0, db)
    if dbs:
        _TIDY_CDB_DIR = _write_merged_cdb(root, [p for p in codetree._CDB_PATHS
                                                 if os.path.isfile(p)])
    return root


def build_index(scope=None):
    """Build (or reuse) the codetree C/C++ index over `scope` (excl vendor), on the
    persistent SSD cache + auto-generated compile DB, so a re-audit is fast and the
    tool works out of the box."""
    setup_caches(scope)
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
def _cap(text, cap, what):
    """Cap `text` at `cap` chars, MARKING the cut when it happens — the model must
    KNOW it sees partial data (CHECK_SYSTEM forbids HIGH/CRITICAL on truncated
    text), instead of silently reasoning over an amputated body/header."""
    if len(text) <= cap:
        return text
    return text[:cap] + "\n... [%s truncated at %d chars — do not be sure about what follows]\n" % (what, cap)


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
            out.append((os.path.relpath(h, REPO_ROOT), _cap(txt, _HEADER_CAP, "header")))
    return out


def _leading_comment(file_path, start_line):
    """The contiguous comment block IMMEDIATELY above the function signature (its
    doc-comment: contract, preconditions, ownership, "len is untrusted", ...).
    source_body() starts at the signature line, so without this the model never
    sees it. Walks UP from the line above the signature over adjacent // lines and
    /* */ blocks; STOPS at the first blank or code line, so a file-level licence
    header (separated by a blank line) is NOT pulled in. Capped, nearest-the-
    function kept. Best-effort: '' on any failure."""
    try:
        lines = open(file_path, "r", errors="replace").readlines()
    except OSError:
        return ""
    # start_line==1 has nothing above it; a line past EOF (clang line-misattribution)
    # has no meaningful "above" either — best-effort, never index out of range.
    if start_line < 2 or start_line > len(lines):
        return ""
    i = start_line - 2                 # 0-indexed line directly above the signature
    out = []
    in_block = False                   # walking up THROUGH a /* ... */ block
    while i >= 0:
        s = lines[i].strip()
        if in_block:
            out.append(lines[i])
            if s.startswith("/*"):     # reached the block opener -> done with it
                in_block = False
            i -= 1
        else:
            if s.endswith("*/"):       # closing of a block comment (seen first, going up)
                out.append(lines[i])
                if not s.startswith("/*"):   # a one-line /* ... */ closes itself
                    in_block = True
                i -= 1
            else:
                if s.startswith("//"):
                    out.append(lines[i])
                    i -= 1
                else:
                    break              # blank or code -> stop (don't grab the licence header)
    if not out:
        return ""
    out.reverse()
    text = "".join(out)
    return text[-_LEADING_COMMENT_CAP:]   # keep the part nearest the function


def _body(fi, cap):
    body, _ = codetree.source_body(fi.file, fi.line)
    if not body:
        return "(body not found)"
    # Prepend the function's own doc-comment (source_body starts at the signature,
    # so it would otherwise be invisible). Each part is independently capped, so the
    # total stays bounded (lead <= _LEADING_COMMENT_CAP, body <= cap).
    return _leading_comment(fi.file, fi.line) + _cap(body, cap, "function body")


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


def _var_types(fi):
    """Param/local variable TYPES of the focused function, via clang's AST
    (`-ast-dump-filter` scopes the dump to just this function). Ordered
    {name: type}, capped at _MAX_TYPES. Cached on disk by file mtime + function.

    This is the ALGORITHMIC (clang-derived, not guessed) fact we hand the model so
    it can reason about the code without the full headers — e.g. that `size` is a
    `const uint32_t &` or `buf` is a `char[4096]`. Empty on any failure: a hint,
    never required."""
    if not codetree.CLANG:
        return {}
    real = os.path.realpath(fi.file)
    try:
        mtime = os.path.getmtime(real)
    except OSError:
        return {}
    name = fi.qual_name.split("::")[-1]
    key = "%s.%s.%s" % (os.path.basename(real), name,
                        format(mtime, ".0f").replace("-", "_"))
    cache = os.path.join(_TYPE_CACHE_DIR, key + ".json")
    try:
        with open(cache) as fh:
            return json.load(fh)
    except (OSError, ValueError):
        pass
    flags = codetree.flags_for(real)
    cmd = [codetree.CLANG, "-fsyntax-only", "-fno-color-diagnostics",
           "-Xclang", "-ast-dump", "-Xclang", "-ast-dump-filter=" + name]
    cmd += flags.split() if flags else ["-std=gnu++23"]
    cmd.append(real)
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=40)
    except (OSError, subprocess.SubprocessError):
        return {}
    types = {}
    for line in r.stdout.splitlines():
        m = _TYPE_RE.search(line)
        if m:
            n, t = m.group(1), m.group(2)
            if n not in types:
                types[n] = t
            if len(types) >= _MAX_TYPES:
                break
    try:
        os.makedirs(_TYPE_CACHE_DIR, exist_ok=True)
        with open(cache, "w") as fh:
            json.dump(types, fh)
    except OSError:
        pass
    return types


def prewarm_types(funcs, workers=8):
    """Populate the var-type cache for many functions in parallel (each is its own
    clang parse) so a whole-tree scan isn't serialized on per-function clang runs.
    Call before iterating build_views over many functions (e.g. a bulk scan)."""
    from concurrent.futures import ThreadPoolExecutor
    with ThreadPoolExecutor(max_workers=workers) as pool:
        list(pool.map(_var_types, funcs))


# ---------------------------------------------------------------------------
# Triviality pre-filter — don't spend an IA call on a function with no logic
# ---------------------------------------------------------------------------
def is_trivial(fi):
    """True for a function not worth auditing: a destructor, or a body of <=
    _TRIVIAL_MAX_LINES real lines (empty/defaulted, a getter/setter, a thin
    forwarder). These are a large fraction of the tree and an LLM finds nothing in
    them — skipping them focuses the budget on real logic."""
    if fi.qual_name.split("::")[-1].startswith("~"):
        return True
    body, _ = codetree.source_body(fi.file, fi.line)
    if not body:
        return False
    # A ctor/dtor the COMPILER generates has no body of its own: its debug info
    # points at the TYPE, so source_body() hands back the whole `class X { ... }`.
    # Reviewing a type declaration under a function name is how a reviewer ends up
    # reporting "this destructor loads XML files" - there is no function to audit.
    if _TYPE_DECL_RE.match(body.lstrip()):
        return True
    n = 0
    for ln in body.splitlines():
        s = ln.strip()
        if s and s not in ("{", "}", "};") and not s.startswith(("//", "/*", "*", "#")):
            n += 1
    return n <= _TRIVIAL_MAX_LINES


def audit_targets(funcs, out=sys.stderr):
    """`funcs` minus the trivial ones (reports how many were skipped)."""
    keep = [f for f in funcs if not is_trivial(f)]
    sk = len(funcs) - len(keep)
    if sk:
        out.write("[codecheck] skipping %d trivial function(s); auditing %d\n"
                  % (sk, len(keep)))
    return keep


# ---------------------------------------------------------------------------
# Algorithmic layer — clang-tidy / static analyzer per function
# ---------------------------------------------------------------------------
def _file_tidy(path, checks, run=True):
    """clang-tidy `path` with `checks` (via the compile-DB dir) -> [(line, msg,
    check), ...] for findings IN this file. Cached per (mtime, checks): clang-tidy
    COMPILES the TU (slow), so one run per file covers all its functions. Empty
    when clang-tidy / a compile DB is unavailable. run=False => cache-only: never
    launch clang-tidy on a miss (build_views uses this; prewarm_tidy fills it)."""
    tidy = shutil.which("clang-tidy")
    if not tidy:
        return []
    real = os.path.realpath(path)
    cdb_dir = _TIDY_CDB_DIR or next((os.path.dirname(p) for p in codetree._CDB_PATHS
                                     if os.path.isfile(p)), None)
    if not cdb_dir:
        return []
    try:
        mtime = os.path.getmtime(real)
    except OSError:
        return []
    key = hashlib.sha256(("%s|%s|%s" % (real, mtime, checks)).encode()).hexdigest()[:24]
    cache = os.path.join(_TIDY_CACHE_DIR, key + ".json")
    try:
        with open(cache) as fh:
            return json.load(fh)
    except (OSError, ValueError):
        pass
    if not run:
        return []                          # cache-only: don't block build_views
    try:
        r = subprocess.run([tidy, "-p", cdb_dir, "--quiet", "--checks=" + checks, real],
                           capture_output=True, text=True, timeout=240)
    except (OSError, subprocess.SubprocessError):
        return []
    out = []
    for line in r.stdout.splitlines():
        m = _TIDY_RE.match(line)
        if m and os.path.realpath(m.group(1)) == real:
            out.append([int(m.group(2)), m.group(3), m.group(4)])
    try:
        os.makedirs(_TIDY_CACHE_DIR, exist_ok=True)
        with open(cache, "w") as fh:
            json.dump(out, fh)
    except OSError:
        pass
    return out


def tidy_for_function(fi, checks=None, run=True):
    """clang-tidy findings whose line is inside the function's source range."""
    items = _file_tidy(fi.file, checks or TIDY_CHECKS, run=run)
    if not items:
        return []
    _b, end = codetree.source_body(fi.file, fi.line)
    hi = end if end and end >= fi.line else fi.line + 300
    return [it for it in items if fi.line <= it[0] <= hi]


def prewarm_tidy(funcs, checks=None, workers=6):
    """Run clang-tidy ONCE per file (parallel) so per-function lookups hit the
    cache. Slower than the AST type dump, so fewer workers."""
    seen, files = set(), []
    for f in funcs:
        rp = os.path.realpath(f.file)
        if rp not in seen:
            seen.add(rp)
            files.append(rp)
    ck = checks or TIDY_CHECKS
    from concurrent.futures import ThreadPoolExecutor
    with ThreadPoolExecutor(max_workers=workers) as pool:
        list(pool.map(lambda p: _file_tidy(p, ck), files))


# ---------------------------------------------------------------------------
# Comprehensive FILE-LEVEL deterministic sweep — covers EVERY function in a file,
# not just the codetree-indexed few (that LLVM-IR index holds only ~4-6 functions
# per file, so a per-indexed-function scan misses most of the code).
# ---------------------------------------------------------------------------
def _scope_files(scope=None):
    """Every non-vendor, non-GENERATED C/C++ source file under the scope roots (or
    a file root). is_generated() drops the build trees (`*_autogen/`, `CMakeFiles/`,
    moc_/qrc_/ui_): they are not source of truth and reviewing them is pure noise -
    the index walk already skips them, this sweep must skip them too."""
    roots = list(scope) if scope else list(DEFAULT_SCOPE)
    out, seen = [], set()
    for root in roots:
        if os.path.isfile(root):
            cand = [root]
        else:
            cand = []
            for dp, _d, fns in os.walk(root):
                for fn in fns:
                    if fn.endswith((".cpp", ".cc", ".cxx", ".c")):
                        cand.append(os.path.join(dp, fn))
        for p in cand:
            rp = os.path.realpath(p)
            if (rp not in seen and not codetree.is_vendor(p)
                    and not codetree.is_generated(p)):
                seen.add(rp)
                out.append(p)
    out.sort()
    return out


def file_sweep(scope=None, checks=None, workers=None):
    """Run clang-tidy over EVERY non-vendor C/C++ file in the scope (cached per file).
    Returns {rel_path: [(line, check, message), ...]} sorted by line — the
    comprehensive deterministic 'where + how' improvement list."""
    setup_caches(scope)                            # compile DB + cache dirs (for --sweep)
    checks = checks or SWEEP_CHECKS
    files = _scope_files(scope)
    from concurrent.futures import ThreadPoolExecutor
    # None -> every core: clang-tidy is one independent process per file.
    with ThreadPoolExecutor(max_workers=workers or os.cpu_count() or 6) as pool:
        pairs = list(pool.map(lambda f: (f, _file_tidy(f, checks)), files))
    by_file = {}
    for f, items in pairs:
        if items:
            rel = os.path.relpath(f, REPO_ROOT)
            by_file[rel] = sorted((ln, chk, msg) for ln, msg, chk in items)
    return by_file


def tidy_level(check):
    """Deterministic LOW/MEDIUM level for a clang-tidy check. These findings are
    machine-verified but pattern-level (no reachability proof), so they are never
    HIGH/CRITICAL: real-bug-shaped patterns rate MEDIUM, style/perf polish LOW."""
    if check.startswith(("bugprone", "clang-analyzer")) \
            or check == "misc-redundant-expression":
        return "MEDIUM"
    return "LOW"


def emit_sweep(by_file, out=sys.stdout):
    """Coherent deterministic output, grouped by file: each line = LEVEL +
    WHERE (file:line) + WHAT (the check) + HOW (the clang-tidy message).
    Returns the finding count."""
    total = sum(len(v) for v in by_file.values())
    med = sum(1 for v in by_file.values() for _ln, chk, _m in v
              if tidy_level(chk) == "MEDIUM")
    out.write("#### Deterministic improvements (clang-tidy): %d in %d file(s) "
              "(%d MEDIUM, %d LOW) — where & how\n"
              % (total, len(by_file), med, total - med))
    for rel in sorted(by_file):
        out.write("\n%s\n" % rel)
        for ln, chk, msg in by_file[rel]:
            out.write("  L%-5d %-6s %-42s %s\n" % (ln, tidy_level(chk), chk, msg))
    if not by_file:
        out.write("  (none with the curated checks — try CC_TIDY_DEEP=1 or a broader "
                  "CC_SWEEP_CHECKS)\n")
    return total


# ---------------------------------------------------------------------------
# Incremental: per-(model, function) verdict cache (skip unchanged functions)
# ---------------------------------------------------------------------------
# The cache is COMPUTE-ONLY: every run STILL prints all findings, so a problem is
# never silently forgotten — the cache only skips the LLM RE-RUN when nothing the
# model sees has changed. `material` is the FULL view the model gets (headers + body
# + callers + callee branches + clang types + tidy) plus model + mode + verify, so
# it recomputes when the file, a header, a callee, the model, or the mode changes.
# CC_NO_CACHE forces a full recompute.
_NO_CACHE = bool(os.environ.get("CC_NO_CACHE", "").strip())
_CACHE_STATS = {"hit": 0, "miss": 0}


def _verdict_path(material):
    h = hashlib.sha256("\x00".join(material).encode("utf-8", "replace")).hexdigest()
    return os.path.join(_VERDICT_CACHE_DIR, h[:32] + ".json")


def verdict_get(material):
    """Cached findings for this exact (view, model, mode, verify), or None. A hit
    means the LLM is skipped — but the caller STILL prints the findings."""
    if _NO_CACHE:
        return None
    try:
        with open(_verdict_path(material)) as fh:
            v = json.load(fh)
    except (OSError, ValueError):
        return None
    _CACHE_STATS["hit"] += 1
    return v


def verdict_put(material, value):
    if _NO_CACHE:
        return
    try:
        os.makedirs(_VERDICT_CACHE_DIR, exist_ok=True)
        with open(_verdict_path(material), "w") as fh:
            json.dump(value, fh)
    except (OSError, TypeError):
        pass


def cache_summary():
    """One line for the user: how many functions were recomputed vs reused — so it
    is always visible that 'reused' only means unchanged file/LLM/mode, not skipped
    output."""
    h, m = _CACHE_STATS["hit"], _CACHE_STATS["miss"]
    return ("cache: %d recomputed, %d reused (unchanged file/LLM/mode)%s"
            % (m, h, "  [CC_NO_CACHE: off]" if _NO_CACHE else ""))


def reset_cache_stats():
    _CACHE_STATS["hit"] = 0
    _CACHE_STATS["miss"] = 0


# ---------------------------------------------------------------------------
# Adversarial verify — a small local model over-reports; double-check each finding
# ---------------------------------------------------------------------------
_VERIFY_SYS = (
    "You are a strict reviewer double-checking a colleague's code-review finding "
    "against the shown code. Reply on ONE line: 'REJECTED <reason>' if the finding "
    "is wrong, a false positive, a pure nitpick, or not actually about the shown "
    "function; 'DOWNGRADE <reason>' if the issue is real but its HIGH/CRITICAL "
    "severity is not PROVEN by the shown code; otherwise 'CONFIRMED <reason>'. "
    "Confirm a HIGH/CRITICAL severity only when you are SURE.")


def verify_finding(idx, fi, finding, model, ctx=None):
    """One adversarial verify turn -> 'confirmed' / 'rejected' / 'downgrade' /
    'error' (transport failure: the caller decides — a finding is never silently
    dropped for it, but an unconfirmable HIGH/CRITICAL is downgraded). `ctx` is
    the SAME per-turn view the finder saw (headers + types + tidy + callers +
    body + branch) so the verifier has ALL the data to judge; without it, falls
    back to the bare body."""
    if not ctx:
        body, _ = codetree.source_body(fi.file, fi.line)
        ctx = body[:6000]
    msg = [{"role": "system", "content": _VERIFY_SYS},
           {"role": "user", "content":
            "Function %s (%s:%d):\n%s\n\nFinding to check:\n%s\n\n"
            "CONFIRMED, DOWNGRADE or REJECTED?"
            % (fi.qual_name, os.path.relpath(fi.file, REPO_ROOT), fi.line, ctx,
               finding)}]
    try:
        ans = (common.chat_with(model, msg) if model else common.chat(msg)) or ""
    except Exception:
        return "error"
    first = ans.upper().split("\n", 1)[0]
    if "REJECTED" in first:
        return "rejected"
    if "DOWNGRADE" in first:
        return "downgrade"
    return "confirmed"


def collapse_repetition(text, max_repeats=3, max_chars=8000):
    """Defang an LLM text-loop: collapse >max_repeats consecutive identical lines and
    hard-cap the total length. A small model sometimes spews the same line thousands
    of times (despite repeat_penalty); left unchecked that bloats the agentic
    conversation (eating context) and the findings output. Applied to every reply
    before it is stored or appended."""
    if not text:
        return text
    out = []
    prev = None
    run = 0
    for ln in text.splitlines():
        if ln == prev:
            run += 1
            if run <= max_repeats:
                out.append(ln)
        else:
            if run > max_repeats:
                out.append("... [%d identical lines collapsed]" % (run - max_repeats))
            prev = ln
            run = 1
            out.append(ln)
    if run > max_repeats:
        out.append("... [%d identical lines collapsed]" % (run - max_repeats))
    s = "\n".join(out)
    if len(s) > max_chars:                             # catches a newline-less loop too
        s = s[:max_chars] + "\n... [reply truncated at %d chars]" % max_chars
    return s


def build_views(idx, fi):
    """Yield (label, context_text) — one SMALL per-turn view for the function:
    a base view (headers + callers + body), then ONE callee branch per turn."""
    rel = os.path.relpath(fi.file, REPO_ROOT)
    hdrs = headers_for(fi)
    hdr_block = "".join("=== HEADER %s ===\n%s\n\n" % (r, t) for r, t in hdrs)
    # cap the caller tree: a hub function has hundreds of callers, which would
    # otherwise blow the per-turn budget the whole point is to keep small.
    caller_tree = _cap(codetree.TreeRender.caller_tree(idx, fi.qual_name, depth=4),
                       _CALLER_TREE_CAP, "caller tree")
    body = _body(fi, _BODY_CAP)
    # ALGORITHMIC input: clang-derived param/local types as one compact comment
    # (capped) so the model knows the types without the full headers (and we don't
    # saturate the small context).
    types = _var_types(fi)
    tblock = ("// param/local types (clang): "
              + ", ".join("%s=%s" % (n, t) for n, t in types.items()) + "\n") if types else ""
    # ALGORITHMIC: clang-tidy / static-analyzer findings in this function, so the
    # model reasons over VERIFIED facts, not just guesses. Cache-only here (the
    # audit loops prewarm_tidy first) so build_views never blocks on clang-tidy.
    tidy = tidy_for_function(fi, run=False)
    tdblock = ("// static analysis (clang-tidy) here:\n"
               + "".join("//   L%d %s [%s]\n" % (ln, m[:110], c)
                         for ln, m, c in tidy[:8])) if tidy else ""
    base = (
        "%s=== CALLERS (who reaches this function) ===\n%s\n\n"
        "=== REVIEW THIS FUNCTION: %s  (%s:%d) ===\n%s%s%s\n"
        % (hdr_block, caller_tree, fi.qual_name, rel, fi.line, tblock, tdblock, body))

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
def audit_function(idx, fi, model=None, system=CHECK_SYSTEM, dry=False, verify=False):
    """Audit ONE function with the limited view, one callee branch per turn.
    Returns a list of finding lines, each carrying a SEVERITY(LOW|MEDIUM|HIGH|
    CRITICAL) tag. INCREMENTAL: the verdict (the LLM findings, ADVERSARIALLY
    VERIFIED when verify=True — and a HIGH/CRITICAL finding is ALWAYS verified,
    downgraded to MEDIUM when unconfirmed) is cached by (model, function-source,
    system, verify), so a re-run skips unchanged functions DETERMINISTICALLY.
    dry=True: no IA — per-turn view SIZES."""
    views = list(build_views(idx, fi))                 # materialize: cache key + loop
    if dry:
        return ["[dry %s] view=%d chars (~%d tok)" % (label, len(ctx), len(ctx) // 4)
                for label, ctx in views]
    material = [model or "", system, "V" if verify else "", TIDY_CHECKS,
                "".join(c for _, c in views)]
    cached = verdict_get(material)
    if cached is not None:
        return cached                                  # caller STILL prints these
    _CACHE_STATS["miss"] += 1
    findings = []                                      # IA findings (the file_sweep
    view_of = {}                                       # gives the deterministic ones)
    for label, ctx in views:
        messages = [{"role": "system", "content": system},
                    {"role": "user", "content": ctx}]
        try:
            answer = common.chat_with(model, messages) if model else common.chat(messages)
        except Exception as exc:                       # transport error: note + go on
            findings.append("[chat error %s: %s]" % (label, exc))
            continue
        a = collapse_repetition((answer or "").strip())
        if a and "NO ISSUES" not in a.upper():
            f = "[%s]\n%s" % (label, a)
            findings.append(f)
            view_of[f] = ctx                           # the verifier sees what the finder saw
    # Adversarial check. Ordinary findings are verified when verify=True; a
    # HIGH/CRITICAL finding is ALWAYS verified (even under CC_NO_VERIFY) — that
    # level claims SURE — and is DOWNGRADED to MEDIUM when the check cannot
    # confirm it (rejected findings drop; transport errors never drop a finding).
    checked = []
    for f in findings:
        if f.startswith(("[clang-tidy]", "[chat error")):
            checked.append(f)
        else:
            high = _SEV_ORDER.get(finding_severity(f), 0) >= _SEV_ORDER["HIGH"]
            if not verify and not high:
                checked.append(f)
            else:
                v = verify_finding(idx, fi, f, model, ctx=view_of.get(f))
                if v == "confirmed":
                    checked.append(f)
                elif v == "rejected":
                    pass                               # a verified false positive: drop
                elif high:                             # downgrade / error: not sure
                    checked.append(downgrade_severity(
                        f, "the adversarial check did not confirm it"))
                else:
                    checked.append(f)                  # error on a low finding: keep
    findings = checked
    if not any(f.startswith("[chat error") for f in findings):
        verdict_put(material, findings)                # don't cache a transient error
    return findings


# ---------------------------------------------------------------------------
# Driver
# ---------------------------------------------------------------------------
def run(scope=None, only_file=None, only_func=None, limit=None,
        model=None, dry=False, out=sys.stdout):
    # `model` is an explicit LLM spec (mandatory): a model[@url] or claude-cli —
    # passed straight to chat_with via audit_function (handles the @url pin + CLI).
    idx = build_index(scope)
    # 1. COMPREHENSIVE deterministic sweep FIRST — every function in every file, not
    #    just the codetree-indexed few — the bulk of the "where & how" improvements.
    if not dry and not only_func:
        emit_sweep(file_sweep([only_file] if only_file else scope), out)
        out.write("\n")
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
    if not dry:
        funcs = audit_targets(funcs)                   # skip trivial functions
        prewarm_types(funcs)
        prewarm_tidy(funcs)
    reset_cache_stats()
    verify = not dry and not os.environ.get("CC_NO_VERIFY", "").strip()
    # 2. IA review on top (judgment the deterministic checks can't give).
    out.write("[codecheck] %s: IA review of %d function(s) %s\n"
              % (_ts_safe(), len(funcs), "(dry)" if dry else ""))
    total = 0
    sev = {"CRITICAL": 0, "HIGH": 0, "MEDIUM": 0, "LOW": 0, "unrated": 0}
    for fi in funcs:
        fl = audit_function(idx, fi, model=model, dry=dry, verify=verify)
        if fl:
            out.write("\n#### %s  (%s:%d)\n"
                      % (fi.qual_name, os.path.relpath(fi.file, REPO_ROOT), fi.line))
            for line in fl:
                out.write(line + "\n")
                if not dry and not line.startswith(("[chat error",)):
                    if count_severities(line, sev) == 0:
                        sev["unrated"] += 1
            total += 0 if dry else len(fl)
    out.write("\n[codecheck] done; %d finding-block(s); severity: %d CRITICAL, "
              "%d HIGH, %d MEDIUM, %d LOW, %d unrated; %s\n"
              % (total, sev["CRITICAL"], sev["HIGH"], sev["MEDIUM"], sev["LOW"],
                 sev["unrated"], cache_summary()))
    return 0


def run_panel(specs, scope=None, only_file=None, only_func=None, limit=None,
              out=sys.stdout):
    """Multi-IA agentic general-QA review over the EXPLICIT `specs` (>= 1 LLM, NEVER
    auto). Each function is reviewed independently by every IA (agentic: one branch
    at a time, can pull more data); the IAs discuss and form consensus WORKGROUPS,
    and each workgroup redacts a brief. Output per function ONLY when a workgroup
    has something to say."""
    import agentic                                  # lazy: avoids an import cycle
    idx = build_index(scope)
    # Comprehensive deterministic sweep first (every function, not just indexed).
    if not only_func:
        emit_sweep(file_sweep([only_file] if only_file else scope), out)
        out.write("\n")
    if only_func:
        funcs = [idx.by_name[only_func]] if only_func in idx.by_name else []
    elif only_file:
        funcs = idx.functions_in(only_file)
    else:
        funcs = leaves_first(idx)
    if limit:
        funcs = funcs[:limit]
    funcs = audit_targets(funcs)                       # skip trivial functions
    prewarm_types(funcs)
    prewarm_tidy(funcs)
    reset_cache_stats()
    out.write("[codecheck-panel] %d IA(s), %d function(s)\n" % (len(specs), len(funcs)))
    spoke = 0
    sev = {"CRITICAL": 0, "HIGH": 0, "MEDIUM": 0, "LOW": 0, "unrated": 0}
    for fi in funcs:
        briefs = agentic.audit_function(idx, fi, specs, role="codecheck",
                                        sysprompt=CHECK_SYSTEM)
        if briefs:
            spoke += 1
            out.write("\n#### %s  (%s:%d)\n"
                      % (fi.qual_name, os.path.relpath(fi.file, REPO_ROOT), fi.line))
            for b in briefs:
                out.write(b + "\n")
                if count_severities(b, sev) == 0:
                    sev["unrated"] += 1
    out.write("\n[codecheck-panel] %d/%d function(s) had something to say; severity: "
              "%d CRITICAL, %d HIGH, %d MEDIUM, %d LOW, %d unrated; %s\n"
              % (spoke, len(funcs), sev["CRITICAL"], sev["HIGH"], sev["MEDIUM"],
                 sev["LOW"], sev["unrated"], cache_summary()))
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
    p.add_argument("--llm", action="append", default=[],
                   help="an LLM to use — REQUIRED, repeatable. A model (gemma4:12b), "
                        "a model pinned to a backend (gemma4:12b@http://gpu1:11434), "
                        "or 'claude' (the official claude CLI). Also reads "
                        "CC_IA_PANEL. >= 2 LLMs => panel/workgroup. NEVER auto-chosen.")
    p.add_argument("--sweep", action="store_true",
                   help="DETERMINISTIC only: comprehensive clang-tidy over every "
                        "file/function (no LLM needed) — the fast 'where & how' list.")
    p.add_argument("--dry", action="store_true",
                   help="don't call the IA; print per-turn view sizes (verify the "
                        "context stays small). No LLM needed.")
    p.add_argument("--panel", action="store_true",
                   help="force the multi-IA agentic engine (discuss -> consensus "
                        "workgroups -> per-function briefs). Implied with >= 2 LLMs.")
    args = p.parse_args(argv[1:])
    import agentic                                  # lazy: avoids an import cycle
    scope = ([os.path.join(REPO_ROOT, s) if not os.path.isabs(s) else s
              for s in args.scope.split(",")] if args.scope else None)
    if args.sweep:                                  # deterministic, no LLM
        emit_sweep(file_sweep([args.file] if args.file else scope))
        return 0
    # Explicit LLMs from --llm + CC_IA_PANEL (deduped, order-preserving). MANDATORY.
    specs, seen = [], set()
    for s in agentic.parse_llms(",".join(args.llm)) + agentic.resolve_llms():
        if s not in seen:
            seen.add(s)
            specs.append(s)
    if args.dry:                                    # deterministic view-size check
        return run(scope=scope, only_file=args.file, only_func=args.func,
                   limit=args.limit, model=(specs[0] if specs else None), dry=True)
    if not specs:
        sys.stderr.write(
            "error: at least one LLM is MANDATORY and is never auto-chosen — pass "
            "--llm <model[@ollama-url]> (repeatable) or --llm claude (the official "
            "CLI), or set CC_IA_PANEL. Two or more LLMs => panel/workgroup.\n")
        return 2
    if args.panel or len(specs) >= 2:
        return run_panel(specs, scope=scope, only_file=args.file,
                         only_func=args.func, limit=args.limit)
    return run(scope=scope, only_file=args.file, only_func=args.func,
               limit=args.limit, model=specs[0], dry=False)


if __name__ == "__main__":
    # Re-enter through the REAL module name so agentic's `import codecheck` resolves
    # to the SAME module object. Run as __main__ alone, `import codecheck` would
    # build a SECOND codecheck with its own globals (cache dirs, _CACHE_STATS),
    # silently diverging the cache location and the recomputed/reused counter.
    import codecheck as _cc
    sys.exit(_cc.main(sys.argv))
