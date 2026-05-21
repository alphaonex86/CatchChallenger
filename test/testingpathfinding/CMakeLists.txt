# test/testingpathfinding/CMakeLists.txt — testpathfinding binary.
#
# Self-contained, one binary per CMakeLists.txt (root CLAUDE.md).
# Compiles the REAL client/libqtcatchchallenger/maprender/PathFinding.cpp
# so any change to the planner is picked up automatically, plus a tiny
# driver (main.cpp) that loads city.tmx into a fixed MapServer (is-a
# CommonMap, see StubMapServer.hpp), injects a deterministic item-gated
# zone and asserts the route never crosses it without the item.
#
# NO network, NO event loop exec, NO DB. Reads city.tmx once, runs in
# RAM, exits in well under a second.
#
# PathFinding is built with -DNOTHREADS so it is a plain QObject on the
# calling thread — the driver runs it synchronously, no QThread, no
# queued-connection metatype marshalling.

cmake_minimum_required(VERSION 3.20)
project(testpathfinding LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

find_package(Qt6 COMPONENTS Core Gui Xml Network OpenGL Widgets REQUIRED)
find_package(Threads REQUIRED)

include(${CC_REPO_ROOT}/general/general.cmake)
# PathFinding.hpp -> MapVisualiserOrder.hpp pulls libtiled headers
# (<map.h>, <maprenderer.h>, …); catchchallenger_tiled provides them.
include(${CC_REPO_ROOT}/client/libqtcatchchallenger/libtiled.cmake)

add_executable(testpathfinding
    main.cpp
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/maprender/PathFinding.cpp
)

set_target_properties(testpathfinding PROPERTIES
    AUTOMOC ON
)

target_include_directories(testpathfinding PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CC_REPO_ROOT}
    ${CC_REPO_ROOT}/client/libcatchchallenger
    ${CC_REPO_ROOT}/client/libqtcatchchallenger
)

# CATCHCHALLENGER_TESTING      -> CommonDatapack::testing_setMonstersCollision
# NOTHREADS                    -> PathFinding is a QObject, not a QThread
# CATCHCHALLENGER_ONLYMAPRENDER -> Map_loader::tryLoadMap() parses a lone
#                                 .tmx without a loaded datapack name
#                                 table (same as tools/map2png); the
#                                 test never needs item/monster names.
# CATCHCHALLENGER_NO_WEBSOCKET  -> keep the websocket stack out (parity
#                                 with map2png; nothing here uses it).
# CATCHCHALLENGER_HARDENED     -> forced ON by the harness (cmake_helpers).
target_compile_definitions(testpathfinding PRIVATE
    CATCHCHALLENGER_TESTING
    NOTHREADS
    CATCHCHALLENGER_ONLYMAPRENDER
    CATCHCHALLENGER_NO_WEBSOCKET
)

target_compile_options(testpathfinding PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    # Several production general/ .cpp files use memcpy/memset without
    # an explicit <cstring>; they get it transitively in a full build.
    # CXX-only (cstring is C++-only).
    $<$<COMPILE_LANGUAGE:CXX>:-include cstring>
)

target_link_libraries(testpathfinding PRIVATE
    catchchallenger_common_flags
    catchchallenger_general
    catchchallenger_tiled
    Qt6::Core
    Qt6::Gui
    Qt6::Xml
    Qt6::Network
    Qt6::OpenGL
    Qt6::Widgets
    Threads::Threads
    ZLIB::ZLIB
)

if(EXTERNALLIBZSTD)
    target_link_libraries(testpathfinding PRIVATE ${ZSTD_LIBRARY})
endif()

# Linker driver: use the test-link-verified accelerator CCCommon.cmake
# picked for THIS target. Never hard-code — CCCommon is the single
# source of truth.
if(CC_LINKER)
    target_link_options(testpathfinding PRIVATE -fuse-ld=${CC_LINKER})
endif()
