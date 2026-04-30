# bot-test-connect-to-gameserver — load-test client used by testing*.py.
# Mirrors bot-test-connect-to-gameserver.pro: Qt6 Core+Network only,
# no GUI/widgets, CATCHCHALLENGER_NO_WEBSOCKET, NOAUDIO. Reuses libcatchchallenger +
# parts of libqtcatchchallenger directly (the .pro hand-lists files).

find_package(Qt6 COMPONENTS Core Network REQUIRED)

add_executable(bot-test-connect-to-gameserver
    main.cpp
    GlobalController.cpp
    ../libbot/MultipleBotConnection.cpp
    ../libbot/MultipleBotConnectionImplForGui.cpp
    ../libbot/BotInterface.cpp
    ../libbot/simple/SimpleBotInterface.cpp
    ../libbot/simple/SimpleAction.cpp
    # libqtcatchchallenger sources hand-listed (the .pro doesn't pull
    # the whole libqt.pri — it picks just what's needed).
    ${CMAKE_SOURCE_DIR}/client/libqtcatchchallenger/Api_client_real_base.cpp
    ${CMAKE_SOURCE_DIR}/client/libqtcatchchallenger/Api_client_real_main.cpp
    ${CMAKE_SOURCE_DIR}/client/libqtcatchchallenger/Api_client_real_sub.cpp
    ${CMAKE_SOURCE_DIR}/client/libqtcatchchallenger/Api_client_real.cpp
    ${CMAKE_SOURCE_DIR}/client/libqtcatchchallenger/Api_client_virtual.cpp
    ${CMAKE_SOURCE_DIR}/client/libqtcatchchallenger/Api_protocol_Qt.cpp
    ${CMAKE_SOURCE_DIR}/client/libqtcatchchallenger/QZstdDecodeThread.cpp
    ${CMAKE_SOURCE_DIR}/client/libqtcatchchallenger/QtDatapackChecksum.cpp
    ${CMAKE_SOURCE_DIR}/client/libqtcatchchallenger/ConnectedSocket.cpp
)

target_compile_definitions(bot-test-connect-to-gameserver PRIVATE
    CATCHCHALLENGER_BOT_TESTCONNECT
    CATCHCHALLENGER_BOT
    CATCHCHALLENGER_NO_WEBSOCKET
    CATCHCHALLENGER_NOAUDIO
)

target_include_directories(bot-test-connect-to-gameserver PRIVATE
    ${CMAKE_SOURCE_DIR}/client/libqtcatchchallenger
    ${CMAKE_SOURCE_DIR}/tools/libbot
)

set_target_properties(bot-test-connect-to-gameserver PROPERTIES AUTOMOC ON)

target_link_libraries(bot-test-connect-to-gameserver PRIVATE
    catchchallenger_general
    catchchallenger_client_lib
    Qt6::Core
    Qt6::Network
)

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(bot-test-connect-to-gameserver PRIVATE -fuse-ld=mold)
endif()
