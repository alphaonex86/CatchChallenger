#include "TimerSendInsertMoveRemove.hpp"
#include "../Epoll.hpp"

#include "../../base/ClientMapManagement/MapVisibilityAlgorithm_WithoutSender.hpp"

#include <iostream>

TimerSendInsertMoveRemove::TimerSendInsertMoveRemove()
{
}

void TimerSendInsertMoveRemove::exec()
{
    CatchChallenger::MapVisibilityAlgorithm_WithoutSender::mapVisibilityAlgorithm_WithoutSender.generalPurgeBuffer();
}
