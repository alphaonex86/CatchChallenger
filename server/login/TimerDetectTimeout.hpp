#ifndef TimerDetectTimeoutLOGINSERVER_H
#define TimerDetectTimeoutLOGINSERVER_H

#include "../epoll/EpollTimer.hpp"

class TimerDetectTimeout : public EpollTimer
{
public:
    TimerDetectTimeout();
    void exec();
};

#endif // TimerDetectTimeoutLOGINSERVER_H
