#include "ActionsAction.h"

void ActionsAction::addBeatenBotFight(CatchChallenger::Api_protocol *api,const uint16_t &botFightId)
{
    CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations();
    char * const botAlreadyBeaten=player.bot_already_beaten;
    if(botAlreadyBeaten==NULL)
        abort();
    botAlreadyBeaten[botFightId/8]|=(1<<(7-botFightId%8));
}

bool ActionsAction::haveBeatBot(CatchChallenger::Api_protocol *api,const uint16_t &botFightId)
{
    const CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations();
    char * const botAlreadyBeaten=player.bot_already_beaten;
    if(botAlreadyBeaten==NULL)
        abort();
    return botAlreadyBeaten[botFightId/8] & (1<<(7-botFightId%8));
}
