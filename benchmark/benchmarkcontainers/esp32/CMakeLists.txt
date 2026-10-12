# benchmark/benchmarkcontainers/esp32/CMakeLists.txt -- ESP-IDF project running
# benchmarkcontainers ON the ESP32 (its own libstdc++ + heap allocator): the
# files and items datasets of the datapack, compiled into flash by the host
# (`benchmark_containers --emit-esp32`), plus the per-player FIFO pattern.
# No WiFi, no filesystem. The generated data comes from the environment and is
# never checked in:
#   CC_CONTAINERS_DATA_CPP=/path/containers_data.cpp idf.py -B <out-of-tree> build
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(benchmark-containers-esp32)
