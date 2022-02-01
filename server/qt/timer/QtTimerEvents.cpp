#include "QtTimerEvents.hpp"
#include "../../base/Client.hpp"

QtTimerEvents::QtTimerEvents(const int &ms, const int &offset, const unsigned char &event, const unsigned char &value) :
    event(event),
    value(value)
{
    connect(&this->timer,&QTimer::timeout,this,&QtTimerEvents::timerFinish);
    if(ms==0)
        this->timer.start(offset);
    else
    {
        connect(&this->offset,&QTimer::timeout,this,&QtTimerEvents::offsetFinish);
        this->timer.setInterval(offset);
        this->offset.start(ms);
    }
}

void QtTimerEvents::offsetFinish()
{
    timer.start();
    CatchChallenger::Client::setEvent(event,value);
}

void QtTimerEvents::timerFinish()
{
    CatchChallenger::Client::setEvent(event,value);
}
