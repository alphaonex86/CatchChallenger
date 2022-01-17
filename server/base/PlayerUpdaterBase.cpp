#include "PlayerUpdaterBase.hpp"
#include "GlobalServerData.hpp"
#include "Client.hpp"

using namespace CatchChallenger;

uint16_t PlayerUpdaterBase::connected_players=0;
uint16_t PlayerUpdaterBase::sended_connected_players=0;

PlayerUpdaterBase::PlayerUpdaterBase()
{
}

void PlayerUpdaterBase::addConnectedPlayer()
{
    internal_addConnectedPlayer();
}

void PlayerUpdaterBase::removeConnectedPlayer()
{
    internal_removeConnectedPlayer();
}

void PlayerUpdaterBase::internal_addConnectedPlayer()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(connected_players<GlobalServerData::serverSettings.max_players)
    #endif
        connected_players++;
    switch(connected_players)
    {
        //Max bandwith: (number max of player in this mode)*(packet size)*(tick by second)=49*16*1=735B/s
        case 10:
            setInterval(1000);
        break;
        //Max bandwith: (number max of player in this mode)*(packet size)*(tick by second)=99*16*0.5=792B/s
        case 50:
            setInterval(2000);
        break;
        //Max bandwith: (number max of player in this mode)*(packet size)*(tick by second)=999*16*0.1=1.6KB/s
        case 100:
            setInterval(10000);
        break;
        //Max bandwith: (number max of player in this mode)*(packet size)*(tick by second)=65535*16*0.016=16.7KB/s
        case 1000:
            setInterval(60000);
        break;
        default:
        break;
    }
}

void PlayerUpdaterBase::internal_removeConnectedPlayer()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(connected_players>0)
    #endif
        connected_players--;
    switch(connected_players)
    {
        case (10-1):
            setInterval(250);
        break;
        case (50-1):
            setInterval(1000);
        break;
        case (100-1):
            setInterval(2000);
        break;
        case (1000-1):
            setInterval(10000);
        break;
        default:
        break;
    }
}

void PlayerUpdaterBase::exec()
{
    if(GlobalServerData::serverSettings.sendPlayerNumber && sended_connected_players!=connected_players)
    {
        sended_connected_players=connected_players;
        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        *reinterpret_cast<uint16_t *>(Client::protocolMessageLogicalGroupAndServerList+Client::protocolMessageLogicalGroupAndServerListPosPlayerNumber)=htole16(connected_players);
        #endif
    }
    BroadCastWithoutSender::broadCastWithoutSender.receive_instant_player_number(connected_players);
}
