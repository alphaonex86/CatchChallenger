#ifndef EVENT_LOOP_TIMER_H
#define EVENT_LOOP_TIMER_H

#include "BaseClassSwitch.hpp"

class EventLoopTimer : public BaseClassSwitch
{
public:
    EventLoopTimer();
    ~EventLoopTimer();
    EventLoopObjectType getType() const;
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
#ifdef _WIN32
    void *timer_ctx;
#endif
};

#endif // EVENT_LOOP_TIMER_H
