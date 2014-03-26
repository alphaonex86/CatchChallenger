#ifndef CATCHCHALLENGER_MAP_VISIBILITY_SIMPLE_SERVER_H
#define CATCHCHALLENGER_MAP_VISIBILITY_SIMPLE_SERVER_H

#include "../../general/base/CommonMap.h"
#include "../crafting/MapServerCrafting.h"

#include <QSet>
#include <QList>
#include <QMultiHash>

namespace CatchChallenger {
class MapVisibilityAlgorithm_Simple;
class MapVisibilityAlgorithm_WithBorder;
class ClientLocalBroadcast;

class MapServer : public CommonMap, public MapServerCrafting
{
public:
    QList<ClientLocalBroadcast *> clientsForBroadcast;//manipulated by thread of ClientLocalBroadcast(), frequent remove/insert due to map change
    QHash<QPair<quint8,quint8>,Orientation> rescue;
};

class Map_server_MapVisibility_simple : public MapServer
{
public:
    QList<MapVisibilityAlgorithm_Simple *> clients;//manipulated by thread of ClientMapManagement()

    bool show;
};

class Map_server_MapVisibility_withBorder : public MapServer
{
public:
    QList<MapVisibilityAlgorithm_WithBorder *> clients;//manipulated by thread of ClientMapManagement()
    quint16 clientsOnBorder;

    bool showWithBorder;
    bool show;
};

}

#endif
