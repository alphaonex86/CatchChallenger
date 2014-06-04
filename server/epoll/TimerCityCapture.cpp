#include "TimerCityCapture.h"
#include "Epoll.h"

#include <iostream>

TimerCityCapture::TimerCityCapture()
{
}

void TimerCityCapture::exec()
{
    EpollTimer::exec();
}
