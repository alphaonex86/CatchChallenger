#include "ProcessControler.h"
#include "base/ServerStructures.h"
#include "../general/base/CommonSettingsCommon.h"

using namespace CatchChallenger;

ProcessControler::ProcessControler()
{
    qRegisterMetaType<Chat_type>("Chat_type");
    qRegisterMetaType<std::string>("std::string");
    connect(&server,&CatchChallenger::NormalServer::is_started,this,&ProcessControler::server_is_started);
    connect(&server,&CatchChallenger::NormalServer::need_be_stopped,this,&ProcessControler::server_need_be_stopped);
    connect(&server,&CatchChallenger::NormalServer::need_be_restarted,this,&ProcessControler::server_need_be_restarted);
    connect(&server,&CatchChallenger::NormalServer::error,this,&ProcessControler::error);
    connect(&server,&CatchChallenger::NormalServer::haveQuitForCriticalDatabaseQueryFailed,               this,&ProcessControler::haveQuitForCriticalDatabaseQueryFailed);
    need_be_restarted=false;
    need_be_closed=false;

    settings=new TinyXMLSettings((QCoreApplication::applicationDirPath()+QStringLiteral("/server-properties.xml")).toStdString());
    NormalServer::checkSettingsFile(settings,FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/datapack/");
    send_settings();
    server.start_server();
}

ProcessControler::~ProcessControler()
{
    delete settings;
}

void ProcessControler::generateTokenStatClient(TinyXMLSettings &settings,char * const data)
{
    FILE *fpRandomFile = fopen(RANDOMFILEDEVICE,"rb");
    if(fpRandomFile==NULL)
    {
        std::cerr << "Unable to open " << RANDOMFILEDEVICE << " to generate random token, use unsafe rand()" << std::endl;
        unsigned int index=0;
        while(index<TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)
        {
            data[index]=rand();
            index++;
        }
        return;
    }
    const int &returnedSize=fread(data,1,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT,fpRandomFile);
    if(returnedSize!=TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)
    {
        std::cerr << "Unable to read the " << TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT << " needed to do the token from, use unsafe rand()" << RANDOMFILEDEVICE << std::endl;
        unsigned int index=0;
        while(index<TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)
        {
            data[index]=rand();
            index++;
        }
        return;
    }
    settings.setValue("token",binarytoHexa(reinterpret_cast<char *>(data)
                                           ,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT).c_str());
    fclose(fpRandomFile);
    settings.sync();
}

void ProcessControler::send_settings()
{
    bool ok;
    CatchChallenger::GameServerSettings formatedServerSettings=server.getSettings();
    NormalServerSettings formatedServerNormalSettings=server.getNormalSettings();

    //common var
    CommonSettingsServer::commonSettingsServer.useSP                            = stringtobool(settings->value("useSP"));
    CommonSettingsServer::commonSettingsServer.autoLearn                        = stringtobool(settings->value("autoLearn")) && !CommonSettingsServer::commonSettingsServer.useSP;
    CommonSettingsServer::commonSettingsServer.forcedSpeed                      = stringtouint32(settings->value("forcedSpeed"));
    CommonSettingsServer::commonSettingsServer.dontSendPseudo					= stringtobool(settings->value("dontSendPseudo"));
    CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer  		= stringtobool(settings->value("plantOnlyVisibleByPlayer"));
    CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange		= stringtobool(settings->value("forceClientToSendAtMapChange"));
    formatedServerSettings.dontSendPlayerType                                   = stringtobool(settings->value("dontSendPlayerType"));
    CommonSettingsCommon::commonSettingsCommon.min_character					= stringtouint8(settings->value("min_character"));
    CommonSettingsCommon::commonSettingsCommon.max_character					= stringtouint8(settings->value("max_character"));
    CommonSettingsCommon::commonSettingsCommon.max_pseudo_size					= stringtouint8(settings->value("max_pseudo_size"));
    CommonSettingsCommon::commonSettingsCommon.character_delete_time			= stringtouint32(settings->value("character_delete_time"));
    CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters                = stringtouint32(settings->value("maxPlayerMonsters"));
    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters       = stringtouint32(settings->value("maxWarehousePlayerMonsters"));
    CommonSettingsCommon::commonSettingsCommon.maxPlayerItems                   = stringtouint32(settings->value("maxPlayerItems"));
    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems          = stringtouint32(settings->value("maxWarehousePlayerItems"));
    formatedServerSettings.everyBodyIsRoot                                      = stringtobool(settings->value("everyBodyIsRoot"));
    formatedServerSettings.teleportIfMapNotFoundOrOutOfMap                       = stringtobool(settings->value("teleportIfMapNotFoundOrOutOfMap"));
    //connection
    formatedServerSettings.automatic_account_creation   = stringtobool(settings->value("automatic_account_creation"));

    if(!settings->contains("compressionLevel"))
        settings->setValue("compressionLevel","6");
    formatedServerSettings.compressionLevel          = stringtouint8(settings->value("compressionLevel"),&ok);
    if(!ok)
    {
        std::cerr << "Compression level not a number fixed by 6" << std::endl;
        formatedServerSettings.compressionLevel=6;
    }
    formatedServerSettings.compressionLevel                                     = stringtouint32(settings->value("compressionLevel"));
    if(settings->value("compression")=="none")
        formatedServerSettings.compressionType                                = CompressionType_None;
    else if(settings->value("compression")=="zstd")
        formatedServerSettings.compressionType                                = CompressionType_Zstandard;
    else
    {
        std::cerr << "Wrong compression: " << settings->value("compression") << std::endl;
        formatedServerSettings.compressionType                                = CompressionType_Zstandard;
    }

    //the listen
    formatedServerNormalSettings.server_port			= stringtouint32(settings->value("server-port"));
    formatedServerNormalSettings.server_ip				= settings->value("server-ip");
    formatedServerNormalSettings.proxy					= settings->value("proxy");
    formatedServerNormalSettings.proxy_port				= stringtouint32(settings->value("proxy_port"));
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
            const std::vector<CatchChallenger::FacilityLibGeneral::InodeDescriptor> &list=CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(GlobalServerData::serverSettings.datapack_basePath+"/map/main/",CatchChallenger::FacilityLibGeneral::ListFolder::Dirs);
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
    #ifdef __linux__
    settings->beginGroup("Linux");
    CommonSettingsServer::commonSettingsServer.tcpCork	= stringtobool(settings->value("tcpCork"));
    formatedServerNormalSettings.tcpNodelay= stringtobool(settings->value("tcpNodelay"));
    settings->endGroup();
    #endif

    //fight
    //CommonSettingsServer::commonSettingsServer.pvp			= stringtobool(settings->value("pvp"));
    formatedServerSettings.sendPlayerNumber         = stringtobool(settings->value("sendPlayerNumber"));

    //rates
    settings->beginGroup("rates");
    CommonSettingsServer::commonSettingsServer.rates_xp             = stringtodouble(settings->value("xp_normal"))*1000;
    CommonSettingsServer::commonSettingsServer.rates_gold			= stringtodouble(settings->value("gold_normal"))*1000;
    CommonSettingsServer::commonSettingsServer.rates_xp_pow			= stringtodouble(settings->value("xp_pow_normal"))*1000;
    CommonSettingsServer::commonSettingsServer.rates_drop			= stringtodouble(settings->value("drop_normal"))*1000;
    //formatedServerSettings.rates_xp_premium                         = stringtodouble(settings->value("xp_premium"));
    //formatedServerSettings.rates_gold_premium                       = stringtodouble(settings->value("gold_premium"));
    /*CommonSettingsCommon::commonSettingsCommon.rates_shiny		= stringtodouble(settings->value("shiny_normal"));
    formatedServerSettings.rates_shiny_premium                      = stringtodouble(settings->value("shiny_premium"));*/
    settings->endGroup();

    settings->beginGroup("DDOS");
    CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick         = stringtouint32(settings->value("waitBeforeConnectAfterKick"));
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

void ProcessControler::error(const std::string &error)
{
    std::cerr << error << std::endl;
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

std::string ProcessControler::sizeToString(double size)
{
    if(size<1024)
        return std::to_string(size)+std::string("B");
    if((size=size/1024)<1024)
        return adaptString(size)+std::string("KB");
    if((size=size/1024)<1024)
        return adaptString(size)+std::string("MB");
    if((size=size/1024)<1024)
        return adaptString(size)+std::string("GB");
    if((size=size/1024)<1024)
        return adaptString(size)+std::string("TB");
    if((size=size/1024)<1024)
        return adaptString(size)+std::string("PB");
    if((size=size/1024)<1024)
        return adaptString(size)+std::string("EB");
    if((size=size/1024)<1024)
        return adaptString(size)+std::string("ZB");
    if((size=size/1024)<1024)
        return adaptString(size)+std::string("YB");
    return std::string("Too big");
}

std::string ProcessControler::adaptString(float size)
{
    if(size>=100)
        return std::to_string(size);
    else
        return std::to_string(size);
}

void ProcessControler::haveQuitForCriticalDatabaseQueryFailed()
{
    qDebug() << "Unable to do critical database query to initialise the server";
    QCoreApplication::quit();
}
