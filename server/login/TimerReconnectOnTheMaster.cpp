#include "TimerReconnectOnTheMaster.hpp"
#include "LinkToMaster.hpp"

using namespace CatchChallenger;

TimerReconnectOnTheMaster::TimerReconnectOnTheMaster()
{
}

void TimerReconnectOnTheMaster::exec()
{
    //Retry the link to the master after it went down (restart/maintenance). The
    //epoll loop reports the broken link ONCE (main-unix-login-slave.cpp, MasterLink
    //case), so this timer is what keeps retrying afterwards - without blocking the
    //loop, so the login slave keeps answering its clients meanwhile.
    LinkToMaster * const link=LinkToMaster::linkToMaster;
    if(link!=NULL)
    {
        //Only when the link is really down: tryReconnect() rebuilds the whole
        //session (query-number pool, max ids, profile list), so calling it on a
        //healthy or in-flight link would drop a working master connection.
        if(link->stat==LinkToMaster::Stat::Unconnected)
            link->tryReconnect();
    }
}
