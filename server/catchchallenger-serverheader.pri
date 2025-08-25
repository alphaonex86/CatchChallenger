DEFINES += TILED_ZLIB
LIBS += -lz

HEADERS += \
    $$PWD/base/MapManagement/ClientWithMap.hpp \
    $$PWD/base/VariableServer.hpp \
    $$PWD/base/NormalServerGlobal.hpp \
    $$PWD/base/GlobalServerData.hpp \
    $$PWD/base/PlayerUpdaterBase.hpp \
    $$PWD/base/SqlFunction.hpp \
    $$PWD/base/BaseServer/BaseServer.hpp \
    $$PWD/base/BaseServerMasterLoadDictionary.hpp \
    $$PWD/base/BaseServerMasterSendDatapack.hpp \
    $$PWD/base/LocalClientHandler/LocalClientHandlerWithoutSender.hpp \
    $$PWD/base/ServerStructures.hpp \
    $$PWD/base/Client.hpp \
    $$PWD/base/ClientNetworkReadWithoutSender.hpp \
    $$PWD/base/BroadCastWithoutSender.hpp \
    $$PWD/base/MapServer.hpp \
    $$PWD/base/MapManagement/ClientMapManagement.hpp \
    $$PWD/base/MapManagement/MapVisibilityAlgorithm_WithoutSender.hpp \
    $$PWD/base/MapManagement/MapVisibilityAlgorithm_None.hpp \
    $$PWD/base/MapManagement/MapVisibilityAlgorithm_WithBorder_StoreOnSender.hpp \
    $$PWD/base/MapManagement/Map_server_MapVisibility_Simple_StoreOnSender.hpp \
    $$PWD/base/MapManagement/Map_server_MapVisibility_WithBorder_StoreOnSender.hpp \
    $$PWD/base/MapManagement/MapBasicMove.hpp \
    $$PWD/base/DictionaryLogin.hpp \
    $$PWD/base/DdosBuffer.hpp \
    $$PWD/base/DictionaryServer.hpp \
    $$PWD/base/PreparedDBQuery.hpp \
    $$PWD/base/DatabaseBase.hpp \
    $$PWD/base/BaseServerLogin.hpp \
    $$PWD/base/TimeRangeEventScanBase.hpp \
    $$PWD/base/TinyXMLSettings.hpp \
    $$PWD/base/DatabaseFunction.hpp \
    $$PWD/base/StringWithReplacement.hpp \
    $$PWD/base/GameServerVariables.hpp \
    $$PWD/base/PreparedStatementUnit.hpp \
    $$PWD/crafting/MapServerCrafting.hpp
