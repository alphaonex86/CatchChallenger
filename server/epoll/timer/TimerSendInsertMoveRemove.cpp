#include "TimerSendInsertMoveRemove.hpp"
#include "../../base/MapManagement/MapVisibilityAlgorithm_WithoutSender.hpp"

#include <iostream>

TimerSendInsertMoveRemove::TimerSendInsertMoveRemove()
{
}

void TimerSendInsertMoveRemove::exec()
{
    CatchChallenger::MapVisibilityAlgorithm_WithoutSender::mapVisibilityAlgorithm_WithoutSender.generalPurgeBuffer();
}
