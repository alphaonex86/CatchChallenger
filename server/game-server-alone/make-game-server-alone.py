import threading
import subprocess
import os
import sys
import multiprocessing
import logging

iterator = -1
logging.basicConfig(level=logging.DEBUG, format='(%(threadName)-9s) %(message)s',)
has_success = True

compile_commands = []
object_files = []
compile_units = []  # list of (source_path, is_cpp)

cxx_flags = "-std=c++0x -fstack-protector-all -std=c++0x -g -fno-rtti -O2 -std=gnu++11 -D_REENTRANT -fPIC -Walloca -Wcast-qual -Wconversion -Wformat=2 -Wformat-security -Wnull-dereference -Wstack-protector -Wvla -Warray-bounds -Warray-bounds-pointer-arithmetic -Wassign-enum -Wbad-function-cast -Wconditional-uninitialized -Wconversion -Wfloat-equal -Wformat-type-confusion -Widiomatic-parentheses -Wimplicit-fallthrough -Wloop-analysis -Wpointer-arith -Wshift-sign-overflow -Wshorten-64-to-32 -Wswitch-enum -Wtautological-constant-in-range-compare -Wunreachable-code-aggressive -Wthread-safety -Wthread-safety-beta -Wcomma -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fPIE -fstack-clash-protection -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DEXTERNALLIBZSTD -D__linux__ -DTILED_ZLIB -DCATCHCHALLENGER_CACHE_HPS -DCATCHCHALLENGER_DB_POSTGRESQL -DCATCHCHALLENGER_DB_PREPAREDSTATEMENT -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DCATCHCHALLENGER_DYNAMIC_MAP_LIST=1 -DXXH_INLINE_ALL -DCATCHCHALLENGER_CLASS_ONLYGAMESERVER -DCATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_DEBUG -I../game-server-alone -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-gcc"
cc_flags = "-fstack-protector-all -g -fno-rtti -O2 -D_REENTRANT -fPIC -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fPIE -fstack-clash-protection -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DEXTERNALLIBZSTD -D__linux__ -DTILED_ZLIB -DCATCHCHALLENGER_CACHE_HPS -DCATCHCHALLENGER_DB_POSTGRESQL -DCATCHCHALLENGER_DB_PREPAREDSTATEMENT -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DCATCHCHALLENGER_DYNAMIC_MAP_LIST=1 -DXXH_INLINE_ALL -DCATCHCHALLENGER_CLASS_ONLYGAMESERVER -DCATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_DEBUG -I../game-server-alone -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-gcc"

# Compiler selection via environment (COMPILER=gcc or COMPILER=clang)
COMPILER = os.environ.get('COMPILER', 'gcc').strip().lower()
if COMPILER == 'clang':
    CC = 'clang'
    CXX = 'clang++'
    cxx_flags = cxx_flags.replace('linux-gcc', 'linux-clang')
    cc_flags = cc_flags.replace('linux-gcc', 'linux-clang')
    # -fsanitize=safe-stack is clang-only; restore it when targeting clang.
    cxx_flags += ' -fsanitize=safe-stack'
    cc_flags += ' -fsanitize=safe-stack'
    SAFESTACK_LINK = ' -fsanitize=safe-stack'
else:
    CC = 'gcc'
    CXX = 'g++'
    cxx_flags = cxx_flags.replace('linux-clang', 'linux-gcc')
    cc_flags = cc_flags.replace('linux-clang', 'linux-gcc')
    # Strip clang-only warning flags so g++ doesn't error out on them.
    # Sorted longest-first so stripping `-Wthread-safety` doesn't leave a
    # `-beta` fragment after `-Wthread-safety-beta`.
    _CLANG_ONLY_W = sorted([
        '-Warray-bounds-pointer-arithmetic',
        '-Wassign-enum',
        '-Wconditional-uninitialized',
        '-Wformat-type-confusion',
        '-Widiomatic-parentheses',
        '-Wloop-analysis',
        '-Wshift-sign-overflow',
        '-Wshorten-64-to-32',
        '-Wtautological-constant-in-range-compare',
        '-Wunreachable-code-aggressive',
        '-Wthread-safety',
        '-Wthread-safety-beta',
        '-Wcomma',
    ], key=len, reverse=True)
    for _w in _CLANG_ONLY_W:
        cxx_flags = cxx_flags.replace(' ' + _w + ' ', ' ')
        cc_flags = cc_flags.replace(' ' + _w + ' ', ' ')
    SAFESTACK_LINK = ''

