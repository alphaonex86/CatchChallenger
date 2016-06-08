#include "../base/BaseServer.h"

#ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
#include "MapServerCrafting.h"
#include "../VariableServer.h"
#include "../base/MapServer.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/MoveOnTheMap.h"
#include "../../general/base/DatapackGeneralLoader.h"
#include "../base/SqlFunction.h"
#include "../base/DictionaryServer.h"
#include "../base/GlobalServerData.h"

#include <QFile>
#include <vector>
#include <QDomDocument>
#include <QDomElement>
#endif

using namespace CatchChallenger;

#ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
void BaseServer::preload_plant_on_map_sql()
{
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    DebugClass::debugConsole(std::stringLiteral("%1 SQL monster max id").arg(GlobalServerData::serverPrivateVariables.maxMonsterId));
    #endif

    std::string queryText;
    switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
    {
        default:
        #if defined(CATCHCHALLENGER_DB_MYSQL) && (not defined(EPOLLCATCHCHALLENGERSERVER))
        case DatabaseBase::Type::Mysql:
            queryText=std::stringLiteral("SELECT `id`,`map`,`x`,`y`,`plant`,`character`,`plant_timestamps` FROM `plant`");
        break;
        #endif
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::Type::SQLite:
            queryText=std::stringLiteral("SELECT id,map,x,y,plant,character,plant_timestamps FROM plant");
        break;
        #endif
        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
        case DatabaseBase::Type::PostgreSQL:
            queryText=std::stringLiteral("SELECT id,map,x,y,plant,character,plant_timestamps FROM plant");
        break;
        #endif
    }
    if(GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText.toLatin1(),this,&BaseServer::preload_plant_on_map_static)==NULL)
    {
        qDebug() << std::stringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db_server->errorMessage());
        preload_market_monsters_sql();
    }
}

void BaseServer::preload_plant_on_map_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_plant_on_map_return();
}

