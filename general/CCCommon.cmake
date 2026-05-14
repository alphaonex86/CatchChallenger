# general/CCCommon.cmake — shared setup for every binary CMakeLists.txt
# in the CatchChallenger tree. There is no root CMakeLists.txt; each
# executable's CMakeLists.txt is its own self-contained project, and
# they all pull this file in for the common boilerplate (ccache, CXX
# standard, common compile flags, top-level options, ZLIB / zstd
# discovery). The CC_REPO_ROOT variable points at the top of the repo
# so that binary CMakeLists.txt files can `add_subdirectory` library
# subdirs (general/, server/base/, client/libcatchchallenger/, …) by
# absolute path.
#
# Usage from a binary CMakeLists.txt:
#   cmake_minimum_required(VERSION 3.20)
#   project(<name> LANGUAGES C CXX)
#   include(${CMAKE_CURRENT_LIST_DIR}/<rel>/general/CCCommon.cmake)
#   find_package(Qt6 …)
#   add_subdirectory(${CC_REPO_ROOT}/general …)
#   add_executable(<name> …)
#   target_link_libraries(<name> PRIVATE catchchallenger_general …)

# Re-include guard. include() doesn't have one built-in, and several
# of the targets defined below (catchchallenger_common_flags) cannot
# be created twice. When a binary CMakeLists.txt add_subdirectory's a
# library subdir that itself includes this file, the second include
# becomes a no-op.
if(CC_COMMON_LOADED)
    return()
endif()
set(CC_COMMON_LOADED TRUE)

# ── build-type defaulting (works for single + multi-config) ───────────────
# Qt Creator's Ninja-Multi-Config kit asks the project for a "Debug"
# configuration on first open; if CMAKE_CONFIGURATION_TYPES doesn't
# expose one the IDE bails with
#     "CMake project configuration failed.  No CMake configuration for
#      build type 'Debug' found."
# testing*.py builds with single-config Ninja and never sets
# CMAKE_CONFIGURATION_TYPES, so without these guards the same project
# builds fine from CLI but breaks the moment you open it in Qt Creator
# with the wrong kit.  Apply uniformly here so every binary's
# CMakeLists.txt inherits the behaviour.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING
        "Choose the type of build (Debug Release RelWithDebInfo MinSizeRel)" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel)
endif()
if(CMAKE_CONFIGURATION_TYPES AND NOT "Debug" IN_LIST CMAKE_CONFIGURATION_TYPES)
    list(APPEND CMAKE_CONFIGURATION_TYPES Debug)
    set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
        "Available multi-config build types" FORCE)
endif()

