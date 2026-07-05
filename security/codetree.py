#!/usr/bin/env python3
"""codetree.py — clang/LLVM-IR-based code tree exploration (in-code, NOT IA).

Builds a function/method index + forward+reverse call graph from LLVM IR
(`clang -S -emit-llvm -g`), then renders 5-depth caller/callee trees in
plain text.  All work is done IN CODE — the IA only receives the pre-built
tree text, slashing context per turn.

Used by:
  server.py    : refactored to audit function-by-function (reduces IA context)
  codecheck.py : general quality review (bugs/perf/logic/duplication/name-vs-purpose)

Imports: from common import * for _ts() (optional); uses only stdlib + c++filt.
"""

import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import threading
from concurrent.futures import ThreadPoolExecutor, as_completed

REPO_ROOT = "/home/user/Desktop/CatchChallenger/git"
OUTPUT_ROOT = "/mnt/data/perso/tmpfs/security/server"
CLANG = shutil.which("clang")

SCOPE_DIRS = tuple(os.path.join(REPO_ROOT, p) for p in (
    "general/base", "server"))
SOURCE_EXT = (".cpp", ".c", ".cc", ".cxx", ".h", ".hpp", ".hxx")

# Known compile DB dirs (fast path, no cmake needed)
_CDB_PATHS = [
    "/tmp/cc-security-cli-compiledb/compile_commands.json",
]
_CDB_LOCK = threading.Lock()
_CDB_FILES = None   # {realpath: command_string} once merged

# IR + function index cache
_FUNCS_FILE = os.path.join(OUTPUT_ROOT, "ast-cache", "func_index.json")
_CALLS_FILE = os.path.join(OUTPUT_ROOT, "ast-cache", "call_index.json")

# IR cache
_IR_CACHE_DIR = os.path.join(OUTPUT_ROOT, "ast-cache", "ir")
os.makedirs(_IR_CACHE_DIR, exist_ok=True)
_BUILD_CDB_ONLY = False  # process all scope dirs, not just compile-DB files


def set_cdb_only(yes=True):
    global _BUILD_CDB_ONLY
    _BUILD_CDB_ONLY = yes
VENDOR_DIRS = tuple(os.path.realpath(os.path.join(REPO_ROOT, p)) for p in (
    "general/blake3", "general/hps", "general/libxxhash", "general/libzstd",
    "general/tinyXML2",
    "client/libqtcatchchallenger/libogg",
    "client/libqtcatchchallenger/libopus",
    "client/libqtcatchchallenger/libopusfile",
    "client/libqtcatchchallenger/libtiled",
))


def is_vendor(path):
    rp = os.path.realpath(path)
    return any(rp == d or rp.startswith(d + os.sep) for d in VENDOR_DIRS)


def _ts():
    import time
    return time.strftime("%H:%M %d/%m/%Y")


# ---------------------------------------------------------------------------
# Compile DB loader (from known paths only — no cmake)
# ---------------------------------------------------------------------------
def _load_cdb():
    """Merge all known compile_commands.json into {realpath: command_string}.
    No cmake — only checks preexisting DBs."""
    global _CDB_FILES
    if _CDB_FILES is not None:
        return _CDB_FILES
    with _CDB_LOCK:
        if _CDB_FILES is not None:
            return _CDB_FILES
        merged = {}
        for p in _CDB_PATHS:
            if not os.path.isfile(p):
                continue
            try:
                entries = json.load(open(p))
            except (OSError, ValueError):
                continue
            for e in entries:
                f = e.get("file")
                if f:
                    merged.setdefault(os.path.realpath(f), e.get("command", ""))
        _CDB_FILES = merged
        return merged


