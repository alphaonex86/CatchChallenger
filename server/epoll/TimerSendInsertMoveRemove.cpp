#include "TimerSendInsertMoveRemove.h"
#include "Epoll.h"

#include <iostream>

TimerSendInsertMoveRemove::TimerSendInsertMoveRemove()
{
}

void TimerSendInsertMoveRemove::exec()
{
    EpollTimer::exec();
}
