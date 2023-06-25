#include "BaseServer.hpp"
#include "Client.hpp"
#include "GlobalServerData.hpp"
#include "DictionaryServer.hpp"
#ifdef CATCHCHALLENGER_CACHE_HPS
#include <fstream>
#endif

#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonDatapack.hpp"

using namespace CatchChallenger;

void BaseServer::preload_12_async_dictionary_map()
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    if(GlobalServerData::serverPrivateVariables.db_server==NULL)
    {
        std::cerr << "GlobalServerData::serverPrivateVariables.db_server==NULL" << std::endl;
        abort();
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
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
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
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
    else
        std::cout << "wait database dictionary_map query" << std::endl;
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    preload_dictionary_map_return();
    #elif CATCHCHALLENGER_DB_FILE
    preload_dictionary_map_return();
    #else
    #error Define what do here
    #endif
}

void BaseServer::preload_dictionary_map_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_dictionary_map_return();
}

void BaseServer::preload_dictionary_map_return()
{
    std::cout << "database dictionary_map query finish" << std::endl;
    if(DictionaryServer::dictionary_map_database_to_internal.size()>0)
    {
        std::cerr << "preload_dictionary_map_return() already call" << std::endl;
        abort();
    }
    std::unordered_set<std::string> foundMap;
    unsigned int maxDatabaseMapId=0;
    unsigned int obsoleteMap=0;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_DB_FILE)
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
        while(GlobalServerData::serverPrivateVariables.db_server->next())
        #else
        size_t s=0;
        *dictionary_serialBuffer >> s;
        for (size_t i = 0; i < s; i++)
        #endif
        {
            #ifdef CATCHCHALLENGER_DB_FILE
            uint32_t databaseMapId=0;
            *dictionary_serialBuffer >> databaseMapId;
            std::string map;
            *dictionary_serialBuffer >> map;
            #else
            bool ok;
            const uint32_t &databaseMapId=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
            if(!ok)
                std::cerr << "BaseServer::preload_dictionary_map_return() stringtouint32 wrong" << std::endl;
            const std::string &map=std::string(GlobalServerData::serverPrivateVariables.db_server->value(1));
            #endif
            if(databaseMapId>maxDatabaseMapId)
                maxDatabaseMapId=databaseMapId;
            if(DictionaryServer::dictionary_map_database_to_internal.size()<=databaseMapId)
            {
                unsigned int index=static_cast<uint32_t>(DictionaryServer::dictionary_map_database_to_internal.size());
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
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
        GlobalServerData::serverPrivateVariables.db_server->clear();
        #endif
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #else
    #error Define what do here
    #endif
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
            maxDatabaseMapId=static_cast<uint32_t>(DictionaryServer::dictionary_map_database_to_internal.size())+1;
            #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
            std::string queryText;
            switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
            {
                default:
                abort();
                break;
                #if defined(CATCHCHALLENGER_DB_MYSQL) || (not defined(EPOLLCATCHCHALLENGERSERVER))
                case DatabaseBase::DatabaseType::Mysql:
                    queryText="INSERT INTO `dictionary_map`(`id`,`map`) VALUES("+std::to_string(maxDatabaseMapId)+",'"+map+"');";
                break;
                #endif
                #ifndef EPOLLCATCHCHALLENGERSERVER
                case DatabaseBase::DatabaseType::SQLite:
                    queryText="INSERT INTO dictionary_map(id,map) VALUES("+std::to_string(maxDatabaseMapId)+",'"+map+"');";
                break;
                #endif
                #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
                case DatabaseBase::DatabaseType::PostgreSQL:
                    queryText="INSERT INTO dictionary_map(id,map) VALUES("+std::to_string(maxDatabaseMapId)+",'"+map+"');";
                break;
                #endif
            }
            if(!GlobalServerData::serverPrivateVariables.db_server->asyncWrite(queryText))
            {
                std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
                criticalDatabaseQueryFailed();return;//stop because can't resolv the name
            }
            #elif CATCHCHALLENGER_DB_BLACKHOLE
            #elif CATCHCHALLENGER_DB_FILE
            dictionary_haveChange=true;
            #else
            #error Define what do here
            #endif
            while(DictionaryServer::dictionary_map_database_to_internal.size()<=maxDatabaseMapId)
                DictionaryServer::dictionary_map_database_to_internal.push_back(NULL);
            DictionaryServer::dictionary_map_database_to_internal[maxDatabaseMapId]=static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list[map]);
            static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list[map])->reverse_db_id=maxDatabaseMapId;
        }
        index++;
    }

    if(obsoleteMap)
        std::cerr << obsoleteMap << " SQL obsolete map dictionary" << std::endl;
    std::cout << DictionaryServer::dictionary_map_database_to_internal.size() << " SQL map dictionary" << std::endl;

    preload_13_async_pointOnMap_item_sql();
}

