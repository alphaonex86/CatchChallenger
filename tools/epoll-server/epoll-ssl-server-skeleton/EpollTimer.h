#ifndef TIMER_H
#define TIMER_H

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

#endif // TIMER_H
