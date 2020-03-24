#ifndef CATCHCHALLENGER_TimerReconnectOnTheMaster_H
#define CATCHCHALLENGER_TimerReconnectOnTheMaster_H

#ifdef EPOLLCATCHCHALLENGERSERVER

#include "../epoll/EpollTimer.hpp"

namespace CatchChallenger {
class TimerReconnectOnTheMaster
        : public EpollTimer
{
public:
    explicit TimerReconnectOnTheMaster();
private:
    void exec();
};
}

#endif // def EPOLLCATCHCHALLENGERSERVER
#endif // TimerReconnectOnTheMaster_H
