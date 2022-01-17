#include "TimeRangeEventScan.hpp"
#include "../../base/GlobalServerData.hpp"
#include "../../base/BroadCastWithoutSender.hpp"

TimeRangeEventScan::TimeRangeEventScan()
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    connect(this,&TimeRangeEventScan::try_initAll,                  this,&TimeRangeEventScan::initAll,              Qt::QueuedConnection);
    /*emit */try_initAll();
    #else
    initAll();
    #endif
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
