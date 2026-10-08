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
import shlex
import shutil
import subprocess
import sys
import threading
from concurrent.futures import ThreadPoolExecutor, as_completed

REPO_ROOT = "/home/user/Desktop/CatchChallenger/git"
OUTPUT_ROOT = "/mnt/data/perso/tmp/security/server"
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
_SOURCE_STAMP = ""
os.makedirs(_IR_CACHE_DIR, exist_ok=True)
_BUILD_CDB_ONLY = False  # process all scope dirs, not just compile-DB files


def set_cdb_only(yes=True):
    global _BUILD_CDB_ONLY
    _BUILD_CDB_ONLY = yes
VENDOR_DIRS = tuple(os.path.realpath(os.path.join(REPO_ROOT, p)) for p in (
    "general/blake3", "general/hps", "general/libxxhash", "general/libzstd",
    "general/tinyXML2", "general/libzlib",
    "client/libqtcatchchallenger/libogg",
    "client/libqtcatchchallenger/libopus",
    "client/libqtcatchchallenger/libopusfile",
    "client/libqtcatchchallenger/libtiled",
))


def is_vendor(path):
    rp = os.path.realpath(path)
    return any(rp == d or rp.startswith(d + os.sep) for d in VENDOR_DIRS)


# Qt machine-generated sources: moc_*/qrc_*/ui_* and the *_autogen build trees
# CMake's AUTOMOC/AUTOUIC emit. They are NOT source of truth (regenerated from
# the real .hpp/.ui), often won't compile standalone (they #include headers from
# a build-dir-relative autogen path), and add zero attack surface. Never index.
_GEN_PREFIXES = ("moc_", "qrc_", "ui_", "mocs_compilation")


def is_generated(path):
    """True for a Qt-generated TU (moc_/qrc_/ui_ prefix) or any file inside a
    build/autogen tree (`*_autogen/`, `mocs_compilation*`, a CMake build dir)."""
    base = os.path.basename(path)
    if base.startswith(_GEN_PREFIXES):
        return True
    parts = os.path.realpath(path).split(os.sep)
    return any(seg.endswith("_autogen") or seg == "CMakeFiles" for seg in parts)


def _ts():
    import time
    return time.strftime("%H:%M %d/%m/%Y")


def refresh_source_stamp():
    """Invalidate derived evidence when any repository C/C++ dependency changes."""
    global _SOURCE_STAMP
    files = subprocess.run(['git', '-C', REPO_ROOT, 'ls-files', '-z', '--cached',
                            '--others', '--exclude-standard'], capture_output=True,
                           check=True).stdout.split(b'\0')
    digest = hashlib.sha256()
    for raw in sorted(set(files)):
        path = os.fsdecode(raw)
        if not path.endswith(SOURCE_EXT + ('.inc', '.inl', '.ipp', '.tpp')):
            continue
        full = os.path.join(REPO_ROOT, path)
        if is_generated(full):
            continue
        digest.update(raw + b'\0')
        try:
            with open(full, 'rb') as source:
                digest.update(hashlib.sha256(source.read()).digest())
        except FileNotFoundError:
            digest.update(b'deleted')
    _SOURCE_STAMP = digest.hexdigest()
    return _SOURCE_STAMP


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
        return _common_flags(real)
    cmd = re.sub(r'^\S+\s+', '', cmd)
    cmd = re.sub(r'-o\s+\S+\s*', '', cmd)
    cmd = re.sub(r'\s+CMakeFiles/\S+\.dir/\S+', '', cmd)
    # shlex, not split(): cmake writes shell-escaped defines (-DTILED_LIB_DIR=\"lib\"),
    # and a raw split hands clang the backslashes -> "missing terminating '\"'" kills
    # the whole TU. shlex unescapes them to -DTILED_LIB_DIR="lib". A token holding a
    # space cannot survive the string round-trip, so drop it rather than corrupt the
    # command line.
    try:
        tokens = shlex.split(cmd)
    except ValueError:
        tokens = cmd.split()
    return ' '.join(_keep_flags(tokens))


# Flags whose VALUE is the next token ("-isystem /usr/include/qt6"). cmake writes
# every Qt/system include that way, and a prefix-only filter dropped them all - so
# every Qt TU died on "'QThread' file not found" and never entered the index. They
# are re-joined into ONE token (clang accepts -isystem<dir>) so the flag list stays
# a plain space-joined string that survives sorting and re-splitting.
_FLAG_WITH_VALUE = ('-isystem', '-iquote', '-idirafter', '-include', '-imacros',
                    '-I', '-D', '--sysroot')
