#include "BaseServer.hpp"
#ifndef EPOLLCATCHCHALLENGERSERVER
#include "GlobalServerData.hpp"
#endif

using namespace CatchChallenger;

#ifndef EPOLLCATCHCHALLENGERSERVER
bool BaseServer::preload_zone()
{
    //open and quick check the file
    entryListZone=CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(GlobalServerData::serverSettings.datapack_basePath+
                            DATAPACK_BASE_PATH_ZONE1+CommonSettingsServer::commonSettingsServer.mainDatapackCode+DATAPACK_BASE_PATH_ZONE2,
                                                                              CatchChallenger::FacilityLibGeneral::ListFolder::Files);
    entryListIndex=0;
    return preload_zone_init();
}

void BaseServer::preload_zone_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_zone_return();
}

void BaseServer::preload_zone_return()
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        bool ok;
        std::string zoneCodeName=entryListZone.at(entryListIndex).name;
        stringreplaceOne(zoneCodeName,".xml","");
        const std::string &tempString=std::string(GlobalServerData::serverPrivateVariables.db_server->value(0));
        const uint32_t &clanId=stringtouint32(tempString,&ok);
        if(ok)
        {
            GlobalServerData::serverPrivateVariables.cityStatusList[zoneCodeName].clan=clanId;
            GlobalServerData::serverPrivateVariables.cityStatusListReverse[clanId]=zoneCodeName;
        }
        else
            std::cerr << "clan id is failed to convert to number for city status" << std::endl;
    }
    #endif
    GlobalServerData::serverPrivateVariables.db_server->clear();
    entryListIndex++;
    preload_market_monsters_prices_sql();
}

void BaseServer::preload_zone_sql()
{
    if(entryListZone.empty())
        preload_market_monsters_prices_sql();
    else
    {
        while(entryListIndex<entryListZone.size())
        {
            std::string zoneCodeName=entryListZone.at(entryListIndex).name;
            stringreplaceOne(zoneCodeName,".xml","");
            std::string queryText;
            switch(GlobalServerData::serverPrivateVariables.db_common->databaseType())
            {
                default:
                abort();
                break;
                #if defined(CATCHCHALLENGER_DB_MYSQL) || (not defined(EPOLLCATCHCHALLENGERSERVER))
                case DatabaseBase::DatabaseType::Mysql:
                    queryText="SELECT `clan` FROM `city` WHERE `city`='"+zoneCodeName+"'";//ORDER BY city-> drop, unique key
                break;
                    #endif
                #ifndef EPOLLCATCHCHALLENGERSERVER
                case DatabaseBase::DatabaseType::SQLite:
                    queryText="SELECT clan FROM city WHERE city='"+zoneCodeName+"'";//ORDER BY city-> drop, unique key
                break;
                #endif
                #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
                case DatabaseBase::DatabaseType::PostgreSQL:
                    queryText="SELECT clan FROM city WHERE city='"+zoneCodeName+"'";//ORDER BY city-> drop, unique key
                break;
                #endif
            }
            if(GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText,this,&BaseServer::preload_zone_static)==NULL)
            {
                std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
                criticalDatabaseQueryFailed();return;//stop because can't do the first db access
                entryListIndex++;
                preload_market_monsters_sql();
                return;
            }
            else
                return;
        }
        preload_market_monsters_sql();
    }
}
#endif
