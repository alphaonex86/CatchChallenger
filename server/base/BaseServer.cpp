#include "BaseServer.hpp"
#include "GlobalServerData.hpp"
#include "DictionaryLogin.hpp"
#include "ClientMapManagement/Map_server_MapVisibility_Simple_StoreOnSender.hpp"
#include "ClientMapManagement/Map_server_MapVisibility_WithBorder_StoreOnSender.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "../../general/base/Version.hpp"
#include "../../general/hps/hps.h"
#include <fstream>

using namespace CatchChallenger;

//cleab this, it's done before the main epoll loop
bool BaseServer::load_next_city_capture()
{
    /*GlobalServerData::serverPrivateVariables.time_city_capture=FacilityLib::nextCaptureTime(GlobalServerData::serverSettings.city);
    std::time_t result = std::time(nullptr);
    const int64_t &time=GlobalServerData::serverPrivateVariables.time_city_capture-result;
    GlobalServerData::serverPrivateVariables.timer_city_capture->start(static_cast<int>(time));*/
    return true;
}

void BaseServer::parseJustLoadedMap(const Map_to_send &,const std::string &)
{
}

bool BaseServer::initialize_the_database()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    #ifdef CATCHCHALLENGER_SOLO
    if(GlobalServerData::serverPrivateVariables.db_server!=NULL && GlobalServerData::serverPrivateVariables.db_server->isConnected())
        if(GlobalServerData::serverPrivateVariables.db_server->databaseType()!=DatabaseBase::DatabaseType::SQLite)
        {
            std::cerr << "Disconnected to incorrect database type for solo: " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_server->databaseType()) << std::endl;
            abort();
        }
    if(GlobalServerData::serverPrivateVariables.db_common!=NULL && GlobalServerData::serverPrivateVariables.db_common->isConnected())
        if(GlobalServerData::serverPrivateVariables.db_common->databaseType()!=DatabaseBase::DatabaseType::SQLite)
        {
            std::cerr << "Disconnected to incorrect database type for solo: " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_common->databaseType()) << std::endl;
            abort();
        }
    if(GlobalServerData::serverPrivateVariables.db_login!=NULL && GlobalServerData::serverPrivateVariables.db_login->isConnected())
        if(GlobalServerData::serverPrivateVariables.db_login->databaseType()!=DatabaseBase::DatabaseType::SQLite)
        {
            std::cerr << "Disconnected to incorrect database type for solo: " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_login->databaseType()) << std::endl;
            abort();
        }
    if(GlobalServerData::serverPrivateVariables.db_base!=NULL && GlobalServerData::serverPrivateVariables.db_base->isConnected())
        if(GlobalServerData::serverPrivateVariables.db_base->databaseType()!=DatabaseBase::DatabaseType::SQLite)
        {
            std::cerr << "Disconnected to incorrect database type for solo: " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_base->databaseType()) << std::endl;
            abort();
        }
    #endif
    #endif
    if(GlobalServerData::serverPrivateVariables.db_server->isConnected())
    {
        std::cout << "Disconnected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_server->databaseType())
                  << " at " << GlobalServerData::serverSettings.database_server.host << std::endl;
        GlobalServerData::serverPrivateVariables.db_server->syncDisconnect();
    }
    if(GlobalServerData::serverPrivateVariables.db_common->isConnected())
    {
        std::cout << "Disconnected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_common->databaseType())
                  << " at " << GlobalServerData::serverSettings.database_common.host << std::endl;
        GlobalServerData::serverPrivateVariables.db_common->syncDisconnect();
    }
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    if(GlobalServerData::serverPrivateVariables.db_login->isConnected())
    {
        std::cout << "Disconnected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_login->databaseType())
                  << " at " << GlobalServerData::serverSettings.database_login.host << std::endl;
        GlobalServerData::serverPrivateVariables.db_login->syncDisconnect();
    }
    if(GlobalServerData::serverPrivateVariables.db_base->isConnected())
    {
        std::cout << "Disconnected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_base->databaseType())
                  << " at " << GlobalServerData::serverSettings.database_base.host << std::endl;
        GlobalServerData::serverPrivateVariables.db_base->syncDisconnect();
    }
    #endif


    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    switch(GlobalServerData::serverSettings.database_login.tryOpenType)
    {
        default:
        std::cerr << "database type unknown" << std::endl;
        return false;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::Mysql:
        if(!GlobalServerData::serverPrivateVariables.db_login->syncConnectMysql(
                    GlobalServerData::serverSettings.database_login.host.c_str(),
                    GlobalServerData::serverSettings.database_login.db.c_str(),
                    GlobalServerData::serverSettings.database_login.login.c_str(),
                    GlobalServerData::serverSettings.database_login.pass.c_str()
                    ))
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_login->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_login->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_login.host
                      << " (" << GlobalServerData::serverSettings.database_login.db << ")"
                      << std::endl;
        break;

        case DatabaseBase::DatabaseType::SQLite:
        if(!GlobalServerData::serverPrivateVariables.db_login->syncConnectSqlite(GlobalServerData::serverSettings.database_login.file.c_str()))
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_login->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_login->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_login.file << std::endl;
        break;
        #endif

        case DatabaseBase::DatabaseType::PostgreSQL:
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(!GlobalServerData::serverPrivateVariables.db_login->syncConnectPostgresql(
                    GlobalServerData::serverSettings.database_login.host.c_str(),
                    GlobalServerData::serverSettings.database_login.db.c_str(),
                    GlobalServerData::serverSettings.database_login.login.c_str(),
                    GlobalServerData::serverSettings.database_login.pass.c_str()
                    ))
        #else
        if(!GlobalServerData::serverPrivateVariables.db_login->syncConnect(
                    GlobalServerData::serverSettings.database_login.host.c_str(),
                    GlobalServerData::serverSettings.database_login.db.c_str(),
                    GlobalServerData::serverSettings.database_login.login.c_str(),
                    GlobalServerData::serverSettings.database_login.pass.c_str()
                    ))
        #endif
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_login->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_login->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_login.host
                      << " (" << GlobalServerData::serverSettings.database_login.db << ")"
                      << std::endl;
        break;
    }
    if(!GlobalServerData::serverPrivateVariables.db_login->asyncWrite("SELECT * FROM account"))
    {
        std::cerr << "Basic test failed: " << GlobalServerData::serverPrivateVariables.db_login->errorMessage() << std::endl;
        return false;
    }

    switch(GlobalServerData::serverSettings.database_base.tryOpenType)
    {
        default:
        std::cerr << "database type unknown" << std::endl;
        return false;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::Mysql:
        if(!GlobalServerData::serverPrivateVariables.db_base->syncConnectMysql(
                    GlobalServerData::serverSettings.database_base.host.c_str(),
                    GlobalServerData::serverSettings.database_base.db.c_str(),
                    GlobalServerData::serverSettings.database_base.login.c_str(),
                    GlobalServerData::serverSettings.database_base.pass.c_str()
                    ))
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_base->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_base->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_base.host
                      << " (" << GlobalServerData::serverSettings.database_base.db << ")"
                      << std::endl;
        break;

        case DatabaseBase::DatabaseType::SQLite:
        if(!GlobalServerData::serverPrivateVariables.db_base->syncConnectSqlite(GlobalServerData::serverSettings.database_base.file.c_str()))
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_base->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_base->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_base.file << std::endl;
        break;
        #endif

        case DatabaseBase::DatabaseType::PostgreSQL:
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(!GlobalServerData::serverPrivateVariables.db_base->syncConnectPostgresql(
                    GlobalServerData::serverSettings.database_base.host.c_str(),
                    GlobalServerData::serverSettings.database_base.db.c_str(),
                    GlobalServerData::serverSettings.database_base.login.c_str(),
                    GlobalServerData::serverSettings.database_base.pass.c_str()
                    ))
        #else
        if(!GlobalServerData::serverPrivateVariables.db_base->syncConnect(
                    GlobalServerData::serverSettings.database_base.host.c_str(),
                    GlobalServerData::serverSettings.database_base.db.c_str(),
                    GlobalServerData::serverSettings.database_base.login.c_str(),
                    GlobalServerData::serverSettings.database_base.pass.c_str()
                    ))
        #endif
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_base->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_base->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_base.host
                      << " (" << GlobalServerData::serverSettings.database_base.db << ")"
                      << std::endl;
        break;
    }
    if(!GlobalServerData::serverPrivateVariables.db_base->asyncWrite("SELECT * FROM dictionary_reputation"))
    {
        std::cerr << "Basic test failed: " << GlobalServerData::serverPrivateVariables.db_base->errorMessage() << std::endl;
        return false;
    }
    #endif

    switch(GlobalServerData::serverSettings.database_common.tryOpenType)
    {
        default:
        std::cerr << "database type unknown" << std::endl;
        return false;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::Mysql:
        if(!GlobalServerData::serverPrivateVariables.db_common->syncConnectMysql(
                    GlobalServerData::serverSettings.database_common.host.c_str(),
                    GlobalServerData::serverSettings.database_common.db.c_str(),
                    GlobalServerData::serverSettings.database_common.login.c_str(),
                    GlobalServerData::serverSettings.database_common.pass.c_str()
                    ))
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_common->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_common.host
                      << " (" << GlobalServerData::serverSettings.database_common.db << ")"
                      << std::endl;
        break;

        case DatabaseBase::DatabaseType::SQLite:
        if(!GlobalServerData::serverPrivateVariables.db_common->syncConnectSqlite(GlobalServerData::serverSettings.database_common.file.c_str()))
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_common->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_common.file << std::endl;
        break;
        #endif

        case DatabaseBase::DatabaseType::PostgreSQL:
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(!GlobalServerData::serverPrivateVariables.db_common->syncConnectPostgresql(
                    GlobalServerData::serverSettings.database_common.host.c_str(),
                    GlobalServerData::serverSettings.database_common.db.c_str(),
                    GlobalServerData::serverSettings.database_common.login.c_str(),
                    GlobalServerData::serverSettings.database_common.pass.c_str()
                    ))
        #else
        if(!GlobalServerData::serverPrivateVariables.db_common->syncConnect(
                    GlobalServerData::serverSettings.database_common.host.c_str(),
                    GlobalServerData::serverSettings.database_common.db.c_str(),
                    GlobalServerData::serverSettings.database_common.login.c_str(),
                    GlobalServerData::serverSettings.database_common.pass.c_str()
                    ))
        #endif
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_common->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_common.host
                      << " (" << GlobalServerData::serverSettings.database_common.db << ")"
                      << std::endl;
        break;
    }
    if(!GlobalServerData::serverPrivateVariables.db_common->asyncWrite("SELECT * FROM character"))
    {
        std::cerr << "Basic test failed: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        return false;
    }
    switch(GlobalServerData::serverSettings.database_server.tryOpenType)
    {
        default:
        std::cerr << "database type unknown" << std::endl;
        return false;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::Mysql:
        if(!GlobalServerData::serverPrivateVariables.db_server->syncConnectMysql(
                    GlobalServerData::serverSettings.database_server.host.c_str(),
                    GlobalServerData::serverSettings.database_server.db.c_str(),
                    GlobalServerData::serverSettings.database_server.login.c_str(),
                    GlobalServerData::serverSettings.database_server.pass.c_str()
                    ))
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_server->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_server.host
                      << " (" << GlobalServerData::serverSettings.database_server.db << ")"
                      << std::endl;
        break;

        case DatabaseBase::DatabaseType::SQLite:
        if(!GlobalServerData::serverPrivateVariables.db_server->syncConnectSqlite(GlobalServerData::serverSettings.database_server.file.c_str()))
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_server->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_server.file << std::endl;
        break;
        #endif

        case DatabaseBase::DatabaseType::PostgreSQL:
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(!GlobalServerData::serverPrivateVariables.db_server->syncConnectPostgresql(
                    GlobalServerData::serverSettings.database_server.host.c_str(),
                    GlobalServerData::serverSettings.database_server.db.c_str(),
                    GlobalServerData::serverSettings.database_server.login.c_str(),
                    GlobalServerData::serverSettings.database_server.pass.c_str()
                    ))
        #else
        if(!GlobalServerData::serverPrivateVariables.db_server->syncConnect(
                    GlobalServerData::serverSettings.database_server.host.c_str(),
                    GlobalServerData::serverSettings.database_server.db.c_str(),
                    GlobalServerData::serverSettings.database_server.login.c_str(),
                    GlobalServerData::serverSettings.database_server.pass.c_str()
                    ))
        #endif
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_server->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_server.host
                      << " (" << GlobalServerData::serverSettings.database_server.db << ")"
                      << std::endl;
        break;
    }
    if(!GlobalServerData::serverPrivateVariables.db_server->asyncWrite("SELECT * FROM character_forserver"))
    {
        std::cerr << "Basic test failed: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
        return false;
    }

    initialize_the_database_prepared_query();
    return true;
}

