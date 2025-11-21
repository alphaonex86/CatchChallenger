#include "MapServer.hpp"
#include "GlobalServerData.hpp"
#include "VariableServer.hpp"
#include "Client.hpp"
#include <string.h>
#ifdef CATCHCHALLENGER_EXTRA_CHECK
#include <vector>
#endif
#include "../../general/base/CommonSettingsServer.hpp"

using namespace CatchChallenger;

MapServer::MapServer() :
    localChatDropTotalCache(0),
    localChatDropNewValue(0),
    id_db(0),
    zone(255)
{
    border.bottom.x_offset=0;
    border.bottom.mapIndex=65535;
    border.top.x_offset=0;
    border.top.mapIndex=65535;
    border.right.y_offset=0;
    border.right.mapIndex=65535;
    border.left.y_offset=0;
    border.left.mapIndex=65535;

    localChatDropTotalCache=0;
    localChatDropNewValue=0;
    id_db=0;
    zone=0;
    memset(localChatDrop,0x00,CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE);
}

//return index into map list
SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED MapServer::insertOnMap(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index_global)
{
    if(!map_removed_index.empty())
    {
        SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED b=map_removed_index.back();
        map_removed_index.pop_back();
        map_clients_id[b]=index_global;
        return b;
    }
    else
    {
        SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED b=map_clients_id.size();
        map_clients_id.resize(b+1);
        map_clients_id[b]=index_global;
        return b;
    }
}

void MapServer::removeOnMap(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index_map)
{
    map_clients_id[index_map]=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX;
    map_removed_index.push_back(index_map);
}

SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED MapServer::map_clients_list_size() const
{
    return map_clients_id.size();
}

bool MapServer::map_clients_list_isValid(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index) const
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(index>=map_clients_id.size())
    {
        std::cerr << "MapServer::map_clients_list_isValid out of range: " << index << "/" << map_clients_id.size() << std::endl;
        abort();
    }
    #endif
    return map_clients_id.at(index)!=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX;
}

//abort if index is not valid
const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &MapServer::map_clients_list_at(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index) const
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(index>=map_clients_id.size())
    {
        std::cerr << "MapServer::map_clients_list_isValid out of range: " << index << "/" << map_clients_id.size() << std::endl;
        abort();
    }
    if(map_clients_id.at(index)==SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
    {
        std::cerr << "MapServer::map_clients_list_isValid wrong value: " << index << std::endl;
        abort();
    }
    #endif
    return map_clients_id.at(index);
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

unsigned int MapServer::playerToFullInsert(const Client& client, char * const bufferForOutput)
{
    unsigned int posOutput=0;
    bufferForOutput[posOutput]=client.getX();
    posOutput+=1;
    bufferForOutput[posOutput]=client.getY();
    posOutput+=1;
    if(GlobalServerData::serverSettings.dontSendPlayerType)
        bufferForOutput[posOutput]=((uint8_t)client.getLastDirection() | (uint8_t)Player_type_normal);
    else
        bufferForOutput[posOutput]=((uint8_t)client.getLastDirection() | (uint8_t)client.public_and_private_informations.public_informations.type);
    posOutput+=1;
    //pseudo
    if(!CommonSettingsServer::commonSettingsServer.dontSendPseudo)
    {
        const std::string &text=client.public_and_private_informations.public_informations.pseudo;
        bufferForOutput[posOutput]=static_cast<uint8_t>(text.size());
        posOutput+=1;
        memcpy(bufferForOutput+posOutput,text.data(),text.size());
        posOutput+=text.size();
    }
    //skin
    bufferForOutput[posOutput]=client.public_and_private_informations.public_informations.skinId;
    posOutput+=1;
    //the following monster id to show
    if(client.public_and_private_informations.playerMonster.empty())
        *reinterpret_cast<uint16_t *>(bufferForOutput+posOutput)=0;
    else
        *reinterpret_cast<uint16_t *>(bufferForOutput+posOutput)=htole16(client.public_and_private_informations.playerMonster.front().monster);
    posOutput+=2;
    return posOutput;
}

bool MapServer::parseUnknownMoving(std::string type,uint32_t object_x,uint32_t object_y,std::unordered_map<std::string,std::string> property_text)
{
    (void)property_text;
    if(type=="rescue")
    {
        std::pair<uint8_t,uint8_t> p(object_x,object_y);
        rescue[p]=Orientation_bottom;
        return true;
    }
    return false;
}

bool MapServer::parseUnknownObject(std::string type,uint32_t object_x,uint32_t object_y,std::unordered_map<std::string,std::string> property_text)
{
    (void)type;
    (void)object_x;
    (void)object_y;
    (void)property_text;
    return false;
}

bool MapServer::parseUnknownBotStep(uint32_t object_x,uint32_t object_y,const tinyxml2::XMLElement *step)
{
    (void)object_x;
    (void)object_y;
    (void)step;
    if(strcmp(step->Attribute("type"),"heal")==0)
    {
        heal.insert(std::pair<uint8_t,uint8_t>(object_x,object_y));
        return true;
    }
    if(strcmp(step->Attribute("type"),"zonecapture")==0)
    {
        if(step->Attribute("zone")==NULL)
            std::cerr << "zonecapture point have not the zone attribute: for bot id: " << object_x << "," << object_y << std::endl;
        else if(zoneCapture.find(std::pair<uint8_t,uint8_t>(object_x,object_y))!=zoneCapture.cend())
            std::cerr << "zonecapture point already on the map: for bot id: " << object_x << "," << object_y << std::endl;
        else
            zoneCapture.insert(std::pair<uint8_t,uint8_t>(object_x,object_y));
        return true;
    }
    return false;
}
