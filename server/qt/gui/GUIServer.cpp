#include "GUIServer.hpp"
#include <iostream>
#include "../base/GlobalServerData.hpp"
#include "../base/TinyXMLSettings.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include <QCoreApplication>
#include <QTcpSocket>
#include <QTcpSocket>
#include <QNetworkProxy>
#include <QProcess>
#include <algorithm>
#include <cstdlib>
#include "QtClientWithMap.hpp"
#include "QtClientList.hpp"

#ifdef CATCHCHALLENGER_SOLO
#include "QFakeServer.hpp"
#include "QFakeSocket.hpp"
#endif

#ifdef __linux__
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif

using namespace CatchChallenger;

bool GUIServer::oneInstanceRunning=false;

std::string GUIServer::text_restart="restart";
std::string GUIServer::text_stop="stop";

GUIServer::GUIServer() :
    QtServer()
{
    sslServer               = NULL;

    normalServerSettings.server_ip      = std::string();
    normalServerSettings.server_port    = 42489;

    //botThread = new EventThreader();
    //crash if this, due to different socket and thread
    //eventDispatcherThread = new EventThreader();
    //moveToThread(eventDispatcherThread);

/*    if(!connect(&BroadCastWithoutSender::broadCastWithoutSender,&BroadCastWithoutSender::serverCommand,this,&GUIServer::serverCommand,Qt::QueuedConnection))
        abort();
    if(!connect(&BroadCastWithoutSender::broadCastWithoutSender,&BroadCastWithoutSender::new_player_is_connected,this,&GUIServer::new_player_is_connected,Qt::QueuedConnection))
        abort();
    if(!connect(&BroadCastWithoutSender::broadCastWithoutSender,&BroadCastWithoutSender::player_is_disconnected,this,&GUIServer::player_is_disconnected,Qt::QueuedConnection))
        abort();
    if(!connect(&BroadCastWithoutSender::broadCastWithoutSender,&BroadCastWithoutSender::new_chat_message,this,&GUIServer::new_chat_message,Qt::QueuedConnection))
        abort();*/
    if(!connect(&purgeKickedHostTimer,&QTimer::timeout,this,&GUIServer::purgeKickedHost,Qt::QueuedConnection))
        abort();
    if(!connect(this,&QtServer::need_be_started,this,&GUIServer::start_internal_server,Qt::QueuedConnection))
        abort();
    if(!connect(&timeRangeEventTimer,&QTimer::timeout,this,&GUIServer::timeRangeEvent,Qt::QueuedConnection))
        abort();
    timeRangeEventTimer.start(1000*60*60*24);
}

/** call only when the server is down
 * \warning this function is thread safe because it quit all thread before remove */
GUIServer::~GUIServer()
{
    if(sslServer!=NULL)
    {
        sslServer->close();/// \warning crash due to different thread
        sslServer->deleteLater();
        sslServer=NULL;
    }
}

void GUIServer::setNormalSettings(const NormalServerSettings &settings)
{
    normalServerSettings=settings;
    loadAndFixSettings();
}

NormalServerSettings GUIServer::getNormalSettings() const
{
    return normalServerSettings;
}

void GUIServer::initAll()
{
    BaseServer::initAll();
}

//////////////////////////////////////////// server starting //////////////////////////////////////