void BaseServer::preload_21_async_industries()
{
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    std::cout << GlobalServerData::serverPrivateVariables.maxClanId << " SQL clan max id" << std::endl;
    #endif
    if(preload_industries_call!=false)
    {
        std::cerr << "preload_industries_call!=false, double call (abort)" << std::endl;
        abort();
    }
    preload_industries_call=true;

    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
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
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    preload_industries_return();
    #elif CATCHCHALLENGER_DB_FILE
    preload_industries_return();
    #else
    #error Define what do here
    #endif
}

void BaseServer::preload_industries_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_industries_return();
}

void BaseServer::preload_industries_return()
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_DB_FILE)
    #ifdef CATCHCHALLENGER_DB_FILE
    size_t s=0;
    *dictionary_serialBuffer >> s;
    for (size_t i = 0; i < s; i++)
    #else
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    #endif
    {
        IndustryStatus industryStatus;
        bool ok=true;
        #ifdef CATCHCHALLENGER_DB_FILE
        std::uint16_t id;
        *dictionary_serialBuffer >> s;
        #else
        uint16_t id=stringtouint16(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
        #endif
        if(!ok)
            std::cerr << "preload_industries: id is not a number" << std::endl;
        if(ok)
        {
            if(CommonDatapack::commonDatapack.get_industriesLink().find(id)==CommonDatapack::commonDatapack.get_industriesLink().end())
            {
                ok=false;
                std::cerr << "preload_industries: industries link not found" << std::endl;
            }
        }
        if(ok)
        {
            #ifdef CATCHCHALLENGER_DB_FILE
            std::vector<std::string> resourcesStringList;
            *dictionary_serialBuffer >> resourcesStringList;
            #else
            const std::vector<std::string> &resourcesStringList=stringsplit(GlobalServerData::serverPrivateVariables.db_server->value(1),';');
            #endif
            unsigned int index=0;
            while(index<resourcesStringList.size())
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
                const IndustryLink &industryLink=CommonDatapack::commonDatapack.get_industriesLink().at(id);
                const Industry &industry=CommonDatapack::commonDatapack.get_industries().at(
                            industryLink.industry
                            );
                unsigned int indexItem=0;
                while(indexItem<industry.resources.size())
                {
                    if(industry.resources.at(indexItem).item==item)
                        break;
                    indexItem++;
                }
                if(indexItem==industry.resources.size())
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
            #ifdef CATCHCHALLENGER_DB_FILE
            std::vector<std::string> productsStringList;
            *dictionary_serialBuffer >> productsStringList;
            #else
            const std::vector<std::string> &productsStringList=stringsplit(GlobalServerData::serverPrivateVariables.db_server->value(2),';');
            #endif
            unsigned int index=0;
            while(index<productsStringList.size())
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
                const IndustryLink &industryLink=CommonDatapack::commonDatapack.get_industriesLink().at(id);
                const Industry &industry=CommonDatapack::commonDatapack.get_industries().at(industryLink.industry);
                unsigned int indexItem=0;
                while(indexItem<industry.products.size())
                {
                    if(industry.products.at(indexItem).item==item)
                        break;
                    indexItem++;
                }
                if(indexItem==industry.products.size())
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
            #ifdef CATCHCHALLENGER_DB_FILE
            *dictionary_serialBuffer >> industryStatus.last_update;
            #else
            industryStatus.last_update=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(3),&ok);
            #endif
            if(!ok)
                std::cerr << "preload_industries: last_update is not a number" << std::endl;
        }
        if(ok)
            GlobalServerData::serverPrivateVariables.industriesStatus[id]=industryStatus;
    }
    std::cout << GlobalServerData::serverPrivateVariables.industriesStatus.size() << " SQL industries loaded" << std::endl;
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #else
    #error Define what do here
    #endif

    #ifdef CATCHCHALLENGER_CACHE_HPS
    if(out_file!=nullptr)
    {
        //save map content to cache
        uint32_t mapListSize=GlobalServerData::serverPrivateVariables.map_list.size();
        hps::to_stream(mapListSize, *out_file);

        std::unordered_map<const CommonMap *,std::string> map_list_reverse;
        for (const auto &x : GlobalServerData::serverPrivateVariables.map_list)
              map_list_reverse[x.second]=x.first;
        std::unordered_map<std::string,uint32_t> id_map_to_map_reverse;
        for (const auto &x : GlobalServerData::serverPrivateVariables.id_map_to_map)
              id_map_to_map_reverse[x.second]=x.first;
        uint32_t idSize=0;
        uint32_t pathSize=0;
        uint32_t mapSize=0;
        size_t lastSize=out_file->tellp();
        for(unsigned int i=0; i<mapListSize; i++)
        {
            const MapServer * const map=static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.flat_map_list[i]);
            const std::string &string=map_list_reverse.at(static_cast<const CommonMap *>(map));
            const uint32_t &id=id_map_to_map_reverse.at(string);

            //std::cerr << "map id " << id << " at " << out_file->tellp() << std::endl;

            hps::to_stream(id, *out_file);
            idSize+=((uint32_t)out_file->tellp()-(uint32_t)lastSize);lastSize=out_file->tellp();

            //std::cerr << "map string " << string << " at " << out_file->tellp() << std::endl;

            hps::to_stream(string, *out_file);
            pathSize+=((uint32_t)out_file->tellp()-(uint32_t)lastSize);lastSize=out_file->tellp();

            //std::cerr << "map at " << out_file->tellp() << " map->pointOnMap_Item.size(): " << std::to_string(map->pointOnMap_Item.size()) << std::endl;
            /*for (const auto& kv : map->pointOnMap_Item)
            {
                const MapServer::ItemOnMap &item=kv.second;
                //std::cerr << "Loaded map item: " << std::to_string(item.item) << " item.pointOnMapDbCode: " << std::to_string(item.pointOnMapDbCode) << " item.infinite: " << std::to_string(item.infinite) << std::endl;
            }*/

            hps::to_stream(*map, *out_file);
            mapSize+=((uint32_t)out_file->tellp()-(uint32_t)lastSize);lastSize=out_file->tellp();

            //std::cerr << "map end at " << out_file->tellp() << std::endl;
        }

        /*std::cout << "DictionaryServer::dictionary_pointOnMap_item_database_to_internal: " << DictionaryServer::dictionary_pointOnMap_item_database_to_internal.size() << std::endl;
        for(unsigned int i=0; i<DictionaryServer::dictionary_pointOnMap_item_database_to_internal.size(); i++)
        {
            const DictionaryServer::MapAndPointItem &t=DictionaryServer::dictionary_pointOnMap_item_database_to_internal.at(i);
            std::cerr << t.datapack_index_item << " " << t.map->id << " " << t.x << " " << t.y << " " << std::endl;
        }*/

        std::cout << "map id size: " << idSize << "B" << std::endl;
        std::cout << "map pathSize size: " << pathSize << "B" << std::endl;
        std::cout << "map size: " << mapSize << "B" << std::endl;

        uint32_t dbSize=0;
        lastSize=out_file->tellp();

        std::unordered_map<void *,int32_t> pointer_to_pos;
        pointer_to_pos[nullptr]=-1;
        for(int32_t i=0; i<(int32_t)GlobalServerData::serverPrivateVariables.map_list.size(); i++)
            pointer_to_pos[GlobalServerData::serverPrivateVariables.flat_map_list[i]]=i;

        //the player load well without this, is loaded by another way: std::vector<MapServer *> DictionaryServer::dictionary_map_database_to_internal;

        hps::to_stream((int32_t)DictionaryServer::dictionary_pointOnMap_item_database_to_internal.size(), *out_file);
        for(int32_t i=0; i<(int32_t)DictionaryServer::dictionary_pointOnMap_item_database_to_internal.size(); i++)
        {
            const DictionaryServer::MapAndPointItem &v=DictionaryServer::dictionary_pointOnMap_item_database_to_internal.at(i);
            hps::to_stream(v.datapack_index_item, *out_file);
            hps::to_stream(pointer_to_pos.at(v.map), *out_file);
            hps::to_stream(v.x, *out_file);
            hps::to_stream(v.y, *out_file);
        }
        hps::to_stream((int32_t)DictionaryServer::dictionary_pointOnMap_plant_database_to_internal.size(), *out_file);
        for(int32_t i=0; i<(int32_t)DictionaryServer::dictionary_pointOnMap_plant_database_to_internal.size(); i++)
        {
            const DictionaryServer::MapAndPointPlant &v=DictionaryServer::dictionary_pointOnMap_plant_database_to_internal.at(i);
            hps::to_stream(v.datapack_index_plant, *out_file);
            hps::to_stream(pointer_to_pos.at(v.map), *out_file);
            hps::to_stream(v.x, *out_file);
            hps::to_stream(v.y, *out_file);
        }

        dbSize+=((uint32_t)out_file->tellp()-(uint32_t)lastSize);lastSize=out_file->tellp();
        std::cout << "DictionaryServer Size: " << dbSize << "B" << std::endl;

        out_file->flush();
        out_file->close();
        delete out_file;
        ::rename((FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/datapack-cache.bin.tmp").c_str(),
                 (FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/datapack-cache.bin").c_str());
        ::exit(0);
        return;
    }
    if(serialBuffer!=nullptr)
        delete serialBuffer;
    if(in_file!=nullptr)
    {
        in_file->close();
        delete in_file;
    }
    #endif
    preload_finish();
}

void BaseServer::preload_17_async_baseServerMasterLoadDictionaryLoad()
{
#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    BaseServerMasterLoadDictionary::load(GlobalServerData::serverPrivateVariables.db_base);
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    preload_dictionary_reputation();
    #elif CATCHCHALLENGER_DB_FILE
    preload_dictionary_reputation();
    #else
    #error Define what do here
    #endif
#else
    preload_18_sync_profile();
    preload_21_async_industries();
#endif
}

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
void BaseServer::preload_20_async_clan_max_id()
{
    //start to 0 due to pre incrementation before use
    GlobalServerData::serverPrivateVariables.maxClanId=1;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
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
        preload_21_async_industries();
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    load_clan_max_id_return();
    #elif CATCHCHALLENGER_DB_FILE
    load_clan_max_id_return();
    #else
    #error Define what do here
    #endif
}

void BaseServer::load_clan_max_id_static(void *object)
{
    static_cast<BaseServer *>(object)->load_clan_max_id_return();
}

void BaseServer::load_clan_max_id_return()
{
    //start to 0 due to pre incrementation before use
    GlobalServerData::serverPrivateVariables.maxClanId=1;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
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
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    *server_serialBuffer >> GlobalServerData::serverPrivateVariables.maxClanId;
    #else
    #error Define what do here
    #endif
    preload_21_async_industries();
}

void BaseServer::load_account_max_id()
{
    //start to 0 due to pre incrementation before use
    GlobalServerData::serverPrivateVariables.maxAccountId=0;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
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
            preload_17_async_baseServerMasterLoadDictionaryLoad();
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    load_account_max_id_return();
    #elif CATCHCHALLENGER_DB_FILE
    load_account_max_id_return();
    #else
    #error Define what do here
    #endif
}

void BaseServer::load_account_max_id_static(void *object)
{
    static_cast<BaseServer *>(object)->load_account_max_id_return();
}

void BaseServer::load_account_max_id_return()
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
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
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    if(CommonSettingsCommon::commonSettingsCommon.max_character)
        load_character_max_id();
    else
        preload_17_async_baseServerMasterLoadDictionaryLoad();
}

void BaseServer::load_character_max_id()
{
    //start to 0 due to pre incrementation before use
    GlobalServerData::serverPrivateVariables.maxCharacterId=0;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
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
        preload_17_async_baseServerMasterLoadDictionaryLoad();
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    load_character_max_id_return();
    #elif CATCHCHALLENGER_DB_FILE
    load_character_max_id_return();
    #else
    #error Define what do here
    #endif
}

void BaseServer::load_character_max_id_static(void *object)
{
    static_cast<BaseServer *>(object)->load_character_max_id_return();
}

void BaseServer::load_character_max_id_return()
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
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
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    preload_17_async_baseServerMasterLoadDictionaryLoad();
}
#endif
