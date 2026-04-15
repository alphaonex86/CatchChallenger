#include "ActionsAction.h"

void ActionsAction::addBeatenBotFight(CatchChallenger::Api_protocol_Qt *api,const uint16_t &botFightId)
{
    if(clientList.find(api)==clientList.cend())
        return;
    const Player &player=clientList.at(api);
    //bot beaten now tracked per-map via mapData
    CatchChallenger::Player_private_and_public_informations &pinfo=api->get_player_informations();
    pinfo.mapData[player.mapId].bots_beaten.insert(botFightId);
}

bool ActionsAction::haveBeatBot(const CatchChallenger::Api_protocol_Qt *api,const uint16_t &botFightId)
{
    CatchChallenger::Api_protocol_Qt *mutableApi=const_cast<CatchChallenger::Api_protocol_Qt *>(api);
    if(clientList.find(mutableApi)==clientList.cend())
        return false;
    const Player &player=clientList.at(mutableApi);
    return api->haveBeatBot(player.mapId,botFightId);
}
