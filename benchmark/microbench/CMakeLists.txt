cmake_minimum_required(VERSION 3.20)
project(benchmark_microbench LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Same -O2 on every node (benchmark_microbench.py builds the host copy alike):
# the optimisation level changes how much loop overhead the MHz figure absorbs.
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

add_executable(benchmark_microbench main.cpp)
