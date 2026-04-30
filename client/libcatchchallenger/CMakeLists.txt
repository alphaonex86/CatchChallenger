# libcatchchallenger — protocol + datapack + checksum, no Qt.
# This is the "core" client lib; libqtcatchchallenger sits on top.
# Mirrors client/libcatchchallenger/lib.pri + libheader.pri.

add_library(catchchallenger_client_lib INTERFACE)

target_sources(catchchallenger_client_lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/BlacklistPassword.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Api_protocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Api_protocol_loadchar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Api_protocol_message.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Api_protocol_query.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Api_protocol_reply.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Map_client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientStructures.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DatapackClientLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DatapackChecksum.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ZstdDecode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TarDecode.cpp
)

target_include_directories(catchchallenger_client_lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(catchchallenger_client_lib INTERFACE
    CATCHCHALLENGER_CLIENT
    CATCHCHALLENGERLIB
)
