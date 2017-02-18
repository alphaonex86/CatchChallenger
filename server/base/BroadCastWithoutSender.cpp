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
    Client::generalChatDrop.flush();
    Client::clanChatDrop.flush();
    Client::privateChatDrop.flush();
}
