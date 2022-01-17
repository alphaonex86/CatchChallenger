#include "BaseServer.hpp"
#include "GlobalServerData.hpp"
#include "../general/base/CommonDatapackServerSpec.hpp"

using namespace CatchChallenger;

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
    if(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        bool ok;
        std::string zoneCodeName=entryListZone.at(entryListIndex).name;
        stringreplaceOne(zoneCodeName,".xml","");
        const std::string &tempString=std::string(GlobalServerData::serverPrivateVariables.db_server->value(0));
        const uint32_t &clanId=stringtouint32(tempString,&ok);
        if(ok)
        {
            if(CommonDatapackServerSpec::commonDatapackServerSpec.zoneToId.find(zoneCodeName)!=CommonDatapackServerSpec::commonDatapackServerSpec.zoneToId.cend())
            {
                const uint16_t &zoneId=CommonDatapackServerSpec::commonDatapackServerSpec.zoneToId.at(zoneCodeName);
                GlobalServerData::serverPrivateVariables.cityStatusList[zoneId].clan=clanId;
                GlobalServerData::serverPrivateVariables.cityStatusListReverse[clanId]=zoneId;
            }
            else
                std::cerr << "preload_zone_return() zone not found: " << zoneCodeName << std::endl;
        }
        else
            std::cerr << "clan id is failed to convert to number for city status" << std::endl;
    }
    GlobalServerData::serverPrivateVariables.db_server->clear();
    entryListIndex++;
    preload_market_monsters_prices_sql();
}

//call before load map
void BaseServer::preload_zone_sql()
{
    uint16_t indexZone=0;
    if(entryListZone.empty())
        preload_market_monsters_prices_sql();
    else
    {
        while(entryListIndex<entryListZone.size())
        {
            std::string zoneCodeName=entryListZone.at(entryListIndex).name;
            stringreplaceOne(zoneCodeName,".xml","");
            std::string queryText;
            if(CommonDatapackServerSpec::commonDatapackServerSpec.zoneToId.find(zoneCodeName)==CommonDatapackServerSpec::commonDatapackServerSpec.zoneToId.cend())
            {
                CommonDatapackServerSpec::commonDatapackServerSpec.zoneToId[zoneCodeName]=indexZone;
                while(CommonDatapackServerSpec::commonDatapackServerSpec.idToZone.size()<=indexZone)
                    CommonDatapackServerSpec::commonDatapackServerSpec.idToZone.push_back(std::string());
                CommonDatapackServerSpec::commonDatapackServerSpec.idToZone[indexZone]=zoneCodeName;
                if(indexZone>60000)
                {
                    std::cerr << "Error, zone count can't be > 60000" << std::endl;
                    abort();
                }
                indexZone++;
            }
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
