#ifndef TIMERDDOSLOGINSERVER_H
#define TIMERDDOSLOGINSERVER_H

#include "../cli/EventLoopTimer.hpp"

class TimerDdos : public EventLoopTimer
{
public:
    TimerDdos();
    void exec();
};

#endif // TIMERDDOSLOGINSERVER_H
