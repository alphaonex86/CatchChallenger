#include "TimerSendInsertMoveRemove.h"
#include "Epoll.h"

#include "../base/ClientMapManagement/MapVisibilityAlgorithm_WithoutSender.h"

#include <iostream>

TimerSendInsertMoveRemove::TimerSendInsertMoveRemove()
{
}

void TimerSendInsertMoveRemove::exec()
{
    CatchChallenger::MapVisibilityAlgorithm_WithoutSender::mapVisibilityAlgorithm_WithoutSender.generalPurgeBuffer();
    EpollTimer::exec();
}
