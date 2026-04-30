"""
make.py — dynamic compile/link for catchchallenger-server-master.

Sources are auto-detected from master.pro (every SOURCES += entry, including
$$PWD-prefixed paths and continuation lines).  Each .cpp/.c is compiled in its
own subprocess inside a thread pool sized to multiprocessing.cpu_count()+1, then
the final binary is linked from the gathered object files.

Environment variables:
  EXTRA_DEFINES   space-separated extra -D macros (e.g. CATCHCHALLENGER_NOXML)
  COMPILER        gcc (default) or clang
  USE_MOLD        1 to add -fuse-ld=mold to the final link
  BUILD_DIR       output directory for .o files and final binary
"""

import threading
import subprocess
import os
import sys
import multiprocessing
import logging
import re

iterator = -1
logging.basicConfig(level=logging.DEBUG, format='(%(threadName)-9s) %(message)s',)
has_success = True

# ── env-driven options ──────────────────────────────────────────────────
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
os.chdir(SCRIPT_DIR)

COMPILER = os.environ.get('COMPILER', 'gcc').strip().lower()
if COMPILER == 'clang':
    CC = 'clang'
    CXX = 'clang++'
    QT_MKSPEC = 'linux-clang'
else:
    CC = 'gcc'
    CXX = 'g++'
    QT_MKSPEC = 'linux-gcc'

_use_mold_env = os.environ.get('USE_MOLD', '').strip().lower()
USE_MOLD = _use_mold_env in ('1', 'true', 'yes', 'on')
MOLD_FLAGS = ' -fuse-ld=mold' if USE_MOLD else ''

extra_def_str = ''
_extra_env = os.environ.get('EXTRA_DEFINES', '').strip()
if _extra_env:
    for _d in _extra_env.split():
        extra_def_str += ' -D' + _d

_build_dir_env = os.environ.get('BUILD_DIR', '').strip()
if _build_dir_env:
    BUILD_DIR = os.path.abspath(_build_dir_env)
    os.makedirs(BUILD_DIR, exist_ok=True)
else:
    BUILD_DIR = SCRIPT_DIR

# ── target-specific configuration ───────────────────────────────────────
PRO_FILE = os.path.join(SCRIPT_DIR, 'master.pro')
TARGET_BIN = 'catchchallenger-server-master'
LINK_LIBS = ['-lpq', '-lxxhash']

DEFINES = [
    'EPOLLCATCHCHALLENGERSERVER', 'QT_NO_EMIT',
    'EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION',
    'EPOLLCATCHCHALLENGERSERVERNOGAMESERVER',
    'CATCHCHALLENGER_CLASS_MASTER',
    'CATCHCHALLENGER_DB_POSTGRESQL',
    'CATCHCHALLENGER_DB_PREPAREDSTATEMENT',
    'QT_NO_DEBUG',
]
CXX_STD = '-std=gnu++17'

CXX_BASE = '-pipe -fstack-protector-all -g -fno-rtti -O2 ' + CXX_STD + ' -D_REENTRANT -fPIC'
CC_BASE = '-pipe -fstack-protector-all -g -fno-rtti -O2 -D_REENTRANT -fPIC'
INCLUDES = (' -I' + os.path.basename(SCRIPT_DIR)
            + ' -I.'
            + ' -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/' + QT_MKSPEC)

_def_flags = ' '.join('-D' + _d for _d in DEFINES)
cxx_flags = CXX_BASE + ' ' + _def_flags + INCLUDES + extra_def_str
cc_flags = CC_BASE + ' ' + _def_flags + INCLUDES + extra_def_str

# ── parse SOURCES += from .pro file ─────────────────────────────────────
def parse_pro_sources(pro_path):
    pro_dir = os.path.dirname(os.path.abspath(pro_path))
    tokens = []
    with open(pro_path, 'r') as _f:
        text = _f.read()
    in_block = False
    for raw in text.splitlines():
        line = raw.split('#', 1)[0].rstrip()
        if not in_block:
            m = re.match(r'^\s*SOURCES\s*\+?=\s*(.*)$', line)
            if m:
                tail = m.group(1)
                cont = tail.endswith('\\')
                payload = tail.rstrip('\\').strip()
                for tok in payload.split():
                    tokens.append(tok)
                in_block = cont
        else:
            cont = line.endswith('\\')
            payload = line.rstrip('\\').strip()
            for tok in payload.split():
                tokens.append(tok)
            if not cont:
                in_block = False
    out = []
    for s in tokens:
        s = s.replace('$$PWD/', pro_dir + '/').replace('$$PWD', pro_dir)
        if not os.path.isabs(s):
            s = os.path.normpath(os.path.join(pro_dir, s))
        if s.endswith('.cpp') or s.endswith('.c'):
            out.append(s)
    return out

