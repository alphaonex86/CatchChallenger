#include "Client.hpp"
#include "ClientList.hpp"
#include <string.h>
#include "../../general/base/ProtocolParsing.hpp"
#include "../../general/base/cpp11addition.hpp"
#include "GlobalServerData.hpp"
#include "MapManagement/MapVisibilityAlgorithm.hpp"

using namespace CatchChallenger;

void Client::sendLocalChatText(const std::string &text)
{
    if(mapIndex>=65535)
        return;
    MapVisibilityAlgorithm &map=MapVisibilityAlgorithm::flat_map_list[mapIndex];
    if((map.localChatDropTotalCache+map.localChatDropNewValue)>=GlobalServerData::serverSettings.ddos.dropGlobalChatMessageLocalClan)
        return;
    map.localChatDropNewValue++;
    if(text.size()>255)
    {
        errorOutput("text too big");
        return;
    }
    normalOutput("[chat local] "+this->public_and_private_informations.public_informations.pseudo+": "+text);

    uint32_t posOutput=0;
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x5F;
        posOutput=1+4;

        //type
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)Chat_type_local;
        posOutput+=1;

        //text
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(text.size());
        posOutput+=1;
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
        posOutput+=text.size();

        //sender pseudo
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(public_and_private_informations.public_informations.pseudo.size());
        posOutput+=1;
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,public_and_private_informations.public_informations.pseudo.data(),public_and_private_informations.public_informations.pseudo.size());
        posOutput+=public_and_private_informations.public_informations.pseudo.size();

        //sender type
        if(GlobalServerData::serverSettings.dontSendPlayerType && public_and_private_informations.public_informations.type==Player_type_premium)
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=Player_type_normal;
        else
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=public_and_private_informations.public_informations.type;
        posOutput+=1;

        //set the dynamic size
        {const uint32_t _tmp_le=(htole32(posOutput-1-4));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1,&_tmp_le,sizeof(_tmp_le));}

    }

    const PLAYER_INDEX_FOR_CONNECTED &size=map.map_clients_list_size();
    PLAYER_INDEX_FOR_CONNECTED index=0;
    while(index<size)
    {
        if(map.map_clients_list_isValid(index))
        {
            const PLAYER_INDEX_FOR_CONNECTED &global_index=map.map_clients_list_at(index);
            ClientList::list->rw(global_index).sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        }
        index++;
    }
}

void Client::insertClientOnMap(MapVisibilityAlgorithm &map)
{
    if(getIndexConnect()==PLAYER_INDEX_FOR_CONNECTED_MAX)
        return;
    /* keep comment to debug, but disable to not flood the console if(index_on_map!=PLAYER_INDEX_FOR_CONNECTED_MAX)
        std::cout << "index_on_map!=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX Client::insertClientOnMap()" << std::endl;*/
    index_on_map=map.insertOnMap(getIndexConnect());
}

void Client::removeClientOnMap(MapVisibilityAlgorithm &map)
{
    if(getIndexConnect()==PLAYER_INDEX_FOR_CONNECTED_MAX)
        return;
    if(index_on_map==PLAYER_INDEX_FOR_CONNECTED_MAX)
    {
        std::cout << "index_on_map==SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX Client::removeClientOnMap()" << std::endl;
        return;
    }
    map.removeOnMap(index_on_map);

    index_on_map=PLAYER_INDEX_FOR_CONNECTED_MAX;
    mapIndex=65535;
}
