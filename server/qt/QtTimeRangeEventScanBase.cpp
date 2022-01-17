#include "QtTimeRangeEventScanBase.hpp"
#include "GlobalServerData.hpp"
#include "Client.hpp"

#ifdef EPOLLCATCHCHALLENGERSERVER
#include "BroadCastWithoutSender.hpp"
#endif

using namespace CatchChallenger;

TimeRangeEventScanQt::TimeRangeEventScan()
      #ifndef EPOLLCATCHCHALLENGERSERVER
      :
        next_send_timer(NULL)
      #endif
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    connect(this,&TimeRangeEventScanQt::try_initAll,                  this,&TimeRangeEventScanQt::initAll,              Qt::QueuedConnection);
    /*emit */try_initAll();
    #else
    initAll();
    #endif
}

void TimeRangeEventScanQt::initAll()
{
    //10 clients per 15s = 40 clients per min = 57600 clients per 24h
    //1ms time used each 15000ms
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(next_send_timer==NULL)
    {
        next_send_timer=new QTimer();
        next_send_timer->setInterval(1000*15);

        connect(next_send_timer,&QTimer::timeout,this,&TimeRangeEventScanQt::exec,Qt::QueuedConnection);
    }
    #else
    setInterval(1000*15);
    #endif
}


void TimeRangeEventScanQt::exec()
{
    if(GlobalServerData::serverPrivateVariables.gift_list.size()==0)
        return;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    /*emit */timeRangeEventTrigger();
    #else
    BroadCastWithoutSender::broadCastWithoutSender.timeRangeEventTrigger();
    #endif
}
