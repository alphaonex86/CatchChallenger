#ifndef TIMERCITYCAPTURE_H
#define TIMERCITYCAPTURE_H

#include "EpollTimer.h"

class TimerCityCapture : public EpollTimer
{
public:
    TimerCityCapture();
    void exec();
};

#endif // TIMERCITYCAPTURE_H
