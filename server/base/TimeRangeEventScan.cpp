#include "TimeRangeEventScan.h"
#include "GlobalServerData.h"
#include "Client.h"

#ifdef EPOLLCATCHCHALLENGERSERVER
#include "BroadCastWithoutSender.h"
#endif

using namespace CatchChallenger;

TimeRangeEventScan::TimeRangeEventScan()
      #ifndef EPOLLCATCHCHALLENGERSERVER
      :
        next_send_timer(NULL)
      #endif
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
    //10 clients per 15s = 40 clients per min = 57600 clients per 24h
    //1ms time used each 15000ms
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(next_send_timer==NULL)
    {
        next_send_timer=new QTimer();
        next_send_timer->setInterval(1000*15);

        connect(next_send_timer,&QTimer::timeout,this,&TimeRangeEventScan::exec,Qt::QueuedConnection);
    }
    #else
    setInterval(1000*15);
    #endif
}


void TimeRangeEventScan::exec()
{
    if(GlobalServerData::serverPrivateVariables.gift_list.size()==0)
        return;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    /*emit */timeRangeEventTrigger();
    #else
    BroadCastWithoutSender::broadCastWithoutSender.timeRangeEventTrigger();
    #endif
}
