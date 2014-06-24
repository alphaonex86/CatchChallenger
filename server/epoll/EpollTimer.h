#ifndef EPOLL_TIMER_H
#define EPOLL_TIMER_H

#include "BaseClassSwitch.h"

class EpollTimer : public BaseClassSwitch
{
public:
    EpollTimer();
    Type getType() const;
    bool start(const unsigned int &msec);
    bool start();
    void setInterval(const unsigned int &msec);
    void setSingleShot(const bool &singleShot);
    virtual void exec();
private:
    int tfd;
    unsigned int msec;
    bool singleShot;
};

#endif // EPOLL_TIMER_H
