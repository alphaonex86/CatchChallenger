
#include "EventLoopServer.hpp"
#include <iostream>
#ifdef CATCHCHALLENGER_DB_POSTGRESQL
#include "db/EventLoopPostgresql.hpp"
#endif
#ifdef CATCHCHALLENGER_DB_MYSQL
#include "db/EventLoopMySQL.hpp"
#endif


#include "../base/ServerStructures.hpp"
#include "../base/GlobalServerData.hpp"

using namespace CatchChallenger;

EventLoopServer::EventLoopServer()
{
    ready=false;

    //normalServerSettings.server_ip
    normalServerSettings.server_port    = 42489;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverPrivateVariables.db_server=NULL;
    GlobalServerData::serverPrivateVariables.db_common=NULL;
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    GlobalServerData::serverPrivateVariables.db_base=NULL;
    GlobalServerData::serverPrivateVariables.db_login=NULL;
    #endif
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}

void EventLoopServer::initTheDatabase()
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    switch(GlobalServerData::serverSettings.database_login.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            GlobalServerData::serverPrivateVariables.db_login=new EventLoopPostgresql();
        break;
        #endif
        #ifdef CATCHCHALLENGER_DB_MYSQL
        case DatabaseBase::DatabaseType::Mysql:
            GlobalServerData::serverPrivateVariables.db_login=new EventLoopMySQL();
        break;
        #endif
        default:
            std::cerr << "database type wrong " << DatabaseBase::databaseTypeToString(GlobalServerData::serverSettings.database_login.tryOpenType) << std::endl;
        abort();
    }
    switch(GlobalServerData::serverSettings.database_base.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            GlobalServerData::serverPrivateVariables.db_base=new EventLoopPostgresql();
        break;
        #endif
        #ifdef CATCHCHALLENGER_DB_MYSQL
        case DatabaseBase::DatabaseType::Mysql:
            GlobalServerData::serverPrivateVariables.db_base=new EventLoopMySQL();
        break;
        #endif
        default:
            std::cerr << "database type wrong " << DatabaseBase::databaseTypeToString(GlobalServerData::serverSettings.database_base.tryOpenType) << std::endl;
        abort();
    }
    #endif
    switch(GlobalServerData::serverSettings.database_common.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            GlobalServerData::serverPrivateVariables.db_common=new EventLoopPostgresql();
        break;
        #endif
        #ifdef CATCHCHALLENGER_DB_MYSQL
        case DatabaseBase::DatabaseType::Mysql:
            GlobalServerData::serverPrivateVariables.db_common=new EventLoopMySQL();
        break;
        #endif
        default:
            std::cerr << "database type wrong " << DatabaseBase::databaseTypeToString(GlobalServerData::serverSettings.database_common.tryOpenType) << std::endl;
        abort();
    }
    switch(GlobalServerData::serverSettings.database_server.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            GlobalServerData::serverPrivateVariables.db_server=new EventLoopPostgresql();
        break;
        #endif
        #ifdef CATCHCHALLENGER_DB_MYSQL
        case DatabaseBase::DatabaseType::Mysql:
            GlobalServerData::serverPrivateVariables.db_server=new EventLoopMySQL();
        break;
        #endif
        default:
            std::cerr << "database type wrong " << DatabaseBase::databaseTypeToString(GlobalServerData::serverSettings.database_server.tryOpenType) << std::endl;
        abort();
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}


EventLoopServer::~EventLoopServer()
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    switch(GlobalServerData::serverSettings.database_server.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            delete static_cast<EventLoopPostgresql *>(GlobalServerData::serverPrivateVariables.db_server);
        break;
        #endif
        #ifdef CATCHCHALLENGER_DB_MYSQL
        case DatabaseBase::DatabaseType::Mysql:
            delete static_cast<EventLoopMySQL *>(GlobalServerData::serverPrivateVariables.db_server);
        break;
        #endif
        default:
        break;
    }
    switch(GlobalServerData::serverSettings.database_common.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            delete static_cast<EventLoopPostgresql *>(GlobalServerData::serverPrivateVariables.db_common);
        break;
        #endif
        #ifdef CATCHCHALLENGER_DB_MYSQL
        case DatabaseBase::DatabaseType::Mysql:
            delete static_cast<EventLoopMySQL *>(GlobalServerData::serverPrivateVariables.db_common);
        break;
        #endif
        default:
        break;
    }
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    switch(GlobalServerData::serverSettings.database_base.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            delete static_cast<EventLoopPostgresql *>(GlobalServerData::serverPrivateVariables.db_base);
        break;
        #endif
        #ifdef CATCHCHALLENGER_DB_MYSQL
        case DatabaseBase::DatabaseType::Mysql:
            delete static_cast<EventLoopMySQL *>(GlobalServerData::serverPrivateVariables.db_base);
        break;
        #endif
        default:
        break;
    }
    switch(GlobalServerData::serverSettings.database_login.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            delete static_cast<EventLoopPostgresql *>(GlobalServerData::serverPrivateVariables.db_login);
        break;
        #endif
        #ifdef CATCHCHALLENGER_DB_MYSQL
        case DatabaseBase::DatabaseType::Mysql:
            delete static_cast<EventLoopMySQL *>(GlobalServerData::serverPrivateVariables.db_login);
        break;
        #endif
        default:
        break;
    }
    #endif
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}

void EventLoopServer::preload_finish()
{
    BaseServer::preload_finish();
    if(!ready)
    {
        std::cout << "Waiting connection on port " << normalServerSettings.server_port << std::endl;
        ready=true;

        if(!tryListen())
            abort();
    }
    else
        std::cerr << "EventLoopServer::preload_finish() double event dropped" << std::endl;
}

bool EventLoopServer::isReady()
{
    return ready;
}

bool EventLoopServer::tryListen()
{
    if(!normalServerSettings.server_ip.empty())
        return tryListenInternal(normalServerSettings.server_ip.c_str(),std::to_string(normalServerSettings.server_port).c_str());
    else
        return tryListenInternal(NULL,std::to_string(normalServerSettings.server_port).c_str());
}

void EventLoopServer::quitForCriticalDatabaseQueryFailed()
{
    abort();
}

void EventLoopServer::preload_1_the_data()
{
    BaseServer::preload_1_the_data();
}

void EventLoopServer::unload_the_data()
{
    close();
    ready=false;
    BaseServer::unload_the_data();
}

void EventLoopServer::setNormalSettings(const NormalServerSettings &settings)
{
    normalServerSettings=settings;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    //BaseServer::setNormalSettings only exists when the HPS cache is
    //compiled in — it's part of the binary-cache load/save lifecycle.
    //Without HPS the BaseServer fields are set directly via the
    //regular settings load path, so this call is a no-op.
    BaseServer::setNormalSettings(settings);
    #endif
    loadAndFixSettings();
    if(normalServerSettings.server_port<=0)
        normalServerSettings.server_port=42489;
    if(normalServerSettings.proxy_port<=0)
        normalServerSettings.proxy.clear();
}

NormalServerSettings EventLoopServer::getNormalSettings() const
{
    return normalServerSettings;
}

void EventLoopServer::loadAndFixSettings()
{
    BaseServer::loadAndFixSettings();
}

