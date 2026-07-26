#ifndef CATCHCHALLENGER_TimerReconnectOnTheMaster_LOGIN_H
#define CATCHCHALLENGER_TimerReconnectOnTheMaster_LOGIN_H

#ifdef CATCHCHALLENGER_SERVER

#include "../cli/EventLoopTimer.hpp"

namespace CatchChallenger {
//Single-shot retry of the link to the master, armed by LinkToMaster::tryReconnect()
//when a connect attempt fails. Same class as the game server node has, so both
//cluster nodes retry the same way: one attempt per timer tick, event loop free in
//between (the login slave keeps serving its clients while the master is down).
class TimerReconnectOnTheMaster
        : public EventLoopTimer
{
public:
    explicit TimerReconnectOnTheMaster();
private:
    void exec();
};
}

#endif // def CATCHCHALLENGER_SERVER
#endif // TimerReconnectOnTheMaster_LOGIN_H
