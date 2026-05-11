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
    EventLoopClientLoginSlave.cpp
    EventLoopServerLoginSlave.cpp
    EventLoopClientLoginSlaveStaticVar.cpp
    EventLoopClientLoginSlaveProtocolParsing.cpp
    EventLoopClientLoginSlaveWrite.cpp
    LinkToGameServerStaticVar.cpp
    LinkToGameServerProtocolParsing.cpp
    LinkToGameServer.cpp
    TimerDdos.cpp
    DatapackDownloaderBase.cpp
    DatapackDownloader_sub.cpp
    DatapackDownloader_main.cpp
    DatapackDownloaderMainSub.cpp
    EventLoopClientLoginSlaveDatapack.cpp
    FacilityLibGateway.cpp
    main-unix-gateway.cpp
    # unix/ shared
    ../cli/EventLoop.cpp
    ../cli/EventLoopGenericServer.cpp
    ../cli/EventLoopClient.cpp
    ../cli/SocketUtil.cpp
    ../cli/EventLoopClientToServer.cpp
    ../cli/EventLoopTimer.cpp
    # tiny client/libcatchchallenger reuse
    ../../client/libcatchchallenger/DatapackChecksum.cpp
    ../../client/libcatchchallenger/TarDecode.cpp
    # XML settings only
    ../base/TinyXMLSettings.cpp
)

add_executable(catchchallenger-gateway ${_gateway_sources})

target_include_directories(catchchallenger-gateway PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../unix
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

# io_uring is MANDATORY for this binary (per server/CLAUDE.md).
if(NOT LIBURING_LIBRARY)
    find_library(LIBURING_LIBRARY NAMES uring)
    find_path(LIBURING_INCLUDE_DIR NAMES liburing.h)
endif()
if(NOT LIBURING_LIBRARY)
    message(FATAL_ERROR
        "catchchallenger-gateway requires liburing (mandatory io_uring backend).")
endif()
target_compile_definitions(catchchallenger-gateway PRIVATE CATCHCHALLENGER_IO_URING)
target_include_directories(catchchallenger-gateway PRIVATE ${LIBURING_INCLUDE_DIR})
target_link_libraries(catchchallenger-gateway PRIVATE ${LIBURING_LIBRARY})

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(catchchallenger-gateway PRIVATE -fuse-ld=mold)
endif()
