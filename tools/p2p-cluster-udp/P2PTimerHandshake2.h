#ifndef CATCHCHALLENGER_P2PTimerConnect_H
#define CATCHCHALLENGER_P2PTimerConnect_H

#ifdef EPOLLCATCHCHALLENGERSERVER

#include "../../server/epoll/EpollTimer.h"
#include <nettle/eddsa.h>
#include <chrono>

namespace CatchChallenger {
class P2PTimerHandshake2
        : public EpollTimer
{
public:
    explicit P2PTimerHandshake2();
    void exec();
private:
    std::chrono::time_point<std::chrono::steady_clock> startTime;
};
}

#endif // def EPOLLCATCHCHALLENGERSERVER
#endif // PLAYERUPDATERTOLOGIN_H
