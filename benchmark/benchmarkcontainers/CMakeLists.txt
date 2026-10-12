# benchmark/benchmarkcontainers/CMakeLists.txt -- benchmark_containers.
#
# std::vector vs std::list vs std::deque on real datapack data, read with the
# production loader (CommonDatapack + FacilityLibGeneral). One binary per
# CMakeLists.txt (root CLAUDE.md). The datapack is a run-time argument.

cmake_minimum_required(VERSION 3.20)
project(benchmark_containers LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CC_REPO_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)

# Vendored static zlib: no runtime dependency on the node's libz.so.
set(EXTERNALLIBZLIB OFF CACHE BOOL "" FORCE)

include(${CC_REPO_ROOT}/general/CCCommon.cmake)
include(${CC_REPO_ROOT}/general/general.cmake)

find_package(Threads REQUIRED)

add_executable(benchmark_containers main.cpp)

target_include_directories(benchmark_containers PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CC_REPO_ROOT}
)

# Same loader configuration as benchmarkmapmanager2's stage 1: the datapack
# parser on its non-server path.
target_compile_definitions(benchmark_containers PRIVATE
    CATCHCHALLENGER_CLIENT
    CATCHCHALLENGER_DB_FILE
)

target_compile_options(benchmark_containers PRIVATE
    -Wall -Wextra -Wno-unused-parameter
    -O2 -DNDEBUG
)
target_compile_options(benchmark_containers PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-include cstring>
)

target_link_libraries(benchmark_containers PRIVATE
    catchchallenger_common_flags
    catchchallenger_general
    Threads::Threads
    ZLIB::ZLIB
)

if(EXTERNALLIBZSTD)
    target_link_libraries(benchmark_containers PRIVATE ${ZSTD_LIBRARY})
endif()

if(CC_LINKER)
    target_link_options(benchmark_containers PRIVATE -fuse-ld=${CC_LINKER})
endif()
