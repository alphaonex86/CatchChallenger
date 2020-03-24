#ifndef CATCHCHALLENGER_PurgeTheLockedAccount_H
#define CATCHCHALLENGER_PurgeTheLockedAccount_H

#ifdef EPOLLCATCHCHALLENGERSERVER

#include "../epoll/EpollTimer.hpp"

namespace CatchChallenger {
class PurgeTheLockedAccount
        : public EpollTimer
{
public:
    explicit PurgeTheLockedAccount(unsigned int intervalInSeconds);
private:
    void exec();
};
}

#endif // def EPOLLCATCHCHALLENGERSERVER
#endif // PurgeTheLockedAccount_H
