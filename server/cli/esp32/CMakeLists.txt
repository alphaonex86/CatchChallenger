# server/cli/esp32/CMakeLists.txt — ESP-IDF project for the CatchChallenger
# all-in-one game server on ESP32 (datapack + settings in flash, no filesystem).
#
# Build (after `. $IDF_PATH/export.sh`):
#   idf.py -B <build> -DCATCHCHALLENGER_DATAPACK_CPP_DIR=/mnt/data/perso/tmpfs/datapack-cpp build
# The DATAPACK_CPP_DIR holds datapack_cpp.{cpp,hpp} produced by stage1
# (catchchallenger-server-cli --save, CATCHCHALLENGER_DATAPACK_CPP_EMIT). See
# test/ESP32.md and test/testingcompilationESP32.py.

cmake_minimum_required(VERSION 3.16)

# Forward the generated-datapack dir into the main component (it is read in
# main/CMakeLists.txt). idf.py passes it as a -D cache var.
set(CATCHCHALLENGER_DATAPACK_CPP_DIR "$CACHE{CATCHCHALLENGER_DATAPACK_CPP_DIR}"
    CACHE PATH "Dir holding the generated datapack_cpp.{cpp,hpp}")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(catchchallenger-server-esp32)
