#include "BaseServer.h"
#include "GlobalServerData.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "../../general/base/DatapackGeneralLoader.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_None.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_Simple_StoreOnSender.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_WithBorder_StoreOnSender.h"
#include "ClientMapManagement/Map_server_MapVisibility_Simple_StoreOnSender.h"
#include "ClientMapManagement/Map_server_MapVisibility_WithBorder_StoreOnSender.h"
#include "LocalClientHandlerWithoutSender.h"
#include "ClientNetworkReadWithoutSender.h"
#include "SqlFunction.h"
#include "DictionaryServer.h"
#include "DictionaryLogin.h"
#include "PreparedDBQuery.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/cpp11addition.h"

#include <vector>
#include <time.h>
#include <iostream>
#include <chrono>
#ifndef EPOLLCATCHCHALLENGERSERVER
#include <QFile>
#include <QTimer>
#include <QDateTime>
#include <QTime>
#include <QCryptographicHash>
#endif

using namespace CatchChallenger;

void BaseServer::preload_zone_sql()
{
    const int &listsize=entryListZone.size();
    while(entryListIndex<listsize)
    {
        std::string zoneCodeName=entryListZone.at(entryListIndex).name;
        stringreplaceOne(zoneCodeName,BaseServer::text_dotxml,"");
        std::string queryText;
        switch(GlobalServerData::serverPrivateVariables.db_common->databaseType())
        {
            default:
            case DatabaseBase::DatabaseType::Mysql:
                queryText="SELECT `clan` FROM `city` WHERE `city`='"+zoneCodeName+"'";//ORDER BY city-> drop, unique key
            break;
            case DatabaseBase::DatabaseType::SQLite:
                queryText="SELECT clan FROM city WHERE city='"+zoneCodeName+"'";//ORDER BY city-> drop, unique key
            break;
            case DatabaseBase::DatabaseType::PostgreSQL:
                queryText="SELECT clan FROM city WHERE city='"+zoneCodeName+"'";//ORDER BY city-> drop, unique key
            break;
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

void BaseServer::preload_pointOnMap_sql()
{
    std::string queryText;
    switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
    {
        default:
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id`,`map`,`x`,`y` FROM `dictionary_pointonmap` ORDER BY `map`,`x`,`y`";
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id,map,x,y FROM dictionary_pointonmap ORDER BY map,x,y";
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id,map,x,y FROM dictionary_pointonmap ORDER BY map,x,y";
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText,this,&BaseServer::preload_pointOnMap_static)==NULL)
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
}

void BaseServer::preload_pointOnMap_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_pointOnMap_return();
}

void BaseServer::preload_pointOnMap_return()
{
    bool ok;
    dictionary_pointOnMap_maxId=0;
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        const uint16_t &id=stringtouint16(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
        if(!ok)
            std::cerr << "preload_itemOnMap_return(): Id not found: " << GlobalServerData::serverPrivateVariables.db_server->value(0) << std::endl;
        else
        {
            if(dictionary_pointOnMap_maxId<id)
                dictionary_pointOnMap_maxId=id;//here to prevent later bug create problem with max id
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

                                        while((uint32_t)DictionaryServer::dictionary_pointOnMap_database_to_internal.size()<=id)
                                        {
                                            DictionaryServer::MapAndPoint mapAndPoint;
                                            mapAndPoint.map=NULL;
                                            mapAndPoint.x=0;
                                            mapAndPoint.y=0;
                                            DictionaryServer::dictionary_pointOnMap_database_to_internal.push_back(mapAndPoint);
                                        }

                                        DictionaryServer::MapAndPoint mapAndPoint;
                                        mapAndPoint.map=map_server;
                                        mapAndPoint.x=x;
                                        mapAndPoint.y=y;
                                        DictionaryServer::dictionary_pointOnMap_database_to_internal[id]=mapAndPoint;

                                        //std::string,std::map<std::pair<uint8_t/*x*/,uint8_t/*y*/>,uint16_t/*db code*/,pairhash>
                                        DictionaryServer::dictionary_pointOnMap_internal_to_database[map_server->map_file][pair]=id;
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
        std::cout << DictionaryServer::dictionary_pointOnMap_internal_to_database.size() << " SQL item on map dictionary" << std::endl;

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
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id`,`map` FROM `dictionary_map` ORDER BY `map`";
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id,map FROM dictionary_map ORDER BY map";
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id,map FROM dictionary_map ORDER BY map";
        break;
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
            databaseMapId++;
            std::string queryText;
            switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
            {
                default:
                case DatabaseBase::DatabaseType::Mysql:
                    queryText="INSERT INTO `dictionary_map`(`id`,`map`) VALUES("+std::to_string(databaseMapId)+",'"+map+"');";
                break;
                case DatabaseBase::DatabaseType::SQLite:
                    queryText="INSERT INTO dictionary_map(id,map) VALUES("+std::to_string(databaseMapId)+",'"+map+"');";
                break;
                case DatabaseBase::DatabaseType::PostgreSQL:
                    queryText="INSERT INTO dictionary_map(id,map) VALUES("+std::to_string(databaseMapId)+",'"+map+"');";
                break;
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

    preload_pointOnMap_sql();
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
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id`,`resources`,`products`,`last_update` FROM `factory`";
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id,resources,products,last_update FROM factory";
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id,resources,products,last_update FROM factory";
        break;
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
    std::cout << GlobalServerData::serverPrivateVariables.industriesStatus.size() << " SQL industrie loaded" << std::endl;

    std::string queryText;
    switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
    {
        default:
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id`,`market_price` FROM `monster_market_price` ORDER BY `id`";
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id,market_price FROM monster_market_price ORDER BY id";
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id,market_price FROM monster_market_price ORDER BY id";
        break;
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
        preload_market_items();
        return;
    }

    std::string queryText;
    if(CommonSettingsServer::commonSettingsServer.useSP)
        switch(GlobalServerData::serverPrivateVariables.db_common->databaseType())
        {
            default:
            case DatabaseBase::DatabaseType::Mysql:
                queryText="SELECT `id`,`hp`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character` FROM `monster` WHERE `id`="+std::to_string(monsterSemiMarketList.at(0).id);
            break;
            case DatabaseBase::DatabaseType::SQLite:
                queryText="SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character FROM monster WHERE id="+std::to_string(monsterSemiMarketList.at(0).id);
            break;
            case DatabaseBase::DatabaseType::PostgreSQL:
                queryText="SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character FROM monster WHERE id="+std::to_string(monsterSemiMarketList.at(0).id);
            break;
        }
    else
        switch(GlobalServerData::serverPrivateVariables.db_common->databaseType())
        {
            default:
            case DatabaseBase::DatabaseType::Mysql:
                queryText="SELECT `id`,`hp`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character` FROM `monster` WHERE `id`="+std::to_string(monsterSemiMarketList.at(0).id);
            break;
            case DatabaseBase::DatabaseType::SQLite:
                queryText="SELECT id,hp,monster,level,xp,captured_with,gender,egg_step,character FROM monster WHERE id="+std::to_string(monsterSemiMarketList.at(0).id);
            break;
            case DatabaseBase::DatabaseType::PostgreSQL:
                queryText="SELECT id,hp,monster,level,xp,captured_with,gender,egg_step,character FROM monster WHERE id="+std::to_string(monsterSemiMarketList.at(0).id);
            break;
        }
    if(GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&BaseServer::preload_market_monsters_static)==NULL)
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
    uint8_t spOffset;
    if(CommonSettingsServer::commonSettingsServer.useSP)
        spOffset=0;
    else
        spOffset=1;
    bool ok;
    if(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        MarketPlayerMonster marketPlayerMonster;
        PlayerMonster playerMonster;
        playerMonster.id=stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
        if(!ok)
            std::cerr << "monsterId: " << GlobalServerData::serverPrivateVariables.db_common->value(0) << " is not a number" << std::endl;
        if(ok)
        {
            playerMonster.monster=stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(2),&ok);
            if(ok)
            {
                if(CommonDatapack::commonDatapack.monsters.find(playerMonster.monster)==CommonDatapack::commonDatapack.monsters.end())
                {
                    ok=false;
                    std::cerr << "monster: " << playerMonster.monster << " is not into monster list" << std::endl;
                }
            }
            else
                std::cerr << "monster: " << GlobalServerData::serverPrivateVariables.db_common->value(2) << " is not a number" << std::endl;
        }
        if(ok)
        {
            playerMonster.level=stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(3),&ok);
            if(ok)
            {
                if(playerMonster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                {
                    std::cerr << "level: " << playerMonster.level << " greater than " << CATCHCHALLENGER_MONSTER_LEVEL_MAX << ", truncated" << std::endl;
                    playerMonster.level=CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                }
            }
            else
                std::cerr << "level: " << GlobalServerData::serverPrivateVariables.db_common->value(3) << " is not a number" << std::endl;
        }
        if(ok)
        {
            playerMonster.remaining_xp=stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(4),&ok);
            if(ok)
            {
                if(playerMonster.remaining_xp>CommonDatapack::commonDatapack.monsters.at(playerMonster.monster).level_to_xp.at(playerMonster.level-1))
                {
                    std::cerr << "monster xp: " << playerMonster.remaining_xp << " greater than " << CommonDatapack::commonDatapack.monsters.at(playerMonster.monster).level_to_xp.at(playerMonster.level-1) << ", truncated" << std::endl;
                    playerMonster.remaining_xp=0;
                }
            }
            else
                std::cerr << "monster xp: " << GlobalServerData::serverPrivateVariables.db_common->value(4) << " is not a number" << std::endl;
        }
        if(CommonSettingsServer::commonSettingsServer.useSP)
        {
            if(ok)
            {
                playerMonster.sp=stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(5),&ok);
                if(!ok)
                    std::cerr << "monster sp: " << GlobalServerData::serverPrivateVariables.db_common->value(5) << " is not a number" << std::endl;
            }
        }
        else
            playerMonster.sp=0;
        if(ok)
        {
            playerMonster.catched_with=stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(6-spOffset),&ok);
            if(ok)
            {
                if(CommonDatapack::commonDatapack.items.item.find(playerMonster.catched_with)==CommonDatapack::commonDatapack.items.item.end())
                    std::cerr << "captured_with: " << playerMonster.catched_with << " is not is not into items list" << std::endl;
            }
            else
                std::cerr << "captured_with: " << GlobalServerData::serverPrivateVariables.db_common->value(6-spOffset) << " is not a number" << std::endl;
        }
        if(ok)
        {
            const uint32_t &value=stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(7-spOffset),&ok);
            if(ok)
            {
                if(value>=1 && value<=3)
                    playerMonster.gender=static_cast<Gender>(value);
                else
                {
                    playerMonster.gender=Gender_Unknown;
                    std::cerr << "unknown monster gender: " << value << std::endl;
                    ok=false;
                }
            }
            else
            {
                playerMonster.gender=Gender_Unknown;
                std::cerr << "unknown monster gender: " << GlobalServerData::serverPrivateVariables.db_common->value(7-spOffset) << std::endl;
                ok=false;
            }
        }
        if(ok)
        {
            playerMonster.egg_step=stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(8-spOffset),&ok);
            if(!ok)
                std::cerr << "monster egg_step: " << GlobalServerData::serverPrivateVariables.db_common->value(8-spOffset) << " is not a number" << std::endl;
        }
        if(ok)
            marketPlayerMonster.player=stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(9-spOffset),&ok);
        if(ok)
            marketPlayerMonster.cash=monsterSemiMarketList.at(0).price;
        //stats
        if(ok)
        {
            playerMonster.hp=stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(1),&ok);
            if(ok)
            {
                const Monster::Stat &stat=CommonFightEngine::getStat(CommonDatapack::commonDatapack.monsters.at(playerMonster.monster),playerMonster.level);
                if(playerMonster.hp>stat.hp)
                {
                    std::cerr << "monster hp: "
                    << playerMonster.hp
                    << " greater than max hp "
                    << stat.hp
                    << " for the level "
                    << playerMonster.level
                    << " of the monster "
                    << playerMonster.monster
                    << " , truncated";
                    playerMonster.hp=stat.hp;
                }
            }
            else
                std::cerr << "monster hp: " << GlobalServerData::serverPrivateVariables.db_common->value(1) << " is not a number" << std::endl;
        }
        //finish it
        if(ok)
        {
            marketPlayerMonster.monster=playerMonster;
            GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.push_back(marketPlayerMonster);
        }
    }
    monsterSemiMarketList.erase(monsterSemiMarketList.begin());
    if(monsterSemiMarketList.size()>0)
    {
        preload_market_monsters_sql();
        return;
    }
    if(GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.size()>0)
        loadMonsterBuffs(0);
    else
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
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `item`,`quantity`,`character`,`market_price` FROM `item_market` ORDER BY `item`";
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT item,quantity,character,market_price FROM item_market ORDER BY item";
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT item,quantity,character,market_price FROM item_market ORDER BY item";
        break;
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
        marketItem.cash=stringtouint64(GlobalServerData::serverPrivateVariables.db_server->value(3),&ok);
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

