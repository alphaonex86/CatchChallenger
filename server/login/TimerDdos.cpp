#include "TimerDdos.hpp"
#include "../epoll/Epoll.hpp"
#include "EpollClientLoginSlave.hpp"

TimerDdos::TimerDdos()
{
    //todo start it
}

void TimerDdos::exec()
{
    CatchChallenger::EpollClientLoginSlave::doDDOSComputeAll();
}
