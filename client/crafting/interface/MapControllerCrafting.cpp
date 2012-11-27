#include "../../base/interface/MapController.h"
#include "../base/interface/DatapackClientLoader.h"

//plant
void MapController::insert_plant(const quint32 &mapId,const quint16 &x,const quint16 &y,const quint8 &plant_id,const quint16 &seconds_to_mature)
{
    if(!mHaveTheDatapack)
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
        qDebug() << "mapId greater than DatapackClientLoader::datapackLoader.maps.size()";
        return;
    }
    if(!DatapackClientLoader::datapackLoader.plants.contains(plant_id))
    {
        qDebug() << "plant_id don't exists";
        return;
    }
    if(!displayed_map.contains(DatapackClientLoader::datapackLoader.maps[mapId]))
    {
        qDebug() << "map not show, ignore it";
        return;
    }
    int index=0;
    while(index<plantList.size())
    {
        if(plantList.at(index).x==x && plantList.at(index).y==y && plantList.at(index).mapId==mapId)
        {
            qDebug() << "map have already an item at this point, remove it";
            remove_plant(mapId,x,y);
        }
        else
            index++;
    }
    Plant plant;
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
    plant.x=x;
    plant.y=y;
    plant.plant_id=plant_id;
    plant.mapId=mapId;
    plantList << plant;
    if(ObjectGroupItem::objectGroupLink.contains(all_map[DatapackClientLoader::datapackLoader.maps[mapId]]->objectGroup))
        ObjectGroupItem::objectGroupLink[all_map[DatapackClientLoader::datapackLoader.maps[mapId]]->objectGroup]->addObject(plant.mapObject);
    else
        qDebug() << QString("insert_plant(), all_map[DatapackClientLoader::datapackLoader.maps[mapId]]->objectGroup not contains current_map->objectGroup");
}

void MapController::remove_plant(const quint32 &mapId,const quint16 &x,const quint16 &y)
{
    if(!mHaveTheDatapack)
    {
        DelayedPlantRemove tempItem;
        tempItem.mapId=mapId;
        tempItem.x=x;
        tempItem.y=y;
        delayedPlantRemove << tempItem;
        return;
    }
    if(mapId>=(quint32)DatapackClientLoader::datapackLoader.maps.size())
    {
        qDebug() << "mapId greater than DatapackClientLoader::datapackLoader.maps.size()";
        return;
    }
    if(!displayed_map.contains(DatapackClientLoader::datapackLoader.maps[mapId]))
    {
        int index=0;
        while(index<plantList.size())
        {
            if(plantList.at(index).x==x && plantList.at(index).y==y && plantList.at(index).mapId==mapId)
            {
                delete plantList.at(index).mapObject;
                plantList.removeAt(index);
            }
            else
                index++;
        }
        qDebug() << "map not show, don't remove fromt the display";
        return;
    }
    int index=0;
    while(index<plantList.size())
    {
        if(plantList.at(index).x==x && plantList.at(index).y==y && plantList.at(index).mapId==mapId)
        {
            //unload the player sprite
            if(ObjectGroupItem::objectGroupLink.contains(plantList.at(index).mapObject->objectGroup()))
                ObjectGroupItem::objectGroupLink[plantList.at(index).mapObject->objectGroup()]->removeObject(plantList.at(index).mapObject);
            else
                qDebug() << QString("remove_plant(), ObjectGroupItem::objectGroupLink not contains plantList.at(index).mapObject->objectGroup()");
            delete plantList.at(index).mapObject;
            plantList.removeAt(index);
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

void MapController::plant_collected(const Pokecraft::Plant_collect &stat)
{
    if(!mHaveTheDatapack)
    {
        qDebug() << "Prevent seed_planted before datapack is loaded";
        return;
    }
    Q_UNUSED(stat);
}
