#include "TimeRangeEvent.hpp"
#include "EventLoopClientLoginMaster.hpp"

using namespace CatchChallenger;

TimeRangeEvent::TimeRangeEvent()
{
    setInterval(1000*60*60*24);
}

void TimeRangeEvent::exec()
{
    EventLoopClientLoginMaster::sendTimeRangeEvent();
}
