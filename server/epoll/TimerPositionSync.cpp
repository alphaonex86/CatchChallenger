#include "TimerPositionSync.h"
#include "Epoll.h"

#include <iostream>

TimerPositionSync::TimerPositionSync()
{
}

void TimerPositionSync::exec()
{
    EpollTimer::exec();
}
