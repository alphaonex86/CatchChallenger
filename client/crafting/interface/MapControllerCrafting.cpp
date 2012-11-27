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
