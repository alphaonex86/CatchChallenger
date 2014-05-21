#ifndef TIMER_H
#define TIMER_H

#include "BaseClassSwitch.h"

class Timer : public BaseClassSwitch
{
public:
    Timer();
    Type getType();
    bool init();
    virtual void exec();
private:
    int tfd;
};

#endif // TIMER_H
