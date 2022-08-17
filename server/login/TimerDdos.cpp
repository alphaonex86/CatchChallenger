#include "TimerDdos.hpp"
#include "EpollClientLoginSlave.hpp"

TimerDdos::TimerDdos()
{
    //todo start it
}

void TimerDdos::exec()
{
    CatchChallenger::EpollClientLoginSlave::doDDOSComputeAll();
}
