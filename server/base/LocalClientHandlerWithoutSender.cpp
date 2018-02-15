#include "LocalClientHandlerWithoutSender.h"
#include "MapServer.h"
#include "Client.h"
#include "GlobalServerData.h"

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
    while(index<GlobalServerData::serverPrivateVariables.map_list.size())
    {
        static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.flat_map_list[index])->doDDOSLocalChat();
        index++;
    }
}
