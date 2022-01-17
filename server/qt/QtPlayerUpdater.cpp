#include "QtPlayerUpdater.hpp"
#include "GlobalServerData.hpp"
#include "Client.hpp"

#ifdef EPOLLCATCHCHALLENGERSERVER
#include "BroadCastWithoutSender.hpp"
#endif

using namespace CatchChallenger;

QtQtPlayerUpdater::QtPlayerUpdater()
      #ifndef EPOLLCATCHCHALLENGERSERVER
      :
        next_send_timer(NULL)
      #endif
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    connect(this,&QtQtPlayerUpdater::send_addConnectedPlayer,this,&QtQtPlayerUpdater::internal_addConnectedPlayer,Qt::QueuedConnection);
    connect(this,&QtQtPlayerUpdater::send_removeConnectedPlayer,this,&QtQtPlayerUpdater::internal_removeConnectedPlayer,Qt::QueuedConnection);

    connect(this,&QtPlayerUpdater::try_initAll,                  this,&QtPlayerUpdater::initAll,              Qt::QueuedConnection);
    /*emit */try_initAll();
    #else
    initAll();
    #endif
}

void QtPlayerUpdater::addConnectedPlayer()
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    /*emit */send_addConnectedPlayer();
    #else
    internal_addConnectedPlayer();
    #endif
}

void QtPlayerUpdater::removeConnectedPlayer()
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    /*emit */send_removeConnectedPlayer();
    #else
    internal_removeConnectedPlayer();
    #endif
}

void QtPlayerUpdater::initAll()
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(next_send_timer==NULL)
    {
        next_send_timer=new QTimer();
        next_send_timer->setSingleShot(true);

        //Max bandwith: (number max of player in this mode)*(packet size)*(tick by second)=9*16*4=576B/s
        next_send_timer->setInterval(250);

        connect(next_send_timer,&QTimer::timeout,this,&QtPlayerUpdater::exec,Qt::QueuedConnection);
    }
    #else
    setInterval(250);
    #endif
}

void QtPlayerUpdater::internal_addConnectedPlayer()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(connected_players<GlobalServerData::serverSettings.max_players)
    #endif
        connected_players++;
    if(!next_send_timer->isActive())
        next_send_timer->start();
}
def set 
next_send_timer->setInterval(250);

void QtPlayerUpdater::internal_removeConnectedPlayer()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(connected_players>0)
    #endif
        connected_players--;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(!next_send_timer->isActive())
        next_send_timer->start();
    #endif
}

void QtPlayerUpdater::exec()
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
