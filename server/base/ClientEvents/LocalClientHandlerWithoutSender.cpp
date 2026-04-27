#include "LocalClientHandlerWithoutSender.hpp"
#include "../MapManagement/MapVisibilityAlgorithm.hpp"
#include "../Client.hpp"
#include "../ClientList.hpp"
#include "../GlobalServerData.hpp"

using namespace CatchChallenger;

LocalClientHandlerWithoutSender LocalClientHandlerWithoutSender::localClientHandlerWithoutSender;

LocalClientHandlerWithoutSender::LocalClientHandlerWithoutSender()
{
}

LocalClientHandlerWithoutSender::~LocalClientHandlerWithoutSender()
{
}

void LocalClientHandlerWithoutSender::doAllAction()
{
    if(GlobalServerData::serverSettings.secondToPositionSync>0)
    {
        PLAYER_INDEX_FOR_CONNECTED index=0;
        while(index<ClientList::list->size())
        {
            if(!ClientList::list->isNull(index))
            {
                Client &c=ClientList::list->rw(index);
                c.savePosition();
            }
            index++;
        }
    }
}

void LocalClientHandlerWithoutSender::doDDOSChat()
{
    {
        CATCHCHALLENGER_TYPE_MAPID index=0;
        while(index<MapVisibilityAlgorithm::flat_map_list.size())
        {
            MapVisibilityAlgorithm::flat_map_list[index].doDDOSLocalChat();
            index++;
        }
    }
    #ifdef CATCHCHALLENGER_DDOS_FILTER
    {
        PLAYER_INDEX_FOR_CONNECTED index=0;
        while(index<ClientList::list->size())
        {
            if(!ClientList::list->isNull(index))
            {
                Client &c=ClientList::list->rw(index);
                c.doDDOSCompute();
            }
            index++;
        }
    }
    #endif
}
