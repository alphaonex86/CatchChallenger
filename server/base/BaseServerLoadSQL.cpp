#include "BaseServer.h"
#include "GlobalServerData.h"
#include "DictionaryServer.h"

#include "../../general/base/CommonSettingsCommon.h"

using namespace CatchChallenger;

#ifndef EPOLLCATCHCHALLENGERSERVER
bool BaseServer::preload_zone()
{
    //open and quick check the file
    entryListZone=CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(GlobalServerData::serverSettings.datapack_basePath+DATAPACK_BASE_PATH_ZONE1+CommonSettingsServer::commonSettingsServer.mainDatapackCode+DATAPACK_BASE_PATH_ZONE2,CatchChallenger::FacilityLibGeneral::ListFolder::Files);
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
        stringreplaceOne(zoneCodeName,CACHEDSTRING_dotxml,"");
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
    const int &listsize=entryListZone.size();
    while(entryListIndex<listsize)
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
#endif

void BaseServer::preload_pointOnMap_item_sql()
{
    std::string queryText;
    switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
    {
        default:
        abort();
        break;
        #if defined(CATCHCHALLENGER_DB_MYSQL) || (not defined(EPOLLCATCHCHALLENGERSERVER))
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id`,`map`,`x`,`y` FROM `dictionary_pointonmap_item` ORDER BY `map`,`x`,`y`";
        break;
        #endif
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id,map,x,y FROM dictionary_pointonmap_item ORDER BY map,x,y";
        break;
        #endif
        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id,map,x,y FROM dictionary_pointonmap_item ORDER BY map,x,y";
        break;
        #endif
    }
    if(GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText,this,&BaseServer::preload_pointOnMap_item_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
        criticalDatabaseQueryFailed();return;//stop because can't do the first db access

        preload_the_visibility_algorithm();
        #ifndef EPOLLCATCHCHALLENGERSERVER
        preload_the_city_capture();
        preload_zone();
        #endif

        const auto now=msFrom1970();
        std::cout << "Loaded the server static datapack into " << (now-timeDatapack) << "ms" << std::endl;
        timeDatapack=now;

        //start SQL load
        preload_pointOnMap_plant_sql();
    }
}

void BaseServer::preload_pointOnMap_item_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_pointOnMap_item_return();
}

void BaseServer::preload_pointOnMap_item_return()
{
    bool ok;
    dictionary_pointOnMap_maxId_item=0;
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        const uint16_t &id=stringtouint16(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
        if(!ok)
            std::cerr << "preload_itemOnMap_return(): Id not found: " << GlobalServerData::serverPrivateVariables.db_server->value(0) << std::endl;
        else
        {
            if(dictionary_pointOnMap_maxId_item<id)
                dictionary_pointOnMap_maxId_item=id;//here to prevent later bug create problem with max id
            const uint32_t &map_id=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(1),&ok);
            if(!ok)
                std::cerr << "preload_itemOnMap_return(): map id not number: " << GlobalServerData::serverPrivateVariables.db_server->value(1) << std::endl;
            else
            {
                if(map_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
                    std::cerr << "preload_itemOnMap_return(): map out of range: " << map_id << std::endl;
                else
                {
                    MapServer * const map_server=DictionaryServer::dictionary_map_database_to_internal.at(map_id);
                    if(map_server==NULL)
                        std::cerr << "preload_itemOnMap_return(): map == NULL for this id, map not found: " << map_id << std::endl;
                    else
                    {
                        const uint32_t &x=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(2),&ok);
                        if(!ok)
                            std::cerr << "preload_itemOnMap_return(): x not number: " << GlobalServerData::serverPrivateVariables.db_server->value(2) << std::endl;
                        else
                        {
                            if(x>255 || x>=map_server->width)
                                std::cerr << "preload_itemOnMap_return(): x out of range: " << x << ", for " << map_id << std::endl;
                            else
                            {
                                const uint32_t &y=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(3),&ok);
                                if(!ok)
                                    std::cerr << "preload_itemOnMap_return(): y not number: " << GlobalServerData::serverPrivateVariables.db_server->value(3) << ", for " << map_id << std::endl;
                                else
                                {
                                    if(y>255 || y>=map_server->height)
                                        std::cerr << "preload_itemOnMap_return(): y out of range: " << y << ", for " << map_id << std::endl;
                                    else
                                    {
                                        const auto &pair=std::pair<uint8_t,uint8_t>(x,y);
                                        //const std::string &map_file=map_server->map_file;

                                        ///used only at map loading, \see BaseServer::preload_the_map()
                                        if(map_server->pointOnMap_Item.find(pair)!=map_server->pointOnMap_Item.cend())
                                            map_server->pointOnMap_Item[pair].pointOnMapDbCode=id;
                                        if(map_server->plants.find(pair)!=map_server->plants.cend())
                                            map_server->plants[pair].pointOnMapDbCode=id;

                                        /* do after the datapack is loaded while((uint32_t)DictionaryServer::dictionary_pointOnMap_database_to_internal.size()<=id)
                                        {
                                            DictionaryServer::MapAndPoint mapAndPoint;
                                            mapAndPoint.map=NULL;
                                            mapAndPoint.x=0;
                                            mapAndPoint.y=0;
                                            mapAndPoint.datapack_index=0;
                                            DictionaryServer::dictionary_pointOnMap_database_to_internal.push_back(mapAndPoint);
                                        }

                                        DictionaryServer::MapAndPoint mapAndPoint;
                                        mapAndPoint.map=map_server;
                                        mapAndPoint.x=x;
                                        mapAndPoint.y=y;
                                        mapAndPoint.datapack_index=0;
                                        DictionaryServer::dictionary_pointOnMap_database_to_internal[id]=mapAndPoint;*/

                                        //std::string,std::map<std::pair<uint8_t/*x*/,uint8_t/*y*/>,uint16_t/*db code*/,pairhash>
                                        DictionaryServer::dictionary_pointOnMap_item_internal_to_database[map_server->map_file][pair]=id;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    GlobalServerData::serverPrivateVariables.db_server->clear();
    {
        std::cout << DictionaryServer::dictionary_pointOnMap_item_internal_to_database.size() << " SQL item on map dictionary" << std::endl;

        preload_the_visibility_algorithm();
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(!preload_the_city_capture())
            return;
        if(!preload_zone())
            return;
        #endif
        const auto now = msFrom1970();
        std::cout << "Loaded the server static datapack into " << (now-timeDatapack) << "ms" << std::endl;
        timeDatapack=now;

        //start SQL load
        preload_pointOnMap_plant_sql();
    }
}

void BaseServer::preload_pointOnMap_plant_sql()
{
    std::string queryText;
    switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
    {
        default:
        abort();
        break;
        #if defined(CATCHCHALLENGER_DB_MYSQL) || (not defined(EPOLLCATCHCHALLENGERSERVER))
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id`,`map`,`x`,`y` FROM `dictionary_pointonmap_plant` ORDER BY `map`,`x`,`y`";
        break;
        #endif
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id,map,x,y FROM dictionary_pointonmap_plant ORDER BY map,x,y";
        break;
        #endif
        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id,map,x,y FROM dictionary_pointonmap_plant ORDER BY map,x,y";
        break;
        #endif
    }
    if(GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText,this,&BaseServer::preload_pointOnMap_plant_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
        criticalDatabaseQueryFailed();return;//stop because can't do the first db access

        preload_the_visibility_algorithm();
        #ifndef EPOLLCATCHCHALLENGERSERVER
        preload_the_city_capture();
        preload_zone();
        #endif

        const auto now=msFrom1970();
        std::cout << "Loaded the server static datapack into " << (now-timeDatapack) << "ms" << std::endl;
        timeDatapack=now;

        //start SQL load
        preload_map_semi_after_db_id();
    }
}

void BaseServer::preload_pointOnMap_plant_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_pointOnMap_plant_return();
}

