#include "TimerEvents.h"
#include "../../base/Client.h"

TimerEvents::TimerEvents(const unsigned char &event,const unsigned char &value) :
    event(event),
    value(value)
{
}

void TimerEvents::exec()
{
    CatchChallenger::Client::setEvent(event,value);
}
