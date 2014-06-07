#ifndef TIMERPOSITIONSYNC_H
#define TIMERPOSITIONSYNC_H

#include "EpollTimer.h"

class TimerPositionSync : public EpollTimer
{
public:
    TimerPositionSync();
    void exec();
};

#endif // TIMERPOSITIONSYNC_H
