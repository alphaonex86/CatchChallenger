# test/testingmapmanagement/CMakeLists.txt — testmapvisibility binary.
#
# Self-contained, one binary per CMakeLists.txt (per root CLAUDE.md).
# Compiles:
#   - main.cpp                (test driver + scenarios)
#   - Stubs.cpp               (out-of-line stub definitions for server-
#                              side Client/ClientList/MapServer surface)
#   - TestApiProtocol.cpp     (concrete Api_protocol subclass: every
#                              pure virtual stubbed, map callbacks
#                              forwarded to MapControllerMpStub).
#   - MapVisibilityAlgorithm.cpp + MapServer.cpp from server/ (compiled
#                              with -DCATCHCHALLENGER_TESTING so
#                              Stubs.hpp intercepts the server-side
#                              header chain).
#   - catchchallenger_general + catchchallenger_client_lib (INTERFACE
#                              libs) for the REAL Api_protocol +
#                              ProtocolParsing pipeline. The test feeds
#                              captured server bytes straight into the
#                              production parseMessage / parseQuery /
#                              parseReplyData paths so payload-format
#                              changes in client/libcatchchallenger/
#                              Api_protocol_*.cpp are picked up
#                              automatically — no test-side edit needed.
#
# NO network, NO event loop, NO datapack data loaded. The binary runs
# in pure RAM and exits in <1s.

cmake_minimum_required(VERSION 3.20)
project(testmapvisibility LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CC_REPO_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)

include(${CC_REPO_ROOT}/general/CCCommon.cmake)
include(${CC_REPO_ROOT}/general/general.cmake)
# DON'T include client/libcatchchallenger/lib.cmake — it pulls
# Api_protocol_loadchar.cpp + DatapackClientLoader.cpp which reference
# CommonDatapack symbols defined only in catchchallenger_general (not
# minimal). The test never exercises character-load or datapack
# parsing; pick exactly the .cpp files that hold the parsing logic
# the visibility-broadcast packets land in.

find_package(Threads REQUIRED)

add_executable(testmapvisibility
    main.cpp
    Stubs.cpp
    TestApiProtocol.cpp
    ${CC_REPO_ROOT}/server/base/MapManagement/MapVisibilityAlgorithm.cpp
    ${CC_REPO_ROOT}/server/base/MapServer.cpp
    # Real production Api_protocol code paths the test drives. When the
    # protocol or 0x6B/0x66/0x69/0x6C/0x65/0xE3 payload layout changes
    # in these files, the test inherits the new behaviour at the next
    # build — no test-side update needed (the user's brief).
    ${CC_REPO_ROOT}/client/libcatchchallenger/Api_protocol.cpp
    ${CC_REPO_ROOT}/client/libcatchchallenger/Api_protocol_message.cpp
    ${CC_REPO_ROOT}/client/libcatchchallenger/Api_protocol_query.cpp
    ${CC_REPO_ROOT}/client/libcatchchallenger/Api_protocol_reply.cpp
    ${CC_REPO_ROOT}/client/libcatchchallenger/Api_protocol_loadchar.cpp
    ${CC_REPO_ROOT}/client/libcatchchallenger/Map_client.cpp
    ${CC_REPO_ROOT}/client/libcatchchallenger/ClientStructures.cpp
    ${CC_REPO_ROOT}/client/libcatchchallenger/BlacklistPassword.cpp
)

target_include_directories(testmapvisibility PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CC_REPO_ROOT}
)

# CATCHCHALLENGER_TESTING enables (a) Stubs.hpp injection at the top of
# the production server-side .cpp files we compile, AND (b) the
# [BRANCH] trace prints inside those .cpp files that
# testingmapmanagement.py harvests from stdout. Production never sets
# the flag — branches are gone.
#
# CATCHCHALLENGER_HARDENED matches the test/all.sh contract: every
# testing*.py binary compiles with HARDENED ON.
#
# CATCHCHALLENGER_NOXML keeps tinyxml2 out of MapServer.cpp's
# parseUnknownBotStep — the test doesn't need XML parsing.
target_compile_definitions(testmapvisibility PRIVATE
    CATCHCHALLENGER_TESTING
    CATCHCHALLENGER_HARDENED
    CATCHCHALLENGER_CLIENT
    CATCHCHALLENGER_DB_FILE
)

target_compile_options(testmapvisibility PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    # Several production client/general .cpp files use memcpy/memset/
    # strcmp without #include <cstring>; they get it transitively in
    # a normal build via the heavy server-side chain we exclude here.
    # Force-include cstring so the test build doesn't fail because of
    # missing transitive includes.
    -include cstring
)

target_link_libraries(testmapvisibility PRIVATE
    catchchallenger_common_flags
    catchchallenger_general
    Threads::Threads
    ZLIB::ZLIB
)

if(EXTERNALLIBZSTD)
    target_link_libraries(testmapvisibility PRIVATE ${ZSTD_LIBRARY})
endif()

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(testmapvisibility PRIVATE -fuse-ld=mold)
endif()