def filter_included_sources(srcs):
    """Drop sources that are #include'd by another listed source (qmake parity)."""
    norm = []
    _i = 0
    while _i < len(srcs):
        norm.append(os.path.normpath(srcs[_i]))
        _i += 1
    listed = set(norm)
    covered = set()
    _i = 0
    while _i < len(srcs):
        s = srcs[_i]
        try:
            with open(s, 'r') as _f:
                text = _f.read()
        except IOError:
            _i += 1
            continue
        srcdir = os.path.dirname(os.path.abspath(s))
        for m in re.finditer(r'^\s*#\s*include\s*"([^"]+\.(?:cpp|c))"', text, re.M):
            inc_path = os.path.normpath(os.path.join(srcdir, m.group(1)))
            if inc_path in listed:
                covered.add(inc_path)
        _i += 1
    out = []
    _i = 0
    while _i < len(srcs):
        if os.path.normpath(srcs[_i]) not in covered:
            out.append(srcs[_i])
        _i += 1
    return out

sources = parse_pro_sources(PRO_FILE)
sources = filter_included_sources(sources)
if not sources:
    logging.error('no SOURCES detected in ' + PRO_FILE)
    sys.exit(124)

# ── build per-file compile commands & object_files list ─────────────────
compile_commands = []
object_files = []
_used_bases = set()
_si = 0
while _si < len(sources):
    src = sources[_si]
    base = os.path.splitext(os.path.basename(src))[0]
    candidate = base
    _i = 1
    while candidate in _used_bases:
        candidate = base + '_' + str(_i)
        _i += 1
    _used_bases.add(candidate)
    ofile = os.path.join(BUILD_DIR, candidate + '.o')
    if src.endswith('.cpp'):
        compile_commands.append(CXX + ' -c ' + cxx_flags + ' -o ' + ofile + ' ' + src)
    else:
        compile_commands.append(CC + ' -c ' + cc_flags + ' -o ' + ofile + ' ' + src)
    object_files.append(ofile)
    _si += 1

# ── resolvCtoO + EXTRA_DEFINES-aware incremental compile ────────────────
# Helpers are duplicated in every make*.py (per CLAUDE.md request) so each
# script stays self-contained.  Mirrors test/qmake_helpers.py so the same
# logic runs whether the build was driven by qmake or by make.py.
def resolvCtoO(sources, build_dir):
    """Return a {source_path: obj_path} dict for the given sources, using
    the same disambiguating-by-counter rule the compile_commands loop
    uses (so two sources sharing a basename each get a unique .o)."""
    out = {}
    used = set()
    si = 0
    while si < len(sources):
        src = sources[si]
        si += 1
        base = os.path.splitext(os.path.basename(src))[0]
        candidate = base
        i = 1
        while candidate in used:
            candidate = base + '_' + str(i)
            i += 1
        used.add(candidate)
        out[src] = os.path.join(build_dir, candidate + '.o')
    return out


def find_sources_referencing(macros, source_paths):
    """Return the subset of *source_paths* that mention any macro in
    *macros* via #ifdef / #ifndef / #if defined() / #elif lines."""
    if not macros:
        return set()
    macro_alt = '|'.join(re.escape(m) for m in macros)
    rx = re.compile(
        r'^\s*#\s*(?:if|ifdef|ifndef|elif)\b.*?\b(?:' + macro_alt + r')\b',
        re.MULTILINE,
    )
    found = set()
    si = 0
    while si < len(source_paths):
        s = source_paths[si]
        si += 1
        try:
            with open(s, 'r', errors='replace') as _f:
                text = _f.read()
        except IOError:
            continue
        if rx.search(text):
            found.add(s)
    return found


# Incremental rebuild gate: macro-level invalidation only runs when the
# compiler (gcc/clang) AND the C++ standard (-std=...) are both unchanged
# from the previous run.  If either changed, the existing .o files were
# built for a different ABI and are not reusable — wipe them all.
src_to_obj = resolvCtoO(sources, BUILD_DIR)
_state_file = os.path.join(BUILD_DIR, '.cc_make_state.txt')
_last_compiler = ''
_last_std = ''
_last_defines = set()
if os.path.isfile(_state_file):
    try:
        with open(_state_file, 'r') as _f:
            for _line in _f:
                _line = _line.strip()
                if not _line or '=' not in _line:
                    continue
                _k, _v = _line.split('=', 1)
                if _k == 'compiler':
                    _last_compiler = _v
                elif _k == 'std':
                    _last_std = _v
                elif _k == 'defines' and _v:
                    for _d in _v.split():
                        _last_defines.add(_d)
    except IOError:
        pass

