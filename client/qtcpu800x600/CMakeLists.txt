# qtcpu800x600 — fixed 800x600 widget UI client.
# Mirrors qtcpu800x600.pro + base/client.pri + base/multi.pri +
# ultimate/specific.pri. Phase 3 scope: Linux native, MULTI mode,
# audio per-flag, single-player optional.
# Per project memory: this client is being phased out (Android already
# only ships qtopengl) — kept here for completeness on desktop builds.

find_package(Qt6 COMPONENTS Core Gui Network Widgets OpenGL Xml REQUIRED)

option(CATCHCHALLENGER_BUILD_QTCPU800X600_WEBSOCKETS
       "Link Qt6::WebSockets into qtcpu800x600" ON)
option(CATCHCHALLENGER_BUILD_QTCPU800X600_SINGLEPLAYER
       "Embed the in-process Qt server (CATCHCHALLENGER_DB_SQLITE)" OFF)
if(CATCHCHALLENGER_BUILD_QTCPU800X600_WEBSOCKETS)
    find_package(Qt6 COMPONENTS WebSockets REQUIRED)
endif()
if(CATCHCHALLENGER_BUILD_QTCPU800X600_SINGLEPLAYER)
    find_package(Qt6 COMPONENTS Sql REQUIRED)
endif()

set(_qtcpu_sources
    # base/client.pri (game UI windows + utilities)
    base/interface/BaseWindow.cpp
    base/interface/BaseWindowMap.cpp
    base/interface/BaseWindowBot.cpp
    base/interface/BaseWindowSelection.cpp
    base/interface/BaseWindowCharacter.cpp
    base/interface/BaseWindowCity.cpp
    base/interface/BaseWindowClan.cpp
    base/interface/BaseWindowFactory.cpp
    base/interface/BaseWindowLoad.cpp
    base/interface/BaseWindowTrade.cpp
    base/interface/BaseWindowOptions.cpp
    base/interface/BaseWindowShop.cpp
    base/interface/BaseWindowWarehouse.cpp
    base/interface/ListEntryEnvolued.cpp
    base/interface/Chat.cpp
    base/interface/WithAnotherPlayer.cpp
    base/interface/GetPrice.cpp
    base/interface/NewProfile.cpp
    base/interface/NewGame.cpp
    crafting/interface/QmlInterface/CraftingAnimation.cpp
    crafting/interface/BaseWindowCrafting.cpp
    fight/interface/QmlInterface/QmlMonsterGeneralInformations.cpp
    fight/interface/QmlInterface/EvolutionControl.cpp
    fight/interface/BaseWindowFight.cpp
    fight/interface/BaseWindowFightNextAction.cpp
    base/QmlInterface/AnimationControl.cpp
    base/Options.cpp
    base/ChatParsing.cpp
    base/LanguagesSelect.cpp
    base/FacilityLibClient.cpp
    base/AutoArgs.cpp

    # base/multi.pri (multiplayer-only entrypoint pieces)
    base/SslCert.cpp

    # ultimate/specific.pri (window shell + add-server flow)
    ultimate/main.cpp
    ultimate/mainwindow.cpp
    ultimate/AddServer.cpp
    ultimate/AskKey.cpp
)

if(CATCHCHALLENGER_BUILD_QTCPU800X600_SINGLEPLAYER)
    list(APPEND _qtcpu_sources base/solo/SoloWindow.cpp)
endif()

set(_qtcpu_resources
    base/resources/client-resources.qrc
    base/resources/client-resources-multi.qrc
    crafting/resources/client-resources-plant.qrc
    fight/resources/client-resources-fight.qrc
)

add_executable(catchchallenger800x600 ${_qtcpu_sources} ${_qtcpu_resources})

set_target_properties(catchchallenger800x600 PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# Make AUTOUIC find the .ui files. They live in interface/ and
# top-level dirs for the various windows.
set_target_properties(catchchallenger800x600 PROPERTIES
    AUTOUIC_SEARCH_PATHS
        "${CMAKE_CURRENT_SOURCE_DIR}/base/interface;\
${CMAKE_CURRENT_SOURCE_DIR}/base;\
${CMAKE_CURRENT_SOURCE_DIR}/base/solo;\
${CMAKE_CURRENT_SOURCE_DIR}/crafting/interface;\
${CMAKE_CURRENT_SOURCE_DIR}/fight/interface;\
${CMAKE_CURRENT_SOURCE_DIR}/ultimate"
)

target_compile_definitions(catchchallenger800x600 PRIVATE
    CATCHCHALLENGER_CLIENT
    CATCHCHALLENGER_CLASS_QT
    CATCHCHALLENGER_CLASS_CLIENT
    CATCHCHALLENGER_MULTI
    CATCHCHALLENGER_DB_SQLITE
    CATCHCHALLENGER_VERSION_ULTIMATE
    PROTOCOLPARSINGDEBUG
)
if(CATCHCHALLENGER_BUILD_QTCPU800X600_SINGLEPLAYER)
    target_compile_definitions(catchchallenger800x600 PRIVATE CATCHCHALLENGER_SOLO)
endif()
if(NOT CATCHCHALLENGER_BUILD_QTCPU800X600_WEBSOCKETS)
    target_compile_definitions(catchchallenger800x600 PRIVATE CATCHCHALLENGER_NO_WEBSOCKET)
endif()

target_include_directories(catchchallenger800x600 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/base
)

target_link_libraries(catchchallenger800x600 PRIVATE
    catchchallenger_qt_lib
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::Xml
)
if(CATCHCHALLENGER_BUILD_QTCPU800X600_WEBSOCKETS)
    target_link_libraries(catchchallenger800x600 PRIVATE Qt6::WebSockets)
endif()
if(CATCHCHALLENGER_BUILD_QTCPU800X600_SINGLEPLAYER)
    target_link_libraries(catchchallenger800x600 PRIVATE
        catchchallenger_general
        catchchallenger_server_base
        catchchallenger_server_sql
        catchchallenger_server_qt
        Qt6::Sql
    )
endif()

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(catchchallenger800x600 PRIVATE -fuse-ld=mold)
endif()
