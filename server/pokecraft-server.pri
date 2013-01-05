QT       += sql

TEMPLATE = app

SOURCES += \
    $$PWD/base/GlobalData.cpp \
    $$PWD/base/SqlFunction.cpp \
    $$PWD/base/BaseServer.cpp \
    $$PWD/base/LocalClientHandler.cpp \
    $$PWD/base/ClientLocalBroadcast.cpp \
    $$PWD/base/EventThreader.cpp \
    $$PWD/base/Client.cpp \
    $$PWD/base/ClientHeavyLoad.cpp \
    $$PWD/base/ClientNetworkWrite.cpp \
    $$PWD/base/ClientNetworkRead.cpp \
    $$PWD/base/ClientBroadCast.cpp \
    $$PWD/base/PlayerUpdater.cpp \
    $$PWD/base/BroadCastWithoutSender.cpp \
    $$PWD/base/Bot/FakeBot.cpp \
    $$PWD/base/ClientMapManagement/ClientMapManagement.cpp \
    $$PWD/base/ClientMapManagement/MapVisibilityAlgorithm_Simple.cpp \
    $$PWD/base/ClientMapManagement/MapBasicMove.cpp \
    $$PWD/base/ClientMapManagement/MapVisibilityAlgorithm_None.cpp \
    $$PWD/crafting/BaseServerCrafting.cpp \
    $$PWD/crafting/ClientLocalBroadcastCrafting.cpp \
    $$PWD/crafting/LocalClientHandlerCrafting.cpp \
    $$PWD/fight/LocalClientHandlerFight.cpp \
    $$PWD/fight/BaseServerFight.cpp \
    $$PWD/crafting/ClientHeavyLoadCrafting.cpp \
    $$PWD/fight/ClientHeavyLoadFight.cpp
HEADERS += \
    $$PWD/base/GlobalData.h \
    $$PWD/base/SqlFunction.h \
    $$PWD/base/BaseServer.h \
    $$PWD/base/LocalClientHandler.h \
    $$PWD/base/ClientLocalBroadcast.h \
    $$PWD/VariableServer.h \
    $$PWD/base/ServerStructures.h \
    $$PWD/base/EventThreader.h \
    $$PWD/base/Client.h \
    $$PWD/base/ClientHeavyLoad.h \
    $$PWD/base/ClientNetworkWrite.h \
    $$PWD/base/ClientNetworkRead.h \
    $$PWD/base/ClientBroadCast.h \
    $$PWD/base/BroadCastWithoutSender.h \
    $$PWD/base/PlayerUpdater.h \
    $$PWD/base/MapServer.h \
    $$PWD/base/Bot/FakeBot.h \
    $$PWD/base/ClientMapManagement/ClientMapManagement.h \
    $$PWD/base/ClientMapManagement/MapVisibilityAlgorithm_Simple.h \
    $$PWD/base/ClientMapManagement/MapBasicMove.h \
    $$PWD/base/ClientMapManagement/MapVisibilityAlgorithm_None.h \
    $$PWD/crafting/BaseServerCrafting.h \
    $$PWD/crafting/MapServerCrafting.h \
    $$PWD/fight/ServerStructuresFight.h \
    $$PWD/fight/BaseServerFight.h

RESOURCES += \
    $$PWD/all-server-resources.qrc \
    $$PWD/resources-server.qrc \

win32:RC_FILE += $$PWD/resources-windows.rc
win32:RESOURCES += $$PWD/base/resources/resources-windows-qt-plugin.qrc