void BaseServer::preload_pointOnMap_plant_return()
{
    bool ok;
    dictionary_pointOnMap_maxId_plant=0;
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        const uint16_t &id=stringtouint16(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
        if(!ok)
            std::cerr << "preload_itemOnMap_return(): Id not found: " << GlobalServerData::serverPrivateVariables.db_server->value(0) << std::endl;
        else
        {
            if(dictionary_pointOnMap_maxId_plant<id)
                dictionary_pointOnMap_maxId_plant=id;//here to prevent later bug create problem with max id
            const uint32_t &map_id=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(1),&ok);
            if(!ok)
                std::cerr << "preload_itemOnMap_return(): map id not number: " << GlobalServerData::serverPrivateVariables.db_server->value(1) << std::endl;
            else
            {
                if(map_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
                    std::cerr << "preload_itemOnMap_return(): map out of range: " << map_id << std::endl;
                else
                {
                    MapServer * const map_server=DictionaryServer::dictionary_map_database_to_internal.at(map_id);
                    if(map_server==NULL)
                        std::cerr << "preload_itemOnMap_return(): map == NULL for this id, map not found: " << map_id << std::endl;
                    else
                    {
                        const uint32_t &x=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(2),&ok);
                        if(!ok)
                            std::cerr << "preload_itemOnMap_return(): x not number: " << GlobalServerData::serverPrivateVariables.db_server->value(2) << std::endl;
                        else
                        {
                            if(x>255 || x>=map_server->width)
                                std::cerr << "preload_itemOnMap_return(): x out of range: " << x << ", for " << map_id << std::endl;
                            else
                            {
                                const uint32_t &y=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(3),&ok);
                                if(!ok)
                                    std::cerr << "preload_itemOnMap_return(): y not number: " << GlobalServerData::serverPrivateVariables.db_server->value(3) << ", for " << map_id << std::endl;
                                else
                                {
                                    if(y>255 || y>=map_server->height)
                                        std::cerr << "preload_itemOnMap_return(): y out of range: " << y << ", for " << map_id << std::endl;
                                    else
                                    {
                                        const auto &pair=std::pair<uint8_t,uint8_t>(x,y);
                                        //const std::string &map_file=map_server->map_file;

                                        ///used only at map loading, \see BaseServer::preload_the_map()
                                        if(map_server->pointOnMap_Item.find(pair)!=map_server->pointOnMap_Item.cend())
                                            map_server->pointOnMap_Item[pair].pointOnMapDbCode=id;
                                        if(map_server->plants.find(pair)!=map_server->plants.cend())
                                            map_server->plants[pair].pointOnMapDbCode=id;

                                        /* do after the datapack is loaded while((uint32_t)DictionaryServer::dictionary_pointOnMap_database_to_internal.size()<=id)
                                        {
                                            DictionaryServer::MapAndPoint mapAndPoint;
                                            mapAndPoint.map=NULL;
                                            mapAndPoint.x=0;
                                            mapAndPoint.y=0;
                                            mapAndPoint.datapack_index=0;
                                            DictionaryServer::dictionary_pointOnMap_database_to_internal.push_back(mapAndPoint);
                                        }

                                        DictionaryServer::MapAndPoint mapAndPoint;
                                        mapAndPoint.map=map_server;
                                        mapAndPoint.x=x;
                                        mapAndPoint.y=y;
                                        mapAndPoint.datapack_index=0;
                                        DictionaryServer::dictionary_pointOnMap_database_to_internal[id]=mapAndPoint;*/

                                        //std::string,std::map<std::pair<uint8_t/*x*/,uint8_t/*y*/>,uint16_t/*db code*/,pairhash>
                                        DictionaryServer::dictionary_pointOnMap_plant_internal_to_database[map_server->map_file][pair]=id;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    GlobalServerData::serverPrivateVariables.db_server->clear();
    {
        std::cout << DictionaryServer::dictionary_pointOnMap_plant_internal_to_database.size() << " SQL plant on map dictionary" << std::endl;

        preload_the_visibility_algorithm();
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(!preload_the_city_capture())
            return;
        if(!preload_zone())
            return;
        #endif
        const auto now = msFrom1970();
        std::cout << "Loaded the server static datapack into " << (now-timeDatapack) << "ms" << std::endl;
        timeDatapack=now;

        //start SQL load
        preload_map_semi_after_db_id();
    }
}

void BaseServer::preload_dictionary_map()
{
    if(GlobalServerData::serverPrivateVariables.db_server==NULL)
    {
        std::cerr << "GlobalServerData::serverPrivateVariables.db_server==NULL" << std::endl;
        abort();
    }
    if(GlobalServerData::serverPrivateVariables.map_list.size()==0)
    {
        std::cerr << "No map to list" << std::endl;
        abort();
    }
    if(DictionaryServer::dictionary_map_database_to_internal.size()>0)
    {
        std::cerr << "preload_dictionary_map() already call" << std::endl;
        abort();
    }
    std::string queryText;
    switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
    {
        default:
        abort();
        break;
        #if defined(CATCHCHALLENGER_DB_MYSQL) || (not defined(EPOLLCATCHCHALLENGERSERVER))
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id`,`map` FROM `dictionary_map` ORDER BY `map`";
        break;
        #endif
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id,map FROM dictionary_map ORDER BY map";
        break;
        #endif
        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id,map FROM dictionary_map ORDER BY map";
        break;
        #endif
    }
    if(GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText,this,&BaseServer::preload_dictionary_map_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
        criticalDatabaseQueryFailed();return;//stop because can't resolv the name
    }
}

void BaseServer::preload_dictionary_map_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_dictionary_map_return();
}

void BaseServer::preload_dictionary_map_return()
{
    if(DictionaryServer::dictionary_map_database_to_internal.size()>0)
    {
        std::cerr << "preload_dictionary_map_return() already call" << std::endl;
        abort();
    }
    std::unordered_set<std::string> foundMap;
    unsigned int databaseMapId=0;
    unsigned int obsoleteMap=0;
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        bool ok;
        databaseMapId=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
        const std::string &map=std::string(GlobalServerData::serverPrivateVariables.db_server->value(1));
        if(DictionaryServer::dictionary_map_database_to_internal.size()<=databaseMapId)
        {
            unsigned int index=DictionaryServer::dictionary_map_database_to_internal.size();
            while(index<=databaseMapId)
            {
                DictionaryServer::dictionary_map_database_to_internal.push_back(NULL);
                index++;
            }
        }
        if(GlobalServerData::serverPrivateVariables.map_list.find(map)!=GlobalServerData::serverPrivateVariables.map_list.end())
        {
            DictionaryServer::dictionary_map_database_to_internal[databaseMapId]=static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list.at(map));
            foundMap.insert(map);
            static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list.at(map))->reverse_db_id=databaseMapId;
        }
        else
            obsoleteMap++;
    }
    GlobalServerData::serverPrivateVariables.db_server->clear();
    if(obsoleteMap>0 && GlobalServerData::serverPrivateVariables.map_list.size()==0)
    {
        std::cerr << "Only obsolete map!" << std::endl;
        abort();
    }
    if(obsoleteMap>0)
        std::cerr << "/!\\ Obsolete map, can due to start previously start with another mainDatapackCode" << std::endl;
    std::vector<std::string> map_list_flat=unordered_map_keys_vector(GlobalServerData::serverPrivateVariables.map_list);
    std::sort(map_list_flat.begin(),map_list_flat.end());
    unsigned int index=0;
    while(index<map_list_flat.size())
    {
        const std::string &map=map_list_flat.at(index);
        if(foundMap.find(map)==foundMap.end())
        {
            databaseMapId=DictionaryServer::dictionary_map_database_to_internal.size()+1;
            std::string queryText;
            switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
            {
                default:
                abort();
                break;
                #if defined(CATCHCHALLENGER_DB_MYSQL) || (not defined(EPOLLCATCHCHALLENGERSERVER))
                case DatabaseBase::DatabaseType::Mysql:
                    queryText="INSERT INTO `dictionary_map`(`id`,`map`) VALUES("+std::to_string(databaseMapId)+",'"+map+"');";
                break;
                #endif
                #ifndef EPOLLCATCHCHALLENGERSERVER
                case DatabaseBase::DatabaseType::SQLite:
                    queryText="INSERT INTO dictionary_map(id,map) VALUES("+std::to_string(databaseMapId)+",'"+map+"');";
                break;
                #endif
                #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
                case DatabaseBase::DatabaseType::PostgreSQL:
                    queryText="INSERT INTO dictionary_map(id,map) VALUES("+std::to_string(databaseMapId)+",'"+map+"');";
                break;
                #endif
            }
            if(!GlobalServerData::serverPrivateVariables.db_server->asyncWrite(queryText))
            {
                std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
                criticalDatabaseQueryFailed();return;//stop because can't resolv the name
            }
            while(DictionaryServer::dictionary_map_database_to_internal.size()<=databaseMapId)
                DictionaryServer::dictionary_map_database_to_internal.push_back(NULL);
            DictionaryServer::dictionary_map_database_to_internal[databaseMapId]=static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list[map]);
            static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list[map])->reverse_db_id=databaseMapId;
        }
        index++;
    }

    if(obsoleteMap)
        std::cerr << obsoleteMap << " SQL obsolete map dictionary" << std::endl;
    std::cout << DictionaryServer::dictionary_map_database_to_internal.size() << " SQL map dictionary" << std::endl;

    preload_pointOnMap_item_sql();
}

void BaseServer::preload_industries()
{
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    std::cout << GlobalServerData::serverPrivateVariables.maxClanId << " SQL clan max id" << std::endl;
    #endif

    std::string queryText;
    switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
    {
        default:
        abort();
        break;
        #if defined(CATCHCHALLENGER_DB_MYSQL) || (not defined(EPOLLCATCHCHALLENGERSERVER))
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id`,`resources`,`products`,`last_update` FROM `factory`";
        break;
        #endif
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id,resources,products,last_update FROM factory";
        break;
        #endif
        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id,resources,products,last_update FROM factory";
        break;
        #endif
    }
    if(GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText,this,&BaseServer::preload_industries_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
        preload_finish();
    }
}

void BaseServer::preload_industries_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_industries_return();
}

void BaseServer::preload_industries_return()
{
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        IndustryStatus industryStatus;
        bool ok;
        uint32_t id=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
        if(!ok)
            std::cerr << "preload_industries: id is not a number" << std::endl;
        if(ok)
        {
            if(CommonDatapack::commonDatapack.industriesLink.find(id)==CommonDatapack::commonDatapack.industriesLink.end())
            {
                ok=false;
                std::cerr << "preload_industries: industries link not found" << std::endl;
            }
        }
        if(ok)
        {
            const std::vector<std::string> &resourcesStringList=stringsplit(GlobalServerData::serverPrivateVariables.db_server->value(1),';');
            unsigned int index=0;
            const unsigned int &listsize=resourcesStringList.size();
            while(index<listsize)
            {
                const std::vector<std::string> &itemStringList=stringsplit(resourcesStringList.at(index),':');
                if(itemStringList.size()!=2)
                {
                    std::cerr << "preload_industries: wrong entry count" << std::endl;
                    ok=false;
                    break;
                }
                const uint32_t &item=stringtouint32(itemStringList.at(0),&ok);
                if(!ok)
                {
                    std::cerr << "preload_industries: item is not a number" << std::endl;
                    break;
                }
                uint32_t quantity=stringtouint32(itemStringList.at(1),&ok);
                if(!ok)
                {
                    std::cerr << "preload_industries: quantity is not a number" << std::endl;
                    break;
                }
                if(industryStatus.resources.find(item)!=industryStatus.resources.end())
                {
                    std::cerr << "preload_industries: item already set" << std::endl;
                    ok=false;
                    break;
                }
                const Industry &industry=CommonDatapack::commonDatapack.industries.at(CommonDatapack::commonDatapack.industriesLink.at(id).industry);
                int indexItem=0;
                const int &resourceslistsize=industry.resources.size();
                while(indexItem<resourceslistsize)
                {
                    if(industry.resources.at(indexItem).item==item)
                        break;
                    indexItem++;
                }
                if(indexItem==resourceslistsize)
                {
                    std::cerr << "preload_industries: item in db not found" << std::endl;
                    ok=false;
                    break;
                }
                if(quantity>(industry.resources.at(indexItem).quantity*industry.cycletobefull))
                    quantity=industry.resources.at(indexItem).quantity*industry.cycletobefull;
                industryStatus.resources[item]=quantity;
                index++;
            }
        }
        if(ok)
        {
            const std::vector<std::string> &productsStringList=stringsplit(GlobalServerData::serverPrivateVariables.db_server->value(2),';');
            int index=0;
            const int &listsize=productsStringList.size();
            while(index<listsize)
            {
                const std::vector<std::string> &itemStringList=stringsplit(productsStringList.at(index),':');
                if(itemStringList.size()!=2)
                {
                    std::cerr << "preload_industries: wrong entry count" << std::endl;
                    ok=false;
                    break;
                }
                const uint32_t &item=stringtouint32(itemStringList.at(0),&ok);
                if(!ok)
                {
                    std::cerr << "preload_industries: item is not a number" << std::endl;
                    break;
                }
                uint32_t quantity=stringtouint32(itemStringList.at(1),&ok);
                if(!ok)
                {
                    std::cerr << "preload_industries: quantity is not a number" << std::endl;
                    break;
                }
                if(industryStatus.products.find(item)!=industryStatus.products.end())
                {
                    std::cerr << "preload_industries: item already set" << std::endl;
                    ok=false;
                    break;
                }
                const Industry &industry=CommonDatapack::commonDatapack.industries.at(CommonDatapack::commonDatapack.industriesLink.at(id).industry);
                int indexItem=0;
                const int &productlistsize=industry.products.size();
                while(indexItem<productlistsize)
                {
                    if(industry.products.at(indexItem).item==item)
                        break;
                    indexItem++;
                }
                if(indexItem==productlistsize)
                {
                    std::cerr << "preload_industries: item in db not found" << std::endl;
                    ok=false;
                    break;
                }
                if(quantity>(industry.products.at(indexItem).quantity*industry.cycletobefull))
                    quantity=industry.products.at(indexItem).quantity*industry.cycletobefull;
                industryStatus.products[item]=quantity;
                index++;
            }
        }
        if(ok)
        {
            industryStatus.last_update=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(3),&ok);
            if(!ok)
                std::cerr << "preload_industries: last_update is not a number" << std::endl;
        }
        if(ok)
            GlobalServerData::serverPrivateVariables.industriesStatus[id]=industryStatus;
    }
    std::cout << GlobalServerData::serverPrivateVariables.industriesStatus.size() << " SQL industries loaded" << std::endl;

    preload_finish();
}

//unique table due to linked datas like skills/buffers product need of id, to be accruate on max id
void BaseServer::preload_market_monsters_prices_sql()
{
    preload_market_monsters_prices_call=true;
    std::cout << GlobalServerData::serverPrivateVariables.industriesStatus.size() << " SQL industrie loaded" << std::endl;

    std::string queryText;
    switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
    {
        default:
        abort();
        break;
        #if defined(CATCHCHALLENGER_DB_MYSQL) || (not defined(EPOLLCATCHCHALLENGERSERVER))
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id`,`market_price` FROM `monster_market_price` ORDER BY `id`";
        break;
        #endif
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id,market_price FROM monster_market_price ORDER BY id";
        break;
        #endif
        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id,market_price FROM monster_market_price ORDER BY id";
        break;
        #endif
    }
    if(GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText,this,&BaseServer::preload_market_monsters_prices_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
        preload_market_monsters_sql();
    }
}

void BaseServer::preload_market_monsters_prices_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_market_monsters_prices_return();
}

void BaseServer::preload_market_monsters_prices_return()
{
    bool ok;
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        Monster_Semi_Market monsterSemi;
        monsterSemi.id=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
        if(!ok)
        {
            std::cerr << "monsterId: " << GlobalServerData::serverPrivateVariables.db_server->value(0) << " is not a number" << std::endl;
            continue;
        }
        monsterSemi.price=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(1),&ok);
        if(!ok)
        {
            std::cerr << "price: " << GlobalServerData::serverPrivateVariables.db_server->value(1) << " is not a number" << std::endl;
            continue;
        }
        //finish it
        if(ok)
            monsterSemiMarketList.push_back(monsterSemi);
    }

    preload_market_monsters_sql();
}

//unique table due to linked datas like skills/buffers product need of id, to be accruate on max id
void BaseServer::preload_market_monsters_sql()
{
    if(monsterSemiMarketList.size()==0)
    {
        if(!preload_market_monsters_prices_call)
        {
            std::cerr << "preload_market_monsters_prices never call" << std::endl;
            abort();
        }
        preload_market_items();
        return;
    }

    std::string queryText;

    switch(GlobalServerData::serverPrivateVariables.db_common->databaseType())
    {
        default:
        abort();
        break;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        std::cerr << "PreparedDBQuery: Unknown database type" << std::endl;
        #else
        std::cerr << "PreparedDBQuery: Unknown database type in epoll mode" << std::endl;
        #endif
        abort();
        return;
        #if defined(CATCHCHALLENGER_DB_MYSQL) || (not defined(EPOLLCATCHCHALLENGERSERVER))
        case DatabaseBase::DatabaseType::Mysql:
        if(CommonSettingsServer::commonSettingsServer.useSP)
            queryText="SELECT `id`,`character`,`place`,`hp`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`buffs`,`skills`,`skills_endurance`,`sp` FROM `monster` WHERE `id`=%1";
        else
            queryText="SELECT `id`,`character`,`place`,`hp`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`buffs`,`skills`,`skills_endurance` FROM `monster` WHERE `id`=%1";
        break;
        #endif

        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
        if(CommonSettingsServer::commonSettingsServer.useSP)
            queryText="SELECT id,character,place,hp,monster,level,xp,captured_with,gender,egg_step,character_origin,buffs,skills,skills_endurance,sp FROM monster WHERE id=%1";
        else
            queryText="SELECT id,character,place,hp,monster,level,xp,captured_with,gender,egg_step,character_origin,buffs,skills,skills_endurance FROM monster WHERE id=%1";
        break;
        #endif

        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
        case DatabaseBase::DatabaseType::PostgreSQL:
        if(CommonSettingsServer::commonSettingsServer.useSP)
            queryText="SELECT id,character,place,hp,monster,level,xp,captured_with,gender,egg_step,character_origin,buffs,skills,skills_endurance,sp FROM monster WHERE id=%1";//for market
        else
            queryText="SELECT id,character,place,hp,monster,level,xp,captured_with,gender,egg_step,character_origin,buffs,skills,skills_endurance FROM monster WHERE id=%1";//for market
        break;
        #endif
    }

    StringWithReplacement finalQuery(queryText);
    if(GlobalServerData::serverPrivateVariables.db_common->asyncRead(
                finalQuery.compose(
                    std::to_string(monsterSemiMarketList.at(0).id)
                    )
                ,this,&BaseServer::preload_market_monsters_static)==NULL)
    {
        monsterSemiMarketList.clear();
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        preload_market_items();
    }
}

void BaseServer::preload_market_monsters_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_market_monsters_return();
}

void BaseServer::preload_market_monsters_return()
{
    bool ok;
    if(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        MarketPlayerMonster marketPlayerMonster;
        const uint8_t &place=GlobalServerData::serverPrivateVariables.db_common->stringtouint8(GlobalServerData::serverPrivateVariables.db_common->value(2),&ok);
        if(ok && place==0x03)
        {
            PlayerMonster playerMonster=Client::loadMonsters_DatabaseReturn_to_PlayerMonster(ok);
            //finish it
            if(ok)
            {
                marketPlayerMonster.player=stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(1),&ok);
                if(!ok)
                    std::cerr << "For character no a number: " << GlobalServerData::serverPrivateVariables.db_common->value(1) << std::endl;
            }
            if(ok)
            {
                marketPlayerMonster.price=monsterSemiMarketList.at(0).price;
                marketPlayerMonster.monster=playerMonster;
                GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.push_back(marketPlayerMonster);
            }
        }
        else
            std::cerr << "For place is not a number or not into place===0x03" << std::endl;
    }
    else
        std::cerr << "For a monster, price found but not entry into common list" << std::endl;

    monsterSemiMarketList.erase(monsterSemiMarketList.begin());
    if(monsterSemiMarketList.size()>0)
    {
        preload_market_monsters_sql();
        return;
    }
    /*if(GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.size()>0)
        loadMonsterBuffs(0);
    else*/
        preload_market_items();
}

void BaseServer::preload_market_items()
{
    std::cout << GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.size() << " SQL monster list loaded" << std::endl;

    Client::marketObjectIdList.clear();
    Client::marketObjectIdList.reserve(65535);
    int index=0;
    while(index<=65535)
    {
        Client::marketObjectIdList.push_back(index);
        index++;
    }
    //do the query
    std::string queryText;
    switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
    {
        default:
        abort();
        break;
        #if defined(CATCHCHALLENGER_DB_MYSQL) || (not defined(EPOLLCATCHCHALLENGERSERVER))
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `item`,`quantity`,`character`,`market_price` FROM `item_market` ORDER BY `item`";
        break;
        #endif
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT item,quantity,character,market_price FROM item_market ORDER BY item";
        break;
        #endif
        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT item,quantity,character,market_price FROM item_market ORDER BY item";
        break;
        #endif
    }
    if(GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText,this,&BaseServer::preload_market_items_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        if(GlobalServerData::serverSettings.automatic_account_creation)
            load_account_max_id();
        else if(CommonSettingsCommon::commonSettingsCommon.max_character)
            load_character_max_id();
        else
        #endif
        baseServerMasterLoadDictionaryLoad();
    }
}

void BaseServer::baseServerMasterLoadDictionaryLoad()
{
#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    BaseServerMasterLoadDictionary::load(GlobalServerData::serverPrivateVariables.db_base);
#else
    preload_profile();
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    load_sql_monsters_max_id();
    #else
    preload_industries();
    #endif
#endif
}

void BaseServer::preload_market_items_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_market_items_return();
}

void BaseServer::preload_market_items_return()
{
    bool ok;
    //parse the result
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        MarketItem marketItem;
        marketItem.item=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
        if(!ok)
        {
            std::cerr << "item id is not a number, skip" << std::endl;
            continue;
        }
        marketItem.quantity=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(1),&ok);
        if(!ok)
        {
            std::cerr << "quantity is not a number, skip" << std::endl;
            continue;
        }
        marketItem.player=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(2),&ok);
        if(!ok)
        {
            std::cerr << "player id is not a number, skip" << std::endl;
            continue;
        }
        marketItem.price=stringtouint64(GlobalServerData::serverPrivateVariables.db_server->value(3),&ok);
        if(!ok)
        {
            std::cerr << "cash is not a number, skip" << std::endl;
            continue;
        }
        if(Client::marketObjectIdList.size()==0)
        {
            std::cerr << "not more marketObjectId into the list, skip" << std::endl;
            return;
        }
        marketItem.marketObjectUniqueId=Client::marketObjectIdList.at(0);
        Client::marketObjectIdList.erase(Client::marketObjectIdList.begin());
        GlobalServerData::serverPrivateVariables.marketItemList.push_back(marketItem);
    }
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    if(GlobalServerData::serverSettings.automatic_account_creation)
        load_account_max_id();
    else if(CommonSettingsCommon::commonSettingsCommon.max_character)
        load_character_max_id();
    else
    #endif
    baseServerMasterLoadDictionaryLoad();
}

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
void BaseServer::load_clan_max_id()
{
    //start to 0 due to pre incrementation before use
    GlobalServerData::serverPrivateVariables.maxClanId=1;
    std::string queryText;
    switch(GlobalServerData::serverPrivateVariables.db_common->databaseType())
    {
        default:
        abort();
        break;
        #if defined(CATCHCHALLENGER_DB_MYSQL) || (not defined(EPOLLCATCHCHALLENGERSERVER))
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id` FROM `clan` ORDER BY `id` DESC LIMIT 0,1;";
        break;
        #endif
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id FROM clan ORDER BY id DESC LIMIT 0,1;";
        break;
        #endif
        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id FROM clan ORDER BY id DESC LIMIT 1;";
        break;
        #endif
    }
    if(GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&BaseServer::load_clan_max_id_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        preload_industries();
    }
}

void BaseServer::load_clan_max_id_static(void *object)
{
    static_cast<BaseServer *>(object)->load_clan_max_id_return();
}

void BaseServer::load_clan_max_id_return()
{
    //start to 0 due to pre incrementation before use
    GlobalServerData::serverPrivateVariables.maxClanId=1;
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        bool ok;
        //not +1 because incremented before use
        GlobalServerData::serverPrivateVariables.maxClanId=stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok)+1;
        if(!ok)
        {
            std::cerr << "Max clan id is failed to convert to number" << std::endl;
            //start to 0 due to pre incrementation before use
            GlobalServerData::serverPrivateVariables.maxClanId=1;
            continue;
        }
    }
    preload_industries();
}

void BaseServer::load_account_max_id()
{
    std::string queryText;
    switch(GlobalServerData::serverPrivateVariables.db_login->databaseType())
    {
        default:
        abort();
        break;
        #if defined(CATCHCHALLENGER_DB_MYSQL) || (not defined(EPOLLCATCHCHALLENGERSERVER))
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id` FROM `account` ORDER BY `id` DESC LIMIT 0,1;";
        break;
        #endif
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id FROM account ORDER BY id DESC LIMIT 0,1;";
        break;
        #endif
        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id FROM account ORDER BY id DESC LIMIT 1;";
        break;
        #endif
    }
    if(GlobalServerData::serverPrivateVariables.db_login->asyncRead(queryText,this,&BaseServer::load_account_max_id_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_login->errorMessage() << std::endl;
        if(CommonSettingsCommon::commonSettingsCommon.max_character)
            load_character_max_id();
        else
            baseServerMasterLoadDictionaryLoad();
    }
    //start to 0 due to pre incrementation before use
    GlobalServerData::serverPrivateVariables.maxAccountId=0;
}

void BaseServer::load_account_max_id_static(void *object)
{
    static_cast<BaseServer *>(object)->load_account_max_id_return();
}

void BaseServer::load_account_max_id_return()
{
    while(GlobalServerData::serverPrivateVariables.db_login->next())
    {
        bool ok;
        //not +1 because incremented before use
        GlobalServerData::serverPrivateVariables.maxAccountId=stringtouint32(GlobalServerData::serverPrivateVariables.db_login->value(0),&ok);
        if(!ok)
        {
            std::cerr << "Max account id is failed to convert to number" << std::endl;
            //start to 0 due to pre incrementation before use
            GlobalServerData::serverPrivateVariables.maxAccountId=0;
            continue;
        }
    }
    if(CommonSettingsCommon::commonSettingsCommon.max_character)
        load_character_max_id();
    else
        baseServerMasterLoadDictionaryLoad();
}

void BaseServer::load_character_max_id()
{
    std::string queryText;
    switch(GlobalServerData::serverPrivateVariables.db_common->databaseType())
    {
        default:
        abort();
        break;
        #if defined(CATCHCHALLENGER_DB_MYSQL) || (not defined(EPOLLCATCHCHALLENGERSERVER))
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id` FROM `character` ORDER BY `id` DESC LIMIT 0,1;";
        break;
        #endif
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id FROM character ORDER BY id DESC LIMIT 0,1;";
        break;
        #endif
        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id FROM character ORDER BY id DESC LIMIT 1;";
        break;
        #endif
    }
    if(GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&BaseServer::load_character_max_id_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        baseServerMasterLoadDictionaryLoad();
    }
    //start to 0 due to pre incrementation before use
    GlobalServerData::serverPrivateVariables.maxCharacterId=0;
}

void BaseServer::load_character_max_id_static(void *object)
{
    static_cast<BaseServer *>(object)->load_character_max_id_return();
}

void BaseServer::load_character_max_id_return()
{
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        bool ok;
        //not +1 because incremented before use
        GlobalServerData::serverPrivateVariables.maxCharacterId=stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
        if(!ok)
        {
            std::cerr << "Max character id is failed to convert to number" << std::endl;
            //start to 0 due to pre incrementation before use
            GlobalServerData::serverPrivateVariables.maxCharacterId=0;
            continue;
        }
    }
    baseServerMasterLoadDictionaryLoad();
}
#endif
