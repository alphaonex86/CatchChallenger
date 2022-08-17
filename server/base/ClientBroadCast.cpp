#include "Client.hpp"
#include "GlobalServerData.hpp"
#include "../../general/base/ProtocolParsing.hpp"
#include "PreparedDBQuery.hpp"
#include "StaticText.hpp"
#ifdef CATCHCHALLENGER_SERVER_DEBUG_COMMAND
#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "LinkToMaster.hpp"
#endif
#endif
#include "GameServerVariables.hpp"

using namespace CatchChallenger;

//without verification of rights
void Client::sendSystemMessage(const std::string &text, const bool &important, const bool &playerInclude)
{
    if(text.size()>65535)
    {
        std::cerr << "sendSystemMessage() text too big (abort): " << text << std::endl;
        abort();
    }
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x5F;
    posOutput+=1+4;

    if(important)
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)Chat_type_system_important;
    else
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)Chat_type_system;
    posOutput+=1;
    {
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(text.size());
        posOutput+=2;
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
        posOutput+=text.size();
    }

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size

    const size_t &size=clientBroadCastList.size();
    unsigned int index=0;
    if(!playerInclude)
        while(index<size)
        {
            Client * const client=clientBroadCastList.at(index);
            if(client!=this)
                client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            index++;
        }
    else
        while(index<size)
        {
            Client * const client=clientBroadCastList.at(index);
            client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            index++;
        }
}

void Client::clanChangeWithoutDb(const uint32_t &clanId)
{
    if(clan!=NULL)
    {
        vectorremoveOne(clan->players,this);
        if(clan->players.empty())
        {
            clanList.erase(public_and_private_informations.clan);
            delete clan;
        }
        clan=NULL;
        public_and_private_informations.clan=0;
    }
    if(clanId>0)
    {
        if(clanList.find(clanId)!=clanList.cend())
        {
            public_and_private_informations.clan=clanId;
            clan=clanList[clanId];
            clan->players.push_back(this);
        }
        else
            errorOutput("Clan not found for insert");
    }
}

bool Client::sendPM(const std::string &text,const std::string &pseudo)
{
    if(text.size()>255)
    {
        errorOutput("Client::sendPM() chat text dropped because is too big for the 8Bits header");
        return false;
    }
    if(privateChatDrop.total()>=GlobalServerData::serverSettings.ddos.dropGlobalChatMessagePrivate)
        return false;
    privateChatDrop.incrementLastValue();
    if(this->public_and_private_informations.public_informations.pseudo==pseudo)
    {
        errorOutput("Can't send them self the PM");
        return false;
    }
    if(playerByPseudo.find(pseudo)==playerByPseudo.cend())
    {
        receiveSystemText("unable to found the connected player: pseudo: \""+pseudo+"\"",false);
        if(GlobalServerData::serverSettings.anonymous)
            normalOutput(std::to_string(character_id)+" have try send message to not connected user");
        else
            normalOutput(this->public_and_private_informations.public_informations.pseudo+" have try send message to not connected user: "+pseudo);
        return true;
    }
    if(!GlobalServerData::serverSettings.anonymous)
        normalOutput("[chat PM]: "+this->public_and_private_informations.public_informations.pseudo+" -> "+pseudo+": "+text);
    playerByPseudo.at(pseudo)->receiveChatText(Chat_type_pm,text,this);
    return false;
}

bool Client::receiveChatText(const Chat_type &chatType,const std::string &text,const Client *sender_informations)
{
    if(text.size()>255)
    {
        std::cerr << "Client::receiveChatText() chat text dropped because is too big for the 8Bits header" << std::endl;
        return false;
    }
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x5F;
    posOutput+=1+4;

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)chatType;
    posOutput+=1;
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(text.size());
        posOutput+=1;
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
        posOutput+=text.size();
    }
    {
        const std::string &text=sender_informations->public_and_private_informations.public_informations.pseudo;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(text.size());
        posOutput+=1;
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
        posOutput+=text.size();
    }
    if(GlobalServerData::serverSettings.dontSendPlayerType)
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)Player_type_normal;
    else
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)sender_informations->public_and_private_informations.public_informations.type;
    posOutput+=1;

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    return true;
}

bool Client::receiveSystemText(const std::string &text,const bool &important)
{
    if(text.size()>65535)
    {
        std::cerr << "Client::receiveSystemText() chat text dropped because is too big for the 16Bits header" << std::endl;
        return false;
    }
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x5F;
    posOutput+=1+4;

    if(important)
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)Chat_type_system_important;
    else
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)Chat_type_system;
    posOutput+=1;
    {
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(text.size());
        posOutput+=2;
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
        posOutput+=text.size();
    }

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    return true;
}

