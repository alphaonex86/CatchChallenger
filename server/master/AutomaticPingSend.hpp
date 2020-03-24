#ifndef CATCHCHALLENGER_AutomaticPingSend_H
#define CATCHCHALLENGER_AutomaticPingSend_H

#ifdef EPOLLCATCHCHALLENGERSERVER

#include "../epoll/EpollTimer.hpp"
#include <stdint.h>

namespace CatchChallenger {
class AutomaticPingSend
        : public EpollTimer
{
public:
    explicit AutomaticPingSend(const uint32_t pingMSecond);
private:
    void exec();
    void timeDrift();
};
}

#endif // def EPOLLCATCHCHALLENGERSERVER
#endif // PLAYERUPDATERTOLOGIN_H
