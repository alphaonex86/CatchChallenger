"""
make.py — dynamic compile/link for catchchallenger-gateway.

Sources are auto-detected from gateway.pro (every SOURCES += entry, including
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
PRO_FILE = os.path.join(SCRIPT_DIR, 'gateway.pro')
TARGET_BIN = 'catchchallenger-gateway'
LINK_LIBS = ['-lcurl', '-lxxhash', '-lzstd']

DEFINES = [
    'EPOLLCATCHCHALLENGERSERVER', 'QT_NO_EMIT',
    'CATCHCHALLENGER_CLASS_GATEWAY',
    'CATCHCHALLENGER_DB_BLACKHOLE',
    'DEBUG_PROTOCOLPARSING_RAW_NETWORK',
    'PROTOCOLPARSINGINPUTOUTPUTDEBUG',
    'BLAKE3_NO_SSE2', 'BLAKE3_NO_SSE41', 'BLAKE3_NO_AVX2', 'BLAKE3_NO_AVX512',
    'QT_NO_DEBUG',
]
CXX_STD = '-std=gnu++20'

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
