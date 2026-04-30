#ifndef CATCHCHALLENGER_CheckTimeoutGameServer_H
#define CATCHCHALLENGER_CheckTimeoutGameServer_H

#ifdef CATCHCHALLENGER_SERVER

#include "../epoll/EpollTimer.hpp"
#include <stdint.h>

namespace CatchChallenger {
class CheckTimeoutGameServer
        : public EpollTimer
{
public:
    explicit CheckTimeoutGameServer(const uint32_t checkTimeoutGameServerMSecond);
private:
    void exec();
    void timeDrift();
};
}

#endif // def CATCHCHALLENGER_SERVER
#endif // PLAYERUPDATERTOLOGIN_H
