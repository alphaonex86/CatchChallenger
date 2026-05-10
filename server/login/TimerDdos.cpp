#include "TimerDdos.hpp"
#include "EventLoopClientLoginSlave.hpp"

TimerDdos::TimerDdos()
{
    //todo start it
}

void TimerDdos::exec()
{
    CatchChallenger::EventLoopClientLoginSlave::doDDOSComputeAll();
}