_current_defines = set()
_cur_extra = os.environ.get('EXTRA_DEFINES', '').strip()
if _cur_extra:
    for _d in _cur_extra.split():
        _current_defines.add(_d)
_std_matches = re.findall(r'(?:^|\s)(-std=\S+)', cxx_flags)
_current_std = _std_matches[-1] if _std_matches else ''
_current_compiler = COMPILER

_abi_changed = ((_last_compiler != '' and _last_compiler != _current_compiler) or
                (_last_std != '' and _last_std != _current_std))
if _abi_changed:
    logging.info('compiler/std changed (' + _last_compiler + ' / '
                 + _last_std + ' -> ' + _current_compiler + ' / '
                 + _current_std + '); wiping all .o for a full rebuild')
    _oi = 0
    while _oi < len(object_files):
        if os.path.isfile(object_files[_oi]):
            try:
                os.unlink(object_files[_oi])
            except OSError:
                pass
        _oi += 1
else:
    _changed_defs = _last_defines.symmetric_difference(_current_defines)
    if _changed_defs:
        _affected = find_sources_referencing(_changed_defs, list(src_to_obj.keys()))
        if _affected:
            logging.info('EXTRA_DEFINES changed (' + ' '.join(sorted(_changed_defs))
                         + '); invalidating ' + str(len(_affected)) + ' .o file(s)')
            _aff_list = list(_affected)
            _ai = 0
            while _ai < len(_aff_list):
                _o = src_to_obj.get(_aff_list[_ai])
                _ai += 1
                if _o and os.path.isfile(_o):
                    try:
                        os.unlink(_o)
                    except OSError:
                        pass

# Skip any compile command whose .o is already up-to-date on disk.
_new_cc = []
_ci = 0
while _ci < len(compile_commands):
    if not os.path.isfile(object_files[_ci]):
        _new_cc.append(compile_commands[_ci])
    _ci += 1
_skipped = len(compile_commands) - len(_new_cc)
if _skipped:
    logging.info('incremental: skipping ' + str(_skipped) + ' .o already up-to-date '
                 '(rm them under ' + BUILD_DIR + ' to force rebuild)')
compile_commands = _new_cc


class Requester(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
        self.lock = threading.Lock()

    def run(self):
        global iterator, compile_commands, length, has_success
        while True:
            self.lock.acquire()
            iterator = iterator + 1
            if iterator < length:
                command = compile_commands[iterator]
            else:
                command = None
            self.lock.release()
            if command is None:
                break
            elif len(command) > 0:
                parts = command.strip().split(' ')
                try:
                    output = subprocess.run(parts, stdout=subprocess.PIPE, check=True)
                    if output.returncode != 0:
                        logging.error(output.stderr)
                except Exception as err:
                    logging.error('command: ' + command + ' exception: ' + str(err))
                    has_success = False

if os.environ.get('DUMP_BUILD_PLAN', '').strip().lower() in ('1', 'true', 'yes', 'on'):
    # Introspection mode: print every compile command + sources/defines summary,
    # then exit without compiling. Used by the qmake-vs-make comparator.
    _di = 0
    while _di < len(compile_commands):
        print('CMD: ' + compile_commands[_di])
        _di += 1
    sys.exit(0)

length = len(compile_commands)
MAX_CONCURRENCE = multiprocessing.cpu_count() + 1
for i in range(MAX_CONCURRENCE):
    t = Requester()
    t.start()

main_thread = threading.currentThread()
for t in threading.enumerate():
    if t is not main_thread:
        t.join()

# Save compiler+std+defines so the next run can decide between a full
# rebuild (compiler/std changed) and a minimal incremental rebuild
# (only EXTRA_DEFINES changed) — see resolvCtoO block above.
try:
    with open(_state_file, 'w') as _f:
        _f.write('compiler=' + _current_compiler + '\n')
        _f.write('std=' + _current_std + '\n')
        _f.write('defines=' + ' '.join(sorted(_current_defines)) + '\n')
except IOError:
    pass

if not has_success:
    sys.exit(123)

# ── final link ──────────────────────────────────────────────────────────
# -fsanitize=safe-stack is clang-only; skip it for gcc.
LINK_SAFESTACK = ' -fsanitize=safe-stack' if COMPILER == 'clang' else ''
LINK_EXTRA = LINK_SAFESTACK + ' -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,-z,separate-code'
link_cmd = (CXX + MOLD_FLAGS + LINK_EXTRA
            + ' -o ' + os.path.join(BUILD_DIR, TARGET_BIN)
            + ' ' + ' '.join(object_files)
            + ' ' + ' '.join(LINK_LIBS))
rc = os.system(link_cmd)
print('Finished')
if not has_success or rc != 0:
    sys.exit(123)
sys.exit(0)
