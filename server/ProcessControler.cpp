#include "ProcessControler.h"
#include "base/ServerStructures.h"

using namespace CatchChallenger;

ProcessControler::ProcessControler()
{
    qRegisterMetaType<Chat_type>("Chat_type");
    connect(&server,&CatchChallenger::NormalServer::is_started,this,&ProcessControler::server_is_started);
    connect(&server,&CatchChallenger::NormalServer::need_be_stopped,this,&ProcessControler::server_need_be_stopped);
    connect(&server,&CatchChallenger::NormalServer::need_be_restarted,this,&ProcessControler::server_need_be_restarted);
    connect(&server,&CatchChallenger::NormalServer::benchmark_result,this,&ProcessControler::benchmark_result);
    connect(&server,&CatchChallenger::NormalServer::error,this,&ProcessControler::error);
    need_be_restarted=false;
    need_be_closed=false;

    settings=new QSettings(QCoreApplication::applicationDirPath()+"/server.properties",QSettings::IniFormat);
    NormalServer::checkSettingsFile(settings);
    send_settings();
    server.start_server();
}

ProcessControler::~ProcessControler()
{
    delete settings;
}

void ProcessControler::send_settings()
{
    CatchChallenger::ServerSettings formatedServerSettings=server.getSettings();;

    //the listen
    formatedServerSettings.server_port					= settings->value("server-port").toUInt();
    formatedServerSettings.server_ip					= settings->value("server-ip").toString();

    //fight
    CommonSettings::commonSettings.pvp			= settings->value("pvp").toBool();
    formatedServerSettings.sendPlayerNumber		= settings->value("sendPlayerNumber").toBool();

    //rates
    settings->beginGroup("rates");
    CommonSettings::commonSettings.rates_xp			= settings->value("xp_normal").toReal();
    formatedServerSettings.rates_xp_premium                         = settings->value("xp_premium").toReal();
    CommonSettings::commonSettings.rates_gold			= settings->value("gold_normal").toReal();
    formatedServerSettings.rates_gold_premium                       = settings->value("gold_premium").toReal();
    CommonSettings::commonSettings.rates_shiny		= settings->value("shiny_normal").toReal();
    formatedServerSettings.rates_shiny_premium                      = settings->value("shiny_premium").toReal();
    settings->endGroup();

    //chat allowed
    settings->beginGroup("chat");
    CommonSettings::commonSettings.chat_allow_all         = settings->value("allow-all").toBool();
    CommonSettings::commonSettings.chat_allow_local		= settings->value("allow-local").toBool();
    CommonSettings::commonSettings.chat_allow_private		= settings->value("allow-private").toBool();
    CommonSettings::commonSettings.chat_allow_aliance		= settings->value("allow-aliance").toBool();
    CommonSettings::commonSettings.chat_allow_clan		= settings->value("allow-clan").toBool();
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
    settings->endGroup();
    settings->beginGroup("MapVisibilityAlgorithm-Simple");
    formatedServerSettings.mapVisibility.simple.max				= settings->value("Max").toUInt();
    formatedServerSettings.mapVisibility.simple.reshow			= settings->value("Reshow").toUInt();
    settings->endGroup();

    settings->beginGroup("city");
    if(settings->value("capture_frequency").toString()=="week")
        formatedServerSettings.city.capture.frenquency=City::Capture::Frequency_week;
    else
        formatedServerSettings.city.capture.frenquency=City::Capture::Frequency_month;

    if(settings->value("capture_day").toString()=="monday")
        formatedServerSettings.city.capture.day=City::Capture::Monday;
    else if(settings->value("capture_day").toString()=="tuesday")
        formatedServerSettings.city.capture.day=City::Capture::Tuesday;
    else if(settings->value("capture_day").toString()=="wednesday")
        formatedServerSettings.city.capture.day=City::Capture::Wednesday;
    else if(settings->value("capture_day").toString()=="thursday")
        formatedServerSettings.city.capture.day=City::Capture::Thursday;
    else if(settings->value("capture_day").toString()=="friday")
        formatedServerSettings.city.capture.day=City::Capture::Friday;
    else if(settings->value("capture_day").toString()=="saturday")
        formatedServerSettings.city.capture.day=City::Capture::Saturday;
    else if(settings->value("capture_day").toString()=="sunday")
        formatedServerSettings.city.capture.day=City::Capture::Sunday;
    else
        formatedServerSettings.city.capture.day=City::Capture::Monday;
    formatedServerSettings.city.capture.hour=0;
    formatedServerSettings.city.capture.minute=0;
    QStringList capture_time_string_list=settings->value("capture_time").toString().split(":");
    if(capture_time_string_list.size()==2)
    {
        bool ok;
        formatedServerSettings.city.capture.hour=capture_time_string_list.first().toUInt(&ok);
        if(!ok)
            formatedServerSettings.city.capture.hour=0;
        else
        {
            formatedServerSettings.city.capture.minute=capture_time_string_list.last().toUInt(&ok);
            if(!ok)
                formatedServerSettings.city.capture.minute=0;
        }
    }
    settings->endGroup();

    {
        bool ok;
        settings->beginGroup("bitcoin");
        formatedServerSettings.bitcoin.address=settings->value("address").toString();
        if(settings->contains("binaryPath") && !settings->value("binaryPath").toString().isEmpty())
            formatedServerSettings.bitcoin.binaryPath=settings->value("binaryPath").toString();
        else
        {
            #ifdef Q_OS_WIN32
            formatedServerSettings.bitcoin.binaryPath                         = "%application_path%/bitcoin/bitcoind.exe";
            #else
            formatedServerSettings.bitcoin.binaryPath                         = "/usr/bin/bitcoind";
            #endif
        }
        formatedServerSettings.bitcoin.enabled=(settings->value("enabled").toString()=="true");
        formatedServerSettings.bitcoin.fee=settings->value("fee").toDouble(&ok);
        if(!ok)
            formatedServerSettings.bitcoin.fee=1.0;
        formatedServerSettings.bitcoin.port=settings->value("port").toUInt(&ok);
        if(!ok)
            formatedServerSettings.bitcoin.port=46349;
        formatedServerSettings.bitcoin.workingPath=settings->value("workingPath").toString();
        if(settings->contains("workingPath") && !settings->value("workingPath").toString().isEmpty())
            formatedServerSettings.bitcoin.workingPath=settings->value("workingPath").toString();
        else
        {
            #ifdef Q_OS_WIN32
            formatedServerSettings.bitcoin.workingPath                        = "%application_path%/bitcoin-storage/";
            #else
            formatedServerSettings.bitcoin.workingPath                        = QDir::homePath()+"/.config/CatchChallenger/server/bitoin/";
            #endif
        }
        settings->endGroup();
    }

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
