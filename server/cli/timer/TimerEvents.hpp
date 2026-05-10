#ifndef TIMEREVENTS_H
#define TIMEREVENTS_H

#include "../EventLoopTimer.hpp"

class TimerEvents : public EventLoopTimer
{
public:
    TimerEvents(const unsigned char &event,const unsigned char &value);
    void exec();
private:
    const unsigned char event;
    const unsigned char value;
};

#endif // TIMERDDOS_H