// Parse one db-* group out of server-properties.xml into a Database struct.
// Mirrors the parser in unix/main-unix2.cpp::send_settings(): convert the
// "type" string ("sqlite" / "mysql" / "postgresql") into the DatabaseType
// enum, then pull host/db/login/pass for SQL or file= for SQLite. Caller is
// responsible for begin/endGroup() around the call.
static bool loadOneDbGroup(TinyXMLSettings *settings,
                           CatchChallenger::GameServerSettings::Database &dst,
                           const std::string &groupName)
{
    settings->beginGroup(groupName);
    const std::string typ = settings->value("type", "sqlite");
    if(typ == "mysql")
        dst.tryOpenType = CatchChallenger::DatabaseBase::DatabaseType::Mysql;
    else if(typ == "sqlite")
        dst.tryOpenType = CatchChallenger::DatabaseBase::DatabaseType::SQLite;
    else if(typ == "postgresql")
        dst.tryOpenType = CatchChallenger::DatabaseBase::DatabaseType::PostgreSQL;
    else
    {
        std::cerr << "[GUIServer::load_settings] unsupported db type "
                  << typ << " in " << groupName << "; falling back to sqlite"
                  << std::endl;
        dst.tryOpenType = CatchChallenger::DatabaseBase::DatabaseType::SQLite;
    }
    if(dst.tryOpenType == CatchChallenger::DatabaseBase::DatabaseType::SQLite)
    {
        // SQLite path is relative to the binary's working dir; keep it
        // pliable so QtDatabaseSQLite::syncConnect can resolve it.
        dst.file = settings->value("file", "database.sqlite");
    }
    else
    {
        dst.host  = settings->value("host", "localhost");
        dst.db    = settings->value("db",   "catchchallenger");
        dst.login = settings->value("login","catchchallenger");
        dst.pass  = settings->value("pass", "catchchallenger");
    }
    {
        const std::string s = settings->value("tryInterval", "5");
        dst.tryInterval = static_cast<unsigned int>(std::max(1, std::atoi(s.c_str())));
    }
    {
        const std::string s = settings->value("considerDownAfterNumberOfTry", "3");
        dst.considerDownAfterNumberOfTry = static_cast<unsigned int>(std::max(1, std::atoi(s.c_str())));
    }
    settings->endGroup();
    return true;
}

void GUIServer::load_settings()
{
    GlobalServerData::serverPrivateVariables.number_of_bots_logged = 0;

    const std::string xmlPath =
        QCoreApplication::applicationDirPath().toStdString()
        + "/server-properties.xml";
    TinyXMLSettings settings(xmlPath);

    // Datapack code: required for preload_6_sync_the_datapack — passing
    // an empty mainDatapackCode aborts inside CATCHCHALLENGER_HARDENED.
    // Mirror the seed logic from unix/main-unix2.cpp: read mainDatapackCode
    // from <content>, fall back to "[main]" → which means "auto-pick the
    // first directory under <datapack>/map/main/". Same idea for sub.
    settings.beginGroup("content");
    if(settings.contains("mainDatapackCode"))
        CommonSettingsServer::commonSettingsServer.mainDatapackCode = settings.value("mainDatapackCode", "[main]");
    else
        CommonSettingsServer::commonSettingsServer.mainDatapackCode = "[main]";
    if(CommonSettingsServer::commonSettingsServer.mainDatapackCode == "[main]")
    {
        const std::vector<CatchChallenger::FacilityLibGeneral::InodeDescriptor> &list =
            CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(
                GlobalServerData::serverSettings.datapack_basePath + "/map/main/",
                CatchChallenger::FacilityLibGeneral::ListFolder::Dirs);
        if(list.empty())
        {
            // No maincode dirs: log + emit error; do NOT abort. The GUI
            // will catch this via the error() signal and pop a
            // QMessageBox::warning so the operator can fix the datapack.
            std::cerr << "No main code detected into the current datapack" << std::endl;
            error("No main code detected in datapack: " + GlobalServerData::serverSettings.datapack_basePath + "map/main/");
        }
        else
        {
            settings.setValue("mainDatapackCode", list.at(0).name);
            CommonSettingsServer::commonSettingsServer.mainDatapackCode = list.at(0).name;
        }
    }
    if(settings.contains("subDatapackCode"))
        CommonSettingsServer::commonSettingsServer.subDatapackCode = settings.value("subDatapackCode", "");
    else
    {
        const std::vector<CatchChallenger::FacilityLibGeneral::InodeDescriptor> &list =
            CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(
                GlobalServerData::serverSettings.datapack_basePath + "/map/main/" +
                CommonSettingsServer::commonSettingsServer.mainDatapackCode + "/sub/",
                CatchChallenger::FacilityLibGeneral::ListFolder::Dirs);
        if(list.size() == 1)
        {
            settings.setValue("subDatapackCode", list.at(0).name);
            CommonSettingsServer::commonSettingsServer.subDatapackCode = list.at(0).name;
        }
        else
        {
            settings.setValue("subDatapackCode", "");
            CommonSettingsServer::commonSettingsServer.subDatapackCode = "";
        }
    }
    settings.endGroup();
    settings.sync();

    // Load DB settings from server-properties.xml. Without this the
    // database_*.tryOpenType fields stay at DatabaseType::Unknown and the
    // first switch() in BaseServer::syncConnectDb hits the default branch
    // (`std::cerr << "database type unknown"; abort()`). The MainWindow
    // already kicks NormalServerGlobal::checkSettingsFile() at construction
    // so the file is guaranteed to exist with seeded defaults.
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    loadOneDbGroup(&settings, GlobalServerData::serverSettings.database_login, "db-login");
    loadOneDbGroup(&settings, GlobalServerData::serverSettings.database_base,  "db-base");
    #endif
    loadOneDbGroup(&settings, GlobalServerData::serverSettings.database_common,"db-common");
    loadOneDbGroup(&settings, GlobalServerData::serverSettings.database_server,"db-server");
    #endif
}

