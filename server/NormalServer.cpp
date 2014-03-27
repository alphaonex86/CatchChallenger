#include "NormalServer.h"
#include "base/GlobalServerData.h"
#include "../general/base/FacilityLib.h"
#include <QSslSocket>
#include <QNetworkProxy>

#ifdef Q_OS_LINUX
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif

using namespace CatchChallenger;

bool NormalServer::oneInstanceRunning=false;

NormalServer::NormalServer() :
    BaseServer()
{
    server                  = NULL;
    timer_benchmark_stop    = NULL;
    sslCertificate          = NULL;
    sslKey                  = NULL;

    GlobalServerData::serverPrivateVariables.eventThreaderList << new EventThreader();//broad cast (0)
    GlobalServerData::serverPrivateVariables.eventThreaderList << new EventThreader();//map management (1)
    GlobalServerData::serverPrivateVariables.eventThreaderList << new EventThreader();//network read (2)
    GlobalServerData::serverPrivateVariables.eventThreaderList << new EventThreader();//heavy load (3)
    GlobalServerData::serverPrivateVariables.eventThreaderList << new EventThreader();//local calcule (4)
    GlobalServerData::serverPrivateVariables.eventThreaderList << new EventThreader();//local broad cast (5)

    botThread = new EventThreader();
    eventDispatcherThread = new EventThreader();
    moveToThread(eventDispatcherThread);

    connect(this,&NormalServer::try_start_benchmark,this,&NormalServer::start_internal_benchmark,Qt::QueuedConnection);

    in_benchmark_mode=false;

    nextStep.start(CATCHCHALLENGER_SERVER_NORMAL_SPEED);

    connect(&BroadCastWithoutSender::broadCastWithoutSender,&BroadCastWithoutSender::serverCommand,this,&NormalServer::serverCommand,Qt::QueuedConnection);
    connect(&BroadCastWithoutSender::broadCastWithoutSender,&BroadCastWithoutSender::new_player_is_connected,this,&NormalServer::new_player_is_connected,Qt::QueuedConnection);
    connect(&BroadCastWithoutSender::broadCastWithoutSender,&BroadCastWithoutSender::player_is_disconnected,this,&NormalServer::player_is_disconnected,Qt::QueuedConnection);
    connect(&BroadCastWithoutSender::broadCastWithoutSender,&BroadCastWithoutSender::new_chat_message,this,&NormalServer::new_chat_message,Qt::QueuedConnection);
}

/** call only when the server is down
 * \warning this function is thread safe because it quit all thread before remove */
NormalServer::~NormalServer()
{
    timer_benchmark_stop->deleteLater();
    int index=0;
    while(index<GlobalServerData::serverPrivateVariables.eventThreaderList.size())
    {
        GlobalServerData::serverPrivateVariables.eventThreaderList.at(index)->quit();
        index++;
    }
    while(GlobalServerData::serverPrivateVariables.eventThreaderList.size()>0)
    {
        EventThreader * tempThread=static_cast<EventThreader *>(GlobalServerData::serverPrivateVariables.eventThreaderList.first());
        GlobalServerData::serverPrivateVariables.eventThreaderList.removeFirst();
        tempThread->wait();
        delete tempThread;
    }
    if(server!=NULL)
    {
        server->close();/// \warning crash due to different thread
        server->deleteLater();
        server=NULL;
    }

    botThread->quit();
    botThread->wait();
    delete botThread;
    eventDispatcherThread->quit();
    eventDispatcherThread->wait();
    delete eventDispatcherThread;
    if(sslKey!=NULL)
        delete sslKey;
    if(sslCertificate!=NULL)
        delete sslCertificate;
}

void NormalServer::initAll()
{
    timer_benchmark_stop=new QTimer();
    timer_benchmark_stop->setInterval(60000);//1min
    timer_benchmark_stop->setSingleShot(true);
    connect(timer_benchmark_stop,&QTimer::timeout,this,&NormalServer::stop_benchmark,Qt::QueuedConnection);
    BaseServer::initAll();
}

//////////////////////////////////////////// server starting //////////////////////////////////////

