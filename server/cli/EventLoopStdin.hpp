#ifndef EVENT_LOOP_STDIN_H
#define EVENT_LOOP_STDIN_H

#include "BaseClassSwitch.h"

class EventLoopStdin : public BaseClassSwitch
{
public:
    EventLoopStdin();
    ~EventLoopStdin();
    EventLoopObjectType getType() const;
    void read();
protected:
    virtual void input(const char *input,unsigned int size) = 0;
};

#endif // EVENT_LOOP_STDIN_H
