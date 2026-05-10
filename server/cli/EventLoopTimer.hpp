#ifndef EPOLL_TIMER_H
#define EPOLL_TIMER_H

#include "BaseClassSwitch.hpp"

class EpollTimer : public BaseClassSwitch
{
public:
    EpollTimer();
    ~EpollTimer();
    EpollObjectType getType() const;
    bool start(const unsigned int &msec,unsigned int offset=0);
    bool start();
    bool stop();
    bool active();
    void setInterval(const unsigned int &msec,const unsigned int &offset=0);
    void setSingleShot(const bool &singleShot);
    virtual void exec() = 0;
    void validateTheTimer();
private:
    int tfd;
    unsigned int msec;
    unsigned int offset;
    bool singleShot;
};

#endif // EPOLL_TIMER_H
