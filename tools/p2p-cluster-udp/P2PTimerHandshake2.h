#ifndef CATCHCHALLENGER_P2PTimerConnect2_H
#define CATCHCHALLENGER_P2PTimerConnect2_H

#ifdef EPOLLCATCHCHALLENGERSERVER

#include "../../server/epoll/EpollTimer.h"
#include <nettle/eddsa.h>
#include <chrono>
#include <unordered_set>

namespace CatchChallenger {
class P2PTimerHandshake2
        : public EpollTimer
{
public:
    explicit P2PTimerHandshake2();
    void exec();
private:
    std::chrono::time_point<std::chrono::steady_clock> startTime;
    std::unordered_set<std::string> clientSend;
};
}

#endif // def EPOLLCATCHCHALLENGERSERVER
#endif // PLAYERUPDATERTOLOGIN_H
