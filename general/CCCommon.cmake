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
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER   "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

# AUTOMOC speed-ups. Same rationale as the qmake build.
set(CMAKE_AUTOGEN_PARALLEL AUTO)
set(CMAKE_AUTOMOC_RELAXED_MODE OFF)

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
# (both behaved as "extra defensive runtime checks"); now a single ON
# default toggles abort()-on-invariant in server + general code.
option(CATCHCHALLENGER_HARDENED
       "Enable extra defensive checks (abort on internal invariant violations)"
       ON)
option(CATCHCHALLENGER_CACHE_HPS
       "Enable HPS binary datapack cache"
       ON)

# Alternative IO backends for the epoll server (server/epoll/, server/
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
find_package(ZLIB REQUIRED)

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
