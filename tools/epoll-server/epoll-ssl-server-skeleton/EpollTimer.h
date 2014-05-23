#ifndef EPOLL_TIMER_H
#define EPOLL_TIMER_H

#include "BaseClassSwitch.h"

class EpollTimer : public BaseClassSwitch
{
public:
    EpollTimer();
    Type getType() const;
    bool init();
    virtual void exec();
private:
    int tfd;
};

#endif // EPOLL_TIMER_H
