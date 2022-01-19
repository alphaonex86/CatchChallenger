#ifndef TIMERDDOS_H
#define TIMERDDOS_H

#include "../EpollTimer.hpp"

class TimerDdos : public EpollTimer
{
public:
    TimerDdos();
    void exec();
};

#endif // TIMERDDOS_H
