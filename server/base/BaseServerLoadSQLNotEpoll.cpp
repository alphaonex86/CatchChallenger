#include "BaseServer.hpp"
#include "GlobalServerData.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/cpp11addition.hpp"
#include <iostream>

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
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    if(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        bool ok;
        std::string zoneCodeName=entryListZone.at(entryListIndex).name;
        stringreplaceOne(zoneCodeName,".xml","");
        const std::string &tempString=std::string(GlobalServerData::serverPrivateVariables.db_server->value(0));
        const uint32_t &clanId=stringtouint32(tempString,&ok);
        if(ok)
        {
            if(CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId().find(zoneCodeName)!=CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId().cend())
            {
                const ZONE_TYPE &zoneId=CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId().at(zoneCodeName);
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
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    entryListIndex++;
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    if(GlobalServerData::serverSettings.automatic_account_creation)
        load_account_max_id();
    else if(CommonSettingsCommon::commonSettingsCommon.max_character)
        load_character_max_id();
    else
    #endif
    preload_17_async_baseServerMasterLoadDictionaryLoad();
}

//call before load map
void BaseServer::preload_16_async_zone_sql()
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    uint16_t indexZone=0;
    if(entryListZone.empty())
    {
        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        if(GlobalServerData::serverSettings.automatic_account_creation)
            load_account_max_id();
        else if(CommonSettingsCommon::commonSettingsCommon.max_character)
            load_character_max_id();
        else
        #endif
        preload_17_async_baseServerMasterLoadDictionaryLoad();
    }
    else
    {
        while(entryListIndex<entryListZone.size())
        {
            std::string zoneCodeName=entryListZone.at(entryListIndex).name;
            stringreplaceOne(zoneCodeName,".xml","");
            if(CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId().find(zoneCodeName)==CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId().cend())
            {
                std::unordered_map<std::string,ZONE_TYPE> &zoneToId=CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId_rw();
                zoneToId[zoneCodeName]=indexZone;
                std::vector<std::string> &idToZone=CommonDatapackServerSpec::commonDatapackServerSpec.get_idToZone_rw();
                while(idToZone.size()<=indexZone)
                    idToZone.push_back(std::string());
                idToZone[indexZone]=zoneCodeName;
                if(indexZone>60000)
                {
                    std::cerr << "Error, zone count can't be > 60000" << std::endl;
                    abort();
                }
                indexZone++;
            }
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

                #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                if(GlobalServerData::serverSettings.automatic_account_creation)
                    load_account_max_id();
                else if(CommonSettingsCommon::commonSettingsCommon.max_character)
                    load_character_max_id();
                else
                #endif
                preload_17_async_baseServerMasterLoadDictionaryLoad();
                return;
            }
            else
                return;
        }
        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        if(GlobalServerData::serverSettings.automatic_account_creation)
            load_account_max_id();
        else if(CommonSettingsCommon::commonSettingsCommon.max_character)
            load_character_max_id();
        else
        #endif
        preload_17_async_baseServerMasterLoadDictionaryLoad();
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    preload_zone_return();
    #elif CATCHCHALLENGER_DB_FILE
    preload_zone_return();
    #else
    #error Define what do here
    #endif
}