void BaseServer::loadMonsterBuffs(const uint32_t &index)
{
    entryListIndex=index;
    if(index>=(uint32_t)GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.size())
    {
        loadMonsterSkills(0);
        return;
    }
    std::string queryText;
    switch(GlobalServerData::serverPrivateVariables.db_common->databaseType())
    {
        default:
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `buff`,`level` FROM `monster_buff` WHERE `monster`="+std::to_string(index)+" ORDER BY `buff`";
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT buff,level FROM monster_buff WHERE monster="+std::to_string(index)+" ORDER BY buff";
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT buff,level FROM monster_buff WHERE monster="+std::to_string(index)+" ORDER BY buff";
        break;
    }

    if(GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&BaseServer::loadMonsterBuffs_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        loadMonsterSkills(0);
    }
}

void BaseServer::loadMonsterBuffs_static(void *object)
{
    static_cast<BaseServer *>(object)->loadMonsterBuffs_return();
}

void BaseServer::loadMonsterBuffs_return()
{
    const uint32_t &monsterId=GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.at(entryListIndex).monster.id;
    std::vector<PlayerBuff> buffs;
    bool ok;
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        PlayerBuff buff;
        buff.buff=stringtouint8(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
        if(ok)
        {
            if(CommonDatapack::commonDatapack.monsterBuffs.find(buff.buff)==CommonDatapack::commonDatapack.monsterBuffs.end())
            {
                ok=false;
                std::cerr << "buff " << buff.buff << " for monsterId: " << monsterId << " is not found into buff list" << std::endl;
            }
        }
        else
            std::cerr << "buff id: " << GlobalServerData::serverPrivateVariables.db_common->value(0) << " is not a number" << std::endl;
        if(ok)
        {
            buff.level=stringtouint8(GlobalServerData::serverPrivateVariables.db_common->value(1),&ok);
            if(ok)
            {
                if(buff.level<=0 || buff.level>CommonDatapack::commonDatapack.monsterBuffs.at(buff.buff).level.size())
                {
                    ok=false;
                    std::cerr << "buff " << buff.buff << " for monsterId: " << monsterId << " have not the level: " << buff.level << std::endl;
                }
            }
            else
                std::cerr << "buff level: " << GlobalServerData::serverPrivateVariables.db_common->value(2) << " is not a number" << std::endl;
        }
        if(ok)
        {
            if(CommonDatapack::commonDatapack.monsterBuffs.at(buff.buff).level.at(buff.level-1).duration!=Buff::Duration_Always)
            {
                ok=false;
                std::cerr << "buff " << buff.buff << " for monsterId: " << monsterId << " can't be loaded from the db if is not permanent" << std::endl;
            }
        }
        if(ok)
            buffs.push_back(buff);
    }
    loadMonsterBuffs(entryListIndex+1);
}

