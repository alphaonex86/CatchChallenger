#ifndef CATCHCHALLENGER_MAP_SERVER_MAPVISIBILITY_SIMPLE_STOREONSENDER_H
#define CATCHCHALLENGER_MAP_SERVER_MAPVISIBILITY_SIMPLE_STOREONSENDER_H

#include "../MapServer.h"
#include "MapVisibilityAlgorithm_Simple_StoreOnSender.h"

#include <QSet>
#include <QList>

namespace CatchChallenger {
class MapVisibilityAlgorithm_Simple_StoreOnSender;

class Map_server_MapVisibility_Simple_StoreOnSender : public MapServer
{
public:
    Map_server_MapVisibility_Simple_StoreOnSender();
    void purgeBuffer();
    QList<MapVisibilityAlgorithm_Simple_StoreOnSender *> clients;//manipulated by thread of ClientMapManagement()

    //mostly less remove than don't remove
    QList<quint16> to_send_remove;
    bool show;
    bool to_send_insert;
    bool send_drop_all;
    bool send_reinsert_all;
};
}

#endif
