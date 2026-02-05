#include "Client.hpp"
#include "ClientList.hpp"
#include "GlobalServerData.hpp"
#include "../../general/base/ProtocolParsing.hpp"
#include "PreparedDBQuery.hpp"
#ifdef CATCHCHALLENGER_SERVER_DEBUG_COMMAND
#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "LinkToMaster.hpp"
#endif
#endif
#include "GameServerVariables.hpp"
#include <cstring>

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

    const size_t &size=ClientList::list->size();
    unsigned int index=0;
    if(!playerInclude)
    {
        while(index<size)
        {
            if(!ClientList::list->empty(index) && index!=index_connected_player)
                ClientList::list->rw(index).sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            index++;
        }
    }
    else
    {
        while(index<size)
        {
            if(!ClientList::list->empty(index))
                ClientList::list->rw(index).sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            index++;
        }
    }
}

void Client::clanChangeWithoutDb(const CLAN_ID_TYPE &clanId)
{
    //if into clan, remove
    if(public_and_private_informations.clan!=0)
    {
        if(GlobalServerData::serverPrivateVariables.clanList.find(public_and_private_informations.clan)!=GlobalServerData::serverPrivateVariables.clanList.cend())
        {
            Clan &clan=GlobalServerData::serverPrivateVariables.clanList[public_and_private_informations.clan];
            vectorremoveOne(clan.connected_players,index_connected_player);
            if(clan.connected_players.empty())
                GlobalServerData::serverPrivateVariables.clanList.erase(public_and_private_informations.clan);
            public_and_private_informations.clan=0;
        }
    }
    //if enter into new clan insert
    if(clanId>0)
    {
        if(GlobalServerData::serverPrivateVariables.clanList.find(clanId)!=GlobalServerData::serverPrivateVariables.clanList.cend())
        {
            Clan &clan=GlobalServerData::serverPrivateVariables.clanList[clanId];
            public_and_private_informations.clan=clanId;
            clan.connected_players.push_back(index_connected_player);
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
    if(index_connected_player==PLAYER_INDEX_FOR_CONNECTED_MAX)
        return false;
    privateChatDrop.incrementLastValue();
    if(this->public_and_private_informations.public_informations.pseudo==pseudo)
    {
        errorOutput("Can't send them self the PM");
        return false;
    }
    const PLAYER_INDEX_FOR_CONNECTED index=ClientList::list->global_clients_list_bypseudo(pseudo);
    if(index==PLAYER_INDEX_FOR_CONNECTED_MAX)
    {
        receiveSystemText("unable to found the connected player: pseudo: \""+pseudo+"\"",false);
        /*if(GlobalServerData::serverSettings.anonymous)
            normalOutput(std::to_string(character_id_db)+" have try send message to not connected user");
        else
            normalOutput(this->public_and_private_informations.public_informations.pseudo+" have try send message to not connected user: "+pseudo);*/
        return false;
    }
    if(ClientList::list->empty(index))
        return false;
    if(!GlobalServerData::serverSettings.anonymous)
        normalOutput("[chat PM]: "+this->public_and_private_informations.public_informations.pseudo+" -> "+pseudo+": "+text);
    ClientList::list->rw(index).receiveChatText(Chat_type_pm,text,*this);
    return true;
}

bool Client::receiveChatText(const Chat_type &chatType, const std::string &text, const Client &sender_informations)
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
        const std::string &text=sender_informations.public_and_private_informations.public_informations.pseudo;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(text.size());
        posOutput+=1;
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
        posOutput+=text.size();
    }
    if(GlobalServerData::serverSettings.dontSendPlayerType)
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)Player_type_normal;
    else
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)sender_informations.public_and_private_informations.public_informations.type;
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
        if(public_and_private_informations.clan==0)
            errorOutput("Unable to chat with clan, you have not clan");
        else
        {
            if(GlobalServerData::serverPrivateVariables.clanList.find(public_and_private_informations.clan)==GlobalServerData::serverPrivateVariables.clanList.cend())
            {
                const Clan &clan=GlobalServerData::serverPrivateVariables.clanList.at(public_and_private_informations.clan);
                if(!GlobalServerData::serverSettings.anonymous)
                    normalOutput("[chat] "+public_and_private_informations.public_informations.pseudo+
                                 ": To the clan "+clan.name+": "+text
                                );

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

                const size_t &size=clan.connected_players.size();
                unsigned int index=0;
                while(index<size)
                {
                    const PLAYER_INDEX_FOR_CONNECTED &index_player=clan.connected_players.at(index);
                    if(index_player!=index_connected_player)
                    {
                        if(!ClientList::list->empty(index_player))
                            ClientList::list->rw(index_player).sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                    }
                    index++;
                }
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

        const size_t &size=ClientList::list->size();
        unsigned int index=0;
        while(index<size)
        {
            if(index!=index_connected_player)
            {
                if(!ClientList::list->empty(index))
                    ClientList::list->rw(index).sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            }
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
    if(this->last_sended_connected_players==connected_players)
        return;
    this->last_sended_connected_players=connected_players;
    sendRawBlock(data,size);
}

void Client::sendBroadCastCommand(const std::string &command,const std::string &extraText)
{
    normalOutput("command: "+command+" "+extraText);
    if(command=="chat")
    {
        std::vector<std::string> list=stringsplit(extraText,' ');
        if(list.size()<2)
        {
            receiveSystemText("command not understand"+command+" "+extraText);
            normalOutput("command not understand"+command+" "+extraText);
            return;
        }
        if(list.front()=="system")
        {
            list.erase(list.begin());
            sendChatText(Chat_type_system,stringimplode(list,' '));
            return;
        }
        if(list.front()=="system_important")
        {
            list.erase(list.begin());
            sendChatText(Chat_type_system_important,stringimplode(list,' '));
            return;
        }
        else
        {
            receiveSystemText("command not understand"+extraText);
            normalOutput("command not understand"+extraText);
            return;
        }
    }
    else if(command=="setrights")
    {
        std::vector<std::string> list=stringsplit(extraText,' ');
        if(list.size()!=2)
        {
            receiveSystemText("command not understand"+command+" "+extraText);
            normalOutput("command not understand"+command+" "+extraText);
            return;
        }
        const PLAYER_INDEX_FOR_CONNECTED index=ClientList::list->global_clients_list_bypseudo(list.front());
        if(index==PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            receiveSystemText("unable to found the connected player: pseudo: \""+list.front()+"\"",false);
            return;
        }
        if(ClientList::list->empty(index))
            return;
        if(list.back()=="normal")
            ClientList::list->rw(index).setRights(Player_type_normal);
        else if(list.back()=="premium")
            ClientList::list->rw(index).setRights(Player_type_premium);
        else if(list.back()=="gm")
            ClientList::list->rw(index).setRights(Player_type_gm);
        else if(list.back()=="dev")
            ClientList::list->rw(index).setRights(Player_type_dev);
        else
        {
            receiveSystemText("Unable to found into allowed list:  \"normal\", \"premium\", \"dev\", \"gm\", this rights level: "+list.back());
            normalOutput("Unable to found into allowed list:  \"normal\", \"premium\", \"dev\", \"gm\", this rights level: "+list.back());
            return;
        }
    }
    else if(command=="playerlist")
    {
        if(!GlobalServerData::serverSettings.sendPlayerNumber)
        {
            receiveSystemText("This is forbidden on this server!");
            return;
        }
        if(ClientList::list->connected_size()>200)
        {
            receiveSystemText("Too long player list to be send!");
            return;
        }
        else
        {
            uint16_t textSize=0;
            std::vector<std::string> playerStringList;
            PLAYER_INDEX_FOR_CONNECTED index=0;
            while(index<ClientList::list->size())
            {
                if(!ClientList::list->empty(index))
                {
                    const std::string &pseudo=ClientList::list->at(index).public_and_private_informations.public_informations.pseudo;
                    textSize+=3/*text_startbold*/+pseudo.size()+4/*text_stopbold*/+2/*text_commaspace*/;
                    if(textSize>5000)
                    {
                        receiveSystemText("Too long player list to be send!");
                        return;
                    }
                    playerStringList.push_back("<b>"+pseudo+"</b>");
                }
                index++;
            }
            receiveSystemText("players connected "+stringimplode(playerStringList,", "));
        }
        return;
    }
    else if(command=="playernumber")
    {
        if(!GlobalServerData::serverSettings.sendPlayerNumber)
        {
            receiveSystemText("This is forbidden on this server!");
            return;
        }
        if(ClientList::list->connected_size()==1)
            receiveSystemText("You are alone on the server!");
        else
            receiveSystemText("<b>"+std::to_string(ClientList::list->connected_size())+"</b> players connected");
        return;
    }
    else if(command=="kick")
    {
        const PLAYER_INDEX_FOR_CONNECTED index=ClientList::list->global_clients_list_bypseudo(extraText);
        if(index==PLAYER_INDEX_FOR_CONNECTED_MAX)
            return;
        if(ClientList::list->empty(index))
            return;
        ClientList::list->rw(index).kick();
        sendSystemMessage(extraText+" have been kicked by "+public_and_private_informations.public_informations.pseudo,false,true);
        return;
    }
    #ifdef CATCHCHALLENGER_SERVER_DEBUG_COMMAND
    else if(command=="debug")
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

    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_change_right.asyncWrite({
                std::to_string(newType),
                std::to_string(character_id)
                });
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    (void)newType;
    #elif CATCHCHALLENGER_DB_FILE
    (void)newType;
    #else
    #error Define what do here
    #endif
}

void Client::timeRangeEvent(const uint64_t &timestamps)
{
    //24h trigger, then useless drop list
    //5min scan + bool to have new
    //null memory allocation pressure
    Client::timeRangeEventNew=true;
    Client::timeRangeEventTimestamps=timestamps;
}