# Linker selection: USE_MOLD=1 to add -fuse-ld=mold to the final link.
_use_mold_env = os.environ.get('USE_MOLD', '').strip().lower()
USE_MOLD = _use_mold_env in ('1', 'true', 'yes', 'on')
MOLD_FLAGS = ' -fuse-ld=mold' if USE_MOLD else ''

# Extra macro defines from environment, space-separated (e.g. EXTRA_DEFINES="CATCHCHALLENGER_NOXML CATCHCHALLENGER_HARDENED")
extra_def_str = ''
extra_defines_env = os.environ.get('EXTRA_DEFINES', '').strip()
if extra_defines_env:
    for _d in extra_defines_env.split():
        extra_def_str += ' -D' + _d
cxx_flags += extra_def_str
cc_flags += extra_def_str

# Output directory for .o files and final binary; default = this script's dir.
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
os.chdir(SCRIPT_DIR)
_build_dir_env = os.environ.get('BUILD_DIR', '').strip()
if _build_dir_env:
    BUILD_DIR = os.path.abspath(_build_dir_env)
    os.makedirs(BUILD_DIR, exist_ok=True)
else:
    BUILD_DIR = SCRIPT_DIR

# files that exist on disk but are not compiled for this target
exclude_list = [
    # general/base/ - Qt socket wrapper, not used by epoll server
    "ConnectedSocket.cpp",
    # general/libxxhash/ - x86 dispatcher not needed
    "xxh_x86dispatch.c",
    # server/base/ClientEvents/ - not used
    "ClientEvents/LocalClientHandlerBattle.cpp",
    # server/epoll/ - client-to-server connectors, not used by this target
    "EpollClientToServer.cpp",
    "EpollSslClientToServer.cpp",
    # server/epoll/ - stdin handler, not used
    "EpollStdin.cpp",
    # server/epoll/ - unix socket support, not used
    "EpollUnixSocketClient.cpp",
    "EpollUnixSocketClientFinal.cpp",
    "EpollUnixSocketServer.cpp",
    # server/epoll/db/ - generic DB base and MySQL driver
    "db/EpollDatabase.cpp",
    "db/EpollMySQL.cpp",
    # server/epoll/filedb-converter/ - separate tool
    "filedb-converter/FiledbConverter.cpp",
    "filedb-converter/main.cpp",
    # server/epoll/timer/ - not used
    "timer/TimerDisplayEventBySeconds.cpp",
    # server/game-server-alone/ - not used
    "TimerReconnectOnTheMaster.cpp",
]

# Active EXTRA_DEFINES set for conditional source exclusion (mirrors .pro/.pri)
_active_defines = set(extra_defines_env.split()) if extra_defines_env else set()
if 'CATCHCHALLENGER_NOXML' in _active_defines:
    cxx_flags = cxx_flags.replace(' -DCATCHCHALLENGER_XLMPARSER_TINYXML2', '')
    cc_flags = cc_flags.replace(' -DCATCHCHALLENGER_XLMPARSER_TINYXML2', '')
    exclude_list += [
        "Map_loader.cpp",
        "Map_loaderMain.cpp",
        "DatapackGeneralLoader/DatapackGeneralLoader.cpp",
        "DatapackGeneralLoader/DatapackGeneralLoaderCrafting.cpp",
        "DatapackGeneralLoader/DatapackGeneralLoaderItem.cpp",
        "DatapackGeneralLoader/DatapackGeneralLoaderMap.cpp",
        "DatapackGeneralLoader/DatapackGeneralLoaderMonsterDrop.cpp",
        "DatapackGeneralLoader/DatapackGeneralLoaderPlant.cpp",
        "DatapackGeneralLoader/DatapackGeneralLoaderQuest.cpp",
        "DatapackGeneralLoader/DatapackGeneralLoaderReputation.cpp",
        "FightLoaderBuff.cpp",
        "FightLoaderMonster.cpp",
        "FightLoaderSkill.cpp",
        "TinyXMLSettings.cpp",
        "tinyxml2.cpp",
        "tinyxml2b.cpp",
        "tinyxml2c.cpp",
    ]
