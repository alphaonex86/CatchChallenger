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
