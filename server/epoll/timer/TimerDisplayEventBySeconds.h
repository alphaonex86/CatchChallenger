#ifndef TIMERDISPLAYEVENTBYSECONDS_H
#define TIMERDISPLAYEVENTBYSECONDS_H

#include "../EpollTimer.h"

class TimerDisplayEventBySeconds : public EpollTimer
{
public:
    TimerDisplayEventBySeconds();
    void exec();
    void addCount();
private:
    int count;
};

#endif // TIMERDISPLAYEVENTBYSECONDS_H
