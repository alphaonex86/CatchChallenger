#include "BaseServer.hpp"
#include "GlobalServerData.hpp"
#include "DictionaryServer.hpp"

#include "../../general/base/CommonSettingsCommon.hpp"

using namespace CatchChallenger;

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
    /*DictionaryServer::MapAndPointItem nullvalue;
    nullvalue.datapack_index_item=0;
    nullvalue.map=nullptr;
    nullvalue.x=0;
    nullvalue.y=0;*/

    unsigned int itemCount=0;
    unsigned int itemValidatedCount=0;
    bool ok;
    dictionary_pointOnMap_maxId_item=0;
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        itemCount++;
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
                        std::cerr << "preload_pointOnMap_item_return(): map == NULL for this id, map not found: " << map_id << std::endl;
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

    GlobalServerData::serverPrivateVariables.db_server->clear();
    {
        #ifdef CATCHCHALLENGER_CACHE_HPS
        if(serialBuffer!=nullptr)
            std::cout << itemCount << "," << itemValidatedCount << " SQL plant parsed" << std::endl;
        else
        #endif
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
    #ifdef CATCHCHALLENGER_CACHE_HPS
    DictionaryServer::MapAndPointPlant nullvalue;
    nullvalue.datapack_index_plant=0;
    nullvalue.map=nullptr;
    nullvalue.x=0;
    nullvalue.y=0;
    #endif
    uint16_t datapack_index_temp_for_plant=0;

    unsigned int plantCount=0;
    unsigned int plantValidatedCount=0;
    bool ok;
    dictionary_pointOnMap_maxId_plant=0;
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        plantCount++;
        const uint16_t &id=stringtouint16(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
        if(!ok)
            std::cerr << "preload_pointOnMap_plant_return(): Id not found: " << GlobalServerData::serverPrivateVariables.db_server->value(0) << std::endl;
        else
        {
            if(dictionary_pointOnMap_maxId_plant<id)
                dictionary_pointOnMap_maxId_plant=id;//here to prevent later bug create problem with max id
            const uint32_t &map_id=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(1),&ok);
            if(!ok)
                std::cerr << "preload_pointOnMap_plant_return(): map id not number: " << GlobalServerData::serverPrivateVariables.db_server->value(1) << std::endl;
            else
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
                        const uint32_t &x=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(2),&ok);
                        if(!ok)
                            std::cerr << "preload_itemOnMap_return(): x not number: " << GlobalServerData::serverPrivateVariables.db_server->value(2) << std::endl;
                        else
                        {
                            if(x>255 || x>=map_server->width)
                                std::cerr << "preload_pointOnMap_plant_return(): x out of range: " << x << ", for " << map_id << std::endl;
                            else
                            {
                                const uint32_t &y=stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(3),&ok);
                                if(!ok)
                                    std::cerr << "preload_pointOnMap_plant_return(): y not number: " << GlobalServerData::serverPrivateVariables.db_server->value(3) << ", for " << map_id << std::endl;
                                else
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
    GlobalServerData::serverPrivateVariables.db_server->clear();
    {
        #ifdef CATCHCHALLENGER_CACHE_HPS
        if(serialBuffer!=nullptr)
            std::cout << plantCount << "," << plantValidatedCount << " SQL plant parsed" << std::endl;
        else
        #endif
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
    Monster_Semi_Market monster_Semi_Market=monsterSemiMarketList.at(0);
    if(GlobalServerData::serverPrivateVariables.db_common->asyncRead(
                finalQuery.compose({
                    std::to_string(monster_Semi_Market.id)
                    })
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
    if(preload_market_items_call!=false)
    {
        std::cerr << "BaseServer::preload_market_items() double call detected" << std::endl;
        abort();
    }
    preload_market_items_call=true;
    std::cout << GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.size() << " SQL monster list loaded" << std::endl;

    /*lot of memory Client::marketObjectUniqueIdList.clear();
    Client::marketObjectUniqueIdList.reserve(65535);
    int index=0;
    while(index<=65535)
    {
        Client::marketObjectUniqueIdList.push_back(static_cast<uint16_t>(index));
        index++;
    }*/
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