void NormalServer::parseJustLoadedMap(const Map_to_send &map_to_send,const QString &map_file)
{
    if(in_benchmark_mode)
    {
        int index_sub=0;
        int size_sub_loop=map_to_send.bot_spawn_points.size();
        while(index_sub<size_sub_loop)
        {
            ServerPrivateVariables::BotSpawn tempPoint;
            tempPoint.map=map_file;
            tempPoint.x=map_to_send.bot_spawn_points.at(index_sub).x;
            tempPoint.y=map_to_send.bot_spawn_points.at(index_sub).y;
            GlobalServerData::serverPrivateVariables.botSpawn << tempPoint;
            DebugClass::debugConsole(QStringLiteral("BotSpawn (bot_spawn_points): %1,%2").arg(tempPoint.x).arg(tempPoint.y));
            index_sub++;
        }
    }
}

void NormalServer::load_settings()
{
    GlobalServerData::serverPrivateVariables.connected_players	= 0;
    GlobalServerData::serverPrivateVariables.number_of_bots_logged= 0;
}

//start with allow real player to connect
void NormalServer::start_internal_server()
{
    if(!QFile(QCoreApplication::applicationDirPath()+"/server.key").exists() && !QFile(QCoreApplication::applicationDirPath()+"/server.crt").exists())
    {
        QStringList args;
        args << "req" << "-newkey" << "rsa:4096" << "-sha512" << "-x509" << "-nodes" << "-days" << "3560" << "-out" << QCoreApplication::applicationDirPath()+"/server.crt"
                << "-keyout" << QCoreApplication::applicationDirPath()+"/server.key" << "-subj" << "/C=FR/ST=South-West/L=Paris/O=Catchchallenger/OU=Developer Department/CN=*"
                   << "-extensions usr_cert";
        #ifdef Q_OS_WIN32
        QString opensslAppPath=QCoreApplication::applicationDirPath()+"/openssl.exe";
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
                DebugClass::debugConsole(QStringLiteral("return code: %1, output: %2, error: %3, error string: %4, exitStatus: %5").arg(process.exitCode()).arg(QString::fromLocal8Bit(process.readAll())).arg(process.error()).arg(process.errorString()).arg(process.exitStatus()));
                DebugClass::debugConsole(QStringLiteral("for start: %1 %2").arg(opensslAppPath).arg(args.join(" ")));
            }
            process.kill();
            DebugClass::debugConsole(QStringLiteral("Certificate for the ssl connexion not found, buy or generate self signed, and put near the application"));
            stat=Down;
            emit is_started(false);
            emit error(QStringLiteral("Certificate for the ssl connexion not found, buy or generate self signed, and put near the application"));
            return;
        }
        process.kill();
    }

    if(sslKey!=NULL)
        delete sslKey;
    QFile key(QCoreApplication::applicationDirPath()+"/server.key");
    if(!key.open(QIODevice::ReadOnly))
    {
        DebugClass::debugConsole(QStringLiteral("Unable to access to the server key: %1").arg(key.errorString()));
        stat=Down;
        emit is_started(false);
        emit error(QStringLiteral("Unable to access to the server key"));
        return;
    }
    QByteArray keyData=key.readAll();
    key.close();
    QSslKey sslKey(keyData,QSsl::Rsa);
    if(sslKey.isNull())
    {
        DebugClass::debugConsole(QStringLiteral("Server key is wrong"));
        stat=Down;
        emit is_started(false);
        emit error(QStringLiteral("Server key is wrong"));
        return;
    }

    if(sslCertificate!=NULL)
        delete sslCertificate;
    QFile certificate(QCoreApplication::applicationDirPath()+"/server.crt");
    if(!certificate.open(QIODevice::ReadOnly))
    {
        DebugClass::debugConsole(QStringLiteral("Unable to access to the server certificate: %1").arg(certificate.errorString()));
        stat=Down;
        emit is_started(false);
        emit error(QStringLiteral("Unable to access to the server certificate"));
        return;
    }
    QByteArray certificateData=certificate.readAll();
    certificate.close();
    QSslCertificate sslCertificate(certificateData);
    if(sslCertificate.isNull())
    {
        DebugClass::debugConsole(QStringLiteral("Server certificate is wrong"));
        stat=Down;
        emit is_started(false);
        emit error(QStringLiteral("Server certificate is wrong"));
        return;
    }

    if(server==NULL)
    {
        server=new QSslServer(sslCertificate,sslKey);
        //to do in the thread
        connect(server,&QSslServer::newConnection,this,&NormalServer::newConnection,Qt::QueuedConnection);
    }
    if(server->isListening())
    {
        DebugClass::debugConsole(QStringLiteral("Already listening on %1").arg(listenIpAndPort(server->serverAddress().toString(),server->serverPort())));
        emit error(QStringLiteral("Already listening on %1").arg(listenIpAndPort(server->serverAddress().toString(),server->serverPort())));
        return;
    }
    if(oneInstanceRunning)
    {
        DebugClass::debugConsole("Other instance already running");
        return;
    }
    if(stat!=Down)
    {
        DebugClass::debugConsole("In wrong stat");
        return;
    }
    stat=InUp;
    load_settings();
    QHostAddress address = QHostAddress::Any;
    if(!GlobalServerData::serverSettings.server_ip.isEmpty())
        address.setAddress(GlobalServerData::serverSettings.server_ip);
    if(!GlobalServerData::serverSettings.proxy.isEmpty())
    {
        QNetworkProxy proxy=server->proxy();
        proxy.setType(QNetworkProxy::Socks5Proxy);
        proxy.setHostName(GlobalServerData::serverSettings.proxy);
        proxy.setPort(GlobalServerData::serverSettings.proxy_port);
        server->setProxy(proxy);
    }
    if(!server->listen(address,GlobalServerData::serverSettings.server_port))
    {
        DebugClass::debugConsole(QStringLiteral("Unable to listen: %1, errror: %2").arg(listenIpAndPort(GlobalServerData::serverSettings.server_ip,GlobalServerData::serverSettings.server_port)).arg(server->errorString()));
        stat=Down;
        emit is_started(false);
        emit error(QStringLiteral("Unable to listen: %1, errror: %2").arg(listenIpAndPort(GlobalServerData::serverSettings.server_ip,GlobalServerData::serverSettings.server_port)).arg(server->errorString()));
        return;
    }
    if(!QFakeServer::server.listen())
    {
        DebugClass::debugConsole(QStringLiteral("Unable to listen the internal server"));
        stat=Down;
        emit is_started(false);
        emit error(QStringLiteral("Unable to listen the internal server"));
        return;
    }
    #ifdef Q_OS_LINUX
    if(GlobalServerData::serverSettings.linuxSettings.tcpCork)
    {
        qintptr socketDescriptor=server->socketDescriptor();
        if(socketDescriptor!=-1)
        {
            int state = 1;
            if(setsockopt(socketDescriptor, IPPROTO_TCP, TCP_CORK, &state, sizeof(state))!=0)
                DebugClass::debugConsole(QStringLiteral("Unable to apply tcp cork under linux"));
        }
        else
            DebugClass::debugConsole(QStringLiteral("Unable to get socket descriptor to apply tcp cork under linux"));
    }
    #endif

    if(GlobalServerData::serverSettings.server_ip.isEmpty())
        DebugClass::debugConsole(QStringLiteral("Listen *:%1").arg(GlobalServerData::serverSettings.server_port));
    else
        DebugClass::debugConsole("Listen "+GlobalServerData::serverSettings.server_ip+":"+QString::number(GlobalServerData::serverSettings.server_port));

    if(!initialize_the_database())
    {
        server->close();
        stat=Down;
        emit is_started(false);
        return;
    }

    if(!GlobalServerData::serverPrivateVariables.db->open())
    {
        DebugClass::debugConsole(QStringLiteral("Unable to connect to the database: %1, with the login: %2, database text: %3").arg(GlobalServerData::serverPrivateVariables.db->lastError().driverText()).arg(GlobalServerData::serverSettings.database.mysql.login).arg(GlobalServerData::serverPrivateVariables.db->lastError().databaseText()));
        server->close();
        stat=Down;
        emit is_started(false);
        emit error(QStringLiteral("Unable to connect to the database: %1, with the login: %2, database text: %3").arg(GlobalServerData::serverPrivateVariables.db->lastError().driverText()).arg(GlobalServerData::serverSettings.database.mysql.login).arg(GlobalServerData::serverPrivateVariables.db->lastError().databaseText()));
        return;
    }
    BaseServer::start_internal_server();
    preload_the_data();
    stat=Up;
    oneInstanceRunning=true;
    emit is_started(true);
    return;
}