void BaseServer::loadMonsterSkills(const uint32_t &index)
{
    entryListIndex=index;
    if(index>=(uint32_t)GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.size())
    {
        preload_market_items();
        return;
    }
    std::string queryText;
    switch(GlobalServerData::serverPrivateVariables.db_common->databaseType())
    {
        default:
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `skill`,`level`,`endurance` FROM `monster_skill` WHERE `monster`="+std::to_string(index)+" ORDER BY `skill`";
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT skill,level,endurance FROM monster_skill WHERE monster="+std::to_string(index)+" ORDER BY skill";
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT skill,level,endurance FROM monster_skill WHERE monster="+std::to_string(index)+" ORDER BY skill";
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&BaseServer::loadMonsterSkills_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        preload_market_items();
    }
}

void BaseServer::loadMonsterSkills_static(void *object)
{
    static_cast<BaseServer *>(object)->loadMonsterSkills_return();
}

void BaseServer::loadMonsterSkills_return()
{
    const uint32_t &monsterId=GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.at(entryListIndex).monster.id;
    std::vector<PlayerMonster::PlayerSkill> skills;
    bool ok;
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        PlayerMonster::PlayerSkill skill;
        skill.skill=stringtouint16(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
        if(ok)
        {
            if(CommonDatapack::commonDatapack.monsterSkills.find(skill.skill)==CommonDatapack::commonDatapack.monsterSkills.end())
            {
                ok=false;
                std::cerr << "skill " << skill.skill << " for monsterId: " << monsterId << " is not found into skill list" << std::endl;
            }
        }
        else
            std::cerr << "skill id: " << GlobalServerData::serverPrivateVariables.db_common->value(0) << " is not a number" << std::endl;
        if(ok)
        {
            skill.level=stringtouint8(GlobalServerData::serverPrivateVariables.db_common->value(1),&ok);
            if(ok)
            {
                if(skill.level>CommonDatapack::commonDatapack.monsterSkills.at(skill.skill).level.size())
                {
                    ok=false;
                    std::cerr << "skill " << skill.skill << " for monsterId: " << monsterId << " have not the level: " << skill.level << std::endl;
                }
            }
            else
                std::cerr << "skill level: " << GlobalServerData::serverPrivateVariables.db_common->value(1) << " is not a number" << std::endl;
        }
        if(ok)
        {
            skill.endurance=stringtouint8(GlobalServerData::serverPrivateVariables.db_common->value(2),&ok);
            if(ok)
            {
                if(skill.endurance>CommonDatapack::commonDatapack.monsterSkills.at(skill.skill).level.at(skill.level-1).endurance)
                {
                    skill.endurance=CommonDatapack::commonDatapack.monsterSkills.at(skill.skill).level.at(skill.level-1).endurance;
                    ok=false;
                    std::cerr << "endurance of skill " << skill.skill << " for monsterId: " <<  monsterId<< " have been fixed by lower at " << skill.endurance << ": truncated" << std::endl;
                }
            }
            else
                std::cerr << "skill level: " << GlobalServerData::serverPrivateVariables.db_common->value(2) << " is not a number" << std::endl;
        }
        if(ok)
            skills.push_back(skill);
    }
    loadMonsterSkills(entryListIndex+1);
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
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id` FROM `clan` ORDER BY `id` DESC LIMIT 0,1;";
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id FROM clan ORDER BY id DESC LIMIT 0,1;";
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id FROM clan ORDER BY id DESC LIMIT 1;";
        break;
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
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id` FROM `account` ORDER BY `id` DESC LIMIT 0,1;";
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id FROM account ORDER BY id DESC LIMIT 0,1;";
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id FROM account ORDER BY id DESC LIMIT 1;";
        break;
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
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id` FROM `character` ORDER BY `id` DESC LIMIT 0,1;";
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id FROM character ORDER BY id DESC LIMIT 0,1;";
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id FROM character ORDER BY id DESC LIMIT 1;";
        break;
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
