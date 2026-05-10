#ifndef TIMERDDOS_H
#define TIMERDDOS_H

#include "../EventLoopTimer.hpp"

class TimerDdos : public EventLoopTimer
{
public:
    TimerDdos();
    void exec();
};

#endif // TIMERDDOS_H
