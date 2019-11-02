#ifndef SERVERSSL

#include "EpollServer.h"
#include "EpollSocket.h"
#include "Epoll.h"

#include "../base/ServerStructures.h"
#include "../base/GlobalServerData.h"

using namespace CatchChallenger;

EpollServer::EpollServer()
{
    ready=false;

    //normalServerSettings.server_ip
    normalServerSettings.server_port    = 42489;
    normalServerSettings.useSsl         = true;
    GlobalServerData::serverPrivateVariables.db_server=NULL;
    GlobalServerData::serverPrivateVariables.db_common=NULL;
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    GlobalServerData::serverPrivateVariables.db_base=NULL;
    GlobalServerData::serverPrivateVariables.db_login=NULL;
    #endif
}

void EpollServer::initTheDatabase()
{
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    switch(GlobalServerData::serverSettings.database_login.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            GlobalServerData::serverPrivateVariables.db_login=new EpollPostgresql();
        break;
        #endif
        #ifdef CATCHCHALLENGER_DB_MYSQL
        case DatabaseBase::DatabaseType::Mysql:
            GlobalServerData::serverPrivateVariables.db_login=new EpollMySQL();
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
            GlobalServerData::serverPrivateVariables.db_base=new EpollPostgresql();
        break;
        #endif
        #ifdef CATCHCHALLENGER_DB_MYSQL
        case DatabaseBase::DatabaseType::Mysql:
            GlobalServerData::serverPrivateVariables.db_base=new EpollMySQL();
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
            GlobalServerData::serverPrivateVariables.db_common=new EpollPostgresql();
        break;
        #endif
        #ifdef CATCHCHALLENGER_DB_MYSQL
        case DatabaseBase::DatabaseType::Mysql:
            GlobalServerData::serverPrivateVariables.db_common=new EpollMySQL();
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
            GlobalServerData::serverPrivateVariables.db_server=new EpollPostgresql();
        break;
        #endif
        #ifdef CATCHCHALLENGER_DB_MYSQL
        case DatabaseBase::DatabaseType::Mysql:
            GlobalServerData::serverPrivateVariables.db_server=new EpollMySQL();
        break;
        #endif
        default:
            std::cerr << "database type wrong " << DatabaseBase::databaseTypeToString(GlobalServerData::serverSettings.database_server.tryOpenType) << std::endl;
        abort();
    }
}


EpollServer::~EpollServer()
{
    switch(GlobalServerData::serverSettings.database_server.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            delete static_cast<EpollPostgresql *>(GlobalServerData::serverPrivateVariables.db_server);
        break;
        #endif
        #ifdef CATCHCHALLENGER_DB_MYSQL
        case DatabaseBase::DatabaseType::Mysql:
            delete static_cast<EpollMySQL *>(GlobalServerData::serverPrivateVariables.db_server);
        break;
        #endif
        default:
        break;
    }
    switch(GlobalServerData::serverSettings.database_common.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            delete static_cast<EpollPostgresql *>(GlobalServerData::serverPrivateVariables.db_common);
        break;
        #endif
        #ifdef CATCHCHALLENGER_DB_MYSQL
        case DatabaseBase::DatabaseType::Mysql:
            delete static_cast<EpollMySQL *>(GlobalServerData::serverPrivateVariables.db_common);
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
            delete static_cast<EpollPostgresql *>(GlobalServerData::serverPrivateVariables.db_base);
        break;
        #endif
        #ifdef CATCHCHALLENGER_DB_MYSQL
        case DatabaseBase::DatabaseType::Mysql:
            delete static_cast<EpollMySQL *>(GlobalServerData::serverPrivateVariables.db_base);
        break;
        #endif
        default:
        break;
    }
    switch(GlobalServerData::serverSettings.database_login.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            delete static_cast<EpollPostgresql *>(GlobalServerData::serverPrivateVariables.db_login);
        break;
        #endif
        #ifdef CATCHCHALLENGER_DB_MYSQL
        case DatabaseBase::DatabaseType::Mysql:
            delete static_cast<EpollMySQL *>(GlobalServerData::serverPrivateVariables.db_login);
        break;
        #endif
        default:
        break;
    }
    #endif



}

void EpollServer::preload_finish()
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
        std::cerr << "EpollServer::preload_finish() double event dropped" << std::endl;
}

bool EpollServer::isReady()
{
    return ready;
}

bool EpollServer::tryListen()
{
    if(!normalServerSettings.server_ip.empty())
        return tryListenInternal(normalServerSettings.server_ip.c_str(),std::to_string(normalServerSettings.server_port).c_str());
    else
        return tryListenInternal(NULL,std::to_string(normalServerSettings.server_port).c_str());
}

void EpollServer::quitForCriticalDatabaseQueryFailed()
{
    abort();
}

void EpollServer::preload_the_data()
{
    BaseServer::preload_the_data();
}

void EpollServer::unload_the_data()
{
    close();
    ready=false;
    BaseServer::unload_the_data();
}

void EpollServer::setNormalSettings(const NormalServerSettings &settings)
{
    normalServerSettings=settings;
    BaseServer::setNormalSettings(settings);
    loadAndFixSettings();
}

NormalServerSettings EpollServer::getNormalSettings() const
{
    return normalServerSettings;
}

void EpollServer::loadAndFixSettings()
{
    if(normalServerSettings.server_port<=0)
        normalServerSettings.server_port=42489;
    if(normalServerSettings.proxy_port<=0)
        normalServerSettings.proxy.clear();
}
#endif
