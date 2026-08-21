# benchmark/benchmarkmapmanager2/stage2/CMakeLists.txt
#   -> benchmark_min_balanced_replay
#
# STAGE 2: the binary that is actually MEASURED. It carries its whole workload
# compiled in -- the .cpp stage 1 generated for one execution node and one
# datapack (players, spawns, movement vectors) -- so at runtime it reads
# nothing: no datapack, no map files, no collision data. That is what lets it
# run on a board with no filesystem, and it keeps the load model down to a
# countdown and a coordinate update per player per tick.
#
# The generated source is passed in, never checked in:
#   cmake -S stage2 -B <build> -DCC_WORKLOAD_CPP=/path/to/workload-<node>.cpp
#
# Self-contained, one binary per CMakeLists.txt (root CLAUDE.md). Re-uses
# test/testingmapmanagement/Stubs.{hpp,cpp} so the production server-side
# MapVisibilityAlgorithm.cpp + MapServer.cpp compile with NO server stack
# pulled in (Client/ClientList/GlobalServerData are stubbed; no event loop,
# no DB, no datapack parser).

cmake_minimum_required(VERSION 3.20)
project(benchmark_min_balanced_replay LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CC_REPO_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../..)

if(NOT CC_WORKLOAD_CPP)
    message(FATAL_ERROR
        "CC_WORKLOAD_CPP is not set: stage 2 has no workload to replay.\n"
        "Generate one with benchmarkmapmanager2/stage1 and pass it:\n"
        "  cmake -S <this dir> -B <build> -DCC_WORKLOAD_CPP=/path/workload.cpp")
endif()
if(NOT EXISTS ${CC_WORKLOAD_CPP})
    message(FATAL_ERROR "CC_WORKLOAD_CPP does not exist: ${CC_WORKLOAD_CPP}")
endif()

# Force vendored static zlib so the binary has no runtime dep on the compile
# node's system libz.so -- exec nodes may carry a different version and the
# benchmark must run unmodified across the fleet.
set(EXTERNALLIBZLIB OFF CACHE BOOL "" FORCE)

include(${CC_REPO_ROOT}/general/CCCommon.cmake)
include(${CC_REPO_ROOT}/general/general.cmake)

find_package(Threads REQUIRED)

add_executable(benchmark_min_balanced_replay
    main.cpp
    ${CC_WORKLOAD_CPP}
    ${CC_REPO_ROOT}/test/testingmapmanagement/Stubs.cpp
    ${CC_REPO_ROOT}/server/base/MapManagement/MapVisibilityAlgorithm.cpp
    ${CC_REPO_ROOT}/server/base/MapServer.cpp
)

target_include_directories(benchmark_min_balanced_replay PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CC_REPO_ROOT}
    ${CC_REPO_ROOT}/test/testingmapmanagement
)

# CATCHCHALLENGER_TESTING enables the Stubs.hpp injection at the top of the
# production server-side .cpp files (the mechanism testingmapmanagement uses),
# and with it the x/y range guard that checks every broadcast coordinate
# against the map dimensions the workload carries.
target_compile_definitions(benchmark_min_balanced_replay PRIVATE
    CATCHCHALLENGER_TESTING
    CATCHCHALLENGER_HARDENED
    CATCHCHALLENGER_BENCHMARK
    CATCHCHALLENGER_CLIENT
    CATCHCHALLENGER_DB_FILE
)

target_compile_options(benchmark_min_balanced_replay PRIVATE
    -Wall -Wextra -Wno-unused-parameter
    -O2 -DNDEBUG
)
target_compile_options(benchmark_min_balanced_replay PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-include cstring>
)

target_link_libraries(benchmark_min_balanced_replay PRIVATE
    catchchallenger_common_flags
    catchchallenger_general
    Threads::Threads
    ZLIB::ZLIB
)

if(EXTERNALLIBZSTD)
    target_link_libraries(benchmark_min_balanced_replay PRIVATE ${ZSTD_LIBRARY})
endif()

# Linker driver: the test-link-verified accelerator CCCommon.cmake picked for
# THIS target (mold on native ELF, lld/bfd where mold has no backend).
if(CC_LINKER)
    target_link_options(benchmark_min_balanced_replay PRIVATE -fuse-ld=${CC_LINKER})
endif()
