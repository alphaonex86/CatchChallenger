#include "BaseServer.h"
#include "GlobalServerData.h"
#include "DictionaryServer.h"

#include "../../general/base/CommonSettingsCommon.h"

using namespace CatchChallenger;

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
    else
        std::cout << "wait database dictionary_map query" << std::endl;
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
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        bool ok;
        const uint32_t &databaseMapId=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
        if(databaseMapId>maxDatabaseMapId)
            maxDatabaseMapId=databaseMapId;
        const std::string &map=std::string(GlobalServerData::serverPrivateVariables.db_server->value(1));
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
            maxDatabaseMapId=static_cast<uint32_t>(DictionaryServer::dictionary_map_database_to_internal.size())+1;
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

    preload_pointOnMap_item_sql();
}

void BaseServer::preload_industries()
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
        uint16_t id=stringtouint16(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
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
                const IndustryLink &industryLink=CommonDatapack::commonDatapack.industriesLink.at(id);
                const Industry &industry=CommonDatapack::commonDatapack.industries.at(
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
            const std::vector<std::string> &productsStringList=stringsplit(GlobalServerData::serverPrivateVariables.db_server->value(2),';');
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
                const IndustryLink &industryLink=CommonDatapack::commonDatapack.industriesLink.at(id);
                const Industry &industry=CommonDatapack::commonDatapack.industries.at(industryLink.industry);
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
            industryStatus.last_update=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(3),&ok);
            if(!ok)
                std::cerr << "preload_industries: last_update is not a number" << std::endl;
        }
        if(ok)
            GlobalServerData::serverPrivateVariables.industriesStatus[id]=industryStatus;
    }
    std::cout << GlobalServerData::serverPrivateVariables.industriesStatus.size() << " SQL industries loaded" << std::endl;

    #ifdef CATCHCHALLENGER_CACHE_HPS
    if(out_file!=nullptr)
    {
        ::exit(0);
        return;
    }
    #endif
    preload_finish();
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
        marketItem.item=stringtouint16(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
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
        if(Client::marketObjectUniqueIdList.size()==0)
        {
            std::cerr << "not more marketObjectId into the list, skip" << std::endl;
            return;
        }
        marketItem.marketObjectUniqueId=Client::marketObjectUniqueIdList.at(0);
        Client::marketObjectUniqueIdList.erase(Client::marketObjectUniqueIdList.begin());
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
