QT       += sql

TEMPLATE = app

SOURCES += \
    $$PWD/base/GlobalServerData.cpp \
    $$PWD/base/SqlFunction.cpp \
    $$PWD/base/BaseServer.cpp \
    $$PWD/base/LocalClientHandler.cpp \
    $$PWD/base/LocalClientHandlerWithoutSender.cpp \
    $$PWD/base/LocalClientHandlerQuest.cpp \
    $$PWD/base/LocalClientHandlerTrade.cpp \
    $$PWD/base/ClientLocalBroadcast.cpp \
    $$PWD/base/EventThreader.cpp \
    $$PWD/base/Client.cpp \
    $$PWD/base/ClientStaticVar.cpp \
    $$PWD/base/ClientHeavyLoad.cpp \
    $$PWD/base/ClientHeavyLoadSelectChar.cpp \
    $$PWD/base/ClientNetworkWrite.cpp \
    $$PWD/base/ClientNetworkRead.cpp \
    $$PWD/base/ClientNetworkReadWithoutSender.cpp \
    $$PWD/base/ClientBroadCast.cpp \
    $$PWD/base/PlayerUpdater.cpp \
    $$PWD/base/MapServer.cpp \
    $$PWD/base/BroadCastWithoutSender.cpp \
    $$PWD/base/ClientMapManagement/ClientMapManagement.cpp \
    $$PWD/base/ClientMapManagement/MapVisibilityAlgorithm_WithoutSender.cpp \
    $$PWD/base/ClientMapManagement/MapVisibilityAlgorithm_None.cpp \
    $$PWD/base/ClientMapManagement/MapVisibilityAlgorithm_Simple_StoreOnSender.cpp \
    $$PWD/base/ClientMapManagement/MapVisibilityAlgorithm_WithBorder_StoreOnSender.cpp \
    $$PWD/base/ClientMapManagement/Map_server_MapVisibility_Simple_StoreOnSender.cpp \
    $$PWD/base/ClientMapManagement/Map_server_MapVisibility_WithBorder_StoreOnSender.cpp \
    $$PWD/base/ClientMapManagement/MapBasicMove.cpp \
    $$PWD/crafting/BaseServerCrafting.cpp \
    $$PWD/crafting/ClientLocalBroadcastCrafting.cpp \
    $$PWD/crafting/LocalClientHandlerCrafting.cpp \
    $$PWD/fight/LocalClientHandlerFight.cpp \
    $$PWD/fight/BaseServerFight.cpp \
    $$PWD/crafting/ClientHeavyLoadCrafting.cpp \
    $$PWD/fight/ClientHeavyLoadFight.cpp

HEADERS += \
    $$PWD/base/GlobalServerData.h \
    $$PWD/base/SqlFunction.h \
    $$PWD/base/BaseServer.h \
    $$PWD/base/LocalClientHandlerWithoutSender.h \
    $$PWD/VariableServer.h \
    $$PWD/base/ServerStructures.h \
    $$PWD/base/EventThreader.h \
    $$PWD/base/Client.h \
    $$PWD/base/ClientNetworkReadWithoutSender.h \
    $$PWD/base/BroadCastWithoutSender.h \
    $$PWD/base/PlayerUpdater.h \
    $$PWD/base/MapServer.h \
    $$PWD/base/ClientMapManagement/ClientMapManagement.h \
    $$PWD/base/ClientMapManagement/MapVisibilityAlgorithm_WithoutSender.h \
    $$PWD/base/ClientMapManagement/MapVisibilityAlgorithm_None.h \
    $$PWD/base/ClientMapManagement/MapVisibilityAlgorithm_Simple_StoreOnSender.h \
    $$PWD/base/ClientMapManagement/MapVisibilityAlgorithm_WithBorder_StoreOnSender.h \
    $$PWD/base/ClientMapManagement/Map_server_MapVisibility_Simple_StoreOnSender.h \
    $$PWD/base/ClientMapManagement/Map_server_MapVisibility_WithBorder_StoreOnSender.h \
    $$PWD/base/ClientMapManagement/MapBasicMove.h \
    $$PWD/crafting/MapServerCrafting.h \
    $$PWD/base/DatabaseBase.h

RESOURCES += \
    $$PWD/all-server-resources.qrc

win32:RC_FILE += $$PWD/resources-windows.rc
