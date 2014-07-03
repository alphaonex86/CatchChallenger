#include "EpollTimer.h"
#include "Epoll.h"

#include <iostream>
#include <sys/timerfd.h>
#include <unistd.h>

char buff_temp[sizeof(uint64_t)];

EpollTimer::EpollTimer()
{
}

BaseClassSwitch::Type EpollTimer::getType() const
{
    return BaseClassSwitch::Type::Timer;
}

bool EpollTimer::init()
{
    if((tfd=::timerfd_create(CLOCK_REALTIME,TFD_NONBLOCK)) < 0)
    {
        std::cerr << "Timer creation error" << std::endl;
        return false;
    }

    timespec now;
    if (clock_gettime(CLOCK_REALTIME, &now) == -1)
    {
        std::cerr << "clock_gettime error" << std::endl;
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
        std::cerr << "settime error" << std::endl;
        return false;
    }
    epoll_event event;
    event.data.ptr = this;
    event.events = EPOLLIN;
    if(Epoll::epoll.ctl(EPOLL_CTL_ADD,tfd,&event) < 0)
    {
        std::cerr << "epoll_ctl error" << std::endl;
        return false;
    }
    return true;
}

void EpollTimer::exec()
{
    ::read(tfd, buff_temp, sizeof(uint64_t));
}