//start without real player possibility
void NormalServer::start_internal_benchmark(quint16 second,quint16 number_of_client)
{
    if(oneInstanceRunning)
    {
        DebugClass::debugConsole("Other instance already running");
        return;
    }
    if(stat!=Down)
    {
        DebugClass::debugConsole("Is in wrong stat for benchmark");
        return;
    }
    timer_benchmark_stop->setInterval(second*1000);
    benchmark_latency=0;
    stat=InUp;
    load_settings();
    //firstly get the spawn point
    preload_the_data();

    int index=0;
    while(index<number_of_client)
    {
        addBot();
        index++;
    }
    timer_benchmark_stop->start();
    stat=Up;
    oneInstanceRunning=true;
}

////////////////////////////////////////////////// server stopping ////////////////////////////////////////////

//call by stop benchmark
void NormalServer::stop_benchmark()
{
    if(in_benchmark_mode==false)
    {
        DebugClass::debugConsole("Double stop_benchmark detected!");
        return;
    }
    DebugClass::debugConsole("Stop the benchmark");
    double TX_speed=0;
    double RX_speed=0;
    double TX_size=0;
    double RX_size=0;
    double second=0;

    if(GlobalServerData::serverPrivateVariables.fakeBotList.size()>=1)
    {
        second=((double)time_benchmark_first_client.elapsed()/1000);
        if(second>0)
        {
            QSetIterator<FakeBot *> i(GlobalServerData::serverPrivateVariables.fakeBotList);
            FakeBot *firstBot=i.next();
            TX_size=firstBot->get_TX_size();
            RX_size=firstBot->get_RX_size();
            TX_speed=((double)TX_size)/second;
            RX_speed=((double)RX_size)/second;
        }
    }
    stat=Up;
    stop_internal_server();
    stat=Down;
    oneInstanceRunning=false;
    in_benchmark_mode=false;
    emit benchmark_result(benchmark_latency,TX_speed,RX_speed,TX_size,RX_size,second);
}

