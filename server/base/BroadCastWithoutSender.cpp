#include "BroadCastWithoutSender.hpp"
#include "Client.hpp"
#include "GlobalServerData.hpp"
#include "../../general/base/ProtocolParsing.hpp"

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
            bufferSendPlayer[0x01]=static_cast<uint8_t>(connected_players);
            outputSize=2;
        }
        else
        {
            *reinterpret_cast<uint16_t *>(bufferSendPlayer+0x01)=htole16(connected_players);
            outputSize=3;
        }

        unsigned int index=0;
        #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
        std::cout << "Send to " << Client::clientBroadCastList.size() << " players: " << binarytoHexa(reinterpret_cast<char *>(bufferSendPlayer),outputSize) << std::endl;
        #endif
        while(index<Client::clientBroadCastList.size())
        {
            Client::clientBroadCastList.at(index)->receive_instant_player_number(connected_players,reinterpret_cast<char *>(bufferSendPlayer),outputSize);
            index++;
        }
    }
}

void BroadCastWithoutSender::timeRangeEventTrigger()
{
    //10 clients per 15s = 40 clients per min = 57600 clients per 24h
    //1ms time used each 15000ms
    if(Client::timeRangeEventNew!=true || Client::timeRangeEventTimestamps==0)
        return;
    uint32_t updatedClient=0;
    unsigned int index=0;
    while(index<Client::clientBroadCastList.size())
    {
        Client *client=Client::clientBroadCastList.at(index);
        if(client->triggerDaillyGift(Client::timeRangeEventTimestamps))
        {
            updatedClient++;
            if(updatedClient==10)
                return;
        }
        index++;
    }
    Client::timeRangeEventNew=false;
}

void BroadCastWithoutSender::doDDOSChat()
{
    Client::generalChatDrop.flush();
    Client::clanChatDrop.flush();
    Client::privateChatDrop.flush();
}
