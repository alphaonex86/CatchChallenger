#include "PlayerUpdater.hpp"
#include "GlobalServerData.hpp"
#include "Client.hpp"

#ifdef EPOLLCATCHCHALLENGERSERVER
#include "BroadCastWithoutSender.hpp"
#endif

using namespace CatchChallenger;

uint16_t PlayerUpdater::connected_players=0;
uint16_t PlayerUpdater::sended_connected_players=0;

PlayerUpdater::PlayerUpdater()
      #ifndef EPOLLCATCHCHALLENGERSERVER
      :
        next_send_timer(NULL)
      #endif
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    connect(this,&PlayerUpdater::send_addConnectedPlayer,this,&PlayerUpdater::internal_addConnectedPlayer,Qt::QueuedConnection);
    connect(this,&PlayerUpdater::send_removeConnectedPlayer,this,&PlayerUpdater::internal_removeConnectedPlayer,Qt::QueuedConnection);

    connect(this,&PlayerUpdater::try_initAll,                  this,&PlayerUpdater::initAll,              Qt::QueuedConnection);
    /*emit */try_initAll();
    #else
    initAll();
    #endif
}

void PlayerUpdater::addConnectedPlayer()
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    /*emit */send_addConnectedPlayer();
    #else
    internal_addConnectedPlayer();
    #endif
}

void PlayerUpdater::removeConnectedPlayer()
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    /*emit */send_removeConnectedPlayer();
    #else
    internal_removeConnectedPlayer();
    #endif
}

void PlayerUpdater::initAll()
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(next_send_timer==NULL)
    {
        next_send_timer=new QTimer();
        next_send_timer->setSingleShot(true);

        //Max bandwith: (number max of player in this mode)*(packet size)*(tick by second)=9*16*4=576B/s
        next_send_timer->setInterval(250);

        connect(next_send_timer,&QTimer::timeout,this,&PlayerUpdater::exec,Qt::QueuedConnection);
    }
    #else
    setInterval(250);
    #endif
}

void PlayerUpdater::internal_addConnectedPlayer()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(connected_players<GlobalServerData::serverSettings.max_players)
    #endif
        connected_players++;

    #ifndef EPOLLCATCHCHALLENGERSERVER
    switch(connected_players)
    {
        //Max bandwith: (number max of player in this mode)*(packet size)*(tick by second)=49*16*1=735B/s
        case 10:
            next_send_timer->setInterval(1000);
        break;
        //Max bandwith: (number max of player in this mode)*(packet size)*(tick by second)=99*16*0.5=792B/s
        case 50:
            next_send_timer->setInterval(2000);
        break;
        //Max bandwith: (number max of player in this mode)*(packet size)*(tick by second)=999*16*0.1=1.6KB/s
        case 100:
            next_send_timer->setInterval(10000);
        break;
        //Max bandwith: (number max of player in this mode)*(packet size)*(tick by second)=65535*16*0.016=16.7KB/s
        case 1000:
            next_send_timer->setInterval(60000);
        break;
        default:
        break;
    }

    if(!next_send_timer->isActive())
        next_send_timer->start();
    #else
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
    #endif
}

void PlayerUpdater::internal_removeConnectedPlayer()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(connected_players>0)
    #endif
        connected_players--;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    switch(connected_players)
    {
        case (10-1):
            next_send_timer->setInterval(250);
        break;
        case (50-1):
            next_send_timer->setInterval(1000);
        break;
        case (100-1):
            next_send_timer->setInterval(2000);
        break;
        case (1000-1):
            next_send_timer->setInterval(10000);
        break;
        default:
        break;
    }
    if(!next_send_timer->isActive())
        next_send_timer->start();
    #else
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
    #endif
}

void PlayerUpdater::exec()
{
    if(GlobalServerData::serverSettings.sendPlayerNumber && sended_connected_players!=connected_players)
    {
        sended_connected_players=connected_players;
        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        *reinterpret_cast<uint16_t *>(Client::protocolMessageLogicalGroupAndServerList+Client::protocolMessageLogicalGroupAndServerListPosPlayerNumber)=htole16(connected_players);
        #endif

        #ifndef EPOLLCATCHCHALLENGERSERVER
        /*emit */newConnectedPlayer(connected_players);
        #else
        BroadCastWithoutSender::broadCastWithoutSender.receive_instant_player_number(connected_players);
        #endif
    }
}
