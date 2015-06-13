#include <iostream>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <ctime>

#include "epoll/EpollSocket.h"
#include "epoll/EpollSslClient.h"
#include "epoll/EpollSslServer.h"
#include "epoll/EpollClient.h"
#include "epoll/EpollUnixSocketClientFinal.h"
#include "epoll/EpollServer.h"
#include "epoll/EpollUnixSocketServer.h"
#include "epoll/Epoll.h"
#include "epoll/EpollTimer.h"
#include "epoll/timer/TimerCityCapture.h"
#include "epoll/timer/TimerDdos.h"
#include "epoll/timer/TimerDisplayEventBySeconds.h"
#include "epoll/timer/TimerPositionSync.h"
#include "epoll/timer/TimerSendInsertMoveRemove.h"
#include "base/ServerStructures.h"
#include "NormalServerGlobal.h"
#include "base/GlobalServerData.h"
#include "base/Client.h"
#include "base/ClientMapManagement/MapVisibilityAlgorithm_None.h"
#include "base/ClientMapManagement/MapVisibilityAlgorithm_Simple_StoreOnSender.h"
#include "base/ClientMapManagement/MapVisibilityAlgorithm_WithBorder_StoreOnSender.h"
#include "../general/base/FacilityLib.h"
#include "../general/base/GeneralVariable.h"
#include "game-server-alone/LoginLinkToMaster.h"

#define MAXEVENTS 512

using namespace CatchChallenger;

#ifdef SERVERSSL
EpollSslServer *server=NULL;
#else
EpollServer *server=NULL;
#endif
QSettings *settings=NULL;

QString master_host;
quint16 master_port;
quint8 master_tryInterval;
quint8 master_considerDownAfterNumberOfTry;

