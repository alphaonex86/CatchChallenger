#include "ClientNetworkReadWithoutSender.hpp"
#include "GlobalServerData.hpp"
#include "Client.hpp"

using namespace CatchChallenger;

ClientNetworkReadWithoutSender ClientNetworkReadWithoutSender::clientNetworkReadWithoutSender;

#ifdef CATCHCHALLENGER_DDOS_FILTER
void ClientNetworkReadWithoutSender::doDDOSAction()
{
    if(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE>0)
    {
        const size_t &size=Client::clientBroadCastList.size();
        unsigned int index=0;
        while(index<size)
        {
            Client::clientBroadCastList.at(index)->doDDOSCompute();
            index++;
        }
    }
}
#endif
