#include "PlayerUpdaterToLogin.h"
#include "EpollClientLoginMaster.h"

using namespace CatchChallenger;

PlayerUpdaterToLogin::PlayerUpdaterToLogin()
{
    setInterval(15*1000);
}

void PlayerUpdaterToLogin::exec()
{
    EpollClientLoginMaster::sendServerChange();
    EpollClientLoginMaster::sendCurrentPlayer();
}
