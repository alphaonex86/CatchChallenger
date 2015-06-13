#include "PlayerUpdaterToLogin.h"
#include "EpollClientLoginMaster.h"

using namespace CatchChallenger;

PlayerUpdaterToLogin::PlayerUpdaterToLogin() :
    sended_connected_players(0)
{
    setInterval(15*1000);
}

void PlayerUpdaterToLogin::exec()
{
    EpollClientLoginMaster::sendCurrentPlayer();
}
