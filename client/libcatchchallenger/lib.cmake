# client/libcatchchallenger/lib.cmake — mirrors lib.pri + libheader.pri.
#
# A binary's CMakeLists.txt include()s this file to pull in the
# libcatchchallenger sources as an INTERFACE lib:
#   include(${CMAKE_CURRENT_LIST_DIR}/<rel>/client/libcatchchallenger/lib.cmake)
# then links catchchallenger_client_lib.

if(NOT TARGET catchchallenger_client_lib)
    add_library(catchchallenger_client_lib INTERFACE)

    target_sources(catchchallenger_client_lib INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/BlacklistPassword.cpp
        ${CMAKE_CURRENT_LIST_DIR}/Api_protocol.cpp
        ${CMAKE_CURRENT_LIST_DIR}/Api_protocol_loadchar.cpp
        ${CMAKE_CURRENT_LIST_DIR}/Api_protocol_message.cpp
        ${CMAKE_CURRENT_LIST_DIR}/Api_protocol_query.cpp
        ${CMAKE_CURRENT_LIST_DIR}/Api_protocol_reply.cpp
        ${CMAKE_CURRENT_LIST_DIR}/Map_client.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ClientStructures.cpp
        ${CMAKE_CURRENT_LIST_DIR}/DatapackClientLoader.cpp
        ${CMAKE_CURRENT_LIST_DIR}/DatapackChecksum.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ZstdDecode.cpp
        ${CMAKE_CURRENT_LIST_DIR}/TarDecode.cpp
    )

    target_include_directories(catchchallenger_client_lib INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
    )

    target_compile_definitions(catchchallenger_client_lib INTERFACE
        CATCHCHALLENGER_CLIENT
        CATCHCHALLENGERLIB
    )
endif()
