#ifndef CATCHCHALLENGER_PLAYERUPDATERTOLOGIN_H
#define CATCHCHALLENGER_PLAYERUPDATERTOLOGIN_H

#ifdef EPOLLCATCHCHALLENGERSERVER

#include "../epoll/EpollTimer.hpp"

namespace CatchChallenger {
class PlayerUpdaterToLogin
        : public EpollTimer
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

#endif // def EPOLLCATCHCHALLENGERSERVER
#endif // PLAYERUPDATERTOLOGIN_H
