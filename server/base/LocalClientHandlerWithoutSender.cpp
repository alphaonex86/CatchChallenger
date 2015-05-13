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
        int index=0;
        const int &list_size=allClient.size();
        while(index<list_size)
        {
            static_cast<Client *>(allClient.at(index))->savePosition();
            index++;
        }
    }
}

void LocalClientHandlerWithoutSender::doDDOSAction()
{
    int index=0;
    const int &map_list_size=GlobalServerData::serverPrivateVariables.map_list.size();
    while(index<map_list_size)
    {
        static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.flat_map_list[index])->doDDOSCompute();
        index++;
    }
}