void BaseServer::initialize_the_database_prepared_query()
{
    #if ! defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLIENT)
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        //test memory leak around prepared string
        StringWithReplacement test;
        test="SELECT id,encode(password,'hex') FROM account WHERE login=decode('%1','hex')";
    }
    #endif
    PreparedDBQueryLogin::initDatabaseQueryLogin(GlobalServerData::serverPrivateVariables.db_login->databaseType(),GlobalServerData::serverPrivateVariables.db_login);
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommonForLogin.initDatabaseQueryCommonForLogin(GlobalServerData::serverPrivateVariables.db_common->databaseType(),GlobalServerData::serverPrivateVariables.db_common);
    #endif
    //PreparedDBQueryBase::initDatabaseQueryBase(GlobalServerData::serverPrivateVariables.db_base->databaseType());//don't exist, allow dictionary and loaded without cache
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.initDatabaseQueryCommonWithoutSP(GlobalServerData::serverPrivateVariables.db_common->databaseType(),GlobalServerData::serverPrivateVariables.db_common);
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.initDatabaseQueryCommonWithSP(GlobalServerData::serverPrivateVariables.db_common->databaseType(),CommonSettingsServer::commonSettingsServer.useSP,GlobalServerData::serverPrivateVariables.db_common);
    GlobalServerData::serverPrivateVariables.preparedDBQueryServer.initDatabaseQueryServer(GlobalServerData::serverPrivateVariables.db_server->databaseType(),GlobalServerData::serverPrivateVariables.db_server);
}

