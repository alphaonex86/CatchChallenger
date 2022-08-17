#ifndef CATCHCHALLENGER_BROADCASTWITHOUTSENDER_H
#define CATCHCHALLENGER_BROADCASTWITHOUTSENDER_H

#include <cstdint>

namespace CatchChallenger {
class BroadCastWithoutSender
{
private:
    explicit BroadCastWithoutSender();
    static unsigned char bufferSendPlayer[3];
public:
    static BroadCastWithoutSender broadCastWithoutSender;
public:
    void receive_instant_player_number(const int16_t &connected_players);
    void timeRangeEventTrigger();
    void doDDOSChat();
};
}

#endif // BROADCASTWITHOUTSENDER_H
