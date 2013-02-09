#include "ProcessControler.h"
#include "base/ServerStructures.h"

using namespace CatchChallenger;

ProcessControler::ProcessControler()
{
    qRegisterMetaType<Chat_type>("Chat_type");
    connect(&server,SIGNAL(is_started(bool)),this,SLOT(server_is_started(bool)));
    connect(&server,SIGNAL(need_be_stopped()),this,SLOT(server_need_be_stopped()));
    connect(&server,SIGNAL(need_be_restarted()),this,SLOT(server_need_be_restarted()));
    connect(&server,SIGNAL(benchmark_result(int,double,double,double,double,double)),this,SLOT(benchmark_result(int,double,double,double,double,double)));
    connect(&server,SIGNAL(error(QString)),this,SLOT(error(QString)));
    need_be_restarted=false;
    need_be_closed=false;

    settings=new QSettings(QCoreApplication::applicationDirPath()+"/server.properties",QSettings::IniFormat);
    NormalServer::load_default_settings(settings);
    send_settings();
    server.start_server();
}

ProcessControler::~ProcessControler()
{
    delete settings;
}

void ProcessControler::send_settings()
{
    CatchChallenger::ServerSettings formatedServerSettings;

    //the listen
    formatedServerSettings.server_port					= settings->value("server-port").toUInt();
    formatedServerSettings.server_ip					= settings->value("server-ip").toString();

    //fight
    formatedServerSettings.commmonServerSettings.pvp			= settings->value("pvp").toBool();
    formatedServerSettings.commmonServerSettings.sendPlayerNumber		= settings->value("sendPlayerNumber").toBool();

    //rates
    settings->beginGroup("rates");
    formatedServerSettings.commmonServerSettings.rates_xp			= settings->value("xp_normal").toReal();
    formatedServerSettings.rates_xp_premium                         = settings->value("xp_premium").toReal();
    formatedServerSettings.commmonServerSettings.rates_gold			= settings->value("gold_normal").toReal();
    formatedServerSettings.rates_gold_premium                       = settings->value("gold_premium").toReal();
    formatedServerSettings.commmonServerSettings.rates_shiny		= settings->value("shiny_normal").toReal();
    formatedServerSettings.rates_shiny_premium                      = settings->value("shiny_premium").toReal();
    settings->endGroup();

    //chat allowed
    settings->beginGroup("chat");
    formatedServerSettings.commmonServerSettings.chat_allow_all         = settings->value("allow-all").toBool();
    formatedServerSettings.commmonServerSettings.chat_allow_local		= settings->value("allow-local").toBool();
    formatedServerSettings.commmonServerSettings.chat_allow_private		= settings->value("allow-private").toBool();
    formatedServerSettings.commmonServerSettings.chat_allow_aliance		= settings->value("allow-aliance").toBool();
    formatedServerSettings.commmonServerSettings.chat_allow_clan		= settings->value("allow-clan").toBool();
    settings->endGroup();

    settings->beginGroup("db");
    if(settings->value("type").toString()=="mysql")
        formatedServerSettings.database.type					= CatchChallenger::ServerSettings::Database::DatabaseType_Mysql;
    else if(settings->value("type").toString()=="sqlite")
        formatedServerSettings.database.type					= CatchChallenger::ServerSettings::Database::DatabaseType_SQLite;
    else
        formatedServerSettings.database.type					= CatchChallenger::ServerSettings::Database::DatabaseType_Mysql;
    switch(formatedServerSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            formatedServerSettings.database.mysql.host				= settings->value("mysql_host").toString();
            formatedServerSettings.database.mysql.db				= settings->value("mysql_db").toString();
            formatedServerSettings.database.mysql.login				= settings->value("mysql_login").toString();
            formatedServerSettings.database.mysql.pass				= settings->value("mysql_pass").toString();
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            formatedServerSettings.database.sqlite.file				= settings->value("file").toString();
        break;
    }
    if(settings->value("db_fight_sync").toString()=="FightSync_AtEachTurn")
        formatedServerSettings.database.fightSync                       = CatchChallenger::ServerSettings::Database::FightSync_AtEachTurn;
    else if(settings->value("db_fight_sync").toString()=="FightSync_AtTheDisconnexion")
        formatedServerSettings.database.fightSync                       = CatchChallenger::ServerSettings::Database::FightSync_AtTheDisconnexion;
    else
        formatedServerSettings.database.fightSync                       = CatchChallenger::ServerSettings::Database::FightSync_AtTheEndOfBattle;
    formatedServerSettings.database.positionTeleportSync=settings->value("positionTeleportSync").toBool();
    formatedServerSettings.database.secondToPositionSync=settings->value("secondToPositionSync").toUInt();
    settings->endGroup();

    //connection
    formatedServerSettings.max_players					= settings->value("max-players").toUInt();
    formatedServerSettings.tolerantMode                 = settings->value("tolerantMode").toBool();

    //visibility algorithm
    settings->beginGroup("MapVisibilityAlgorithm");
    switch(settings->value("MapVisibilityAlgorithm").toUInt())
    {
        case 0:
            formatedServerSettings.mapVisibility.mapVisibilityAlgorithm		= MapVisibilityAlgorithm_simple;
        break;
        case 1:
            formatedServerSettings.mapVisibility.mapVisibilityAlgorithm		= MapVisibilityAlgorithm_none;
        break;
        default:
            formatedServerSettings.mapVisibility.mapVisibilityAlgorithm		= MapVisibilityAlgorithm_simple;
        break;
    }
    formatedServerSettings.mapVisibility.simple.max				= settings->value("Max").toUInt();
    formatedServerSettings.mapVisibility.simple.reshow			= settings->value("Reshow").toUInt();
    settings->endGroup();

    server.setSettings(formatedServerSettings);
}

