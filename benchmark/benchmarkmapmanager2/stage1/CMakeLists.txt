# benchmark/benchmarkmapmanager2/stage1/CMakeLists.txt -- benchmark_world_stage1.
#
# STAGE 1: reads the datapack's world with the PRODUCTION loader and WRITES the
# workload (players, spawns, movement vectors) as a C++ source for stage 2.
# It runs on the orchestrating host and is never measured, so it may link the
# whole loader + datapack parser; stage 2 links none of it.
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
project(benchmark_world_stage1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CC_REPO_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../..)

# Force vendored static zlib so the binary has no runtime dep on the
# compile node's system libz.so — exec nodes may carry a different
# version and the benchmark must run unmodified across the fleet.
set(EXTERNALLIBZLIB OFF CACHE BOOL "" FORCE)

include(${CC_REPO_ROOT}/general/CCCommon.cmake)
include(${CC_REPO_ROOT}/general/general.cmake)

find_package(Threads REQUIRED)

add_executable(benchmark_world_stage1
    main.cpp
)

target_include_directories(benchmark_world_stage1 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CC_REPO_ROOT}
)

# A plain host tool: no stub world, no server-side sources, so none of the
# testing/benchmark seams are needed. CATCHCHALLENGER_CLIENT keeps the datapack
# parser on its non-server path (the one that can free its XML cache).
target_compile_definitions(benchmark_world_stage1 PRIVATE
    CATCHCHALLENGER_HARDENED
    CATCHCHALLENGER_CLIENT
    CATCHCHALLENGER_DB_FILE
)

target_compile_options(benchmark_world_stage1 PRIVATE
    -Wall -Wextra -Wno-unused-parameter
    -O2 -DNDEBUG
)
target_compile_options(benchmark_world_stage1 PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-include cstring>
)

target_link_libraries(benchmark_world_stage1 PRIVATE
    catchchallenger_common_flags
    catchchallenger_general
    Threads::Threads
    ZLIB::ZLIB
)

if(EXTERNALLIBZSTD)
    target_link_libraries(benchmark_world_stage1 PRIVATE ${ZSTD_LIBRARY})
endif()

# Linker driver: use the test-link-verified accelerator that
# CCCommon.cmake picked for THIS target (mold on native ELF,
# lld/bfd where mold has no backend, e.g. MIPS). Never hard-code
# a specific linker here — CCCommon is the single source of truth.
if(CC_LINKER)
    target_link_options(benchmark_world_stage1 PRIVATE -fuse-ld=${CC_LINKER})
endif()
