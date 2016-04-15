#include "Client.h"
#include "GlobalServerData.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/FacilityLib.h"
#include "PreparedDBQuery.h"

using namespace CatchChallenger;

//without verification of rights
void Client::sendSystemMessage(const std::string &text,const bool &important)
{
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x5F;
    posOutput+=1+4;

    if(important)
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x08;
    else
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x07;
    posOutput+=1;
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
        posOutput+=1;
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
        posOutput+=text.size();
    }

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size

    const int &size=clientBroadCastList.size();
    int index=0;
    while(index<size)
    {
        Client * const client=clientBroadCastList.at(index);
        if(client!=this)
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

void Client::sendPM(const std::string &text,const std::string &pseudo)
{
    if((privateChatDropTotalCache+privateChatDropNewValue)>=GlobalServerData::serverSettings.ddos.dropGlobalChatMessagePrivate)
        return;
    privateChatDropNewValue++;
    if(this->public_and_private_informations.public_informations.pseudo==pseudo)
    {
        errorOutput("Can't send them self the PM");
        return;
    }
    if(playerByPseudo.find(pseudo)==playerByPseudo.cend())
    {
        receiveSystemText("unable to found the connected player: pseudo: \""+pseudo+"\"",false);
        if(GlobalServerData::serverSettings.anonymous)
            normalOutput(std::to_string(character_id)+" have try send message to not connected user");
        else
            normalOutput(this->public_and_private_informations.public_informations.pseudo+" have try send message to not connected user: "+pseudo);
        return;
    }
    if(!GlobalServerData::serverSettings.anonymous)
        normalOutput("[chat PM]: "+this->public_and_private_informations.public_informations.pseudo+" -> "+pseudo+": "+text);
    #ifndef EPOLLCATCHCHALLENGERSERVER
    BroadCastWithoutSender::broadCastWithoutSender.emit_new_chat_message(this->public_and_private_informations.public_informations.pseudo,Chat_type_pm,"to "+pseudo+": "+text);
    #endif
    playerByPseudo.at(pseudo)->receiveChatText(Chat_type_pm,text,this);
}

void Client::receiveChatText(const Chat_type &chatType,const std::string &text,const Client *sender_informations)
{
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x5F;
    posOutput+=1+4;

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)chatType;
    posOutput+=1;
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
        posOutput+=1;
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
        posOutput+=text.size();
    }
    {
        const std::string &text=sender_informations->public_and_private_informations.public_informations.pseudo;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
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
}

