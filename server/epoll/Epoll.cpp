#include "Epoll.h"
#include "../../general/base/GeneralVariable.h"

#include <iostream>

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
        std::cerr << "epoll_create failed" << std::endl;
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
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(efd==-1)
    {
        std::cerr << "Epoll::ctl failed, efd==-1, call Epoll::init() before" << std::endl;
        abort();
    }
    #endif
    return epoll_ctl(efd, __op, __fd, __event);
}
