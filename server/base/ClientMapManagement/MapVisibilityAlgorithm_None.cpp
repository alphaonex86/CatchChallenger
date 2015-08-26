#include "MapVisibilityAlgorithm_None.h"

using namespace CatchChallenger;

MapVisibilityAlgorithm_None::MapVisibilityAlgorithm_None(
        #ifdef EPOLLCATCHCHALLENGERSERVER
            #ifdef SERVERSSL
                const int &infd, SSL_CTX *ctx
            #else
                const int &infd
            #endif
        #else
        ConnectedSocket *socket
        #endif
        ) :
    Client(
        #ifdef EPOLLCATCHCHALLENGERSERVER
            #ifdef SERVERSSL
                infd,ctx
            #else
                infd
            #endif
        #else
        socket
        #endif
        )
{
}

void MapVisibilityAlgorithm_None::insertClient()
{
    //auto insert to know where it have spawn
    //ClientMapManagement::insertAnotherClient(player_id,map,x,y,direction,speed);
}

void MapVisibilityAlgorithm_None::moveClient(const uint8_t &movedUnit,const Direction &direction)
{
    Q_UNUSED(movedUnit);
    Q_UNUSED(direction);
}

void MapVisibilityAlgorithm_None::removeClient()
{
}

void MapVisibilityAlgorithm_None::mapVisiblity_unloadFromTheMap()
{
    removeClient();
}

void MapVisibilityAlgorithm_None::extraStop()
{
}

//map slots, transmited by the current ClientNetworkRead
/*void MapVisibilityAlgorithm_None::put_on_the_map(const SIMPLIFIED_PLAYER_ID_TYPE &player_id,Map_server_MapVisibility_simple *map,const uint16_t &x,const uint16_t &y,const Orientation &orientation,const uint16_t &speed)
{
    Q_UNUSED(player_id);
    Q_UNUSED(map);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(orientation);
    Q_UNUSED(speed);
}*/

bool MapVisibilityAlgorithm_None::moveThePlayer(const uint8_t &previousMovedUnit,const Direction &direction)
{
    //real move to save the possition
    if(!Client::moveThePlayer(previousMovedUnit,direction))
        return false;
    return true;
}

void MapVisibilityAlgorithm_None::purgeBuffer()
{
}

bool MapVisibilityAlgorithm_None::singleMove(const Direction &direction)
{
    return Client::singleMove(direction);
}
