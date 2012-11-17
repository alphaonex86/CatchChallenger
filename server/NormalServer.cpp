#include "NormalServer.h"
#include "base/GlobalData.h"
#include "../general/base/FacilityLib.h"

/*
  When disconnect the fake client, stop the benchmark
  */

using namespace Pokecraft;

bool NormalServer::oneInstanceRunning=false;

NormalServer::NormalServer() :
    BaseServer()
{
    server                                                  = NULL;
    timer_benchmark_stop                                    = NULL;

    GlobalData::serverPrivateVariables.eventThreaderList << new EventThreader();//broad cast (0)
    GlobalData::serverPrivateVariables.eventThreaderList << new EventThreader();//map management (1)
    GlobalData::serverPrivateVariables.eventThreaderList << new EventThreader();//network read (2)
    GlobalData::serverPrivateVariables.eventThreaderList << new EventThreader();//heavy load (3)
    GlobalData::serverPrivateVariables.eventThreaderList << new EventThreader();//local calcule (4)

    botThread = new EventThreader();
    eventDispatcherThread = new EventThreader();
    moveToThread(eventDispatcherThread);

    connect(this,SIGNAL(try_start_benchmark(quint16,quint16,bool)),this,SLOT(start_internal_benchmark(quint16,quint16,bool)),Qt::QueuedConnection);

    in_benchmark_mode=false;

    nextStep.start(POKECRAFT_SERVER_NORMAL_SPEED*50);

    connect(&BroadCastWithoutSender::broadCastWithoutSender,SIGNAL(serverCommand(QString,QString)),this,SLOT(serverCommand(QString,QString)),Qt::QueuedConnection);
    connect(&BroadCastWithoutSender::broadCastWithoutSender,SIGNAL(new_player_is_connected(Player_internal_informations)),this,SIGNAL(new_player_is_connected(Player_internal_informations)),Qt::QueuedConnection);
    connect(&BroadCastWithoutSender::broadCastWithoutSender,SIGNAL(player_is_disconnected(QString)),this,SIGNAL(player_is_disconnected(QString)),Qt::QueuedConnection);
    connect(&BroadCastWithoutSender::broadCastWithoutSender,SIGNAL(new_chat_message(QString,Chat_type,QString)),this,SIGNAL(new_chat_message(QString,Chat_type,QString)),Qt::QueuedConnection);
}

/** call only when the server is down
 * \warning this function is thread safe because it quit all thread before remove */
NormalServer::~NormalServer()
{
    timer_benchmark_stop->deleteLater();
    int index=0;
    while(index<GlobalData::serverPrivateVariables.eventThreaderList.size())
    {
        GlobalData::serverPrivateVariables.eventThreaderList.at(index)->quit();
        index++;
    }
    while(GlobalData::serverPrivateVariables.eventThreaderList.size()>0)
    {
        EventThreader * tempThread=GlobalData::serverPrivateVariables.eventThreaderList.first();
        GlobalData::serverPrivateVariables.eventThreaderList.removeFirst();
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
    connect(timer_benchmark_stop,SIGNAL(timeout()),this,SLOT(stop_benchmark()),Qt::QueuedConnection);
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
            GlobalData::serverPrivateVariables.botSpawn << tempPoint;
            DebugClass::debugConsole(QString("BotSpawn (bot_spawn_points): %1,%2").arg(tempPoint.x).arg(tempPoint.y));
            index_sub++;
        }
    }
}

void NormalServer::load_settings()
{
    GlobalData::serverPrivateVariables.connected_players	= 0;
    GlobalData::serverPrivateVariables.number_of_bots_logged= 0;
}

