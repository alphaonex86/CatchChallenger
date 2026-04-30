#ifndef CATCHCHALLENGER_PurgeTheLockedAccount_H
#define CATCHCHALLENGER_PurgeTheLockedAccount_H

#ifdef CATCHCHALLENGER_SERVER

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

#endif // def CATCHCHALLENGER_SERVER
#endif // PurgeTheLockedAccount_H
