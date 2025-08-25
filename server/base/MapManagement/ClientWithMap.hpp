#ifndef CATCHCHALLENGER_MAPVISIBILITYALGORITHM_SIMPLE_STOREONSENDER_H
#define CATCHCHALLENGER_MAPVISIBILITYALGORITHM_SIMPLE_STOREONSENDER_H

#include "../Client.hpp"
#include "Map_server_MapVisibility_Simple_StoreOnSender.hpp"
#include "../../../general/base/CommonMap.hpp"

namespace CatchChallenger {
class Map_server_MapVisibility_Simple_StoreOnSender;

class ClientWithMap : public Client
{
public:
    explicit ClientWithMap();
    ~ClientWithMap();
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
private:
    /* static allocation, with holes, see doc/algo/visibility/constant-time-player-visibility.png
     * can add carbage collector to not search free holes */
    static std::vector<ClientWithMap> clientBroadCastList;

    //map load/unload and change
    void			loadOnTheMap();
    void			unloadFromTheMap();

    //map move
    bool singleMove(const Direction &direction) override;
    bool loadTheRawUTF8String();
    int sizeOfOneSmallReinsert();

    #ifdef CATCHCHALLENGER_SERVER_MAP_DROP_BLOCKED_MOVE
    uint8_t previousMovedUnitBlocked;
    #endif
public:
public:
    //map slots, transmited by the current ClientNetworkRead
    void put_on_the_map(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation) override;
    bool moveThePlayer(const uint8_t &previousMovedUnit,const Direction &direction) override;
    void teleportValidatedTo(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation) override;
private:
    void extraStop() override;
};
}

#endif
