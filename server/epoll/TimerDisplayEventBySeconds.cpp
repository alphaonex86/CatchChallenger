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
    fprintf(stdout, "event by seconds: %d\n",count);
    count=0;
    EpollTimer::exec();
}

void TimerDisplayEventBySeconds::addCount()
{
    if(count<=2*1000*1000*1000)
        count++;
}
