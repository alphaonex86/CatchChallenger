#include "BroadCastWithoutSender.h"
#include "Client.h"
#include "GlobalServerData.h"
#include "../../general/base/ProtocolParsing.h"

using namespace CatchChallenger;

unsigned char BroadCastWithoutSender::bufferSendPlayer[]={0x64/*reply server to client*/,0x00,0x00};

BroadCastWithoutSender BroadCastWithoutSender::broadCastWithoutSender;

BroadCastWithoutSender::BroadCastWithoutSender()
{
}

#ifndef EPOLLCATCHCHALLENGERSERVER
void BroadCastWithoutSender::emit_serverCommand(const std::string &command,const std::string &extraText)
{
    /*emit */serverCommand(command,extraText);
}

void BroadCastWithoutSender::emit_new_player_is_connected(const Player_private_and_public_informations &newPlayer)
{
    /*emit */new_player_is_connected(newPlayer);
}

void BroadCastWithoutSender::emit_player_is_disconnected(const std::string &oldPlayer)
{
    /*emit */player_is_disconnected(oldPlayer);
}

void BroadCastWithoutSender::emit_new_chat_message(const std::string &pseudo,const Chat_type &type,const std::string &text)
{
    /*emit */new_chat_message(pseudo,type,text);
}
#endif

void BroadCastWithoutSender::receive_instant_player_number(const int16_t &connected_players)
{
    if(GlobalServerData::serverSettings.sendPlayerNumber)
    {
        if(Client::clientBroadCastList.empty())
            return;

        uint8_t outputSize;
        if(GlobalServerData::serverSettings.max_players<=255)
        {
            bufferSendPlayer[0x01]=connected_players;
            outputSize=2;
        }
        else
        {
            *reinterpret_cast<uint16_t *>(bufferSendPlayer+0x01)=htole16(connected_players);
            outputSize=3;
        }

        int index=0;
        const int &list_size=Client::clientBroadCastList.size();
        #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
        std::cout << "Send to " << Client::clientBroadCastList.size() << " players: " << binarytoHexa(reinterpret_cast<char *>(bufferSendPlayer),outputSize) << std::endl;
        #endif
        while(index<list_size)
        {
            Client::clientBroadCastList.at(index)->receive_instant_player_number(connected_players,reinterpret_cast<char *>(bufferSendPlayer),outputSize);
            index++;
        }
    }
}

void BroadCastWithoutSender::doDDOSChat()
{
    /*int index=0;while(index<(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1))
    {
        Client::generalChatDrop[index]=Client::generalChatDrop[index+1];
        index++;
    }*/
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(Client::generalChatDropTotalCache<Client::generalChatDrop[0])
        {
            std::cerr << "doDDOSChat() general int overflow error" << Client::generalChatDropTotalCache << "<" << Client::generalChatDrop[0] << std::endl;
            abort();
        }
        #endif
        Client::generalChatDropTotalCache-=Client::generalChatDrop[0];
        memmove(Client::generalChatDrop,Client::generalChatDrop+2,sizeof(Client::generalChatDrop)-2);
        Client::generalChatDrop[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1]=Client::generalChatDropNewValue;
        Client::generalChatDropTotalCache+=Client::generalChatDropNewValue;
        Client::generalChatDropNewValue=0;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(Client::generalChatDropTotalCache>GlobalServerData::serverSettings.ddos.dropGlobalChatMessageGeneral)
        {
            std::cerr << "doDDOSChat() private 2 int overflow error" << Client::generalChatDropTotalCache << ">" << GlobalServerData::serverSettings.ddos.dropGlobalChatMessageGeneral << std::endl;
            abort();
        }
        #endif
    }
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(Client::clanChatDropTotalCache<Client::clanChatDrop[0])
        {
            std::cerr << "doDDOSChat() clan int overflow error" << Client::clanChatDropTotalCache << "<" << Client::clanChatDrop[0] << std::endl;
            abort();
        }
        #endif
        Client::clanChatDropTotalCache-=Client::clanChatDrop[0];
        memmove(Client::clanChatDrop,Client::clanChatDrop+2,sizeof(Client::clanChatDrop)-2);
        Client::clanChatDrop[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1]=Client::clanChatDropNewValue;
        Client::clanChatDropTotalCache+=Client::clanChatDropNewValue;
        Client::clanChatDropNewValue=0;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(Client::clanChatDropTotalCache>GlobalServerData::serverSettings.ddos.dropGlobalChatMessageLocalClan)
        {
            std::cerr << "doDDOSChat() private 2 int overflow error" << Client::clanChatDropTotalCache << ">" << GlobalServerData::serverSettings.ddos.dropGlobalChatMessageLocalClan << std::endl;
            abort();
        }
        #endif
    }
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(Client::privateChatDropTotalCache<Client::privateChatDrop[0])
        {
            std::cerr << "doDDOSChat() private int overflow error" << Client::privateChatDropTotalCache << "<" << Client::privateChatDrop[0] << std::endl;
            abort();
        }
        #endif
        Client::privateChatDropTotalCache-=Client::privateChatDrop[0];
        memmove(Client::privateChatDrop,Client::privateChatDrop+2,sizeof(Client::privateChatDrop)-2);
        Client::privateChatDrop[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1]=Client::privateChatDropNewValue;
        Client::privateChatDropTotalCache+=Client::privateChatDropNewValue;
        Client::privateChatDropNewValue=0;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(Client::privateChatDropTotalCache>GlobalServerData::serverSettings.ddos.dropGlobalChatMessagePrivate)
        {
            std::cerr << "doDDOSChat() private 2 int overflow error" << Client::privateChatDropTotalCache << ">" << GlobalServerData::serverSettings.ddos.dropGlobalChatMessagePrivate << std::endl;
            abort();
        }
        #endif
    }
}
