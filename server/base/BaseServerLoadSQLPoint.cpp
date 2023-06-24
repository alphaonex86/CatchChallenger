#include "BaseServer.hpp"
#include "GlobalServerData.hpp"
#include "DictionaryServer.hpp"
#include "Client.hpp"

#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonSettingsServer.hpp"

using namespace CatchChallenger;

void BaseServer::preload_pointOnMap_item_sql()
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
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
        preload_the_city_capture();
        preload_zone();

        const auto now=msFrom1970();
        std::cout << "Loaded the server static datapack into " << (now-timeDatapack) << "ms" << std::endl;
        timeDatapack=now;

        //start SQL load
        preload_pointOnMap_plant_sql();
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    preload_pointOnMap_item_return();
    #elif CATCHCHALLENGER_DB_FILE
    preload_pointOnMap_item_return();
    #else
    #error Define what do here
    #endif
}

void BaseServer::preload_pointOnMap_item_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_pointOnMap_item_return();
}

void BaseServer::preload_pointOnMap_item_return()
{
    /*DictionaryServer::MapAndPointItem nullvalue;
    nullvalue.datapack_index_item=0;
    nullvalue.map=nullptr;
    nullvalue.x=0;
    nullvalue.y=0;*/

    unsigned int itemCount=0;
    unsigned int itemValidatedCount=0;
    dictionary_pointOnMap_maxId_item=0;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_DB_FILE)
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    bool ok;
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    #else
    size_t s=0;
    *dictionary_serialBuffer >> s;
    for (size_t i = 0; i < s; i++)
    #endif
    {
        itemCount++;
        #ifndef CATCHCHALLENGER_DB_FILE
        const uint16_t &id=stringtouint16(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
        if(!ok)
            std::cerr << "preload_itemOnMap_return(): Id not found: " << GlobalServerData::serverPrivateVariables.db_server->value(0) << std::endl;
        else
        #else
        uint16_t id=0;
        *dictionary_serialBuffer >> id;
        #endif
        {
            if(dictionary_pointOnMap_maxId_item<id)
                dictionary_pointOnMap_maxId_item=id;//here to prevent later bug create problem with max id
            #ifndef CATCHCHALLENGER_DB_FILE
            const uint32_t &map_id=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(1),&ok);
            if(!ok)
                std::cerr << "preload_itemOnMap_return(): map id not number: " << GlobalServerData::serverPrivateVariables.db_server->value(1) << std::endl;
            else
            #else
            uint32_t map_id=0;
            *dictionary_serialBuffer >> map_id;
            #endif
            {
                if(map_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
                    std::cerr << "preload_itemOnMap_return(): map out of range: " << map_id << std::endl;
                else
                {
                    MapServer * const map_server=DictionaryServer::dictionary_map_database_to_internal.at(map_id);
                    if(map_server==NULL)
                        std::cerr << "preload_pointOnMap_item_return(): map == NULL for this id, map not found: " << map_id << std::endl;
                    else
                    {
                        #ifndef CATCHCHALLENGER_DB_FILE
                        const uint32_t &x=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(2),&ok);
                        if(!ok)
                            std::cerr << "preload_itemOnMap_return(): x not number: " << GlobalServerData::serverPrivateVariables.db_server->value(2) << std::endl;
                        else
                        #else
                        uint32_t x=0;
                        *dictionary_serialBuffer >> x;
                        #endif
                        {
                            if(x>255 || x>=map_server->width)
                                std::cerr << "preload_itemOnMap_return(): x out of range: " << x << ", for " << map_id << std::endl;
                            else
                            {
                                #ifndef CATCHCHALLENGER_DB_FILE
                                const uint32_t &y=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(3),&ok);
                                if(!ok)
                                    std::cerr << "preload_itemOnMap_return(): y not number: " << GlobalServerData::serverPrivateVariables.db_server->value(3) << ", for " << map_id << std::endl;
                                else
                                #else
                                uint32_t y=0;
                                *dictionary_serialBuffer >> y;
                                #endif
                                {
                                    if(y>255 || y>=map_server->height)
                                        std::cerr << "preload_itemOnMap_return(): y out of range: " << y << ", for " << map_id << std::endl;
                                    else
                                    {
                                        itemValidatedCount++;
                                        const auto &pair=std::pair<uint8_t,uint8_t>(x,y);
                                        //const std::string &map_file=map_server->map_file;

                                        ///used only at map loading, \see BaseServer::preload_the_map()
                                        if(map_server->pointOnMap_Item.find(pair)!=map_server->pointOnMap_Item.cend())
                                            map_server->pointOnMap_Item[pair].pointOnMapDbCode=id;
                                        if(map_server->plants.find(pair)!=map_server->plants.cend())
                                            map_server->plants[pair].pointOnMapDbCode=id;

                                        /* do after the datapack is loaded while((uint32_t)DictionaryServer::dictionary_pointOnMap_database_to_internal.size()<=id)
                                        DictionaryServer::dictionary_pointOnMap_database_to_internal[id]=mapAndPoint;*/

                                        //std::string,std::map<std::pair<uint8_t/*x*/,uint8_t/*y*/>,uint16_t/*db code*/,pairhash>
                                        #ifdef CATCHCHALLENGER_CACHE_HPS
                                        //if no new map (datapack cache)
                                        if(serialBuffer!=nullptr)
                                        {
                                        }
                                        else
                                        #endif
                                        {
                                            //post processing (temp var)
                                            //std::cerr << "DictionaryServer::dictionary_pointOnMap_item_internal_to_database[" << map_server->map_file << "," << map_server->id << "][" << std::to_string(x) << "," << std::to_string(y) << "]=" << id << std::endl;
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
    }
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverPrivateVariables.db_server->clear();
    #endif
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #else
    #error Define what do here
    #endif
    {
        #ifdef CATCHCHALLENGER_CACHE_HPS
        if(serialBuffer!=nullptr)
            std::cout << itemCount << "," << itemValidatedCount << " SQL plant parsed" << std::endl;
        else
        #endif
            std::cout << DictionaryServer::dictionary_pointOnMap_item_internal_to_database.size() << " SQL item on map dictionary" << std::endl;

        preload_the_visibility_algorithm();
        if(!preload_the_city_capture())
            return;
        if(!preload_zone())
            return;
        const auto now = msFrom1970();
        std::cout << "Loaded the server static datapack into " << (now-timeDatapack) << "ms" << std::endl;
        timeDatapack=now;

        //start SQL load
        preload_pointOnMap_plant_sql();
    }
}

void BaseServer::preload_pointOnMap_plant_sql()
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
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
        preload_the_city_capture();
        preload_zone();

        const auto now=msFrom1970();
        std::cout << "Loaded the server static datapack into " << (now-timeDatapack) << "ms" << std::endl;
        timeDatapack=now;

        //start SQL load
        preload_map_semi_after_db_id();
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    preload_pointOnMap_plant_return();
    #elif CATCHCHALLENGER_DB_FILE
    preload_pointOnMap_plant_return();
    #else
    #error Define what do here
    #endif
}

void BaseServer::preload_pointOnMap_plant_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_pointOnMap_plant_return();
}

void BaseServer::preload_pointOnMap_plant_return()
{
    #ifdef CATCHCHALLENGER_CACHE_HPS
    DictionaryServer::MapAndPointPlant nullvalue;
    nullvalue.datapack_index_plant=0;
    nullvalue.map=nullptr;
    nullvalue.x=0;
    nullvalue.y=0;
    uint16_t datapack_index_temp_for_plant=0;
    #endif

    unsigned int plantCount=0;
    unsigned int plantValidatedCount=0;
    bool ok;
    dictionary_pointOnMap_maxId_plant=0;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_DB_FILE)
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    #else
    size_t s=0;
    *dictionary_serialBuffer >> s;
    for (size_t i = 0; i < s; i++)
    #endif
    {
        plantCount++;
        #ifndef CATCHCHALLENGER_DB_FILE
        const uint16_t &id=stringtouint16(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
        if(!ok)
            std::cerr << "preload_pointOnMap_plant_return(): Id not found: " << GlobalServerData::serverPrivateVariables.db_server->value(0) << std::endl;
        else
        #else
        uint16_t id=0;
        *dictionary_serialBuffer >> id;
        #endif
        {
            if(dictionary_pointOnMap_maxId_plant<id)
                dictionary_pointOnMap_maxId_plant=id;//here to prevent later bug create problem with max id
            #ifndef CATCHCHALLENGER_DB_FILE
            const uint32_t &map_id=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(1),&ok);
            if(!ok)
                std::cerr << "preload_pointOnMap_plant_return(): map id not number: " << GlobalServerData::serverPrivateVariables.db_server->value(1) << std::endl;
            else
            #else
            uint32_t map_id=0;
            *dictionary_serialBuffer >> map_id;
            #endif
            {
                if(map_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
                    std::cerr << "preload_pointOnMap_plant_return(): map out of range: " << map_id << std::endl;
                else
                {
                    MapServer * const map_server=DictionaryServer::dictionary_map_database_to_internal.at(map_id);
                    if(map_server==NULL)
                        std::cerr << "preload_pointOnMap_plant_return(): map == NULL for this id, map not found: " << map_id << std::endl;
                    else
                    {
                        #ifndef CATCHCHALLENGER_DB_FILE
                        const uint32_t &x=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(2),&ok);
                        if(!ok)
                            std::cerr << "preload_itemOnMap_return(): x not number: " << GlobalServerData::serverPrivateVariables.db_server->value(2) << std::endl;
                        else
                        #else
                        uint32_t x=0;
                        *dictionary_serialBuffer >> x;
                        #endif
                        {
                            if(x>255 || x>=map_server->width)
                                std::cerr << "preload_pointOnMap_plant_return(): x out of range: " << x << ", for " << map_id << std::endl;
                            else
                            {
                                #ifndef CATCHCHALLENGER_DB_FILE
                                const uint32_t &y=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(3),&ok);
                                if(!ok)
                                    std::cerr << "preload_pointOnMap_plant_return(): y not number: " << GlobalServerData::serverPrivateVariables.db_server->value(3) << ", for " << map_id << std::endl;
                                else
                                #else
                                uint32_t y=0;
                                *dictionary_serialBuffer >> y;
                                #endif
                                {
                                    if(y>255 || y>=map_server->height)
                                        std::cerr << "preload_pointOnMap_plant_return(): y out of range: " << y << ", for " << map_id << std::endl;
                                    else
                                    {
                                        plantValidatedCount++;
                                        const auto &pair=std::pair<uint8_t,uint8_t>(x,y);
                                        //const std::string &map_file=map_server->map_file;

                                        ///used only at map loading, \see BaseServer::preload_the_map()
                                        if(map_server->pointOnMap_Item.find(pair)!=map_server->pointOnMap_Item.cend())
                                            map_server->pointOnMap_Item[pair].pointOnMapDbCode=id;
                                        if(map_server->plants.find(pair)!=map_server->plants.cend())
                                            map_server->plants[pair].pointOnMapDbCode=id;

                                        /* do after the datapack is loaded while((uint32_t)DictionaryServer::dictionary_pointOnMap_database_to_internal.size()<=id)
                                        DictionaryServer::dictionary_pointOnMap_database_to_internal[id]=mapAndPoint;*/

                                        //std::string,std::map<std::pair<uint8_t/*x*/,uint8_t/*y*/>,uint16_t/*db code*/,pairhash>

                                        //if no new map (datapack cache)
                                        #ifdef CATCHCHALLENGER_CACHE_HPS
                                        if(serialBuffer!=nullptr)
                                        {
                                            //directly store
                                            map_server->plants[pair].pointOnMapDbCode=id;

                                            while(DictionaryServer::dictionary_pointOnMap_plant_database_to_internal.size()<=id)
                                                DictionaryServer::dictionary_pointOnMap_plant_database_to_internal.push_back(nullvalue);

                                            DictionaryServer::MapAndPointPlant value;
                                            value.datapack_index_plant=datapack_index_temp_for_plant;
                                            value.map=map_server;
                                            value.x=x;
                                            value.y=y;
                                            DictionaryServer::dictionary_pointOnMap_plant_database_to_internal[id]=value;
                                            datapack_index_temp_for_plant++;
                                        }
                                        else
                                        #endif
                                        //post processing (temporary)
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
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverPrivateVariables.db_server->clear();
    #endif
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #else
    #error Define what do here
    #endif
    {
        #ifdef CATCHCHALLENGER_CACHE_HPS
        if(serialBuffer!=nullptr)
            std::cout << plantCount << "," << plantValidatedCount << " SQL plant parsed" << std::endl;
        else
        #endif
            std::cout << DictionaryServer::dictionary_pointOnMap_plant_internal_to_database.size() << " SQL plant on map dictionary" << std::endl;

        preload_the_visibility_algorithm();
        if(!preload_the_city_capture())
            return;
        if(!preload_zone())
            return;
        const auto now = msFrom1970();
        std::cout << "Loaded the server static datapack into " << (now-timeDatapack) << "ms" << std::endl;
        timeDatapack=now;

        //start SQL load
        preload_map_semi_after_db_id();
    }
}
