#include "NormalServer.h"
#include "base/GlobalServerData.h"
#include "../general/base/FacilityLib.h"
#include <QSslSocket>
#include <QTcpSocket>
#include <QNetworkProxy>
#include <QProcess>
#include "base/ClientMapManagement/MapVisibilityAlgorithm_None.h"
#include "base/ClientMapManagement/MapVisibilityAlgorithm_Simple_StoreOnSender.h"
#include "base/ClientMapManagement/MapVisibilityAlgorithm_WithBorder_StoreOnSender.h"

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
    sslCertificate          = NULL;
    sslKey                  = NULL;

    normalServerSettings.server_ip      = std::string();
    normalServerSettings.server_port    = 42489;
    normalServerSettings.useSsl         = true;
    #ifdef __linux__
    CommonSettingsServer::commonSettingsServer.tcpCork  = false;
    normalServerSettings.tcpNodelay         = false;
    #endif


    //botThread = new EventThreader();
    //crash if this, due to different socket and thread
    //eventDispatcherThread = new EventThreader();
    //moveToThread(eventDispatcherThread);

    connect(&BroadCastWithoutSender::broadCastWithoutSender,&BroadCastWithoutSender::serverCommand,this,&NormalServer::serverCommand,Qt::QueuedConnection);
    connect(&BroadCastWithoutSender::broadCastWithoutSender,&BroadCastWithoutSender::new_player_is_connected,this,&NormalServer::new_player_is_connected,Qt::QueuedConnection);
    connect(&BroadCastWithoutSender::broadCastWithoutSender,&BroadCastWithoutSender::player_is_disconnected,this,&NormalServer::player_is_disconnected,Qt::QueuedConnection);
    connect(&BroadCastWithoutSender::broadCastWithoutSender,&BroadCastWithoutSender::new_chat_message,this,&NormalServer::new_chat_message,Qt::QueuedConnection);
    connect(&purgeKickedHostTimer,&QTimer::timeout,this,&NormalServer::purgeKickedHost,Qt::QueuedConnection);
    connect(this,&QtServer::need_be_started,this,&NormalServer::start_internal_server,Qt::QueuedConnection);
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

    /*botThread->quit();
    botThread->wait();
    delete botThread;*/
    /*eventDispatcherThread->quit();
    eventDispatcherThread->wait();
    delete eventDispatcherThread;*/
    if(sslKey!=NULL)
        delete sslKey;
    if(sslCertificate!=NULL)
        delete sslCertificate;
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

void NormalServer::parseJustLoadedMap(const Map_to_send &map_to_send,const std::string &map_file)
{
    Q_UNUSED(map_to_send);
    Q_UNUSED(map_file);
}

void NormalServer::load_settings()
{
    GlobalServerData::serverPrivateVariables.connected_players	= 0;
    GlobalServerData::serverPrivateVariables.number_of_bots_logged= 0;
}

