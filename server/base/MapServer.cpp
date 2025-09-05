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
    zone(65535),
    map_clients_id(nullptr),
    map_removed_index(nullptr)
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

#ifdef CATCHCHALLENGER_EXTRA_CHECK
void MapServer::check6B(const char * const data,const unsigned int size)
{
    int pos=0;
    if((size-pos)<(unsigned int)sizeof(uint8_t))
    {
        std::cerr << "MapServer::checkB6(" << binarytoHexa(data,size) << ") error (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
        return;
    }
    std::vector<Player_public_informations> playerinsertList;
    uint8_t mapListSize=data[pos];
    pos+=sizeof(uint8_t);
    int index=0;
    while(index<mapListSize)
    {
        uint32_t mapId;
        if(GlobalServerData::serverPrivateVariables.map_list.size()==0)
        {
            std::cerr << "MapServer::checkB6(" << binarytoHexa(data,size) << ") error (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
            return;
        }
        else if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
        {
            if((size-pos)<(unsigned int)sizeof(uint8_t))
            {
                std::cerr << "MapServer::checkB6(" << binarytoHexa(data,size) << ") error (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
                abort();
                return;
            }
            uint8_t mapTempId=data[pos];
            pos+=sizeof(uint8_t);
            mapId=mapTempId;
        }
        else
        {
            if((size-pos)<(unsigned int)sizeof(uint16_t))
            {
                std::cerr << "MapServer::checkB6(" << binarytoHexa(data,size) << ") error (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
                abort();
                return;
            }
            uint16_t mapTempId=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
            pos+=sizeof(uint16_t);
            mapId=mapTempId;
        }
        (void)mapId;

        uint16_t playerSizeList;
        if(GlobalServerData::serverSettings.max_players<=255)
        {
            if((size-pos)<(unsigned int)sizeof(uint8_t))
            {
                std::cerr << "MapServer::checkB6(" << binarytoHexa(data,size) << ") error (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
                abort();
                return;
            }
            uint8_t numberOfPlayer=data[pos];
            pos+=sizeof(uint8_t);
            playerSizeList=numberOfPlayer;
        }
        else
        {
            if((size-pos)<(unsigned int)sizeof(uint16_t))
            {
                std::cerr << "MapServer::checkB6(" << binarytoHexa(data,size) << ") error (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
                abort();
                return;
            }
            playerSizeList=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
            pos+=sizeof(uint16_t);
        }
        if(playerSizeList>GlobalServerData::serverSettings.max_players)
        {
            std::cerr << "MapServer::checkB6(" << binarytoHexa(data,size) << ") error (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
            return;
        }
        int index_sub_loop=0;
        while(index_sub_loop<playerSizeList)
        {
            //player id
            Player_public_informations public_informations;
            if(GlobalServerData::serverSettings.max_players<=255)
            {
                if((size-pos)<(unsigned int)sizeof(uint8_t))
                {
                    std::cerr << "MapServer::checkB6(" << binarytoHexa(data,size) << ") error (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
                    abort();
                    return;
                }
                uint8_t playerSmallId=data[pos];
                pos+=sizeof(uint8_t);
                public_informations.simplifiedId=playerSmallId;
            }
            else
            {
                if((size-pos)<(unsigned int)sizeof(uint16_t))
                {
                    std::cerr << "MapServer::checkB6(" << binarytoHexa(data,size) << ") error (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
                    abort();
                    return;
                }
                public_informations.simplifiedId=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                pos+=sizeof(uint16_t);
            }

            //x and y
            if((size-pos)<(unsigned int)sizeof(uint8_t)*2)
            {
                std::cerr << "MapServer::checkB6(" << binarytoHexa(data,size) << ") error (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
                abort();
                return;
            }
            uint8_t x=data[pos];
            (void)x;
            pos+=sizeof(uint8_t);
            uint8_t y=data[pos];
            (void)y;
            pos+=sizeof(uint8_t);

            //direction and player type
            if((size-pos)<(unsigned int)sizeof(uint8_t))
            {
                std::cerr << "MapServer::checkB6(" << binarytoHexa(data,size) << ") error (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
                abort();
                return;
            }
            uint8_t directionAndPlayerType=data[pos];
            pos+=sizeof(uint8_t);
            uint8_t directionInt,playerTypeInt;
            directionInt=directionAndPlayerType & 0x0F;
            playerTypeInt=directionAndPlayerType & 0xF0;
            if(directionInt<1 || directionInt>8)
            {
                std::cerr << "MapServer::checkB6(" << binarytoHexa(data,size) << ") error (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
                abort();
                return;
            }
            Direction direction=(Direction)directionInt;
            (void)direction;
            Player_type playerType=(Player_type)playerTypeInt;
            if(playerType!=Player_type_normal && playerType!=Player_type_premium && playerType!=Player_type_gm && playerType!=Player_type_dev)
            {
                std::cerr << "MapServer::checkB6(" << binarytoHexa(data,size) << ") error (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
                abort();
                return;
            }
            public_informations.type=playerType;

            //the speed
            if(CommonSettingsServer::commonSettingsServer.forcedSpeed==0)
            {
                if((size-pos)<(unsigned int)sizeof(uint8_t))
                {
                    std::cerr << "MapServer::checkB6(" << binarytoHexa(data,size) << ") error (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
                    abort();
                    return;
                }
                public_informations.speed=data[pos];
                pos+=sizeof(uint8_t);
            }
            else
                public_informations.speed=CommonSettingsServer::commonSettingsServer.forcedSpeed;

            if(!CommonSettingsServer::commonSettingsServer.dontSendPseudo)
            {
                //the pseudo
                if((size-pos)<(unsigned int)sizeof(uint8_t))
                {
                    std::cerr << "MapServer::checkB6(" << binarytoHexa(data,size) << ") error (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
                    abort();
                    return;
                }
                uint8_t pseudoSize=data[pos];
                pos+=sizeof(uint8_t);
                if(pseudoSize>0)
                {
                    if((size-pos)<(unsigned int)pseudoSize)
                    {
                        std::cerr << "MapServer::checkB6(" << binarytoHexa(data,size) << ") error (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
                        abort();
                        return;
                    }
                    public_informations.pseudo=std::string(data+pos,pseudoSize);
                    pos+=pseudoSize;
                    if(public_informations.pseudo.empty())
                    {
                        std::cerr << "MapServer::checkB6(" << binarytoHexa(data,size) << ") error (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
                        abort();
                        return;
                    }
                }
            }
            else
                public_informations.pseudo.clear();

            //the skin
            if((size-pos)<(unsigned int)sizeof(uint8_t))
            {
                std::cerr << "MapServer::checkB6(" << binarytoHexa(data,size) << ") error (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
                abort();
                return;
            }
            uint8_t skinId=data[pos];
            pos+=sizeof(uint8_t);
            public_informations.skinId=skinId;

            //the following monster id to show
            if((size-pos)<(unsigned int)sizeof(uint16_t))
            {
                std::cerr << "MapServer::checkB6(" << binarytoHexa(data,size) << ") error (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
                abort();
                return;
            }
            uint16_t monsterId=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
            pos+=sizeof(uint16_t);
            public_informations.monsterId=monsterId;

            playerinsertList.push_back(public_informations);
            //std::cerr << "insert_player(public_informations,mapId,x,y,direction);" << std::endl;
            index_sub_loop++;
        }
        index++;
    }
    if((size-pos)!=0)
    {
        std::cerr << "MapServer::checkB6(" << binarytoHexa(data,size) << ") playerinsertList: " << playerinsertList.size() << " error (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
        unsigned int index=0;
        while(index<playerinsertList.size())
        {
            Player_public_informations temp=playerinsertList.at(index);
            std::cerr << std::string("- player:") +
                        " " + std::to_string((int)temp.monsterId) +
                        " \"" + temp.pseudo +
                        "\" " + std::to_string((int)temp.simplifiedId) +
                        " " + std::to_string((int)temp.skinId) +
                        " " + std::to_string((int)temp.speed) +
                        " " + std::to_string((int)temp.type)
                      << std::endl;
            index++;
        }
        abort();
        return;
    }
}
#endif

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

bool MapServer::parseUnknownMoving(std::string type,uint32_t object_x,uint32_t object_y,std::unordered_map<std::string,std::string> property_text)
{
    (void)property_text;
    if(type=="rescue")
    {
        std::pair<uint8_t,uint8_t> p(object_x,object_y);
        rescue.insert(p,Orientation_bottom);
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
            std::cerr << "zonecapture point have not the zone attribute: for bot id: " << searchID << " " << botFile << std::endl;
        else if(zonecapture.find(std::pair<uint8_t,uint8_t>(x,y))!=zonecapture.cend())
            std::cerr << "zonecapture point already on the map: for bot id: " << searchID << " " << botFile << std::endl;
        else
            zonecapture.insert(std::pair<uint8_t,uint8_t>(x,y));
        return true;
    }
    return false;
}