void send_settings()
{
    GameServerSettings formatedServerSettings=server->getSettings();
    NormalServerSettings formatedServerNormalSettings=server->getNormalSettings();

    //common var
    CommonSettingsCommon::commonSettingsCommon.min_character					= settings->value(QLatin1Literal("min_character")).toUInt();
    CommonSettingsCommon::commonSettingsCommon.max_character					= settings->value(QLatin1Literal("max_character")).toUInt();
    CommonSettingsCommon::commonSettingsCommon.max_pseudo_size					= settings->value(QLatin1Literal("max_pseudo_size")).toUInt();
    CommonSettingsCommon::commonSettingsCommon.character_delete_time			= settings->value(QLatin1Literal("character_delete_time")).toUInt();
    CommonSettingsServer::commonSettingsServer.useSP                            = settings->value(QLatin1Literal("useSP")).toBool();
    CommonSettingsServer::commonSettingsServer.autoLearn                        = settings->value(QLatin1Literal("autoLearn")).toBool() && !CommonSettingsServer::commonSettingsServer.useSP;
    CommonSettingsServer::commonSettingsServer.forcedSpeed                      = settings->value(QLatin1Literal("forcedSpeed")).toUInt();
    CommonSettingsServer::commonSettingsServer.dontSendPseudo					= settings->value(QLatin1Literal("dontSendPseudo")).toBool();
    CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange		= settings->value(QLatin1Literal("forceClientToSendAtMapChange")).toBool();
    CommonSettingsServer::commonSettingsServer.exportedXml                      = settings->value(QLatin1Literal("exportedXml")).toString();
    formatedServerSettings.dontSendPlayerType                                   = settings->value(QLatin1Literal("dontSendPlayerType")).toBool();
    CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters                = settings->value(QLatin1Literal("maxPlayerMonsters")).toUInt();
    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters       = settings->value(QLatin1Literal("maxWarehousePlayerMonsters")).toUInt();
    CommonSettingsCommon::commonSettingsCommon.maxPlayerItems                   = settings->value(QLatin1Literal("maxPlayerItems")).toUInt();
    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems          = settings->value(QLatin1Literal("maxWarehousePlayerItems")).toUInt();
    if(settings->value(QLatin1Literal("compression")).toString()==QStringLiteral("none"))
        formatedServerSettings.compressionType                                = CompressionType_None;
    else if(settings->value(QLatin1Literal("compression")).toString()==QStringLiteral("xz"))
        formatedServerSettings.compressionType                                = CompressionType_Xz;
    else
        formatedServerSettings.compressionType                                = CompressionType_Zlib;

    //the listen
    formatedServerNormalSettings.server_port			= settings->value(QLatin1Literal("server-port")).toUInt();
    formatedServerNormalSettings.server_ip				= settings->value(QLatin1Literal("server-ip")).toString();
    formatedServerNormalSettings.proxy					= settings->value(QLatin1Literal("proxy")).toString();
    formatedServerNormalSettings.proxy_port				= settings->value(QLatin1Literal("proxy_port")).toUInt();
    formatedServerNormalSettings.useSsl					= settings->value(QLatin1Literal("useSsl")).toBool();

    CommonSettingsServer::commonSettingsServer.mainDatapackCode             = settings->value(QLatin1Literal("mainDatapackCode")).toString();
    if(CommonSettingsServer::commonSettingsServer.mainDatapackCode.isEmpty())
    {
        DebugClass::debugConsole(QStringLiteral("mainDatapackCode is empty, please put it into the settings"));
        abort();
    }
    CommonSettingsServer::commonSettingsServer.subDatapackCode              = settings->value(QLatin1Literal("subDatapackCode")).toString();
    formatedServerSettings.anonymous					= settings->value(QLatin1Literal("anonymous")).toBool();
    formatedServerSettings.server_message				= settings->value(QLatin1Literal("server_message")).toString();
    CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase	= settings->value(QLatin1Literal("httpDatapackMirror")).toString();
    CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer=CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase;
    formatedServerSettings.datapackCache				= settings->value(QLatin1Literal("datapackCache")).toInt();
    #ifdef Q_OS_LINUX
    settings->beginGroup(QLatin1Literal("Linux"));
    CommonSettingsServer::commonSettingsServer.tcpCork	= settings->value(QLatin1Literal("tcpCork")).toBool();
    formatedServerNormalSettings.tcpNodelay= settings->value(QLatin1Literal("tcpNodelay")).toBool();
    settings->endGroup();
    #endif

    //fight
    //CommonSettingsCommon::commonSettingsCommon.pvp			= settings->value(QLatin1Literal("pvp")).toBool();
    formatedServerSettings.sendPlayerNumber         = settings->value(QLatin1Literal("sendPlayerNumber")).toBool();

    //rates
    settings->beginGroup(QLatin1Literal("rates"));
    CommonSettingsServer::commonSettingsServer.rates_xp             = settings->value(QLatin1Literal("xp_normal")).toReal();
    CommonSettingsServer::commonSettingsServer.rates_gold			= settings->value(QLatin1Literal("gold_normal")).toReal();
    CommonSettingsServer::commonSettingsServer.rates_xp_pow			= settings->value(QLatin1Literal("xp_pow_normal")).toReal();
    CommonSettingsServer::commonSettingsServer.rates_drop			= settings->value(QLatin1Literal("drop_normal")).toReal();
    //formatedServerSettings.rates_xp_premium                         = settings->value(QLatin1Literal("xp_premium")).toReal();
    //formatedServerSettings.rates_gold_premium                       = settings->value(QLatin1Literal("gold_premium")).toReal();
    /*CommonSettingsCommon::commonSettingsCommon.rates_shiny		= settings->value(QLatin1Literal("shiny_normal")).toReal();
    formatedServerSettings.rates_shiny_premium                      = settings->value(QLatin1Literal("shiny_premium")).toReal();*/
    settings->endGroup();

    settings->beginGroup(QLatin1Literal("DDOS"));
    CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick         = settings->value(QLatin1Literal("waitBeforeConnectAfterKick")).toUInt();
    formatedServerSettings.ddos.computeAverageValueNumberOfValue      = settings->value(QLatin1Literal("computeAverageValueNumberOfValue")).toUInt();
    formatedServerSettings.ddos.computeAverageValueTimeInterval       = settings->value(QLatin1Literal("computeAverageValueTimeInterval")).toUInt();
    formatedServerSettings.ddos.kickLimitMove                         = settings->value(QLatin1Literal("kickLimitMove")).toUInt();
    formatedServerSettings.ddos.kickLimitChat                         = settings->value(QLatin1Literal("kickLimitChat")).toUInt();
    formatedServerSettings.ddos.kickLimitOther                        = settings->value(QLatin1Literal("kickLimitOther")).toUInt();
    formatedServerSettings.ddos.dropGlobalChatMessageGeneral          = settings->value(QLatin1Literal("dropGlobalChatMessageGeneral")).toUInt();
    formatedServerSettings.ddos.dropGlobalChatMessageLocalClan        = settings->value(QLatin1Literal("dropGlobalChatMessageLocalClan")).toUInt();
    formatedServerSettings.ddos.dropGlobalChatMessagePrivate          = settings->value(QLatin1Literal("dropGlobalChatMessagePrivate")).toUInt();
    settings->endGroup();

    //chat allowed
    settings->beginGroup(QLatin1Literal("chat"));
    CommonSettingsServer::commonSettingsServer.chat_allow_all         = settings->value(QLatin1Literal("allow-all")).toBool();
    CommonSettingsServer::commonSettingsServer.chat_allow_local		= settings->value(QLatin1Literal("allow-local")).toBool();
    CommonSettingsServer::commonSettingsServer.chat_allow_private		= settings->value(QLatin1Literal("allow-private")).toBool();
    //CommonSettingsServer::commonSettingsServer.chat_allow_aliance		= settings->value(QLatin1Literal("allow-aliance")).toBool();
    CommonSettingsServer::commonSettingsServer.chat_allow_clan		= settings->value(QLatin1Literal("allow-clan")).toBool();
    settings->endGroup();

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    settings->beginGroup(QLatin1Literal("db-login"));
    if(settings->value(QLatin1Literal("type")).toString()==QLatin1Literal("mysql"))
        formatedServerSettings.database_login.tryOpenType					= DatabaseBase::Type::Mysql;
    else if(settings->value(QLatin1Literal("type")).toString()==QLatin1Literal("sqlite"))
        formatedServerSettings.database_login.tryOpenType					= DatabaseBase::Type::SQLite;
    else if(settings->value(QLatin1Literal("type")).toString()==QLatin1Literal("postgresql"))
        formatedServerSettings.database_login.tryOpenType					= DatabaseBase::Type::PostgreSQL;
    else
        formatedServerSettings.database_login.tryOpenType					= DatabaseBase::Type::Mysql;
    switch(formatedServerSettings.database_login.tryOpenType)
    {
        default:
        case DatabaseBase::Type::PostgreSQL:
        case DatabaseBase::Type::Mysql:
            formatedServerSettings.database_login.host				= settings->value(QLatin1Literal("host")).toString();
            formatedServerSettings.database_login.db				= settings->value(QLatin1Literal("db")).toString();
            formatedServerSettings.database_login.login				= settings->value(QLatin1Literal("login")).toString();
            formatedServerSettings.database_login.pass				= settings->value(QLatin1Literal("pass")).toString();
        break;
        case DatabaseBase::Type::SQLite:
            formatedServerSettings.database_login.file				= settings->value(QLatin1Literal("file")).toString();
        break;
    }
    formatedServerSettings.database_login.tryInterval       = settings->value(QLatin1Literal("tryInterval")).toUInt();
    formatedServerSettings.database_login.considerDownAfterNumberOfTry = settings->value(QLatin1Literal("considerDownAfterNumberOfTry")).toUInt();
    settings->endGroup();
    #endif

    settings->beginGroup(QLatin1Literal("db-base"));
    if(settings->value(QLatin1Literal("type")).toString()==QLatin1Literal("mysql"))
        formatedServerSettings.database_base.tryOpenType                   = DatabaseBase::Type::Mysql;
    else if(settings->value(QLatin1Literal("type")).toString()==QLatin1Literal("sqlite"))
        formatedServerSettings.database_base.tryOpenType                   = DatabaseBase::Type::SQLite;
    else if(settings->value(QLatin1Literal("type")).toString()==QLatin1Literal("postgresql"))
        formatedServerSettings.database_base.tryOpenType                   = DatabaseBase::Type::PostgreSQL;
    else
        formatedServerSettings.database_base.tryOpenType                   = DatabaseBase::Type::Mysql;
    switch(formatedServerSettings.database_base.tryOpenType)
    {
        default:
        case DatabaseBase::Type::PostgreSQL:
        case DatabaseBase::Type::Mysql:
            formatedServerSettings.database_base.host              = settings->value(QLatin1Literal("host")).toString();
            formatedServerSettings.database_base.db                = settings->value(QLatin1Literal("db")).toString();
            formatedServerSettings.database_base.login             = settings->value(QLatin1Literal("login")).toString();
            formatedServerSettings.database_base.pass              = settings->value(QLatin1Literal("pass")).toString();
        break;
        case DatabaseBase::Type::SQLite:
            formatedServerSettings.database_base.file              = settings->value(QLatin1Literal("file")).toString();
        break;
    }
    formatedServerSettings.database_base.tryInterval       = settings->value(QLatin1Literal("tryInterval")).toUInt();
    formatedServerSettings.database_base.considerDownAfterNumberOfTry = settings->value(QLatin1Literal("considerDownAfterNumberOfTry")).toUInt();
    settings->endGroup();

    settings->beginGroup(QLatin1Literal("db-common"));
    if(settings->value(QLatin1Literal("type")).toString()==QLatin1Literal("mysql"))
        formatedServerSettings.database_common.tryOpenType					= DatabaseBase::Type::Mysql;
    else if(settings->value(QLatin1Literal("type")).toString()==QLatin1Literal("sqlite"))
        formatedServerSettings.database_common.tryOpenType					= DatabaseBase::Type::SQLite;
    else if(settings->value(QLatin1Literal("type")).toString()==QLatin1Literal("postgresql"))
        formatedServerSettings.database_common.tryOpenType					= DatabaseBase::Type::PostgreSQL;
    else
        formatedServerSettings.database_common.tryOpenType					= DatabaseBase::Type::Mysql;
    switch(formatedServerSettings.database_common.tryOpenType)
    {
        default:
        case DatabaseBase::Type::PostgreSQL:
        case DatabaseBase::Type::Mysql:
            formatedServerSettings.database_common.host				= settings->value(QLatin1Literal("host")).toString();
            formatedServerSettings.database_common.db				= settings->value(QLatin1Literal("db")).toString();
            formatedServerSettings.database_common.login				= settings->value(QLatin1Literal("login")).toString();
            formatedServerSettings.database_common.pass				= settings->value(QLatin1Literal("pass")).toString();
        break;
        case DatabaseBase::Type::SQLite:
            formatedServerSettings.database_common.file				= settings->value(QLatin1Literal("file")).toString();
        break;
    }
    formatedServerSettings.database_common.tryInterval       = settings->value(QLatin1Literal("tryInterval")).toUInt();
    formatedServerSettings.database_common.considerDownAfterNumberOfTry = settings->value(QLatin1Literal("considerDownAfterNumberOfTry")).toUInt();
    settings->endGroup();

    settings->beginGroup(QLatin1Literal("db-server"));
    if(settings->value(QLatin1Literal("type")).toString()==QLatin1Literal("mysql"))
        formatedServerSettings.database_server.tryOpenType					= DatabaseBase::Type::Mysql;
    else if(settings->value(QLatin1Literal("type")).toString()==QLatin1Literal("sqlite"))
        formatedServerSettings.database_server.tryOpenType					= DatabaseBase::Type::SQLite;
    else if(settings->value(QLatin1Literal("type")).toString()==QLatin1Literal("postgresql"))
        formatedServerSettings.database_server.tryOpenType					= DatabaseBase::Type::PostgreSQL;
    else
        formatedServerSettings.database_server.tryOpenType					= DatabaseBase::Type::Mysql;
    switch(formatedServerSettings.database_server.tryOpenType)
    {
        default:
        case DatabaseBase::Type::PostgreSQL:
        case DatabaseBase::Type::Mysql:
            formatedServerSettings.database_server.host				= settings->value(QLatin1Literal("host")).toString();
            formatedServerSettings.database_server.db				= settings->value(QLatin1Literal("db")).toString();
            formatedServerSettings.database_server.login				= settings->value(QLatin1Literal("login")).toString();
            formatedServerSettings.database_server.pass				= settings->value(QLatin1Literal("pass")).toString();
        break;
        case DatabaseBase::Type::SQLite:
            formatedServerSettings.database_server.file				= settings->value(QLatin1Literal("file")).toString();
        break;
    }
    formatedServerSettings.database_server.tryInterval       = settings->value(QLatin1Literal("tryInterval")).toUInt();
    formatedServerSettings.database_server.considerDownAfterNumberOfTry = settings->value(QLatin1Literal("considerDownAfterNumberOfTry")).toUInt();
    settings->endGroup();

    settings->beginGroup(QLatin1Literal("db"));
    if(settings->value(QLatin1Literal("db_fight_sync")).toString()==QLatin1Literal("FightSync_AtEachTurn"))
        formatedServerSettings.fightSync                       = CatchChallenger::GameServerSettings::FightSync_AtEachTurn;
    else if(settings->value(QLatin1Literal("db_fight_sync")).toString()==QLatin1Literal("FightSync_AtTheDisconnexion"))
        formatedServerSettings.fightSync                       = CatchChallenger::GameServerSettings::FightSync_AtTheDisconnexion;
    else
        formatedServerSettings.fightSync                       = CatchChallenger::GameServerSettings::FightSync_AtTheEndOfBattle;
    formatedServerSettings.positionTeleportSync=settings->value(QLatin1Literal("positionTeleportSync")).toBool();
    formatedServerSettings.secondToPositionSync=settings->value(QLatin1Literal("secondToPositionSync")).toUInt();
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
                            GameServerSettings::ProgrammedEvent event;
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

    {
        bool ok;
        settings->beginGroup(QStringLiteral("master"));
        master_host=settings->value(QStringLiteral("host")).toString();
        master_port=settings->value(QStringLiteral("port")).toUInt(&ok);
        if(master_port==0 || !ok)
        {
            std::cerr << "Master port not a number or 0:" << settings->value(QStringLiteral("port")).toString().toStdString() << std::endl;
            abort();
        }
        master_tryInterval=settings->value(QStringLiteral("tryInterval")).toUInt(&ok);
        if(master_tryInterval==0 || !ok)
        {
            std::cerr << "tryInterval==0 (abort)" << std::endl;
            abort();
        }
        master_considerDownAfterNumberOfTry=settings->value(QStringLiteral("considerDownAfterNumberOfTry")).toUInt(&ok);
        if(master_considerDownAfterNumberOfTry==0 || !ok)
        {
            std::cerr << "considerDownAfterNumberOfTry==0 (abort)" << std::endl;
            abort();
        }
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

    if(QCoreApplication::arguments().contains("--benchmark"))
        GlobalServerData::serverSettings.benchmark=true;

    server->setSettings(formatedServerSettings);
    server->setNormalSettings(formatedServerNormalSettings);
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Q_UNUSED(a);

    bool datapack_loaded=false;

    QFileInfo datapackFolder(QCoreApplication::applicationDirPath()+QLatin1Literal("/datapack/informations.xml"));
    if(!datapackFolder.isFile())
    {
        qDebug() << "No datapack found into: " << datapackFolder.absoluteFilePath();
        return EXIT_FAILURE;
    }

    const QString &configFile=QCoreApplication::applicationDirPath()+QLatin1Literal("/server.properties");
    settings=new QSettings(configFile,QSettings::IniFormat);
    if(settings->status()!=QSettings::NoError)
    {
        qDebug() << "Error settings (1): " << settings->status();
        return EXIT_FAILURE;
    }
    NormalServerGlobal::checkSettingsFile(settings,QCoreApplication::applicationDirPath()+QLatin1Literal("/datapack/"));

    if(settings->status()!=QSettings::NoError)
    {
        qDebug() << "Error settings (2): " << settings->status();
        return EXIT_FAILURE;
    }
    if(!Epoll::epoll.init())
        return EPOLLERR;

    #ifdef SERVERSSL
    server=new EpollSslServer();
    #else
    server=new EpollServer();
    #endif

    //before linkToMaster->registerGameServer() to have the correct settings loaded
    //after server to have the settings
    send_settings();

    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    const int &linkfd=LoginLinkToMaster::tryConnect(
                master_host.toLocal8Bit().constData(),
                master_port,
                master_tryInterval,
                master_considerDownAfterNumberOfTry
                );
    if(linkfd<0)
    {
        std::cerr << "Unable to connect on master" << std::endl;
        abort();
    }
    #ifdef SERVERSSL
    ctx from what?
    LoginLinkToMaster::loginLinkToMaster=new LoginLinkToMaster(linkfd,ctx);
    #else
    LoginLinkToMaster::loginLinkToMaster=new LoginLinkToMaster(linkfd);
    #endif
    {
        epoll_event event;
        event.data.ptr = LoginLinkToMaster::loginLinkToMaster;
        event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;//EPOLLET | EPOLLOUT
        int s = Epoll::epoll.ctl(EPOLL_CTL_ADD, linkfd, &event);
        if(s == -1)
        {
            std::cerr << "epoll_ctl on socket (master link) error" << std::endl;
            abort();
        }
    }
    LoginLinkToMaster::loginLinkToMaster->setSettings(settings);
    LoginLinkToMaster::loginLinkToMaster->sendProtocolHeader();
    #endif

    bool tcpCork,tcpNodelay;
    {
        const GameServerSettings &formatedServerSettings=server->getSettings();
        const NormalServerSettings &formatedServerNormalSettings=server->getNormalSettings();
        tcpCork=CommonSettingsServer::commonSettingsServer.tcpCork;
        tcpNodelay=formatedServerNormalSettings.tcpNodelay;

        if(!formatedServerNormalSettings.proxy.isEmpty())
        {
            qDebug() << "Proxy not supported: " << settings->status();
            return EXIT_FAILURE;
        }
        if(!formatedServerNormalSettings.proxy.isEmpty())
        {
            qDebug() << "Proxy not supported";
            return EXIT_FAILURE;
        }
        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        if(formatedServerSettings.database_login.tryOpenType!=DatabaseBase::Type::PostgreSQL)
        {
            settings->beginGroup(QLatin1Literal("db-login"));
            qDebug() << "Only postgresql is supported for now: " << settings->value(QLatin1Literal("type")).toString();
            settings->endGroup();
            return EXIT_FAILURE;
        }
        #endif
        if(formatedServerSettings.database_base.tryOpenType!=DatabaseBase::Type::PostgreSQL)
        {
            settings->beginGroup(QLatin1Literal("db-base"));
            qDebug() << "Only postgresql is supported for now: " << settings->value(QLatin1Literal("type")).toString();
            settings->endGroup();
            return EXIT_FAILURE;
        }
        if(formatedServerSettings.database_common.tryOpenType!=DatabaseBase::Type::PostgreSQL)
        {
            settings->beginGroup(QLatin1Literal("db-common"));
            qDebug() << "Only postgresql is supported for now: " << settings->value(QLatin1Literal("type")).toString();
            settings->endGroup();
            return EXIT_FAILURE;
        }
        if(formatedServerSettings.database_server.tryOpenType!=DatabaseBase::Type::PostgreSQL)
        {
            settings->beginGroup(QLatin1Literal("db-server"));
            qDebug() << "Only postgresql is supported for now: " << settings->value(QLatin1Literal("type")).toString();
            settings->endGroup();
            return EXIT_FAILURE;
        }
        #ifdef SERVERSSL
        if(!formatedServerNormalSettings.useSsl)
        {
            qDebug() << "Ssl connexion requested but server not compiled with ssl support!";
            return EXIT_FAILURE;
        }
        #else
        if(formatedServerNormalSettings.useSsl)
        {
            qDebug() << "Clear connexion requested but server compiled with ssl support!";
            return EXIT_FAILURE;
        }
        #endif
        if(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.isEmpty())
        {
            #ifdef CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION
            qDebug() << "Need mirror because CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION is def, need decompression to datapack list input";
            return EXIT_FAILURE;
            #endif
        }
        else
        {
            QStringList newMirrorList;
            QRegularExpression httpMatch("^https?://.+$");
            const QStringList &mirrorList=CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.split(";");
            int index=0;
            while(index<mirrorList.size())
            {
                const QString &mirror=mirrorList.at(index);
                if(!mirror.contains(httpMatch))
                {
                    qDebug() << "Mirror wrong: " << mirror.toLocal8Bit();
                    return EXIT_FAILURE;
                }
                if(mirror.endsWith("/"))
                    newMirrorList << mirror;
                else
                    newMirrorList << mirror+"/";
                index++;
            }
            CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer=newMirrorList.join(";");
            CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase=CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer;
        }
    }
    server->loadAndFixSettings();

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    if(!GlobalServerData::serverPrivateVariables.db_login->syncConnect(
                GlobalServerData::serverSettings.database_login.host.toLatin1(),
                GlobalServerData::serverSettings.database_login.db.toLatin1(),
                GlobalServerData::serverSettings.database_login.login.toLatin1(),
                GlobalServerData::serverSettings.database_login.pass.toLatin1()))
    {
        qDebug() << "Unable to connect to database login:" << GlobalServerData::serverPrivateVariables.db_login->errorMessage();
        return EXIT_FAILURE;
    }
    #endif
    if(!GlobalServerData::serverPrivateVariables.db_base->syncConnect(
                GlobalServerData::serverSettings.database_base.host.toLatin1(),
                GlobalServerData::serverSettings.database_base.db.toLatin1(),
                GlobalServerData::serverSettings.database_base.login.toLatin1(),
                GlobalServerData::serverSettings.database_base.pass.toLatin1()))
    {
        qDebug() << "Unable to connect to database base:" << GlobalServerData::serverPrivateVariables.db_base->errorMessage();
        return EXIT_FAILURE;
    }
    if(!GlobalServerData::serverPrivateVariables.db_common->syncConnect(
                GlobalServerData::serverSettings.database_common.host.toLatin1(),
                GlobalServerData::serverSettings.database_common.db.toLatin1(),
                GlobalServerData::serverSettings.database_common.login.toLatin1(),
                GlobalServerData::serverSettings.database_common.pass.toLatin1()))
    {
        qDebug() << "Unable to connect to database common:" << GlobalServerData::serverPrivateVariables.db_common->errorMessage();
        return EXIT_FAILURE;
    }
    if(!GlobalServerData::serverPrivateVariables.db_server->syncConnect(
                GlobalServerData::serverSettings.database_server.host.toLatin1(),
                GlobalServerData::serverSettings.database_server.db.toLatin1(),
                GlobalServerData::serverSettings.database_server.login.toLatin1(),
                GlobalServerData::serverSettings.database_server.pass.toLatin1()))
    {
        qDebug() << "Unable to connect to database server:" << GlobalServerData::serverPrivateVariables.db_server->errorMessage();
        return EXIT_FAILURE;
    }
    server->initialize_the_database_prepared_query();

    EpollUnixSocketServer unixServer;
    if(!unixServer.tryListen())
    {
        server->close();
        return EPOLLERR;
    }

    TimerCityCapture timerCityCapture;
    TimerDdos timerDdos;
    #ifdef SERVERBENCHMARKFULL
    TimerDisplayEventBySeconds timerDisplayEventBySeconds;
    #endif
    TimerPositionSync timerPositionSync;
    TimerSendInsertMoveRemove timerSendInsertMoveRemove;
    {
        GlobalServerData::serverPrivateVariables.time_city_capture=FacilityLib::nextCaptureTime(GlobalServerData::serverSettings.city);
        const qint64 &time=GlobalServerData::serverPrivateVariables.time_city_capture.toMSecsSinceEpoch()-QDateTime::currentMSecsSinceEpoch();
        timerCityCapture.setSingleShot(true);
        if(!timerCityCapture.start(time))
        {
            std::cerr << "timerCityCapture fail to set" << std::endl;
            return EXIT_FAILURE;
        }
    }
    {
        if(!timerDdos.start(GlobalServerData::serverSettings.ddos.computeAverageValueTimeInterval*1000))
        {
            std::cerr << "timerDdos fail to set" << std::endl;
            return EXIT_FAILURE;
        }
    }
    #ifdef SERVERBENCHMARKFULL
    {
        if(!timerDisplayEventBySeconds.start(60*1000))
        {
            std::cerr << "timerDisplayEventBySeconds fail to set" << std::endl;
            return EXIT_FAILURE;
        }
    }
    #endif
    {
        if(GlobalServerData::serverSettings.secondToPositionSync>0)
            if(!timerPositionSync.start(GlobalServerData::serverSettings.secondToPositionSync*1000))
            {
                std::cerr << "timerPositionSync fail to set" << std::endl;
                return EXIT_FAILURE;
            }
    }
    {
        if(!timerSendInsertMoveRemove.start(CATCHCHALLENGER_SERVER_MAP_TIME_TO_SEND_MOVEMENT))
        {
            std::cerr << "timerSendInsertMoveRemove fail to set" << std::endl;
            return EXIT_FAILURE;
        }
    }
    {
        if(GlobalServerData::serverSettings.sendPlayerNumber)
        {
            if(!GlobalServerData::serverPrivateVariables.player_updater.start())
            {
                std::cerr << "player_updater timer fail to set" << std::endl;
                return EXIT_FAILURE;
            }
            if(!GlobalServerData::serverPrivateVariables.player_updater_to_master.start())
            {
                std::cerr << "player_updater_to_master timer fail to set" << std::endl;
                return EXIT_FAILURE;
            }
        }
    }

    #ifndef SERVERNOBUFFER
    #ifdef SERVERSSL
    EpollSslClient::staticInit();
    #endif
    #endif
    char buf[4096];
    memset(buf,0,4096);
    /* Buffer where events are returned */
    epoll_event events[MAXEVENTS];

    char encodingBuff[1];
    #ifdef SERVERSSL
    encodingBuff[0]=0x01;
    #else
    encodingBuff[0]=0x00;
    #endif

    #ifdef SERVERBENCHMARKFULL
    std::chrono::time_point<std::chrono::high_resolution_clock> start_inter;
    #endif
    int numberOfConnectedClient=0,numberOfConnectedUnixClient=0;
    /* The event loop */
    int number_of_events, i;
    while(1)
    {
        number_of_events = Epoll::epoll.wait(events, MAXEVENTS);
        #ifdef SERVERBENCHMARK
        EpollUnixSocketClientFinal::start = std::chrono::high_resolution_clock::now();
        #endif
        for(i = 0; i < number_of_events; i++)
        {
            switch(static_cast<BaseClassSwitch *>(events[i].data.ptr)->getType())
            {
                case BaseClassSwitch::Type::Server:
                {
                    #ifdef SERVERBENCHMARKFULL
                    timerDisplayEventBySeconds.addServerCount();
                    #endif
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occured on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        std::cerr << "server epoll error" << std::endl;
                        continue;
                    }
                    /* We have a notification on the listening socket, which
                    means one or more incoming connections. */
                    while(1)
                    {
                        sockaddr in_addr;
                        socklen_t in_len = sizeof(in_addr);
                        const int &infd = server->accept(&in_addr, &in_len);
                        if(!server->isReady())
                        {
                            /// \todo dont clean error on client into this case
                            std::cerr << "client connect when the server is not ready" << std::endl;
                            ::close(infd);
                            break;
                        }
                        if(infd == -1)
                        {
                            if((errno == EAGAIN) ||
                            (errno == EWOULDBLOCK))
                            {
                                /* We have processed all incoming
                                connections. */
                                break;
                            }
                            else
                            {
                                std::cout << "connexion accepted" << std::endl;
                                break;
                            }
                        }
                        /* do at the protocol negociation to send the reason
                        if(numberOfConnectedClient>=GlobalServerData::serverSettings.max_players)
                        {
                            ::close(infd);
                            break;
                        }*/

                        /* Make the incoming socket non-blocking and add it to the
                        list of fds to monitor. */
                        numberOfConnectedClient++;

                        int s = EpollSocket::make_non_blocking(infd);
                        if(s == -1)
                            std::cerr << "unable to make to socket non blocking" << std::endl;
                        else
                        {
                            if(tcpCork)
                            {
                                //set cork for CatchChallener because don't have real time part
                                int state = 1;
                                if(setsockopt(infd, IPPROTO_TCP, TCP_CORK, &state, sizeof(state))!=0)
                                    std::cerr << "Unable to apply tcp cork" << std::endl;
                            }
                            else if(tcpNodelay)
                            {
                                //set no delay to don't try group the packet and improve the performance
                                int state = 1;
                                if(setsockopt(infd, IPPROTO_TCP, TCP_NODELAY, &state, sizeof(state))!=0)
                                    std::cerr << "Unable to apply tcp no delay" << std::endl;
                            }

                            Client *client;
                            switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
                            {
                                case MapVisibilityAlgorithmSelection_Simple:
                                    client=new MapVisibilityAlgorithm_Simple_StoreOnSender(infd
                                                       #ifdef SERVERSSL
                                                       ,server->getCtx()
                                                       #endif
                                                                                           );
                                break;
                                case MapVisibilityAlgorithmSelection_WithBorder:
                                    client=new MapVisibilityAlgorithm_WithBorder_StoreOnSender(infd
                                                           #ifdef SERVERSSL
                                                           ,server->getCtx()
                                                           #endif
                                                                                               );
                                break;
                                default:
                                case MapVisibilityAlgorithmSelection_None:
                                    client=new MapVisibilityAlgorithm_None(infd
                                       #ifdef SERVERSSL
                                       ,server->getCtx()
                                       #endif
                                                                           );
                                break;
                            }
                            //just for informations
                            {
                                char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
                                const int &s = getnameinfo(&in_addr, in_len,
                                hbuf, sizeof hbuf,
                                sbuf, sizeof sbuf,
                                NI_NUMERICHOST | NI_NUMERICSERV);
                                if(s == 0)
                                {
                                    //std::cout << "Accepted connection on descriptor " << infd << "(host=" << hbuf << ", port=" << sbuf << ")" << std::endl;
                                    client->socketStringSize=strlen(hbuf)+strlen(sbuf)+1+1;
                                    client->socketString=new char[client->socketStringSize];
                                    strcpy(client->socketString,hbuf);
                                    strcat(client->socketString,":");
                                    strcat(client->socketString,sbuf);
                                    client->socketString[client->socketStringSize-1]='\0';
                                }
                            }
                            epoll_event event;
                            event.data.ptr = client;
                            event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;//EPOLLET | EPOLLOUT
                            s = Epoll::epoll.ctl(EPOLL_CTL_ADD, infd, &event);
                            if(s == -1)
                            {
                                std::cerr << "epoll_ctl on socket error" << std::endl;
                                delete client;
                            }
                            else
                                ::write(infd,encodingBuff,sizeof(encodingBuff));
                        }
                    }
                    continue;
                }
                break;
                case BaseClassSwitch::Type::UnixServer:
                {
                    #ifdef SERVERBENCHMARKFULL
                    timerDisplayEventBySeconds.addServerCount();
                    #endif
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occured on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        std::cerr << "unix server epoll error" << std::endl;
                        continue;
                    }
                    /* We have a notification on the listening socket, which
                    means one or more incoming connections. */
                    while(1)
                    {
                        sockaddr in_addr;
                        socklen_t in_len = sizeof(in_addr);
                        const int &infd = unixServer.accept(&in_addr, &in_len);
                        if(infd == -1)
                        {
                            if((errno == EAGAIN) ||
                            (errno == EWOULDBLOCK))
                            {
                                /* We have processed all incoming
                                connections. */
                                break;
                            }
                            else
                            {
                                std::cout << "connexion accepted" << std::endl;
                                break;
                            }
                        }

                        int s = EpollSocket::make_non_blocking(infd);
                        if(s == -1)
                            std::cerr << "unable to make to socket non blocking" << std::endl;
                        else
                        {
                            EpollUnixSocketClientFinal *client=new EpollUnixSocketClientFinal(infd);

                            epoll_event event;
                            event.data.ptr = client;
                            event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;//EPOLLET | EPOLLOUT
                            s = Epoll::epoll.ctl(EPOLL_CTL_ADD, infd, &event);
                            if(s == -1)
                            {
                                std::cerr << "epoll_ctl on socket error" << std::endl;
                                delete client;
                            }
                            if(numberOfConnectedUnixClient==0)
                            {
                                #ifdef SERVERBENCHMARK
                                EpollUnixSocketClientFinal::start = std::chrono::high_resolution_clock::now();
                                EpollUnixSocketClientFinal::timeUsed=0;
                                #ifdef SERVERBENCHMARKFULL
                                EpollUnixSocketClientFinal::timeUsedForTimer=0;
                                EpollUnixSocketClientFinal::timeUsedForUser=0;
                                EpollUnixSocketClientFinal::timeUsedForDatabase=0;
                                #endif
                                #endif
                            }
                            numberOfConnectedUnixClient++;
                            client->parseIncommingData();
                        }
                    }
                    continue;
                }
                break;
                case BaseClassSwitch::Type::Client:
                {
                    #ifdef SERVERBENCHMARKFULL
                    start_inter = std::chrono::high_resolution_clock::now();
                    #endif
                    #ifdef SERVERBENCHMARKFULL
                    timerDisplayEventBySeconds.addClientCount();
                    #endif
                    Client *client=static_cast<Client *>(events[i].data.ptr);
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occured on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        if(!(events[i].events & EPOLLHUP))
                            std::cerr << "client epoll error: " << events[i].events << std::endl;
                        numberOfConnectedClient--;
                        client->disconnectClient();
                        delete client;
                        continue;
                    }
                    //ready to read
                    if(events[i].events & EPOLLIN)
                        client->parseIncommingData();
                    #ifndef SERVERNOBUFFER
                    //ready to write
                    if(events[i].events & EPOLLOUT)
                        if(!closed)
                            client->flush();
                    #endif
                    if(events[i].events & EPOLLHUP || events[i].events & EPOLLRDHUP)
                    {
                        numberOfConnectedClient--;
                        client->disconnectClient();
                        delete client;//disconnected, remove the object
                    }
                    #ifdef SERVERBENCHMARKFULL
                    std::chrono::duration<unsigned long long int,std::nano> elapsed_seconds = std::chrono::high_resolution_clock::now()-start_inter;
                    EpollUnixSocketClientFinal::timeUsedForUser+=elapsed_seconds.count();
                    #endif
                }
                break;
                case BaseClassSwitch::Type::UnixClient:
                {
                    #ifdef SERVERBENCHMARKFULL
                    timerDisplayEventBySeconds.addClientCount();
                    #endif
                    EpollUnixSocketClientFinal *client=static_cast<EpollUnixSocketClientFinal *>(events[i].data.ptr);
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occured on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        std::cerr << "client epoll error: " << events[i].events << std::endl;
                        numberOfConnectedUnixClient--;
                        client->close();
                        delete client;
                        continue;
                    }
                    //ready to read
                    if(events[i].events & EPOLLIN)
                        client->parseIncommingData();
                    if(events[i].events & EPOLLHUP || events[i].events & EPOLLRDHUP)
                    {
                        numberOfConnectedUnixClient--;
                        client->close();
                        delete client;//disconnected, remove the object
                    }
                }
                break;
                case BaseClassSwitch::Type::Timer:
                {
                    #ifdef SERVERBENCHMARKFULL
                    start_inter = std::chrono::high_resolution_clock::now();
                    #endif
                    #ifdef SERVERBENCHMARKFULL
                    timerDisplayEventBySeconds.addTimerCount();
                    #endif
                    static_cast<EpollTimer *>(events[i].data.ptr)->exec();
                    static_cast<EpollTimer *>(events[i].data.ptr)->validateTheTimer();
                    #ifdef SERVERBENCHMARKFULL
                    std::chrono::duration<unsigned long long int,std::nano> elapsed_seconds = std::chrono::high_resolution_clock::now()-start_inter;
                    EpollUnixSocketClientFinal::timeUsedForTimer+=elapsed_seconds.count();
                    #endif
                }
                break;
                case BaseClassSwitch::Type::Database:
                {
                    #ifdef SERVERBENCHMARKFULL
                    start_inter = std::chrono::high_resolution_clock::now();
                    #endif
                    #ifdef SERVERBENCHMARKFULL
                    timerDisplayEventBySeconds.addDbCount();
                    #endif
                    EpollPostgresql *db=static_cast<EpollPostgresql *>(events[i].data.ptr);
                    db->epollEvent(events[i].events);
                    if(!datapack_loaded)
                    {
                        if(db->isConnected())
                        {
                            std::cout << "datapack_loaded not loaded: start preload data " << std::endl;
                            server->preload_the_data();
                            datapack_loaded=true;
                        }
                        else
                            std::cerr << "datapack_loaded not loaded: but database seam don't be connected" << std::endl;
                    }
                    #ifdef SERVERBENCHMARKFULL
                    std::chrono::duration<unsigned long long int,std::nano> elapsed_seconds = std::chrono::high_resolution_clock::now()-start_inter;
                    EpollUnixSocketClientFinal::timeUsedForDatabase+=elapsed_seconds.count();
                    #endif
                    if(!db->isConnected())
                    {
                        std::cerr << "database disconnect, quit now" << std::endl;
                        return EXIT_FAILURE;
                    }
                }
                break;
                default:
                    #ifdef SERVERBENCHMARKFULL
                    timerDisplayEventBySeconds.addOtherCount();
                    #endif
                    std::cerr << "unknown event" << std::endl;
                break;
            }
        }
        #ifdef SERVERBENCHMARK
        std::chrono::duration<unsigned long long int,std::nano> elapsed_seconds = std::chrono::high_resolution_clock::now()-EpollUnixSocketClientFinal::start;
        EpollUnixSocketClientFinal::timeUsed+=elapsed_seconds.count();
        #endif
    }
    server->close();
    server->unload_the_data();
    LoginLinkToMaster::loginLinkToMaster->closeSocket();
    delete server;
    delete LoginLinkToMaster::loginLinkToMaster;
    return EXIT_SUCCESS;
}
