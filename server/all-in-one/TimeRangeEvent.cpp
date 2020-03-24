#ifdef CATCHCHALLENGER_CLASS_ALLINONESERVER
#include "TimeRangeEvent.hpp"
#include "../base/Client.hpp"

using namespace CatchChallenger;

TimeRangeEvent::TimeRangeEvent()
{
    setInterval(1000*60*60*24);
}

void TimeRangeEvent::exec()
{
    Client::timeRangeEvent(msFrom1970()/1000);
}
#endif
