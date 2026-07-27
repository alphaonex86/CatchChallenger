# test/testingoverworld/CMakeLists.txt — testoverworld binary.
#
# Self-contained, one binary per CMakeLists.txt (root CLAUDE.md).
# Compiles the REAL client/libqtcatchchallenger/maprender/MonsterSheet.cpp so any
# change to the monster overworld sheet layouts is picked up automatically, plus a
# tiny driver (main.cpp) that walks every direction and every animation step and
# paints the tiles the renderer would show into a contact sheet.
#
# NO network, NO event loop exec, NO DB. Reads two PNGs, runs in RAM, exits in well
# under a second.

cmake_minimum_required(VERSION 3.20)
project(testoverworld LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

find_package(Qt6 COMPONENTS Core Gui REQUIRED)
find_package(Threads REQUIRED)

# MonsterSheet.hpp -> libtiled <tileset.h>; catchchallenger_tiled provides it
include(${CC_REPO_ROOT}/client/libqtcatchchallenger/libtiled.cmake)

add_executable(testoverworld
    main.cpp
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/maprender/MonsterSheet.cpp
)

target_include_directories(testoverworld PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CC_REPO_ROOT}
    ${CC_REPO_ROOT}/client/libcatchchallenger
    ${CC_REPO_ROOT}/client/libqtcatchchallenger
)

# CATCHCHALLENGER_NOAUDIO / _NO_WEBSOCKET -> nothing here needs them, keep the
#                                            dependency surface minimal
# CATCHCHALLENGER_HARDENED                -> forced ON by the harness (cmake_helpers)
target_compile_definitions(testoverworld PRIVATE
    CATCHCHALLENGER_NOAUDIO
    CATCHCHALLENGER_NO_WEBSOCKET
)

target_compile_options(testoverworld PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
)

target_link_libraries(testoverworld PRIVATE
    catchchallenger_common_flags
    catchchallenger_tiled
    Qt6::Core
    Qt6::Gui
    Threads::Threads
    ZLIB::ZLIB
)

if(EXTERNALLIBZSTD)
    target_link_libraries(testoverworld PRIVATE ${ZSTD_LIBRARY})
endif()

# Linker driver: use the test-link-verified accelerator CCCommon.cmake picked for
# THIS target. Never hard-code — CCCommon is the single source of truth.
if(CC_LINKER)
    target_link_options(testoverworld PRIVATE -fuse-ld=${CC_LINKER})
endif()
