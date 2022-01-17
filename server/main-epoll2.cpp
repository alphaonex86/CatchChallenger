#include <string>
#include "base/ServerStructures.hpp"
#include "base/TinyXMLSettings.hpp"
#include "base/GlobalServerData.hpp"
#include "../general/base/CommonSettingsCommon.hpp"
#include "epoll/EpollServer.hpp"
#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "game-server-alone/LinkToMaster.hpp"
#endif

using namespace CatchChallenger;

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
void generateTokenStatClient(TinyXMLSettings &settings,char * const data);
#endif

void send_settings(
    #ifdef SERVERSSL
    EpollSslServer *server
    #else
    EpollServer *server
    #endif
, TinyXMLSettings *settings,
        std::string &master_host,
        uint16_t &master_port,
        uint8_t &master_tryInterval,
        uint8_t &master_considerDownAfterNumberOfTry
        )
{
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    (void)master_host;
    (void)master_port;
    (void)master_tryInterval;
    (void)master_considerDownAfterNumberOfTry;
    #endif
    bool ok;
    GameServerSettings formatedServerSettings=server->getSettings();
    NormalServerSettings formatedServerNormalSettings=server->getNormalSettings();

    //common var
    CommonSettingsServer::commonSettingsServer.useSP                            = stringtobool(settings->value("useSP"));
    CommonSettingsServer::commonSettingsServer.autoLearn                        = stringtobool(settings->value("autoLearn")) && !CommonSettingsServer::commonSettingsServer.useSP;
    CommonSettingsServer::commonSettingsServer.forcedSpeed                      = stringtouint8(settings->value("forcedSpeed"));
    CommonSettingsServer::commonSettingsServer.dontSendPseudo					= stringtobool(settings->value("dontSendPseudo"));
    CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange		= stringtobool(settings->value("forceClientToSendAtMapChange"));
    formatedServerSettings.dontSendPlayerType                                   = stringtobool(settings->value("dontSendPlayerType"));
    formatedServerSettings.everyBodyIsRoot                                      = stringtobool(settings->value("everyBodyIsRoot"));
    formatedServerSettings.teleportIfMapNotFoundOrOutOfMap                       = stringtobool(settings->value("teleportIfMapNotFoundOrOutOfMap"));
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    CommonSettingsCommon::commonSettingsCommon.min_character					= stringtouint8(settings->value("min_character"));
    CommonSettingsCommon::commonSettingsCommon.max_character					= stringtouint8(settings->value("max_character"));
    CommonSettingsCommon::commonSettingsCommon.max_pseudo_size					= stringtouint8(settings->value("max_pseudo_size"));
    CommonSettingsCommon::commonSettingsCommon.character_delete_time			= stringtouint32(settings->value("character_delete_time"));
    CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters                = stringtouint8(settings->value("maxPlayerMonsters"));
    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters       = stringtouint16(settings->value("maxWarehousePlayerMonsters"));
    CommonSettingsCommon::commonSettingsCommon.maxPlayerItems                   = stringtouint8(settings->value("maxPlayerItems"));
    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems          = stringtouint16(settings->value("maxWarehousePlayerItems"));
    //connection
    formatedServerSettings.automatic_account_creation   = stringtobool(settings->value("automatic_account_creation"));
    #endif

    if(!settings->contains("compressionLevel"))
        settings->setValue("compressionLevel","6");
    CompressionProtocol::compressionLevel          = stringtouint8(settings->value("compressionLevel"),&ok);
    if(!ok)
    {
        std::cerr << "Compression level not a number fixed by 6" << std::endl;
        CompressionProtocol::compressionLevel=6;
    }
    CompressionProtocol::compressionLevel                                     = stringtouint8(settings->value("compressionLevel"));
    if(settings->value("compression")=="none")
        formatedServerSettings.compressionType                                = CompressionProtocol::CompressionType::None;
    else if(settings->value("compression")=="zstd")
        formatedServerSettings.compressionType                                = CompressionProtocol::CompressionType::Zstandard;
    else
    {
        std::cerr << "compression not supported: " << settings->value("compression") << std::endl;
        formatedServerSettings.compressionType                                = CompressionProtocol::CompressionType::Zstandard;
    }

    //the listen
    formatedServerNormalSettings.server_port			= stringtouint16(settings->value("server-port"));
    formatedServerNormalSettings.server_ip				= settings->value("server-ip");
    formatedServerNormalSettings.proxy					= settings->value("proxy");
    formatedServerNormalSettings.proxy_port				= stringtouint16(settings->value("proxy_port"));
    formatedServerNormalSettings.useSsl					= stringtobool(settings->value("useSsl"));
    formatedServerSettings.common_blobversion_datapack= stringtouint8(settings->value("common_blobversion_datapack"),&ok);
    if(!ok)
    {
        std::cerr << "common_blobversion_datapack is not a number" << std::endl;
        abort();
    }
    if(formatedServerSettings.common_blobversion_datapack>15)
    {
        std::cerr << "common_blobversion_datapack > 15" << std::endl;
        abort();
    }
    formatedServerSettings.server_blobversion_datapack= stringtouint8(settings->value("server_blobversion_datapack"),&ok);
    if(!ok)
    {
        std::cerr << "server_blobversion_datapack is not a number" << std::endl;
        abort();
    }
    if(formatedServerSettings.server_blobversion_datapack>15)
    {
        std::cerr << "server_blobversion_datapack > 15" << std::endl;
        abort();
    }

    settings->beginGroup("content");
        if(settings->contains("mainDatapackCode"))
            CommonSettingsServer::commonSettingsServer.mainDatapackCode=settings->value("mainDatapackCode","[main]");
        else
            CommonSettingsServer::commonSettingsServer.mainDatapackCode="[main]";
        if(CommonSettingsServer::commonSettingsServer.mainDatapackCode=="[main]")
        {
            const std::vector<CatchChallenger::FacilityLibGeneral::InodeDescriptor> &list=
                    CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(GlobalServerData::serverSettings.datapack_basePath+"/map/main/",CatchChallenger::FacilityLibGeneral::ListFolder::Dirs);
            if(list.empty())
            {
                std::cerr << "No main code detected into the current datapack (abort)" << std::endl;
                settings->sync();
                abort();
            }
            if(list.size()>=1)
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
        formatedServerSettings.server_message				= settings->value("server_message");
        CommonSettingsServer::commonSettingsServer.exportedXml                      = settings->value("exportedXml");
        formatedServerSettings.daillygift                      = settings->value("daillygift");
    settings->endGroup();

    formatedServerSettings.anonymous					= stringtobool(settings->value("anonymous"));
    CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase	= settings->value("httpDatapackMirror");
    CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer=CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase;
    formatedServerSettings.datapackCache				= stringtoint32(settings->value("datapackCache"));

    //fight
    //CommonSettingsCommon::commonSettingsCommon.pvp			= stringtobool(settings->value("pvp"));
    formatedServerSettings.sendPlayerNumber         = stringtobool(settings->value("sendPlayerNumber"));

    //rates
    settings->beginGroup("rates");
    CommonSettingsServer::commonSettingsServer.rates_xp             = stringtofloat(settings->value("xp_normal"))*1000;
    CommonSettingsServer::commonSettingsServer.rates_gold			= stringtofloat(settings->value("gold_normal"))*1000;
    CommonSettingsServer::commonSettingsServer.rates_xp_pow			= stringtofloat(settings->value("xp_pow_normal"))*1000;
    CommonSettingsServer::commonSettingsServer.rates_drop			= stringtofloat(settings->value("drop_normal"))*1000;
    //formatedServerSettings.rates_xp_premium                         = stringtodouble(settings->value("xp_premium"));
    //formatedServerSettings.rates_gold_premium                       = stringtodouble(settings->value("gold_premium"));
    /*CommonSettingsCommon::commonSettingsCommon.rates_shiny		= stringtodouble(settings->value("shiny_normal"));
    formatedServerSettings.rates_shiny_premium                      = stringtodouble(settings->value("shiny_premium"));*/
    settings->endGroup();

    settings->beginGroup("DDOS");
    CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick         = stringtouint32(settings->value("waitBeforeConnectAfterKick"));
    formatedServerSettings.ddos.computeAverageValueTimeInterval       = stringtouint8(settings->value("computeAverageValueTimeInterval"));
    #ifdef CATCHCHALLENGER_DDOS_FILTER
    formatedServerSettings.ddos.kickLimitMove                         = stringtouint8(settings->value("kickLimitMove"));
    formatedServerSettings.ddos.kickLimitChat                         = stringtouint8(settings->value("kickLimitChat"));
    formatedServerSettings.ddos.kickLimitOther                        = stringtouint8(settings->value("kickLimitOther"));
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
    #endif

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
    formatedServerSettings.max_players					= stringtouint16(settings->value("max-players"));

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
        formatedServerSettings.mapVisibility.simple.max				= stringtouint16(settings->value("Max"));
        formatedServerSettings.mapVisibility.simple.reshow			= stringtouint16(settings->value("Reshow"));
        formatedServerSettings.mapVisibility.simple.reemit          = stringtobool(settings->value("Reemit"));
        settings->endGroup();
    }
    else if(formatedServerSettings.mapVisibility.mapVisibilityAlgorithm==MapVisibilityAlgorithmSelection_WithBorder)
    {
        settings->beginGroup("MapVisibilityAlgorithm-WithBorder");
        formatedServerSettings.mapVisibility.withBorder.maxWithBorder	= stringtouint16(settings->value("MaxWithBorder"));
        formatedServerSettings.mapVisibility.withBorder.reshowWithBorder= stringtouint16(settings->value("ReshowWithBorder"));
        formatedServerSettings.mapVisibility.withBorder.max				= stringtouint16(settings->value("Max"));
        formatedServerSettings.mapVisibility.withBorder.reshow			= stringtouint16(settings->value("Reshow"));
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
                            event.cycle=stringtouint16(settings->value("cycle"),&ok);
                            if(!ok)
                                event.cycle=0;
                            event.offset=stringtouint16(settings->value("offset"),&ok);
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
        if(master_tryInterval<=0 || !ok)
        {
            std::cerr << "master_tryInterval<=0 || !ok (abort)" << std::endl;
            abort();
        }
        if(master_tryInterval>=60)
            std::cerr << "Take care: master_tryInterval>=60" << std::endl;
        master_considerDownAfterNumberOfTry=stringtouint8(settings->value("considerDownAfterNumberOfTry"),&ok);
        if(master_considerDownAfterNumberOfTry<=0 || !ok)
        {
            std::cerr << "considerDownAfterNumberOfTry==0 || !ok (abort) " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        if(master_considerDownAfterNumberOfTry>=60)
            std::cerr << "Take care: master_considerDownAfterNumberOfTry>=60 " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
        LinkToMaster::maxLockAge=stringtouint16(settings->value("maxLockAge"),&ok);
        if(LinkToMaster::maxLockAge<1 || LinkToMaster::maxLockAge>3600 || !ok)
        {
            std::cerr << "maxLockAge<1 || maxLockAge>3600 || not number (abort)" << std::endl;
            abort();
        }
        LinkToMaster::purgeLockPeriod=stringtouint16(settings->value("purgeLockPeriod"),&ok);
        if(LinkToMaster::purgeLockPeriod<1 || LinkToMaster::purgeLockPeriod>3600 || LinkToMaster::purgeLockPeriod>LinkToMaster::maxLockAge || !ok)
        {
            std::cerr << "purgeLockPeriod<1 || purgeLockPeriod>3600 || purgeLockPeriod>maxLockAge || not number (abort)" << std::endl;
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
        formatedServerSettings.city.capture.hour=stringtouint8(capture_time_string_list.front(),&ok);
        if(!ok)
            formatedServerSettings.city.capture.hour=0;
        else
        {
            formatedServerSettings.city.capture.minute=stringtouint8(capture_time_string_list.back(),&ok);
            if(!ok)
                formatedServerSettings.city.capture.minute=0;
        }
    }
    settings->endGroup();

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    settings->beginGroup("statclient");
    {
        if(!settings->contains("token"))
            generateTokenStatClient(*settings,formatedServerSettings.private_token_statclient);
        std::string token=settings->value("token");
        if(token.size()!=TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT*2/*String Hexa, not binary*/)
            generateTokenStatClient(*settings,formatedServerSettings.private_token_statclient);
        token=settings->value("token");
        const std::vector<char> &tokenBinary=hexatoBinary(token);
        if(tokenBinary.empty())
        {
            std::cerr << "convertion to binary for pass failed for: " << token << std::endl;
            abort();
        }
        memcpy(formatedServerSettings.private_token_statclient,tokenBinary.data(),TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
    }
    settings->endGroup();
    #endif

    /*if(QCoreApplication::arguments().contains("--benchmark"))
        GlobalServerData::serverSettings.benchmark=true;*/

    server->setSettings(formatedServerSettings);
    server->setNormalSettings(formatedServerNormalSettings);
    #ifdef CATCHCHALLENGER_CACHE_HPS
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    server->setMaster(master_host,master_port,master_tryInterval,master_considerDownAfterNumberOfTry);
    #endif
    #endif
}
