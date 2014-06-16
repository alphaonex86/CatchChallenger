#include "ClientNetworkReadWithoutSender.h"
#include "Client.h"

using namespace CatchChallenger;

ClientNetworkReadWithoutSender ClientNetworkReadWithoutSender::clientNetworkReadWithoutSender;

void ClientNetworkReadWithoutSender::doDDOSAction()
{
    const int &size=Client::clientBroadCastList.size();
    int index=0;
    while(index<size)
    {
        Client::clientBroadCastList.at(index)->doDDOSCompute();
        index++;
    }
}
