#include "../../base/interface/MapController.h"
#include "../base/interface/DatapackClientLoader.h"

//plant
void MapController::insert_plant(const quint32 &mapId,const quint16 &x,const quint16 &y,const quint8 &plant_id,const quint16 &seconds_to_mature)
{
    if(!mHaveTheDatapack || !player_informations_is_set)
    {
        DelayedPlantInsert tempItem;
        tempItem.mapId=mapId;
        tempItem.x=x;
        tempItem.y=y;
        tempItem.plant_id=plant_id;
        tempItem.seconds_to_mature=seconds_to_mature;
        delayedPlantInsert << tempItem;
        return;
    }
    if(mapId>=(quint32)DatapackClientLoader::datapackLoader.maps.size())
    {
        qDebug() << "MapController::insert_plant() mapId greater than DatapackClientLoader::datapackLoader.maps.size()";
        return;
    }
    if(!DatapackClientLoader::datapackLoader.plants.contains(plant_id))
    {
        qDebug() << "plant_id don't exists";
        return;
    }
    if(!displayed_map.contains(datapackMapPath+DatapackClientLoader::datapackLoader.maps[mapId]))
    {
        QStringList map_list;
        QSetIterator<QString> i(displayed_map);
         while (i.hasNext())
             map_list << i.next();
        qDebug() << QString("map (%1) not show (into map list: %2), ignore it").arg(datapackMapPath+DatapackClientLoader::datapackLoader.maps[mapId]).arg(map_list.join(";"));
        return;
    }
    if(!all_map.contains(datapackMapPath+DatapackClientLoader::datapackLoader.maps[mapId]))
    {
        QStringList map_list;
        QHashIterator<QString, Map_full *> i(all_map);
        while (i.hasNext()) {
            i.next();
            map_list << i.key();
        }
        qDebug() << QString("map (%1) is not into map list: %2, ignore it").arg(datapackMapPath+DatapackClientLoader::datapackLoader.maps[mapId]).arg(map_list.join(";"));
        return;
    }
    Map_full * map_full=all_map[datapackMapPath+DatapackClientLoader::datapackLoader.maps[mapId]];
    int index=0;
    while(index<map_full->logicalMap.plantList.size())
    {
        if(map_full->logicalMap.plantList.at(index).x==x && map_full->logicalMap.plantList.at(index).y==y)
        {
            qDebug() << "map have already an item at this point, remove it";
            remove_plant(mapId,x,y);
        }
        else
            index++;
    }
    quint64 current_time=QDateTime::currentMSecsSinceEpoch()/1000;
    CatchChallenger::Map_client::Plant plant;
    plant.mapObject=new Tiled::MapObject();
    if(seconds_to_mature==0)
        plant.mapObject->setTile(DatapackClientLoader::datapackLoader.plants[plant_id].tileset->tileAt(4));
    else if(seconds_to_mature<(DatapackClientLoader::datapackLoader.plants[plant_id].fruits_seconds-DatapackClientLoader::datapackLoader.plants[plant_id].flowering_seconds))
        plant.mapObject->setTile(DatapackClientLoader::datapackLoader.plants[plant_id].tileset->tileAt(3));
    else if(seconds_to_mature<(DatapackClientLoader::datapackLoader.plants[plant_id].fruits_seconds-DatapackClientLoader::datapackLoader.plants[plant_id].taller_seconds))
        plant.mapObject->setTile(DatapackClientLoader::datapackLoader.plants[plant_id].tileset->tileAt(2));
    else if(seconds_to_mature<(DatapackClientLoader::datapackLoader.plants[plant_id].fruits_seconds-DatapackClientLoader::datapackLoader.plants[plant_id].sprouted_seconds))
        plant.mapObject->setTile(DatapackClientLoader::datapackLoader.plants[plant_id].tileset->tileAt(1));
    else
        plant.mapObject->setTile(DatapackClientLoader::datapackLoader.plants[plant_id].tileset->tileAt(0));
    //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
    plant.mapObject->setPosition(QPoint(x,y+1));
    plant.x=x;
    plant.y=y;
    plant.plant_id=plant_id;
    plant.mature_at=current_time+seconds_to_mature;

    map_full->logicalMap.plantList << plant;
    #ifdef DEBUG_CLIENT_PLANTS
    qDebug() << QString("insert_plant(), map: %1 at: %2,%3").arg(DatapackClientLoader::datapackLoader.maps[mapId]).arg(x).arg(y);
    #endif
    if(ObjectGroupItem::objectGroupLink.contains(all_map[datapackMapPath+DatapackClientLoader::datapackLoader.maps[mapId]]->objectGroup))
        ObjectGroupItem::objectGroupLink[all_map[datapackMapPath+DatapackClientLoader::datapackLoader.maps[mapId]]->objectGroup]->addObject(plant.mapObject);
    else
        qDebug() << QString("insert_plant(), all_map[datapackMapPath+DatapackClientLoader::datapackLoader.maps[mapId]]->objectGroup not contains current_map->objectGroup");
    MapObjectItem::objectLink[plant.mapObject]->setZValue(y);
}

