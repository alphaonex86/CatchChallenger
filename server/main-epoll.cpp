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
#include "../general/base/CommonSettingsCommon.h"
#include "../general/base/CommonSettingsServer.h"

#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "game-server-alone/LinkToMaster.h"
#include "epoll/timer/TimerPurgeTokenAuthList.h"
#endif

#define MAXEVENTS 512

using namespace CatchChallenger;

#ifdef SERVERSSL
EpollSslServer *server=NULL;
#else
EpollServer *server=NULL;
#endif
TinyXMLSettings *settings=NULL;

std::string master_host;
uint16_t master_port;
uint8_t master_tryInterval;
uint8_t master_considerDownAfterNumberOfTry;

void send_settings()
{
    GameServerSettings formatedServerSettings=server->getSettings();
    NormalServerSettings formatedServerNormalSettings=server->getNormalSettings();

    //common var
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    CommonSettingsCommon::commonSettingsCommon.min_character					= stringtouint8(settings->value("min_character"));
    CommonSettingsCommon::commonSettingsCommon.max_character					= stringtouint8(settings->value("max_character"));
    CommonSettingsCommon::commonSettingsCommon.max_pseudo_size					= stringtouint8(settings->value("max_pseudo_size"));
    CommonSettingsCommon::commonSettingsCommon.character_delete_time			= stringtouint32(settings->value("character_delete_time"));
    #endif
    CommonSettingsServer::commonSettingsServer.useSP                            = stringtobool(settings->value("useSP"));
    CommonSettingsServer::commonSettingsServer.autoLearn                        = stringtobool(settings->value("autoLearn")) && !CommonSettingsServer::commonSettingsServer.useSP;
    CommonSettingsServer::commonSettingsServer.forcedSpeed                      = stringtouint32(settings->value("forcedSpeed"));
    CommonSettingsServer::commonSettingsServer.dontSendPseudo					= stringtobool(settings->value("dontSendPseudo"));
    CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer  		= stringtobool(settings->value("plantOnlyVisibleByPlayer"));
    CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange		= stringtobool(settings->value("forceClientToSendAtMapChange"));
    CommonSettingsServer::commonSettingsServer.exportedXml                      = settings->value("exportedXml");
    formatedServerSettings.dontSendPlayerType                                   = stringtobool(settings->value("dontSendPlayerType"));
    CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters                = stringtouint32(settings->value("maxPlayerMonsters"));
    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters       = stringtouint32(settings->value("maxWarehousePlayerMonsters"));
    CommonSettingsCommon::commonSettingsCommon.maxPlayerItems                   = stringtouint32(settings->value("maxPlayerItems"));
    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems          = stringtouint32(settings->value("maxWarehousePlayerItems"));
    formatedServerSettings.compressionLevel                                     = stringtouint32(settings->value("compressionLevel"));
    if(settings->value("compression")=="none")
        formatedServerSettings.compressionType                                = CompressionType_None;
    else if(settings->value("compression")=="xz")
        formatedServerSettings.compressionType                                = CompressionType_Xz;
    else if(settings->value("compression")=="lz4")
        formatedServerSettings.compressionType                                = CompressionType_Lz4;
    else
        formatedServerSettings.compressionType                                = CompressionType_Zlib;

    //the listen
    formatedServerNormalSettings.server_port			= stringtouint32(settings->value("server-port"));
    formatedServerNormalSettings.server_ip				= settings->value("server-ip");
    formatedServerNormalSettings.proxy					= settings->value("proxy");
    formatedServerNormalSettings.proxy_port				= stringtouint32(settings->value("proxy_port"));
    formatedServerNormalSettings.useSsl					= stringtobool(settings->value("useSsl"));

    if(settings->contains("mainDatapackCode"))
        CommonSettingsServer::commonSettingsServer.mainDatapackCode=settings->value("mainDatapackCode","[main]");
    else
    {
        const std::vector<CatchChallenger::FacilityLibGeneral::InodeDescriptor> &list=CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(GlobalServerData::serverSettings.datapack_basePath+"/map/main/",CatchChallenger::FacilityLibGeneral::ListFolder::Dirs);
        if(list.empty())
        {
            std::cerr << "No main code detected into the current datapack (abort)" << std::endl;
            settings->sync();
            abort();
        }
        if(list.size()==1)
        {
            settings->setValue("mainDatapackCode",list.at(0).name);
            CommonSettingsServer::commonSettingsServer.mainDatapackCode=list.at(0).name;
        }
    }
    if(settings->contains("subDatapackCode"))
        CommonSettingsServer::commonSettingsServer.subDatapackCode=settings->value("subDatapackCode","");
    else
    {
        const std::vector<CatchChallenger::FacilityLibGeneral::InodeDescriptor> &list=CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(GlobalServerData::serverSettings.datapack_basePath+"/map/main/"+CommonSettingsServer::commonSettingsServer.mainDatapackCode+"/sub/",CatchChallenger::FacilityLibGeneral::ListFolder::Dirs);
        if(!list.empty())
        {
            if(list.size()==1)
            {
                settings->setValue("subDatapackCode",list.at(0).name);
                CommonSettingsServer::commonSettingsServer.subDatapackCode=list.at(0).name;
            }
            else
            {
                std::cerr << "No sub code detected into the current datapack" << std::endl;
                settings->setValue("subDatapackCode","");
                settings->sync();
                CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();
            }
        }
        else
            CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();
    }
    formatedServerSettings.anonymous					= stringtobool(settings->value("anonymous"));
    formatedServerSettings.server_message				= settings->value("server_message");
    CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase	= settings->value("httpDatapackMirror");
    CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer=CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase;
    formatedServerSettings.datapackCache				= stringtoint32(settings->value("datapackCache"));
    #ifdef __linux__
    settings->beginGroup("Linux");
    CommonSettingsServer::commonSettingsServer.tcpCork	= stringtobool(settings->value("tcpCork"));
    formatedServerNormalSettings.tcpNodelay= stringtobool(settings->value("tcpNodelay"));
    settings->endGroup();
    #endif

    //fight
    //CommonSettingsCommon::commonSettingsCommon.pvp			= stringtobool(settings->value("pvp"));
    formatedServerSettings.sendPlayerNumber         = stringtobool(settings->value("sendPlayerNumber"));

    //rates
    settings->beginGroup("rates");
    CommonSettingsServer::commonSettingsServer.rates_xp             = stringtodouble(settings->value("xp_normal"));
    CommonSettingsServer::commonSettingsServer.rates_gold			= stringtodouble(settings->value("gold_normal"));
    CommonSettingsServer::commonSettingsServer.rates_xp_pow			= stringtodouble(settings->value("xp_pow_normal"));
    CommonSettingsServer::commonSettingsServer.rates_drop			= stringtodouble(settings->value("drop_normal"));
    //formatedServerSettings.rates_xp_premium                         = stringtodouble(settings->value("xp_premium"));
    //formatedServerSettings.rates_gold_premium                       = stringtodouble(settings->value("gold_premium"));
    /*CommonSettingsCommon::commonSettingsCommon.rates_shiny		= stringtodouble(settings->value("shiny_normal"));
    formatedServerSettings.rates_shiny_premium                      = stringtodouble(settings->value("shiny_premium"));*/
    settings->endGroup();

    settings->beginGroup("DDOS");
    CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick         = stringtouint32(settings->value("waitBeforeConnectAfterKick"));
    formatedServerSettings.ddos.computeAverageValueNumberOfValue      = stringtouint32(settings->value("computeAverageValueNumberOfValue"));
    formatedServerSettings.ddos.computeAverageValueTimeInterval       = stringtouint32(settings->value("computeAverageValueTimeInterval"));
    #ifdef CATCHCHALLENGER_DDOS_FILTER
    formatedServerSettings.ddos.kickLimitMove                         = stringtouint32(settings->value("kickLimitMove"));
    formatedServerSettings.ddos.kickLimitChat                         = stringtouint32(settings->value("kickLimitChat"));
    formatedServerSettings.ddos.kickLimitOther                        = stringtouint32(settings->value("kickLimitOther"));
    #endif
    formatedServerSettings.ddos.dropGlobalChatMessageGeneral          = stringtouint32(settings->value("dropGlobalChatMessageGeneral"));
    formatedServerSettings.ddos.dropGlobalChatMessageLocalClan        = stringtouint32(settings->value("dropGlobalChatMessageLocalClan"));
    formatedServerSettings.ddos.dropGlobalChatMessagePrivate          = stringtouint32(settings->value("dropGlobalChatMessagePrivate"));
    settings->endGroup();

    //chat allowed
    settings->beginGroup("chat");
    CommonSettingsServer::commonSettingsServer.chat_allow_all         = stringtobool(settings->value("allow-all"));
    CommonSettingsServer::commonSettingsServer.chat_allow_local		= stringtobool(settings->value("allow-local"));
    CommonSettingsServer::commonSettingsServer.chat_allow_private		= stringtobool(settings->value("allow-private"));
    //CommonSettingsServer::commonSettingsServer.chat_allow_aliance		= stringtobool(settings->value("allow-aliance"));
    CommonSettingsServer::commonSettingsServer.chat_allow_clan		= stringtobool(settings->value("allow-clan"));
    settings->endGroup();

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    settings->beginGroup("db-login");
    if(settings->value("type")=="mysql")
        formatedServerSettings.database_login.tryOpenType					= DatabaseBase::DatabaseType::Mysql;
    else if(settings->value("type")=="sqlite")
        formatedServerSettings.database_login.tryOpenType					= DatabaseBase::DatabaseType::SQLite;
    else if(settings->value("type")=="postgresql")
        formatedServerSettings.database_login.tryOpenType					= DatabaseBase::DatabaseType::PostgreSQL;
    else
        formatedServerSettings.database_login.tryOpenType					= DatabaseBase::DatabaseType::Mysql;
    switch(formatedServerSettings.database_login.tryOpenType)
    {
        default:
        case DatabaseBase::DatabaseType::PostgreSQL:
        case DatabaseBase::DatabaseType::Mysql:
            formatedServerSettings.database_login.host				= settings->value("host");
            formatedServerSettings.database_login.db				= settings->value("db");
            formatedServerSettings.database_login.login				= settings->value("login");
            formatedServerSettings.database_login.pass				= settings->value("pass");
        break;
        case DatabaseBase::DatabaseType::SQLite:
            formatedServerSettings.database_login.file				= settings->value("file");
        break;
    }
    formatedServerSettings.database_login.tryInterval       = stringtouint32(settings->value("tryInterval"));
    formatedServerSettings.database_login.considerDownAfterNumberOfTry = stringtouint32(settings->value("considerDownAfterNumberOfTry"));
    settings->endGroup();
    #endif

    settings->beginGroup("db-base");
    if(settings->value("type")=="mysql")
        formatedServerSettings.database_base.tryOpenType                   = DatabaseBase::DatabaseType::Mysql;
    else if(settings->value("type")=="sqlite")
        formatedServerSettings.database_base.tryOpenType                   = DatabaseBase::DatabaseType::SQLite;
    else if(settings->value("type")=="postgresql")
        formatedServerSettings.database_base.tryOpenType                   = DatabaseBase::DatabaseType::PostgreSQL;
    else
        formatedServerSettings.database_base.tryOpenType                   = DatabaseBase::DatabaseType::Mysql;
    switch(formatedServerSettings.database_base.tryOpenType)
    {
        default:
        case DatabaseBase::DatabaseType::PostgreSQL:
        case DatabaseBase::DatabaseType::Mysql:
            formatedServerSettings.database_base.host              = settings->value("host");
            formatedServerSettings.database_base.db                = settings->value("db");
            formatedServerSettings.database_base.login             = settings->value("login");
            formatedServerSettings.database_base.pass              = settings->value("pass");
        break;
        case DatabaseBase::DatabaseType::SQLite:
            formatedServerSettings.database_base.file              = settings->value("file");
        break;
    }
    formatedServerSettings.database_base.tryInterval       = stringtouint32(settings->value("tryInterval"));
    formatedServerSettings.database_base.considerDownAfterNumberOfTry = stringtouint32(settings->value("considerDownAfterNumberOfTry"));
    settings->endGroup();

    settings->beginGroup("db-common");
    if(settings->value("type")=="mysql")
        formatedServerSettings.database_common.tryOpenType					= DatabaseBase::DatabaseType::Mysql;
    else if(settings->value("type")=="sqlite")
        formatedServerSettings.database_common.tryOpenType					= DatabaseBase::DatabaseType::SQLite;
    else if(settings->value("type")=="postgresql")
        formatedServerSettings.database_common.tryOpenType					= DatabaseBase::DatabaseType::PostgreSQL;
    else
        formatedServerSettings.database_common.tryOpenType					= DatabaseBase::DatabaseType::Mysql;
    switch(formatedServerSettings.database_common.tryOpenType)
    {
        default:
        case DatabaseBase::DatabaseType::PostgreSQL:
        case DatabaseBase::DatabaseType::Mysql:
            formatedServerSettings.database_common.host				= settings->value("host");
            formatedServerSettings.database_common.db				= settings->value("db");
            formatedServerSettings.database_common.login				= settings->value("login");
            formatedServerSettings.database_common.pass				= settings->value("pass");
        break;
        case DatabaseBase::DatabaseType::SQLite:
            formatedServerSettings.database_common.file				= settings->value("file");
        break;
    }
    formatedServerSettings.database_common.tryInterval       = stringtouint32(settings->value("tryInterval"));
    formatedServerSettings.database_common.considerDownAfterNumberOfTry = stringtouint32(settings->value("considerDownAfterNumberOfTry"));
    settings->endGroup();

    settings->beginGroup("db-server");
    if(settings->value("type")=="mysql")
        formatedServerSettings.database_server.tryOpenType					= DatabaseBase::DatabaseType::Mysql;
    else if(settings->value("type")=="sqlite")
        formatedServerSettings.database_server.tryOpenType					= DatabaseBase::DatabaseType::SQLite;
    else if(settings->value("type")=="postgresql")
        formatedServerSettings.database_server.tryOpenType					= DatabaseBase::DatabaseType::PostgreSQL;
    else
        formatedServerSettings.database_server.tryOpenType					= DatabaseBase::DatabaseType::Mysql;
    switch(formatedServerSettings.database_server.tryOpenType)
    {
        default:
        case DatabaseBase::DatabaseType::PostgreSQL:
        case DatabaseBase::DatabaseType::Mysql:
            formatedServerSettings.database_server.host				= settings->value("host");
            formatedServerSettings.database_server.db				= settings->value("db");
            formatedServerSettings.database_server.login				= settings->value("login");
            formatedServerSettings.database_server.pass				= settings->value("pass");
        break;
        case DatabaseBase::DatabaseType::SQLite:
            formatedServerSettings.database_server.file				= settings->value("file");
        break;
    }
    formatedServerSettings.database_server.tryInterval       = stringtouint32(settings->value("tryInterval"));
    formatedServerSettings.database_server.considerDownAfterNumberOfTry = stringtouint32(settings->value("considerDownAfterNumberOfTry"));
    settings->endGroup();

    settings->beginGroup("db");
    if(settings->value("db_fight_sync")=="FightSync_AtEachTurn")
        formatedServerSettings.fightSync                       = CatchChallenger::GameServerSettings::FightSync_AtEachTurn;
    else if(settings->value("db_fight_sync")=="FightSync_AtTheDisconnexion")
        formatedServerSettings.fightSync                       = CatchChallenger::GameServerSettings::FightSync_AtTheDisconnexion;
    else
        formatedServerSettings.fightSync                       = CatchChallenger::GameServerSettings::FightSync_AtTheEndOfBattle;
    formatedServerSettings.positionTeleportSync=stringtobool(settings->value("positionTeleportSync"));
    formatedServerSettings.secondToPositionSync=stringtouint32(settings->value("secondToPositionSync"));
    settings->endGroup();

    //connection
    formatedServerSettings.automatic_account_creation   = stringtobool(settings->value("automatic_account_creation"));
    formatedServerSettings.max_players					= stringtouint32(settings->value("max-players"));

    //visibility algorithm
    settings->beginGroup("MapVisibilityAlgorithm");
    switch(stringtouint32(settings->value("MapVisibilityAlgorithm")))
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
        settings->beginGroup("MapVisibilityAlgorithm-Simple");
        formatedServerSettings.mapVisibility.simple.max				= stringtouint32(settings->value("Max"));
        formatedServerSettings.mapVisibility.simple.reshow			= stringtouint32(settings->value("Reshow"));
        formatedServerSettings.mapVisibility.simple.reemit          = stringtobool(settings->value("Reemit"));
        settings->endGroup();
    }
    else if(formatedServerSettings.mapVisibility.mapVisibilityAlgorithm==MapVisibilityAlgorithmSelection_WithBorder)
    {
        settings->beginGroup("MapVisibilityAlgorithm-WithBorder");
        formatedServerSettings.mapVisibility.withBorder.maxWithBorder	= stringtouint32(settings->value("MaxWithBorder"));
        formatedServerSettings.mapVisibility.withBorder.reshowWithBorder= stringtouint32(settings->value("ReshowWithBorder"));
        formatedServerSettings.mapVisibility.withBorder.max				= stringtouint32(settings->value("Max"));
        formatedServerSettings.mapVisibility.withBorder.reshow			= stringtouint32(settings->value("Reshow"));
        settings->endGroup();
    }

    {
        settings->beginGroup("programmedEvent");
            const std::vector<std::string> &tempListType=stringsplit(settings->value("types"),';');
            unsigned int indexType=0;
            while(indexType<tempListType.size())
            {
                const std::string &type=tempListType.at(indexType);
                settings->beginGroup(type);
                    const std::vector<std::string> &tempList=stringsplit(settings->value("group"),';');
                    unsigned int index=0;
                    while(index<tempList.size())
                    {
                        const std::string &groupName=tempList.at(index);
                        settings->beginGroup(groupName);
                        if(settings->contains("value") && settings->contains("cycle") && settings->contains("offset"))
                        {
                            GameServerSettings::ProgrammedEvent event;
                            event.value=settings->value("value");
                            bool ok;
                            event.cycle=stringtouint32(settings->value("cycle"),&ok);
                            if(!ok)
                                event.cycle=0;
                            event.offset=stringtouint32(settings->value("offset"),&ok);
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

    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    {
        bool ok;
        settings->beginGroup("master");
        master_host=settings->value("host");
        master_port=stringtouint16(settings->value("port"),&ok);
        if(master_port==0 || !ok)
        {
            std::cerr << "Master port not a number or 0:" << settings->value("port") << std::endl;
            abort();
        }
        master_tryInterval=stringtouint8(settings->value("tryInterval"),&ok);
        if(master_tryInterval==0 || !ok)
        {
            std::cerr << "tryInterval==0 (abort)" << std::endl;
            abort();
        }
        master_considerDownAfterNumberOfTry=stringtouint8(settings->value("considerDownAfterNumberOfTry"),&ok);
        if(master_considerDownAfterNumberOfTry==0 || !ok)
        {
            std::cerr << "considerDownAfterNumberOfTry==0 (abort)" << std::endl;
            abort();
        }
        settings->endGroup();
    }
    #endif

    settings->beginGroup("city");
    if(settings->value("capture_frequency")=="week")
        formatedServerSettings.city.capture.frenquency=City::Capture::Frequency_week;
    else
        formatedServerSettings.city.capture.frenquency=City::Capture::Frequency_month;

    if(settings->value("capture_day")=="monday")
        formatedServerSettings.city.capture.day=City::Capture::Monday;
    else if(settings->value("capture_day")=="tuesday")
        formatedServerSettings.city.capture.day=City::Capture::Tuesday;
    else if(settings->value("capture_day")=="wednesday")
        formatedServerSettings.city.capture.day=City::Capture::Wednesday;
    else if(settings->value("capture_day")=="thursday")
        formatedServerSettings.city.capture.day=City::Capture::Thursday;
    else if(settings->value("capture_day")=="friday")
        formatedServerSettings.city.capture.day=City::Capture::Friday;
    else if(settings->value("capture_day")=="saturday")
        formatedServerSettings.city.capture.day=City::Capture::Saturday;
    else if(settings->value("capture_day")=="sunday")
        formatedServerSettings.city.capture.day=City::Capture::Sunday;
    else
        formatedServerSettings.city.capture.day=City::Capture::Monday;
    formatedServerSettings.city.capture.hour=0;
    formatedServerSettings.city.capture.minute=0;
    const std::vector<std::string> &capture_time_string_list=stringsplit(settings->value("capture_time"),':');
    if(capture_time_string_list.size()==2)
    {
        bool ok;
        formatedServerSettings.city.capture.hour=stringtouint32(capture_time_string_list.front(),&ok);
        if(!ok)
            formatedServerSettings.city.capture.hour=0;
        else
        {
            formatedServerSettings.city.capture.minute=stringtouint32(capture_time_string_list.back(),&ok);
            if(!ok)
                formatedServerSettings.city.capture.minute=0;
        }
    }
    settings->endGroup();

    /*if(QCoreApplication::arguments().contains("--benchmark"))
        GlobalServerData::serverSettings.benchmark=true;*/

    server->setSettings(formatedServerSettings);
    server->setNormalSettings(formatedServerNormalSettings);
}

int main(int argc, char *argv[])
{
    NormalServerGlobal::displayInfo();
    if(argc<1)
    {
        std::cerr << "argc<1: wrong arg count" << std::endl;
        return EXIT_FAILURE;
    }
    CatchChallenger::FacilityLibGeneral::applicationDirPath=argv[0];

    bool datapack_loaded=false;

    if(!CatchChallenger::FacilityLibGeneral::isFile(FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/datapack/informations.xml"))
    {
        std::cerr << "No datapack found into: " << FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath) << "/datapack/" << std::endl;
        return EXIT_FAILURE;
    }

    settings=new TinyXMLSettings(FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/server-properties.xml");
    NormalServerGlobal::checkSettingsFile(settings,FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/datapack/");

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
    {
        const int &linkfd=LinkToMaster::tryConnect(
                    master_host.c_str(),
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
        LinkToMaster::linkToMaster=new LinkToMaster(linkfd);
        #endif
        LinkToMaster::linkToMaster->stat=LinkToMaster::Stat::Connected;
        LinkToMaster::linkToMaster->setSettings(settings);
        LinkToMaster::linkToMaster->readTheFirstSslHeader();
        LinkToMaster::linkToMaster->setConnexionSettings();
    }
    #endif

    bool tcpCork,tcpNodelay;
    {
        const GameServerSettings &formatedServerSettings=server->getSettings();
        const NormalServerSettings &formatedServerNormalSettings=server->getNormalSettings();
        tcpCork=CommonSettingsServer::commonSettingsServer.tcpCork;
        tcpNodelay=formatedServerNormalSettings.tcpNodelay;

        if(!formatedServerNormalSettings.proxy.empty())
        {
            std::cerr << "Proxy not supported: " << formatedServerNormalSettings.proxy << std::endl;
            return EXIT_FAILURE;
        }
        if(!formatedServerNormalSettings.proxy.empty())
        {
            std::cerr << "Proxy not supported" << std::endl;
            return EXIT_FAILURE;
        }
        #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        if(!CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer)
        {
            std::cerr << "plantOnlyVisibleByPlayer at false but server compiled with plantOnlyVisibleByPlayer at true, recompil to change this options" << std::endl;
            return EXIT_FAILURE;
        }
        #else
        if(CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer)
        {
            qDebug() << "plantOnlyVisibleByPlayer at true but server compiled with plantOnlyVisibleByPlayer at false, recompil to change this options";
            return EXIT_FAILURE;
        }
        #endif

        {
            #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
            if(
                    true
                    #ifdef CATCHCHALLENGER_DB_POSTGRESQL
                    && formatedServerSettings.database_login.tryOpenType!=DatabaseBase::DatabaseType::PostgreSQL
                    #endif
                    #ifdef CATCHCHALLENGER_DB_MYSQL
                    && formatedServerSettings.database_login.tryOpenType!=DatabaseBase::DatabaseType::Mysql
                    #endif
                    )
            {
                settings->beginGroup("db-login");
                std::cerr << "Database type not supported for now: " << settings->value("type") << std::endl;
                settings->endGroup();
                return EXIT_FAILURE;
            }
            #endif
            if(
                    true
                    #ifdef CATCHCHALLENGER_DB_POSTGRESQL
                    && formatedServerSettings.database_base.tryOpenType!=DatabaseBase::DatabaseType::PostgreSQL
                    #endif
                    #ifdef CATCHCHALLENGER_DB_MYSQL
                    && formatedServerSettings.database_base.tryOpenType!=DatabaseBase::DatabaseType::Mysql
                    #endif
                    )
            {
                settings->beginGroup("db-base");
                std::cerr << "Database type not supported for now: " << settings->value("type") << std::endl;
                settings->endGroup();
                return EXIT_FAILURE;
            }
            if(
                    true
                    #ifdef CATCHCHALLENGER_DB_POSTGRESQL
                    && formatedServerSettings.database_common.tryOpenType!=DatabaseBase::DatabaseType::PostgreSQL
                    #endif
                    #ifdef CATCHCHALLENGER_DB_MYSQL
                    && formatedServerSettings.database_common.tryOpenType!=DatabaseBase::DatabaseType::Mysql
                    #endif
                    )
            {
                settings->beginGroup("db-common");
                std::cerr << "Database type not supported for now: " << settings->value("type") << std::endl;
                settings->endGroup();
                return EXIT_FAILURE;
            }
            if(
                    true
                    #ifdef CATCHCHALLENGER_DB_POSTGRESQL
                    && formatedServerSettings.database_server.tryOpenType!=DatabaseBase::DatabaseType::PostgreSQL
                    #endif
                    #ifdef CATCHCHALLENGER_DB_MYSQL
                    && formatedServerSettings.database_server.tryOpenType!=DatabaseBase::DatabaseType::Mysql
                    #endif
                    )
            {
                settings->beginGroup("db-server");
                std::cerr << "Database type not supported for now: " << settings->value("type") << std::endl;
                settings->endGroup();
                return EXIT_FAILURE;
            }
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
            std::cerr << "Clear connexion requested but server compiled with ssl support!" << std::endl;
            return EXIT_FAILURE;
        }
        #endif
        if(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.empty())
        {
            #ifdef CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION
            qDebug() << "Need mirror because CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION is def, need decompression to datapack list input";
            return EXIT_FAILURE;
            #endif
        }
        else
        {
            std::vector<std::string> newMirrorList;
            std::regex httpMatch("^https?://.+$");
            const std::vector<std::string> &mirrorList=stringsplit(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer,';');
            unsigned int index=0;
            while(index<mirrorList.size())
            {
                const std::string &mirror=mirrorList.at(index);
                if(!regex_search(mirror,httpMatch))
                {
                    std::cerr << "Mirror wrong: " << mirror << std::endl;
                    return EXIT_FAILURE;
                }
                if(stringEndsWith(mirror,'/'))
                    newMirrorList.push_back(mirror);
                else
                    newMirrorList.push_back(mirror+'/');
                index++;
            }
            CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer=stringimplode(newMirrorList,';');
            CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase=CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer;
        }
    }
    server->initTheDatabase();
    server->loadAndFixSettings();

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    if(!GlobalServerData::serverPrivateVariables.db_login->syncConnect(
                GlobalServerData::serverSettings.database_login.host,
                GlobalServerData::serverSettings.database_login.db,
                GlobalServerData::serverSettings.database_login.login,
                GlobalServerData::serverSettings.database_login.pass))
    {
        std::cerr << "Unable to connect to database login:" << GlobalServerData::serverPrivateVariables.db_login->errorMessage() << std::endl;
        return EXIT_FAILURE;
    }
    #endif
    if(!GlobalServerData::serverPrivateVariables.db_base->syncConnect(
                GlobalServerData::serverSettings.database_base.host,
                GlobalServerData::serverSettings.database_base.db,
                GlobalServerData::serverSettings.database_base.login,
                GlobalServerData::serverSettings.database_base.pass))
    {
        std::cerr << "Unable to connect to database base:" << GlobalServerData::serverPrivateVariables.db_base->errorMessage() << std::endl;
        return EXIT_FAILURE;
    }
    if(!GlobalServerData::serverPrivateVariables.db_common->syncConnect(
                GlobalServerData::serverSettings.database_common.host,
                GlobalServerData::serverSettings.database_common.db,
                GlobalServerData::serverSettings.database_common.login,
                GlobalServerData::serverSettings.database_common.pass))
    {
        std::cerr << "Unable to connect to database common:" << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        return EXIT_FAILURE;
    }
    if(!GlobalServerData::serverPrivateVariables.db_server->syncConnect(
                GlobalServerData::serverSettings.database_server.host,
                GlobalServerData::serverSettings.database_server.db,
                GlobalServerData::serverSettings.database_server.login,
                GlobalServerData::serverSettings.database_server.pass))
    {
        std::cerr << "Unable to connect to database server:" << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
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
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    TimerPurgeTokenAuthList timerPurgeTokenAuthList;
    #endif
    {
        GlobalServerData::serverPrivateVariables.time_city_capture=FacilityLib::nextCaptureTime(GlobalServerData::serverSettings.city);
        const int64_t &time=GlobalServerData::serverPrivateVariables.time_city_capture-std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
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
            #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
            if(!GlobalServerData::serverPrivateVariables.player_updater_to_master.start())
            {
                std::cerr << "player_updater_to_master timer fail to set" << std::endl;
                return EXIT_FAILURE;
            }
            #endif
        }
    }
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    {
        if(!timerPurgeTokenAuthList.start(15*60*1000))
        {
            std::cerr << "timerPurgeTokenAuthList fail to set" << std::endl;
            return EXIT_FAILURE;
        }
    }
    #endif

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
                case BaseClassSwitch::EpollObjectType::Server:
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
                            {
                                if(::write(infd,encodingBuff,sizeof(encodingBuff))!=sizeof(encodingBuff))
                                {
                                    std::cerr << "socket first byte write error" << std::endl;
                                    delete client;
                                }
                            }

                        }
                    }
                    continue;
                }
                break;
                case BaseClassSwitch::EpollObjectType::UnixServer:
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
                case BaseClassSwitch::EpollObjectType::Client:
                {
                    #ifdef SERVERBENCHMARKFULL
                    start_inter = std::chrono::high_resolution_clock::now();
                    #endif
                    #ifdef SERVERBENCHMARKFULL
                    timerDisplayEventBySeconds.addClientCount();
                    #endif
                    Client * const client=static_cast<Client *>(events[i].data.ptr);
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
                case BaseClassSwitch::EpollObjectType::UnixClient:
                {
                    #ifdef SERVERBENCHMARKFULL
                    timerDisplayEventBySeconds.addClientCount();
                    #endif
                    EpollUnixSocketClientFinal * const client=static_cast<EpollUnixSocketClientFinal *>(events[i].data.ptr);
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
                case BaseClassSwitch::EpollObjectType::Timer:
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
                case BaseClassSwitch::EpollObjectType::Database:
                {
                    #ifdef SERVERBENCHMARKFULL
                    start_inter = std::chrono::high_resolution_clock::now();
                    #endif
                    #ifdef SERVERBENCHMARKFULL
                    timerDisplayEventBySeconds.addDbCount();
                    #endif
                    switch(static_cast<CatchChallenger::DatabaseBase *>(events[i].data.ptr)->databaseType())
                    {
                        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
                        case DatabaseBase::DatabaseType::PostgreSQL:
                        {
                            EpollPostgresql * const db=static_cast<EpollPostgresql *>(events[i].data.ptr);
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
                        #endif
                        #ifdef CATCHCHALLENGER_DB_MYSQL
                        case DatabaseBase::DatabaseType::Mysql:
                        {
                            EpollMysql * const db=static_cast<EpollMysql *>(events[i].data.ptr);
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
                        #endif
                        default:
                        std::cerr << "epoll database type return not supported" << std::endl;
                        abort();
                    }
                }
                break;
                #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                case BaseClassSwitch::EpollObjectType::MasterLink:
                {
                    LinkToMaster * const client=static_cast<LinkToMaster *>(events[i].data.ptr);
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occured on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        if(!(events[i].events & EPOLLHUP))
                            std::cerr << "client epoll error: " << events[i].events << std::endl;
                        client->tryReconnect();
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
                        client->tryReconnect();
                }
                break;
                #endif
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
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    LinkToMaster::linkToMaster->closeSocket();
    #endif
    delete server;
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    delete LinkToMaster::linkToMaster;
    #endif
    return EXIT_SUCCESS;
}
