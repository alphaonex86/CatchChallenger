#include "../base/BaseServer.hpp"
#include "../base/GlobalServerData.hpp"
#include "../base/DatabaseFunction.hpp"
#include "../../general/base/GeneralVariable.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/DatapackGeneralLoader.hpp"

#include <string>
#include <vector>
#include <iostream>

using namespace CatchChallenger;

void BaseServer::preload_8_sync_monsters_drops()
{
    GlobalServerData::serverPrivateVariables.monsterDrops=DatapackGeneralLoader::loadMonsterDrop(GlobalServerData::serverSettings.datapack_basePath+DATAPACK_BASE_PATH_MONSTERS,CommonDatapack::commonDatapack.get_items().item,CommonDatapack::commonDatapack.get_monsters());

    std::cout << CommonDatapack::commonDatapack.get_monsters().size() << " monster drop(s) loaded" << std::endl;
}

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
void BaseServer::preload_19_async_sql_monsters_max_id()
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    std::cout << GlobalServerData::serverPrivateVariables.cityStatusList.size() << " SQL city loaded" << std::endl;
    #endif

    //start to 0 due to pre incrementation before use
    GlobalServerData::serverPrivateVariables.maxMonsterId=1;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    std::string queryText;
    switch(GlobalServerData::serverPrivateVariables.db_common->databaseType())
    {
        default:
        #if defined(CATCHCHALLENGER_DB_MYSQL)
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id` FROM `monster` ORDER BY `id` DESC LIMIT 0,1;";
        break;
        #endif
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id FROM monster ORDER BY id DESC LIMIT 0,1;";
        break;
        #endif
        #if defined(CATCHCHALLENGER_DB_POSTGRESQL)
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id FROM monster ORDER BY id DESC LIMIT 1;";
        break;
        #endif
    }
    if(GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&BaseServer::load_monsters_max_id_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        abort();//stop because can't do the first db access
        //preload_20_async_clan_max_id();->abort
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    preload_20_async_clan_max_id();
    #elif CATCHCHALLENGER_DB_FILE
    preload_20_async_clan_max_id();
    #else
    #error Define what do here
    #endif
    return;
}

void BaseServer::load_monsters_max_id_static(void *object)
{
    static_cast<BaseServer *>(object)->load_monsters_max_id_return();
}

void BaseServer::load_monsters_max_id_return()
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        bool ok;
        const uint32_t &maxMonsterId=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
        if(!ok)
        {
            std::cerr << "Max monster id is failed to convert to number" << std::endl;
            continue;
        }
        else
            if(maxMonsterId>=GlobalServerData::serverPrivateVariables.maxMonsterId)
                GlobalServerData::serverPrivateVariables.maxMonsterId=maxMonsterId+1;
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    preload_20_async_clan_max_id();
}
#endif
