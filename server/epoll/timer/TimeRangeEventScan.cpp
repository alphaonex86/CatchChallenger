#include "TimeRangeEventScan.hpp"
#include "../../base/GlobalServerData.hpp"
#include "../../base/BroadCastWithoutSender.hpp"

TimeRangeEventScan::TimeRangeEventScan()
{
    initAll();
}

void TimeRangeEventScan::initAll()
{
    setInterval(1000*15);
}


void TimeRangeEventScan::exec()
{
    if(CatchChallenger::GlobalServerData::serverPrivateVariables.gift_list.size()==0)
        return;
    CatchChallenger::BroadCastWithoutSender::broadCastWithoutSender.timeRangeEventTrigger();
}
