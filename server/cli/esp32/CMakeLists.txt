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

# FD_SETSIZE must EXCEED CONFIG_LWIP_MAX_SOCKETS so lwIP's socket fd range starts
# at a non-zero LWIP_SOCKET_OFFSET (= FD_SETSIZE - MAX_SOCKETS), leaving the low
# FDs for stdin/stdout/stderr. Without this, FD_SETSIZE collapses to
# MEMP_NUM_NETCONN(=MAX_SOCKETS) -> offset 0 -> esp_vfs_lwip_sockets_register
# tries to claim fd 0 (already owned by the console VFS) and ABORTS at boot
# (ESP_ERR_INVALID_ARG, vfs_lwip.c:109). The lwIP Kconfig help spells this out:
# "If using value above 61, update CMakeLists defining FD_SETSIZE to the number
# of sockets used plus the expected open files (minimum of +3 for stdout, stderr
# and stdin)." We raise MAX_SOCKETS to 60 for the benchmark's 50-bot slice, so
# pin FD_SETSIZE generously above CONFIG_LWIP_MAX_SOCKETS so LWIP_SOCKET_OFFSET
# (= FD_SETSIZE - MAX_SOCKETS) leaves ample room for the stdio/console FDs the
# console opens at boot before lwIP registers its fd-range. With MAX_SOCKETS=24,
# FD_SETSIZE=40 -> offset 16 (well clear of the 3 stdio FDs; an offset of 0 made
# esp_vfs_lwip_sockets_register collide with fd 0 and abort at boot). This is a
# GLOBAL build define (idf_build_set_property) so the lwip component sees it too,
# not just main/. ESP32-only — host builds are untouched.
idf_build_set_property(COMPILE_DEFINITIONS "FD_SETSIZE=40" APPEND)