# Always emit compile_commands.json so Qt Creator / clangd / any other
# IDE works out of the box.  No-op for multi-config generators (cmake
# already produces it per-config) but harmless to set everywhere.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# CC_REPO_ROOT is the directory one level up from this file
# (general/CCCommon.cmake → general/ → repo root). Compute it once and
# expose it to the caller's scope so they can spell out absolute paths
# in add_subdirectory calls.
get_filename_component(CC_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

# Auto-pick ccache as the compiler launcher when available. Vendored
# third-party libs (general/blake3, general/libxxhash, general/libzstd,
# client/libqtcatchchallenger/lib{ogg,opus,opusfile,tiled}) almost never
# change, so their .o files come from cache nearly every test run.
# Project sources also benefit on incremental edits. CMAKE_<LANG>_COMPILER_LAUNCHER
# only takes effect on targets created AFTER it is set, so this block
# must run before any add_executable / add_library that the consumer
# calls.
# CCACHE_DIR is honoured if the caller exports it (test/all.sh does on
# this host); otherwise ccache defaults to ~/.ccache, which is the right
# behaviour on remote build nodes that don't share the local cache path.
# Auto-pick ccache when it's on PATH. Testing (testingcompilationwindows.py,
# all.sh) forces a specific ccache binary via -DCMAKE_<LANG>_COMPILER_LAUNCHER
# — respect that and never overwrite it here. On deploy VPSes ccache is
# optional: if it isn't installed, the build still works (just slower).
#
# Re-probe every configure: find_program caches its result, so a stale
# CCACHE_PROGRAM-NOTFOUND from a configure run before ccache was installed
# would otherwise stick forever even after the binary lands on PATH.
if(NOT CMAKE_C_COMPILER_LAUNCHER AND NOT CMAKE_CXX_COMPILER_LAUNCHER)
    unset(CCACHE_PROGRAM CACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        set(CMAKE_C_COMPILER_LAUNCHER   "${CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        message(STATUS "ccache: ${CCACHE_PROGRAM} (auto-detected)")
    endif()
else()
    message(STATUS "ccache: respecting caller's CMAKE_<LANG>_COMPILER_LAUNCHER override")
endif()

# AUTOMOC speed-ups. Same rationale as the qmake build.
set(CMAKE_AUTOGEN_PARALLEL AUTO)
set(CMAKE_AUTOMOC_RELAXED_MODE OFF)

# ── compile-time parallelism ───────────────────────────────────────────
# Per-file compilation in gcc/clang is inherently single-threaded — cc1plus
# is one process per .cpp. Wall-clock parallelism comes from the build
# system (ninja -jN, make -jN). The ONE compiler-side knob that actually
# multi-threads is LTO's worker pool: gcc's -flto=auto / clang's
# -flto-jobs=N spawn that many parallel LTO partitions during link.
#
# Only emit the flag when LTO is actually enabled (CMAKE_INTERPROCEDURAL_
# OPTIMIZATION) — otherwise -flto=N would *enable* LTO as a side-effect,
# which CLAUDE.md forbids ("optional accelerators must NEVER be required").
if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    if(NOT DEFINED _cc_link_jobs)
        include(ProcessorCount)
        ProcessorCount(_cc_link_jobs)
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # gcc accepts =auto (since 10) which means "use $(nproc) — same as
        # passing the count explicitly, but resilient to cores being added.
        add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:-flto=auto>)
        add_link_options(-flto=auto)
        message(STATUS "lto: gcc -flto=auto")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # clang's ThinLTO: -flto=thin then -flto-jobs=N for the link pool.
        if(_cc_link_jobs GREATER 0)
            add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:-flto=thin>)
            add_link_options(-flto=thin -flto-jobs=${_cc_link_jobs})
            message(STATUS "lto: clang -flto=thin -flto-jobs=${_cc_link_jobs}")
        endif()
    endif()
endif()

# Default standards: C++23 / C11. A binary CMakeLists.txt can override
# by setting CMAKE_CXX_STANDARD before include()ing this file (the
# testing*.py matrix flips between c++11 and c++23 to cover both ends).
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
if(NOT DEFINED CMAKE_C_STANDARD)
    set(CMAKE_C_STANDARD 11)
endif()

# Common compile flags shared by every executable. catchchallenger_common_flags
# is an INTERFACE library — link it into your exec to inherit the flags.
add_library(catchchallenger_common_flags INTERFACE)
target_compile_options(catchchallenger_common_flags INTERFACE
    -fstack-protector-all
    -fno-rtti
    -fno-exceptions
    -Wall
    -Wextra
    -Wno-missing-braces
)
target_compile_options(catchchallenger_common_flags INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-delete-non-virtual-dtor>
)
target_compile_definitions(catchchallenger_common_flags INTERFACE
    $<$<PLATFORM_ID:Linux>:__linux__>
)

# ── multi-threaded linker autopick ─────────────────────────────────────
# GNU bfd ld is single-threaded; mold and lld are multi-threaded by
# default (mold uses all hw threads, ld.lld uses --threads=all unless
# overridden). Both produce identical link output. Probe what's on the
# host and propagate -fuse-ld=… to every consumer of catchchallenger_common_flags.
#
# Order:
#   1. mold  — fastest on ELF (Linux), but no PE/COFF or Mach-O backend yet
#   2. lld   — works on ELF/PE/Mach-O, so cross-compiles to MinGW/macOS
#              also benefit
#   3. default linker (bfd/system) when neither is installed
#
# Per CLAUDE.md: optional accelerators must NEVER be hard-required —
# the build still produces a working binary on a vanilla VPS without
# either tool. The CACHE-stamped CC_LINKER variable lets a binary's
# CMakeLists.txt override (e.g. force lld even on Linux) without
# editing this file.
if(NOT DEFINED CC_LINKER)
    find_program(_cc_mold_exe mold)
    find_program(_cc_lld_exe NAMES ld.lld lld)
    if(_cc_mold_exe AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
        set(CC_LINKER "mold" CACHE STRING "Linker driver flag picked by CCCommon.cmake")
    elseif(_cc_lld_exe)
        set(CC_LINKER "lld" CACHE STRING "Linker driver flag picked by CCCommon.cmake")
    else()
        set(CC_LINKER "" CACHE STRING "Linker driver flag picked by CCCommon.cmake")
    endif()
endif()
if(CC_LINKER)
    target_link_options(catchchallenger_common_flags INTERFACE -fuse-ld=${CC_LINKER})
    # Both mold and ld.lld default to "all hardware threads" already, but
    # be explicit so a future env where the default changed (or an older
    # lld < 11 that defaulted to single-threaded) still gets full
    # parallelism. ProcessorCount returns 0 when it can't tell — guard
    # against that and only emit the flag with a positive count.
    include(ProcessorCount)
    ProcessorCount(_cc_link_jobs)
    if(_cc_link_jobs GREATER 0)
        if(CC_LINKER STREQUAL "mold")
            # mold spelling: --thread-count=N (also accepts --threads=N as alias)
            target_link_options(catchchallenger_common_flags INTERFACE
                "LINKER:--thread-count=${_cc_link_jobs}")
        elseif(CC_LINKER STREQUAL "lld")
            # ld.lld spelling: --threads=N
            target_link_options(catchchallenger_common_flags INTERFACE
                "LINKER:--threads=${_cc_link_jobs}")
        endif()
    endif()
    message(STATUS "linker: -fuse-ld=${CC_LINKER} (threads=${_cc_link_jobs})")
endif()

# Top-level toggles. Mirror qmake CONFIG+= switches. These are options
# (not cache vars), so callers can override via -D on the cmake line.
# Server-only / client-only options live in the binary CMakeLists.txt
# that consumes them; the ones declared here are shared across multiple
# binaries.
option(CATCHCHALLENGER_NOXML
       "Disable XML parsing (datapack-cache.bin must exist)"
       OFF)
option(CATCHCHALLENGER_DB_FILE
       "Use file-based database (no SQL)"
       OFF)
option(CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK
       "Dump the datapack tree cache for debugging"
       OFF)
option(CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
       "Disable in-server datapack send (mirror-only)"
       OFF)
# Merged from former CATCHCHALLENGER_EXTRA_CHECK + CATCHCHALLENGER_HARDENED
# (both behaved as "extra defensive runtime checks").
#
# **Default OFF** — opt-in via -DCATCHCHALLENGER_HARDENED=ON. Production
# deploys (deploy.sh, Qt Creator) leave it off so a live server never
# turns an invariant breach into a SIGABRT that drops every connected
# player; production prefers the graceful-disconnect path. The
# testing*.py harness opts in unconditionally via
# test/cmake_helpers.py:build_cmake_command(), so every CI build has
# the abort()s wired and protocol-formula drift / invariant breaks
# surface as SIGABRT instead of silent disconnect + parent-side
# wall timeout. Toggles:
#   1. Existing `#ifdef CATCHCHALLENGER_HARDENED` invariant blocks
#      across server/general code → abort() instead of best-effort.
#   2. parseReplyData/parseMessage/parseQuery returning false in
#      general/base/ProtocolParsingInput.cpp → abort() with
#      "error: the protocol parsing was wrong, start under gdb and
#      catch the backtrace" + packetCode + queryNumber + hex dump.
option(CATCHCHALLENGER_HARDENED
       "Enable extra defensive checks (abort on internal invariant violations) — off by default; opt-in for tests"
       OFF)
option(CATCHCHALLENGER_CACHE_HPS
       "Enable HPS binary datapack cache"
       ON)

# Alternative IO backends for the epoll server (server/cli/, server/
# game-server-alone/). Mutually exclusive — the foreach below FATAL_ERRORs
# when more than one is set. Per-platform defaults: Linux defaults to
# epoll(7); other platforms default to select(2).
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(_default_select OFF)
else()
    set(_default_select ON)
endif()
option(CATCHCHALLENGER_SELECT
       "Use select(2) instead of epoll for the server event loop"
       ${_default_select})
option(CATCHCHALLENGER_POLL
       "Use poll(2) instead of epoll for the server event loop"
       OFF)
option(CATCHCHALLENGER_IO_URING
       "Use io_uring instead of epoll for the server event loop"
       OFF)
set(_backend_count 0)
foreach(_be CATCHCHALLENGER_SELECT CATCHCHALLENGER_POLL CATCHCHALLENGER_IO_URING)
    if(${_be})
        math(EXPR _backend_count "${_backend_count}+1")
    endif()
endforeach()
if(_backend_count GREATER 1)
    message(FATAL_ERROR
        "CATCHCHALLENGER_SELECT, CATCHCHALLENGER_POLL, and CATCHCHALLENGER_IO_URING are mutually exclusive")
endif()
if(CATCHCHALLENGER_IO_URING)
    find_library(LIBURING_LIBRARY NAMES uring REQUIRED)
    find_path(LIBURING_INCLUDE_DIR NAMES liburing.h REQUIRED)
endif()
# Detect provided-buffer-ring helpers (added in liburing 2.3, kernel
# 5.19). Remote build nodes on older distros (e.g. Debian-stable
# liburing 2.1) lack these inline helpers; the buf-ring code path in
# server/cli/EventLoop.cpp must compile out cleanly there. The cluster
# binaries (master/login/gateway/game-server-alone) define
# CATCHCHALLENGER_IO_URING as a per-target compile flag (not via the
# CCCommon option above), so the check must run unconditionally
# whenever liburing.h is on the system.
find_path(_cc_liburing_h_for_check NAMES liburing.h)
if(_cc_liburing_h_for_check)
    find_library(_cc_liburing_lib_for_check NAMES uring)
    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_INCLUDES "${_cc_liburing_h_for_check}")
    if(_cc_liburing_lib_for_check)
        set(CMAKE_REQUIRED_LIBRARIES "${_cc_liburing_lib_for_check}")
    endif()
    check_cxx_source_compiles("
        #include <liburing.h>
        int main(){
            io_uring r; int err=0;
            io_uring_buf_ring *br=io_uring_setup_buf_ring(&r,4,0,0,&err);
            io_uring_free_buf_ring(&r,br,4,0);
            return 0;
        }
    " CC_HAS_URING_BUF_RING)
    if(CC_HAS_URING_BUF_RING)
        add_compile_definitions(CC_HAS_URING_BUF_RING=1)
    endif()
    set(CMAKE_REQUIRED_INCLUDES "")
    set(CMAKE_REQUIRED_LIBRARIES "")
endif()

# zstd: external system libzstd, or vendored static build under
# general/libzstd. Cross-compile targets (Android NDK, MXE/mingw) almost
# never have a ready system libzstd; auto-default to the vendored copy
# there so qt-cmake configure succeeds out of the box.
if(ANDROID OR CMAKE_CROSSCOMPILING)
    set(_externallibzstd_default OFF)
else()
    set(_externallibzstd_default ON)
endif()
option(EXTERNALLIBZSTD "Link against system libzstd" ${_externallibzstd_default})

# zlib: server uses it for tiled map decompression (TILED_ZLIB).
# Uses system zlib when available, vendored fallback otherwise.
include(${CMAKE_CURRENT_LIST_DIR}/libzlib.cmake)

if(EXTERNALLIBZSTD)
    find_library(ZSTD_LIBRARY NAMES zstd REQUIRED)
else()
    # Suppress libzstd's own programs/tests; we only consume libzstd_static.
    set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_SHARED   OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_STATIC   ON  CACHE BOOL "" FORCE)
    set(ZSTD_LEGACY_SUPPORT OFF CACHE BOOL "" FORCE)
    add_subdirectory(${CC_REPO_ROOT}/general/libzstd/build/cmake
                     ${CMAKE_BINARY_DIR}/_libzstd EXCLUDE_FROM_ALL)
    set(ZSTD_LIBRARY libzstd_static)
endif()

# ── system-vs-vendored discovery for the small leaf libs ───────────────
# For each of blake3 / xxhash / tinyxml2 we try to find the system shared
# library AND its public header, and we accept it only when it advertises
# at least the minimum version the vendored copy ships. When system is
# accepted CC_USE_SYSTEM_<X>=ON; otherwise the vendored sources are
# compiled in by general/general.cmake. Cross-compile builds default to
# vendored (system is the host's, not the target's).
include(CheckCSourceCompiles)

# Helper: probe a system <lib>.h for a "<MAJOR>.<MINOR>.<PATCH>" string
# >= the vendored minimum. Pure preprocessor — no link required, so we
# can short-circuit the find_library call when the version is too old.
function(_cc_check_system_header HEADER_PATH VERSION_DEFINE MIN_VERSION OUT_VAR)
    if(NOT EXISTS "${HEADER_PATH}")
        set(${OUT_VAR} OFF PARENT_SCOPE)
        return()
    endif()
    file(STRINGS "${HEADER_PATH}" _ver_line REGEX "${VERSION_DEFINE}.*\"[0-9]+\\.[0-9]+\\.[0-9]+\"")
    if(NOT _ver_line)
        set(${OUT_VAR} OFF PARENT_SCOPE)
        return()
    endif()
    string(REGEX REPLACE ".*\"([0-9]+\\.[0-9]+\\.[0-9]+)\".*" "\\1" _ver "${_ver_line}")
    if(_ver VERSION_LESS "${MIN_VERSION}")
        set(${OUT_VAR} OFF PARENT_SCOPE)
    else()
        set(${OUT_VAR} ON PARENT_SCOPE)
        set(${OUT_VAR}_VERSION "${_ver}" PARENT_SCOPE)
    endif()
endfunction()

# Force-toggle hooks for distros / CI nodes that need to pin behaviour.
# Default: try system on a normal host build; force vendored on cross.
if(ANDROID OR CMAKE_CROSSCOMPILING)
    set(_system_default OFF)
else()
    set(_system_default ON)
endif()
option(EXTERNALLIBBLAKE3   "Use system libblake3 if found"   ${_system_default})
option(EXTERNALLIBXXHASH   "Use system libxxhash if found"   ${_system_default})
option(EXTERNALLIBTINYXML2 "Use system libtinyxml2 if found" ${_system_default})

# blake3 — vendored is 1.8.2; require >= 1.8.0 from system.
set(CC_USE_SYSTEM_BLAKE3 OFF)
if(EXTERNALLIBBLAKE3)
    find_path(BLAKE3_INCLUDE_DIR NAMES blake3.h)
    find_library(BLAKE3_LIBRARY NAMES blake3)
    if(BLAKE3_INCLUDE_DIR AND BLAKE3_LIBRARY)
        _cc_check_system_header("${BLAKE3_INCLUDE_DIR}/blake3.h" "BLAKE3_VERSION_STRING" "1.8.0" _blake3_ok)
        if(_blake3_ok)
            set(CC_USE_SYSTEM_BLAKE3 ON)
            message(STATUS "blake3: system ${BLAKE3_INCLUDE_DIR}/blake3.h v${_blake3_ok_VERSION}")
        endif()
    endif()
endif()

# xxhash — vendored matches upstream 0.8.x (XXH_VERSION_NUMBER 800+); require >= 0.8.0.
set(CC_USE_SYSTEM_XXHASH OFF)
if(EXTERNALLIBXXHASH)
    find_path(XXHASH_INCLUDE_DIR NAMES xxhash.h)
    find_library(XXHASH_LIBRARY NAMES xxhash)
    if(XXHASH_INCLUDE_DIR AND XXHASH_LIBRARY)
        # xxhash exposes its version as 3 separate numeric defines, not a
        # quoted string — synthesise a comparison without the helper.
        file(STRINGS "${XXHASH_INCLUDE_DIR}/xxhash.h" _xxh_major REGEX "#define[ \t]+XXH_VERSION_MAJOR[ \t]+[0-9]+")
        file(STRINGS "${XXHASH_INCLUDE_DIR}/xxhash.h" _xxh_minor REGEX "#define[ \t]+XXH_VERSION_MINOR[ \t]+[0-9]+")
        if(_xxh_major AND _xxh_minor)
            string(REGEX REPLACE ".*XXH_VERSION_MAJOR[ \t]+([0-9]+).*" "\\1" _xxh_M "${_xxh_major}")
            string(REGEX REPLACE ".*XXH_VERSION_MINOR[ \t]+([0-9]+).*" "\\1" _xxh_m "${_xxh_minor}")
            if(_xxh_M GREATER 0 OR _xxh_m GREATER_EQUAL 8)
                set(CC_USE_SYSTEM_XXHASH ON)
                message(STATUS "xxhash: system ${XXHASH_INCLUDE_DIR}/xxhash.h v${_xxh_M}.${_xxh_m}.x")
            endif()
        endif()
    endif()
endif()

# tinyxml2 — vendored is the 2016 v3 fork. The system-side header switched
# to <tinyxml2.h> with the same tinyxml2:: namespace and the call sites we
# use (XMLDocument, LoadFile, FirstChildElement, IntAttribute, …) are
# stable from v6 onwards. Require >= 9.0.0 to stay safely above the minor
# API renames in 5.x.
set(CC_USE_SYSTEM_TINYXML2 OFF)
if(EXTERNALLIBTINYXML2)
    find_path(TINYXML2_INCLUDE_DIR NAMES tinyxml2.h)
    find_library(TINYXML2_LIBRARY NAMES tinyxml2)
    if(TINYXML2_INCLUDE_DIR AND TINYXML2_LIBRARY)
        # tinyxml2 exposes TIXML2_MAJOR_VERSION / _MINOR / _PATCH as
        # `static const int` constants (not preprocessor strings), so we
        # match the integer literal directly.
        file(STRINGS "${TINYXML2_INCLUDE_DIR}/tinyxml2.h" _tx2_major REGEX "TIXML2_MAJOR_VERSION[ \t]*=[ \t]*[0-9]+")
        if(_tx2_major)
            string(REGEX REPLACE ".*TIXML2_MAJOR_VERSION[ \t]*=[ \t]*([0-9]+).*" "\\1" _tx2_M "${_tx2_major}")
            if(_tx2_M GREATER_EQUAL 9)
                set(CC_USE_SYSTEM_TINYXML2 ON)
                message(STATUS "tinyxml2: system ${TINYXML2_INCLUDE_DIR}/tinyxml2.h v${_tx2_M}.x")
            endif()
        endif()
    endif()
endif()

# When system tinyxml2 is in use we still need the source-level compat
# header path "tinyxml2.hpp" (vendored uses the .hpp suffix) to resolve.
# Generate a one-line shim into the build tree and put that on the
# include path so every existing `#include "tinyxml2.hpp"` keeps working.
if(CC_USE_SYSTEM_TINYXML2)
    set(_tx2_shim_dir "${CMAKE_BINARY_DIR}/_tinyxml2_shim")
    file(MAKE_DIRECTORY "${_tx2_shim_dir}")
    file(WRITE "${_tx2_shim_dir}/tinyxml2.hpp"
         "#pragma once\n#include <tinyxml2.h>\n")
    set(CC_TINYXML2_SHIM_DIR "${_tx2_shim_dir}")
    # Source-level branch flag for callers that touch APIs renamed
    # between v3 (vendored, 2016) and v6+ (e.g. GetErrorStr1/2 → ErrorStr).
    add_compile_definitions(CC_TINYXML2_SYSTEM=1)
endif()
