#include "BroadCastWithoutSender.hpp"
#include "Client.hpp"
#include "ClientList.hpp"
#include "GlobalServerData.hpp"

using namespace CatchChallenger;

unsigned char BroadCastWithoutSender::bufferSendPlayer[]={0x64/*reply server to client*/,0x00,0x00};

BroadCastWithoutSender BroadCastWithoutSender::broadCastWithoutSender;

BroadCastWithoutSender::BroadCastWithoutSender()
{
}

void BroadCastWithoutSender::receive_instant_player_number(const int16_t &connected_players)
{
    if(GlobalServerData::serverSettings.sendPlayerNumber)
    {
        if(ClientList::list->connected_size()<1)
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
        while(index<ClientList::list->size())
        {
            if(!ClientList::list->empty(index))
                ClientList::list->rw(index).receive_instant_player_number(connected_players,reinterpret_cast<char *>(bufferSendPlayer),outputSize);
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
    while(index<ClientList::list->size())
    {
        if(!ClientList::list->empty(index))
        {
            if(ClientList::list->rw(index).triggerDaillyGift(Client::timeRangeEventTimestamps))
            {
                updatedClient++;
                if(updatedClient==10)
                    return;
            }
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
