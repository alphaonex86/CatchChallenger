#ifndef CATCHCHALLENGER_PLAYERUPDATERTOLOGIN_H
#define CATCHCHALLENGER_PLAYERUPDATERTOLOGIN_H

#ifdef EPOLLCATCHCHALLENGERSERVER

#include "../epoll/EpollTimer.h"

namespace CatchChallenger {
class PlayerUpdaterToLogin
        : public EpollTimer
{
public:
    explicit PlayerUpdaterToLogin();
private:
    void exec();
};
}

#endif // def EPOLLCATCHCHALLENGERSERVER
#endif // PLAYERUPDATERTOLOGIN_H
