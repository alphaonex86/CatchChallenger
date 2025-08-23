#include "LocalClientHandlerWithoutSender.hpp"
#include "../MapManagement/Map_server_MapVisibility_Simple_StoreOnSender.hpp"
#include "../Client.hpp"
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
        unsigned int index=0;
        while(index<allClient.size())
        {
            static_cast<Client *>(allClient.at(index))->savePosition();
            index++;
        }
    }
}

void LocalClientHandlerWithoutSender::doDDOSChat()
{
    unsigned int index=0;
    while(index<Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.size())
    {
        Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list[index].doDDOSLocalChat();
        index++;
    }
}
