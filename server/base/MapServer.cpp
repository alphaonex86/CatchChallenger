#include "MapServer.h"
#include "GlobalServerData.h"
#include "../VariableServer.h"

using namespace CatchChallenger;

MapServer::MapServer()
{
    int index=0;
    while(index<CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
    {
        localChatDrop << 0;
        index++;
    }
    localChatDropTotalCache=0;
    localChatDropNewValue=0;
}

void MapServer::doDDOSCompute()
{
    if(localChatDrop.size()==CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
    {
        localChatDropTotalCache=0;
        int index=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue;
        while(index<(int)(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1))
        {
            localChatDrop[index]=localChatDrop[index+1];
            localChatDropTotalCache+=localChatDrop[index];
            index++;
        }
        localChatDrop[(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1)]=localChatDropNewValue;
        localChatDropTotalCache+=localChatDropNewValue;
        localChatDropNewValue=0;
    }
}
