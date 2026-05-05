# bot-test-connect-to-gameserver — load-test client. Self-contained,
# one binary. Qt6 Core+Network only; no GUI/widgets.

cmake_minimum_required(VERSION 3.20)
project(bot_test_connect LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

find_package(Qt6 COMPONENTS Core Network REQUIRED)

include(${CC_REPO_ROOT}/general/general.cmake)
include(${CC_REPO_ROOT}/client/libcatchchallenger/lib.cmake)

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
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/Api_client_real_base.cpp
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/Api_client_real_main.cpp
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/Api_client_real_sub.cpp
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/Api_client_real.cpp
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/Api_client_virtual.cpp
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/Api_protocol_Qt.cpp
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/QZstdDecodeThread.cpp
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/QtDatapackChecksum.cpp
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/ConnectedSocket.cpp
)

target_compile_definitions(bot-test-connect-to-gameserver PRIVATE
    CATCHCHALLENGER_BOT_TESTCONNECT
    CATCHCHALLENGER_BOT
    CATCHCHALLENGER_NO_WEBSOCKET
    CATCHCHALLENGER_NOAUDIO
)

target_include_directories(bot-test-connect-to-gameserver PRIVATE
    ${CC_REPO_ROOT}/client/libqtcatchchallenger
    ${CC_REPO_ROOT}/tools/libbot
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
