#include "TimerPositionSync.h"
#include "../Epoll.h"

#include "../../base/LocalClientHandlerWithoutSender.h"

#include <iostream>

TimerPositionSync::TimerPositionSync()
{
}

void TimerPositionSync::exec()
{
    CatchChallenger::LocalClientHandlerWithoutSender::localClientHandlerWithoutSender.doAllAction();
    EpollTimer::exec();
}
