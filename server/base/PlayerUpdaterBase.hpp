#ifndef CATCHCHALLENGER_PLAYERUPDATER_H
#define CATCHCHALLENGER_PLAYERUPDATER_H

#include <stdint.h>

namespace CatchChallenger {
class PlayerUpdaterBase
{
    friend class PlayerUpdaterToMaster;
public:
    explicit PlayerUpdaterBase();
public:
    void addConnectedPlayer();
    void removeConnectedPlayer();
protected:
    void internal_addConnectedPlayer();
    void internal_removeConnectedPlayer();
    virtual void setInterval(int ms) = 0;
    virtual void exec();
protected:
    static uint16_t connected_players;
    static uint16_t sended_connected_players;
};
}

#endif // PLAYERUPDATER_H
