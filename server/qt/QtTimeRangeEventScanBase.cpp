#include "QtTimeRangeEventScanBase.hpp"
#include "../base/GlobalServerData.hpp"
#include "../base/BroadCastWithoutSender.hpp"

QtTimeRangeEventScan::QtTimeRangeEventScan()
      :
        next_send_timer(NULL)
{
    if(!connect(this,&QtTimeRangeEventScan::try_initAll,                  this,&QtTimeRangeEventScan::initAll,              Qt::QueuedConnection))
        abort();
    /*emit */try_initAll();
}

void QtTimeRangeEventScan::initAll()
{
    //10 clients per 15s = 40 clients per min = 57600 clients per 24h
    //1ms time used each 15000ms
    if(next_send_timer==NULL)
    {
        next_send_timer=new QTimer();
        next_send_timer->setInterval(1000*15);

        connect(next_send_timer,&QTimer::timeout,this,&QtTimeRangeEventScan::exec,Qt::QueuedConnection);
    }
}


void QtTimeRangeEventScan::exec()
{
    if(CatchChallenger::GlobalServerData::serverPrivateVariables.gift_list.size()==0)
        return;
    /*emit */timeRangeEventTrigger();
    CatchChallenger::BroadCastWithoutSender::broadCastWithoutSender.timeRangeEventTrigger();
}
