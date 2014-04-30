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
class MapVisibilityAlgorithm_WithBorder_StoreOnReceiver;
class MapVisibilityAlgorithm_Simple_StoreOnSender;
class MapVisibilityAlgorithm_WithBorder_StoreOnSender;
class ClientLocalBroadcast;

class MapServer : public CommonMap, public MapServerCrafting
{
public:
    MapServer();
    void doDDOSCompute();
    QList<ClientLocalBroadcast *> clientsForBroadcast;//manipulated by thread of ClientLocalBroadcast(), frequent remove/insert due to map change
    QHash<QPair<quint8,quint8>,Orientation> rescue;
    QList<int> localChatDrop;
    int localChatDropTotalCache;
    int localChatDropNewValue;
};

class Map_server_MapVisibility_Simple_StoreOnReceiver : public MapServer
{
public:
    QList<MapVisibilityAlgorithm_Simple_StoreOnReceiver *> clients;//manipulated by thread of ClientMapManagement()

    bool show;
};
class Map_server_MapVisibility_WithBorder_StoreOnReceiver : public MapServer
{
public:
    QList<MapVisibilityAlgorithm_WithBorder_StoreOnReceiver *> clients;//manipulated by thread of ClientMapManagement()
    quint16 clientsOnBorder;

    bool showWithBorder;
    bool show;
};
}

#endif
