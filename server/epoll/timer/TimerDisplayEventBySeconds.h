#ifdef SERVERBENCHMARKFULL

#ifndef TIMERDISPLAYEVENTBYSECONDS_H
#define TIMERDISPLAYEVENTBYSECONDS_H

#include "../EpollTimer.h"

class TimerDisplayEventBySeconds : public EpollTimer
{
public:
    TimerDisplayEventBySeconds();
    void exec();
    void addServerCount();
    void addClientCount();
    void addDbCount();
    void addTimerCount();
    void addOtherCount();
private:
    int serverCount,clientCount,dbCount,timerCount,otherCount;
};

#endif // TIMERDISPLAYEVENTBYSECONDS_H

#endif
