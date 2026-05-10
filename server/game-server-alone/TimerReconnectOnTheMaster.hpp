#ifndef CATCHCHALLENGER_TimerReconnectOnTheMaster_H
#define CATCHCHALLENGER_TimerReconnectOnTheMaster_H

#ifdef CATCHCHALLENGER_SERVER

#include "../cli/EventLoopTimer.hpp"

namespace CatchChallenger {
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
#endif // TimerReconnectOnTheMaster_H