bool Client::sendChatText(const Chat_type &chatType,const std::string &text)
{
    if(text.size()>255)
    {
        std::cerr << "Client::sendChatText() chat text dropped because is too big for the 8Bits header" << std::endl;
        return false;
    }
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x5F;
    posOutput+=1+4;

    if(chatType==Chat_type_clan)
    {
        if(clanChatDrop.total()>=GlobalServerData::serverSettings.ddos.dropGlobalChatMessageLocalClan)
            return false;
        clanChatDrop.incrementLastValue();
        if(clan==0)
            errorOutput("Unable to chat with clan, you have not clan");
        else
        {
            if(!GlobalServerData::serverSettings.anonymous)
                normalOutput("[chat] "+public_and_private_informations.public_informations.pseudo+
                             ": To the clan "+clan->name+": "+text
                            );
            const std::vector<Client *> &playerWithSameClan = clan->players;

            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)chatType;
            posOutput+=1;
            {
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(text.size());
                posOutput+=1;
                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
                posOutput+=text.size();
            }
            {
                const std::string &text=public_and_private_informations.public_informations.pseudo;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(text.size());
                posOutput+=1;
                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
                posOutput+=text.size();
            }
            if(GlobalServerData::serverSettings.dontSendPlayerType)
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)Player_type_normal;
            else
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)public_and_private_informations.public_informations.type;
            posOutput+=1;

            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size

            const size_t &size=playerWithSameClan.size();
            unsigned int index=0;
            while(index<size)
            {
                Client * const client=playerWithSameClan.at(index);
                if(client!=this)
                    client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                index++;
            }
        }
        return true;
    }
/* Now do by command
 *	else if(chatType==Chat_type_system || chatType==Chat_type_system_important)*/
    else
    {
        if(generalChatDrop.total()>=GlobalServerData::serverSettings.ddos.dropGlobalChatMessageGeneral)
            return false;
        generalChatDrop.incrementLastValue();
        if(!GlobalServerData::serverSettings.anonymous)
            normalOutput("[chat all] "+public_and_private_informations.public_informations.pseudo+": "+text);

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)chatType;
        posOutput+=1;
        {
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(text.size());
            posOutput+=1;
            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
            posOutput+=text.size();
        }
        {
            const std::string &text=public_and_private_informations.public_informations.pseudo;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(text.size());
            posOutput+=1;
            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
            posOutput+=text.size();
        }
        if(GlobalServerData::serverSettings.dontSendPlayerType)
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)Player_type_normal;
        else
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)public_and_private_informations.public_informations.type;
        posOutput+=1;

        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size

        const size_t &size=clientBroadCastList.size();
        unsigned int index=0;
        while(index<size)
        {
            Client * const client=clientBroadCastList.at(index);
            if(client!=this)
                client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            index++;
        }
        return true;
    }
}

void Client::receive_instant_player_number(const uint16_t &connected_players, const char * const data, const uint8_t &size)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size!=2 && size!=3)
        return;
    #endif
    if(stat!=ClientStat::CharacterSelected)
        return;
    if(this->connected_players==connected_players)
        return;
    this->connected_players=connected_players;
    sendRawBlock(data,size);
}

