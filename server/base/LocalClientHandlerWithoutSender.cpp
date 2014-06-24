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
    if(GlobalServerData::serverSettings.database.secondToPositionSync>0)
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
    while(index<GlobalServerData::serverPrivateVariables.flat_map_list.size())
    {
        static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.flat_map_list.at(index))->doDDOSCompute();
        index++;
    }
}