void MapController::remove_plant(const quint32 &mapId,const quint16 &x,const quint16 &y)
{
    if(!mHaveTheDatapack)
    {
        int index=0;
        while(index<delayedPlantInsert.size())
        {
            if(delayedPlantInsert.at(index).mapId==mapId && delayedPlantInsert.at(index).x==x && delayedPlantInsert.at(index).y==y)
            {
                delayedPlantInsert.removeAt(index);
                return;
            }
            index++;
        }
        qDebug() << "MapController::remove_plant() remove item not found into the insert";
        return;
    }
    if(mapId>=(quint32)DatapackClientLoader::datapackLoader.maps.size())
    {
        qDebug() << "MapController::remove_plant() mapId greater than DatapackClientLoader::datapackLoader.maps.size()";
        return;
    }
    #ifdef DEBUG_CLIENT_PLANTS
    qDebug() << QString("remove_plant(%1,%2,%3)").arg(DatapackClientLoader::datapackLoader.maps[mapId]).arg(x).arg(y);
    #endif
    if(!all_map.contains(datapackMapPath+DatapackClientLoader::datapackLoader.maps[mapId]))
    {
        QStringList map_list;
        QHashIterator<QString, Map_full *> i(all_map);
        while (i.hasNext()) {
            i.next();
            map_list << i.key();
        }
        qDebug() << QString("map (%1) is not into map list: %2, ignore it").arg(datapackMapPath+DatapackClientLoader::datapackLoader.maps[mapId]).arg(map_list.join(";"));
        return;
    }
    if(!displayed_map.contains(datapackMapPath+DatapackClientLoader::datapackLoader.maps[mapId]))
    {
        Map_full * map_full=all_map[datapackMapPath+DatapackClientLoader::datapackLoader.maps[mapId]];
        int index=0;
        while(index<all_map[datapackMapPath+DatapackClientLoader::datapackLoader.maps[mapId]]->logicalMap.plantList.size())
        {
            if(map_full->logicalMap.plantList.at(index).x==x && map_full->logicalMap.plantList.at(index).y==y)
            {
                delete map_full->logicalMap.plantList.at(index).mapObject;
                map_full->logicalMap.plantList.removeAt(index);
            }
            else
                index++;
        }
        qDebug() << "map not show, don't remove from the display";
        return;
    }
    Map_full * map_full=all_map[datapackMapPath+DatapackClientLoader::datapackLoader.maps[mapId]];
    int index=0;
    while(index<map_full->logicalMap.plantList.size())
    {
        if(map_full->logicalMap.plantList.at(index).x==x && map_full->logicalMap.plantList.at(index).y==y)
        {
            //unload the player sprite
            if(ObjectGroupItem::objectGroupLink.contains(map_full->logicalMap.plantList.at(index).mapObject->objectGroup()))
                ObjectGroupItem::objectGroupLink[map_full->logicalMap.plantList.at(index).mapObject->objectGroup()]->removeObject(map_full->logicalMap.plantList.at(index).mapObject);
            else
                qDebug() << QString("remove_plant(), ObjectGroupItem::objectGroupLink not contains map_full->logicalMap.plantList.at(index).mapObject->objectGroup()");
            delete map_full->logicalMap.plantList.at(index).mapObject;
            map_full->logicalMap.plantList.removeAt(index);
        }
        else
            index++;
    }
}

void MapController::seed_planted(const bool &ok)
{
    if(!mHaveTheDatapack)
    {
        qDebug() << "Prevent seed_planted before datapack is loaded";
        return;
    }
    Q_UNUSED(ok);
}

void MapController::plant_collected(const CatchChallenger::Plant_collect &stat)
{
    if(!mHaveTheDatapack)
    {
        qDebug() << "Prevent seed_planted before datapack is loaded";
        return;
    }
    Q_UNUSED(stat);
}

void MapController::reinject_signals()
{
    MapControllerMP::reinject_signals();

    int index;

    if(mHaveTheDatapack && player_informations_is_set)
    {
        index=0;
        while(index<delayedPlantInsert.size())
        {
            insert_plant(delayedPlantInsert.at(index).mapId,delayedPlantInsert.at(index).x,delayedPlantInsert.at(index).y,delayedPlantInsert.at(index).plant_id,delayedPlantInsert.at(index).seconds_to_mature);
            index++;
        }
        delayedPlantInsert.clear();
    }
}