//start with allow real player to connect
void NormalServer::start_internal_server()
{
    if(server==NULL)
    {
        server=new QTcpServer();
        //to do in the thread
        connect(server,SIGNAL(newConnection()),this,SLOT(newConnection()),Qt::QueuedConnection);
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
        address.setAddress(GlobalData::serverSettings.server_ip);
    if(!server->listen(address,GlobalData::serverSettings.server_port))
    {
        DebugClass::debugConsole(QString("Unable to listen: %1, errror: %2").arg(listenIpAndPort(server_ip,GlobalData::serverSettings.server_port)).arg(server->errorString()));
        stat=Down;
        emit error(QString("Unable to listen: %1, errror: %2").arg(listenIpAndPort(server_ip,GlobalData::serverSettings.server_port)).arg(server->errorString()));
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
        DebugClass::debugConsole(QString("Listen *:%1").arg(GlobalData::serverSettings.server_port));
    else
        DebugClass::debugConsole("Listen "+server_ip+":"+QString::number(GlobalData::serverSettings.server_port));

    if(!initialize_the_database())
    {
        server->close();
        stat=Down;
        return;
    }

    if(!GlobalData::serverPrivateVariables.db->open())
    {
        DebugClass::debugConsole(QString("Unable to connect to the database: %1, with the login: %2, database text: %3").arg(GlobalData::serverPrivateVariables.db->lastError().driverText()).arg(GlobalData::serverSettings.database.mysql.login).arg(GlobalData::serverPrivateVariables.db->lastError().databaseText()));
        server->close();
        stat=Down;
        emit error(QString("Unable to connect to the database: %1, with the login: %2, database text: %3").arg(GlobalData::serverPrivateVariables.db->lastError().driverText()).arg(GlobalData::serverSettings.database.mysql.login).arg(GlobalData::serverPrivateVariables.db->lastError().databaseText()));
        return;
    }
    DebugClass::debugConsole(QString("Connected to %1 at %2 (%3)").arg(GlobalData::serverPrivateVariables.db_type_string).arg(GlobalData::serverSettings.database.mysql.host).arg(GlobalData::serverPrivateVariables.db->isOpen()));
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
        GlobalData::serverPrivateVariables.datapack_basePath=":/datapack/";
    else
        GlobalData::serverPrivateVariables.datapack_basePath=QCoreApplication::applicationDirPath()+"/datapack/";
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
    if(GlobalData::serverPrivateVariables.fakeBotList.size()>=1)
    {
        second=((double)time_benchmark_first_client.elapsed()/1000);
        if(second>0)
        {
            TX_size=GlobalData::serverPrivateVariables.fakeBotList.first()->get_TX_size();
            RX_size=GlobalData::serverPrivateVariables.fakeBotList.first()->get_RX_size();
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

void NormalServer::check_if_now_stopped()
{
    if(GlobalData::serverPrivateVariables.fakeBotList.size()!=0)
        return;
    BaseServer::check_if_now_stopped();
    oneInstanceRunning=false;
    if(server!=NULL)
    {
        server->close();
        delete server;
        server=NULL;
    }
    emit is_started(false);
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
}

void NormalServer::removeOneBot()
{
    FakeBot *client=qobject_cast<FakeBot *>(QObject::sender());
    if(client==NULL)
    {
        DebugClass::debugConsole("removeOneBot(): NULL client at disconnection");
        return;
    }
    GlobalData::serverPrivateVariables.fakeBotList.removeOne(client);
    client->deleteLater();
}

void NormalServer::removeBots()
{
    int list_size=GlobalData::serverPrivateVariables.fakeBotList.size();
    int index=0;
    while(index<list_size)
    {
        GlobalData::serverPrivateVariables.fakeBotList.at(index)->disconnect();
        //disconnect(&nextStep,SIGNAL(timeout()),fake_clients.last(),SLOT(doStep()));
        //connect(fake_clients.last(),SIGNAL(disconnected()),this,SLOT(removeOneBot()),Qt::QueuedConnection);
        //delete after the signal
        index++;
    }
}

/////////////////////////////////////// Add object //////////////////////////////////////

void NormalServer::addBot()
{
    GlobalData::serverPrivateVariables.fakeBotList << new FakeBot();
    GlobalData::serverPrivateVariables.botSockets << &GlobalData::serverPrivateVariables.fakeBotList.last()->fakeSocket;

    if(GlobalData::serverPrivateVariables.fakeBotList.size()==1)
    {
        //GlobalData::serverPrivateVariables.fake_clients.last()->show_details();
        time_benchmark_first_client.start();
    }
    GlobalData::serverPrivateVariables.fakeBotList.last()->moveToThread(botThread);
    GlobalData::serverPrivateVariables.fakeBotList.last()->start_step();
    GlobalData::serverPrivateVariables.fakeBotList.last()->tryLink();
    connect(&nextStep,SIGNAL(timeout()),GlobalData::serverPrivateVariables.fakeBotList.last(),SLOT(doStep()),Qt::QueuedConnection);
    connect(GlobalData::serverPrivateVariables.fakeBotList.last(),SIGNAL(isDisconnected()),this,SLOT(removeOneBot()),Qt::QueuedConnection);
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
            if(!GlobalData::serverPrivateVariables.fakeBotList.empty())
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
            if(number_player>(GlobalData::serverSettings.max_players-client_list.size()))
                number_player=GlobalData::serverSettings.max_players-client_list.size();
            int index=0;
            while(index<number_player && client_list.size()<GlobalData::serverSettings.max_players)
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
            if(GlobalData::serverPrivateVariables.botSockets.contains(socket->getTheOtherSocket()))
            {
                DebugClass::debugConsole(QString("new client connected by fake socket"));
                GlobalData::serverPrivateVariables.botSockets.remove(socket->getTheOtherSocket());
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
    return GlobalData::serverPrivateVariables.connected_players;
}

quint16 NormalServer::player_max()
{
    return GlobalData::serverSettings.max_players;
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
