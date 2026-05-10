#include "TimerDdos.hpp"
#include "EventLoopClientLoginSlave.hpp"

TimerDdos::TimerDdos()
{
}

void TimerDdos::exec()
{
    CatchChallenger::EventLoopClientLoginSlave::doDDOSComputeAll();
}
