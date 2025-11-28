#include "ClientWithMap.hpp"

using namespace CatchChallenger;

ClientWithMap::ClientWithMap(const uint16_t &index_connected_player) :
    Client(index_connected_player),
    sendedMap(65535)
{
}
