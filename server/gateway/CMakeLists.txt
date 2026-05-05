# server/gateway/CMakeLists.txt — catchchallenger-gateway binary.
# Self-contained, one binary. CATCHCHALLENGER_CLASS_GATEWAY +
# CATCHCHALLENGER_DB_BLACKHOLE. Links libcurl. Pulls a couple of
# client/libcatchchallenger sources directly (DatapackChecksum,
# TarDecode) since gateway re-uses the client-side datapack download
# code path.

cmake_minimum_required(VERSION 3.20)
project(catchchallenger_gateway LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

include(${CC_REPO_ROOT}/general/general.cmake)

find_package(Threads REQUIRED)
find_library(LIBCURL_LIBRARY NAMES curl REQUIRED)

set(_gateway_sources
    EpollClientLoginSlave.cpp
    EpollServerLoginSlave.cpp
    EpollClientLoginSlaveStaticVar.cpp
    EpollClientLoginSlaveProtocolParsing.cpp
    EpollClientLoginSlaveWrite.cpp
    LinkToGameServerStaticVar.cpp
    LinkToGameServerProtocolParsing.cpp
    LinkToGameServer.cpp
    TimerDdos.cpp
    DatapackDownloaderBase.cpp
    DatapackDownloader_sub.cpp
    DatapackDownloader_main.cpp
    DatapackDownloaderMainSub.cpp
    EpollClientLoginSlaveDatapack.cpp
    FacilityLibGateway.cpp
    main-epoll-gateway.cpp
    # epoll/ shared
    ../epoll/Epoll.cpp
    ../epoll/EpollGenericServer.cpp
    ../epoll/EpollClient.cpp
    ../epoll/EpollSocket.cpp
    ../epoll/EpollClientToServer.cpp
    ../epoll/EpollTimer.cpp
    # tiny client/libcatchchallenger reuse
    ../../client/libcatchchallenger/DatapackChecksum.cpp
    ../../client/libcatchchallenger/TarDecode.cpp
    # XML settings only
    ../base/TinyXMLSettings.cpp
)

add_executable(catchchallenger-gateway ${_gateway_sources})

target_include_directories(catchchallenger-gateway PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../epoll
    ${CMAKE_CURRENT_SOURCE_DIR}/../base
    ${CMAKE_CURRENT_SOURCE_DIR}/../../client/libcatchchallenger
)

target_link_libraries(catchchallenger-gateway PRIVATE
    catchchallenger_common_flags
    catchchallenger_general_minimal
    Threads::Threads
    ZLIB::ZLIB
    ${LIBCURL_LIBRARY}
)

target_compile_definitions(catchchallenger-gateway PRIVATE
    CATCHCHALLENGER_SERVER
    QT_NO_EMIT
    CATCHCHALLENGER_CLASS_GATEWAY
    CATCHCHALLENGER_DB_BLACKHOLE
    DEBUG_PROTOCOLPARSING_RAW_NETWORK
    PROTOCOLPARSINGINPUTOUTPUTDEBUG
)

if(EXTERNALLIBZSTD)
    target_link_libraries(catchchallenger-gateway PRIVATE ${ZSTD_LIBRARY})
endif()

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(catchchallenger-gateway PRIVATE -fuse-ld=mold)
endif()
