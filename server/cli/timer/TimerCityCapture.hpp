#ifndef TIMERCITYCAPTURE_H
#define TIMERCITYCAPTURE_H

#include "../EventLoopTimer.hpp"

class TimerCityCapture : public EventLoopTimer
{
public:
    TimerCityCapture();
    void exec();
};

#endif // TIMERCITYCAPTURE_H
