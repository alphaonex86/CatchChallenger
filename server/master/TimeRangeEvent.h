#ifndef CATCHCHALLENGER_TimeRangeEvent_H
#define CATCHCHALLENGER_TimeRangeEvent_H

#ifdef EPOLLCATCHCHALLENGERSERVER

#include "../epoll/EpollTimer.h"

namespace CatchChallenger {
class TimeRangeEvent
        : public EpollTimer
{
public:
    explicit TimeRangeEvent();
private:
    void exec();
};
}

#endif // def EPOLLCATCHCHALLENGERSERVER
#endif // PLAYERUPDATERTOLOGIN_H
