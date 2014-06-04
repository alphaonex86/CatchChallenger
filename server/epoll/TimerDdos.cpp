#include "TimerDdos.h"
#include "Epoll.h"

#include <iostream>

TimerDdos::TimerDdos()
{
}

void TimerDdos::exec()
{
    EpollTimer::exec();
}
