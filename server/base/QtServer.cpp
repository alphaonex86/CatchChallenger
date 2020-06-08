#include "QtServer.hpp"
#include "GlobalServerData.hpp"
#include "ClientMapManagement/MapVisibilityAlgorithm_WithoutSender.hpp"
#include "ClientMapManagement/MapVisibilityAlgorithm_Simple_StoreOnSender.hpp"
#include "ClientMapManagement/MapVisibilityAlgorithm_None.hpp"
#include "ClientMapManagement/MapVisibilityAlgorithm_WithBorder_StoreOnSender.hpp"
#include "LocalClientHandlerWithoutSender.hpp"
#include "ClientNetworkReadWithoutSender.hpp"
#ifdef CATCHCHALLENGER_SOLO
#include "../../client/qt/QFakeServer.hpp"
#endif

using namespace CatchChallenger;

QtServer::QtServer()
{
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<QSqlDatabase>("QSqlDatabase");
    qRegisterMetaType<QSqlQuery>("QSqlQuery");
    connect(GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove,	&QTimer::timeout,this,&QtServer::send_insert_move_remove,Qt::QueuedConnection);
    if(GlobalServerData::serverSettings.secondToPositionSync>0)
        if(!connect(&GlobalServerData::serverPrivateVariables.positionSync,&QTimer::timeout,this,&QtServer::positionSync,Qt::QueuedConnection))
        {
            std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
    if(!connect(&GlobalServerData::serverPrivateVariables.ddosTimer,&QTimer::timeout,this,&QtServer::ddosTimer,Qt::QueuedConnection))
    {
        std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    #ifdef CATCHCHALLENGER_SOLO
    if(!connect(&QFakeServer::server,&QFakeServer::newConnection,this,&QtServer::newConnection))
    {
        std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    #endif
    if(!connect(this,&QtServer::try_stop_server,this,&QtServer::stop_internal_server))
    {
        std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    GlobalServerData::serverPrivateVariables.db_login=new QtDatabase();
    GlobalServerData::serverPrivateVariables.db_login->setObjectName("db_login");
    GlobalServerData::serverPrivateVariables.db_login->dbThread.setObjectName("db_login");
    GlobalServerData::serverPrivateVariables.db_base=new QtDatabase();
    GlobalServerData::serverPrivateVariables.db_base->setObjectName("db_base");
    GlobalServerData::serverPrivateVariables.db_base->dbThread.setObjectName("db_base");
    GlobalServerData::serverPrivateVariables.db_common=new QtDatabase();
    GlobalServerData::serverPrivateVariables.db_common->setObjectName("db_common");
    GlobalServerData::serverPrivateVariables.db_common->dbThread.setObjectName("db_common");
    GlobalServerData::serverPrivateVariables.db_server=new QtDatabase();
    GlobalServerData::serverPrivateVariables.db_server->setObjectName("db_server");
    GlobalServerData::serverPrivateVariables.db_server->dbThread.setObjectName("db_server");
    if(!connect(&GlobalServerData::serverPrivateVariables.player_updater,&PlayerUpdater::newConnectedPlayer,
                &BroadCastWithoutSender::broadCastWithoutSender,&BroadCastWithoutSender::receive_instant_player_number,
                Qt::QueuedConnection))
    {
        std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    if(!connect(&GlobalServerData::serverPrivateVariables.timeRangeEventScan,&TimeRangeEventScan::timeRangeEventTrigger,
                &BroadCastWithoutSender::broadCastWithoutSender,&BroadCastWithoutSender::timeRangeEventTrigger,
                Qt::QueuedConnection))
    {
        std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
}

QtServer::~QtServer()
{
    auto i=client_list.begin();
    while(i!=client_list.cend())
    {
        ClientWithSocket *client=(*i);
        client->disconnectClient();
        delete client;
        ++i;
    }
    client_list.clear();
    if(GlobalServerData::serverPrivateVariables.db_server!=NULL)
    {
        GlobalServerData::serverPrivateVariables.db_server->syncDisconnect();
        delete GlobalServerData::serverPrivateVariables.db_server;
        GlobalServerData::serverPrivateVariables.db_server=NULL;
    }
    if(GlobalServerData::serverPrivateVariables.db_common!=NULL)
    {
        GlobalServerData::serverPrivateVariables.db_common->syncDisconnect();
        delete GlobalServerData::serverPrivateVariables.db_common;
        GlobalServerData::serverPrivateVariables.db_common=NULL;
    }
    if(GlobalServerData::serverPrivateVariables.db_base!=NULL)
    {
        GlobalServerData::serverPrivateVariables.db_base->syncDisconnect();
        delete GlobalServerData::serverPrivateVariables.db_base;
        GlobalServerData::serverPrivateVariables.db_base=NULL;
    }
    if(GlobalServerData::serverPrivateVariables.db_login!=NULL)
    {
        GlobalServerData::serverPrivateVariables.db_login->syncDisconnect();
        delete GlobalServerData::serverPrivateVariables.db_login;
        GlobalServerData::serverPrivateVariables.db_login=NULL;
    }
}

void QtServer::preload_the_city_capture()
{
    if(!connect(GlobalServerData::serverPrivateVariables.timer_city_capture,&QTimer::timeout,this,&QtServer::load_next_city_capture,Qt::QueuedConnection))
        abort();
    if(!connect(GlobalServerData::serverPrivateVariables.timer_city_capture,&QTimer::timeout,&Client::startTheCityCapture))
        abort();
    BaseServer::preload_the_city_capture();
}

void QtServer::preload_finish()
{
    BaseServer::preload_finish();
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
    QIODevice *socket=qobject_cast<QIODevice *>(QObject::sender());
    if(socket==NULL)
    {
        qDebug() << ("removeOneClient(): NULL ClientWithSocket at disconnection");
        return;
    }
    for(ClientWithSocket * const client : client_list)
    {
        if(client->qtSocket==socket)
        {
            delete client;
            client_list.erase(client);
            break;
        }
    }
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
            qDebug() << ("newConnection(): new ClientWithSocket connected by fake socket");
            ClientWithSocket *client=nullptr;
            switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
            {
                case MapVisibilityAlgorithmSelection_Simple:
                    client=new MapVisibilityAlgorithm_Simple_StoreOnSender();
                    client->qtSocket=new ConnectedSocket(socket);
                break;
                case MapVisibilityAlgorithmSelection_WithBorder:
                    client=new MapVisibilityAlgorithm_WithBorder_StoreOnSender();
                    client->qtSocket=new ConnectedSocket(socket);
                break;
                default:
                case MapVisibilityAlgorithmSelection_None:
                    client=new MapVisibilityAlgorithm_None();
                    client->qtSocket=new ConnectedSocket(socket);
                break;
            }
            connect_the_last_client(client,socket);

            QByteArray data;
            data.resize(1);
            data[0x00]=0x00;
            socket->write(data.constData(),data.size());
        }
        else
            qDebug() << ("NULL ClientWithSocket at BaseServer::newConnection()");
    }
}
#endif

void QtServer::connect_the_last_client(ClientWithSocket * client,QIODevice *socket)
{
    if(!connect(socket,&QIODevice::readyRead,client,&ClientWithSocket::parseIncommingData,Qt::QueuedConnection))
        abort();
    if(!connect(socket,&QObject::destroyed,this,&QtServer::removeOneClient,Qt::DirectConnection))
        abort();
    client->qtSocket=socket;
    client_list.insert(client);
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

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    #ifdef CATCHCHALLENGER_SOLO
    if(GlobalServerData::serverPrivateVariables.db_server!=NULL && GlobalServerData::serverPrivateVariables.db_server->isConnected())
        if(GlobalServerData::serverPrivateVariables.db_server->databaseType()!=DatabaseBase::DatabaseType::SQLite)
        {
            std::cerr << "Disconnected to incorrect database type for solo: " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_server->databaseType()) << std::endl;
            abort();
        }
    if(GlobalServerData::serverPrivateVariables.db_common!=NULL && GlobalServerData::serverPrivateVariables.db_common->isConnected())
        if(GlobalServerData::serverPrivateVariables.db_common->databaseType()!=DatabaseBase::DatabaseType::SQLite)
        {
            std::cerr << "Disconnected to incorrect database type for solo: " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_common->databaseType()) << std::endl;
            abort();
        }
    if(GlobalServerData::serverPrivateVariables.db_login!=NULL && GlobalServerData::serverPrivateVariables.db_login->isConnected())
        if(GlobalServerData::serverPrivateVariables.db_login->databaseType()!=DatabaseBase::DatabaseType::SQLite)
        {
            std::cerr << "Disconnected to incorrect database type for solo: " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_login->databaseType()) << std::endl;
            abort();
        }
    if(GlobalServerData::serverPrivateVariables.db_base!=NULL && GlobalServerData::serverPrivateVariables.db_base->isConnected())
        if(GlobalServerData::serverPrivateVariables.db_base->databaseType()!=DatabaseBase::DatabaseType::SQLite)
        {
            std::cerr << "Disconnected to incorrect database type for solo: " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_base->databaseType()) << std::endl;
            abort();
        }
    #endif
    #endif
    qDebug() << ("Fully stopped");
    if(GlobalServerData::serverPrivateVariables.db_login->isConnected())
    {
        qDebug() << (QStringLiteral("Disconnected to %1 at %2")
                                 .arg(QString::fromStdString(DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_login->databaseType())))
                                 .arg(QString::fromStdString(GlobalServerData::serverSettings.database_login.host)));
        GlobalServerData::serverPrivateVariables.db_login->syncDisconnect();
    }
    if(GlobalServerData::serverPrivateVariables.db_base->isConnected())
    {
        qDebug() << (QStringLiteral("Disconnected to %1 at %2")
                                 .arg(QString::fromStdString(DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_base->databaseType())))
                                 .arg(QString::fromStdString(GlobalServerData::serverSettings.database_base.host)));
        GlobalServerData::serverPrivateVariables.db_base->syncDisconnect();
    }
    if(GlobalServerData::serverPrivateVariables.db_common->isConnected())
    {
        qDebug() << (QStringLiteral("Disconnected to %1 at %2")
                                 .arg(QString::fromStdString(DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_common->databaseType())))
                                 .arg(QString::fromStdString(GlobalServerData::serverSettings.database_common.host)));
        GlobalServerData::serverPrivateVariables.db_common->syncDisconnect();
    }
    if(GlobalServerData::serverPrivateVariables.db_server->isConnected())
    {
        qDebug() << (QStringLiteral("Disconnected to %1 at %2")
                                 .arg(QString::fromStdString(DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_server->databaseType())))
                                 .arg(QString::fromStdString(GlobalServerData::serverSettings.database_server.host)));
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
            qDebug() << ("Is in wrong stat for stopping: "+QString::number((int)stat));
        return;
    }
    qDebug() << ("Try stop");
    stat=InDown;

    //#ifndef CATCHCHALLENGER_SOLO
    auto i=client_list.begin();
    while(i!=client_list.cend())
    {
        ClientWithSocket * client=*i;
        client->disconnectClient();
        delete client;
        ++i;
    }
    //#endif
    client_list.clear();
    #ifdef CATCHCHALLENGER_SOLO
    QFakeServer::server.disconnectedSocket();
    QFakeServer::server.close();
    #endif

    check_if_now_stopped();
}

void QtServer::quitForCriticalDatabaseQueryFailed()
{
    emit haveQuitForCriticalDatabaseQueryFailed();
    stop_internal_server();
}