bool NormalServer::check_if_now_stopped()
{
    if(GlobalServerData::serverPrivateVariables.fakeBotList.size()!=0)
        return false;
    if(!BaseServer::check_if_now_stopped())
        return false;
    oneInstanceRunning=false;
    if(server!=NULL)
    {
        server->close();
        delete server;
        server=NULL;
    }
    return true;
}

//call by normal stop
void NormalServer::stop_internal_server()
{
    BaseServer::stop_internal_server();

    removeBots();
    if(server!=NULL)
    {
        server->close();
        delete server;
        server=NULL;
    }

    if(sslKey!=NULL)
        delete sslKey;
    if(sslCertificate!=NULL)
        delete sslCertificate;
}

/////////////////////////////////////////////////// Object removing /////////////////////////////////////

void NormalServer::removeOneClient()
{
    Client *client=qobject_cast<Client *>(QObject::sender());
    if(client==NULL)
    {
        DebugClass::debugConsole("removeOneClient(): NULL client at disconnection");
        return;
    }
    client_list.remove(client);
    client->deleteLater();
    check_if_now_stopped();
}

void NormalServer::removeOneBot()
{
    FakeBot *client=qobject_cast<FakeBot *>(QObject::sender());
    if(client==NULL)
    {
        DebugClass::debugConsole("removeOneBot(): NULL client at disconnection");
        return;
    }
    GlobalServerData::serverPrivateVariables.fakeBotList.remove(client);
    client->deleteLater();
    check_if_now_stopped();
}

void NormalServer::removeBots()
{
    QSetIterator<FakeBot *> i(GlobalServerData::serverPrivateVariables.fakeBotList);
    while(i.hasNext())
        i.next()->disconnect();
}

