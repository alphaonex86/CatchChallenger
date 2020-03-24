#ifdef CATCHCHALLENGER_CLASS_ALLINONESERVER
#ifndef CATCHCHALLENGER_TimeRangeEvent_H
#define CATCHCHALLENGER_TimeRangeEvent_H

#ifdef EPOLLCATCHCHALLENGERSERVER

#include "../epoll/EpollTimer.hpp"

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
#endif
