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
    explicit MapVisibilityAlgorithm_Simple_StoreOnSender(
        #ifdef EPOLLCATCHCHALLENGERSERVER
            #ifdef SERVERSSL
                const int &infd, SSL_CTX *ctx
            #else
                const int &infd
            #endif
        #else
        ConnectedSocket *socket
        #endif
        );
    ~MapVisibilityAlgorithm_Simple_StoreOnSender();
    void reinsertAllClient();
    //drop all clients
    void dropAllClients();
    void purgeBuffer();
protected:
    //add clients linked
    void insertClient();
    void moveClient(const uint8_t &previousMovedUnit,const Direction &direction);
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
    bool loadTheRawUTF8String();
    int sizeOfOneSmallReinsert();

    static bool mapHaveChanged;

    #ifdef CATCHCHALLENGER_SERVER_MAP_DROP_BLOCKED_MOVE
    uint8_t previousMovedUnitBlocked;
    #endif
public:
    // stuff to send
    bool                                to_send_insert;
    bool                    			haveNewMove;
public:
    //map slots, transmited by the current ClientNetworkRead
    void put_on_the_map(CommonMap *map,const /*COORD_TYPE*/uint8_t &x,const /*COORD_TYPE*/uint8_t &y,const Orientation &orientation);
    bool moveThePlayer(const uint8_t &previousMovedUnit,const Direction &direction);
    void teleportValidatedTo(CommonMap *map,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation);
private:
    void extraStop();
};
}

#endif
