# qtopengl — main responsive Qt6 OpenGL client (CatchChallenger UI).
# Mirrors qtopengl/catchchallenger-qtopengl.pro + client.pri.
# Phase 3 scope: Linux native, MULTI mode (NOSINGLEPLAYER), NOAUDIO.
# Single-player support pulls server/qt/ + Qt SQL — deferred.

find_package(Qt6 COMPONENTS Core Gui Network Widgets OpenGL OpenGLWidgets Xml REQUIRED)

option(CATCHCHALLENGER_BUILD_QTOPENGL_WEBSOCKETS
       "Link Qt6::WebSockets into qtopengl (matches default qmake build)"
       ON)
option(CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER
       "Embed the in-process Qt server (CATCHCHALLENGER_CLASS_QT + CATCHCHALLENGER_DB_SQLITE)"
       OFF)
if(CATCHCHALLENGER_BUILD_QTOPENGL_WEBSOCKETS)
    find_package(Qt6 COMPONENTS WebSockets REQUIRED)
endif()
if(CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER)
    find_package(Qt6 COMPONENTS Sql REQUIRED)
endif()

set(_qtopengl_sources
    # client.pri SOURCES (UI + game flow)
    CCGraphicsTextItem.cpp
    CCSliderH.cpp
    MultiItem.cpp
    ConnexionInfo.cpp
    CCScrollZone.cpp
    LineEdit.cpp
    ComboBox.cpp
    SpinBox.cpp
    above/AddOrEditServer.cpp
    above/OptionsDialog.cpp
    above/Login.cpp
    CheckBox.cpp
    foreground/Battle.cpp
    foreground/MainScreen.cpp
    foreground/Multi.cpp
    foreground/CharacterList.cpp
    foreground/SubServer.cpp
    foreground/QGraphicsTextItemOutline.cpp
    above/AddCharacter.cpp
    above/NewGame.cpp
    FacilityLibClient.cpp
    background/CCMap.cpp
    foreground/OverMap.cpp
    CustomText.cpp
    foreground/widgets/MapMonsterPreview.cpp
    ImagesStrechMiddle.cpp
    MyApplication.cpp
    above/DebugDialog.cpp
    foreground/OverMapLogic.cpp
    foreground/OverMapLogicTrade.cpp
    foreground/OverMapLogicShop.cpp
    foreground/OverMapLogicBot.cpp
    foreground/OverMapLogicFactory.cpp
    foreground/OverMapLogicClan.cpp
    above/inventory/Inventory.cpp
    above/inventory/Plant.cpp
    above/inventory/Crafting.cpp
    above/Animation.cpp
    above/Encyclopedia.cpp
    above/Shop.cpp
    above/Factory.cpp
    above/Trade.cpp
    above/Warehouse.cpp
    above/player/Player.cpp
    above/player/Reputations.cpp
    above/player/Quests.cpp
    above/player/FinishedQuests.cpp
    QGraphicsPixmapItemClick.cpp
    above/MonsterDetails.cpp
    GameLoader.cpp
    ChatParsing.cpp
    GameLoaderThread.cpp
    ProgressBarPixel.cpp
    main.cpp
    CCTitle.cpp
    ConnexionManager.cpp
    foreground/LoadingScreen.cpp
    CustomButton.cpp
    ScreenTransition.cpp
    background/CCBackground.cpp
    CCprogressbar.cpp
    ScreenInput.cpp
    AudioGL.cpp
)

set(_qtopengl_resources
    resources/client-resources.qrc
)

add_executable(catchchallenger ${_qtopengl_sources} ${_qtopengl_resources})

target_compile_definitions(catchchallenger PRIVATE
    CATCHCHALLENGER_CLIENT
    CATCHCHALLENGER_CLASS_QT
    CATCHCHALLENGER_CLASS_CLIENT
    CATCHCHALLENGER_MULTI
    CATCHCHALLENGER_DB_SQLITE
)
if(CATCHCHALLENGER_CACHE_HPS)
    target_compile_definitions(catchchallenger PRIVATE CATCHCHALLENGER_CACHE_HPS)
endif()
if(CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER)
    target_compile_definitions(catchchallenger PRIVATE CATCHCHALLENGER_SOLO)
endif()
if(CATCHCHALLENGER_NOAUDIO)
    target_compile_definitions(catchchallenger PRIVATE CATCHCHALLENGER_NOAUDIO)
endif()
if(NOT CATCHCHALLENGER_BUILD_QTOPENGL_WEBSOCKETS)
    target_compile_definitions(catchchallenger PRIVATE CATCHCHALLENGER_NO_WEBSOCKET)
endif()

set_target_properties(catchchallenger PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

target_link_libraries(catchchallenger PRIVATE
    catchchallenger_qt_lib
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Xml
)
if(CATCHCHALLENGER_BUILD_QTOPENGL_WEBSOCKETS)
    target_link_libraries(catchchallenger PRIVATE Qt6::WebSockets)
endif()
if(CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER)
    # In-process server: pulls the game-logic INTERFACE lib, the SQL
    # plumbing, and the Qt-based server I/O glue. CLASS_QT is set on
    # catchchallenger_server_qt INTERFACE; the exec also gets it via
    # the public define above.
    target_link_libraries(catchchallenger PRIVATE
        catchchallenger_general
        catchchallenger_server_base
        catchchallenger_server_sql
        catchchallenger_server_qt
        Qt6::Sql
    )
endif()

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(catchchallenger PRIVATE -fuse-ld=mold)
endif()