//start with allow real player to connect
void NormalServer::start_internal_server()
{
    if(normalServerSettings.useSsl)
    {
        if(!QFile(QCoreApplication::applicationDirPath()+"/server.key").exists() || !QFile(QCoreApplication::applicationDirPath()+"/server.crt").exists())
        {
            QStringList args;
            args << "req" << "-newkey" << "rsa:4096" << "-sha512" << "-x509" << "-nodes" << "-days" << "3560" << "-out" << QCoreApplication::applicationDirPath()+"/server.crt"
                    << "-keyout" << QCoreApplication::applicationDirPath()+"/server.key" << "-subj" << "/C=FR/ST=South-West/L=Paris/O=Catchchallenger/OU=Developer Department/CN=*"
                    << "-extensions usr_cert";
            #ifdef _WIN32
            QString opensslAppPath=QCoreApplication::applicationDirPath()+"/openssl/openssl.exe";
            #else
            QString opensslAppPath="/usr/bin/openssl";
            #endif
            QProcess process;
            process.start(opensslAppPath,args);
            process.waitForFinished();
            process.setWorkingDirectory(QCoreApplication::applicationDirPath());
            if(process.exitCode()!=0 || !QFile(QCoreApplication::applicationDirPath()+"/server.key").exists() || !QFile(QCoreApplication::applicationDirPath()+"/server.crt").exists())
            {
                if(process.exitCode()!=0)
                {
                    std::cerr << "return code: " << process.exitCode()
                              << ", output: " << QString::fromLocal8Bit(process.readAll()).toStdString()
                              << ", error: " << process.error()
                              << ", error string: " << process.errorString().toStdString()
                              << ", exitStatus: " << process.exitStatus()
                              << std::endl;
                    std::cerr << "To start: " << opensslAppPath.toStdString() << " " << args.join(" ").toStdString() << std::endl;
                }
                process.kill();
                std::cerr << "Certificate for the ssl connexion not found, buy or generate self signed, and put near the application" << std::endl;
                stat=Down;
                is_started(false);
                error("Certificate for the ssl connexion not found, buy or generate self signed, and put near the application");
                return;
            }
            process.kill();
        }

        if(sslKey!=NULL)
            delete sslKey;
        QFile key(QCoreApplication::applicationDirPath()+"/server.key");
        if(!key.open(QIODevice::ReadOnly))
        {
            std::cerr << "Unable to access to the server key: " << key.errorString().toStdString() << std::endl;
            stat=Down;
            is_started(false);
            error("Unable to access to the server key");
            return;
        }
        QByteArray keyData=key.readAll();
        key.close();
        QSslKey sslKey(keyData,QSsl::Rsa);
        if(sslKey.isNull())
        {
            std::cerr << "Server key is wrong" << std::endl;
            stat=Down;
            is_started(false);
            error("Server key is wrong");
            return;
        }

        if(sslCertificate!=NULL)
            delete sslCertificate;
        QFile certificate(QCoreApplication::applicationDirPath()+"/server.crt");
        if(!certificate.open(QIODevice::ReadOnly))
        {
            std::cerr << "Unable to access to the server certificate: " << certificate.errorString().toStdString() << std::endl;
            stat=Down;
            is_started(false);
            error("Unable to access to the server certificate");
            return;
        }
        QByteArray certificateData=certificate.readAll();
        certificate.close();
        QSslCertificate sslCertificate(certificateData);
        if(sslCertificate.isNull())
        {
            std::cerr << "Server certificate is wrong" << std::endl;
            stat=Down;
            is_started(false);
            error("Server certificate is wrong");
            return;
        }
        if(sslServer==NULL)
            sslServer=new QSslServer(sslCertificate,sslKey);
    }
    else
    {
        if(sslServer==NULL)
            sslServer=new QSslServer();
    }
    connect(sslServer,&QTcpServer::newConnection,this,&NormalServer::newConnection,Qt::QueuedConnection);
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
    if(!QFakeServer::server.listen())
    {
        std::cerr << "Unable to listen the internal server" << std::endl;
        stat=Down;
        is_started(false);
        error("Unable to listen the internal server");
        return;
    }
    #ifdef __linux__
    if(CommonSettingsServer::commonSettingsServer.tcpCork)
    {
        qintptr socketDescriptor=sslServer->socketDescriptor();
        if(socketDescriptor!=-1)
        {
            int state = 1;
            if(setsockopt(socketDescriptor, IPPROTO_TCP, TCP_CORK, &state, sizeof(state))!=0)
                std::cerr << "Unable to apply tcp cork under linux" << std::endl;
        }
        else
            std::cerr << "Unable to get socket descriptor to apply tcp cork under linux" << std::endl;
    }
    else if(normalServerSettings.tcpNodelay)
    {
        qintptr socketDescriptor=sslServer->socketDescriptor();
        if(socketDescriptor!=-1)
        {
            int state = 1;
            if(setsockopt(socketDescriptor, IPPROTO_TCP, TCP_NODELAY, &state, sizeof(state))!=0)
                std::cerr << "Unable to apply tcp cork under linux" << std::endl;
        }
        else
            std::cerr << "Unable to get socket descriptor to apply tcp cork under linux" << std::endl;
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
    preload_the_data();
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

    if(sslKey!=NULL)
        delete sslKey;
    if(sslCertificate!=NULL)
        delete sslCertificate;
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
    while(QFakeServer::server.hasPendingConnections())
    {
        QFakeSocket *socket = QFakeServer::server.nextPendingConnection();
        if(socket!=NULL)
        {
            std::cout << "new client connected on internal socket" << std::endl;
            switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
            {
                case MapVisibilityAlgorithmSelection_Simple:
                    connect_the_last_client(new MapVisibilityAlgorithm_Simple_StoreOnSender(new ConnectedSocket(socket)),socket);
                break;
                case MapVisibilityAlgorithmSelection_WithBorder:
                    connect_the_last_client(new MapVisibilityAlgorithm_WithBorder_StoreOnSender(new ConnectedSocket(socket)),socket);
                break;
                default:
                case MapVisibilityAlgorithmSelection_None:
                    connect_the_last_client(new MapVisibilityAlgorithm_None(new ConnectedSocket(socket)),socket);
                break;
            }
        }
        else
            std::cout << "NULL client with fake socket" << std::endl;
    }
    if(sslServer!=NULL)
        while(sslServer->hasPendingConnections())
        {
            QSslSocket *socket = static_cast<QSslSocket *>(sslServer->nextPendingConnection());
            const QHostAddress &peerAddress=socket->peerAddress();
            bool kicked=kickedHosts.contains(peerAddress);
            if(kicked)
                if((QDateTime::currentDateTime().toTime_t()-kickedHosts.value(peerAddress).toTime_t())>=(uint32_t)CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick)
                {
                    kickedHosts.remove(peerAddress);
                    kicked=false;
                }
            if(!kicked)
            {
                connect(socket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),      this,&NormalServer::sslErrors);
                if(socket!=NULL)
                {
                    #ifdef __linux__
                    if(CommonSettingsServer::commonSettingsServer.tcpCork)
                    {
                        qintptr socketDescriptor=socket->socketDescriptor();
                        if(socketDescriptor!=-1)
                        {
                            int state = 1;
                            if(setsockopt(socketDescriptor, IPPROTO_TCP, TCP_CORK, &state, sizeof(state))!=0)
                                std::cerr << "Unable to apply tcp cork under linux" << std::endl;
                        }
                        else
                            std::cerr << "Unable to get socket descriptor to apply tcp cork under linux" << std::endl;
                    }
                    else if(normalServerSettings.tcpNodelay)
                    {
                        qintptr socketDescriptor=socket->socketDescriptor();
                        if(socketDescriptor!=-1)
                        {
                            int state = 1;
                            if(setsockopt(socketDescriptor, IPPROTO_TCP, TCP_NODELAY, &state, sizeof(state))!=0)
                                std::cerr << "Unable to apply tcp cork under linux" << std::endl;
                        }
                        else
                            std::cerr << "Unable to get socket descriptor to apply tcp cork under linux" << std::endl;
                    }
                    #endif
                    //DebugClass::debugConsole(std::stringLiteral("new client connected by tcp socket"));-> prevent DDOS logs
                    Client *client;
                    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
                    {
                        case MapVisibilityAlgorithmSelection_Simple:
                            client=new MapVisibilityAlgorithm_Simple_StoreOnSender(new ConnectedSocket(socket));
                        break;
                        case MapVisibilityAlgorithmSelection_WithBorder:
                            client=new MapVisibilityAlgorithm_WithBorder_StoreOnSender(new ConnectedSocket(socket));
                        break;
                        default:
                        case MapVisibilityAlgorithmSelection_None:
                            client=new MapVisibilityAlgorithm_None(new ConnectedSocket(socket));
                        break;
                    }
                    connect_the_last_client(client,socket);
                    //connect(client,&Client::kicked,this,&NormalServer::kicked,Qt::QueuedConnection);
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
        if((currentDateTime.toTime_t()-i.value().toTime_t())>=(uint32_t)CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick)
            hostsToRemove.push_back(i.key());
    }
    unsigned int index=0;
    while(index<hostsToRemove.size())
    {
        kickedHosts.remove(hostsToRemove.at(index));
        index++;
    }
}

void NormalServer::sslErrors(const QList<QSslError> &errors)
{
    std::cerr << "Ssl error: " << std::endl;
    int index=0;
    while(index<errors.size())
    {
        std::cerr << errors.at(index).errorString().toStdString() << std::endl;
        index++;
    }
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
    return GlobalServerData::serverPrivateVariables.connected_players;
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
