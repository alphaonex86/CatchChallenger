#include "BaseServerCrafting.h"
#include "MapServerCrafting.h"
#include "../VariableServer.h"
#include "../base/MapServer.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/DebugClass.h"
#include "../../general/base/MoveOnTheMap.h"
#include "../base/SqlFunction.h"

#include <QFile>
#include <QByteArray>
#include <QDomDocument>
#include <QDomElement>

using namespace Pokecraft;

void BaseServerCrafting::preload_the_plant()
{
    //open and quick check the file
    QFile plantsFile(GlobalData::serverPrivateVariables.datapack_basePath+DATAPACK_BASE_PATH_PLANTS+"plants.xml");
    QByteArray xmlContent;
    if(!plantsFile.open(QIODevice::ReadOnly))
    {
        DebugClass::debugConsole(QString("Unable to open the plants file: %1, error: %2").arg(plantsFile.fileName()).arg(plantsFile.errorString()));
        return;
    }
    xmlContent=plantsFile.readAll();
    plantsFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        DebugClass::debugConsole(QString("Unable to open the plants file: %1, Parse error at line %2, column %3: %4").arg(plantsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="plants")
    {
        DebugClass::debugConsole(QString("Unable to open the plants file: %1, \"plants\" root balise not found for the xml file").arg(plantsFile.fileName()));
        return;
    }

    //load the content
    bool ok,ok2;
    QDomElement plantItem = root.firstChildElement("plant");
    while(!plantItem.isNull())
    {
        if(plantItem.isElement())
        {
            if(plantItem.hasAttribute("id") && plantItem.hasAttribute("itemUsed"))
            {
                quint8 id=plantItem.attribute("id").toULongLong(&ok);
                quint32 itemUsed=plantItem.attribute("itemUsed").toULongLong(&ok2);
                if(ok && ok2)
                {
                    if(!GlobalData::serverPrivateVariables.plants.contains(id))
                    {
                        ok=false;
                        Plant plant;
                        plant.itemUsed=itemUsed;
                        QDomElement grow = plantItem.firstChildElement("grow");
                        if(!grow.isNull())
                            if(grow.isElement())
                            {
                                QDomElement fruits = grow.firstChildElement("fruits");
                                if(!fruits.isNull())
                                    if(fruits.isElement())
                                    {
                                        plant.mature_seconds=fruits.text().toULongLong(&ok2)*60;
                                        if(ok2)
                                            ok=true;
                                    }
                            }
                        if(ok)
                        {
                            QDomElement quantity = plantItem.firstChildElement("quantity");
                            if(!quantity.isNull())
                                if(quantity.isElement())
                                {
                                    plant.quantity=quantity.text().toFloat(&ok2);
                                    if(ok2)
                                        ok=true;
                                }
                        }
                        if(ok)
                            GlobalData::serverPrivateVariables.plants[id]=plant;
                    }
                    else
                        DebugClass::debugConsole(QString("Unable to open the plants file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber()));
                }
                else
                    DebugClass::debugConsole(QString("Unable to open the plants file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber()));
            }
            else
                DebugClass::debugConsole(QString("Unable to open the plants file: %1, have not the plant id: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber()));
        }
        else
            DebugClass::debugConsole(QString("Unable to open the plants file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber()));
        plantItem = plantItem.nextSiblingElement("plant");
    }

    DebugClass::debugConsole(QString("%1 plant(s) loaded").arg(GlobalData::serverPrivateVariables.plants.size()));
}

void BaseServerCrafting::preload_the_plant_on_map()
{
    int plant_on_the_map=0;
    QString queryText;
    switch(GlobalData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("SELECT map,x,y,plant,player_id,plant_timestamps FROM plant");
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT map,x,y,plant,player_id,plant_timestamps FROM plant");
        break;
    }
    QSqlQuery plantOnMapQuery(queryText);
    if(plantOnMapQuery.isValid())
        DebugClass::debugConsole(QString("SQL query is not valid: %1").arg(queryText));
    while(plantOnMapQuery.next())
    {
        bool ok;
        QString map=plantOnMapQuery.value(0).toString();
        if(!GlobalData::serverPrivateVariables.map_list.contains(map))
        {
            DebugClass::debugConsole(QString("Plant ignored because the map not exists: %1").arg(map));
            continue;
        }
        quint8 x=plantOnMapQuery.value(1).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QString("Plant ignored because the x is not a number"));
            continue;
        }
        if(x>=GlobalData::serverPrivateVariables.map_list[map]->width)
        {
            DebugClass::debugConsole(QString("Plant ignored because the x>%1 for the map %2: %3").arg(GlobalData::serverPrivateVariables.map_list[map]->width).arg(map).arg(x));
            continue;
        }
        quint8 y=plantOnMapQuery.value(2).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QString("Plant ignored because the y is not a number"));
            continue;
        }
        if(y>=GlobalData::serverPrivateVariables.map_list[map]->height)
        {
            DebugClass::debugConsole(QString("Plant ignored because the y>%1 for the map %2: %3").arg(GlobalData::serverPrivateVariables.map_list[map]->height).arg(map).arg(y));
            continue;
        }
        quint8 plant=plantOnMapQuery.value(3).toUInt(&ok);
        if(!ok)
            continue;
        if(!GlobalData::serverPrivateVariables.plants.contains(plant))
        {
            DebugClass::debugConsole(QString("Plant dropped to not block the player, due to missing plant into the list: %1").arg(plant));
            remove_plant_on_map(map,x,y);
            continue;
        }
        quint32 player_id=plantOnMapQuery.value(4).toUInt(&ok);
        if(!ok)
            continue;
        if(!MoveOnTheMap::isDirt(*GlobalData::serverPrivateVariables.map_list[map],x,y))
        {
            DebugClass::debugConsole(QString("Plant ignored because coor is no dirt: %1 (%2,%3)").arg(map).arg(x).arg(y));
            continue;
        }
        quint64 plant_timestamps=plantOnMapQuery.value(5).toULongLong(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QString("Plant timestamps is not a number: %1 (%2,%3)").arg(map).arg(x).arg(y));
            continue;
        }

        //plant_timestamps
        MapServerCrafting::PlantOnMap plantOnMap;
        plantOnMap.x=x;
        plantOnMap.y=y;
        plantOnMap.plant=plant;
        plantOnMap.player_id=player_id;
        plantOnMap.mature_at=plant_timestamps+GlobalData::serverPrivateVariables.plants[plant].mature_seconds;
        plantOnMap.player_owned_expire_at=plant_timestamps+GlobalData::serverPrivateVariables.plants[plant].mature_seconds+60*60*24;
        static_cast<MapServer *>(GlobalData::serverPrivateVariables.map_list[map])->plants << plantOnMap;
        #ifdef DEBUG_MESSAGE_MAP_PLANTS
        DebugClass::debugConsole(QString("put on the map: %1 (%2,%3) the plant: %4, owned by played id: %5, mature at: %6 (%7+%8)")
                                 .arg(map).arg(x).arg(y)
                                 .arg(plant)
                                 .arg(player_id)
                                 .arg(plantOnMap.mature_at).arg(plant_timestamps).arg(GlobalData::serverPrivateVariables.plants[plant].mature_seconds)
                                 );
        #endif
        plant_on_the_map++;
    }

    DebugClass::debugConsole(QString("%1 plant(s) on the map loaded").arg(plant_on_the_map));
}

void BaseServerCrafting::remove_plant_on_map(const QString &map,const quint8 &x,const quint8 &y)
{
    QString queryText;
    switch(GlobalData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("DELETE FROM plant WHERE map=\"%1\" AND x=%2 AND y=%3")
                .arg(SqlFunction::quoteSqlVariable(map))
                .arg(x)
                .arg(y);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
        queryText=QString("DELETE FROM plant WHERE map=\"%1\" AND x=%2 AND y=%3")
            .arg(SqlFunction::quoteSqlVariable(map))
            .arg(x)
            .arg(y);
        break;
    }
    QSqlQuery removePlantOnMapQuery(queryText);
    Q_UNUSED(removePlantOnMapQuery);
}

void BaseServerCrafting::unload_the_plant()
{
    GlobalData::serverPrivateVariables.plants.clear();
}

void BaseServerCrafting::unload_the_plant_on_map()
{
}
