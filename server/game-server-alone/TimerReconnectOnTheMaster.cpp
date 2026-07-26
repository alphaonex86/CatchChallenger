#include "TimerReconnectOnTheMaster.hpp"
#include "LinkToMaster.hpp"

using namespace CatchChallenger;

TimerReconnectOnTheMaster::TimerReconnectOnTheMaster()
{
}

void TimerReconnectOnTheMaster::exec()
{
    //Retry the link to the master after the master went down (restart/maintenance).
    //The epoll loop reports the broken link ONCE (main-unix.cpp, MasterLink case:
    //EPOLLERR/EPOLLHUP -> tryReconnect()); if that attempt gives up while the master
    //is still down, nothing wakes the node again and it stays master-less until an
    //operator restarts it. This timer is that second chance.
    //The old call was LoginLinkToMaster::loginLinkToMaster->timeoutTryAsyncReconnect():
    //neither the class nor the method has ever existed on this side (copy/paste from
    //the login node, 2015). The live singleton is LinkToMaster::linkToMaster - the
    //same object the epoll loop reconnects, and the same pattern the login node uses
    //in main-unix-login-slave.cpp.
    LinkToMaster * const link=LinkToMaster::linkToMaster;
    if(link!=NULL)
    {
        //Only when the link is really down: tryReconnect() rebuilds the whole session
        //(query-number pool, max ids, protocol header), so calling it on a healthy or
        //in-flight link would drop a working master connection.
        if(link->stat==LinkToMaster::Stat::Unconnected)
            link->tryReconnect();
    }
}
