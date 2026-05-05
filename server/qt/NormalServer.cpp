#include "NormalServer.hpp"
#include "../base/GlobalServerData.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include <QTcpSocket>
#include <QTcpSocket>
#include <QNetworkProxy>
#include <QProcess>
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

bool NormalServer::oneInstanceRunning=false;

std::string NormalServer::text_restart="restart";
std::string NormalServer::text_stop="stop";

NormalServer::NormalServer() :
    QtServer()
{
    sslServer               = NULL;

    normalServerSettings.server_ip      = std::string();
    normalServerSettings.server_port    = 42489;

    //botThread = new EventThreader();
    //crash if this, due to different socket and thread
    //eventDispatcherThread = new EventThreader();
    //moveToThread(eventDispatcherThread);

/*    if(!connect(&BroadCastWithoutSender::broadCastWithoutSender,&BroadCastWithoutSender::serverCommand,this,&NormalServer::serverCommand,Qt::QueuedConnection))
        abort();
    if(!connect(&BroadCastWithoutSender::broadCastWithoutSender,&BroadCastWithoutSender::new_player_is_connected,this,&NormalServer::new_player_is_connected,Qt::QueuedConnection))
        abort();
    if(!connect(&BroadCastWithoutSender::broadCastWithoutSender,&BroadCastWithoutSender::player_is_disconnected,this,&NormalServer::player_is_disconnected,Qt::QueuedConnection))
        abort();
    if(!connect(&BroadCastWithoutSender::broadCastWithoutSender,&BroadCastWithoutSender::new_chat_message,this,&NormalServer::new_chat_message,Qt::QueuedConnection))
        abort();*/
    if(!connect(&purgeKickedHostTimer,&QTimer::timeout,this,&NormalServer::purgeKickedHost,Qt::QueuedConnection))
        abort();
    if(!connect(this,&QtServer::need_be_started,this,&NormalServer::start_internal_server,Qt::QueuedConnection))
        abort();
    if(!connect(&timeRangeEventTimer,&QTimer::timeout,this,&NormalServer::timeRangeEvent,Qt::QueuedConnection))
        abort();
    timeRangeEventTimer.start(1000*60*60*24);
}

/** call only when the server is down
 * \warning this function is thread safe because it quit all thread before remove */
NormalServer::~NormalServer()
{
    if(sslServer!=NULL)
    {
        sslServer->close();/// \warning crash due to different thread
        sslServer->deleteLater();
        sslServer=NULL;
    }
}

void NormalServer::setNormalSettings(const NormalServerSettings &settings)
{
    normalServerSettings=settings;
    loadAndFixSettings();
}

NormalServerSettings NormalServer::getNormalSettings() const
{
    return normalServerSettings;
}

void NormalServer::initAll()
{
    BaseServer::initAll();
}

//////////////////////////////////////////// server starting //////////////////////////////////////

void NormalServer::load_settings()
{
    GlobalServerData::serverPrivateVariables.number_of_bots_logged= 0;
}

//start with allow real player to connect
void NormalServer::start_internal_server()
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
    if(!connect(sslServer,&QTcpServer::newConnection,this,&NormalServer::newConnection,Qt::QueuedConnection))
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

bool NormalServer::check_if_now_stopped()
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
void NormalServer::stop_internal_server()
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

void NormalServer::removeOneClient()
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

void NormalServer::serverCommand(const std::string &command, const std::string &extraText)
{
    Q_UNUSED(command);
    Q_UNUSED(extraText);
    /*Client *client=qobject_cast<Client *>(QObject::sender());
    if(client==NULL)
    {
        DebugClass::debugConsole("NULL client at serverCommand()");
        return;
    }
    if(command==NormalServer::text_restart)
        need_be_restarted();
    else if(command==NormalServer::text_stop)
        need_be_stopped();
    else
        DebugClass::debugConsole(std::stringLiteral("unknow command: %1").arg(command));*/
}

//////////////////////////////////// Function secondary //////////////////////////////
std::string NormalServer::listenIpAndPort(std::string server_ip,uint16_t server_port)
{
    if(server_ip.empty())
        server_ip="*";
    return server_ip+":"+std::to_string(server_port);
}

void NormalServer::newConnection()
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

void NormalServer::kicked(const QHostAddress &host)
{
    if(CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick>0)
        kickedHosts[host]=QDateTime::currentDateTime();
}

void NormalServer::purgeKickedHost()
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

void NormalServer::timeRangeEvent()
{
    Client::timeRangeEvent(QDateTime::currentMSecsSinceEpoch()/1000);
}

bool NormalServer::isListen()
{
    return QtServer::isListen();
}

bool NormalServer::isStopped()
{
    return QtServer::isStopped();
}

uint16_t NormalServer::player_current()
{
    if(ClientList::list==nullptr)
        return 0;
    return ClientList::list->connected_size();
}

uint16_t NormalServer::player_max()
{
    return GlobalServerData::serverSettings.max_players;
}

/////////////////////////////////// Async the call ///////////////////////////////////
/// \brief Called when event loop is setup
void NormalServer::start_server()
{
    need_be_started();
}

void NormalServer::stop_server()
{
    try_stop_server();
}

void NormalServer::loadAndFixSettings()
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

void NormalServer::preload_finish()
{
    QtServer::preload_finish();
    std::cerr << "Waiting connection on port " << normalServerSettings.server_port << std::endl;
}
