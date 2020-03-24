#include "PlayerUpdaterToLogin.hpp"
#include "EpollClientLoginMaster.hpp"

using namespace CatchChallenger;

PlayerUpdaterToLogin::PlayerUpdaterToLogin()
{
    setInterval(500);
    frequencyUpdate=FrequencyUpdate_fast;
    oldnumberOfPlayer=0;
}

void PlayerUpdaterToLogin::exec()
{
    EpollClientLoginMaster::sendServerChange();
    if(!EpollClientLoginMaster::havePlayerCountChange)
        return;
    EpollClientLoginMaster::havePlayerCountChange=false;
    const int numberOfPlayer=EpollClientLoginMaster::sendCurrentPlayer();

    if(oldnumberOfPlayer!=numberOfPlayer)
    {
        oldnumberOfPlayer=numberOfPlayer;
        switch(frequencyUpdate)
        {
            case FrequencyUpdate_fast:
            if(numberOfPlayer>1000)
            {
                frequencyUpdate=FrequencyUpdate_slow;
                setInterval(60*1000);
            }
            else if(numberOfPlayer>30)
            {
                frequencyUpdate=FrequencyUpdate_medium;
                setInterval(15*1000);
            }
            break;
            case FrequencyUpdate_medium:
            if(numberOfPlayer>1000)
            {
                frequencyUpdate=FrequencyUpdate_slow;
                setInterval(60*1000);
            }
            else if(numberOfPlayer<30)
            {
                frequencyUpdate=FrequencyUpdate_fast;
                setInterval(500);
            }
            break;
            case FrequencyUpdate_slow:
            if(numberOfPlayer<30)
            {
                frequencyUpdate=FrequencyUpdate_fast;
                setInterval(500);
            }
            else if(numberOfPlayer<1000)
            {
                frequencyUpdate=FrequencyUpdate_medium;
                setInterval(15*1000);
            }
            break;
            default:abort();break;
        }
    }
}
