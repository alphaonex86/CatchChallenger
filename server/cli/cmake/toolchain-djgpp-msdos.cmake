# toolchain-djgpp-msdos.cmake — cross-compile catchchallenger-server-cli
# from Linux to MS-DOS 6 / FreeDOS with the DJGPP GCC tools + Watt-32 for TCP.
#
# Usage:
#   cmake -S server/cli -B build/msdos \
#     -DCMAKE_TOOLCHAIN_FILE=server/cli/cmake/toolchain-djgpp-msdos.cmake
#
# Env (operator-specific paths kept OUT of the repo):
#   DJGPP_ROOT    DJGPP toolchain root (has bin/i586-pc-msdosdjgpp-g++).
#   WATT_ROOT     Watt-32 root (has inc/ + lib/libwatt.a).
# Both default to the standard CatchChallenger MS-DOS prefix when unset.
# (NB: DJGPP_ROOT is the install root, distinct from Watt-32's DJGPP_PREFIX
#  env var which must be the i586-pc-msdosdjgpp triple — don't conflate them.)

# CMake has no "MSDOS" platform; Generic is the bare cross target. The DJGPP
# linker still emits a real MZ/COFF .exe — we just name it so.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR i586)
set(CMAKE_EXECUTABLE_SUFFIX ".exe")

if(DEFINED ENV{DJGPP_ROOT})
    set(_dj "$ENV{DJGPP_ROOT}")
else()
    set(_dj "/mnt/data/perso/progs/catchchallenger-msdos/djgpp")
endif()
if(DEFINED ENV{WATT_ROOT})
    set(_watt "$ENV{WATT_ROOT}")
else()
    set(_watt "/mnt/data/perso/progs/catchchallenger-msdos/watt32")
endif()
set(_triple i586-pc-msdosdjgpp)

set(CMAKE_C_COMPILER   "${_dj}/bin/${_triple}-gcc")
set(CMAKE_CXX_COMPILER "${_dj}/bin/${_triple}-g++")
set(CMAKE_AR           "${_dj}/bin/${_triple}-ar"     CACHE FILEPATH "")
set(CMAKE_RANLIB       "${_dj}/bin/${_triple}-ranlib" CACHE FILEPATH "")

# DJGPP gates EVERY POSIX/DOS extension (DIR, sockets, …) behind
# !__STRICT_ANSI__, so the build MUST use the GNU dialect (-std=gnu++NN),
# never plain -std=c++NN. Keep extensions on and pin C++17 (GCC 12.2's fully
# supported level for this code); CCCommon won't override an already-set std.
set(CMAKE_C_EXTENSIONS   ON)
set(CMAKE_CXX_EXTENSIONS ON)
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()

# The compiler check links a tiny program; a static-lib check avoids dragging
# in the go32/CWSDPMI runtime at configure time.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Watt-32 provides the BSD socket API + select() the SELECT backend needs.
include_directories(SYSTEM "${_watt}/inc")
link_directories("${_watt}/lib")

# Flag the project's CMakeLists keys off to take the MS-DOS path (force the
# select(2) backend, drop pthreads, link Watt-32). The compiler itself
# predefines __DJGPP__, so the C++ #ifdef shims need no extra -D.
set(CC_TARGET_MSDOS ON)

# DOS is single-threaded: build vendored zstd without its pthread-based
# multithread path (threading.h would pull <pthread.h>, absent in DJGPP).
set(ZSTD_MULTITHREAD_SUPPORT OFF CACHE BOOL "" FORCE)

set(CMAKE_FIND_ROOT_PATH "${_dj}/${_triple}" "${_watt}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