/////////////////////////////////////// Add object //////////////////////////////////////

void NormalServer::addBot()
{
    FakeBot * newFakeBot=new FakeBot();
    GlobalServerData::serverPrivateVariables.fakeBotList << newFakeBot;
    GlobalServerData::serverPrivateVariables.botSockets << &newFakeBot->fakeSocket;

    if(GlobalServerData::serverPrivateVariables.fakeBotList.size()==1)
    {
        //GlobalServerData::serverPrivateVariables.fake_clients.last()->show_details();
        time_benchmark_first_client.start();
    }
    newFakeBot->moveToThread(botThread);
    newFakeBot->start_step();
    connect(&nextStep,&QTimer::timeout,newFakeBot,&FakeBot::doStep,Qt::QueuedConnection);
    connect(newFakeBot,&FakeBot::isDisconnected,this,&NormalServer::removeOneBot,Qt::QueuedConnection);
}

///////////////////////////////////// Generic command //////////////////////////////////

void NormalServer::serverCommand(const QString &command, const QString &extraText)
{
    Client *client=qobject_cast<Client *>(QObject::sender());
    if(client==NULL)
    {
        DebugClass::debugConsole("NULL client at serverCommand()");
        return;
    }
    if(command=="restart")
        emit need_be_restarted();
    else if(command=="stop")
        emit need_be_stopped();
    else if(command=="addbots" || command=="removebots")
    {
        if(stat!=Up)
        {
            DebugClass::debugConsole("Is in wrong stat for bots manipulation");
            return;
        }
        if(in_benchmark_mode)
        {
            DebugClass::debugConsole("Is in benchmark mode, unable to do bots manipulation");
            return;
        }
        if(command=="addbots")
        {
            if(!GlobalServerData::serverPrivateVariables.fakeBotList.empty())
            {
                //client->local_sendPM(client->getPseudo(),"Remove previous bots firstly");
                DebugClass::debugConsole("Remove previous bots firstly");
                return;
            }
            quint16 number_player=2;
            if(extraText!="")
            {
                bool ok;
                number_player=extraText.toUInt(&ok);
                if(!ok)
                {
                    DebugClass::debugConsole("the arguement is not as number!");
                    return;
                }
            }
            if(number_player>(GlobalServerData::serverSettings.max_players-client_list.size()))
                number_player=GlobalServerData::serverSettings.max_players-client_list.size();
            int index=0;
            while(index<number_player && client_list.size()<GlobalServerData::serverSettings.max_players)
            {
                addBot();//add way to locate the bot spawn
                index++;
            }
        }
        else if(command=="removebots")
        {
            removeBots();
        }
        else
            DebugClass::debugConsole(QStringLiteral("unknow command in bots case: %1").arg(command));
    }
    else
        DebugClass::debugConsole(QStringLiteral("unknow command: %1").arg(command));
}

//////////////////////////////////// Function secondary //////////////////////////////
QString NormalServer::listenIpAndPort(QString server_ip,quint16 server_port)
{
    if(server_ip=="")
        server_ip="*";
    return server_ip+":"+QString::number(server_port);
}

