#include "MapController.h"
#include "../Api_client_real.h"

MapController* MapController::mapController=NULL;

MapController::MapController(const bool &centerOnPlayer,const bool &debugTags,const bool &useCache,const bool &OpenGL) :
    MapControllerMP(centerOnPlayer,debugTags,useCache,OpenGL)
{
    qRegisterMetaType<CatchChallenger::Plant_collect>("CatchChallenger::Plant_collect");
    connect(CatchChallenger::Api_client_real::client,SIGNAL(insert_plant(quint32,quint16,quint16,quint8,quint16)),this,SLOT(insert_plant(quint32,quint16,quint16,quint8,quint16)));
    connect(CatchChallenger::Api_client_real::client,SIGNAL(remove_plant(quint32,quint16,quint16)),this,SLOT(remove_plant(quint32,quint16,quint16)));
    connect(CatchChallenger::Api_client_real::client,SIGNAL(seed_planted(bool)),this,SLOT(seed_planted(bool)));
    connect(CatchChallenger::Api_client_real::client,SIGNAL(plant_collected(CatchChallenger::Plant_collect)),this,SLOT(plant_collected(CatchChallenger::Plant_collect)));
}

MapController::~MapController()
{
}

void MapController::resetAll()
{
    delayedPlantInsert.clear();
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
}

void MapController::loadBotOnTheMap(Map_full *parsedMap,const quint8 &x,const quint8 &y,const QString &lookAt,const QString &skin)
{
    if(skin.isEmpty())
        return;
    if(!ObjectGroupItem::objectGroupLink.contains(parsedMap->objectGroup))
    {
        qDebug() << QString("loadBotOnTheMap(), ObjectGroupItem::objectGroupLink not contains parsedMap->objectGroup");
        return;
    }
    int baseTile=-1;
    if(lookAt=="left")
        baseTile=10;
    else if(lookAt=="right")
        baseTile=4;
    else if(lookAt=="top")
        baseTile=1;
    else if(lookAt=="bottom")
        baseTile=7;
    else
    {
        CatchChallenger::DebugClass::debugConsole(QString("Wrong direction for the bot at %1 (%2,%3)").arg(parsedMap->logicalMap.map_file).arg(x).arg(y));
        baseTile=7;
    }
    parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].mapObject=new Tiled::MapObject();
    parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].tileset=new Tiled::Tileset("bot",16,24);
    QString skinPath=datapackPath+DATAPACK_BASE_PATH_SKIN+"/"+skin+"/trainer.png";
    if(!parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].tileset->loadFromImage(QImage(skinPath),skinPath))
    {
        qDebug() << "Unable the load the bot tileset";
        if(!parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].tileset->loadFromImage(QImage(":/images/player_default/trainer.png"),":/images/player_default/trainer.png"))
            qDebug() << "Unable the load the default bot tileset";
    }
    parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].mapObject->setTile(parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].tileset->tileAt(baseTile));
    ObjectGroupItem::objectGroupLink[parsedMap->objectGroup]->addObject(parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].mapObject);
    //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
    parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].mapObject->setPosition(QPoint(x,y+1));
    MapObjectItem::objectLink[parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].mapObject]->setZValue(y);
}
