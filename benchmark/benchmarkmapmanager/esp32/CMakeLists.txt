# benchmark/benchmarkmapmanager/esp32/CMakeLists.txt — ESP-IDF project for the
# MapVisibilityAlgorithm::min_network() PURE-CPU microbenchmark on ESP32.
#
# Unlike the server firmware (server/cli/esp32/) this image has NO WiFi, NO
# lwIP, NO datapack-cpp, NO server: app_main() just runs the same harness the
# native fleet runs (benchmark/benchmarkmapmanager/main.cpp) and prints the
# identical `BENCH players=N ...` lines to UART0 (serial), so the on-device
# numbers are directly comparable against the rest of the fleet.
#
# Build (after `. $IDF_PATH/export.sh`):
#   idf.py -B <build-dir-out-of-tree> set-target esp32 build
# Then flash + read serial later (the benchmark*.py drives this).

cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(benchmark-mapmanager-esp32)
