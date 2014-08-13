#ifndef CATCHCHALLENGER_MAPVISIBILITYALGORITHM_NONE_H
#define CATCHCHALLENGER_MAPVISIBILITYALGORITHM_NONE_H

#include "../Client.h"

namespace CatchChallenger {
class MapVisibilityAlgorithm_None : public Client
{
public:
    explicit MapVisibilityAlgorithm_None(
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
protected:
    //add clients linked
    void insertClient();
    void moveClient(const quint8 &previousMovedUnit,const Direction &direction);
    void removeClient();
    void mapVisiblity_unloadFromTheMap();
    void extraStop();
    //map move
    bool singleMove(const Direction &direction);
public:
    //map slots, transmited by the current ClientNetworkRead
    //void put_on_the_map(const SIMPLIFIED_PLAYER_ID_TYPE &player_id,Map_server_MapVisibility_simple *map,const quint16 &x,const quint16 &y,const Orientation &orientation,const quint16 &speed);
    bool moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction);

    void purgeBuffer();
};
}

#endif // MAPVISIBILITYALGORITHM_SIMPLE_H
