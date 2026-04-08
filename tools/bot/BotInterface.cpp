#include "BotInterface.h"

void BotInterface::dropAllPlayerOnTheMap(CatchChallenger::Api_protocol_Qt *api)
{
    (void)api;
}

void BotInterface::insert_player_all(CatchChallenger::Api_protocol_Qt *api, const CatchChallenger::Player_public_informations &player,
                const uint8_t &mapId, const uint8_t &x, const uint8_t &y, const CatchChallenger::Direction &direction)
{
    (void)api;
    (void)player;
    (void)mapId;
    (void)x;
    (void)y;
    (void)direction;
}

void BotInterface::remove_player(CatchChallenger::Api_protocol_Qt *api, const uint8_t &id)
{
    (void)api;
    (void)id;
}
