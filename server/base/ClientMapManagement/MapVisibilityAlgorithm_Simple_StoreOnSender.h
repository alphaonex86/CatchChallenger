#ifndef CATCHCHALLENGER_MAPVISIBILITYALGORITHM_SIMPLE_STOREONSENDER_H
#define CATCHCHALLENGER_MAPVISIBILITYALGORITHM_SIMPLE_STOREONSENDER_H

#include "../Client.h"
#include "Map_server_MapVisibility_Simple_StoreOnSender.h"
#include "../../../general/base/CommonMap.h"

#include <QList>

namespace CatchChallenger {
class MapVisibilityAlgorithm_Simple_StoreOnSender : public Client
{
public:
    explicit MapVisibilityAlgorithm_Simple_StoreOnSender(ConnectedSocket *socket);
    virtual ~MapVisibilityAlgorithm_Simple_StoreOnSender();
    void reinsertAllClient();
    //drop all clients
    void dropAllClients();
    void purgeBuffer();
protected:
    //add clients linked
    void insertClient();
    void moveClient(const quint8 &previousMovedUnit,const Direction &direction);
    void removeClient();
    void mapVisiblity_unloadFromTheMap();
    void reinsertClientForOthersOnSameMap();
private:
    static MapVisibilityAlgorithm_Simple_StoreOnSender *current_client;//static to drop down the memory
    //map load/unload and change
    void			loadOnTheMap();
    void			unloadFromTheMap();

    //map move
    bool singleMove(const Direction &direction);

    static bool mapHaveChanged;
    static CommonMap*			old_map;
    static CommonMap*			new_map;

    #ifdef CATCHCHALLENGER_SERVER_MAP_DROP_BLOCKED_MOVE
    quint8 previousMovedUnitBlocked;
    #endif

    //temp variable to move on the map
    static map_management_movement moveClient_tempMov;

public:
    // stuff to send
    bool                                to_send_insert;
    bool                    			to_send_reinsert;
    QList<map_management_movement>      to_send_move;
public:
    //map slots, transmited by the current ClientNetworkRead
    void put_on_the_map(CommonMap *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
    bool moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction);
    void teleportValidatedTo(CommonMap *map,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation);
private:
    void extraStop();
};
}

#endif
