#ifndef CATCHCHALLENGER_CLIENTNETWORKREADWITHOUTSENDER_H
#define CATCHCHALLENGER_CLIENTNETWORKREADWITHOUTSENDER_H

#include "VariableServer.hpp"

namespace CatchChallenger {
class ClientNetworkRead;

class ClientNetworkReadWithoutSender
{
public:
    static ClientNetworkReadWithoutSender clientNetworkReadWithoutSender;
    #ifdef CATCHCHALLENGER_DDOS_FILTER
public:
    void doDDOSAction();
    #endif
};
}

#endif // CLIENTNETWORKREAD_H
