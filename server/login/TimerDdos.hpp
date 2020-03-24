#ifndef TIMERDDOSLOGINSERVER_H
#define TIMERDDOSLOGINSERVER_H

#include "../epoll/EpollTimer.hpp"

class TimerDdos : public EpollTimer
{
public:
    TimerDdos();
    void exec();
};

#endif // TIMERDDOSLOGINSERVER_H
