#ifndef TimerDetectTimeoutLOGINSERVER_H
#define TimerDetectTimeoutLOGINSERVER_H

#include "../epoll/EpollTimer.h"

class TimerDetectTimeout : public EpollTimer
{
public:
    TimerDetectTimeout();
    void exec();
};

#endif // TimerDetectTimeoutLOGINSERVER_H
