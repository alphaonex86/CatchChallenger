#ifndef CATCHCHALLENGER_MAPVISIBILITYALGORITHM_SIMPLE_STOREONSENDER_H
#define CATCHCHALLENGER_MAPVISIBILITYALGORITHM_SIMPLE_STOREONSENDER_H

#include "../Client.hpp"
#include "Map_server_MapVisibility_Simple_StoreOnSender.hpp"
#include "../../../general/base/CommonMap.hpp"

namespace CatchChallenger {
class Map_server_MapVisibility_Simple_StoreOnSender;

class MapVisibilityAlgorithm_Simple_StoreOnSender : public Client
{
public:
    explicit MapVisibilityAlgorithm_Simple_StoreOnSender();
    ~MapVisibilityAlgorithm_Simple_StoreOnSender();
    void reinsertAllClient();
    //drop all clients
    void dropAllClients();
    void purgeBuffer() override;
protected:
    //add clients linked
    void insertClient() override;
    void moveClient(const uint8_t &previousMovedUnit,const Direction &direction) override;
    void removeClient();
    void mapVisiblity_unloadFromTheMap();
    void reinsertClientForOthersOnSameMap();
    void saveChange(Map_server_MapVisibility_Simple_StoreOnSender * const map);
private:
    static MapVisibilityAlgorithm_Simple_StoreOnSender *current_client;//static to drop down the memory
    //map load/unload and change
    void			loadOnTheMap();
    void			unloadFromTheMap();

    //map move
    bool singleMove(const Direction &direction) override;
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
    void put_on_the_map(CommonMap * const map,const /*COORD_TYPE*/uint8_t &x,const /*COORD_TYPE*/uint8_t &y,const Orientation &orientation) override;
    bool moveThePlayer(const uint8_t &previousMovedUnit,const Direction &direction) override;
    void teleportValidatedTo(CommonMap * const map,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation) override;
private:
    void extraStop() override;
};
}

#endif
