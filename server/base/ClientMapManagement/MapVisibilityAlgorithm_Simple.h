#ifndef CATCHCHALLENGER_MAPVISIBILITYALGORITHM_SIMPLE_H
#define CATCHCHALLENGER_MAPVISIBILITYALGORITHM_SIMPLE_H

#include "ClientMapManagement.h"
#include "../MapServer.h"
#include "../../../general/base/Map.h"

namespace CatchChallenger {
class MapVisibilityAlgorithm_Simple : public ClientMapManagement
{
public:
    explicit MapVisibilityAlgorithm_Simple();
    virtual ~MapVisibilityAlgorithm_Simple();
    void reinsertAllClient();
    //drop all clients
    virtual void dropAllClients();
protected:
    //add clients linked
    void insertClient();
    void moveClient(const quint8 &previousMovedUnit,const Direction &direction);
    void removeClient();
    void mapVisiblity_unloadFromTheMap();
    void reinsertClientForOthers();
private:
    static int index;
    static int loop_size;
    static MapVisibilityAlgorithm_Simple *current_client;//static to drop down the memory
    //overwrite
    //remove the move/remove
    void moveAnotherClientWithMap(const SIMPLIFIED_PLAYER_ID_TYPE &player_id,MapVisibilityAlgorithm_Simple *the_another_player,const quint8 &movedUnit,const Direction &direction);
    #if defined(CATCHCHALLENGER_SERVER_VISIBILITY_CLEAR) && defined(CATCHCHALLENGER_SERVER_MAP_DROP_OVER_MOVE)
    QHash<SIMPLIFIED_PLAYER_ID_TYPE, MapVisibilityAlgorithm_Simple *>			to_send_over_move;
    void send_reinsert();
    #endif
    //for the purge buffer
    void send_insert();
    void send_move();
    void send_remove();
    #ifdef CATCHCHALLENGER_SERVER_VISIBILITY_CLEAR
    void insertAnotherClient(const SIMPLIFIED_PLAYER_ID_TYPE &player_id,MapVisibilityAlgorithm_Simple *the_another_player);
    void removeAnotherClient(const SIMPLIFIED_PLAYER_ID_TYPE &player_id);
    #endif
    //map load/unload and change
    virtual void			loadOnTheMap();
    virtual void			unloadFromTheMap();

    //map move
    bool singleMove(const Direction &direction);

    //temp variable for purge buffer
    static QByteArray purgeBuffer_outputData;
    static int purgeBuffer_index;
    static int purgeBuffer_list_size;
    static int purgeBuffer_list_size_internal;
    static int purgeBuffer_indexMovement;
    static bool mapHaveChanged;
    static map_management_move purgeBuffer_move;
    static QHash<SIMPLIFIED_PLAYER_ID_TYPE, QList<map_management_movement> >::const_iterator i_move;
    static QHash<SIMPLIFIED_PLAYER_ID_TYPE, QList<map_management_movement> >::const_iterator i_move_end;
    static QHash<SIMPLIFIED_PLAYER_ID_TYPE, MapVisibilityAlgorithm_Simple *>::const_iterator i_insert;
    static QHash<SIMPLIFIED_PLAYER_ID_TYPE, MapVisibilityAlgorithm_Simple *>::const_iterator i_insert_end;
    static QSet<SIMPLIFIED_PLAYER_ID_TYPE>::const_iterator i_remove;
    static QSet<SIMPLIFIED_PLAYER_ID_TYPE>::const_iterator i_remove_end;
    static Map*			old_map;
    static Map*			new_map;

    #ifdef CATCHCHALLENGER_SERVER_MAP_DROP_BLOCKED_MOVE
    quint8 previousMovedUnitBlocked;
    #endif

    //temp variable to move on the map
    static map_management_movement moveClient_tempMov;

    // stuff to send
    QHash<SIMPLIFIED_PLAYER_ID_TYPE, MapVisibilityAlgorithm_Simple *>			to_send_insert;
    QHash<SIMPLIFIED_PLAYER_ID_TYPE, QList<map_management_movement> >	to_send_move;
    QSet<SIMPLIFIED_PLAYER_ID_TYPE>						to_send_remove;
public slots:
    virtual void purgeBuffer();
    //map slots, transmited by the current ClientNetworkRead
    virtual void put_on_the_map(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
    virtual bool moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction);
    virtual void teleportValidatedTo(Map *map,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation);
private slots:
    virtual void extraStop();
};
}

#endif
