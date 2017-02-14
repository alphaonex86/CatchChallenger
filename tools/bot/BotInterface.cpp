#include "BotInterface.h"

void BotInterface::dropAllPlayerOnTheMap(CatchChallenger::Api_protocol *api)
{
    (void)api;
}

void BotInterface::insert_player_all(CatchChallenger::Api_protocol *api, const CatchChallenger::Player_public_informations &player, const quint32 &mapId, const quint16 &x, const quint16 &y, const CatchChallenger::Direction &direction)
{
    (void)api;
    (void)player;
    (void)mapId;
    (void)x;
    (void)y;
    (void)direction;
}

void BotInterface::remove_player(CatchChallenger::Api_protocol *api, const uint16_t &id)
{
    (void)api;
    (void)id;
}