void Client::sendBroadCastCommand(const std::string &command,const std::string &extraText)
{
    normalOutput(StaticText::text_command+command+StaticText::text_space+extraText);
    if(command==StaticText::text_chat)
    {
        std::vector<std::string> list=stringsplit(extraText,' ');
        if(list.size()<2)
        {
            receiveSystemText(StaticText::text_commandnotunderstand+command+StaticText::text_space+extraText);
            normalOutput(StaticText::text_commandnotunderstand+command+StaticText::text_space+extraText);
            return;
        }
        if(list.front()==StaticText::text_system)
        {
            list.erase(list.begin());
            sendChatText(Chat_type_system,stringimplode(list,' '));
            return;
        }
        if(list.front()==StaticText::text_system_important)
        {
            list.erase(list.begin());
            sendChatText(Chat_type_system_important,stringimplode(list,' '));
            return;
        }
        else
        {
            receiveSystemText(StaticText::text_commandnotunderstand+extraText);
            normalOutput(StaticText::text_commandnotunderstand+extraText);
            return;
        }
    }
    else if(command==StaticText::text_setrights)
    {
        std::vector<std::string> list=stringsplit(extraText,' ');
        if(list.size()!=2)
        {
            receiveSystemText(StaticText::text_commandnotunderstand+command+StaticText::text_space+extraText);
            normalOutput(StaticText::text_commandnotunderstand+command+StaticText::text_space+extraText);
            return;
        }
        if(playerByPseudo.find(list.front())==playerByPseudo.cend())
        {
            receiveSystemText(StaticText::text_unabletofoundtheconnectedplayertokick+extraText);
            normalOutput(StaticText::text_unabletofoundtheconnectedplayertokick+extraText);
            return;
        }
        Client * client=playerByPseudo.at(list.front());
        if(client==NULL)
        {
            std::cerr << "Internal bug for setrights, client is NULL" << std::endl;
            normalOutput(StaticText::text_unabletofoundtheconnectedplayertokick+extraText);
            return;
        }
        if(list.back()==StaticText::text_normal)
            client->setRights(Player_type_normal);
        else if(list.back()==StaticText::text_premium)
            client->setRights(Player_type_premium);
        else if(list.back()==StaticText::text_gm)
            client->setRights(Player_type_gm);
        else if(list.back()==StaticText::text_dev)
            client->setRights(Player_type_dev);
        else
        {
            receiveSystemText(StaticText::text_unabletofoundthisrightslevel+list.back());
            normalOutput(StaticText::text_unabletofoundthisrightslevel+list.back());
            return;
        }
    }
    else if(command==StaticText::text_playerlist)
    {
        if(!GlobalServerData::serverSettings.sendPlayerNumber)
        {
            receiveSystemText(StaticText::text_forbidenonthisserver);
            return;
        }
        /*if(playerByPseudo.size()==1)
            receiveSystemText(StaticText::text_Youarealoneontheserver);
        else*/ if(playerByPseudo.size()>200)
        {
            receiveSystemText(StaticText::text_toolongplayerlist);
            return;
        }
        else
        {
            uint16_t textSize=0;
            std::vector<std::string> playerStringList;
            auto i=playerByPseudo.begin();
            while(i!=playerByPseudo.cend())
            {
                const std::string &pseudo=i->second->public_and_private_informations.public_informations.pseudo;
                textSize+=3/*text_startbold*/+pseudo.size()+4/*text_stopbold*/+2/*text_commaspace*/;
                if(textSize>5000)
                {
                    receiveSystemText(StaticText::text_toolongplayerlist);
                    return;
                }
                playerStringList.push_back(StaticText::text_startbold+pseudo+StaticText::text_stopbold);
                ++i;
            }
            receiveSystemText(StaticText::text_playersconnectedspace+stringimplode(playerStringList,StaticText::text_commaspace));
        }
        return;
    }
    else if(command==StaticText::text_playernumber)
    {
        if(!GlobalServerData::serverSettings.sendPlayerNumber)
        {
            receiveSystemText(StaticText::text_forbidenonthisserver);
            return;
        }
        if(playerByPseudo.size()==1)
            receiveSystemText(StaticText::text_Youarealoneontheserver);
        else
            receiveSystemText(StaticText::text_startbold+std::to_string(playerByPseudo.size())+StaticText::text_stopbold+StaticText::text_playersconnected);
        return;
    }
    else if(command==StaticText::text_kick)
    {
        //drop, and do the command here to separate the loop
        if(playerByPseudo.find(extraText)==playerByPseudo.cend())
        {
            receiveSystemText(StaticText::text_unabletofoundtheconnectedplayertokick+extraText);
            normalOutput(StaticText::text_unabletofoundtheconnectedplayertokick+extraText);
            return;
        }
        Client * const player=playerByPseudo.at(extraText);
        player->kick();
        sendSystemMessage(extraText+StaticText::text_havebeenkickedby+public_and_private_informations.public_informations.pseudo,false,true);
        return;
    }
    #ifdef CATCHCHALLENGER_SERVER_DEBUG_COMMAND
    else if(command==StaticText::text_debug)
    {
        if(extraText=="moreid")
        {
            #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
            if(LinkToMaster::linkToMaster->askMoreMaxMonsterId())
                receiveSystemText("More monster id asked");
            else
                receiveSystemText("Monster id ask already in progress");
            if(LinkToMaster::linkToMaster->askMoreMaxClanId())
                receiveSystemText("More clan id asked");
            else
                receiveSystemText("Clan id ask already in progress");
            #else
            receiveSystemText("This can be do on gameserver alone, not all in one");
            normalOutput("This can be do on gameserver alone, not all in one");
            #endif
        }
        else
        {
            receiveSystemText("unknown sub command: "+command+", text: "+extraText);
            normalOutput("unknown sub command: "+command+", text: "+extraText);
        }
    }
    #endif
    else
        normalOutput("unknown command: "+command+", text: "+extraText);
}

void Client::setRights(const Player_type& type)
{
    public_and_private_informations.public_informations.type=type;
    const int &newType=type/0x10-1;

    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_change_right.asyncWrite({
                std::to_string(newType),
                std::to_string(character_id)
                });
}

void Client::timeRangeEvent(const uint64_t &timestamps)
{
    //24h trigger, then useless drop list
    //5min scan + bool to have new
    //null memory allocation pressure
    Client::timeRangeEventNew=true;
    Client::timeRangeEventTimestamps=timestamps;
}
