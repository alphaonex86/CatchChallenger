#include "Timer.h"
#include "Epoll.h"

#include <time.h>
#include <sys/timerfd.h>
#include <stdio.h>
#include <unistd.h>

char buff_temp[sizeof(uint64_t)];

Timer::Timer()
{
}

BaseClassSwitch::Type Timer::getType()
{
    return BaseClassSwitch::Type::Timer;
}

bool Timer::init()
{
    if((tfd=::timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK)) < 0)
    {
        perror("timerfd create error");
        return false;
    }

    timespec now;
    if (clock_gettime(CLOCK_REALTIME, &now) == -1)
    {
        perror("clock_gettime");
        return false;
    }
    itimerspec new_value;
    new_value.it_value.tv_sec = now.tv_sec + 2;
    new_value.it_value.tv_nsec = now.tv_nsec;
    if(new_value.it_value.tv_nsec>999999999)
    {
        new_value.it_value.tv_nsec-=1000000000;
        new_value.it_value.tv_sec++;
    }
    new_value.it_interval.tv_sec = 1;
    new_value.it_interval.tv_nsec = 0;


    int result=::timerfd_settime(tfd, TFD_TIMER_ABSTIME, &new_value, NULL);
    if(result<0)
    {
        perror("settime error");
        return false;
    }
    epoll_event event;
    event.data.ptr = this;
    event.events = EPOLLIN;
    if(Epoll::epoll.ctl(EPOLL_CTL_ADD,tfd,&event) < 0)
    {
        perror("epoll_ctl error");
        return false;
    }
    return true;
}

void Timer::exec()
{
    ::read(tfd, buff_temp, sizeof(uint64_t));
}
