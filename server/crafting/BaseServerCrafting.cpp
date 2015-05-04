#include "../base/BaseServer.h"
#include "MapServerCrafting.h"
#include "../VariableServer.h"
#include "../base/MapServer.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/DebugClass.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/MoveOnTheMap.h"
#include "../../general/base/DatapackGeneralLoader.h"
#include "../base/SqlFunction.h"
#include "../base/DictionaryServer.h"
#include "../base/GlobalServerData.h"

#include <QFile>
#include <QByteArray>
#include <QDomDocument>
#include <QDomElement>

using namespace CatchChallenger;

void BaseServer::preload_plant_on_map_sql()
{
    DebugClass::debugConsole(QStringLiteral("%1 SQL monster max id").arg(GlobalServerData::serverPrivateVariables.maxMonsterId));

    QString queryText;
    switch(GlobalServerData::serverPrivateVariables.db.databaseType())
    {
        default:
        case DatabaseBase::Type::Mysql:
            queryText=QStringLiteral("SELECT `id`,`map`,`x`,`y`,`plant`,`character`,`plant_timestamps` FROM `plant`");
        break;
        case DatabaseBase::Type::SQLite:
            queryText=QStringLiteral("SELECT id,map,x,y,plant,character,plant_timestamps FROM plant");
        break;
        case DatabaseBase::Type::PostgreSQL:
            queryText=QStringLiteral("SELECT id,map,x,y,plant,character,plant_timestamps FROM plant");
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&BaseServer::preload_plant_on_map_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        preload_dictionary_map();
    }
}

void BaseServer::preload_plant_on_map_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_plant_on_map_return();
}

void BaseServer::preload_plant_on_map_return()
{
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        bool ok;
        const quint32 &id=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("Plant id ignored because is not a number: %1").arg(GlobalServerData::serverPrivateVariables.db.value(0)));
            continue;
        }
        if(GlobalServerData::serverPrivateVariables.plantUsedId.contains(id))
        {
            DebugClass::debugConsole(QStringLiteral("Plant id already found: %1").arg(id));
            continue;
        }
        if(GlobalServerData::serverPrivateVariables.plantUnusedId.contains(id))
            GlobalServerData::serverPrivateVariables.plantUnusedId.removeOne(id);
        GlobalServerData::serverPrivateVariables.plantUsedId << id;
        //put the unused id
        if(GlobalServerData::serverPrivateVariables.maxPlantId<id)
        {
            quint32 index=GlobalServerData::serverPrivateVariables.maxPlantId+1;
            while(index<id)
            {
                GlobalServerData::serverPrivateVariables.plantUnusedId << index;
                index++;
            }
            GlobalServerData::serverPrivateVariables.maxPlantId=id;
        }
        const quint32 &map_database_id=QString(GlobalServerData::serverPrivateVariables.db.value(1)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("Plant ignored because the map is not a number"));
            continue;
        }
        if(map_database_id>=(quint32)DictionaryServer::dictionary_map_database_to_internal.size())
        {
            DebugClass::debugConsole(QStringLiteral("Plant ignored because the map not exists (out of range): %1").arg(map_database_id));
            continue;
        }
        MapServer * const mapForPlantOnServer=static_cast<MapServer *>(DictionaryServer::dictionary_map_database_to_internal.at(map_database_id));
        if(mapForPlantOnServer==NULL)
        {
            DebugClass::debugConsole(QStringLiteral("Plant ignored because the map not exists (not found): %1").arg(map_database_id));
            continue;
        }
        if(mapForPlantOnServer->plants.size()>=255)
        {
            DebugClass::debugConsole(QStringLiteral("Plant ignored because the map have 255 or more plant: %1").arg(mapForPlantOnServer->map_file));
            continue;
        }
        const quint8 &x=QString(GlobalServerData::serverPrivateVariables.db.value(2)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("Plant ignored because the x is not a number"));
            continue;
        }
        if(x>=mapForPlantOnServer->width)
        {
            DebugClass::debugConsole(QStringLiteral("Plant ignored because the x>%1 for the map %2: %3").arg(mapForPlantOnServer->width).arg(mapForPlantOnServer->map_file).arg(x));
            continue;
        }
        const quint8 &y=QString(GlobalServerData::serverPrivateVariables.db.value(3)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("Plant ignored because the y is not a number"));
            continue;
        }
        if(y>=mapForPlantOnServer->height)
        {
            DebugClass::debugConsole(QStringLiteral("Plant ignored because the y>%1 for the map %2: %3").arg(mapForPlantOnServer->height).arg(mapForPlantOnServer->map_file).arg(y));
            continue;
        }
        const quint8 &plant=QString(GlobalServerData::serverPrivateVariables.db.value(4)).toUInt(&ok);
        if(!ok)
            continue;
        if(!CommonDatapack::commonDatapack.plants.contains(plant))
        {
            DebugClass::debugConsole(QStringLiteral("Plant dropped to not block the player, due to missing plant into the list: %1").arg(plant));
            continue;
        }
        const quint32 &character=QString(GlobalServerData::serverPrivateVariables.db.value(5)).toUInt(&ok);
        if(!ok)
            continue;
        if(!MoveOnTheMap::isDirt(*mapForPlantOnServer,x,y))
        {
            DebugClass::debugConsole(QStringLiteral("Plant ignored because is not into dirt layer: %1 (%2,%3)").arg(mapForPlantOnServer->map_file).arg(x).arg(y));
            continue;
        }
        const quint64 &plant_timestamps=QString(GlobalServerData::serverPrivateVariables.db.value(6)).toULongLong(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("Plant timestamps is not a number: %1 (%2,%3)").arg(mapForPlantOnServer->map_file).arg(x).arg(y));
            continue;
        }

        //plant_timestamps
        MapServerCrafting::PlantOnMap plantOnMap;
        plantOnMap.id=id;
        plantOnMap.x=x;
        plantOnMap.y=y;
        plantOnMap.plant=plant;
        plantOnMap.character=character;
        plantOnMap.mature_at=plant_timestamps+CommonDatapack::commonDatapack.plants.value(plant).fruits_seconds;
        plantOnMap.player_owned_expire_at=plant_timestamps+CommonDatapack::commonDatapack.plants.value(plant).fruits_seconds+60*60*24;
        mapForPlantOnServer->plants << plantOnMap;
        #ifdef DEBUG_MESSAGE_MAP_PLANTS
        DebugClass::debugConsole(QStringLiteral("put on the map: %1 (%2,%3) the plant: %4, owned by played id: %5, mature at: %6 (%7+%8)")
                                 .arg(map).arg(x).arg(y)
                                 .arg(plant)
                                 .arg(character)
                                 .arg(plantOnMap.mature_at).arg(plant_timestamps).arg(CommonDatapack::commonDatapack.plants.value(plant).fruits_seconds)
                                 );
        #endif
        plant_on_the_map++;
    }

    preload_dictionary_map();
}

void BaseServer::unload_the_plant_on_map()
{
}
