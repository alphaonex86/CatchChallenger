#ifndef CATCHCHALLENGER_MAPVISIBILITYALGORITHM_NONE_H
#define CATCHCHALLENGER_MAPVISIBILITYALGORITHM_NONE_H

#include "../Client.hpp"

namespace CatchChallenger {
class MapVisibilityAlgorithm_None : public Client
{
public:
    explicit MapVisibilityAlgorithm_None();
protected:
    //add clients linked
    void insertClient() override;
    void moveClient(const uint8_t &previousMovedUnit,const Direction &direction) override;
    void removeClient();
    void mapVisiblity_unloadFromTheMap();
    void extraStop() override;
    //map move
    bool singleMove(const Direction &direction) override;
public:
    //map slots, transmited by the current ClientNetworkRead
    //void put_on_the_map(const SIMPLIFIED_PLAYER_ID_TYPE &player_id,Map_server_MapVisibility_simple *map,const uint16_t &x,const uint16_t &y,const Orientation &orientation,const uint16_t &speed);
    bool moveThePlayer(const uint8_t &previousMovedUnit,const Direction &direction) override;

    void purgeBuffer() override;
};
}

#endif // MAPVISIBILITYALGORITHM_SIMPLE_H
