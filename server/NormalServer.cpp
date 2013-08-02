#include "NormalServer.h"
#include "base/GlobalServerData.h"
#include "../general/base/FacilityLib.h"

/*
  When disconnect the fake client, stop the benchmark
  */

using namespace CatchChallenger;

bool NormalServer::oneInstanceRunning=false;

NormalServer::NormalServer() :
    BaseServer()
{
    server                                                  = NULL;
    timer_benchmark_stop                                    = NULL;

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
        EventThreader * tempThread=GlobalServerData::serverPrivateVariables.eventThreaderList.first();
        GlobalServerData::serverPrivateVariables.eventThreaderList.removeFirst();
        tempThread->wait();
        delete tempThread;
    }
    if(server!=NULL)
    {
        server->close();
        delete server;
        server=NULL;
    }

    botThread->quit();
    botThread->wait();
    delete botThread;
    eventDispatcherThread->quit();
    eventDispatcherThread->wait();
    delete eventDispatcherThread;
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
            DebugClass::debugConsole(QString("BotSpawn (bot_spawn_points): %1,%2").arg(tempPoint.x).arg(tempPoint.y));
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
    if(server==NULL)
    {
        server=new QTcpServer();
        //to do in the thread
        connect(server,&QTcpServer::newConnection,this,&NormalServer::newConnection,Qt::QueuedConnection);
    }
    if(server->isListening())
    {
        DebugClass::debugConsole(QString("Already listening on %1").arg(listenIpAndPort(server->serverAddress().toString(),server->serverPort())));
        emit error(QString("Already listening on %1").arg(listenIpAndPort(server->serverAddress().toString(),server->serverPort())));
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
    if(!server_ip.isEmpty())
        address.setAddress(GlobalServerData::serverSettings.server_ip);
    if(!server->listen(address,GlobalServerData::serverSettings.server_port))
    {
        DebugClass::debugConsole(QString("Unable to listen: %1, errror: %2").arg(listenIpAndPort(server_ip,GlobalServerData::serverSettings.server_port)).arg(server->errorString()));
        stat=Down;
        emit error(QString("Unable to listen: %1, errror: %2").arg(listenIpAndPort(server_ip,GlobalServerData::serverSettings.server_port)).arg(server->errorString()));
        return;
    }
    if(!QFakeServer::server.listen())
    {
        DebugClass::debugConsole(QString("Unable to listen the internal server"));
        stat=Down;
        emit error(QString("Unable to listen the internal server"));
        return;
    }
    if(server_ip.isEmpty())
        DebugClass::debugConsole(QString("Listen *:%1").arg(GlobalServerData::serverSettings.server_port));
    else
        DebugClass::debugConsole("Listen "+server_ip+":"+QString::number(GlobalServerData::serverSettings.server_port));

    if(!initialize_the_database())
    {
        server->close();
        stat=Down;
        return;
    }

    if(!GlobalServerData::serverPrivateVariables.db->open())
    {
        DebugClass::debugConsole(QString("Unable to connect to the database: %1, with the login: %2, database text: %3").arg(GlobalServerData::serverPrivateVariables.db->lastError().driverText()).arg(GlobalServerData::serverSettings.database.mysql.login).arg(GlobalServerData::serverPrivateVariables.db->lastError().databaseText()));
        server->close();
        stat=Down;
        emit error(QString("Unable to connect to the database: %1, with the login: %2, database text: %3").arg(GlobalServerData::serverPrivateVariables.db->lastError().driverText()).arg(GlobalServerData::serverSettings.database.mysql.login).arg(GlobalServerData::serverPrivateVariables.db->lastError().databaseText()));
        return;
    }
    preload_the_data();
    stat=Up;
    oneInstanceRunning=true;
    emit is_started(true);
    return;
}

