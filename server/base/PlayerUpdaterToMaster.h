#ifndef CATCHCHALLENGER_PLAYERUPDATERTOMASTER_H
#define CATCHCHALLENGER_PLAYERUPDATERTOMASTER_H

#ifdef EPOLLCATCHCHALLENGERSERVER

#include "../epoll/EpollTimer.h"

namespace CatchChallenger {
class PlayerUpdaterToMaster
        : public EpollTimer
{
public:
    explicit PlayerUpdaterToMaster();
private:
    void exec();
private:
    unsigned short int sended_connected_players;
};
}

#endif // def EPOLLCATCHCHALLENGERSERVER
#endif // PLAYERUPDATERTOMASTER_H
