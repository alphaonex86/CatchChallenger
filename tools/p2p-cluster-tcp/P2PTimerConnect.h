#ifndef CATCHCHALLENGER_P2PTimerConnect_H
#define CATCHCHALLENGER_P2PTimerConnect_H

#ifdef EPOLLCATCHCHALLENGERSERVER

#include "../../server/epoll/EpollTimer.h"

namespace CatchChallenger {
class P2PTimerConnect
        : public EpollTimer
{
public:
    explicit P2PTimerConnect();
    void exec();
};
}

#endif // def EPOLLCATCHCHALLENGERSERVER
#endif // PLAYERUPDATERTOLOGIN_H
