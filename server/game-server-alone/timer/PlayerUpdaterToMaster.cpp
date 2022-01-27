#include "PlayerUpdaterToMaster.hpp"
#include "../base/PlayerUpdaterBase.hpp"
#include "LinkToMaster.hpp"

using namespace CatchChallenger;
PlayerUpdaterToMaster::PlayerUpdaterToMaster() :
    sended_connected_players(0)
{
    setInterval(500);
    frequencyUpdate=FrequencyUpdate_fast;
}

void PlayerUpdaterToMaster::exec()
{
    if(sended_connected_players!=PlayerUpdaterBase::connected_players)
    {
        sended_connected_players=PlayerUpdaterBase::connected_players;
        LinkToMaster::linkToMaster->currentPlayerChange(PlayerUpdaterBase::connected_players);
        //update frequencyUpdate if needed
        switch(frequencyUpdate)
        {
            case FrequencyUpdate_fast:
            if(PlayerUpdaterBase::connected_players>1000)
            {
                frequencyUpdate=FrequencyUpdate_slow;
                setInterval(60*1000);
            }
            else if(PlayerUpdaterBase::connected_players>30)
            {
                frequencyUpdate=FrequencyUpdate_medium;
                setInterval(15*1000);
            }
            break;
            case FrequencyUpdate_medium:
            if(PlayerUpdaterBase::connected_players>1000)
            {
                frequencyUpdate=FrequencyUpdate_slow;
                setInterval(60*1000);
            }
            else if(PlayerUpdaterBase::connected_players<30)
            {
                frequencyUpdate=FrequencyUpdate_fast;
                setInterval(500);
            }
            break;
            case FrequencyUpdate_slow:
            if(PlayerUpdaterBase::connected_players<30)
            {
                frequencyUpdate=FrequencyUpdate_fast;
                setInterval(500);
            }
            else if(PlayerUpdaterBase::connected_players<1000)
            {
                frequencyUpdate=FrequencyUpdate_medium;
                setInterval(15*1000);
            }
            break;
            default:abort();break;
        }
    }
}