def flags_for(path):
    """Extract -I/-D/-std flags from the compile DB command for `path`.
    Falls back to common project flags when the file is not in any known DB."""
    real = os.path.realpath(path)
    cmd = _load_cdb().get(real, "")
    if not cmd:
        return _common_flags()
    cmd = re.sub(r'^\S+\s+', '', cmd)
    cmd = re.sub(r'-o\s+\S+\s*', '', cmd)
    cmd = re.sub(r'\s+CMakeFiles/\S+\.dir/\S+', '', cmd)
    parts = []
    for p in cmd.split():
        if p.startswith(('-I', '-D', '-std', '-f', '-W', '-O', '-m', '-g')):
            parts.append(p)
    return ' '.join(parts)


_FLAGS_LOCK = threading.Lock()
_FLAGS_CACHED = None


def _common_flags():
    """Extract -I (all), -D/-std (majority) flags from the CDB.  Cached."""
    global _FLAGS_CACHED
    if _FLAGS_CACHED is not None:
        return _FLAGS_CACHED
    with _FLAGS_LOCK:
        if _FLAGS_CACHED is not None:
            return _FLAGS_CACHED
        include_set = set()
        def_counts = {}
        std_set = set()
        for path, cmd in _load_cdb().items():
            for p in cmd.split():
                if p.startswith('-I'):
                    include_set.add(p)
                elif p.startswith('-D'):
                    def_counts[p] = def_counts.get(p, 0) + 1
                elif p.startswith('-std'):
                    std_set.add(p)
        threshold = max(1, len(_load_cdb()) // 2)
        defines = sorted(p for p, c in def_counts.items() if c >= threshold)
        includes = sorted(include_set)
        stden = sorted(std_set)
        _FLAGS_CACHED = ' '.join(stden + includes + defines)
        return _FLAGS_CACHED


# ---------------------------------------------------------------------------
# LLVM IR extraction
# ---------------------------------------------------------------------------
def ir_for(path):
    """Compile one TU to LLVM IR text.  Returns (ir_text, stderr) or
    ('', err_msg).  Uses `clang -S -emit-llvm -g -O0` with the file's
    compile flags (from the DB), or bare clang with -std=gnu++23 as fallback."""
    if not CLANG:
        return ('', "clang not found")
    flags = flags_for(path)
    cmd = [CLANG, '-S', '-emit-llvm', '-g', '-O0', '-o', '-']
    if flags:
        cmd += flags.split()
    else:
        cmd += ['-std=gnu++23']
    cmd.append(path)
    try:
        res = subprocess.run(cmd, capture_output=True, text=True, timeout=25)
    except subprocess.TimeoutExpired:
        return ('', 'timeout')
    except (OSError, subprocess.SubprocessError) as exc:
        return ('', str(exc))
    if res.returncode != 0:
        return ('', res.stderr[-1000:])
    return (res.stdout, '')


_CACHE_LOCK = threading.Lock()


def _ir_cached(path):
    """Return cached IR or compile fresh.  Uses file mtime as cache key.
    Returns (ir_text, '') or ('', err_msg)."""
    real = os.path.realpath(path)
    try:
        mtime = os.path.getmtime(real)
    except OSError:
        return ('', 'cannot stat ' + real)
    cache_file = os.path.join(_IR_CACHE_DIR,
                              os.path.basename(real) + '.'
                              + format(mtime, '.9f').replace('.', 'x')
                              + '.ir')
    # Fast path: cache hit
    try:
        with open(cache_file, 'r', errors='replace') as fh:
            ir = fh.read()
            if ir:
                return (ir, '')
    except (OSError, ValueError):
        pass
    # Slow path: compile
    ir, err = ir_for(real)
    if not ir:
        return ('', err)
    with _CACHE_LOCK:
        try:
            with open(cache_file, 'w') as fh:
                fh.write(ir)
        except OSError:
            pass
    return (ir, '')


# ---------------------------------------------------------------------------
# IR parser: extract function definitions + call edges
# ---------------------------------------------------------------------------
# Match a mangled name after '@' on a define line (handles nested parens
# in the parameter list).  The `^define` anchor ensures we only capture
# function *definitions*, not call sites.
_DEF_RE = re.compile(
    r'^define\s+[^@]*@\"?([A-Za-z_0-9$<>]+)\"?\('
    r'(?:[^()]*(?:\([^()]*\)[^()]*)*)\)\s*(?:unnamed_addr|#\d+|!dbg|{)',
    re.MULTILINE)
# Call instructions: @<mangled>(
_CALL_RE = re.compile(
    r'\bcall\b\s+\S+\s+@\"?([A-Za-z_0-9$<>]+)\"?\(', re.MULTILINE)
# DILocation metadata: line number from !dbg
_DBG_MD = re.compile(r'!(\d+)\s*=\s*!DILocation\(line:\s*(\d+),', re.MULTILINE)
# Map from IR metadata ID to line number for a function (also DISubprogram)
_DBG_SUBPROG = re.compile(
    r'!(\d+)\s*=\s*distinct\s+!DISubprogram\([^)]*?line:\s*(\d+)', re.MULTILINE)


class FuncInfo:
    """One function/method extracted from LLVM IR."""
    __slots__ = ("name", "demangled", "file", "line", "end_line",
                 "kind", "class_name")

    def __init__(self, name, demangled, file, line, end_line,
                 kind="FunctionDecl", class_name=""):
        self.name = name            # mangled name
        self.demangled = demangled  # human-readable (no params)
        self.file = file
        self.line = line
        self.end_line = end_line
        self.kind = kind
        self.class_name = class_name

    def __repr__(self):
        return "Func(%s @ %s:%d)" % (self.demangled, self.file, self.line)

    @property
    def qual_name(self):
        """The demangled function name without parameter types,
        e.g. 'CatchChallenger::Client::handle'."""
        # Remove everything after (including) the first '('
        paren = self.demangled.find('(')
        if paren >= 0:
            return self.demangled[:paren].strip()
        return self.demangled.strip()


def _demangle(name):
    """Demangle a single mangled name, returning the full string.
    Returns the original on failure."""
    if not name or not name.startswith('_Z'):
        return name
    try:
        r = subprocess.run(['c++filt', '-p'], input=name,
                           capture_output=True, text=True, timeout=5)
        return r.stdout.strip() if r.stdout.strip() else name
    except (OSError, subprocess.SubprocessError):
        return name


def _extract_line_map(ir_text):
    """Build {dbg_id: line_number} from DILocation + DISubprogram metadata."""
    m = {}
    for match in _DBG_MD.finditer(ir_text):
        m[match.group(1)] = int(match.group(2))
    for match in _DBG_SUBPROG.finditer(ir_text):
        m[match.group(1)] = int(match.group(2))
    return m


def parse_function_defs(ir_text, src_file):
    """Extract function definitions and call edges from LLVM IR text.

    Returns (funcs: [FuncInfo], calls: [(caller_qual, callee_qual, call_line)]).

    Performance: builds a position-sorted definition index on a single IR scan,
    then resolves caller→callee via bisect (O(log N)), not O(N²).
    """
    import bisect

    line_map = _extract_line_map(ir_text)

    # ---- build definition index (one regex scan) ----
    # Each entry: (start_pos, end_pos, mname, demangled, dbg_id)
    defs = []
    for match in _DEF_RE.finditer(ir_text):
        mname = match.group(1)
        demangled = _demangle(mname)
        if demangled.startswith(('std::', '__cxa', '__gnu', '_ZSt', 'llvm::')):
            continue
        # Get source line from full physical line
        ms = ir_text.rfind('\n', 0, match.start()) + 1
        me = ir_text.find('\n', match.end())
        full = ir_text[ms:me if me >= 0 else len(ir_text)]
        dbg = re.search(r'!dbg\s*!(\d+)', full)
        dbg_id = dbg.group(1) if dbg else None
        end_pos = match.end()
        defs.append((match.start(), end_pos, mname, demangled, dbg_id))

    # Sort by start position (they should already be in order, but be safe)
    defs.sort(key=lambda x: x[0])

    # ---- second pass: build FuncInfo list ----
    funcs = []
    # Cache demangled lookups
    demangled_cache = {}
    for start, end, mname, demangled, dbg_id in defs:
        line = line_map.get(dbg_id, 0) if dbg_id else 0
        qname = FuncInfo(mname, demangled, "", 0, 0).qual_name
        cls = ""
        if "::" in qname:
            parts = qname.split("::")
            if len(parts) >= 2:
                cls = parts[-2] if len(parts) >= 3 else ""
        fi = FuncInfo(mname, demangled, src_file, line, 0, class_name=cls)
        funcs.append(fi)
        demangled_cache[mname] = demangled

    # Build a list of definition start positions for bisect lookup
    def_starts = [d[0] for d in defs]
    # For each definition entry: also the end-of-line position is `end`
    # We'll use (pos, mname) pairs; bisect the sorted start positions

    # ---- extract call edges via binary search ----
    calls = []
    for cmatch in _CALL_RE.finditer(ir_text):
        callee = cmatch.group(1)
        if not callee:
            continue
        if callee.startswith(("llvm.", "__", "printf", "memset", "memcpy",
                               "memmove", "strlen", "strcpy", "malloc",
                               "free", "realloc", "calloc", "assert",
                               "fprintf", "sprintf", "snprintf")):
            continue
        callee_dm = demangled_cache.get(callee) or _demangle(callee)
        demangled_cache[callee] = callee_dm
        if not callee_dm:
            continue
        if callee_dm.startswith(('std::', '__cxa', '__gnu', '_ZSt')):
            continue

        # Find enclosing definition: largest start_pos <= cmatch.start()
        idx = bisect.bisect_right(def_starts, cmatch.start()) - 1
        if idx < 0:
            continue
        caller_name = defs[idx][2]
        if caller_name == callee:
            continue

        qcaller = FuncInfo(caller_name, demangled_cache[caller_name], "", 0, 0).qual_name
        qcallee = FuncInfo(callee, callee_dm, "", 0, 0).qual_name
        if qcaller == qcallee:
            continue

        # Get call line from !dbg on the full line
        cl_start = ir_text.rfind('\n', 0, cmatch.start()) + 1
        cl_end = ir_text.find('\n', cmatch.end())
        cl_full = ir_text[cl_start:cl_end if cl_end >= 0 else len(ir_text)]
        dbg_cm = re.search(r'!dbg\s*!(\d+)', cl_full)
        cl = 0
        if dbg_cm:
            cl = line_map.get(dbg_cm.group(1), 0)

        calls.append((qcaller, qcallee, cl))

    return funcs, calls


# ---------------------------------------------------------------------------
# Source body extraction (brace matching)
# ---------------------------------------------------------------------------
def source_body(file_path, start_line):
    """Extract a function body from source starting at `start_line`.

    Brace-matches from the first real '{' to the '}' that brings nesting depth back
    to zero. A single character scanner tracks // line comments, /* */ block
    comments (which span lines), and "..."/'...' literals, so a '{' or '}' that
    lives INSIDE a comment or a literal never moves the depth. The old line-prefix
    "skip whole-comment-lines" rule miscounted a brace in a TRAILING comment, a
    multi-line block-comment body, a string ("[a-z]{2,4}$") or a char literal
    ('{'/'}') — cutting the body short at a fake brace, so the reviewer saw a
    TRUNCATED function. Returns (body_text, end_line) or ('', start_line) on
    failure. (Raw strings R"(...)" are the only residual edge — rare here.)"""
    try:
        lines = open(file_path, "r", errors="replace").readlines()
    except OSError:
        return ("", start_line)
    if start_line < 1 or start_line > len(lines):
        return ("", start_line)
    depth = 0
    opened = False
    in_block = False                 # inside a /* ... */ block (persists across lines)
    in_str = None                    # the quote char while inside "..." or '...'
    lineno = start_line - 1
    while lineno < len(lines):
        line = lines[lineno]
        n = len(line)
        col = 0
        while col < n:
            ch = line[col]
            nxt = line[col + 1] if col + 1 < n else ''
            if in_block:
                if ch == '*' and nxt == '/':
                    in_block = False
                    col += 2
                    continue
            elif in_str is not None:
                if ch == '\\':                       # escape: skip the escaped char
                    col += 2
                    continue
                if ch == in_str:                     # closing quote
                    in_str = None
            else:
                if ch == '/' and nxt == '/':
                    break                            # // comment -> rest of line ignored
                if ch == '/' and nxt == '*':
                    in_block = True
                    col += 2
                    continue
                if ch == '"':
                    in_str = '"'
                elif ch == "'":
                    # A char literal 'X' or '\e' — skip it WHOLE so a '{'/'}'/'"'
                    # inside cannot move the depth. A lone ' that is not a well-formed
                    # literal (e.g. a C++ digit separator 1'000) falls through as an
                    # ordinary char, so it can't open a phantom string.
                    if nxt == '\\':
                        j = line.find("'", col + 3)  # past the escaped char
                        col = (j + 1) if j >= 0 else (col + 1)
                        continue
                    if col + 2 < n and line[col + 2] == "'":
                        col += 3
                        continue
                elif ch == '{':
                    depth += 1
                    opened = True
                elif ch == '}' and opened:           # a '}' before the body opens is a
                    depth -= 1                        # stray close (misattributed start
                    if depth == 0:                   # line / #ifdef scope) — ignore it
                        body = "".join(lines[start_line - 1:lineno + 1])
                        return (body, lineno + 1)
            col += 1
        lineno += 1
    return ("", start_line)


# ---------------------------------------------------------------------------
# #ifdef guard tracker (line-scan)
# ---------------------------------------------------------------------------
class IfdefMap:
    def __init__(self):
        self._cache = {}

    def _scan(self, path):
        real = os.path.realpath(path)
        if real in self._cache:
            return self._cache[real]
        try:
            text = open(path, "r", errors="replace").read()
        except OSError:
            self._cache[real] = []
            return []
        out = []
        for i, line in enumerate(text.splitlines(), 1):
            s = line.strip()
            if s.startswith("#if "):
                out.append((i, s[4:].strip(), "if"))
            elif s.startswith("#ifdef "):
                out.append((i, s[7:].strip(), "ifdef"))
            elif s.startswith("#ifndef "):
                out.append((i, s[7:].strip(), "ifndef"))
            elif s.startswith("#elif "):
                out.append((i, s[6:].strip(), "elif"))
            elif s == "#else":
                out.append((i, "", "else"))
            elif s == "#endif":
                out.append((i, "", "endif"))
        self._cache[real] = out
        return out

    def guards(self, path, line):
        items = self._scan(path)
        stack = []
        for lno, cond, kind in items:
            if kind == "endif":
                if stack:
                    stack.pop()
            elif kind == "else" or kind.startswith("elif"):
                if stack:
                    stack.pop()
                    stack.append((lno, cond, kind))
            else:
                stack.append((lno, cond, kind))
            if lno > line:
                break
        active = [s for s in stack if s[2] in ("if", "ifdef", "ifndef",
                                                "elif", "else")]
        return list(reversed(active))


# ---------------------------------------------------------------------------
# Cross-TU index (built once, from per-TU IR results)
# ---------------------------------------------------------------------------
class Index:
    """Cross-TU function index + call graph, built from LLVM IR parsing.

    All calls: index.build() -> then query by qual_name.
    """

    def __init__(self, index_file=None):
        self.index_file = index_file or _FUNCS_FILE
        self.calls_file = _FUNCS_FILE.replace("func_index", "call_index")
        self.by_name = {}      # qual_name -> FuncInfo
        self.def_loc = {}      # qual_name -> (file, line)
        self._forward_callees = {}   # caller_qual -> {callee_qual: [line,...]}
        self._reverse_callers = {}   # callee_qual -> {caller_qual: [line,...]}
        self._lock = threading.Lock()
        self._built = False

    def _collect_sources(self):
        seen = set()
        out = []
        cdb_files = set(_load_cdb().keys()) if _BUILD_CDB_ONLY else None
        for d in SCOPE_DIRS:
            if not os.path.isdir(d):
                continue
            for root, _dirs, names in os.walk(os.path.realpath(d)):
                if is_vendor(root):
                    continue
                for n in sorted(names):
                    if n.endswith((".cpp", ".cc", ".cxx")):
                        fp = os.path.realpath(os.path.join(root, n))
                        if fp not in seen:
                            seen.add(fp)
                            if cdb_files is None or fp in cdb_files:
                                out.append(fp)
        if cdb_files is not None and len(out) < 10:
            return [f for f in sorted(seen) if f in cdb_files or True]
        return sorted(out)

    def build(self, max_workers=8):
        if self._built:
            return
        with self._lock:
            if self._built:
                return
            sources = self._collect_sources()
            total = len(sources)
            sys.stderr.write("[codetree] indexing %d TUs (%d workers)\n"
                             % (total, max_workers))
            done = [0]
            lock = threading.Lock()
            errs = []

            def process(src):
                ir, err = _ir_cached(src)
                if not ir:
                    if err:
                        with lock:
                            errs.append((src, err))
                    return
                fi_lst, call_lst = parse_function_defs(ir, src)
                with lock:
                    for fi in fi_lst:
                        qn = fi.qual_name
                        if (qn not in self.by_name
                                or self.by_name[qn].line == 0):
                            self.by_name[qn] = fi
                            self.def_loc[qn] = (fi.file, fi.line)
                    for caller_q, callee_q, cl in call_lst:
                        self._forward_callees.setdefault(
                            caller_q, {}).setdefault(callee_q, []).append(cl)
                        self._reverse_callers.setdefault(
                            callee_q, {}).setdefault(caller_q, []).append(cl)
                    done[0] += 1
                    if done[0] % 50 == 0:
                        sys.stderr.write("  %d/%d TUs\n"
                                         % (done[0], total))

            with ThreadPoolExecutor(max_workers=max_workers) as pool:
                futures = [pool.submit(process, src) for src in sources]
                for f in as_completed(futures):
                    try:
                        f.result()
                    except BaseException as exc:
                        sys.stderr.write(
                            "  worker error: %s\n" % exc)

            if errs:
                for src, err in errs[:10]:
                    sys.stderr.write("  IR fail %s: %s\n"
                                     % (os.path.basename(src),
                                        err.replace('\n', ' ')[:120]))
                if len(errs) > 10:
                    sys.stderr.write("  ... %d more failures\n"
                                     % (len(errs) - 10))

            self._built = True
            sys.stderr.write("[codetree] index done: %d functions, "
                             "%d forward edges, %d failures\n"
                             % (len(self.by_name),
                                sum(len(v)
                                    for v in self._forward_callees.values()),
                                len(errs)))

    # ---- queries -----------------------------------------------------------

    def functions_in(self, file_path):
        self.build()
        real = os.path.realpath(file_path)
        return [f for f in self.by_name.values() if f.file == real]

    def callees_of(self, qual_name, depth=5):
        self.build()
        seen = set()
        result = {}

        def walk(qn, d):
            if d <= 0 or qn in seen:
                return
            seen.add(qn)
            for callee_q, lines in self._forward_callees.get(qn, {}).items():
                result.setdefault(callee_q, []).extend(lines)
                walk(callee_q, d - 1)

        walk(qual_name, depth)
        return result

    def callers_of(self, qual_name, depth=5):
        self.build()
        seen = set()
        result = {}

        def walk(qn, d):
            if d <= 0 or qn in seen:
                return
            seen.add(qn)
            for caller_q, lines in self._reverse_callers.get(qn, {}).items():
                result.setdefault(caller_q, []).extend(lines)
                walk(caller_q, d - 1)

        walk(qual_name, depth)
        return result

    def all_functions(self):
        self.build()
        return self.by_name.items()

    def count(self):
        self.build()
        return len(self.by_name)


# ---------------------------------------------------------------------------
# Tree renderer (deterministic, in code)
# ---------------------------------------------------------------------------
class TreeRender:
    @staticmethod
    def callee_tree(idx, qual_name, depth=5, ifdef=None):
        idx.build()
        callees = idx.callees_of(qual_name, depth)
        if not callees:
            return "  (no callees within %d depth)" % depth
        lines = ["=== Callee tree for %s (depth %d) ===" % (qual_name, depth)]
        seen = set()

        def render(qn, d, indent):
            if qn in seen:
                return
            seen.add(qn)
            for callee_q, call_lines in \
                    idx._forward_callees.get(qn, {}).items():
                if callee_q in seen:
                    continue
                loc = ""
                fi = idx.by_name.get(callee_q)
                if fi:
                    loc = " (%s:%d)" % (
                        os.path.relpath(fi.file, REPO_ROOT), fi.line)
                if ifdef and fi:
                    g = ifdef.guards(fi.file, fi.line)
                    if g:
                        loc += " [#%s]" % ", #".join(c for _, c, _ in g)
                lines.append(
                    "%s> %s%s  [lines %s]"
                    % (indent, callee_q, loc,
                       ",".join(str(l) for l in call_lines[:5])))
                render(callee_q, d - 1, indent + "  ")

        render(qual_name, depth, "")
        return "\n".join(lines)

    @staticmethod
    def caller_tree(idx, qual_name, depth=5, ifdef=None):
        idx.build()
        lines = ["=== Caller tree for %s (depth %d) ===" % (qual_name, depth)]
        seen = set()

        def render(qn, d, indent):
            if qn in seen:
                return
            seen.add(qn)
            for caller_q, call_lines in \
                    idx._reverse_callers.get(qn, {}).items():
                if caller_q in seen:
                    continue
                loc = ""
                fi = idx.by_name.get(caller_q)
                if fi:
                    loc = " (%s:%d)" % (
                        os.path.relpath(fi.file, REPO_ROOT), fi.line)
                if ifdef and fi:
                    g = ifdef.guards(fi.file, fi.line)
                    if g:
                        loc += " [#%s]" % ", #".join(c for _, c, _ in g)
                lines.append(
                    "%s< %s%s  [lines %s]"
                    % (indent, caller_q, loc,
                       ",".join(str(l) for l in call_lines[:5])))
                render(caller_q, d - 1, indent + "  ")

        render(qual_name, depth, "")
        return "\n".join(lines)

    @staticmethod
    def func_summary(idx, qual_name):
        """One function's full summary: location, guards, 5-depth trees,
        and body text."""
        idx.build()
        fi = idx.by_name.get(qual_name)
        if not fi:
            return "[not in index: %s]" % qual_name
        rel = os.path.relpath(fi.file, REPO_ROOT)
        ifd = IfdefMap()
        g = ifd.guards(fi.file, fi.line)
        guard_str = " [#%s]" % ", #".join(c for _, c, _ in g) if g else ""
        body, endline = source_body(fi.file, fi.line)

        parts = [
            "=== %s (%s:%d%s) ===" % (qual_name, rel, fi.line, guard_str),
            TreeRender.callee_tree(idx, qual_name, depth=5, ifdef=ifd),
            "",
            TreeRender.caller_tree(idx, qual_name, depth=5, ifdef=ifd),
        ]
        if body:
            parts.append("")
            clipped = body[:10000]
            parts.append("BODY:\n%s" % clipped)
            if len(body) > 10000:
                parts[-1] += "\n...[body truncated]..."

        return "\n".join(parts)
