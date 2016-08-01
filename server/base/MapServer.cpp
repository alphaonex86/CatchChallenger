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

void MapServer::doDDOSLocalChat()
{
    int index=0;
    localChatDropTotalCache-=localChatDrop[index];
    while(index<(int)(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1))
    {
        localChatDrop[index]=localChatDrop[index+1];
        index++;
    }
    localChatDrop[(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1)]=localChatDropNewValue;
    localChatDropTotalCache+=localChatDropNewValue;
    localChatDropNewValue=0;
}