//start without real player possibility
void NormalServer::start_internal_benchmark(quint16 second,quint16 number_of_client,const bool &benchmark_map)
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
    if(benchmark_map)
        GlobalServerData::serverPrivateVariables.datapack_basePath=":/datapack/";
    else
        GlobalServerData::serverPrivateVariables.datapack_basePath=QCoreApplication::applicationDirPath()+"/datapack/";
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
        server->deleteLater();
        server=NULL;
    }
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
            DebugClass::debugConsole(QString("unknow command in bots case: %1").arg(command));
    }
    else
        DebugClass::debugConsole(QString("unknow command: %1").arg(command));
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
                DebugClass::debugConsole(QString("new client connected by fake socket"));
                GlobalServerData::serverPrivateVariables.botSockets.remove(socket->getTheOtherSocket());
                connect_the_last_client(new Client(new ConnectedSocket(socket),true,getClientMapManagement()));
            }
            else
            {
                DebugClass::debugConsole(QString("new bot connected by fake socket"));
                connect_the_last_client(new Client(new ConnectedSocket(socket),false,getClientMapManagement()));
            }
        }
        else
            DebugClass::debugConsole("NULL client with fake socket");
    }
    if(server!=NULL)
        while(server->hasPendingConnections())
        {
            QTcpSocket *socket = server->nextPendingConnection();
            if(socket!=NULL)
            {
                DebugClass::debugConsole(QString("new client connected by tcp socket"));
                connect_the_last_client(new Client(new ConnectedSocket(socket),false,getClientMapManagement()));
            }
            else
                DebugClass::debugConsole("NULL client: "+socket->peerAddress().toString());
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

void NormalServer::load_default_settings(QSettings *settings)
{
    if(!settings->contains("max-players"))
        settings->setValue("max-players",200);
    if(!settings->contains("server-ip"))
        settings->setValue("server-ip","");
    if(!settings->contains("pvp"))
        settings->setValue("pvp",true);
    if(!settings->contains("server-port"))
        settings->setValue("server-port",42489);
    if(!settings->contains("benchmark_map"))
        settings->setValue("benchmark_map",true);
    if(!settings->contains("benchmark_seconds"))
        settings->setValue("benchmark_seconds",60);
    if(!settings->contains("benchmark_clients"))
        settings->setValue("benchmark_clients",400);
    if(!settings->contains("sendPlayerNumber"))
        settings->setValue("sendPlayerNumber",false);
    if(!settings->contains("tolerantMode"))
        settings->setValue("tolerantMode",false);

    settings->beginGroup("MapVisibilityAlgorithm");
    if(!settings->contains("MapVisibilityAlgorithm"))
        settings->setValue("MapVisibilityAlgorithm",0);
    settings->endGroup();

    settings->beginGroup("MapVisibilityAlgorithm-Simple");
    if(!settings->contains("Max"))
        settings->setValue("Max",50);
    if(!settings->contains("Reshow"))
        settings->setValue("Reshow",30);
    settings->endGroup();

    settings->beginGroup("rates");
    if(!settings->contains("xp_normal"))
        settings->setValue("xp_normal",1.0);
    if(!settings->contains("xp_premium"))
        settings->setValue("xp_premium",1.0);
    if(!settings->contains("gold_normal"))
        settings->setValue("gold_normal",1.0);
    if(!settings->contains("gold_premium"))
        settings->setValue("gold_premium",1.0);
    if(!settings->contains("shiny_normal"))
        settings->setValue("shiny_normal",0.0001);
    if(!settings->contains("shiny_premium"))
        settings->setValue("shiny_premium",0.0002);
    settings->endGroup();

    settings->beginGroup("chat");
    if(!settings->contains("allow-all"))
        settings->setValue("allow-all",true);
    if(!settings->contains("allow-local"))
        settings->setValue("allow-local",true);
    if(!settings->contains("allow-private"))
        settings->setValue("allow-private",true);
    if(!settings->contains("allow-aliance"))
        settings->setValue("allow-aliance",true);
    if(!settings->contains("allow-clan"))
        settings->setValue("allow-clan",true);
    settings->endGroup();

    settings->beginGroup("db");
    if(!settings->contains("type"))
        settings->setValue("type","mysql");
    if(!settings->contains("mysql_host"))
        settings->setValue("mysql_host","localhost");
    if(!settings->contains("mysql_login"))
        settings->setValue("mysql_login","catchchallenger-login");
    if(!settings->contains("mysql_pass"))
        settings->setValue("mysql_pass","catchchallenger-pass");
    if(!settings->contains("mysql_db"))
        settings->setValue("mysql_db","catchchallenger");
    if(!settings->contains("db_fight_sync"))
        settings->setValue("db_fight_sync","FightSync_AtTheEndOfBattle");
    if(!settings->contains("secondToPositionSync"))
        settings->setValue("secondToPositionSync",0);
    if(!settings->contains("positionTeleportSync"))
        settings->setValue("positionTeleportSync",true);
    settings->endGroup();
}
