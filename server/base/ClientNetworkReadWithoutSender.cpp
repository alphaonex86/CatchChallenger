#include "ClientNetworkReadWithoutSender.h"
#include "ClientNetworkRead.h"

using namespace CatchChallenger;

ClientNetworkReadWithoutSender ClientNetworkReadWithoutSender::clientNetworkReadWithoutSender;

void ClientNetworkReadWithoutSender::doDDOSAction()
{
    int index=0;
    while(index<clientList.size())
    {
        clientList.at(index)->doDDOSCompute();
        index++;
    }
}
