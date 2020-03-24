#include "TimerPositionSync.hpp"
#include "../Epoll.hpp"

#include "../../base/LocalClientHandlerWithoutSender.hpp"

#include <iostream>

TimerPositionSync::TimerPositionSync()
{
}

void TimerPositionSync::exec()
{
    CatchChallenger::LocalClientHandlerWithoutSender::localClientHandlerWithoutSender.doAllAction();
}
