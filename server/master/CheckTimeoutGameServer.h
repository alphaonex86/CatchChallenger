#ifndef CATCHCHALLENGER_CheckTimeoutGameServer_H
#define CATCHCHALLENGER_CheckTimeoutGameServer_H

#ifdef EPOLLCATCHCHALLENGERSERVER

#include "../epoll/EpollTimer.h"
#include <stdint.h>

namespace CatchChallenger {
class CheckTimeoutGameServer
        : public EpollTimer
{
public:
    explicit CheckTimeoutGameServer(const uint32_t pingMSecond);
private:
    void exec();
    void timeDrift();
};
}

#endif // def EPOLLCATCHCHALLENGERSERVER
#endif // PLAYERUPDATERTOLOGIN_H