if 'CATCHCHALLENGER_DB_FILE' in _active_defines:
    exclude_list += [
        "SqlFunction.cpp",
        "PreparedDBQueryLogin.cpp",
        "PreparedDBQueryCommon.cpp",
        "PreparedDBQueryServer.cpp",
        "DatabaseBase.cpp",
        "DatabaseFunction.cpp",
        "PreparedStatementUnit.cpp",
    ]

def compileFolder(folder, isRecursive):
    if isRecursive:
        for root, dirs, files in os.walk(folder):
            dirs.sort()
            for f in sorted(files):
                if not (f.endswith('.cpp') or f.endswith('.c')):
                    continue
                filepath = os.path.join(root, f)
                relpath = os.path.relpath(filepath, folder)
                if relpath in exclude_list:
                    continue
                compile_units.append((filepath, f.endswith('.cpp')))
    else:
        for f in sorted(os.listdir(folder)):
            if not (f.endswith('.cpp') or f.endswith('.c')):
                continue
            filepath = os.path.join(folder, f)
            if not os.path.isfile(filepath):
                continue
            if f in exclude_list:
                continue
            compile_units.append((filepath, f.endswith('.cpp')))

compileFolder("../../general/base", True)
compileFolder("../../general/fight", False)
compileFolder("../../general/blake3", False)
compileFolder("../../general/libxxhash", False)
compileFolder("../../general/tinyXML2", False)
compileFolder("../base", True)
compileFolder("../crafting", False)
compileFolder("../fight", False)
compileFolder("../epoll", True)
compileFolder(".", True)

# Drop sources that are #include'd by another listed source (qmake parity).
import re as _re_filter
_listed = set()
_ui = 0
while _ui < len(compile_units):
    _listed.add(os.path.normpath(os.path.abspath(compile_units[_ui][0])))
    _ui += 1
_covered = set()
_ui = 0
while _ui < len(compile_units):
    _src = compile_units[_ui][0]
    try:
        with open(_src, 'r') as _f:
            _text = _f.read()
        _srcdir = os.path.dirname(os.path.abspath(_src))
        for _m in _re_filter.finditer(r'^\s*#\s*include\s*"([^"]+\.(?:cpp|c))"', _text, _re_filter.M):
            _ip = os.path.normpath(os.path.join(_srcdir, _m.group(1)))
            if _ip in _listed:
                _covered.add(_ip)
    except IOError:
        pass
    _ui += 1
_filtered = []
_ui = 0
while _ui < len(compile_units):
    if os.path.normpath(os.path.abspath(compile_units[_ui][0])) not in _covered:
        _filtered.append(compile_units[_ui])
    _ui += 1
compile_units = _filtered

# Build per-file compile commands and object_files list with unique basenames.
_used_bases = set()
_ui = 0
while _ui < len(compile_units):
    _src, _is_cpp = compile_units[_ui]
    _base = os.path.splitext(os.path.basename(_src))[0]
    _candidate = _base
    _i = 1
    while _candidate in _used_bases:
        _candidate = _base + '_' + str(_i)
        _i += 1
    _used_bases.add(_candidate)
    _ofile = os.path.join(BUILD_DIR, _candidate + '.o')
    if _is_cpp:
        compile_commands.append(CXX + " -c -pipe " + cxx_flags + " -o " + _ofile + " " + _src)
    else:
        compile_commands.append(CC + " -c -pipe " + cc_flags + " -o " + _ofile + " " + _src)
    object_files.append(_ofile)
    _ui += 1

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
MAX_CONCURRENCE = multiprocessing.cpu_count()+1

for i in range(MAX_CONCURRENCE):
    t = Requester()
    t.start()

main_thread = threading.currentThread()
for t in threading.enumerate():
    if t is not main_thread:
        t.join()

if not has_success:
	sys.exit(123)

os.system(CXX + MOLD_FLAGS + SAFESTACK_LINK + " -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,-z,separate-code -o " + os.path.join(BUILD_DIR, "catchchallenger-game-server-alone") + " " + " ".join(object_files) + " -lzstd -lz -lpq")
print('Finished')
if not has_success:
	sys.exit(123)
else:
	sys.exit(0)
