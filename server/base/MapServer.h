#ifndef CATCHCHALLENGER_MAP_VISIBILITY_SIMPLE_SERVERMAP_H
#define CATCHCHALLENGER_MAP_VISIBILITY_SIMPLE_SERVERMAP_H

#include "../../general/base/CommonMap.h"
#include "../crafting/MapServerCrafting.h"
#include "../VariableServer.h"

#include <QSet>
#include <QList>
#include <QMultiHash>

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
    QHash<QPair<quint8,quint8>,Orientation> rescue;
    int localChatDrop[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE];
    int localChatDropTotalCache;
    int localChatDropNewValue;
    int reverse_db_id;

    //at last to improve the other variable cache
    QList<Client *> clientsForBroadcast;//frequent remove/insert due to map change
    struct ItemOnMap
    {
        quint8 indexOfOnMap;
        quint16 item;
        quint16 pointOnMapDbCode;
        bool infinite;
    };
    QHash<QPair<quint8,quint8>,ItemOnMap> itemsOnMap;//first is x,y, second is db code, item
    QHash<QPair<quint8,quint8>,quint32> dictionary_pointOnMap_internal_to_database;
};

class Map_server_MapVisibility_Simple_StoreOnReceiver : public MapServer
{
public:
    QList<MapVisibilityAlgorithm_Simple_StoreOnReceiver *> clients;//manipulated by thread of ClientMapManagement()

    bool show;
};
}

#endif
