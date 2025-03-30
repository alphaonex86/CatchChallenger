#include "Client.hpp"
#include <string.h>
#include "../../general/base/ProtocolParsing.hpp"
#include "../../general/base/cpp11addition.hpp"
#include "GlobalServerData.hpp"
#include "MapServer.hpp"

using namespace CatchChallenger;

void Client::sendLocalChatText(const std::string &text)
{
    if((static_cast<MapServer *>(map)->localChatDropTotalCache+static_cast<MapServer *>(map)->localChatDropNewValue)>=GlobalServerData::serverSettings.ddos.dropGlobalChatMessageLocalClan)
        return;
    static_cast<MapServer *>(map)->localChatDropNewValue++;
    if(map==NULL)
        return;
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
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);
    }

    const size_t &size=static_cast<MapServer *>(map)->clientsForLocalBroadcast.size();
    unsigned int index=0;
    while(index<size)
    {
        Client * const client=static_cast<MapServer *>(map)->clientsForLocalBroadcast.at(index);
        if(client!=this)
            client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        index++;
    }
}

void Client::insertClientOnMap(CommonMap *map)
{
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(vectorcontainsAtLeastOne(static_cast<MapServer *>(map)->clientsForBroadcast,this))
        normalOutput("static_cast<MapServer *>(map)->clientsForBroadcast already have this");
    else
    #endif
    static_cast<MapServer *>(map)->clientsForLocalBroadcast.push_back(this);
}

void Client::removeClientOnMap(CommonMap *map)
{
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(vectorcontainsCount(static_cast<MapServer *>(map)->clientsForBroadcast,this)!=1)
        normalOutput("static_cast<MapServer *>(map)->clientsForBroadcast.count(this)!=1: "+std::to_string(vectorcontainsCount(static_cast<MapServer *>(map)->clientsForBroadcast,this)));
    #endif
    vectorremoveOne(static_cast<MapServer *>(map)->clientsForLocalBroadcast,this);

    map=NULL;
}
