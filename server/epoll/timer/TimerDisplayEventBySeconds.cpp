#include "TimerDisplayEventBySeconds.h"
#include "../Epoll.h"

#include <iostream>

TimerDisplayEventBySeconds::TimerDisplayEventBySeconds()
{
    count=0;
}

void TimerDisplayEventBySeconds::exec()
{
    if(count<=1)
    {
        count=0;
        return;
    }
    std::cout << "event by seconds:" << (count-1) << std::endl;
    count=0;
    EpollTimer::exec();
}

void TimerDisplayEventBySeconds::addCount()
{
    if(count<=2*1000*1000*1000)
        count++;
}
