#include "TimerDdos.h"
#include "../epoll/Epoll.h"
#include "EpollClientLoginSlave.h"

TimerDdos::TimerDdos()
{
    //todo start it
}

void TimerDdos::exec()
{
    CatchChallenger::EpollClientLoginSlave::doDDOSComputeAll();
}
