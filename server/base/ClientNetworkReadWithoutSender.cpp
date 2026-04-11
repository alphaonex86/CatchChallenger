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
            //ClientList::list->empty() filters out everything that is not
            //CharacterSelected, but DdosBuffer::total()/incrementLastValue()
            //is already called from parseInputBeforeLogin() (i.e. when stat
            //is None/ProtocolGood/LoginInProgress/...). Those buffers must
            //therefore also be flushed periodically or DdosBuffer::total()
            //will eventually trip the ">300s without flush" guard.
            if(ClientList::list->rw(index).getClientStat()!=Client::ClientStat::Free)
                ClientList::list->rw(index).doDDOSCompute();
            index++;
        }
    }
}
#endif
