#ifndef CATCHCHALLENGER_P2PTimerConnect3_H
#define CATCHCHALLENGER_P2PTimerConnect3_H

#ifdef EPOLLCATCHCHALLENGERSERVER

#include "../../server/epoll/EpollTimer.h"
#include <nettle/eddsa.h>
#include <chrono>

namespace CatchChallenger {
class P2PTimerHandshake3
        : public EpollTimer
{
public:
    explicit P2PTimerHandshake3();
    void exec();
private:
    std::chrono::time_point<std::chrono::steady_clock> startTime;
};
}

#endif // def EPOLLCATCHCHALLENGERSERVER
#endif // PLAYERUPDATERTOLOGIN_H
