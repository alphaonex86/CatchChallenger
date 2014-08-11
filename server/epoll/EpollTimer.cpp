#include "EpollTimer.h"
#include "Epoll.h"

#include <iostream>
#include <sys/timerfd.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>

char buff_temp[sizeof(uint64_t)];

EpollTimer::EpollTimer() :
    tfd(-1),
    msec(0),
    offset(0),
    singleShot(false)
{
}

BaseClassSwitch::Type EpollTimer::getType() const
{
    return BaseClassSwitch::Type::Timer;
}

bool EpollTimer::start(const unsigned int &msec, unsigned int offset)
{
    if(tfd!=-1)
        return false;
    if(msec<1)
        return false;
    if(offset==0)
        offset=msec;
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
    if(singleShot)
    {
        new_value.it_value.tv_sec = now.tv_sec + msec/1000;
        new_value.it_value.tv_nsec = now.tv_nsec + (msec%1000)*1000000;
        if(new_value.it_value.tv_nsec>999999999)
        {
            new_value.it_value.tv_nsec-=1000000000;
            new_value.it_value.tv_sec++;
        }
        new_value.it_interval.tv_sec = 0;
        new_value.it_interval.tv_nsec = 0;
    }
    else
    {
        new_value.it_value.tv_sec = now.tv_sec + offset/1000;
        new_value.it_value.tv_nsec = now.tv_nsec + (offset%1000)*1000000;
        if(new_value.it_value.tv_nsec>999999999)
        {
            new_value.it_value.tv_nsec-=1000000000;
            new_value.it_value.tv_sec++;
        }
        new_value.it_interval.tv_sec = msec/1000;
        new_value.it_interval.tv_nsec = (msec%1000)*1000000;
        if(new_value.it_interval.tv_nsec>999999999)
        {
            new_value.it_interval.tv_nsec-=1000000000;
            new_value.it_interval.tv_sec++;
        }
    }

    const int &result=::timerfd_settime(tfd, TFD_TIMER_ABSTIME, &new_value, NULL);
    if(result<0)
    {
        //settime error: 22: Invalid argument
        std::cerr << "settime error: " << errno << ": " << strerror(errno) << std::endl;
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

bool EpollTimer::start()
{
    return start(msec,offset);
}

bool EpollTimer::stop()
{
    if(tfd==-1)
        return false;
    if(Epoll::epoll.ctl(EPOLL_CTL_DEL,tfd,NULL) < 0)
    {
        std::cerr << "epoll_ctl error" << std::endl;
        return false;
    }
    ::close(tfd);
    tfd=-1;
    return true;
}

void EpollTimer::setInterval(const unsigned int &msec,const unsigned int &offset)
{
    this->msec=msec;
    if(offset==0)
        this->offset=msec;
    else
        this->offset=offset;
    if(tfd!=-1)
    {
        stop();
        start();
    }
}

void EpollTimer::setSingleShot(const bool &singleShot)
{
    this->singleShot=singleShot;
}

void EpollTimer::validateTheTimer()
{
    ::read(tfd, buff_temp, sizeof(uint64_t));
}
