#include "TimerDdos.h"
#include "../Epoll.h"

#include "../../base/LocalClientHandlerWithoutSender.h"
#include "../../base/BroadCastWithoutSender.h"
#include "../../base/ClientNetworkReadWithoutSender.h"

#include <iostream>

TimerDdos::TimerDdos()
{
}

void TimerDdos::exec()
{
    CatchChallenger::LocalClientHandlerWithoutSender::localClientHandlerWithoutSender.doDDOSAction();
    CatchChallenger::BroadCastWithoutSender::broadCastWithoutSender.doDDOSAction();
    CatchChallenger::ClientNetworkReadWithoutSender::clientNetworkReadWithoutSender.doDDOSAction();
    EpollTimer::exec();
}
