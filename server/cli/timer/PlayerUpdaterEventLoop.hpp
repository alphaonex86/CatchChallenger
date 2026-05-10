#ifndef CATCHCHALLENGER_PLAYERUPDATEREPOLL_H
#define CATCHCHALLENGER_PLAYERUPDATEREPOLL_H

#include "../EventLoopTimer.hpp"
#include "../../base/PlayerUpdaterBase.hpp"
#include <stdint.h>

class PlayerUpdaterEventLoop : public EventLoopTimer, public CatchChallenger::PlayerUpdaterBase
{
    friend class PlayerUpdaterToMaster;
public:
    explicit PlayerUpdaterEventLoop();
private:
    void exec();
    void setInterval(int ms);
};

#endif // PLAYERUPDATER_H
