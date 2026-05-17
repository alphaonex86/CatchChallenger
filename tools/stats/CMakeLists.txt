# stats — tiny stats collector that connects to the login server over
# unix socket. Self-contained, one binary. No Qt; reuses a small subset
# of server/cli + general/.

cmake_minimum_required(VERSION 3.20)
project(stats LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

include(${CC_REPO_ROOT}/general/general.cmake)

add_executable(stats
    main.cpp
    LinkToLogin.cpp
    LinkToLoginProtocolParsing.cpp
    LinkToLoginStaticVar.cpp
    UnixServerStats.cpp
    ${CC_REPO_ROOT}/server/cli/EventLoop.cpp
    ${CC_REPO_ROOT}/server/cli/SocketUtil.cpp
    ${CC_REPO_ROOT}/server/cli/EventLoopClient.cpp
    ${CC_REPO_ROOT}/server/cli/UnixSocketServer.cpp
    ${CC_REPO_ROOT}/server/base/TinyXMLSettings.cpp
)

target_compile_definitions(stats PRIVATE
    CATCHCHALLENGER_SERVER
    CATCHCHALLENGER_SERVER_NO_COMPRESSION
    SERVERNOBUFFER
    CATCHCHALLENGER_CLASS_STATS
    CATCHCHALLENGER_NO_WEBSOCKET
)

target_include_directories(stats PRIVATE
    ${CC_REPO_ROOT}/server/cli
    ${CC_REPO_ROOT}/server/base
)

target_link_libraries(stats PRIVATE
    catchchallenger_common_flags
    catchchallenger_general_minimal
)

# Linker driver: use the test-link-verified accelerator that
# CCCommon.cmake picked for THIS target (mold on native ELF,
# lld/bfd where mold has no backend, e.g. MIPS). Never hard-code
# a specific linker here — CCCommon is the single source of truth.
if(CC_LINKER)
    target_link_options(stats PRIVATE -fuse-ld=${CC_LINKER})
endif()
