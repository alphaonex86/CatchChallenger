#ifdef CATCHCHALLENGER_CLASS_ALLINONESERVER
#ifndef CATCHCHALLENGER_TimeRangeEvent_H
#define CATCHCHALLENGER_TimeRangeEvent_H

#ifdef CATCHCHALLENGER_SERVER

#include "../cli/EventLoopTimer.hpp"

namespace CatchChallenger {
class TimeRangeEvent
        : public EventLoopTimer
{
public:
    explicit TimeRangeEvent();
private:
    void exec();
};
}

#endif // def CATCHCHALLENGER_SERVER
#endif // PLAYERUPDATERTOLOGIN_H
#endif
