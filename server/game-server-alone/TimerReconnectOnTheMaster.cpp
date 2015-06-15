#include "TimerReconnectOnTheMaster.h"
#include "LoginLinkToMaster.h"

using namespace CatchChallenger;

TimerReconnectOnTheMaster::TimerReconnectOnTheMaster()
{
}

void TimerReconnectOnTheMaster::exec()
{
    LoginLinkToMaster::loginLinkToMaster->timeoutTryAsyncReconnect();
}
