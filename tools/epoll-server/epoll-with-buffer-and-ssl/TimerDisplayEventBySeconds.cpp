#include "TimerDisplayEventBySeconds.h"
#include "Epoll.h"

#include <cstdio>
#include <stdio.h>

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
    fprintf(stdout, "event by seconds: %d\n",(count-1));
    count=0;
    Timer::exec();
}

void TimerDisplayEventBySeconds::addCount()
{
    if(count<=2*1000*1000*1000)
        count++;
}
