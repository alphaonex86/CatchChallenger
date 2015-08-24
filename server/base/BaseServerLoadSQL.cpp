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

#include <QFile>
#include <QByteArray>
#include <QDateTime>
#include <QTime>
#include <QCryptographicHash>
#include <time.h>
#include <iostream>
#ifndef EPOLLCATCHCHALLENGERSERVER
#include <QTimer>
#endif

using namespace CatchChallenger;

void BaseServer::preload_zone_sql()
{
    const int &listsize=entryListZone.size();
    while(entryListIndex<listsize)
    {
        std::string zoneCodeName=entryListZone.at(entryListIndex).fileName().toStdString();
        zoneCodeName.remove(BaseServer::text_dotxml);
        std::string queryText;
        switch(GlobalServerData::serverPrivateVariables.db_common->databaseType())
        {
            default:
            case DatabaseBase::DatabaseType::Mysql:
                queryText=std::stringLiteral("SELECT `clan` FROM `city` WHERE `city`='%1' ORDER BY `city`").arg(zoneCodeName);
            break;
            case DatabaseBase::DatabaseType::SQLite:
                queryText=std::stringLiteral("SELECT clan FROM city WHERE city='%1' ORDER BY city").arg(zoneCodeName);
            break;
            case DatabaseBase::DatabaseType::PostgreSQL:
                queryText=std::stringLiteral("SELECT clan FROM city WHERE city='%1' ORDER BY city").arg(zoneCodeName);
            break;
        }
        if(GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText.toLatin1(),this,&BaseServer::preload_zone_static)==NULL)
        {
            std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
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
    if(DictionaryServer::dictionary_map_database_to_internal.isEmpty())
    {
        qDebug() << "Need be called after preload_dictionary_map()";
        abort();
    }
    if(!DictionaryServer::dictionary_pointOnMap_internal_to_database.isEmpty())
    {
        qDebug() << "!DictionaryServer::dictionary_pointOnMap_internal_to_database.isEmpty(), already called?";
        abort();
    }
    std::string queryText;
    switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
    {
        default:
        case DatabaseBase::DatabaseType::Mysql:
            queryText=std::stringLiteral("SELECT `id`,`map`,`x`,`y` FROM `dictionary_pointonmap` ORDER BY `map`,`x`,`y`");
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText=std::stringLiteral("SELECT id,map,x,y FROM dictionary_pointonmap ORDER BY map,x,y");
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText=std::stringLiteral("SELECT id,map,x,y FROM dictionary_pointonmap ORDER BY map,x,y");
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText.toLatin1(),this,&BaseServer::preload_pointOnMap_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
        criticalDatabaseQueryFailed();return;//stop because can't do the first db access

        preload_the_visibility_algorithm();
        preload_the_city_capture();
        preload_zone();
        qDebug() << std::stringLiteral("Loaded the server static datapack into %1ms").arg(timeDatapack.elapsed());
        timeDatapack.restart();

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
    if(DictionaryServer::dictionary_map_database_to_internal.isEmpty())
    {
        qDebug() << "Call preload_dictionary_map() before";
        abort();
    }
    bool ok;
    dictionary_pointOnMap_maxId=0;
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        const uint16_t &id=std::string(GlobalServerData::serverPrivateVariables.db_server->value(0)).toUInt(&ok);
        if(!ok)
            qDebug() << std::stringLiteral("preload_itemOnMap_return(): Id not found: %1").arg(std::string(GlobalServerData::serverPrivateVariables.db_server->value(0)));
        else
        {
            const uint32_t &map_id=std::string(GlobalServerData::serverPrivateVariables.db_server->value(1)).toUInt(&ok);
            if(!ok)
                qDebug() << std::stringLiteral("preload_itemOnMap_return(): map id not number: %1").arg(std::string(GlobalServerData::serverPrivateVariables.db_server->value(1)));
            else
            {
                if(map_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
                    qDebug() << std::stringLiteral("preload_itemOnMap_return(): map out of range: %1 %2 %3").arg(map_id);
                else
                {
                    if(DictionaryServer::dictionary_map_database_to_internal.value(map_id)==NULL)
                        qDebug() << std::stringLiteral("preload_itemOnMap_return(): map == NULL for this id, map not found: %1 %2 %3").arg(map_id);
                    else
                    {
                        const uint32_t &x=std::string(GlobalServerData::serverPrivateVariables.db_server->value(2)).toUInt(&ok);
                        if(!ok)
                            qDebug() << std::stringLiteral("preload_itemOnMap_return(): x not number: %1").arg(std::string(GlobalServerData::serverPrivateVariables.db_server->value(2)));
                        else
                        {
                            if(x>255 || x>=DictionaryServer::dictionary_map_database_to_internal.value(map_id)->width)
                                qDebug() << std::stringLiteral("preload_itemOnMap_return(): x out of range").arg(x);
                            else
                            {
                                const uint32_t &y=std::string(GlobalServerData::serverPrivateVariables.db_server->value(3)).toUInt(&ok);
                                if(!ok)
                                    qDebug() << std::stringLiteral("preload_itemOnMap_return(): y not number: %1").arg(std::string(GlobalServerData::serverPrivateVariables.db_server->value(3)));
                                else
                                {
                                    if(y>255 || y>=DictionaryServer::dictionary_map_database_to_internal.value(map_id)->height)
                                        qDebug() << std::stringLiteral("preload_itemOnMap_return(): y out of range").arg(y);
                                    else
                                    {
                                        const std::string &map_file=DictionaryServer::dictionary_map_database_to_internal.value(map_id)->map_file;

                                        ///used only at map loading, \see BaseServer::preload_the_map()
                                        DictionaryServer::dictionary_pointOnMap_internal_to_database[map_file][QPair<uint8_t/*x*/,uint8_t/*y*/>(x,y)]=id;

                                        while((uint32_t)DictionaryServer::dictionary_pointOnMap_database_to_internal.size()<=id)
                                        {
                                            DictionaryServer::MapAndPoint mapAndPoint;
                                            mapAndPoint.map=NULL;
                                            mapAndPoint.x=0;
                                            mapAndPoint.y=0;
                                            //less bandwith than send map,x,y
                                            mapAndPoint.indexOfItemOnMap=255;
                                            #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
                                            mapAndPoint.indexOfDirtOnMap=255;
                                            #endif
                                            DictionaryServer::dictionary_pointOnMap_database_to_internal << mapAndPoint;
                                        }

                                        DictionaryServer::MapAndPoint mapAndPoint;
                                        mapAndPoint.indexOfItemOnMap=255;
                                        #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
                                        mapAndPoint.indexOfDirtOnMap=255;
                                        #endif
                                        mapAndPoint.map=DictionaryServer::dictionary_map_database_to_internal.value(map_id);
                                        mapAndPoint.x=x;
                                        mapAndPoint.y=y;
                                        DictionaryServer::dictionary_pointOnMap_database_to_internal[id]=mapAndPoint;

                                        dictionary_pointOnMap_maxId=id;
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
        DebugClass::debugConsole(std::stringLiteral("%1 SQL item on map dictionary").arg(DictionaryServer::dictionary_pointOnMap_internal_to_database.size()));

        preload_the_visibility_algorithm();
        if(!preload_the_city_capture())
            return;
        if(!preload_zone())
            return;
        qDebug() << std::stringLiteral("Loaded the server static datapack into %1ms").arg(timeDatapack.elapsed());
        timeDatapack.restart();

        //start SQL load
        preload_map_semi_after_db_id();
    }
}

void BaseServer::preload_dictionary_map()
{
    if(GlobalServerData::serverPrivateVariables.db_server==NULL)
    {
        qDebug() << "GlobalServerData::serverPrivateVariables.db_server==NULL";
        abort();
    }
    if(GlobalServerData::serverPrivateVariables.map_list.isEmpty())
    {
        qDebug() << "No map to list";
        abort();
    }
    if(!DictionaryServer::dictionary_map_database_to_internal.isEmpty())
    {
        qDebug() << "preload_dictionary_map() already call";
        abort();
    }
    std::string queryText;
    switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
    {
        default:
        case DatabaseBase::DatabaseType::Mysql:
            queryText=std::stringLiteral("SELECT `id`,`map` FROM `dictionary_map` ORDER BY `map`");
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText=std::stringLiteral("SELECT id,map FROM dictionary_map ORDER BY map");
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText=std::stringLiteral("SELECT id,map FROM dictionary_map ORDER BY map");
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText.toLatin1(),this,&BaseServer::preload_dictionary_map_static)==NULL)
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
    if(!DictionaryServer::dictionary_map_database_to_internal.isEmpty())
    {
        qDebug() << "preload_dictionary_map_return() already call";
        abort();
    }
    QSet<std::string> foundMap;
    int databaseMapId=0;
    int obsoleteMap=0;
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        bool ok;
        databaseMapId=std::string(GlobalServerData::serverPrivateVariables.db_server->value(0)).toUInt(&ok);
        const std::string &map=std::string(GlobalServerData::serverPrivateVariables.db_server->value(1));
        if(DictionaryServer::dictionary_map_database_to_internal.size()<=databaseMapId)
        {
            int index=DictionaryServer::dictionary_map_database_to_internal.size();
            while(index<=databaseMapId)
            {
                DictionaryServer::dictionary_map_database_to_internal << NULL;
                index++;
            }
        }
        if(GlobalServerData::serverPrivateVariables.map_list.contains(map))
        {
            DictionaryServer::dictionary_map_database_to_internal[databaseMapId]=static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list.value(map));
            foundMap << map;
            static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list.value(map))->reverse_db_id=databaseMapId;
        }
        else
            obsoleteMap++;
    }
    GlobalServerData::serverPrivateVariables.db_server->clear();
    if(obsoleteMap>0 && GlobalServerData::serverPrivateVariables.map_list.isEmpty())
    {
        qDebug() << "Only obsolete map!";
        abort();
    }
    std::stringList map_list_flat=GlobalServerData::serverPrivateVariables.map_list.keys();
    map_list_flat.sort();
    int index=0;
    while(index<map_list_flat.size())
    {
        const std::string &map=map_list_flat.at(index);
        if(!foundMap.contains(map))
        {
            databaseMapId++;
            std::string queryText;
            switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
            {
                default:
                case DatabaseBase::DatabaseType::Mysql:
                    queryText=std::stringLiteral("INSERT INTO `dictionary_map`(`id`,`map`) VALUES(%1,'%2');").arg(databaseMapId).arg(map);
                break;
                case DatabaseBase::DatabaseType::SQLite:
                    queryText=std::stringLiteral("INSERT INTO dictionary_map(id,map) VALUES(%1,'%2');").arg(databaseMapId).arg(map);
                break;
                case DatabaseBase::DatabaseType::PostgreSQL:
                    queryText=std::stringLiteral("INSERT INTO dictionary_map(id,map) VALUES(%1,'%2');").arg(databaseMapId).arg(map);
                break;
            }
            if(!GlobalServerData::serverPrivateVariables.db_server->asyncWrite(queryText.toLatin1()))
            {
                std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
                criticalDatabaseQueryFailed();return;//stop because can't resolv the name
            }
            while(DictionaryServer::dictionary_map_database_to_internal.size()<=databaseMapId)
                DictionaryServer::dictionary_map_database_to_internal << NULL;
            DictionaryServer::dictionary_map_database_to_internal[databaseMapId]=static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list[map]);
            static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list[map])->reverse_db_id=databaseMapId;
        }
        index++;
    }

    if(obsoleteMap)
        DebugClass::debugConsole(std::stringLiteral("%1 SQL obsolete map dictionary").arg(obsoleteMap));
    DebugClass::debugConsole(std::stringLiteral("%1 SQL map dictionary").arg(DictionaryServer::dictionary_map_database_to_internal.size()));

    preload_pointOnMap_sql();
}


bool BaseServer::preload_zone()
{
    //open and quick check the file
    entryListZone=QFileInfoList(QDir(GlobalServerData::serverSettings.datapack_basePath+DATAPACK_BASE_PATH_ZONE).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot));
    entryListIndex=0;
    return preload_zone_init();
}

void BaseServer::preload_industries()
{
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    DebugClass::debugConsole(std::stringLiteral("%1 SQL clan max id").arg(GlobalServerData::serverPrivateVariables.maxClanId));
    #endif

    std::string queryText;
    switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
    {
        default:
        case DatabaseBase::DatabaseType::Mysql:
            queryText=QLatin1String("SELECT `id`,`resources`,`products`,`last_update` FROM `factory`");
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText=QLatin1String("SELECT id,resources,products,last_update FROM factory");
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText=QLatin1String("SELECT id,resources,products,last_update FROM factory");
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText.toLatin1(),this,&BaseServer::preload_industries_static)==NULL)
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
        uint32_t id=std::string(GlobalServerData::serverPrivateVariables.db_server->value(0)).toUInt(&ok);
        if(!ok)
            DebugClass::debugConsole(std::stringLiteral("preload_industries: id is not a number"));
        if(ok)
        {
            if(!CommonDatapack::commonDatapack.industriesLink.contains(id))
            {
                ok=false;
                DebugClass::debugConsole(std::stringLiteral("preload_industries: industries link not found"));
            }
        }
        if(ok)
        {
            const std::stringList &resourcesStringList=std::string(GlobalServerData::serverPrivateVariables.db_server->value(1)).split(BaseServer::text_dotcomma);
            int index=0;
            const int &listsize=resourcesStringList.size();
            while(index<listsize)
            {
                const std::stringList &itemStringList=resourcesStringList.at(index).split(BaseServer::text_arrow);
                if(itemStringList.size()!=2)
                {
                    DebugClass::debugConsole(std::stringLiteral("preload_industries: wrong entry count"));
                    ok=false;
                    break;
                }
                const uint32_t &item=itemStringList.first().toUInt(&ok);
                if(!ok)
                {
                    DebugClass::debugConsole(std::stringLiteral("preload_industries: item is not a number"));
                    break;
                }
                uint32_t quantity=itemStringList.last().toUInt(&ok);
                if(!ok)
                {
                    DebugClass::debugConsole(std::stringLiteral("preload_industries: quantity is not a number"));
                    break;
                }
                if(industryStatus.resources.contains(item))
                {
                    DebugClass::debugConsole(std::stringLiteral("preload_industries: item already set"));
                    ok=false;
                    break;
                }
                const Industry &industry=CommonDatapack::commonDatapack.industries.value(CommonDatapack::commonDatapack.industriesLink.value(id).industry);
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
                    DebugClass::debugConsole(std::stringLiteral("preload_industries: item in db not found"));
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
            const std::stringList &productsStringList=std::string(GlobalServerData::serverPrivateVariables.db_server->value(2)).split(BaseServer::text_dotcomma);
            int index=0;
            const int &listsize=productsStringList.size();
            while(index<listsize)
            {
                const std::stringList &itemStringList=productsStringList.at(index).split(BaseServer::text_arrow);
                if(itemStringList.size()!=2)
                {
                    DebugClass::debugConsole(std::stringLiteral("preload_industries: wrong entry count"));
                    ok=false;
                    break;
                }
                const uint32_t &item=itemStringList.first().toUInt(&ok);
                if(!ok)
                {
                    DebugClass::debugConsole(std::stringLiteral("preload_industries: item is not a number"));
                    break;
                }
                uint32_t quantity=itemStringList.last().toUInt(&ok);
                if(!ok)
                {
                    DebugClass::debugConsole(std::stringLiteral("preload_industries: quantity is not a number"));
                    break;
                }
                if(industryStatus.products.contains(item))
                {
                    DebugClass::debugConsole(std::stringLiteral("preload_industries: item already set"));
                    ok=false;
                    break;
                }
                const Industry &industry=CommonDatapack::commonDatapack.industries.value(CommonDatapack::commonDatapack.industriesLink.value(id).industry);
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
                    DebugClass::debugConsole(std::stringLiteral("preload_industries: item in db not found"));
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
            industryStatus.last_update=std::string(GlobalServerData::serverPrivateVariables.db_server->value(3)).toUInt(&ok);
            if(!ok)
                DebugClass::debugConsole(std::stringLiteral("preload_industries: last_update is not a number"));
        }
        if(ok)
            GlobalServerData::serverPrivateVariables.industriesStatus[id]=industryStatus;
    }
    DebugClass::debugConsole(std::stringLiteral("%1 SQL industries loaded").arg(GlobalServerData::serverPrivateVariables.industriesStatus.size()));

    preload_finish();
}

//unique table due to linked datas like skills/buffers product need of id, to be accruate on max id
void BaseServer::preload_market_monsters_prices_sql()
{
    DebugClass::debugConsole(std::stringLiteral("%1 SQL industrie loaded").arg(GlobalServerData::serverPrivateVariables.industriesStatus.size()));

    std::string queryText;
    switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
    {
        default:
        case DatabaseBase::DatabaseType::Mysql:
            queryText=QLatin1String("SELECT `id`,`market_price` FROM `monster_market_price` ORDER BY `id`");
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText=QLatin1String("SELECT id,market_price FROM monster_market_price ORDER BY id");
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText=QLatin1String("SELECT id,market_price FROM monster_market_price ORDER BY id");
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText.toLatin1(),this,&BaseServer::preload_market_monsters_prices_static)==NULL)
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
        monsterSemi.id=std::string(GlobalServerData::serverPrivateVariables.db_server->value(0)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(std::stringLiteral("monsterId: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db_server->value(0)));
            continue;
        }
        monsterSemi.price=std::string(GlobalServerData::serverPrivateVariables.db_server->value(1)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(std::stringLiteral("price: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db_server->value(1)));
            continue;
        }
        //finish it
        if(ok)
            monsterSemiMarketList << monsterSemi;
    }

    preload_market_monsters_sql();
}

//unique table due to linked datas like skills/buffers product need of id, to be accruate on max id
void BaseServer::preload_market_monsters_sql()
{
    if(monsterSemiMarketList.isEmpty())
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
                queryText=std::stringLiteral("SELECT `id`,`hp`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character` FROM `monster` WHERE `id`=%1").arg(monsterSemiMarketList.first().id);
            break;
            case DatabaseBase::DatabaseType::SQLite:
                queryText=std::stringLiteral("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character FROM monster WHERE id=%1").arg(monsterSemiMarketList.first().id);
            break;
            case DatabaseBase::DatabaseType::PostgreSQL:
                queryText=std::stringLiteral("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character FROM monster WHERE id=%1").arg(monsterSemiMarketList.first().id);
            break;
        }
    else
        switch(GlobalServerData::serverPrivateVariables.db_common->databaseType())
        {
            default:
            case DatabaseBase::DatabaseType::Mysql:
                queryText=std::stringLiteral("SELECT `id`,`hp`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character` FROM `monster` WHERE `id`=%1").arg(monsterSemiMarketList.first().id);
            break;
            case DatabaseBase::DatabaseType::SQLite:
                queryText=std::stringLiteral("SELECT id,hp,monster,level,xp,captured_with,gender,egg_step,character FROM monster WHERE id=%1").arg(monsterSemiMarketList.first().id);
            break;
            case DatabaseBase::DatabaseType::PostgreSQL:
                queryText=std::stringLiteral("SELECT id,hp,monster,level,xp,captured_with,gender,egg_step,character FROM monster WHERE id=%1").arg(monsterSemiMarketList.first().id);
            break;
        }
    if(GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText.toLatin1(),this,&BaseServer::preload_market_monsters_static)==NULL)
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
        playerMonster.id=std::string(GlobalServerData::serverPrivateVariables.db_common->value(0)).toUInt(&ok);
        if(!ok)
            DebugClass::debugConsole(std::stringLiteral("monsterId: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db_common->value(0)));
        if(ok)
        {
            playerMonster.monster=std::string(GlobalServerData::serverPrivateVariables.db_common->value(2)).toUInt(&ok);
            if(ok)
            {
                if(!CommonDatapack::commonDatapack.monsters.contains(playerMonster.monster))
                {
                    ok=false;
                    DebugClass::debugConsole(std::stringLiteral("monster: %1 is not into monster list").arg(playerMonster.monster));
                }
            }
            else
            DebugClass::debugConsole(std::stringLiteral("monster: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db_common->value(2)));
        }
        if(ok)
        {
            playerMonster.level=std::string(GlobalServerData::serverPrivateVariables.db_common->value(3)).toUInt(&ok);
            if(ok)
            {
                if(playerMonster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                {
                    DebugClass::debugConsole(std::stringLiteral("level: %1 greater than %2, truncated").arg(playerMonster.level).arg(CATCHCHALLENGER_MONSTER_LEVEL_MAX));
                    playerMonster.level=CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                }
            }
            else
                DebugClass::debugConsole(std::stringLiteral("level: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db_common->value(3)));
        }
        if(ok)
        {
            playerMonster.remaining_xp=std::string(GlobalServerData::serverPrivateVariables.db_common->value(4)).toUInt(&ok);
            if(ok)
            {
                if(playerMonster.remaining_xp>CommonDatapack::commonDatapack.monsters.value(playerMonster.monster).level_to_xp.at(playerMonster.level-1))
                {
                    DebugClass::debugConsole(std::stringLiteral("monster xp: %1 greater than %2, truncated").arg(playerMonster.remaining_xp).arg(CommonDatapack::commonDatapack.monsters.value(playerMonster.monster).level_to_xp.at(playerMonster.level-1)));
                    playerMonster.remaining_xp=0;
                }
            }
            else
                DebugClass::debugConsole(std::stringLiteral("monster xp: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db_common->value(4)));
        }
        if(CommonSettingsServer::commonSettingsServer.useSP)
        {
            if(ok)
            {
                playerMonster.sp=std::string(GlobalServerData::serverPrivateVariables.db_common->value(5)).toUInt(&ok);
                if(!ok)
                    DebugClass::debugConsole(std::stringLiteral("monster sp: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db_common->value(5)));
            }
        }
        else
            playerMonster.sp=0;
        if(ok)
        {
            playerMonster.catched_with=std::string(GlobalServerData::serverPrivateVariables.db_common->value(6-spOffset)).toUInt(&ok);
            if(ok)
            {
                if(!CommonDatapack::commonDatapack.items.item.contains(playerMonster.catched_with))
                    DebugClass::debugConsole(std::stringLiteral("captured_with: %1 is not is not into items list").arg(playerMonster.catched_with));
            }
            else
                DebugClass::debugConsole(std::stringLiteral("captured_with: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db_common->value(6-spOffset)));
        }
        if(ok)
        {
            const uint32_t &value=std::string(GlobalServerData::serverPrivateVariables.db_common->value(7-spOffset)).toUInt(&ok);
            if(ok)
            {
                if(value>=1 && value<=3)
                    playerMonster.gender=static_cast<Gender>(value);
                else
                {
                    playerMonster.gender=Gender_Unknown;
                    DebugClass::debugConsole(std::stringLiteral("unknown monster gender: %1").arg(value));
                    ok=false;
                }
            }
            else
            {
                playerMonster.gender=Gender_Unknown;
                DebugClass::debugConsole(std::stringLiteral("unknown monster gender: %1").arg(GlobalServerData::serverPrivateVariables.db_common->value(7-spOffset)));
                ok=false;
            }
        }
        if(ok)
        {
            playerMonster.egg_step=std::string(GlobalServerData::serverPrivateVariables.db_common->value(8-spOffset)).toUInt(&ok);
            if(!ok)
                DebugClass::debugConsole(std::stringLiteral("monster egg_step: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db_common->value(8-spOffset)));
        }
        if(ok)
            marketPlayerMonster.player=std::string(GlobalServerData::serverPrivateVariables.db_common->value(9-spOffset)).toUInt(&ok);
        if(ok)
            marketPlayerMonster.cash=monsterSemiMarketList.first().price;
        //stats
        if(ok)
        {
            playerMonster.hp=std::string(GlobalServerData::serverPrivateVariables.db_common->value(1)).toUInt(&ok);
            if(ok)
            {
                const Monster::Stat &stat=CommonFightEngine::getStat(CommonDatapack::commonDatapack.monsters.value(playerMonster.monster),playerMonster.level);
                if(playerMonster.hp>stat.hp)
                {
                    DebugClass::debugConsole(std::stringLiteral("monster hp: %1 greater than max hp %2 for the level %3 of the monster %4, truncated")
                    .arg(playerMonster.hp)
                    .arg(stat.hp)
                    .arg(playerMonster.level)
                    .arg(playerMonster.monster)
                    );
                    playerMonster.hp=stat.hp;
                }
            }
            else
            DebugClass::debugConsole(std::stringLiteral("monster hp: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db_common->value(1)));
        }
        //finish it
        if(ok)
        {
            marketPlayerMonster.monster=playerMonster;
            GlobalServerData::serverPrivateVariables.marketPlayerMonsterList << marketPlayerMonster;
        }
    }
    monsterSemiMarketList.removeFirst();
    if(!monsterSemiMarketList.isEmpty())
    {
        preload_market_monsters_sql();
        return;
    }
    if(!GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.isEmpty())
        loadMonsterBuffs(0);
    else
        preload_market_items();
}

void BaseServer::preload_market_items()
{
    DebugClass::debugConsole(std::stringLiteral("%1 SQL monster list loaded").arg(GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.size()));

    Client::marketObjectIdList.clear();
    Client::marketObjectIdList.reserve(65535);
    int index=0;
    while(index<=65535)
    {
        Client::marketObjectIdList << index;
        index++;
    }
    //do the query
    std::string queryText;
    switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
    {
        default:
        case DatabaseBase::DatabaseType::Mysql:
            queryText=QLatin1String("SELECT `item`,`quantity`,`character`,`market_price` FROM `item_market` ORDER BY `item`");
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText=QLatin1String("SELECT item,quantity,character,market_price FROM item_market ORDER BY item");
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText=QLatin1String("SELECT item,quantity,character,market_price FROM item_market ORDER BY item");
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText.toLatin1(),this,&BaseServer::preload_market_items_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        if(GlobalServerData::serverSettings.automatic_account_creation)
            load_account_max_id();
        else if(CommonSettingsCommon::commonSettingsCommon.max_character)
            load_character_max_id();
        else
        #endif
            BaseServerMasterLoadDictionary::load(GlobalServerData::serverPrivateVariables.db_base);
    }
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
        marketItem.item=std::string(GlobalServerData::serverPrivateVariables.db_server->value(0)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(std::stringLiteral("item id is not a number, skip"));
            continue;
        }
        marketItem.quantity=std::string(GlobalServerData::serverPrivateVariables.db_server->value(1)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(std::stringLiteral("quantity is not a number, skip"));
            continue;
        }
        marketItem.player=std::string(GlobalServerData::serverPrivateVariables.db_server->value(2)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(std::stringLiteral("player id is not a number, skip"));
            continue;
        }
        marketItem.cash=std::string(GlobalServerData::serverPrivateVariables.db_server->value(3)).toULongLong(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(std::stringLiteral("cash is not a number, skip"));
            continue;
        }
        if(Client::marketObjectIdList.isEmpty())
        {
            DebugClass::debugConsole(std::stringLiteral("not more marketObjectId into the list, skip"));
            return;
        }
        marketItem.marketObjectId=Client::marketObjectIdList.first();
        Client::marketObjectIdList.removeFirst();
        GlobalServerData::serverPrivateVariables.marketItemList << marketItem;
    }
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    if(GlobalServerData::serverSettings.automatic_account_creation)
        load_account_max_id();
    else if(CommonSettingsCommon::commonSettingsCommon.max_character)
        load_character_max_id();
    else
    #endif
        BaseServerMasterLoadDictionary::load(GlobalServerData::serverPrivateVariables.db_base);
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
            queryText=std::stringLiteral("SELECT `buff`,`level` FROM `monster_buff` WHERE `monster`=%1 ORDER BY `buff`").arg(index);
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText=std::stringLiteral("SELECT buff,level FROM monster_buff WHERE monster=%1 ORDER BY buff").arg(index);
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText=std::stringLiteral("SELECT buff,level FROM monster_buff WHERE monster=%1 ORDER BY buff").arg(index);
        break;
    }

    if(GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText.toLatin1(),this,&BaseServer::loadMonsterBuffs_static)==NULL)
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
    QList<PlayerBuff> buffs;
    bool ok;
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        PlayerBuff buff;
        buff.buff=std::string(GlobalServerData::serverPrivateVariables.db_common->value(0)).toUInt(&ok);
        if(ok)
        {
            if(!CommonDatapack::commonDatapack.monsterBuffs.contains(buff.buff))
            {
                ok=false;
                DebugClass::debugConsole(std::stringLiteral("buff %1 for monsterId: %2 is not found into buff list").arg(buff.buff).arg(monsterId));
            }
        }
        else
            DebugClass::debugConsole(std::stringLiteral("buff id: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db_common->value(0)));
        if(ok)
        {
            buff.level=std::string(GlobalServerData::serverPrivateVariables.db_common->value(1)).toUInt(&ok);
            if(ok)
            {
                if(buff.level<=0 || buff.level>CommonDatapack::commonDatapack.monsterBuffs.value(buff.buff).level.size())
                {
                    ok=false;
                    DebugClass::debugConsole(std::stringLiteral("buff %1 for monsterId: %2 have not the level: %3").arg(buff.buff).arg(monsterId).arg(buff.level));
                }
            }
            else
                DebugClass::debugConsole(std::stringLiteral("buff level: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db_common->value(2)));
        }
        if(ok)
        {
            if(CommonDatapack::commonDatapack.monsterBuffs.value(buff.buff).level.at(buff.level-1).duration!=Buff::Duration_Always)
            {
                ok=false;
                DebugClass::debugConsole(std::stringLiteral("buff %1 for monsterId: %2 can't be loaded from the db if is not permanent").arg(buff.buff).arg(monsterId));
            }
        }
        if(ok)
            buffs << buff;
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
            queryText=std::stringLiteral("SELECT `skill`,`level`,`endurance` FROM `monster_skill` WHERE `monster`=%1 ORDER BY `skill`").arg(index);
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText=std::stringLiteral("SELECT skill,level,endurance FROM monster_skill WHERE monster=%1 ORDER BY skill").arg(index);
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText=std::stringLiteral("SELECT skill,level,endurance FROM monster_skill WHERE monster=%1 ORDER BY skill").arg(index);
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText.toLatin1(),this,&BaseServer::loadMonsterSkills_static)==NULL)
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
    QList<PlayerMonster::PlayerSkill> skills;
    bool ok;
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        PlayerMonster::PlayerSkill skill;
        skill.skill=std::string(GlobalServerData::serverPrivateVariables.db_common->value(0)).toUInt(&ok);
        if(ok)
        {
            if(!CommonDatapack::commonDatapack.monsterSkills.contains(skill.skill))
            {
                ok=false;
                DebugClass::debugConsole(std::stringLiteral("skill %1 for monsterId: %2 is not found into skill list").arg(skill.skill).arg(monsterId));
            }
        }
        else
            DebugClass::debugConsole(std::stringLiteral("skill id: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db_common->value(0)));
        if(ok)
        {
            skill.level=std::string(GlobalServerData::serverPrivateVariables.db_common->value(1)).toUInt(&ok);
            if(ok)
            {
                if(skill.level>CommonDatapack::commonDatapack.monsterSkills.value(skill.skill).level.size())
                {
                    ok=false;
                    DebugClass::debugConsole(std::stringLiteral("skill %1 for monsterId: %2 have not the level: %3").arg(skill.skill).arg(monsterId).arg(skill.level));
                }
            }
            else
                DebugClass::debugConsole(std::stringLiteral("skill level: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db_common->value(1)));
        }
        if(ok)
        {
            skill.endurance=std::string(GlobalServerData::serverPrivateVariables.db_common->value(2)).toUInt(&ok);
            if(ok)
            {
                if(skill.endurance>CommonDatapack::commonDatapack.monsterSkills.value(skill.skill).level.at(skill.level-1).endurance)
                {
                    skill.endurance=CommonDatapack::commonDatapack.monsterSkills.value(skill.skill).level.at(skill.level-1).endurance;
                    ok=false;
                    DebugClass::debugConsole(std::stringLiteral("endurance of skill %1 for monsterId: %2 have been fixed by lower at ").arg(skill.endurance));
                }
            }
            else
                DebugClass::debugConsole(std::stringLiteral("skill level: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db_common->value(2)));
        }
        if(ok)
            skills << skill;
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
            queryText=QLatin1String("SELECT `id` FROM `clan` ORDER BY `id` DESC LIMIT 0,1;");
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText=QLatin1String("SELECT id FROM clan ORDER BY id DESC LIMIT 0,1;");
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText=QLatin1String("SELECT id FROM clan ORDER BY id DESC LIMIT 1;");
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText.toLatin1(),this,&BaseServer::load_clan_max_id_static)==NULL)
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
        GlobalServerData::serverPrivateVariables.maxClanId=std::string(GlobalServerData::serverPrivateVariables.db_common->value(0)).toUInt(&ok)+1;
        if(!ok)
        {
            DebugClass::debugConsole(std::stringLiteral("Max clan id is failed to convert to number"));
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
            queryText=QLatin1String("SELECT `id` FROM `account` ORDER BY `id` DESC LIMIT 0,1;");
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText=QLatin1String("SELECT id FROM account ORDER BY id DESC LIMIT 0,1;");
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText=QLatin1String("SELECT id FROM account ORDER BY id DESC LIMIT 1;");
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db_login->asyncRead(queryText.toLatin1(),this,&BaseServer::load_account_max_id_static)==NULL)
    {
        qDebug() << std::stringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db_login->errorMessage());
        if(CommonSettingsCommon::commonSettingsCommon.max_character)
            load_character_max_id();
        else
            BaseServerMasterLoadDictionary::load(GlobalServerData::serverPrivateVariables.db_base);
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
        GlobalServerData::serverPrivateVariables.maxAccountId=std::string(GlobalServerData::serverPrivateVariables.db_login->value(0)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(std::stringLiteral("Max account id is failed to convert to number"));
            //start to 0 due to pre incrementation before use
            GlobalServerData::serverPrivateVariables.maxAccountId=0;
            continue;
        }
    }
    if(CommonSettingsCommon::commonSettingsCommon.max_character)
        load_character_max_id();
    else
        BaseServerMasterLoadDictionary::load(GlobalServerData::serverPrivateVariables.db_base);
}

void BaseServer::load_character_max_id()
{
    std::string queryText;
    switch(GlobalServerData::serverPrivateVariables.db_common->databaseType())
    {
        default:
        case DatabaseBase::DatabaseType::Mysql:
            queryText=QLatin1String("SELECT `id` FROM `character` ORDER BY `id` DESC LIMIT 0,1;");
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText=QLatin1String("SELECT id FROM character ORDER BY id DESC LIMIT 0,1;");
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText=QLatin1String("SELECT id FROM character ORDER BY id DESC LIMIT 1;");
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText.toLatin1(),this,&BaseServer::load_character_max_id_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        BaseServerMasterLoadDictionary::load(GlobalServerData::serverPrivateVariables.db_base);
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
        GlobalServerData::serverPrivateVariables.maxCharacterId=std::string(GlobalServerData::serverPrivateVariables.db_common->value(0)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(std::stringLiteral("Max character id is failed to convert to number"));
            //start to 0 due to pre incrementation before use
            GlobalServerData::serverPrivateVariables.maxCharacterId=0;
            continue;
        }
    }
    BaseServerMasterLoadDictionary::load(GlobalServerData::serverPrivateVariables.db_base);
}
#endif
