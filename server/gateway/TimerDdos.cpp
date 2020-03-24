#include "TimerDdos.hpp"
#include "../epoll/Epoll.hpp"
#include "EpollClientLoginSlave.hpp"

TimerDdos::TimerDdos()
{
}

void TimerDdos::exec()
{
    CatchChallenger::EpollClientLoginSlave::doDDOSComputeAll();
}
