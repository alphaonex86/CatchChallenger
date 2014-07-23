#include "MapServer.h"
#include "GlobalServerData.h"
#include "../VariableServer.h"

using namespace CatchChallenger;

MapServer::MapServer() :
    localChatDropTotalCache(0),
    localChatDropNewValue(0),
    reverse_db_id(0)
{
    memset(localChatDrop,0x00,CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE);
}

void MapServer::doDDOSCompute()
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
