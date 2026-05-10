#ifndef TimerDetectTimeoutLOGINSERVER_H
#define TimerDetectTimeoutLOGINSERVER_H

#include "../cli/EventLoopTimer.hpp"

class TimerDetectTimeout : public EventLoopTimer
{
public:
    TimerDetectTimeout();
    void exec();
};

#endif // TimerDetectTimeoutLOGINSERVER_H