void ProcessControler::server_is_started(bool is_started)
{
    if(need_be_closed)
    {
        QCoreApplication::exit();
        return;
    }
    if(!is_started)
    {
        if(need_be_restarted)
        {
            need_be_restarted=false;
            server.start_server();
        }
    }
}

void ProcessControler::error(const QString &error)
{
    qDebug() << error;
    QCoreApplication::quit();
}

void ProcessControler::server_need_be_stopped()
{
    server.stop_server();
}

void ProcessControler::server_need_be_restarted()
{
    need_be_restarted=true;
    server.stop_server();
}

QString ProcessControler::sizeToString(double size)
{
    if(size<1024)
        return QString::number(size)+tr("B");
    if((size=size/1024)<1024)
        return adaptString(size)+tr("KB");
    if((size=size/1024)<1024)
        return adaptString(size)+tr("MB");
    if((size=size/1024)<1024)
        return adaptString(size)+tr("GB");
    if((size=size/1024)<1024)
        return adaptString(size)+tr("TB");
    if((size=size/1024)<1024)
        return adaptString(size)+tr("PB");
    if((size=size/1024)<1024)
        return adaptString(size)+tr("EB");
    if((size=size/1024)<1024)
        return adaptString(size)+tr("ZB");
    if((size=size/1024)<1024)
        return adaptString(size)+tr("YB");
    return tr("Too big");
}

QString ProcessControler::adaptString(float size)
{
    if(size>=100)
        return QString::number(size,'f',0);
    else
        return QString::number(size,'g',3);
}


void ProcessControler::benchmark_result(int latency,double TX_speed,double RX_speed,double TX_size,double RX_size,double second)
{
    DebugClass::debugConsole(QString("The latency of the benchmark: %1\nTX_speed: %2/s, RX_speed %3/s\nTX_size: %4, RX_size: %5, in %6s\nThis latency is cumulated latency of different point. That's not show the database performance.")
                 .arg(latency)
                 .arg(sizeToString(TX_speed))
                 .arg(sizeToString(RX_speed))
                 .arg(sizeToString(TX_size))
                 .arg(sizeToString(RX_size))
                 .arg(second)
                 );
}