//start with allow real player to connect
void GUIServer::start_internal_server()
{
    // Cleartext-only listen path. The qmake-era code branched on a
    // server-side `useSsl` setting and constructed QSslServer with a
    // self-signed cert when it was on; that branch (and the openssl
    // sub-process that generated the cert on first launch) is gone,
    // along with the 1-byte SSL preamble the server used to write at
    // accept-time. TLS deployments now belong to a separate listener
    // (e.g. an external reverse proxy) rather than being negotiated
    // in-band by the protocol.
    if(sslServer==NULL)
        sslServer=new QTcpServer();
    if(!connect(sslServer,&QTcpServer::newConnection,this,&GUIServer::newConnection,Qt::QueuedConnection))
        abort();
    if(sslServer->isListening())
    {
        const std::string &listenAddressAndPort=listenIpAndPort(sslServer->serverAddress().toString().toStdString(),sslServer->serverPort());
        std::cerr << "Already listening on " <<  listenAddressAndPort << std::endl;
        error("Already listening on "+listenAddressAndPort);
        return;
    }
    if(oneInstanceRunning)
    {
        std::cerr << "Other instance already running" << std::endl;
        return;
    }
    if(stat!=Down)
    {
        std::cerr << "In wrong stat" << std::endl;
        return;
    }
    stat=InUp;
    load_settings();
    QHostAddress address = QHostAddress::Any;
    if(!normalServerSettings.server_ip.empty())
        address.setAddress(QString::fromStdString(normalServerSettings.server_ip));
    if(!normalServerSettings.proxy.empty())
    {
        QNetworkProxy proxy=sslServer->proxy();
        proxy.setType(QNetworkProxy::Socks5Proxy);
        proxy.setHostName(QString::fromStdString(normalServerSettings.proxy));
        proxy.setPort(normalServerSettings.proxy_port);
        sslServer->setProxy(proxy);
    }
    if(!sslServer->listen(address,normalServerSettings.server_port))
    {
        const std::string &listenAddressAndPort=listenIpAndPort(sslServer->serverAddress().toString().toStdString(),sslServer->serverPort());
        std::cerr << "Unable to listen: " << listenAddressAndPort << ", errror: " << sslServer->errorString().toStdString() << std::endl;
        stat=Down;
        is_started(false);
        error("Unable to listen: "+listenAddressAndPort+", errror: "+sslServer->errorString().toStdString());
        return;
    }
    #ifdef CATCHCHALLENGER_SOLO
    if(!QFakeServer::server.listen())
    {
        std::cerr << "Unable to listen the internal server" << std::endl;
        stat=Down;
        is_started(false);
        error("Unable to listen the internal server");
        return;
    }
    #endif

    if(normalServerSettings.server_ip.empty())
        std::cout << "Listen *:" << std::to_string(normalServerSettings.server_port) << std::endl;
    else
        std::cout << "Listen "+normalServerSettings.server_ip+":"+std::to_string(normalServerSettings.server_port) << std::endl;

    if(!initialize_the_database())
    {
        // Make the failure visible. The dominant real-world cause is a
        // missing Qt SQL driver plugin (sqldrivers/qsqlite.dll not
        // shipped next to the .exe → "Driver not loaded"); without an
        // error() emit the GUI just silently stays "Running" and the
        // operator reports it as a crash. Prefer the concrete DB error
        // message, append the actionable hint.
        std::string dbMsg;
        if(GlobalServerData::serverPrivateVariables.db_common!=NULL)
            dbMsg=GlobalServerData::serverPrivateVariables.db_common->errorMessage();
        if(dbMsg.empty() && GlobalServerData::serverPrivateVariables.db_server!=NULL)
            dbMsg=GlobalServerData::serverPrivateVariables.db_server->errorMessage();
        std::string full="Database initialization failed";
        if(!dbMsg.empty())
            full+=": "+dbMsg;
        if(dbMsg.find("river not loaded")!=std::string::npos ||
           dbMsg.find("river not available")!=std::string::npos)
            full+=" — the Qt SQL driver plugin is missing "
                  "(reinstall; sqldrivers/qsqlite.dll must sit next to "
                  "the executable).";
        std::cerr << full << std::endl;
        error(full);
        sslServer->close();
        stat=Down;
        is_started(false);
        return;
    }
    preload_1_the_data();
    stat=Up;
    oneInstanceRunning=true;
    is_started(true);
    return;
}

