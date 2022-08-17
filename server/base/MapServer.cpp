#include "MapServer.hpp"
#include "GlobalServerData.hpp"
#include "VariableServer.hpp"
#include "Client.hpp"
#include "../../general/base/CommonSettingsServer.hpp"

using namespace CatchChallenger;

#ifdef CATCHCHALLENGER_CACHE_HPS
uint32_t MapServer::mapListSize=0;
#endif

MapServer::MapServer() :
    localChatDropTotalCache(0),
    localChatDropNewValue(0),
    reverse_db_id(0),
    zone(65535)
{
    border.bottom.x_offset=0;
    border.bottom.map=nullptr;
    border.top.x_offset=0;
    border.top.map=nullptr;
    border.right.y_offset=0;
    border.right.map=nullptr;
    border.left.y_offset=0;
    border.left.map=nullptr;
    memset(localChatDrop,0x00,CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE);
}

void MapServer::doDDOSLocalChat()
{
    /*int index=0;
    localChatDropTotalCache-=localChatDrop[index];*/
    localChatDropTotalCache-=localChatDrop[0];
    memmove(localChatDrop,localChatDrop+1,sizeof(localChatDrop)-1);
/*    while(index<(int)(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1))
    {
        localChatDrop[index]=localChatDrop[index+1];
        index++;
    }*/
    localChatDrop[(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1)]=localChatDropNewValue;
    localChatDropTotalCache+=localChatDropNewValue;
    localChatDropNewValue=0;
}

unsigned int MapServer::playerToFullInsert(const Client * const client, char * const bufferForOutput)
{
    unsigned int posOutput=0;
    if(GlobalServerData::serverSettings.max_players<=255)
    {
        bufferForOutput[posOutput]=
                static_cast<uint8_t>(client->public_and_private_informations.public_informations.simplifiedId);
        posOutput+=1;
    }
    else
    {
        *reinterpret_cast<uint16_t *>(bufferForOutput+posOutput)=htole16(client->public_and_private_informations.public_informations.simplifiedId);
        posOutput+=2;
    }
    bufferForOutput[posOutput]=client->getX();
    posOutput+=1;
    bufferForOutput[posOutput]=client->getY();
    posOutput+=1;
    if(GlobalServerData::serverSettings.dontSendPlayerType)
        bufferForOutput[posOutput]=((uint8_t)client->getLastDirection() | (uint8_t)Player_type_normal);
    else
        bufferForOutput[posOutput]=((uint8_t)client->getLastDirection() | (uint8_t)client->public_and_private_informations.public_informations.type);
    posOutput+=1;
    if(CommonSettingsServer::commonSettingsServer.forcedSpeed==0)
    {
        bufferForOutput[posOutput]=client->public_and_private_informations.public_informations.speed;
        posOutput+=1;
    }
    //pseudo
    if(!CommonSettingsServer::commonSettingsServer.dontSendPseudo)
    {
        const std::string &text=client->public_and_private_informations.public_informations.pseudo;
        bufferForOutput[posOutput]=static_cast<uint8_t>(text.size());
        posOutput+=1;
        memcpy(bufferForOutput+posOutput,text.data(),text.size());
        posOutput+=text.size();
    }
    //skin
    bufferForOutput[posOutput]=client->public_and_private_informations.public_informations.skinId;
    posOutput+=1;
    //the following monster id to show
    if(client->public_and_private_informations.playerMonster.empty())
        *reinterpret_cast<uint16_t *>(bufferForOutput+posOutput)=0;
    else
        *reinterpret_cast<uint16_t *>(bufferForOutput+posOutput)=htole16(client->public_and_private_informations.playerMonster.front().monster);
    posOutput+=2;
    return posOutput;
}

#ifdef CATCHCHALLENGER_CACHE_HPS
CommonMap * MapServer::posToPointer(const int32_t &pos)
{
    if(pos==-1)
        return nullptr;
    if((uint32_t)pos<mapListSize)
        return GlobalServerData::serverPrivateVariables.flat_map_list[pos];
    else
    {
        std::cerr << pos << " not into list: ";
        size_t index=0;
        while(index<mapListSize)
        {
            if(index>0)
                std::cerr << ", ";
            std::cerr << GlobalServerData::serverPrivateVariables.flat_map_list[index]->id;
            index++;
        }
        std::cerr << " (" << mapListSize << ")";
        abort();
    }
    return nullptr;
}
#endif
