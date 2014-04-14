#ifndef CATCHCHALLENGER_MAPVISIBILITYALGORITHM_SIMPLE_H
#define CATCHCHALLENGER_MAPVISIBILITYALGORITHM_SIMPLE_H

#include "ClientMapManagement.h"
#include "../MapServer.h"
#include "../../../general/base/CommonMap.h"

namespace CatchChallenger {
class MapVisibilityAlgorithm_Simple_StoreOnReceiver : public ClientMapManagement
{
    Q_OBJECT
public:
    explicit MapVisibilityAlgorithm_Simple_StoreOnReceiver();
    virtual ~MapVisibilityAlgorithm_Simple_StoreOnReceiver();
    void reinsertAllClient();
    //drop all clients
    virtual void dropAllClients();
    virtual void purgeBuffer();
protected:
    //add clients linked
    void insertClient();
    void moveClient(const quint8 &previousMovedUnit,const Direction &direction);
    void removeClient();
    void mapVisiblity_unloadFromTheMap();
    void reinsertClientForOthersOnSameMap();
private:
    static int index;
    static int loop_size;
    static MapVisibilityAlgorithm_Simple_StoreOnReceiver *current_client;//static to drop down the memory
    //overwrite
    //remove the move/remove
    void moveAnotherClientWithMap(MapVisibilityAlgorithm_Simple_StoreOnReceiver *the_another_player,const quint8 &movedUnit,const Direction &direction);
    void moveAnotherClientWithMap(const SIMPLIFIED_PLAYER_ID_TYPE &player_id,MapVisibilityAlgorithm_Simple_StoreOnReceiver *the_another_player,const quint8 &movedUnit,const Direction &direction);
    //for the purge buffer
    void send_insert();
    void send_move();
    void send_remove();
    void send_reinsert();
    #ifdef CATCHCHALLENGER_SERVER_VISIBILITY_CLEAR
    void insertAnotherClient(MapVisibilityAlgorithm_Simple_StoreOnReceiver *the_another_player);
    void insertAnotherClient(const SIMPLIFIED_PLAYER_ID_TYPE &player_id,MapVisibilityAlgorithm_Simple_StoreOnReceiver *the_another_player);
    void removeAnotherClient(const SIMPLIFIED_PLAYER_ID_TYPE &player_id);
    #endif
    void reinsertAnotherClient(MapVisibilityAlgorithm_Simple_StoreOnReceiver *the_another_player);
    void reinsertAnotherClient(const SIMPLIFIED_PLAYER_ID_TYPE &player_id,MapVisibilityAlgorithm_Simple_StoreOnReceiver *the_another_player);
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
    static QHash<SIMPLIFIED_PLAYER_ID_TYPE, MapVisibilityAlgorithm_Simple_StoreOnReceiver *>::const_iterator i_insert;
    static QHash<SIMPLIFIED_PLAYER_ID_TYPE, MapVisibilityAlgorithm_Simple_StoreOnReceiver *>::const_iterator i_insert_end;
    static QSet<SIMPLIFIED_PLAYER_ID_TYPE>::const_iterator i_remove;
    static QSet<SIMPLIFIED_PLAYER_ID_TYPE>::const_iterator i_remove_end;
    static CommonMap*			old_map;
    static CommonMap*			new_map;
    bool haveBufferToPurge;

    #ifdef CATCHCHALLENGER_SERVER_MAP_DROP_BLOCKED_MOVE
    quint8 previousMovedUnitBlocked;
    #endif

    //temp variable to move on the map
    static map_management_movement moveClient_tempMov;

    // stuff to send
    QHash<SIMPLIFIED_PLAYER_ID_TYPE, MapVisibilityAlgorithm_Simple_StoreOnReceiver *>			to_send_insert;
    QHash<SIMPLIFIED_PLAYER_ID_TYPE, QList<map_management_movement> >           to_send_move;
    QSet<SIMPLIFIED_PLAYER_ID_TYPE>                                             to_send_remove;
    QHash<SIMPLIFIED_PLAYER_ID_TYPE, MapVisibilityAlgorithm_Simple_StoreOnReceiver *>			to_send_reinsert;
public slots:
    //map slots, transmited by the current ClientNetworkRead
    virtual void put_on_the_map(CommonMap *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
    virtual bool moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction);
    virtual void teleportValidatedTo(CommonMap *map,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation);
private slots:
    virtual void extraStop();
};
}

#endif