_FLAG_PREFIXES = ('-I', '-D', '-std', '-f', '-W', '-O', '-m', '-g', '-isystem',
                  '-iquote', '-idirafter', '-include', '-imacros', '--sysroot')


def _keep_flags(tokens):
    """The -I/-D/-std/-f/-W/-O/-m/-g flags of a compile command, separated-value
    forms folded into one token. A token holding a space cannot survive the string
    round-trip, so it is dropped rather than corrupting the command line."""
    out = []
    i = 0
    while i < len(tokens):
        t = tokens[i]
        if t in _FLAG_WITH_VALUE and i + 1 < len(tokens):
            # short options join directly (-isystem/usr/include), long ones with '='
            t += ("=" if t.startswith("--") else "") + tokens[i + 1]
            i += 1
        if t.startswith(_FLAG_PREFIXES) and ' ' not in t:
            out.append(t)
        i += 1
    return out


_FLAGS_LOCK = threading.Lock()
_FLAGS_CACHED = None


def _common_flags(path=None):
    """Extract -I (all), -D (majority) flags from the CDB, plus the -std matching
    `path`'s LANGUAGE.  Cached.

    The -std MUST be language-picked: the DB mixes C and C++ entries, and emitting
    both ("-std=gnu++23 -std=gnu11") makes clang reject every fallback TU outright
    ("invalid argument '-std=gnu11' not allowed with 'C++'"), so a source in no
    compile DB was never analysed at all."""
    global _FLAGS_CACHED
    if _FLAGS_CACHED is None:
        with _FLAGS_LOCK:
            if _FLAGS_CACHED is None:
                include_set = set()
                def_counts = {}
                std_set = set()
                for _p, cmd in _load_cdb().items():
                    try:
                        tokens = shlex.split(cmd)
                    except ValueError:
                        tokens = cmd.split()
                    for p in _keep_flags(tokens):
                        if p.startswith(('-I', '-isystem', '-iquote',
                                         '-idirafter')):
                            include_set.add(p)
                        elif p.startswith('-D'):
                            def_counts[p] = def_counts.get(p, 0) + 1
                        elif p.startswith('-std'):
                            std_set.add(p)
                threshold = max(1, len(_load_cdb()) // 2)
                _FLAGS_CACHED = (
                    sorted(p for p in std_set if '++' in p),      # C++ standards
                    sorted(p for p in std_set if '++' not in p),  # C standards
                    sorted(include_set),
                    sorted(p for p, c in def_counts.items() if c >= threshold))
    cxx_std, c_std, includes, defines = _FLAGS_CACHED
    std = c_std if (path and path.endswith('.c')) else cxx_std
    return ' '.join(std[-1:] + includes + defines)


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
    cmd = [CLANG, '-S', '-emit-llvm', '-o', '-']
    if flags:
        cmd += flags.split()
    else:
        cmd += ['-std=gnu++23']
    # Compile-DB release flags must not override the indexer's settings: inlining
    # and dead-code elimination hide functions/calls; -g0 hides source locations.
    cmd += ['-g', '-O0', path]
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
    """Return cached IR or compile fresh, keyed by source, mtime and flags.
    Returns (ir_text, '') or ('', err_msg)."""
    real = os.path.realpath(path)
    if is_generated(real):
        return ('', '')          # Qt-generated TU: skip silently, not an error
    try:
        mtime = os.path.getmtime(real)
    except OSError:
        return ('', 'cannot stat ' + real)
    # Version the extraction policy so old optimized IR is never reused. Include
    # path and flags: same-basename TUs and changed build defines are not equivalent.
    key = hashlib.sha256(json.dumps(
        ["unoptimized-v3", CLANG, real, flags_for(real), _SOURCE_STAMP]
    ).encode()).hexdigest()[:20]
    cache_file = os.path.join(_IR_CACHE_DIR,
                              os.path.basename(real) + '.' + key + '.'
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
# Only parse the symbol prefix, not the parameter/attribute grammar (nested
# initializes((0, 48)), local_unnamed_addr, align, etc.). Stay on the define line.
_DEF_RE = re.compile(
    r'^define\s+[^\n@]*@\"?([A-Za-z_0-9.$<>]+)\"?\(',
    re.MULTILINE)
# Call instructions: @<mangled>(
_CALL_RE = re.compile(
    r'^\s*(?:%[^\s=]+\s*=\s*)?(?:(?:tail|musttail|notail)\s+)?'
    r'(?:call|invoke)\s+[^\n@]*@\"?([A-Za-z_0-9.$<>]+)\"?\(', re.MULTILINE)
# DILocation metadata: line number from !dbg
_DBG_MD = re.compile(r'!(\d+)\s*=\s*!DILocation\(line:\s*(\d+),', re.MULTILINE)
# Parentheses inside quoted names (operator(), lambdas) are not metadata delimiters.
_DBG_SUBPROG_FULL = re.compile(
    r'!(\d+)\s*=\s*distinct\s+!DISubprogram\('
    r'((?:[^"()\n]|"(?:\\.|[^"\\])*")*)\)', re.MULTILINE)
_SUBPROG_LINE_RE = re.compile(r'\bline:\s*(\d+)')
_SUBPROG_FILE_RE = re.compile(r'\bfile:\s*!(\d+)')
_DBG_DIFILE = re.compile(
    r'!(\d+)\s*=\s*!DIFile\(filename:\s*"([^"]*)"'
    r'(?:,\s*directory:\s*"([^"]*)")?')
_STD_SYMBOL_RE = re.compile(r'^_Z(?:St|N[rVK]*[RO]?St)')


def _excluded_symbol(name, demangled):
    """Library internals and compiler trampolines are not source review targets."""
    return bool(_STD_SYMBOL_RE.match(name)) or demangled.startswith((
        'std::', '__cxa', '__gnu', 'hps::', 'llvm::',
        '__cxx_global_var_init', '__cxx_global_array_dtor', '_GLOBAL__sub_I_',
        'non-virtual thunk to ', 'virtual thunk to ', 'covariant return thunk to '))


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
        """Full signature: overloads and operator() must remain separate targets."""
        return self.demangled.strip()


def _demangle_many(names):
    """One c++filt process per TU instead of one process per symbol."""
    result = {name: name for name in names}
    mangled = sorted(name for name in result if name.startswith('_Z'))
    if not mangled:
        return result
    try:
        run = subprocess.run(['c++filt'], input='\n'.join(mangled) + '\n',
                             capture_output=True, text=True, timeout=30)
        values = run.stdout.splitlines()
        if run.returncode != 0 or len(values) != len(mangled):
            raise ValueError("c++filt returned incomplete symbol output")
        result.update(zip(mangled, values))
    except (OSError, subprocess.SubprocessError, ValueError) as exc:
        sys.stderr.write("[codetree] demangling unavailable: %s; keeping symbols\n" % exc)
    return result


def _extract_line_map(ir_text):
    """Build {dbg_id: line_number} from DILocation + DISubprogram metadata."""
    m = {}
    for match in _DBG_MD.finditer(ir_text):
        m[match.group(1)] = int(match.group(2))
    for match in _DBG_SUBPROG_FULL.finditer(ir_text):
        line = _SUBPROG_LINE_RE.search(match.group(2))
        if line:
            m[match.group(1)] = int(line.group(1))
    return m


def _extract_subprog_files(ir_text):
    """{dbg_id: definition file} for every DISubprogram carrying a DIFile.

    A method DEFINED IN A HEADER - inline, or a ctor/dtor the compiler generates -
    gets its `line:` from THAT HEADER, while the TU we compiled is the .cpp. Pairing
    the two slices an unrelated function's body out of the .cpp and hands it to the
    reviewer under the header function's name: an implicit `~UnknownObjectEntry`
    (CommonMap.hpp:27) was reviewed as Map_loaderMain.cpp:27, and the model reported
    a CRITICAL about code the function does not contain. The file must come from the
    same metadata as the line."""
    files = {}
    for m in _DBG_DIFILE.finditer(ir_text):
        name, directory = m.group(2), m.group(3) or ""
        files[m.group(1)] = (name if os.path.isabs(name)
                             else os.path.normpath(os.path.join(directory, name)))
    out = {}
    for m in _DBG_SUBPROG_FULL.finditer(ir_text):
        fid = _SUBPROG_FILE_RE.search(m.group(2))
        if fid is not None and fid.group(1) in files:
            out[m.group(1)] = files[fid.group(1)]
    return out


def parse_function_defs(ir_text, src_file):
    """Extract function definitions and call edges from LLVM IR text.

    Returns (funcs: [FuncInfo], calls: [(caller_qual, callee_qual, call_line)]).

    Performance: builds a position-sorted definition index on a single IR scan,
    then resolves caller→callee via bisect (O(log N)), not O(N²).
    """
    import bisect

    line_map = _extract_line_map(ir_text)
    subprog_files = _extract_subprog_files(ir_text)
    definitions = list(_DEF_RE.finditer(ir_text))
    call_sites = list(_CALL_RE.finditer(ir_text))
    demangled_cache = _demangle_many(m.group(1) for m in definitions + call_sites)

    # ---- build definition index (one regex scan) ----
    # Each entry: (start_pos, end_pos, mname, demangled, dbg_id)
    defs = []
    for match in definitions:
        mname = match.group(1)
        demangled = demangled_cache[mname]
        # Keep every boundary, including excluded functions: otherwise their calls
        # are incorrectly attributed to the previous project function by bisect.
        # Get source line from full physical line
        ms = ir_text.rfind('\n', 0, match.start()) + 1
        me = ir_text.find('\n', match.end())
        full = ir_text[ms:me if me >= 0 else len(ir_text)]
        if re.search(r'\b(?:internal|private)\b', full[:full.find('@')]):
            demangled += " [TU %s]" % os.path.relpath(src_file, REPO_ROOT)
        dbg = re.search(r'!dbg\s*!(\d+)', full)
        dbg_id = dbg.group(1) if dbg else None
        end_pos = match.end()
        defs.append((match.start(), end_pos, mname, demangled, dbg_id))

    # Sort by start position (they should already be in order, but be safe)
    defs.sort(key=lambda x: x[0])

    # ---- second pass: build FuncInfo list ----
    funcs = []
    # Cache demangled lookups
    for start, end, mname, demangled, dbg_id in defs:
        # EVERY definition feeds the demangle cache, including the ones skipped
        # below: the call-edge pass looks a CALLER up by its mangled name and a
        # miss raises KeyError inside the worker thread, losing that whole TU's
        # functions and edges ("worker error: _ZNK8tinyxml2...").
        demangled_cache[mname] = demangled
        if _excluded_symbol(mname, demangled):
            continue
        line = line_map.get(dbg_id, 0) if dbg_id else 0
        qname = FuncInfo(mname, demangled, "", 0, 0).qual_name
        cls = ""
        if "::" in qname:
            parts = qname.split("::")
            if len(parts) >= 2:
                cls = parts[-2] if len(parts) >= 3 else ""
        # The file the DEFINITION lives in, which is NOT always the TU we compiled
        # (see _extract_subprog_files); fall back to the TU when the metadata has
        # no DIFile.
        def_file = subprog_files.get(dbg_id) or src_file
        # A template instantiated from a VENDORED header (general/hps, libzstd,
        # blake3, libtiled...) is emitted into our TU but belongs to code we must
        # not touch. The TU list already skips vendor .cpp; now that the real
        # definition file is known, skip vendor headers too - else the reviewer
        # spends its budget on hps/map_serializer.h and reports findings nobody
        # may act on.
        if is_vendor(def_file):
            continue
        fi = FuncInfo(mname, demangled, def_file, line, 0, class_name=cls)
        if '::$_' in demangled and '::operator()' in demangled:
            fi.kind = "LambdaExpr"
        funcs.append(fi)

    # Build a list of definition start positions for bisect lookup
    def_starts = [d[0] for d in defs]
    # For each definition entry: also the end-of-line position is `end`
    # We'll use (pos, mname) pairs; bisect the sorted start positions

    # ---- extract call edges via binary search ----
    calls = []
    for cmatch in call_sites:
        callee = cmatch.group(1)
        if not callee:
            continue
        if callee.startswith(("llvm.", "__", "printf", "memset", "memcpy",
                               "memmove", "strlen", "strcpy", "malloc",
                               "free", "realloc", "calloc", "assert",
                               "fprintf", "sprintf", "snprintf")):
            continue
        callee_dm = demangled_cache[callee]
        if not callee_dm:
            continue
        if _excluded_symbol(callee, callee_dm):
            continue

        # Find enclosing definition: largest start_pos <= cmatch.start()
        idx = bisect.bisect_right(def_starts, cmatch.start()) - 1
        if idx < 0:
            continue
        caller_name = defs[idx][2]
        if _excluded_symbol(caller_name, demangled_cache[caller_name]):
            continue
        if caller_name == callee:
            continue

        # .get(): a miss must degrade to the mangled name (an edge that matches
        # nothing), never raise inside the worker and drop the whole TU.
        qcaller = FuncInfo(caller_name,
                           demangled_cache.get(caller_name, caller_name),
                           "", 0, 0).qual_name
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
def function_body(fi):
    return source_body(fi.file, fi.line, lambda_body=fi.kind == "LambdaExpr")


def source_body(file_path, start_line, lambda_body=False):
    """Extract a function body from source starting at `start_line`.

    Brace-matches from the first real '{' to the '}' that brings nesting depth back
    to zero. A single character scanner tracks // line comments, /* */ block
    comments (which span lines), and "..."/'...' literals, so a '{' or '}' that
    lives INSIDE a comment or a literal never moves the depth. The old line-prefix
    "skip whole-comment-lines" rule miscounted a brace in a TRAILING comment, a
    multi-line block-comment body, a string ("[a-z]{2,4}$") or a char literal
    ('{'/'}') — cutting the body short at a fake brace, so the reviewer saw a
    TRUNCATED function. Returns (body_text, end_line) or ('', start_line) on
    failure. Signature/default-argument and constructor initializer braces are
    skipped before matching the actual body."""
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
    raw_end = None
    parens = brackets = prefix_braces = 0
    initializer = False
    has_parameters = False
    previous = ''
    conditionals = []
    lineno = start_line - 1
    while lineno < len(lines):
        line = lines[lineno]
        if opened and not in_block and in_str is None and raw_end is None:
            directive = re.match(r'^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b', line)
            if directive:
                name = directive.group(1)
                if name in ('if', 'ifdef', 'ifndef'):
                    conditionals.append(depth)
                elif conditionals:
                    if name == 'endif':
                        conditionals.pop()
                    else:
                        depth = conditionals[-1]
                lineno += 1
                continue
        n = len(line)
        col = 0
        while col < n:
            ch = line[col]
            nxt = line[col + 1] if col + 1 < n else ''
            if raw_end is not None:
                end = line.find(raw_end, col)
                if end < 0:
                    break
                col = end + len(raw_end)
                raw_end = None
                previous = '"'
                continue
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
                if ch == 'R' and nxt == '"':
                    raw = re.match(r'R"([^\s()\\]{0,16})\(', line[col:])
                    if raw:
                        raw_end = ')' + raw.group(1) + '"'
                        col += raw.end()
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
                    if not opened and (prefix_braces or parens or brackets or
                                       (initializer and (previous.isalnum() or
                                                         previous in '_>'))):
                        prefix_braces += 1
                        col += 1
                        continue
                    depth += 1
                    opened = True
                elif ch == '}' and prefix_braces:
                    prefix_braces -= 1
                elif ch == '}' and opened:           # a '}' before the body opens is a
                    depth -= 1                        # stray close (misattributed start
                    if depth == 0:                   # line / #ifdef scope) — ignore it
                        body = "".join(lines[start_line - 1:lineno + 1])
                        return (body, lineno + 1)
                elif not opened and not prefix_braces:
                    if ch == '(':
                        parens += 1
                    elif ch == ')':
                        parens = max(0, parens - 1)
                        if not parens:
                            has_parameters = True
                    elif ch == '[':
                        if lambda_body:
                            # Debug locations for inline lambdas start on a call
                            # line (sort(..., [](...){...})), outside its capture.
                            parens = 0
                            lambda_body = False
                        brackets += 1
                    elif ch == ']':
                        brackets = max(0, brackets - 1)
                    elif (ch == ':' and has_parameters and not parens
                          and previous != ':' and nxt != ':'):
                        initializer = True
                    elif ch == ';' and not parens and not brackets:
                        return ("", start_line)  # declaration, not the next function
                if not ch.isspace():
                    previous = ch
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
        self.errors = []

    def _collect_sources(self):
        seen = set()
        out = []
        cdb_files = set(_load_cdb().keys()) if _BUILD_CDB_ONLY else None
        for d in SCOPE_DIRS:
            if os.path.isfile(d):
                fp = os.path.realpath(d)
                if (fp not in seen and not is_vendor(fp) and not is_generated(fp)
                        and fp.endswith((".cpp", ".cc", ".cxx", ".c"))
                        and (cdb_files is None or fp in cdb_files)):
                    seen.add(fp)
                    out.append(fp)
                continue
            if not os.path.isdir(d):
                continue
            for root, dirs, names in os.walk(os.path.realpath(d)):
                if is_vendor(root):
                    continue
                # Prune Qt-generated / build trees in place so os.walk never
                # descends into *_autogen, CMakeFiles, etc.
                dirs[:] = [x for x in dirs
                           if not (x.endswith("_autogen") or x == "CMakeFiles")]
                for n in sorted(names):
                    if n.endswith((".cpp", ".cc", ".cxx", ".c")) and not is_generated(n):
                        fp = os.path.realpath(os.path.join(root, n))
                        if fp not in seen:
                            seen.add(fp)
                            if cdb_files is None or fp in cdb_files:
                                out.append(fp)
        return sorted(out)

    def build(self, max_workers=None):
        # None -> every core: each TU is an independent clang -emit-llvm, so the
        # index build scales with the machine (a 32-core box was doing 8 at a time).
        if not max_workers:
            max_workers = os.cpu_count() or 8
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
            errs = self.errors
            parsed = []

            def process(src):
                ir, err = _ir_cached(src)
                if not ir:
                    if err:
                        with lock:
                            errs.append((src, err))
                    return
                fi_lst, call_lst = parse_function_defs(ir, src)
                with lock:
                    parsed.append((src, fi_lst, call_lst))
                    done[0] += 1
                    if done[0] % 50 == 0:
                        sys.stderr.write("  %d/%d TUs\n"
                                         % (done[0], total))

            with ThreadPoolExecutor(max_workers=max_workers) as pool:
                futures = {pool.submit(process, src): src for src in sources}
                for f in as_completed(futures):
                    try:
                        f.result()
                    except Exception as exc:
                        errs.append((futures[f], "worker error: %s" % exc))
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

            self._merge_definitions(parsed)
            self._built = True
            sys.stderr.write("[codetree] index done: %d functions, "
                             "%d forward edges, %d failures\n"
                             % (len(self.by_name),
                                sum(len(v)
                                    for v in self._forward_callees.values()),
                                 len(errs)))

    def _merge_definitions(self, parsed):
        """Keep same-signature implementations from different server binaries.

        Calls prefer definitions emitted in their own TU. An external call with
        several possible implementations retains all candidates, never an
        arbitrary first-completed worker's implementation.
        """
        variants = {}
        for _src, funcs, _calls in sorted(parsed, key=lambda item: item[0]):
            for fi in funcs:
                if os.path.realpath(fi.file).startswith(REPO_ROOT + os.sep):
                    variants.setdefault(fi.qual_name, {}).setdefault((fi.file, fi.line), fi)
        keys = {}
        for name, locations in sorted(variants.items()):
            # Prefer a real location over missing debug info for the same file.
            for loc, fi in sorted(locations.items()):
                if fi.line == 0 and any(f == fi.file and line > 0 for f, line in locations):
                    continue
                key = name
                if len(locations) > 1:
                    key += " [definition %s:%d]" % (os.path.relpath(fi.file, REPO_ROOT), fi.line)
                keys[(name, loc)] = key
                self.by_name[key] = fi
                self.def_loc[key] = loc
        for _src, funcs, calls in sorted(parsed, key=lambda item: item[0]):
            local = {}
            for fi in funcs:
                key = keys.get((fi.qual_name, (fi.file, fi.line)))
                if key:
                    local.setdefault(fi.qual_name, set()).add(key)
            for caller, callee, line in calls:
                targets = local.get(callee) or {
                    keys[(callee, loc)] for loc in variants.get(callee, {})
                    if (callee, loc) in keys} or {callee}
                for origin in local.get(caller, ()):
                    for target in sorted(targets):
                        self._forward_callees.setdefault(origin, {}).setdefault(target, set()).add(line)
                        self._reverse_callers.setdefault(target, {}).setdefault(origin, set()).add(line)
        for graph in (self._forward_callees, self._reverse_callers):
            for edges in graph.values():
                for target, lines in edges.items():
                    edges[target] = sorted(lines)
        for key, fi in self.by_name.items():
            fi.demangled = key

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
            if d <= 0 or qn in seen:
                return
            seen.add(qn)
            for callee_q, call_lines in \
                    sorted(idx._forward_callees.get(qn, {}).items()):
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
            if d <= 0 or qn in seen:
                return
            seen.add(qn)
            for caller_q, call_lines in \
                    sorted(idx._reverse_callers.get(qn, {}).items()):
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
        body, endline = function_body(fi)

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
