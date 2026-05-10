#ifndef CATCHCHALLENGER_PurgeTheLockedAccount_H
#define CATCHCHALLENGER_PurgeTheLockedAccount_H

#ifdef CATCHCHALLENGER_SERVER

#include "../cli/EventLoopTimer.hpp"

namespace CatchChallenger {
class PurgeTheLockedAccount
        : public EventLoopTimer
{
public:
    explicit PurgeTheLockedAccount(unsigned int intervalInSeconds);
private:
    void exec();
};
}

#endif // def CATCHCHALLENGER_SERVER
#endif // PurgeTheLockedAccount_H
