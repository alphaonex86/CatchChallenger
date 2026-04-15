#include "BotInterface.h"

void BotInterface::dropAllPlayerOnTheMap(CatchChallenger::Api_protocol_Qt *api)
{
    (void)api;
}

void BotInterface::insert_player_all(CatchChallenger::Api_protocol_Qt *api, const CatchChallenger::Player_public_informations &player,
                const CATCHCHALLENGER_TYPE_MAPID &mapId, const COORD_TYPE &x, const COORD_TYPE &y, const CatchChallenger::Direction &direction)
{
    (void)api;
    (void)player;
    (void)mapId;
    (void)x;
    (void)y;
    (void)direction;
}

void BotInterface::remove_player(CatchChallenger::Api_protocol_Qt *api, const SIMPLIFIED_PLAYER_ID_FOR_MAP &id)
{
    (void)api;
    (void)id;
}
