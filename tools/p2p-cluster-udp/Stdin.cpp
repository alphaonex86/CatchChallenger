#include "Stdin.h"
#include "P2PServerUDP.h"

using namespace CatchChallenger;

Stdin::Stdin()
{
}

void Stdin::input(const char *input,unsigned int size)
{
    for( const auto& n : P2PServerUDP::p2pserver->hostConnectionEstablished ) {
        P2PPeer * peer=n.second;
        peer->sendData(reinterpret_cast<const uint8_t *>(input),size);
    }
}
