# bot-actions — Qt6 GUI bot debugger. Mirrors bot-actions.pro: pulls
# the full libqtcatchchallenger middleware + an Actions sub-engine
# from tools/libbot/actions/. CATCHCHALLENGER_NOAUDIO ON to match.

find_package(Qt6 COMPONENTS Core Gui Xml Network Sql Widgets WebSockets REQUIRED)

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