void Client::receiveSystemText(const std::string &text,const bool &important)
{
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
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
        posOutput+=1;
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
        posOutput+=text.size();
    }

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::sendChatText(const Chat_type &chatType,const std::string &text)
{
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x5F;
    posOutput+=1+4;

    if(chatType==Chat_type_clan)
    {
        if((clanChatDropTotalCache+clanChatDropNewValue)>=GlobalServerData::serverSettings.ddos.dropGlobalChatMessageLocalClan)
            return;
        if(clan==0)
            errorOutput("Unable to chat with clan, you have not clan");
        else
        {
            if(!GlobalServerData::serverSettings.anonymous)
                normalOutput("[chat] "+public_and_private_informations.public_informations.pseudo+
                             ": To the clan "+clan->name+": "+text
                            );
            #ifndef EPOLLCATCHCHALLENGERSERVER
            BroadCastWithoutSender::broadCastWithoutSender.emit_new_chat_message(public_and_private_informations.public_informations.pseudo,chatType,text);
            #endif
            const std::vector<Client *> &playerWithSameClan = clan->players;

            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)chatType;
            posOutput+=1;
            {
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
                posOutput+=1;
                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
                posOutput+=text.size();
            }
            {
                const std::string &text=public_and_private_informations.public_informations.pseudo;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
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

            const int &size=playerWithSameClan.size();
            int index=0;
            while(index<size)
            {
                Client * const client=playerWithSameClan.at(index);
                if(client!=this)
                    client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                index++;
            }
        }
        return;
    }
/* Now do by command
 *	else if(chatType==Chat_type_system || chatType==Chat_type_system_important)*/
    else
    {
        if((generalChatDropTotalCache+generalChatDropNewValue)>=GlobalServerData::serverSettings.ddos.dropGlobalChatMessageGeneral)
            return;
        if(!GlobalServerData::serverSettings.anonymous)
            normalOutput("[chat all] "+public_and_private_informations.public_informations.pseudo+": "+text);
        #ifndef EPOLLCATCHCHALLENGERSERVER
        BroadCastWithoutSender::broadCastWithoutSender.emit_new_chat_message(public_and_private_informations.public_informations.pseudo,chatType,text);
        #endif

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)chatType;
        posOutput+=1;
        {
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
            posOutput+=1;
            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
            posOutput+=text.size();
        }
        {
            const std::string &text=public_and_private_informations.public_informations.pseudo;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
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

        const int &size=clientBroadCastList.size();
        int index=0;
        while(index<size)
        {
            Client * const client=clientBroadCastList.at(index);
            if(client!=this)
                client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            index++;
        }
        return;
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
    normalOutput(Client::text_command+command+Client::text_space+extraText);
    if(command==Client::text_chat)
    {
        std::vector<std::string> list=stringsplit(extraText,' ');
        if(list.size()<2)
        {
            receiveSystemText(Client::text_commandnotunderstand+command+Client::text_space+extraText);
            normalOutput(Client::text_commandnotunderstand+command+Client::text_space+extraText);
            return;
        }
        if(list.front()==Client::text_system)
        {
            list.erase(list.begin());
            sendChatText(Chat_type_system,stringimplode(list,' '));
            return;
        }
        if(list.front()==Client::text_system_important)
        {
            list.erase(list.begin());
            sendChatText(Chat_type_system_important,stringimplode(list,' '));
            return;
        }
        else
        {
            receiveSystemText(Client::text_commandnotunderstand+extraText);
            normalOutput(Client::text_commandnotunderstand+extraText);
            return;
        }
    }
    else if(command==Client::text_setrights)
    {
        std::vector<std::string> list=stringsplit(extraText,' ');
        if(list.size()!=2)
        {
            receiveSystemText(Client::text_commandnotunderstand+command+Client::text_space+extraText);
            normalOutput(Client::text_commandnotunderstand+command+Client::text_space+extraText);
            return;
        }
        if(playerByPseudo.find(list.front())==playerByPseudo.cend())
        {
            receiveSystemText(Client::text_unabletofoundtheconnectedplayertokick+extraText);
            normalOutput(Client::text_unabletofoundtheconnectedplayertokick+extraText);
            return;
        }
        Client * client=playerByPseudo.at(list.front());
        if(client==NULL)
        {
            std::cerr << "Internal bug for setrights, client is NULL" << std::endl;
            normalOutput(Client::text_unabletofoundtheconnectedplayertokick+extraText);
            return;
        }
        if(list.back()==Client::text_normal)
            client->setRights(Player_type_normal);
        else if(list.back()==Client::text_premium)
            client->setRights(Player_type_premium);
        else if(list.back()==Client::text_gm)
            client->setRights(Player_type_gm);
        else if(list.back()==Client::text_dev)
            client->setRights(Player_type_dev);
        else
        {
            receiveSystemText(Client::text_unabletofoundthisrightslevel+list.back());
            normalOutput(Client::text_unabletofoundthisrightslevel+list.back());
            return;
        }
    }
    else if(command==Client::text_playerlist)
    {
        if(playerByPseudo.size()==1)
            receiveSystemText(Client::text_Youarealoneontheserver);
        else
        {
            std::vector<std::string> playerStringList;
            auto i=playerByPseudo.begin();
            while(i!=playerByPseudo.cend())
            {
                playerStringList.push_back(Client::text_startbold+i->second->public_and_private_informations.public_informations.pseudo+Client::text_stopbold);
                ++i;
            }
            receiveSystemText(Client::text_playersconnectedspace+stringimplode(playerStringList,Client::text_commaspace));
        }
        return;
    }
    else if(command==Client::text_playernumber)
    {
        if(playerByPseudo.size()==1)
            receiveSystemText(Client::text_Youarealoneontheserver);
        else
            receiveSystemText(Client::text_startbold+std::to_string(playerByPseudo.size())+Client::text_stopbold+Client::text_playersconnected);
        return;
    }
    else if(command==Client::text_kick)
    {
        //drop, and do the command here to separate the loop
        if(playerByPseudo.find(extraText)==playerByPseudo.cend())
        {
            receiveSystemText(Client::text_unabletofoundtheconnectedplayertokick+extraText);
            normalOutput(Client::text_unabletofoundtheconnectedplayertokick+extraText);
            return;
        }
        playerByPseudo.at(extraText)->kick();
        sendSystemMessage(extraText+Client::text_havebeenkickedby+public_and_private_informations.public_informations.pseudo);
        return;
    }
    else
        normalOutput("unknow command: "+command+", text: "+extraText);
}

void Client::setRights(const Player_type& type)
{
    public_and_private_informations.public_informations.type=type;
    const int &newType=type/0x10-1;

    const std::string &queryText=PreparedDBQueryCommon::db_query_change_right.compose(
                std::to_string(newType),
                std::to_string(account_id)
                );
    dbQueryWriteCommon(queryText);
}
