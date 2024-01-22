#ifndef TimerMove_H
#define TimerMove_H

#include "../../server/epoll/EpollTimer.hpp"

class TimerMove : public EpollTimer
{
public:
    TimerMove();
    void exec();
};

#endif // TimerMove_H
