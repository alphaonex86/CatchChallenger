#include "TimerPositionSync.hpp"

#include "../../base/LocalClientHandlerWithoutSender.hpp"

#include <iostream>

TimerPositionSync::TimerPositionSync()
{
}

void TimerPositionSync::exec()
{
    CatchChallenger::LocalClientHandlerWithoutSender::localClientHandlerWithoutSender.doAllAction();
}
