# test/testingclientlogic/CMakeLists.txt — testclientlogic binary.
#
# Client-side game rules in client/libcatchchallenger/Api_protocol.cpp
# (quest requirements, beaten bots), driven through the concrete
# TestApiProtocol of test/testingmapmanagement/, plus the Qt-core logic shared
# by every client in client/libqtcatchchallenger/. No network, no event loop,
# no datapack. Run by test/testingclientlogic.py.

cmake_minimum_required(VERSION 3.20)
project(testclientlogic LANGUAGES C CXX)

set(CC_REPO_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)

include(${CC_REPO_ROOT}/general/CCCommon.cmake)
find_package(Qt6 COMPONENTS Core REQUIRED)
include(${CC_REPO_ROOT}/general/general.cmake)
include(${CC_REPO_ROOT}/client/libcatchchallenger/lib.cmake)

# the Qt-core-only part of libqtcatchchallenger: the whole static lib needs Gui/Network/...
add_executable(testclientlogic
    main.cpp
    ${CC_REPO_ROOT}/test/testingmapmanagement/TestApiProtocol.cpp
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/ChatParsing.cpp
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/ChatInputGuard.cpp
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/CliClientOptions.cpp
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/Language.cpp
)
set_target_properties(testclientlogic PROPERTIES AUTOMOC ON)

target_include_directories(testclientlogic PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CC_REPO_ROOT}
)

# HARDENED matches the test/all.sh contract (every testing*.py binary).
target_compile_definitions(testclientlogic PRIVATE
    CATCHCHALLENGER_HARDENED
)

target_link_libraries(testclientlogic PRIVATE
    catchchallenger_common_flags
    catchchallenger_client_lib
    catchchallenger_general
    Qt6::Core
)

if(CC_LINKER)
    target_link_options(testclientlogic PRIVATE -fuse-ld=${CC_LINKER})
endif()
