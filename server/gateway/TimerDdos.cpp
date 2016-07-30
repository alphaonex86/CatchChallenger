#include "TimerDdos.h"
#include "../epoll/Epoll.h"
#include "EpollClientLoginSlave.h"

TimerDdos::TimerDdos()
{
}

void TimerDdos::exec()
{
    CatchChallenger::EpollClientLoginSlave::doDDOSComputeAll();
}
