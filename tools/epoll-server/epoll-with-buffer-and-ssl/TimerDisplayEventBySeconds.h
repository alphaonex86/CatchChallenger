#ifndef TIMERDISPLAYEVENTBYSECONDS_H
#define TIMERDISPLAYEVENTBYSECONDS_H

#include "Timer.h"

class TimerDisplayEventBySeconds : public Timer
{
public:
    TimerDisplayEventBySeconds();
    void exec();
    void addCount();
private:
    int count;
};

#endif // TIMERDISPLAYEVENTBYSECONDS_H
