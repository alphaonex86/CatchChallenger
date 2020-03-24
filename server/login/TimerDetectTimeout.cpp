#include "TimerDetectTimeout.hpp"
#include "LinkToMaster.hpp"

TimerDetectTimeout::TimerDetectTimeout()
{
    //todo start it
}

void TimerDetectTimeout::exec()
{
    CatchChallenger::LinkToMaster::linkToMaster->detectTimeout();
}
