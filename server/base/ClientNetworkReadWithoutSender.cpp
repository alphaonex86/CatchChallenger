#include "ClientNetworkReadWithoutSender.hpp"
#include "ClientList.hpp"
#include "Client.hpp"

using namespace CatchChallenger;

ClientNetworkReadWithoutSender ClientNetworkReadWithoutSender::clientNetworkReadWithoutSender;

#ifdef CATCHCHALLENGER_DDOS_FILTER
void ClientNetworkReadWithoutSender::doDDOSAction()
{
    if(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE>0)
    {
        const size_t &size=ClientList::list->size();
        unsigned int index=0;
        while(index<size)
        {
            if(!ClientList::list->empty(index))
                ClientList::list->rw(index).doDDOSCompute();
            index++;
        }
    }
}
#endif
