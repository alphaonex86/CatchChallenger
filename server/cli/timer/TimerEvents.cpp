#include "TimerEvents.hpp"
#include "../../base/Client.hpp"

TimerEvents::TimerEvents(const unsigned char &event,const unsigned char &value) :
    event(event),
    value(value)
{
}

void TimerEvents::exec()
{
    CatchChallenger::Client::setEvent(event,value);
}
