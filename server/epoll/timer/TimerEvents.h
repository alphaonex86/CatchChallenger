#ifndef TIMEREVENTS_H
#define TIMEREVENTS_H

#include "../EpollTimer.h"

class TimerEvents : public EpollTimer
{
public:
    TimerEvents(const unsigned char &event,const unsigned char &value);
    void exec();
private:
    const unsigned char event;
    const unsigned char value;
};

#endif // TIMERDDOS_H
