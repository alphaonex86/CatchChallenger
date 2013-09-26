#include "MapController.h"
#include "DatapackClientLoader.h"
#include "../Api_client_real.h"
#include "../../../general/base/CommonDatapack.h"
#include "../../../general/base/GeneralStructures.h"

MapController* MapController::mapController=NULL;

MapController::MapController(const bool &centerOnPlayer,const bool &debugTags,const bool &useCache,const bool &OpenGL) :
    MapControllerMP(centerOnPlayer,debugTags,useCache,OpenGL)
{
    connect(this,&MapController::mapDisplayed,this,&MapController::tryLoadPlantOnMapDisplayed,Qt::QueuedConnection);
}

MapController::~MapController()
{
}

void MapController::connectAllSignals()
{
    MapControllerMP::connectAllSignals();
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::insert_plant,this,&MapController::insert_plant);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::remove_plant,this,&MapController::remove_plant);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::seed_planted,this,&MapController::seed_planted);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::plant_collected,this,&MapController::plant_collected);
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

bool MapController::canGoTo(const CatchChallenger::Direction &direction,CatchChallenger::Map map,COORD_TYPE x,COORD_TYPE y,const bool &checkCollision)
{
    if(!MapVisualiserPlayerWithFight::canGoTo(direction,map,x,y,checkCollision))
        return false;
    CatchChallenger::Map *new_map=&map;
    CatchChallenger::MoveOnTheMap::move(direction,&new_map,&x,&y,false);
    if(all_map[new_map->map_file]->logicalMap.bots.contains(QPair<quint8,quint8>(x,y)))
        return false;
    return true;
}

void MapController::loadBotOnTheMap(MapVisualiserThread::Map_full *parsedMap,const quint32 &botId,const quint8 &x,const quint8 &y,const QString &lookAt,const QString &skin)
{
    Q_UNUSED(botId);
    if(skin.isEmpty())
        return;
    if(!ObjectGroupItem::objectGroupLink.contains(parsedMap->objectGroup))
    {
        qDebug() << QString("loadBotOnTheMap(), ObjectGroupItem::objectGroupLink not contains parsedMap->objectGroup");
        return;
    }
    CatchChallenger::Direction direction;
    int baseTile=-1;
    if(lookAt=="left")
    {
        baseTile=10;
        direction=CatchChallenger::Direction_move_at_left;
    }
    else if(lookAt=="right")
    {
        baseTile=4;
        direction=CatchChallenger::Direction_move_at_right;
    }
    else if(lookAt=="top")
    {
        baseTile=1;
        direction=CatchChallenger::Direction_move_at_top;
    }
    else
    {
        if(lookAt!="bottom")
            CatchChallenger::DebugClass::debugConsole(QString("Wrong direction for the bot at %1 (%2,%3)").arg(parsedMap->logicalMap.map_file).arg(x).arg(y));
        baseTile=7;
        direction=CatchChallenger::Direction_move_at_bottom;
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
    Tiled::Cell cell=parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].mapObject->cell();
    cell.tile=parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].tileset->tileAt(baseTile);
    parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].mapObject->setCell(cell);
    ObjectGroupItem::objectGroupLink[parsedMap->objectGroup]->addObject(parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].mapObject);
    //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
    parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].mapObject->setPosition(QPoint(x,y+1));
    MapObjectItem::objectLink[parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].mapObject]->setZValue(y);

    if(parsedMap->logicalMap.bots[QPair<quint8,quint8>(x,y)].step.contains(1))
    {
        QDomElement stepBot=parsedMap->logicalMap.bots[QPair<quint8,quint8>(x,y)].step[1];
        if(stepBot.hasAttribute("type") && stepBot.attribute("type")=="fight" && stepBot.hasAttribute("fightid"))
        {
            bool ok;
            quint32 fightid=stepBot.attribute("fightid").toUInt(&ok);
            if(ok)
            {
                if(CatchChallenger::CommonDatapack::commonDatapack.botFights.contains(fightid))
                {
                    #ifdef DEBUG_CLIENT_BOT
                    CatchChallenger::DebugClass::debugConsole(QString("Put bot fight point %1 at %2 (%3,%4) in direction: %5").arg(fightid).arg(parsedMap->logicalMap.map_file).arg(x).arg(y).arg(direction));
                    #endif
                    quint8 temp_x=x,temp_y=y;
                    int index_botfight_range=0;
                    CatchChallenger::Map *map=&parsedMap->logicalMap;
                    CatchChallenger::Map *old_map=map;
                    while(index_botfight_range<CATCHCHALLENGER_BOTFIGHT_RANGE)
                    {
                        if(!CatchChallenger::MoveOnTheMap::canGoTo(direction,*map,temp_x,temp_y,true,false))
                            break;
                        if(!CatchChallenger::MoveOnTheMap::move(direction,&map,&temp_x,&temp_y,true,false))
                            break;
                        if(map!=old_map)
                            break;
                        parsedMap->logicalMap.botsFightTrigger.insert(QPair<quint8,quint8>(temp_x,temp_y),fightid);
                        parsedMap->logicalMap.botsFightTriggerExtra.insert(QPair<quint8,quint8>(temp_x,temp_y),QPair<quint8,quint8>(x,y));
                        index_botfight_range++;
                    }
                }
                else
                    CatchChallenger::DebugClass::debugConsole(QString("No fightid %1 at MapController::loadBotOnTheMap").arg(fightid));
            }
        }
    }
}
