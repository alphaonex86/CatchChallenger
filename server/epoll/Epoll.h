#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>

class Epoll
{
public:
    Epoll();
    bool init();
    int wait(epoll_event *events,const int &maxevents);
    int ctl(int __op, int __fd,epoll_event *__event);
    static Epoll epoll;
private:
    int efd;
};

#endif // EPOLL_H
