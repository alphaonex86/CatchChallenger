#include "TimerDdos.hpp"
#include "EpollClientLoginSlave.hpp"

TimerDdos::TimerDdos()
{
}

void TimerDdos::exec()
{
    CatchChallenger::EpollClientLoginSlave::doDDOSComputeAll();
}