void BaseServer::preload_plant_on_map_return()
{
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        bool ok;
        const uint32_t &id=DatabaseFunction::std::string(GlobalServerData::serverPrivateVariables.db_server->value(0)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(std::stringLiteral("Plant id ignored because is not a number: %1").arg(GlobalServerData::serverPrivateVariables.db_server->value(0)));
            continue;
        }
        if(GlobalServerData::serverPrivateVariables.plantUsedId.contains(id))
        {
            DebugClass::debugConsole(std::stringLiteral("Plant id already found: %1").arg(id));
            continue;
        }
        if(GlobalServerData::serverPrivateVariables.plantUnusedId.contains(id))
            GlobalServerData::serverPrivateVariables.plantUnusedId.removeOne(id);
        GlobalServerData::serverPrivateVariables.plantUsedId << id;
        //put the unused id
        if(GlobalServerData::serverPrivateVariables.maxPlantId<id)
        {
            uint32_t index=GlobalServerData::serverPrivateVariables.maxPlantId+1;
            while(index<id)
            {
                GlobalServerData::serverPrivateVariables.plantUnusedId << index;
                index++;
            }
            GlobalServerData::serverPrivateVariables.maxPlantId=id;
        }
        const uint32_t &map_database_id=DatabaseFunction::std::string(GlobalServerData::serverPrivateVariables.db_server->value(1)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(std::stringLiteral("Plant ignored because the map is not a number"));
            continue;
        }
        if(map_database_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
        {
            DebugClass::debugConsole(std::stringLiteral("Plant ignored because the map not exists (out of range): %1").arg(map_database_id));
            continue;
        }
        MapServer * const mapForPlantOnServer=static_cast<MapServer *>(DictionaryServer::dictionary_map_database_to_internal.at(map_database_id));
        if(mapForPlantOnServer==NULL)
        {
            DebugClass::debugConsole(std::stringLiteral("Plant ignored because the map not exists (not found): %1").arg(map_database_id));
            continue;
        }
        if(mapForPlantOnServer->plants.size()>=255)
        {
            DebugClass::debugConsole(std::stringLiteral("Plant ignored because the map have 255 or more plant: %1").arg(mapForPlantOnServer->map_file));
            continue;
        }
        const uint8_t &x=DatabaseFunction::std::string(GlobalServerData::serverPrivateVariables.db_server->value(2)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(std::stringLiteral("Plant ignored because the x is not a number"));
            continue;
        }
        if(x>=mapForPlantOnServer->width)
        {
            DebugClass::debugConsole(std::stringLiteral("Plant ignored because the x>%1 for the map %2: %3").arg(mapForPlantOnServer->width).arg(mapForPlantOnServer->map_file).arg(x));
            continue;
        }
        const uint8_t &y=DatabaseFunction::std::string(GlobalServerData::serverPrivateVariables.db_server->value(3)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(std::stringLiteral("Plant ignored because the y is not a number"));
            continue;
        }
        if(y>=mapForPlantOnServer->height)
        {
            DebugClass::debugConsole(std::stringLiteral("Plant ignored because the y>%1 for the map %2: %3").arg(mapForPlantOnServer->height).arg(mapForPlantOnServer->map_file).arg(y));
            continue;
        }
        const uint8_t &plant=DatabaseFunction::std::string(GlobalServerData::serverPrivateVariables.db_server->value(4)).toUInt(&ok);
        if(!ok)
            continue;
        if(!CommonDatapack::commonDatapack.plants.contains(plant))
        {
            DebugClass::debugConsole(std::stringLiteral("Plant dropped to not block the player, due to missing plant into the list: %1").arg(plant));
            continue;
        }
        const uint32_t &character=DatabaseFunction::std::string(GlobalServerData::serverPrivateVariables.db_server->value(5)).toUInt(&ok);
        if(!ok)
            continue;
        if(!MoveOnTheMap::isDirt(*mapForPlantOnServer,x,y))
        {
            DebugClass::debugConsole(std::stringLiteral("Plant ignored because is not into dirt layer: %1 (%2,%3)").arg(mapForPlantOnServer->map_file).arg(x).arg(y));
            continue;
        }
        const uint64_t &plant_timestamps=DatabaseFunction::std::string(GlobalServerData::serverPrivateVariables.db_server->value(6)).toULongLong(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(std::stringLiteral("Plant timestamps is not a number: %1 (%2,%3)").arg(mapForPlantOnServer->map_file).arg(x).arg(y));
            continue;
        }

        //plant_timestamps
        MapServerCrafting::PlantOnMap plantOnMap;
        plantOnMap.pointOnMapDbCode=id;
        #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        plantOnMap.x=x;
        plantOnMap.y=y;
        #endif
        plantOnMap.plant=plant;
        plantOnMap.character=character;
        plantOnMap.mature_at=plant_timestamps+CommonDatapack::commonDatapack.plants.value(plant).fruits_seconds;
        plantOnMap.player_owned_expire_at=plant_timestamps+CommonDatapack::commonDatapack.plants.value(plant).fruits_seconds+60*60*24;
        #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        //mapForPlantOnServer->plants << plantOnMap;
        mapForPlantOnServer->plants.insert(std::pair<uint8_t,uint8_t>(x,y),plantOnMap);
        #else
        mapForPlantOnServer->plants.insert(std::pair<uint8_t,uint8_t>(x,y),plantOnMap);
        #endif
        #ifdef DEBUG_MESSAGE_MAP_PLANTS
        DebugClass::debugConsole(std::stringLiteral("put on the map: %1 (%2,%3) the plant: %4, owned by played id: %5, mature at: %6 (%7+%8)")
                                 .arg(map).arg(x).arg(y)
                                 .arg(plant)
                                 .arg(character)
                                 .arg(plantOnMap.mature_at).arg(plant_timestamps).arg(CommonDatapack::commonDatapack.plants.value(plant).fruits_seconds)
                                 );
        #endif
        plant_on_the_map++;
    }

    preload_market_monsters_sql();
}
#else
void BaseServer::preload_plant_on_map_sql()
{
    preload_market_monsters_sql();
}
#endif

void BaseServer::unload_the_plant_on_map()
{
}