void NormalServer::newConnection()
{
    while(QFakeServer::server.hasPendingConnections())
    {
        QFakeSocket *socket = QFakeServer::server.nextPendingConnection();
        if(socket!=NULL)
        {
            //to know if need login or not
            if(GlobalServerData::serverPrivateVariables.botSockets.contains(socket->getTheOtherSocket()))
            {
                DebugClass::debugConsole(QStringLiteral("new client connected by fake socket"));
                GlobalServerData::serverPrivateVariables.botSockets.remove(socket->getTheOtherSocket());
                connect_the_last_client(new Client(new ConnectedSocket(socket),true,getClientMapManagement()));
            }
            else
            {
                DebugClass::debugConsole(QStringLiteral("new bot connected by fake socket"));
                connect_the_last_client(new Client(new ConnectedSocket(socket),false,getClientMapManagement()));
            }
        }
        else
            DebugClass::debugConsole("NULL client with fake socket");
    }
    if(server!=NULL)
        while(server->hasPendingConnections())
        {
            QSslSocket *socket = static_cast<QSslSocket *>(server->nextPendingConnection());
            connect(socket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),      this,&NormalServer::sslErrors);
            if(socket!=NULL)
            {
                #ifdef Q_OS_LINUX
                if(GlobalServerData::serverSettings.linuxSettings.tcpCork)
                {
                    qintptr socketDescriptor=socket->socketDescriptor();
                    if(socketDescriptor!=-1)
                    {
                        int state = 1;
                        if(setsockopt(socketDescriptor, IPPROTO_TCP, TCP_CORK, &state, sizeof(state))!=0)
                            DebugClass::debugConsole(QStringLiteral("Unable to apply tcp cork under linux"));
                    }
                    else
                        DebugClass::debugConsole(QStringLiteral("Unable to get socket descriptor to apply tcp cork under linux"));
                }
                #endif
                //DebugClass::debugConsole(QStringLiteral("new client connected by tcp socket"));-> prevent DDOS logs
                connect_the_last_client(new Client(new ConnectedSocket(socket),false,getClientMapManagement()));
            }
            else
                DebugClass::debugConsole("NULL client: "+socket->peerAddress().toString());
        }
}

void NormalServer::sslErrors(const QList<QSslError> &errors)
{
    int index=0;
    while(index<errors.size())
    {
        DebugClass::debugConsole(QStringLiteral("Ssl error: %1").arg(errors.at(index).errorString()));
        index++;
    }
}

bool NormalServer::isListen()
{
    if(in_benchmark_mode)
        return false;
    return BaseServer::isListen();
}

bool NormalServer::isStopped()
{
    if(in_benchmark_mode)
        return false;
    return BaseServer::isStopped();
}

bool NormalServer::isInBenchmark()
{
    return in_benchmark_mode;
}

quint16 NormalServer::player_current()
{
    return GlobalServerData::serverPrivateVariables.connected_players;
}

quint16 NormalServer::player_max()
{
    return GlobalServerData::serverSettings.max_players;
}

/////////////////////////////////// Async the call ///////////////////////////////////
/// \brief Called when event loop is setup
void NormalServer::start_server()
{
    emit need_be_started();
}

void NormalServer::stop_server()
{
    emit try_stop_server();
}

void NormalServer::start_benchmark(quint16 second,quint16 number_of_client,bool benchmark_map)
{
    if(in_benchmark_mode)
    {
        DebugClass::debugConsole("Already in benchmark");
        return;
    }
    in_benchmark_mode=true;
    emit try_start_benchmark(second,number_of_client,benchmark_map);
}

