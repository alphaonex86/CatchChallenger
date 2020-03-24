#ifndef EPOLL_STDIN_H
#define EPOLL_STDIN_H

#include "BaseClassSwitch.h"

class EpollStdin : public BaseClassSwitch
{
public:
    EpollStdin();
    ~EpollStdin();
    EpollObjectType getType() const;
    void read();
protected:
    virtual void input(const char *input,unsigned int size) = 0;
};

#endif // EPOLL_STDIN_H
