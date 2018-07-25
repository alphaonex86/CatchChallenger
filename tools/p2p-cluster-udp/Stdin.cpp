#include "Stdin.h"
#include "P2PServerUDP.h"
#include <iostream>

using namespace CatchChallenger;

Stdin::Stdin()
{
}

void Stdin::input(const char *input,unsigned int size)
{
    if(P2PServerUDP::p2pserver->hostConnectionEstablished.empty())
        std::cerr << "Stdin::input(), \e[1m\e[91mno peer, error\e[0m" << std::endl;
    else
        std::cout << "Stdin::input()" << std::endl;
    for( const auto& n : P2PServerUDP::p2pserver->hostConnectionEstablished ) {
        P2PPeer * peer=n.second;
        if(!peer->sendData(reinterpret_cast<const uint8_t *>(input),size))
            std::cerr << "Stdin::input(), send error" << std::endl;
    }
}
