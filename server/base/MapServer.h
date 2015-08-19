#ifndef CATCHCHALLENGER_MAP_VISIBILITY_SIMPLE_SERVERMAP_H
#define CATCHCHALLENGER_MAP_VISIBILITY_SIMPLE_SERVERMAP_H

#include "../../general/base/CommonMap.h"
#include "../../general/base/GeneralStructures.h"
#include "../crafting/MapServerCrafting.h"
#include "../VariableServer.h"

#include <vector>

namespace CatchChallenger {
class MapVisibilityAlgorithm_Simple_StoreOnReceiver;
class MapVisibilityAlgorithm_Simple_StoreOnSender;
class MapVisibilityAlgorithm_WithBorder_StoreOnSender;
class Client;

class MapServer : public CommonMap, public MapServerCrafting
{
public:
    MapServer();
    void doDDOSLocalChat();
    std::unordered_map<std::pair<quint8,quint8>,Orientation,pairhash> rescue;
    int localChatDrop[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE];
    int localChatDropTotalCache;
    int localChatDropNewValue;
    int reverse_db_id;

    //at last to improve the other variable cache
    std::vector<Client *> clientsForBroadcast;//frequent remove/insert due to map change
    struct ItemOnMap
    {
        quint8 indexOfOnMap;
        quint16 item;
        quint16 pointOnMapDbCode;
        bool infinite;
    };
    std::unordered_map<std::pair<quint8,quint8>,ItemOnMap,pairhash> itemsOnMap;//first is x,y, second is db code, item
    std::unordered_map<std::pair<quint8,quint8>,quint32,pairhash> dictionary_pointOnMap_internal_to_database;
};

class Map_server_MapVisibility_Simple_StoreOnReceiver : public MapServer
{
public:
    std::vector<MapVisibilityAlgorithm_Simple_StoreOnReceiver *> clients;//manipulated by thread of ClientMapManagement()

    bool show;
};
}

#endif
