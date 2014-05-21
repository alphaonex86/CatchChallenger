#include "Epoll.h"

#include <cstdio>

Epoll Epoll::epoll;

Epoll::Epoll()
{
    efd=-1;
}

bool Epoll::init()
{
    efd = epoll_create1(0);
    if(efd == -1)
    {
        perror("epoll_create");
        return false;
    }
    return true;
}

int Epoll::wait(epoll_event *events,const int &maxevents)
{
    return epoll_wait(efd, events, maxevents, -1);
}

int Epoll::ctl(int __op, int __fd,epoll_event *__event)
{
    return epoll_ctl(efd, __op, __fd, __event);
}
