#include "TimerDetectTimeout.h"
#include "LinkToMaster.h"

TimerDetectTimeout::TimerDetectTimeout()
{
    //todo start it
}

void TimerDetectTimeout::exec()
{
    CatchChallenger::LinkToMaster::linkToMaster->detectTimeout();
}
