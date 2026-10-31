# test/testingclientlogic/CMakeLists.txt — testclientlogic binary.
#
# Client-side game rules in client/libcatchchallenger/Api_protocol.cpp
# (quest requirements, beaten bots), driven through the concrete
# TestApiProtocol of test/testingmapmanagement/. Pure RAM: no network, no
# event loop, no datapack. Run by test/testingclientlogic.py.

cmake_minimum_required(VERSION 3.20)
project(testclientlogic LANGUAGES C CXX)

set(CC_REPO_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)

include(${CC_REPO_ROOT}/general/CCCommon.cmake)
include(${CC_REPO_ROOT}/general/general.cmake)

add_executable(testclientlogic
    main.cpp
    ${CC_REPO_ROOT}/test/testingmapmanagement/TestApiProtocol.cpp
    ${CC_REPO_ROOT}/client/libcatchchallenger/Api_protocol.cpp
    ${CC_REPO_ROOT}/client/libcatchchallenger/Api_protocol_message.cpp
    ${CC_REPO_ROOT}/client/libcatchchallenger/Api_protocol_query.cpp
    ${CC_REPO_ROOT}/client/libcatchchallenger/Api_protocol_reply.cpp
    ${CC_REPO_ROOT}/client/libcatchchallenger/Api_protocol_loadchar.cpp
    ${CC_REPO_ROOT}/client/libcatchchallenger/Map_client.cpp
    ${CC_REPO_ROOT}/client/libcatchchallenger/ClientStructures.cpp
    ${CC_REPO_ROOT}/client/libcatchchallenger/BlacklistPassword.cpp
)

target_include_directories(testclientlogic PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CC_REPO_ROOT}
)

# HARDENED matches the test/all.sh contract (every testing*.py binary).
target_compile_definitions(testclientlogic PRIVATE
    CATCHCHALLENGER_HARDENED
    CATCHCHALLENGER_CLIENT
)

target_link_libraries(testclientlogic PRIVATE
    catchchallenger_common_flags
    catchchallenger_general
)

if(CC_LINKER)
    target_link_options(testclientlogic PRIVATE -fuse-ld=${CC_LINKER})
endif()
