#include "QtTimerEvents.h"
#include "Client.h"

QtTimerEvents::QtTimerEvents(const int &offset,const int &timer,const unsigned char &event,const unsigned char &value) :
    event(event),
    value(value)
{
    connect(&this->timer,&QTimer::timeout,this,&QtTimerEvents::timerFinish);
	if(offset==0)
        this->timer.start(timer);
    else
    {
        connect(&this->offset,&QTimer::timeout,this,&QtTimerEvents::offsetFinish);
        this->timer.setInterval(timer);
        this->offset.start(offset);
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