void NormalServer::checkSettingsFile(QSettings *settings)
{
    if(!settings->contains(QLatin1Literal("max-players")))
        settings->setValue(QLatin1Literal("max-players"),200);
    if(!settings->contains(QLatin1Literal("server-ip")))
        settings->setValue(QLatin1Literal("server-ip"),QString());
    if(!settings->contains(QLatin1Literal("pvp")))
        settings->setValue(QLatin1Literal("pvp"),true);
    if(!settings->contains(QLatin1Literal("server-port")))
        settings->setValue(QLatin1Literal("server-port"),42489);
    if(!settings->contains(QLatin1Literal("benchmark_map")))
        settings->setValue(QLatin1Literal("benchmark_map"),true);
    if(!settings->contains(QLatin1Literal("benchmark_seconds")))
        settings->setValue(QLatin1Literal("benchmark_seconds"),60);
    if(!settings->contains(QLatin1Literal("benchmark_clients")))
        settings->setValue(QLatin1Literal("benchmark_clients"),400);
    if(!settings->contains(QLatin1Literal("sendPlayerNumber")))
        settings->setValue(QLatin1Literal("sendPlayerNumber"),false);
    if(!settings->contains(QLatin1Literal("tolerantMode")))
        settings->setValue(QLatin1Literal("tolerantMode"),false);
    if(!settings->contains(QLatin1Literal("compression")))
        settings->setValue(QLatin1Literal("compression"),QLatin1Literal("zlib"));
    if(!settings->contains(QLatin1Literal("character_delete_time")))
        settings->setValue(QLatin1Literal("character_delete_time"),604800);
    if(!settings->contains(QLatin1Literal("max_pseudo_size")))
        settings->setValue(QLatin1Literal("max_pseudo_size"),20);
    if(!settings->contains(QLatin1Literal("max_character")))
        settings->setValue(QLatin1Literal("max_character"),3);
    if(!settings->contains(QLatin1Literal("min_character")))
        settings->setValue(QLatin1Literal("min_character"),1);
    if(!settings->contains(QLatin1Literal("automatic_account_creation")))
        settings->setValue(QLatin1Literal("automatic_account_creation"),false);
    if(!settings->contains(QLatin1Literal("anonymous")))
        settings->setValue(QLatin1Literal("anonymous"),false);
    if(!settings->contains(QLatin1Literal("server_message")))
        settings->setValue(QLatin1Literal("server_message"),QString());
    if(!settings->contains(QLatin1Literal("proxy")))
        settings->setValue(QLatin1Literal("proxy"),QString());
    if(!settings->contains(QLatin1Literal("proxy_port")))
        settings->setValue(QLatin1Literal("proxy_port"),9050);
    if(!settings->contains(QLatin1Literal("forcedSpeed")))
        settings->setValue(QLatin1Literal("forcedSpeed"),CATCHCHALLENGER_SERVER_NORMAL_SPEED);
    if(!settings->contains(QLatin1Literal("dontSendPseudo")))
        settings->setValue(QLatin1Literal("dontSendPseudo"),false);
    if(!settings->contains(QLatin1Literal("dontSendPlayerType")))
        settings->setValue(QLatin1Literal("dontSendPlayerType"),false);
    if(!settings->contains(QLatin1Literal("forceClientToSendAtMapChange")))
        settings->setValue(QLatin1Literal("forceClientToSendAtMapChange"),true);
    if(!settings->contains(QLatin1Literal("httpDatapackMirror")))
        settings->setValue(QLatin1Literal("httpDatapackMirror"),QString());
    if(!settings->contains(QLatin1Literal("datapackCache")))
        settings->setValue(QLatin1Literal("datapackCache"),-1);

    #ifdef Q_OS_LINUX
    settings->beginGroup(QLatin1Literal("Linux"));
    if(!settings->contains(QLatin1Literal("tcpCork")))
        settings->setValue(QLatin1Literal("tcpCork"),true);
    settings->endGroup();
    #endif

    settings->beginGroup(QLatin1Literal("MapVisibilityAlgorithm"));
    if(!settings->contains(QLatin1Literal("MapVisibilityAlgorithm")))
        settings->setValue(QLatin1Literal("MapVisibilityAlgorithm"),0);
    settings->endGroup();

    settings->beginGroup(QLatin1Literal("MapVisibilityAlgorithm-Simple"));
    if(!settings->contains(QLatin1Literal("Max")))
        settings->setValue(QLatin1Literal("Max"),50);
    if(!settings->contains(QLatin1Literal("Reshow")))
        settings->setValue(QLatin1Literal("Reshow"),30);
    settings->endGroup();

    settings->beginGroup(QLatin1Literal("MapVisibilityAlgorithm-WithBorder"));
    if(!settings->contains(QLatin1Literal("MaxWithBorder")))
        settings->setValue(QLatin1Literal("MaxWithBorder"),20);
    if(!settings->contains(QLatin1Literal("ReshowWithBorder")))
        settings->setValue(QLatin1Literal("ReshowWithBorder"),10);
    if(!settings->contains(QLatin1Literal("Max")))
        settings->setValue(QLatin1Literal("Max"),50);
    if(!settings->contains(QLatin1Literal("Reshow")))
        settings->setValue(QLatin1Literal("Reshow"),30);
    settings->endGroup();

    settings->beginGroup(QLatin1Literal("rates"));
    if(!settings->contains(QLatin1Literal("xp_normal")))
        settings->setValue(QLatin1Literal("xp_normal"),0.1);
    if(!settings->contains(QLatin1Literal("gold_normal")))
        settings->setValue(QLatin1Literal("gold_normal"),1.0);
    if(!settings->contains(QLatin1Literal("drop_normal")))
        settings->setValue(QLatin1Literal("drop_normal"),1.0);
    if(!settings->contains(QLatin1Literal("xp_pow_normal")))
        settings->setValue(QLatin1Literal("xp_pow_normal"),1.6);
    settings->endGroup();

    settings->beginGroup(QLatin1Literal("chat"));
    if(!settings->contains(QLatin1Literal("allow-all")))
        settings->setValue(QLatin1Literal("allow-all"),true);
    if(!settings->contains(QLatin1Literal("allow-local")))
        settings->setValue(QLatin1Literal("allow-local"),true);
    if(!settings->contains(QLatin1Literal("allow-private")))
        settings->setValue(QLatin1Literal("allow-private"),true);
    if(!settings->contains(QLatin1Literal("allow-clan")))
        settings->setValue(QLatin1Literal("allow-clan"),true);
    settings->endGroup();

    settings->beginGroup(QLatin1Literal("db"));
    if(!settings->contains(QLatin1Literal("type")))
        settings->setValue(QLatin1Literal("type"),QLatin1Literal("mysql"));
    if(!settings->contains(QLatin1Literal("mysql_host")))
        settings->setValue(QLatin1Literal("mysql_host"),QLatin1Literal("localhost"));
    if(!settings->contains(QLatin1Literal("mysql_login")))
        settings->setValue(QLatin1Literal("mysql_login"),QLatin1Literal("catchchallenger-login"));
    if(!settings->contains(QLatin1Literal("mysql_pass")))
        settings->setValue(QLatin1Literal("mysql_pass"),QLatin1Literal("catchchallenger-pass"));
    if(!settings->contains(QLatin1Literal("mysql_db")))
        settings->setValue(QLatin1Literal("mysql_db"),QLatin1Literal("catchchallenger"));
    if(!settings->contains(QLatin1Literal("db_fight_sync")))
        settings->setValue(QLatin1Literal("db_fight_sync"),QLatin1Literal("FightSync_AtTheEndOfBattle"));
    if(!settings->contains(QLatin1Literal("secondToPositionSync")))
        settings->setValue(QLatin1Literal("secondToPositionSync"),0);
    if(!settings->contains(QLatin1Literal("positionTeleportSync")))
        settings->setValue(QLatin1Literal("positionTeleportSync"),true);
    settings->endGroup();


    settings->beginGroup(QLatin1Literal("city"));
    if(!settings->contains(QLatin1Literal("capture_frequency")))
        settings->setValue(QLatin1Literal("capture_frequency"),QLatin1Literal("month"));
    if(!settings->contains(QLatin1Literal("capture_day")))
        settings->setValue(QLatin1Literal("capture_day"),QLatin1Literal("monday"));
    if(!settings->contains(QLatin1Literal("capture_time")))
        settings->setValue(QLatin1Literal("capture_time"),QLatin1Literal("0:0"));
    settings->endGroup();

    settings->beginGroup(QLatin1Literal("bitcoin"));
    if(!settings->contains(QLatin1Literal("address")))
        settings->setValue(QLatin1Literal("address"),QLatin1Literal("1Hz3GtkiDBpbWxZixkQPuTGDh2DUy9bQUJ"));
    if(!settings->contains(QLatin1Literal("binaryPath")))
        settings->setValue(QLatin1Literal("binaryPath"),QString());
    if(!settings->contains(QLatin1Literal("enabled")))
        settings->setValue(QLatin1Literal("enabled"),false);
    if(!settings->contains(QLatin1Literal("fee")))
        settings->setValue(QLatin1Literal("fee"),1.0);
    if(!settings->contains(QLatin1Literal("history")))
        settings->setValue(QLatin1Literal("history"),30);
    if(!settings->contains(QLatin1Literal("port")))
        settings->setValue(QLatin1Literal("port"),46349);
    if(!settings->contains(QLatin1Literal("workingPath")))
        settings->setValue(QLatin1Literal("workingPath"),QLatin1Literal("0:0"));
    settings->endGroup();
}

