#ifndef TIMERPOSITIONSYNC_H
#define TIMERPOSITIONSYNC_H

#include "../EventLoopTimer.hpp"

class TimerPositionSync : public EventLoopTimer
{
public:
    TimerPositionSync();
    void exec();
};

#endif // TIMERPOSITIONSYNC_H