////////////////////////////////////////////////// server stopping ////////////////////////////////////////////

bool GUIServer::check_if_now_stopped()
{
    if(!QtServer::check_if_now_stopped())
        return false;
    oneInstanceRunning=false;
    if(sslServer!=NULL)
    {
        sslServer->close();
        delete sslServer;
        sslServer=NULL;
    }
    return true;
}

//call by normal stop
void GUIServer::stop_internal_server()
{
    QtServer::stop_internal_server();

    if(sslServer!=NULL)
    {
        sslServer->close();
        delete sslServer;
        sslServer=NULL;
    }
}

/////////////////////////////////////////////////// Object removing /////////////////////////////////////

void GUIServer::removeOneClient()
{
    /*Client *client=qobject_cast<Client *>(QObject::sender());
    if(client==NULL)
    {
        DebugClass::debugConsole("removeOneClient(): NULL client at disconnection");
        return;
    }
    client_list.remove(client);
    delete client;
    check_if_now_stopped();*/
}

///////////////////////////////////// Generic command //////////////////////////////////

void GUIServer::serverCommand(const std::string &command, const std::string &extraText)
{
    Q_UNUSED(command);
    Q_UNUSED(extraText);
    /*Client *client=qobject_cast<Client *>(QObject::sender());
    if(client==NULL)
    {
        DebugClass::debugConsole("NULL client at serverCommand()");
        return;
    }
    if(command==GUIServer::text_restart)
        need_be_restarted();
    else if(command==GUIServer::text_stop)
        need_be_stopped();
    else
        DebugClass::debugConsole(std::stringLiteral("unknow command: %1").arg(command));*/
}

//////////////////////////////////// Function secondary //////////////////////////////
std::string GUIServer::listenIpAndPort(std::string server_ip,uint16_t server_port)
{
    if(server_ip.empty())
        server_ip="*";
    return server_ip+":"+std::to_string(server_port);
}

