#include "QtServer.h"
#include "GlobalServerData.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_WithoutSender.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_Simple_StoreOnSender.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_None.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_WithBorder_StoreOnSender.h"
#include "LocalClientHandlerWithoutSender.h"
#include "ClientNetworkReadWithoutSender.h"

using namespace CatchChallenger;

void QtServer::preload_the_city_capture()
{
    connect(GlobalServerData::serverPrivateVariables.timer_city_capture,&QTimer::timeout,this,&QtServer::load_next_city_capture,Qt::QueuedConnection);
    connect(GlobalServerData::serverPrivateVariables.timer_city_capture,&QTimer::timeout,&Client::startTheCityCapture);
    connect(GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove,	&QTimer::timeout,this,&QtServer::send_insert_move_remove,Qt::QueuedConnection);
    if(GlobalServerData::serverSettings.database.secondToPositionSync>0)
        connect(&GlobalServerData::serverPrivateVariables.positionSync,&QTimer::timeout,this,&QtServer::positionSync,Qt::QueuedConnection);
    connect(&GlobalServerData::serverPrivateVariables.ddosTimer,&QTimer::timeout,this,&QtServer::ddosTimer,Qt::QueuedConnection);
    BaseServer::preload_the_city_capture();
}

void QtServer::start()
{
    need_be_started();
}

bool QtServer::isListen()
{
    return (stat==Up);
}

bool QtServer::isStopped()
{
    return (stat==Down);
}

void QtServer::stop()
{
    try_stop_server();
}

void QtServer::send_insert_move_remove()
{
    MapVisibilityAlgorithm_WithoutSender::mapVisibilityAlgorithm_WithoutSender.generalPurgeBuffer();
}

void QtServer::positionSync()
{
    LocalClientHandlerWithoutSender::localClientHandlerWithoutSender.doAllAction();
}

void QtServer::ddosTimer()
{
    LocalClientHandlerWithoutSender::localClientHandlerWithoutSender.doDDOSAction();
    BroadCastWithoutSender::broadCastWithoutSender.doDDOSAction();
    ClientNetworkReadWithoutSender::clientNetworkReadWithoutSender.doDDOSAction();
}

/////////////////////////////////////////////////// Object removing /////////////////////////////////////

void QtServer::removeOneClient()
{
/*    Client *client=qobject_cast<Client *>(QObject::sender());
    if(client==NULL)
    {
        DebugClass::debugConsole("removeOneClient(): NULL client at disconnection");
        return;
    }
    client_list.remove(client);
    delete client;*/
}

/////////////////////////////////////// player related //////////////////////////////////////

#ifndef EPOLLCATCHCHALLENGERSERVER
void QtServer::newConnection()
{
    while(QFakeServer::server.hasPendingConnections())
    {
        QFakeSocket *socket = QFakeServer::server.nextPendingConnection();
        if(socket!=NULL)
        {
            DebugClass::debugConsole(QStringLiteral("newConnection(): new client connected by fake socket"));
            switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
            {
                default:
                case MapVisibilityAlgorithmSelection_Simple:
                    connect_the_last_client(new MapVisibilityAlgorithm_Simple_StoreOnSender(new ConnectedSocket(socket)));
                break;
                case MapVisibilityAlgorithmSelection_WithBorder:
                    connect_the_last_client(new MapVisibilityAlgorithm_WithBorder_StoreOnSender(new ConnectedSocket(socket)));
                break;
                case MapVisibilityAlgorithmSelection_None:
                    connect_the_last_client(new MapVisibilityAlgorithm_None(new ConnectedSocket(socket)));
                break;
            }
        }
        else
            DebugClass::debugConsole("NULL client at BaseServer::newConnection()");
    }
}
#endif

void QtServer::connect_the_last_client(Client * client)
{
    client_list << client;
}

void QtServer::load_next_city_capture()
{
    BaseServer::load_next_city_capture();
}

bool QtServer::check_if_now_stopped()
{
    if(client_list.size()!=0)
        return false;
    if(stat!=InDown)
        return false;

    DebugClass::debugConsole("Fully stopped");
    if(GlobalServerData::serverPrivateVariables.db.isConnected())
    {
        DebugClass::debugConsole(QStringLiteral("Disconnected to %1 at %2")
                                 .arg(GlobalServerData::serverPrivateVariables.db_type_string)
                                 .arg(GlobalServerData::serverSettings.database.mysql.host));
        GlobalServerData::serverPrivateVariables.db.syncDisconnect();
    }
    stat=Down;
    is_started(false);

    unload_the_data();
    return true;
}

//call by normal stop
void QtServer::stop_internal_server()
{
    if(stat!=Up && stat!=InDown)
    {
        if(stat!=Down)
            DebugClass::debugConsole("Is in wrong stat for stopping: "+QString::number((int)stat));
        return;
    }
    DebugClass::debugConsole("Try stop");
    stat=InDown;

    QSetIterator<Client *> i(client_list);
     while (i.hasNext())
         i.next()->disconnectClient();
    QFakeServer::server.disconnectedSocket();
    QFakeServer::server.close();

    check_if_now_stopped();
}
