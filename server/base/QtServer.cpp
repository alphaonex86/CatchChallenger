#include "QtServer.h"
#include "GlobalServerData.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_WithoutSender.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_Simple_StoreOnSender.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_None.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_WithBorder_StoreOnSender.h"
#include "LocalClientHandlerWithoutSender.h"
#include "ClientNetworkReadWithoutSender.h"

using namespace CatchChallenger;

QtServer::QtServer()
{
    qRegisterMetaType<QSqlDatabase>("QSqlDatabase");
    qRegisterMetaType<QSqlQuery>("QSqlQuery");
    connect(GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove,	&QTimer::timeout,this,&QtServer::send_insert_move_remove,Qt::QueuedConnection);
    if(GlobalServerData::serverSettings.secondToPositionSync>0)
        connect(&GlobalServerData::serverPrivateVariables.positionSync,&QTimer::timeout,this,&QtServer::positionSync,Qt::QueuedConnection);
    connect(&GlobalServerData::serverPrivateVariables.ddosTimer,&QTimer::timeout,this,&QtServer::ddosTimer,Qt::QueuedConnection);
    connect(&QFakeServer::server,&QFakeServer::newConnection,this,&QtServer::newConnection);
    connect(this,&QtServer::try_stop_server,this,&QtServer::stop_internal_server);
    GlobalServerData::serverPrivateVariables.db_login=new QtDatabase();
    GlobalServerData::serverPrivateVariables.db_base=new QtDatabase();
    GlobalServerData::serverPrivateVariables.db_common=new QtDatabase();
    GlobalServerData::serverPrivateVariables.db_server=new QtDatabase();
}

QtServer::~QtServer()
{
    std::unordered_setIterator<Client *> i(client_list);
    while (i.hasNext())
    {
        Client *client=i.next();
        client->disconnectClient();
        delete client;
    }
    client_list.clear();
    delete GlobalServerData::serverPrivateVariables.db_server;
    delete GlobalServerData::serverPrivateVariables.db_common;
    delete GlobalServerData::serverPrivateVariables.db_base;
    delete GlobalServerData::serverPrivateVariables.db_login;
}

void QtServer::preload_the_city_capture()
{
    connect(GlobalServerData::serverPrivateVariables.timer_city_capture,&QTimer::timeout,this,&QtServer::load_next_city_capture,Qt::QueuedConnection);
    connect(GlobalServerData::serverPrivateVariables.timer_city_capture,&QTimer::timeout,&Client::startTheCityCapture);
    BaseServer::preload_the_city_capture();
}

void QtServer::preload_finish()
{
    emit is_started(true);
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
    LocalClientHandlerWithoutSender::localClientHandlerWithoutSender.doAllAction();
    BroadCastWithoutSender::broadCastWithoutSender.doDDOSChat();
    ClientNetworkReadWithoutSender::clientNetworkReadWithoutSender.doDDOSAction();
}

/////////////////////////////////////////////////// Object removing /////////////////////////////////////

void QtServer::removeOneClient()
{
    Client *client=qobject_cast<Client *>(QObject::sender());
    if(client==NULL)
    {
        DebugClass::debugConsole("removeOneClient(): NULL client at disconnection");
        return;
    }
    client_list.remove(client);
    delete client;
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
            DebugClass::debugConsole(std::stringLiteral("newConnection(): new client connected by fake socket"));
            switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
            {
                default:
                case MapVisibilityAlgorithmSelection_Simple:
                    connect_the_last_client(new MapVisibilityAlgorithm_Simple_StoreOnSender(new ConnectedSocket(socket)),socket);
                break;
                case MapVisibilityAlgorithmSelection_WithBorder:
                    connect_the_last_client(new MapVisibilityAlgorithm_WithBorder_StoreOnSender(new ConnectedSocket(socket)),socket);
                break;
                case MapVisibilityAlgorithmSelection_None:
                    connect_the_last_client(new MapVisibilityAlgorithm_None(new ConnectedSocket(socket)),socket);
                break;
            }
        }
        else
            DebugClass::debugConsole("NULL client at BaseServer::newConnection()");
    }
}
#endif

void QtServer::connect_the_last_client(Client * client,QIODevice *socket)
{
    connect(socket,&QIODevice::readyRead,client,&Client::parseIncommingData,Qt::QueuedConnection);
    connect(client,&QObject::destroyed,this,&QtServer::removeOneClient,Qt::DirectConnection);
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
    if(GlobalServerData::serverPrivateVariables.db_login->isConnected())
    {
        DebugClass::debugConsole(std::stringLiteral("Disconnected to %1 at %2")
                                 .arg(DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_login->databaseType()))
                                 .arg(GlobalServerData::serverSettings.database_login.host));
        GlobalServerData::serverPrivateVariables.db_login->syncDisconnect();
    }
    if(GlobalServerData::serverPrivateVariables.db_base->isConnected())
    {
        DebugClass::debugConsole(std::stringLiteral("Disconnected to %1 at %2")
                                 .arg(DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_base->databaseType()))
                                 .arg(GlobalServerData::serverSettings.database_base.host));
        GlobalServerData::serverPrivateVariables.db_base->syncDisconnect();
    }
    if(GlobalServerData::serverPrivateVariables.db_common->isConnected())
    {
        DebugClass::debugConsole(std::stringLiteral("Disconnected to %1 at %2")
                                 .arg(DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_common->databaseType()))
                                 .arg(GlobalServerData::serverSettings.database_common.host));
        GlobalServerData::serverPrivateVariables.db_common->syncDisconnect();
    }
    if(GlobalServerData::serverPrivateVariables.db_server->isConnected())
    {
        DebugClass::debugConsole(std::stringLiteral("Disconnected to %1 at %2")
                                 .arg(DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_server->databaseType()))
                                 .arg(GlobalServerData::serverSettings.database_server.host));
        GlobalServerData::serverPrivateVariables.db_server->syncDisconnect();
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
            DebugClass::debugConsole("Is in wrong stat for stopping: "+std::string::number((int)stat));
        return;
    }
    DebugClass::debugConsole("Try stop");
    stat=InDown;

    std::unordered_setIterator<Client *> i(client_list);
    while (i.hasNext())
    {
        Client * client=i.next();
        client->disconnectClient();
        delete client;
    }
    client_list.clear();
    QFakeServer::server.disconnectedSocket();
    QFakeServer::server.close();

    check_if_now_stopped();
}

void QtServer::quitForCriticalDatabaseQueryFailed()
{
    emit haveQuitForCriticalDatabaseQueryFailed();
    stop_internal_server();
}
