#ifdef CATCHCHALLENGER_CLASS_ALLINONESERVER
#include "TimeRangeEvent.h"
#include "../base/Client.h"

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
