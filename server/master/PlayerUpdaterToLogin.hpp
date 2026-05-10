#ifndef CATCHCHALLENGER_PLAYERUPDATERTOLOGIN_H
#define CATCHCHALLENGER_PLAYERUPDATERTOLOGIN_H

#ifdef CATCHCHALLENGER_SERVER

#include "../cli/EventLoopTimer.hpp"

namespace CatchChallenger {
class PlayerUpdaterToLogin
        : public EventLoopTimer
{
public:
    explicit PlayerUpdaterToLogin();
private:
    void exec();
    enum FrequencyUpdate
    {
        FrequencyUpdate_slow,
        FrequencyUpdate_medium,
        FrequencyUpdate_fast,
    };
    FrequencyUpdate frequencyUpdate;
    int oldnumberOfPlayer;
};
}

#endif // def CATCHCHALLENGER_SERVER
#endif // PLAYERUPDATERTOLOGIN_H
