#include "ProcessControler.h"
#include "base/ServerStructures.h"

using namespace CatchChallenger;

ProcessControler::ProcessControler()
{
    qRegisterMetaType<Chat_type>("Chat_type");
    connect(&server,&CatchChallenger::NormalServer::is_started,this,&ProcessControler::server_is_started);
    connect(&server,&CatchChallenger::NormalServer::need_be_stopped,this,&ProcessControler::server_need_be_stopped);
    connect(&server,&CatchChallenger::NormalServer::need_be_restarted,this,&ProcessControler::server_need_be_restarted);
    connect(&server,&CatchChallenger::NormalServer::error,this,&ProcessControler::error);
    need_be_restarted=false;
    need_be_closed=false;

    settings=new QSettings(QCoreApplication::applicationDirPath()+QLatin1Literal("/server.properties"),QSettings::IniFormat);
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
    CatchChallenger::ServerSettings formatedServerSettings=server.getSettings();
    NormalServerSettings formatedServerNormalSettings=server.getNormalSettings();

    //common var
    CommonSettings::commonSettings.min_character					= settings->value(QLatin1Literal("min_character")).toUInt();
    CommonSettings::commonSettings.max_character					= settings->value(QLatin1Literal("max_character")).toUInt();
    CommonSettings::commonSettings.max_pseudo_size					= settings->value(QLatin1Literal("max_pseudo_size")).toUInt();
    CommonSettings::commonSettings.character_delete_time			= settings->value(QLatin1Literal("character_delete_time")).toUInt();
    CommonSettings::commonSettings.useSP                            = settings->value(QLatin1Literal("useSP")).toBool();
    CommonSettings::commonSettings.autoLearn                        = settings->value(QLatin1Literal("autoLearn")).toBool() && !CommonSettings::commonSettings.useSP;
    CommonSettings::commonSettings.forcedSpeed                      = settings->value(QLatin1Literal("forcedSpeed")).toUInt();
    CommonSettings::commonSettings.dontSendPseudo					= settings->value(QLatin1Literal("dontSendPseudo")).toBool();
    CommonSettings::commonSettings.forceClientToSendAtMapChange		= settings->value(QLatin1Literal("forceClientToSendAtMapChange")).toBool();
    formatedServerSettings.dontSendPlayerType                       = settings->value(QLatin1Literal("dontSendPlayerType")).toBool();

    //the listen
    formatedServerNormalSettings.server_port			= settings->value(QLatin1Literal("server-port")).toUInt();
    formatedServerNormalSettings.server_ip				= settings->value(QLatin1Literal("server-ip")).toString();
    formatedServerNormalSettings.proxy					= settings->value(QLatin1Literal("proxy")).toString();
    formatedServerNormalSettings.proxy_port				= settings->value(QLatin1Literal("proxy_port")).toUInt();
    formatedServerNormalSettings.useSsl					= settings->value(QLatin1Literal("useSsl")).toBool();
    formatedServerSettings.anonymous					= settings->value(QLatin1Literal("anonymous")).toBool();
    formatedServerSettings.server_message				= settings->value(QLatin1Literal("server_message")).toString();

    CommonSettings::commonSettings.httpDatapackMirror			= settings->value(QLatin1Literal("httpDatapackMirror")).toString();
    formatedServerSettings.datapackCache				= settings->value(QLatin1Literal("datapackCache")).toInt();
    #ifdef Q_OS_LINUX
    settings->beginGroup(QLatin1Literal("Linux"));
    formatedServerNormalSettings.linuxSettings.tcpCork		= settings->value(QLatin1Literal("tcpCork")).toBool();
    settings->endGroup();
    #endif

    //fight
    //CommonSettings::commonSettings.pvp			= settings->value(QLatin1Literal("pvp")).toBool();
    formatedServerSettings.sendPlayerNumber         = settings->value(QLatin1Literal("sendPlayerNumber")).toBool();

    //rates
    settings->beginGroup(QLatin1Literal("rates"));
    CommonSettings::commonSettings.rates_xp             = settings->value(QLatin1Literal("xp_normal")).toReal();
    CommonSettings::commonSettings.rates_gold			= settings->value(QLatin1Literal("gold_normal")).toReal();
    CommonSettings::commonSettings.rates_xp_pow			= settings->value(QLatin1Literal("xp_pow_normal")).toReal();
    CommonSettings::commonSettings.rates_drop			= settings->value(QLatin1Literal("drop_normal")).toReal();
    //formatedServerSettings.rates_xp_premium                         = settings->value(QLatin1Literal("xp_premium")).toReal();
    //formatedServerSettings.rates_gold_premium                       = settings->value(QLatin1Literal("gold_premium")).toReal();
    /*CommonSettings::commonSettings.rates_shiny		= settings->value(QLatin1Literal("shiny_normal")).toReal();
    formatedServerSettings.rates_shiny_premium                      = settings->value(QLatin1Literal("shiny_premium")).toReal();*/
    settings->endGroup();

    settings->beginGroup(QLatin1Literal("DDOS"));
    CommonSettings::commonSettings.waitBeforeConnectAfterKick         = settings->value(QLatin1Literal("waitBeforeConnectAfterKick")).toUInt();
    formatedServerSettings.ddos.computeAverageValueNumberOfValue      = settings->value(QLatin1Literal("computeAverageValueNumberOfValue")).toUInt();
    formatedServerSettings.ddos.computeAverageValueTimeInterval       = settings->value(QLatin1Literal("computeAverageValueTimeInterval")).toUInt();
    formatedServerSettings.ddos.kickLimitMove                         = settings->value(QLatin1Literal("kickLimitMove")).toUInt();
    formatedServerSettings.ddos.kickLimitChat                         = settings->value(QLatin1Literal("kickLimitChat")).toUInt();
    formatedServerSettings.ddos.kickLimitOther                        = settings->value(QLatin1Literal("kickLimitOther")).toUInt();
    formatedServerSettings.ddos.dropGlobalChatMessageGeneral          = settings->value(QLatin1Literal("dropGlobalChatMessageGeneral")).toUInt();
    formatedServerSettings.ddos.dropGlobalChatMessageLocalClan        = settings->value(QLatin1Literal("dropGlobalChatMessageLocalClan")).toUInt();
    formatedServerSettings.ddos.dropGlobalChatMessagePrivate          = settings->value(QLatin1Literal("dropGlobalChatMessagePrivate")).toUInt();
    settings->endGroup();

    {
        settings->beginGroup(QLatin1Literal("programmedEvent"));
            const QStringList &tempListType=settings->childGroups();
            int indexType=0;
            while(indexType<tempListType.size())
            {
                const QString &type=tempListType.at(indexType);
                settings->beginGroup(type);
                    const QStringList &tempList=settings->childGroups();
                    int index=0;
                    while(index<tempList.size())
                    {
                        const QString &groupName=tempList.at(index);
                        settings->beginGroup(groupName);
                        if(settings->contains(QLatin1Literal("value")) && settings->contains(QLatin1Literal("cycle")) && settings->contains(QLatin1Literal("offset")))
                        {
                            ServerSettings::ProgrammedEvent event;
                            event.value=settings->value(QLatin1Literal("value")).toString();
                            bool ok;
                            event.cycle=settings->value(QLatin1Literal("cycle")).toUInt(&ok);
                            if(!ok)
                                event.cycle=0;
                            event.offset=settings->value(QLatin1Literal("offset")).toUInt(&ok);
                            if(!ok)
                                event.offset=0;
                            if(event.cycle>0)
                                formatedServerSettings.programmedEventList[type][groupName]=event;
                        }
                        settings->endGroup();
                        index++;
                    }
                settings->endGroup();
                indexType++;
            }
        settings->endGroup();
    }

    //chat allowed
    settings->beginGroup(QLatin1Literal("chat"));
    CommonSettings::commonSettings.chat_allow_all         = settings->value(QLatin1Literal("allow-all")).toBool();
    CommonSettings::commonSettings.chat_allow_local		= settings->value(QLatin1Literal("allow-local")).toBool();
    CommonSettings::commonSettings.chat_allow_private		= settings->value(QLatin1Literal("allow-private")).toBool();
    //CommonSettings::commonSettings.chat_allow_aliance		= settings->value(QLatin1Literal("allow-aliance")).toBool();
    CommonSettings::commonSettings.chat_allow_clan		= settings->value(QLatin1Literal("allow-clan")).toBool();
    settings->endGroup();

    settings->beginGroup(QLatin1Literal("db"));
    if(settings->value(QLatin1Literal("type")).toString()==QLatin1Literal("mysql"))
        formatedServerSettings.database.type					= CatchChallenger::ServerSettings::Database::DatabaseType_Mysql;
    else if(settings->value(QLatin1Literal("type")).toString()==QLatin1Literal("sqlite"))
        formatedServerSettings.database.type					= CatchChallenger::ServerSettings::Database::DatabaseType_SQLite;
    else if(settings->value(QLatin1Literal("type")).toString()==QLatin1Literal("postgresql"))
        formatedServerSettings.database.type					= CatchChallenger::ServerSettings::Database::DatabaseType_PostgreSQL;
    else
        formatedServerSettings.database.type					= CatchChallenger::ServerSettings::Database::DatabaseType_Mysql;
    switch(formatedServerSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            formatedServerSettings.database.mysql.host				= settings->value(QLatin1Literal("mysql_host")).toString();
            formatedServerSettings.database.mysql.db				= settings->value(QLatin1Literal("mysql_db")).toString();
            formatedServerSettings.database.mysql.login				= settings->value(QLatin1Literal("mysql_login")).toString();
            formatedServerSettings.database.mysql.pass				= settings->value(QLatin1Literal("mysql_pass")).toString();
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            formatedServerSettings.database.sqlite.file				= settings->value(QLatin1Literal("file")).toString();
        break;
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            formatedServerSettings.database.mysql.host				= settings->value(QLatin1Literal("mysql_host")).toString();
            formatedServerSettings.database.mysql.db				= settings->value(QLatin1Literal("mysql_db")).toString();
            formatedServerSettings.database.mysql.login				= settings->value(QLatin1Literal("mysql_login")).toString();
            formatedServerSettings.database.mysql.pass				= settings->value(QLatin1Literal("mysql_pass")).toString();
        break;
    }
    if(settings->value(QLatin1Literal("db_fight_sync")).toString()==QLatin1Literal("FightSync_AtEachTurn"))
        formatedServerSettings.database.fightSync                       = CatchChallenger::ServerSettings::Database::FightSync_AtEachTurn;
    else if(settings->value(QLatin1Literal("db_fight_sync")).toString()==QLatin1Literal("FightSync_AtTheDisconnexion"))
        formatedServerSettings.database.fightSync                       = CatchChallenger::ServerSettings::Database::FightSync_AtTheDisconnexion;
    else
        formatedServerSettings.database.fightSync                       = CatchChallenger::ServerSettings::Database::FightSync_AtTheEndOfBattle;
    formatedServerSettings.database.positionTeleportSync=settings->value(QLatin1Literal("positionTeleportSync")).toBool();
    formatedServerSettings.database.secondToPositionSync=settings->value(QLatin1Literal("secondToPositionSync")).toUInt();
    settings->endGroup();

    //connection
    formatedServerSettings.automatic_account_creation   = settings->value(QLatin1Literal("automatic_account_creation")).toBool();
    formatedServerSettings.max_players					= settings->value(QLatin1Literal("max-players")).toUInt();
    formatedServerSettings.tolerantMode                 = settings->value(QLatin1Literal("tolerantMode")).toBool();

    //visibility algorithm
    settings->beginGroup(QLatin1Literal("MapVisibilityAlgorithm"));
    switch(settings->value(QLatin1Literal("MapVisibilityAlgorithm")).toUInt())
    {
        case 0:
            formatedServerSettings.mapVisibility.mapVisibilityAlgorithm		= MapVisibilityAlgorithmSelection_Simple;
        break;
        case 1:
            formatedServerSettings.mapVisibility.mapVisibilityAlgorithm		= MapVisibilityAlgorithmSelection_None;
        break;
        case 2:
            formatedServerSettings.mapVisibility.mapVisibilityAlgorithm		= MapVisibilityAlgorithmSelection_WithBorder;
        break;
        default:
            formatedServerSettings.mapVisibility.mapVisibilityAlgorithm		= MapVisibilityAlgorithmSelection_Simple;
        break;
    }
    settings->endGroup();
    if(formatedServerSettings.mapVisibility.mapVisibilityAlgorithm==MapVisibilityAlgorithmSelection_Simple)
    {
        settings->beginGroup(QLatin1Literal("MapVisibilityAlgorithm-Simple"));
        formatedServerSettings.mapVisibility.simple.max				= settings->value(QLatin1Literal("Max")).toUInt();
        formatedServerSettings.mapVisibility.simple.reshow			= settings->value(QLatin1Literal("Reshow")).toUInt();
        formatedServerSettings.mapVisibility.simple.reemit          = settings->value(QLatin1Literal("Reemit")).toBool();
        settings->endGroup();
    }
    else if(formatedServerSettings.mapVisibility.mapVisibilityAlgorithm==MapVisibilityAlgorithmSelection_WithBorder)
    {
        settings->beginGroup(QLatin1Literal("MapVisibilityAlgorithm-WithBorder"));
        formatedServerSettings.mapVisibility.withBorder.maxWithBorder	= settings->value(QLatin1Literal("MaxWithBorder")).toUInt();
        formatedServerSettings.mapVisibility.withBorder.reshowWithBorder= settings->value(QLatin1Literal("ReshowWithBorder")).toUInt();
        formatedServerSettings.mapVisibility.withBorder.max				= settings->value(QLatin1Literal("Max")).toUInt();
        formatedServerSettings.mapVisibility.withBorder.reshow			= settings->value(QLatin1Literal("Reshow")).toUInt();
        settings->endGroup();
    }

    settings->beginGroup(QLatin1Literal("city"));
    if(settings->value(QLatin1Literal("capture_frequency")).toString()==QLatin1Literal("week"))
        formatedServerSettings.city.capture.frenquency=City::Capture::Frequency_week;
    else
        formatedServerSettings.city.capture.frenquency=City::Capture::Frequency_month;

    if(settings->value(QLatin1Literal("capture_day")).toString()==QLatin1Literal("monday"))
        formatedServerSettings.city.capture.day=City::Capture::Monday;
    else if(settings->value(QLatin1Literal("capture_day")).toString()==QLatin1Literal("tuesday"))
        formatedServerSettings.city.capture.day=City::Capture::Tuesday;
    else if(settings->value(QLatin1Literal("capture_day")).toString()==QLatin1Literal("wednesday"))
        formatedServerSettings.city.capture.day=City::Capture::Wednesday;
    else if(settings->value(QLatin1Literal("capture_day")).toString()==QLatin1Literal("thursday"))
        formatedServerSettings.city.capture.day=City::Capture::Thursday;
    else if(settings->value(QLatin1Literal("capture_day")).toString()==QLatin1Literal("friday"))
        formatedServerSettings.city.capture.day=City::Capture::Friday;
    else if(settings->value(QLatin1Literal("capture_day")).toString()==QLatin1Literal("saturday"))
        formatedServerSettings.city.capture.day=City::Capture::Saturday;
    else if(settings->value(QLatin1Literal("capture_day")).toString()==QLatin1Literal("sunday"))
        formatedServerSettings.city.capture.day=City::Capture::Sunday;
    else
        formatedServerSettings.city.capture.day=City::Capture::Monday;
    formatedServerSettings.city.capture.hour=0;
    formatedServerSettings.city.capture.minute=0;
    const QStringList &capture_time_string_list=settings->value(QLatin1Literal("capture_time")).toString().split(QLatin1Literal(":"));
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

    server.setSettings(formatedServerSettings);
    server.setNormalSettings(formatedServerNormalSettings);
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
