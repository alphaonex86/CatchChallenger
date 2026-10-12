# benchmark/benchmarkcontainers/embedded/CMakeLists.txt --
# benchmark_containers_embedded: the embedded suite of benchmarkcontainers as a
# Linux binary, data compiled in (no datapack loader): for the i486 laptop.
#   cmake -S . -B <out> -DCC_CONTAINERS_DATA_CPP=<generated .cpp>

cmake_minimum_required(VERSION 3.20)
project(benchmark_containers_embedded LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CC_REPO_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../..)
include(${CC_REPO_ROOT}/general/CCCommon.cmake)

if(NOT CC_CONTAINERS_DATA_CPP OR NOT EXISTS "${CC_CONTAINERS_DATA_CPP}")
    message(FATAL_ERROR "CC_CONTAINERS_DATA_CPP is not set or does not exist: "
                        "generate it with benchmark_containers --emit-esp32 (benchmarkcontainers.py does)")
endif()

add_executable(benchmark_containers_embedded main.cpp ${CC_CONTAINERS_DATA_CPP})
target_compile_options(benchmark_containers_embedded PRIVATE -Wall -Wextra)
target_link_libraries(benchmark_containers_embedded PRIVATE catchchallenger_common_flags)

if(CC_LINKER)
    target_link_options(benchmark_containers_embedded PRIVATE -fuse-ld=${CC_LINKER})
endif()
