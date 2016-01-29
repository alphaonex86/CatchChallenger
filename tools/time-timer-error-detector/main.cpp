#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <ctime>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define MAXEVENTS 512

int main()
{
    /* Buffer where events are returned */
    epoll_event events[MAXEVENTS];
    char buff_temp[sizeof(uint64_t)];

    auto efd = epoll_create1(0);
    if(efd == -1)
    {
        std::cerr << "epoll_create failed" << std::endl;
        return -1;
    }

    auto tfd=::timerfd_create(CLOCK_REALTIME,TFD_NONBLOCK);
    if(tfd < 0)
    {
        std::cerr << "Timer creation error" << std::endl;
        return -1;
    }

    timespec now;
    if (clock_gettime(CLOCK_REALTIME, &now) == -1)
    {
        std::cerr << "clock_gettime error" << std::endl;
        return -1;
    }
    itimerspec new_value;

    int msec=1000;
    new_value.it_value.tv_sec = now.tv_sec + msec/1000;
    new_value.it_value.tv_nsec = now.tv_nsec + (msec%1000)*1000000;
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

    const int &result=::timerfd_settime(tfd, TFD_TIMER_ABSTIME, &new_value, NULL);
    if(result<0)
    {
        //settime error: 22: Invalid argument
        std::cerr << "settime error: " << errno << ": " << strerror(errno) << std::endl;
        return -1;
    }
    epoll_event event;
    event.data.ptr = NULL;
    event.events = EPOLLIN;
    if(epoll_ctl(efd,EPOLL_CTL_ADD,tfd,&event) < 0)
    {
        std::cerr << "epoll_ctl error" << std::endl;
        return -1;
    }

    /* The event loop */
    int number_of_events, i;
    while(1)
    {
        const uint64_t &timestartms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        number_of_events = epoll_wait(efd, events, MAXEVENTS, -1);
        const uint64_t &timestopms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        for(i = 0; i < number_of_events; i++)
        {
            if(::read(tfd, buff_temp, sizeof(uint64_t))!=sizeof(uint64_t))
            {
            }
            if(timestartms>timestopms)
                std::cerr << "Time drift detected (negative value), timer on 1000ms" << std::endl;
            else if(timestartms==timestopms)
                std::cerr << "Timer directly return, are you debugging? If not it's a problem" << std::endl;
            else
            {
                const uint64_t &timediffms=timestopms-timestartms;
                if(timediffms<900 || timediffms>1100)//drift >10%
                    std::cerr << "Time drift more than +/-10% detected, timer on 1000ms, but time diff detected is: " << timediffms << "ms" << std::endl;
                else
                    std::cout << "Timer and time match, all is good, drift is: " << (100-100*(float)timediffms/1000) << "%" << std::endl;
            }
        }
    }

    return 0;
}
