#include "TimerPositionSync.hpp"
#include "../../base/ClientEvents/LocalClientHandlerWithoutSender.hpp"
#include "../../base/GlobalServerData.hpp"

#include <iostream>

TimerPositionSync::TimerPositionSync()
{
}

void TimerPositionSync::exec()
{
    if(CatchChallenger::GlobalServerData::serverSettings.secondToPositionSync>0)
        CatchChallenger::LocalClientHandlerWithoutSender::localClientHandlerWithoutSender.doAllAction();
}