void GUIServer::newConnection()
{
    CatchChallenger::QtClientList *qtClientList=static_cast<CatchChallenger::QtClientList *>(CatchChallenger::ClientList::list);
    #ifdef CATCHCHALLENGER_SOLO
    while(QFakeServer::server.hasPendingConnections())
    {
        QFakeSocket *socket = QFakeServer::server.nextPendingConnection();
        if(socket!=NULL)
        {
            std::cout << "new client connected on internal socket" << std::endl;
            const PLAYER_INDEX_FOR_CONNECTED index=qtClientList->insert(nullptr);
            QtClientWithMap *client=new QtClientWithMap(socket,index);
            CatchChallenger::QtClientList::clients[index]=client;
            connect_the_last_client(client,socket);
        }
        else
            std::cout << "NULL client with fake socket" << std::endl;
    }
    #endif
    if(sslServer!=NULL)
        while(sslServer->hasPendingConnections())
        {
            // Plain QTcpSocket — the QSslSocket cast disappeared along
            // with the QSslServer wrapper; the wire is cleartext.
            QTcpSocket *socket = sslServer->nextPendingConnection();
            const QHostAddress &peerAddress=socket->peerAddress();
            bool kicked=kickedHosts.contains(peerAddress);
            if(kicked)
                if((QDateTime::currentDateTime().toSecsSinceEpoch()-kickedHosts.value(peerAddress).toSecsSinceEpoch())>=(uint32_t)CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick)
                {
                    kickedHosts.remove(peerAddress);
                    kicked=false;
                }
            if(!kicked)
            {
                if(socket!=NULL)
                {
                    const PLAYER_INDEX_FOR_CONNECTED index=qtClientList->insert(nullptr);
                    QtClientWithMap *client=new QtClientWithMap(socket,index);
                    CatchChallenger::QtClientList::clients[index]=client;
                    connect_the_last_client(client,socket);
                }
                else
                    std::cerr << "NULL client: " << socket->peerAddress().toString().toStdString() << std::endl;
            }
            else
                socket->disconnectFromHost();
        }
}

void GUIServer::kicked(const QHostAddress &host)
{
    if(CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick>0)
        kickedHosts[host]=QDateTime::currentDateTime();
}

void GUIServer::purgeKickedHost()
{
    std::vector<QHostAddress> hostsToRemove;
    const QDateTime &currentDateTime=QDateTime::currentDateTime();
    QHashIterator<QHostAddress,QDateTime> i(kickedHosts);
    while (i.hasNext()) {
        i.next();
        if((currentDateTime.toSecsSinceEpoch()-i.value().toSecsSinceEpoch())>=(uint32_t)CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick)
            hostsToRemove.push_back(i.key());
    }
    unsigned int index=0;
    while(index<hostsToRemove.size())
    {
        kickedHosts.remove(hostsToRemove.at(index));
        index++;
    }
}

void GUIServer::timeRangeEvent()
{
    Client::timeRangeEvent(QDateTime::currentMSecsSinceEpoch()/1000);
}

bool GUIServer::isListen()
{
    return QtServer::isListen();
}

bool GUIServer::isStopped()
{
    return QtServer::isStopped();
}

uint16_t GUIServer::player_current()
{
    if(ClientList::list==nullptr)
        return 0;
    return ClientList::list->connected_size();
}

uint16_t GUIServer::player_max()
{
    return GlobalServerData::serverSettings.max_players;
}

/////////////////////////////////// Async the call ///////////////////////////////////
/// \brief Called when event loop is setup
void GUIServer::start_server()
{
    need_be_started();
}

void GUIServer::stop_server()
{
    try_stop_server();
}

void GUIServer::loadAndFixSettings()
{
    if(normalServerSettings.server_port<=0)
    {
        normalServerSettings.server_port=42489;
        std::cerr << "normalServerSettings.server_port<=0 fix by 42489" << std::endl;
    }
    if(normalServerSettings.proxy_port<=0)
    {
        normalServerSettings.proxy=std::string();
        normalServerSettings.server_port=8080;
        std::cerr << "normalServerSettings.proxy_port<=0 fix by 8080" << std::endl;
    }
}

void GUIServer::preload_finish()
{
    QtServer::preload_finish();
    std::cerr << "Waiting connection on port " << normalServerSettings.server_port << std::endl;
}
