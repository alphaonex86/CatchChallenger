#include "MapController.h"

MapController::MapController(Pokecraft::Api_protocol *client,const bool &centerOnPlayer,const bool &debugTags,const bool &useCache,const bool &OpenGL) :
    MapControllerMP(client,centerOnPlayer,debugTags,useCache,OpenGL)
{
    qRegisterMetaType<Pokecraft::Plant_collect>("Pokecraft::Plant_collect");
    connect(client,SIGNAL(insert_plant(quint32,quint16,quint16,quint8,quint16)),this,SLOT(insert_plant(quint32,quint16,quint16,quint8,quint16)));
    connect(client,SIGNAL(remove_plant(quint32,quint16,quint16)),this,SLOT(remove_plant(quint32,quint16,quint16)));
    connect(client,SIGNAL(seed_planted(bool)),this,SLOT(seed_planted(bool)));
    connect(client,SIGNAL(plant_collected(Pokecraft::Plant_collect)),this,SLOT(plant_collected(Pokecraft::Plant_collect)));
}

MapController::~MapController()
{
}

void MapController::resetAll()
{
    delayedPlantInsert.clear();
    delayedPlantRemove.clear();
    MapControllerMP::resetAll();
}

void MapController::datapackParsed()
{
    if(mHaveTheDatapack)
        return;
    MapControllerMP::datapackParsed();
    int index;

    index=0;
    while(index<delayedPlantInsert.size())
    {
        insert_plant(delayedPlantInsert.at(index).mapId,delayedPlantInsert.at(index).x,delayedPlantInsert.at(index).y,delayedPlantInsert.at(index).plant_id,delayedPlantInsert.at(index).seconds_to_mature);
        index++;
    }
    delayedPlantInsert.clear();

    index=0;
    while(index<delayedPlantRemove.size())
    {
        remove_plant(delayedPlantRemove.at(index).mapId,delayedPlantRemove.at(index).x,delayedPlantRemove.at(index).y);
        index++;
    }
    delayedPlantRemove.clear();
}
