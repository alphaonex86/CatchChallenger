#include "PlayerUpdaterToMaster.h"
#include "PlayerUpdater.h"
#include "../game-server-alone/LoginLinkToMaster.h"

using namespace CatchChallenger;
PlayerUpdaterToMaster::PlayerUpdaterToMaster() :
    sended_connected_players(0)
{
    setInterval(15*1000);
}

void PlayerUpdaterToMaster::exec()
{
    if(sended_connected_players!=PlayerUpdater::connected_players)
    {
        sended_connected_players=PlayerUpdater::connected_players;
        LoginLinkToMaster::loginLinkToMaster->currentPlayerChange(PlayerUpdater::connected_players);
    }
}