#ifdef CATCHCHALLENGER_CACHE_HPS
#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
void BaseServer::setMaster(
    const std::string &master_host,
    const uint16_t &master_port,
    const uint8_t &master_tryInterval,
    const uint8_t &master_considerDownAfterNumberOfTry)
{
    if(out_file!=nullptr)
    {
        hps::to_stream(master_host, *out_file);
        hps::to_stream(master_port, *out_file);
        hps::to_stream(master_tryInterval, *out_file);
        hps::to_stream(master_considerDownAfterNumberOfTry, *out_file);
    }
}
#endif
#endif

void BaseServer::setSettings(const GameServerSettings &settings)
{
    //load it
    GlobalServerData::serverSettings=settings;
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    memcpy(Client::private_token_statclient,settings.private_token_statclient,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
    #endif

    loadAndFixSettings();

    #ifdef CATCHCHALLENGER_CACHE_HPS
    if(out_file!=nullptr)
    {
        hps::to_stream(GlobalServerData::serverSettings, *out_file);

        hps::to_stream(CommonSettingsServer::commonSettingsServer.useSP, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.autoLearn, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.useSP, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.forcedSpeed, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.dontSendPseudo, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.mainDatapackCode, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.subDatapackCode, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.exportedXml, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.rates_xp, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.rates_gold, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.rates_xp_pow, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.rates_drop, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.chat_allow_all, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.chat_allow_local, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.chat_allow_private, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.chat_allow_clan, *out_file);
    }
    #endif
}

#ifdef CATCHCHALLENGER_CACHE_HPS
void BaseServer::setNormalSettings(const NormalServerSettings &normalServerSettings)
{
    if(out_file!=nullptr)
    {
        hps::to_stream(normalServerSettings.proxy, *out_file);
        hps::to_stream(normalServerSettings.proxy_port, *out_file);
        hps::to_stream(normalServerSettings.server_ip, *out_file);
        hps::to_stream(normalServerSettings.server_port, *out_file);
        hps::to_stream(normalServerSettings.useSsl, *out_file);
    }
}
#endif

GameServerSettings BaseServer::getSettings() const
{
    return GlobalServerData::serverSettings;
}
