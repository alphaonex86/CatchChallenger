# benchmark/benchmarkmapmanager/CMakeLists.txt -- benchmark_min_network.
#
# Self-contained, one binary per CMakeLists.txt (root CLAUDE.md).
# Re-uses test/testingmapmanagement/Stubs.{hpp,cpp} so the production
# server-side MapVisibilityAlgorithm.cpp + MapServer.cpp compile here
# under -DCATCHCHALLENGER_TESTING with NO server stack pulled in
# (Client/ClientList/GlobalServerData are stubbed; zero syscalls
# beyond libc init, no event loop, no DB).
#
# CATCHCHALLENGER_BENCHMARK is also defined here (per benchmark/
# CLAUDE.md) so future benchmark-only #ifdef hooks can land in the
# production .cpp without any cost in the production build.

cmake_minimum_required(VERSION 3.20)
project(benchmark_min_network LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CC_REPO_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)

# Force vendored static zlib so the binary has no runtime dep on the
# compile node's system libz.so — exec nodes may carry a different
# version and the benchmark must run unmodified across the fleet.
set(EXTERNALLIBZLIB OFF CACHE BOOL "" FORCE)

include(${CC_REPO_ROOT}/general/CCCommon.cmake)
include(${CC_REPO_ROOT}/general/general.cmake)

find_package(Threads REQUIRED)

add_executable(benchmark_min_network
    main.cpp
    ${CC_REPO_ROOT}/test/testingmapmanagement/Stubs.cpp
    ${CC_REPO_ROOT}/server/base/MapManagement/MapVisibilityAlgorithm.cpp
    ${CC_REPO_ROOT}/server/base/MapServer.cpp
)

target_include_directories(benchmark_min_network PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CC_REPO_ROOT}
    ${CC_REPO_ROOT}/test/testingmapmanagement
)

# CATCHCHALLENGER_TESTING enables the Stubs.hpp injection at the top of
# the production server-side .cpp files (same mechanism testingmapmanagement
# uses). Branch-trace prints are also gated on it -- harmless overhead
# for a benchmark, but it makes timing figures comparable across runs.
target_compile_definitions(benchmark_min_network PRIVATE
    CATCHCHALLENGER_TESTING
    CATCHCHALLENGER_HARDENED
    CATCHCHALLENGER_BENCHMARK
    CATCHCHALLENGER_CLIENT
    CATCHCHALLENGER_DB_FILE
)

# Opt-in: wrap the timed loop in CALLGRIND_TOGGLE_COLLECT so a callgrind
# profile excludes process startup (dynamic linking dominates an
# otherwise-tiny single-core run). Needs <valgrind/callgrind.h>; the
# harness pairs this with `valgrind --collect-atstart=no`. Off by default
# => macros expand to nothing, default benchmark build unchanged. The
# header is auto-probed so passing the flag on a node WITHOUT valgrind
# headers degrades gracefully (markers stay no-ops) instead of failing
# the build -- the harness builds one binary for all profilers.
if(CATCHCHALLENGER_BENCH_CALLGRIND)
    include(CheckIncludeFileCXX)
    check_include_file_cxx("valgrind/callgrind.h" CC_HAS_CALLGRIND_H)
    if(CC_HAS_CALLGRIND_H)
        target_compile_definitions(benchmark_min_network PRIVATE CATCHCHALLENGER_BENCH_CALLGRIND)
    else()
        message(STATUS "benchmark_min_network: valgrind/callgrind.h absent, "
                "CALLGRIND_TOGGLE_COLLECT markers stay inert")
    endif()
endif()

target_compile_options(benchmark_min_network PRIVATE
    -Wall -Wextra -Wno-unused-parameter
    -O2 -DNDEBUG
)
target_compile_options(benchmark_min_network PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-include cstring>
)

target_link_libraries(benchmark_min_network PRIVATE
    catchchallenger_common_flags
    catchchallenger_general
    Threads::Threads
    ZLIB::ZLIB
)

if(EXTERNALLIBZSTD)
    target_link_libraries(benchmark_min_network PRIVATE ${ZSTD_LIBRARY})
endif()

# Linker driver: use the test-link-verified accelerator that
# CCCommon.cmake picked for THIS target (mold on native ELF,
# lld/bfd where mold has no backend, e.g. MIPS). Never hard-code
# a specific linker here — CCCommon is the single source of truth.
if(CC_LINKER)
    target_link_options(benchmark_min_network PRIVATE -fuse-ld=${CC_LINKER})
endif()
