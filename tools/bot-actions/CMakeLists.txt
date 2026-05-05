# bot-actions — Qt6 GUI bot debugger. Self-contained, one binary.
# Pulls libqtcatchchallenger (catchchallenger_qt_lib) which transitively
# brings in libcatchchallenger and Qt6.

cmake_minimum_required(VERSION 3.20)
project(bot_actions LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

find_package(Qt6 COMPONENTS Core Gui Xml Network Sql Widgets WebSockets REQUIRED)

include(${CC_REPO_ROOT}/general/general.cmake)
include(${CC_REPO_ROOT}/client/libcatchchallenger/lib.cmake)
include(${CC_REPO_ROOT}/client/libqtcatchchallenger/libqt.cmake)

add_executable(bot-actions
    main.cpp
    GenerateMapZone.cpp
    MainWindow.cpp
    BotTargetList.cpp
    BotTargetListGUI.cpp
    BotTargetListNear.cpp
    MapBrowse.cpp
    WaitScreen.cpp
    BotTargetListPlayerInfo.cpp
    BotTargetListNextPos.cpp
    SocialChat.cpp
    TargetFilter.cpp
    MultipleBotConnectionAction.cpp
    DatabaseBot.cpp
    # Qt resource: ships config.db that DatabaseBot::init() loads from
    # :/config.db before opening the SQLite handle. AUTORCC alone does
    # not discover .qrc files; they have to appear in sources.
    localr.qrc
    # tools/libbot common helpers
    ../libbot/MultipleBotConnection.cpp
    ../libbot/MultipleBotConnectionImplForGui.cpp
    ../libbot/BotInterface.cpp
    # tools/libbot/actions/ (per Actions.pri)
    ../libbot/actions/ActionsBotInterface.cpp
    ../libbot/actions/ActionsAction.cpp
    ../libbot/actions/ActionsMap.cpp
    ../libbot/actions/ActionsMapStep1.cpp
    ../libbot/actions/MapServerMini.cpp
    ../libbot/actions/ActionsBot.cpp
    ../libbot/actions/MapServerMiniStep1.cpp
    ../libbot/actions/MapServerMiniStep2.cpp
    ../libbot/actions/MapServerMiniNear.cpp
    ../libbot/actions/ActionsCheck.cpp
    ../libbot/actions/ActionsFight.cpp
    ../libbot/actions/ActionsQuests.cpp
    ../libbot/actions/ActionsAPIReturn.cpp
    # qtcpu800x600's ChatParsing reused (per .pro)
    ../../client/qtcpu800x600/base/ChatParsing.cpp
)

target_compile_definitions(bot-actions PRIVATE
    CATCHCHALLENGER_BOT_ACTIONS
    CATCHCHALLENGER_BOT
    CATCHCHALLENGER_ABORTIFERROR
    CATCHCHALLENGER_NOAUDIO
)

target_include_directories(bot-actions PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../libbot
    ${CMAKE_CURRENT_SOURCE_DIR}/../libbot/actions
    ${CMAKE_CURRENT_SOURCE_DIR}/../../client/qtcpu800x600/base
)

set_target_properties(bot-actions PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

target_link_libraries(bot-actions PRIVATE
    catchchallenger_qt_lib
    Qt6::Core
    Qt6::Gui
    Qt6::Xml
    Qt6::Network
    Qt6::Sql
    Qt6::Widgets
    Qt6::WebSockets
)

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(bot-actions PRIVATE -fuse-ld=mold)
endif()
