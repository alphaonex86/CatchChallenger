#ifndef CATCHCHALLENGER_PLAYERUPDATEREPOLL_H
#define CATCHCHALLENGER_PLAYERUPDATEREPOLL_H

#include "../EpollTimer.hpp"
#include "../../base/PlayerUpdaterBase.hpp"
#include <stdint.h>

class PlayerUpdaterEpoll : public EpollTimer, public CatchChallenger::PlayerUpdaterBase
{
    friend class PlayerUpdaterToMaster;
public:
    explicit PlayerUpdaterEpoll();
private:
    void exec();
    void setInterval(int ms);
};

#endif // PLAYERUPDATER_H
