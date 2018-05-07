#ifndef CATCHCHALLENGER_P2PTimerConnect_H
#define CATCHCHALLENGER_P2PTimerConnect_H

#ifdef EPOLLCATCHCHALLENGERSERVER

#include "../../server/epoll/EpollTimer.h"
#include <nettle/eddsa.h>
#include <chrono>

namespace CatchChallenger {
class P2PTimerConnect
        : public EpollTimer
{
public:
    explicit P2PTimerConnect();
    void exec();
private:
    //[8(current sequence number)+8(next sequence number -> random)+4(size)+1(request type)+ED25519_SIGNATURE_SIZE+ED25519_KEY_SIZE+ED25519_SIGNATURE_SIZE]
    static char handShake1[8+8+4+1+ED25519_SIGNATURE_SIZE+ED25519_KEY_SIZE+ED25519_SIGNATURE_SIZE];

    std::chrono::time_point<std::chrono::steady_clock> startTime;
};
}

#endif // def EPOLLCATCHCHALLENGERSERVER
#endif // PLAYERUPDATERTOLOGIN_H
