# stats — tiny stats collector that connects to the login server over
# unix socket. Self-contained, one binary. No Qt; reuses a small subset
# of server/epoll + general/.

cmake_minimum_required(VERSION 3.20)
project(stats LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

include(${CC_REPO_ROOT}/general/general.cmake)

add_executable(stats
    main.cpp
    LinkToLogin.cpp
    LinkToLoginProtocolParsing.cpp
    LinkToLoginStaticVar.cpp
    EpollServerStats.cpp
    ${CC_REPO_ROOT}/server/epoll/Epoll.cpp
    ${CC_REPO_ROOT}/server/epoll/EpollSocket.cpp
    ${CC_REPO_ROOT}/server/epoll/EpollClient.cpp
    ${CC_REPO_ROOT}/server/epoll/EpollUnixSocketServer.cpp
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
    ${CC_REPO_ROOT}/server/epoll
    ${CC_REPO_ROOT}/server/base
)

target_link_libraries(stats PRIVATE
    catchchallenger_common_flags
    catchchallenger_general_minimal
)

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(stats PRIVATE -fuse-ld=mold)
endif()
