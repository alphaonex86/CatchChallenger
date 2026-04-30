# stats — tiny stats collector that connects to the login server over
# unix socket. No Qt; reuses a small subset of server/epoll + general/.

add_executable(stats
    main.cpp
    LinkToLogin.cpp
    LinkToLoginProtocolParsing.cpp
    LinkToLoginStaticVar.cpp
    EpollServerStats.cpp
    ${CMAKE_SOURCE_DIR}/server/epoll/Epoll.cpp
    ${CMAKE_SOURCE_DIR}/server/epoll/EpollSocket.cpp
    ${CMAKE_SOURCE_DIR}/server/epoll/EpollClient.cpp
    ${CMAKE_SOURCE_DIR}/server/epoll/EpollUnixSocketServer.cpp
    ${CMAKE_SOURCE_DIR}/server/base/TinyXMLSettings.cpp
)

target_compile_definitions(stats PRIVATE
    CATCHCHALLENGER_SERVER
    CATCHCHALLENGER_SERVER_NO_COMPRESSION
    SERVERNOBUFFER
    CATCHCHALLENGER_CLASS_STATS
    CATCHCHALLENGER_NO_WEBSOCKET
)

target_include_directories(stats PRIVATE
    ${CMAKE_SOURCE_DIR}/server/epoll
    ${CMAKE_SOURCE_DIR}/server/base
)

target_link_libraries(stats PRIVATE
    catchchallenger_common_flags
    catchchallenger_general_minimal
)

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(stats PRIVATE -fuse-ld=mold)
endif()
