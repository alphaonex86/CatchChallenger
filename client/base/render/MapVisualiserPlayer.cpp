#include "MapVisualiserPlayer.h"

#include "../../../general/base/MoveOnTheMap.h"
#include "../../../general/base/CommonDatapack.h"
#include "../../../general/base/CommonDatapackServerSpec.h"
#include "../../../general/base/CommonSettingsCommon.h"
#include "../../../general/base/CommonSettingsServer.h"
#include "../interface/DatapackClientLoader.h"
#include "../../../general/base/GeneralVariable.h"

#include <qmath.h>
#include <QFileInfo>
#include <QMessageBox>
#include <QDebug>

QString MapVisualiserPlayer::text_slashtrainerpng=QLatin1Literal("/trainer.png");
QString MapVisualiserPlayer::text_slash=QLatin1Literal("/");
QString MapVisualiserPlayer::text_antislash=QLatin1Literal("\\");
QString MapVisualiserPlayer::text_dotpng=QLatin1Literal(".png");
QString MapVisualiserPlayer::text_type=QLatin1Literal("type");
QString MapVisualiserPlayer::text_zone=QLatin1Literal("zone");
QString MapVisualiserPlayer::text_backgroundsound=QLatin1Literal("backgroundsound");

/* why send the look at because blocked into the wall?
to be sync if connexion is stop, but use more bandwith
To not send: store "is blocked but direction not send", cautch the close event, at close: if "is blocked but direction not send" then send it
*/

MapVisualiserPlayer::MapVisualiserPlayer(const bool &centerOnPlayer,const bool &debugTags,const bool &useCache,const bool &OpenGL) :
    MapVisualiser(debugTags,useCache,OpenGL)
{
    blocked=false;
    inMove=false;
    teleportedOnPush=false;
    x=0;
    y=0;
    events=NULL;
    items=NULL;
    quests=NULL;
    itemOnMap=NULL;
    plantOnMap=NULL;
    animationDisplayed=false;

    keyAccepted << Qt::Key_Left << Qt::Key_Right << Qt::Key_Up << Qt::Key_Down << Qt::Key_Return;

    lookToMove.setInterval(200);
    lookToMove.setSingleShot(true);
    connect(&lookToMove,&QTimer::timeout,this,&MapVisualiserPlayer::transformLookToMove);
    connect(this,&MapVisualiserPlayer::mapDisplayed,this,&MapVisualiserPlayer::mapDisplayedSlot);

    currentPlayerSpeed=250;
    moveTimer.setInterval(currentPlayerSpeed/5);
    moveTimer.setSingleShot(true);
    connect(&moveTimer,&QTimer::timeout,this,&MapVisualiserPlayer::moveStepSlot);
    moveAnimationTimer.setInterval(currentPlayerSpeed/5);
    moveAnimationTimer.setSingleShot(true);
    connect(&moveAnimationTimer,&QTimer::timeout,this,&MapVisualiserPlayer::doMoveAnimation);

    this->centerOnPlayer=centerOnPlayer;

    if(centerOnPlayer)
    {
        setSceneRect(-2000,-2000,4000,4000);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
    stepAlternance=false;
    animationTileset=new Tiled::Tileset(QLatin1Literal("animation"),16,16);
    nextCurrentObject=new Tiled::MapObject();
    grassCurrentObject=new Tiled::MapObject();
    grassCurrentObject->setName("grassCurrentObject");
    haveGrassCurrentObject=false;
    haveNextCurrentObject=false;

    defaultTileset="trainer";
    playerMapObject = new Tiled::MapObject();
    grassCurrentObject->setName("playerMapObject");

    lastTileset=defaultTileset;
    playerTileset = new Tiled::Tileset(QLatin1Literal("player"),16,24);
    playerTilesetCache[lastTileset]=playerTileset;
}

MapVisualiserPlayer::~MapVisualiserPlayer()
{
    delete animationTileset;
    delete nextCurrentObject;
    delete grassCurrentObject;
    delete playerMapObject;
    //delete playerTileset;
    QSet<Tiled::Tileset *> deletedTileset;
    QHashIterator<QString,Tiled::Tileset *> i(playerTilesetCache);
    while (i.hasNext()) {
        i.next();
        if(!deletedTileset.contains(i.value()))
        {
            deletedTileset << i.value();
            delete i.value();
        }
    }
}

bool MapVisualiserPlayer::haveMapInMemory(const QString &mapPath)
{
    return all_map.contains(mapPath) || old_all_map.contains(mapPath);
}

void MapVisualiserPlayer::keyPressEvent(QKeyEvent * event)
{
    if(current_map.isEmpty() || !all_map.contains(current_map))
        return;

    //ignore the no arrow key
    if(!keyAccepted.contains(event->key()))
    {
        event->ignore();
        return;
    }

    //ignore the repeated event
    if(event->isAutoRepeat())
        return;

    //add to pressed key list
    keyPressed << event->key();

    //apply the key
    keyPressParse();
}

void MapVisualiserPlayer::keyPressParse()
{
    //ignore is already in move
    if(inMove || blocked)
        return;

    if(keyPressed.size()==1 && keyPressed.contains(Qt::Key_Return))
    {
        keyPressed.remove(Qt::Key_Return);
        parseAction();
        return;
    }

    if(keyPressed.contains(Qt::Key_Left))
    {
        //already turned on this direction, then try move into this direction
        if(direction==CatchChallenger::Direction_look_at_left)
        {
            if(!canGoTo(CatchChallenger::Direction_move_at_left,all_map.value(current_map)->logicalMap,x,y,true))
                return;//Can't do at the left!
            //the first step
            direction=CatchChallenger::Direction_move_at_left;
            inMove=true;
            moveStep=1;
            moveStepSlot();
            emit send_player_direction(direction);
            //startGrassAnimation(direction);
        }
        //look in this direction
        else
        {
            Tiled::Cell cell=playerMapObject->cell();
            cell.tile=playerTileset->tileAt(10);
            playerMapObject->setCell(cell);
            direction=CatchChallenger::Direction_look_at_left;
            lookToMove.start();
            emit send_player_direction(direction);
            parseStop();
        }
    }
    else if(keyPressed.contains(Qt::Key_Right))
    {
        //already turned on this direction, then try move into this direction
        if(direction==CatchChallenger::Direction_look_at_right)
        {
            if(!canGoTo(CatchChallenger::Direction_move_at_right,all_map.value(current_map)->logicalMap,x,y,true))
                return;//Can't do at the right!
            //the first step
            direction=CatchChallenger::Direction_move_at_right;
            inMove=true;
            moveStep=1;
            moveStepSlot();
            emit send_player_direction(direction);
            //startGrassAnimation(direction);
        }
        //look in this direction
        else
        {
            Tiled::Cell cell=playerMapObject->cell();
            cell.tile=playerTileset->tileAt(4);
            playerMapObject->setCell(cell);
            direction=CatchChallenger::Direction_look_at_right;
            lookToMove.start();
            emit send_player_direction(direction);
            parseStop();
        }
    }
    else if(keyPressed.contains(Qt::Key_Up))
    {
        //already turned on this direction, then try move into this direction
        if(direction==CatchChallenger::Direction_look_at_top)
        {
            if(!canGoTo(CatchChallenger::Direction_move_at_top,all_map.value(current_map)->logicalMap,x,y,true))
                return;//Can't do at the top!
            //the first step
            direction=CatchChallenger::Direction_move_at_top;
            inMove=true;
            moveStep=1;
            moveStepSlot();
            emit send_player_direction(direction);
            //startGrassAnimation(direction);
        }
        //look in this direction
        else
        {
            Tiled::Cell cell=playerMapObject->cell();
            cell.tile=playerTileset->tileAt(1);
            playerMapObject->setCell(cell);
            direction=CatchChallenger::Direction_look_at_top;
            lookToMove.start();
            emit send_player_direction(direction);
            parseStop();
        }
    }
    else if(keyPressed.contains(Qt::Key_Down))
    {
        //already turned on this direction, then try move into this direction
        if(direction==CatchChallenger::Direction_look_at_bottom)
        {
            if(!canGoTo(CatchChallenger::Direction_move_at_bottom,all_map.value(current_map)->logicalMap,x,y,true))
                return;//Can't do at the bottom!
            //the first step
            direction=CatchChallenger::Direction_move_at_bottom;
            inMove=true;
            moveStep=1;
            moveStepSlot();
            emit send_player_direction(direction);
            //startGrassAnimation(direction);
        }
        //look in this direction
        else
        {
            Tiled::Cell cell=playerMapObject->cell();
            cell.tile=playerTileset->tileAt(7);
            playerMapObject->setCell(cell);
            direction=CatchChallenger::Direction_look_at_bottom;
            lookToMove.start();
            emit send_player_direction(direction);
            parseStop();
        }
    }
}

void MapVisualiserPlayer::doMoveAnimation()
{
    moveStepSlot();
}

void MapVisualiserPlayer::moveStepSlot()
{
    MapVisualiserThread::Map_full * map_full=all_map.value(current_map);
    if(!animationDisplayed)
    {
        //leave
        if(map_full->triggerAnimations.contains(QPair<uint8_t,uint8_t>(x,y)))
        {
            TriggerAnimation* triggerAnimation=map_full->triggerAnimations.value(QPair<uint8_t,uint8_t>(x,y));
            triggerAnimation->startLeave();
        }
        animationDisplayed=true;
        //tiger the next tile
        CatchChallenger::CommonMap * map=&map_full->logicalMap;
        uint8_t x=this->x;
        uint8_t y=this->y;
        //set the final value (direction, position, ...)
        switch(direction)
        {
            case CatchChallenger::Direction_move_at_right:
            case CatchChallenger::Direction_move_at_top:
            case CatchChallenger::Direction_move_at_bottom:
            case CatchChallenger::Direction_move_at_left:
                CatchChallenger::MoveOnTheMap::move(direction,&map,&x,&y);
            break;
            default:
            break;
        }
        //enter
        if(map_full->triggerAnimations.contains(QPair<uint8_t,uint8_t>(x,y)))
        {
            TriggerAnimation* triggerAnimation=map_full->triggerAnimations.value(QPair<uint8_t,uint8_t>(x,y));
            triggerAnimation->startEnter();
        }
        //door
        if(map_full->doors.contains(QPair<uint8_t,uint8_t>(x,y)))
        {
            MapDoor* door=map_full->doors.value(QPair<uint8_t,uint8_t>(x,y));
            door->startOpen(currentPlayerSpeed);
            moveAnimationTimer.start(door->timeToOpen());

            //block but set good look direction
            uint8_t baseTile;
            switch(direction)
            {
                case CatchChallenger::Direction_move_at_left:
                baseTile=10;
                break;
                case CatchChallenger::Direction_move_at_right:
                baseTile=4;
                break;
                case CatchChallenger::Direction_move_at_top:
                baseTile=1;
                break;
                case CatchChallenger::Direction_move_at_bottom:
                baseTile=7;
                break;
                default:
                qDebug() << QStringLiteral("moveStepSlot(): moveStep: %1, wrong direction").arg(moveStep);
                return;
            }
            Tiled::Cell cell=playerMapObject->cell();
            cell.tile=playerTileset->tileAt(baseTile+0);
            playerMapObject->setCell(cell);
            return;
        }
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(!moveTimer.isSingleShot())
    {
        qDebug() << QStringLiteral("moveTimer is not in single shot").arg(moveStep);
        moveTimer.setSingleShot(true);
    }
    #endif
    //moveTimer.stop();
    int baseTile=1;
    //move the player for intermediate step and define the base tile (define the stopped step with direction)
    switch(direction)
    {
        case CatchChallenger::Direction_move_at_left:
        baseTile=10;
        switch(moveStep)
        {
            case 1:
            case 2:
            case 3:
            case 4:
            playerMapObject->setX(playerMapObject->x()-0.20);
            break;
        }
        break;
        case CatchChallenger::Direction_move_at_right:
        baseTile=4;
        switch(moveStep)
        {
            case 1:
            case 2:
            case 3:
            case 4:
            playerMapObject->setX(playerMapObject->x()+0.20);
            break;
        }
        break;
        case CatchChallenger::Direction_move_at_top:
        baseTile=1;
        switch(moveStep)
        {
            case 1:
            case 2:
            case 3:
            case 4:
            playerMapObject->setY(playerMapObject->y()-0.20);
            break;
        }
        break;
        case CatchChallenger::Direction_move_at_bottom:
        baseTile=7;
        switch(moveStep)
        {
            case 1:
            case 2:
            case 3:
            case 4:
            playerMapObject->setY(playerMapObject->y()+0.20);
            break;
        }
        break;
        default:
        qDebug() << QStringLiteral("moveStepSlot(): moveStep: %1, wrong direction").arg(moveStep);
        return;
    }

    //apply the right step of the base step defined previously by the direction
    switch(moveStep%5)
    {
        //stopped step
        case 0:
        {
            Tiled::Cell cell=playerMapObject->cell();
            cell.tile=playerTileset->tileAt(baseTile+0);
            playerMapObject->setCell(cell);
        }
        break;
        case 1:
        MapObjectItem::objectLink.value(playerMapObject)->setZValue(qCeil(playerMapObject->y()));
        break;
        //transition step
        case 2:
        {
            Tiled::Cell cell=playerMapObject->cell();
            if(stepAlternance)
                cell.tile=playerTileset->tileAt(baseTile-1);
            else
                cell.tile=playerTileset->tileAt(baseTile+1);
            playerMapObject->setCell(cell);
            stepAlternance=!stepAlternance;
        }
        break;
        //stopped step
        case 4:
        {
            Tiled::Cell cell=playerMapObject->cell();
            cell.tile=playerTileset->tileAt(baseTile+0);
            playerMapObject->setCell(cell);
        }
        break;
    }

    if(centerOnPlayer)
        centerOn(MapObjectItem::objectLink.value(playerMapObject));
    loadGrassTile();

    moveStep++;

    //if have finish the step
    if(moveStep>5)
    {
        animationDisplayed=false;
        CatchChallenger::CommonMap * map=&all_map.value(current_map)->logicalMap;
        const CatchChallenger::CommonMap * old_map=map;
        //set the final value (direction, position, ...)
        switch(direction)
        {
            case CatchChallenger::Direction_move_at_left:
            case CatchChallenger::Direction_move_at_right:
            case CatchChallenger::Direction_move_at_top:
            case CatchChallenger::Direction_move_at_bottom:
                if(!CatchChallenger::MoveOnTheMap::move(direction,&map,&x,&y))
                {
                    qDebug() << "Bug at move";
                    return;
                }
                direction=CatchChallenger::MoveOnTheMap::directionToDirectionLook(direction);
            break;
            default:
                qDebug() << QStringLiteral("moveStepSlot(): moveStep: %1, wrong direction (%2) when moveStep>2").arg(moveStep).arg(direction);
            return;
        }
        //if the map have changed
        if(old_map!=map)
        {
            unloadPlayerFromCurrentMap();
            passMapIntoOld();
            current_map=QString::fromStdString(map->map_file);
            if(old_all_map.find(current_map)==old_all_map.cend())
                emit inWaitingOfMap();
            loadOtherMap(current_map);
            hideNotloadedMap();
            return;
        }
        else
        {
            if(!blocked)
                finalPlayerStep();
        }
    }
    else
        moveTimer.start();
}

bool MapVisualiserPlayer::asyncMapLoaded(const QString &fileName,MapVisualiserThread::Map_full * tempMapObject)
{
    if(current_map.isEmpty())
        return false;
    if(MapVisualiser::asyncMapLoaded(fileName,tempMapObject))
    {
        if(tempMapObject!=NULL)
        {
            int index=0;
            while(index<tempMapObject->tiledMap->layerCount())
            {
                if(Tiled::ObjectGroup *objectGroup = tempMapObject->tiledMap->layerAt(index)->asObjectGroup())
                {
                    if(objectGroup->name()==MapVisualiserThread::text_Object)
                    {
                        QList<Tiled::MapObject*> objects=objectGroup->objects();
                        int index2=0;
                        while(index2<objects.size())
                        {
                            Tiled::MapObject* object=objects.at(index2);
                            const uint32_t &x=object->x();
                            const uint32_t &y=object->y()-1;

                            if(object->type()==MapVisualiserThread::text_object)
                            {
                                //found into the logical map
                                if(tempMapObject->logicalMap.itemsOnMap.contains(QPair<uint8_t,uint8_t>(x,y)))
                                {
                                    if(object->property(MapVisualiserThread::text_visible)==MapVisualiserThread::text_false)
                                    {
                                        //The tiled object not exist on this layer
                                        ObjectGroupItem::objectGroupLink.value(objectGroup)->removeObject(object);
                                        tempMapObject->logicalMap.itemsOnMap[QPair<uint8_t,uint8_t>(x,y)].tileObject=NULL;
                                        objects.removeAt(index2);
                                    }
                                    else
                                    {
                                        const QString tempMap=QString::fromStdString(tempMapObject->logicalMap.map_file);
                                        if(DatapackClientLoader::datapackLoader.itemOnMap.contains(tempMap))
                                        {
                                            if(DatapackClientLoader::datapackLoader.itemOnMap.value(tempMap).contains(QPair<uint8_t,uint8_t>(x,y)))
                                            {
                                                const uint8_t &itemIndex=DatapackClientLoader::datapackLoader.itemOnMap.value(tempMap).value(QPair<uint8_t,uint8_t>(x,y));
                                                if(itemOnMap->find(itemIndex)!=itemOnMap->cend())
                                                {
                                                    ObjectGroupItem::objectGroupLink.value(objectGroup)->removeObject(object);
                                                    objects.removeAt(index2);
                                                }
                                                else
                                                {
                                                    tempMapObject->logicalMap.itemsOnMap[QPair<uint8_t,uint8_t>(x,y)].tileObject=object;
                                                    index2++;
                                                }
                                            }
                                            else
                                            {
                                                tempMapObject->logicalMap.itemsOnMap[QPair<uint8_t,uint8_t>(x,y)].tileObject=object;
                                                index2++;
                                            }
                                        }
                                        else
                                        {
                                            tempMapObject->logicalMap.itemsOnMap[QPair<uint8_t,uint8_t>(x,y)].tileObject=object;
                                            index2++;
                                        }
                                    }
                                }
                                else
                                {
                                    ObjectGroupItem::objectGroupLink.value(objectGroup)->removeObject(object);
                                    objects.removeAt(index2);
                                }
                            }
                            else
                                index2++;
                        }
                    }
                }
                index++;
            }

            if(fileName==current_map)
            {
                if(tempMapObject!=NULL)
                    finalPlayerStep();
                else
                {
                    emit errorWithTheCurrentMap();
                    return false;
                }
            }
            return true;
        }
        else
            return false;
    }
    else
        return false;
}

void MapVisualiserPlayer::setInformations(std::unordered_map<uint16_t, uint32_t> *items, std::unordered_map<uint16_t, CatchChallenger::PlayerQuest> *quests, std::vector<uint8_t> *events, std::unordered_set<uint8_t> *itemOnMap, std::unordered_map<uint8_t, CatchChallenger::PlayerPlant> *plantOnMap)
{
    this->events=events;
    this->items=items;
    this->quests=quests;
    this->itemOnMap=itemOnMap;
    this->plantOnMap=plantOnMap;
}

void MapVisualiserPlayer::unblock()
{
    blocked=false;
}

void MapVisualiserPlayer::finalPlayerStep()
{
    if(!all_map.contains(current_map))
    {
        qDebug() << "current map not loaded, unable to do finalPlayerStep()";
        return;
    }
    const MapVisualiserThread::Map_full * current_map_pointer=all_map.value(current_map);
    if(current_map_pointer==NULL)
    {
        qDebug() << "current map not loaded null pointer, unable to do finalPlayerStep()";
        return;
    }

    {
        const CatchChallenger::MonstersCollisionValue &monstersCollisionValue=CatchChallenger::MoveOnTheMap::getZoneCollision(current_map_pointer->logicalMap,x,y);
        unsigned int index=0;
        while(index<monstersCollisionValue.walkOn.size())
        {
            const CatchChallenger::MonstersCollision &monstersCollision=CatchChallenger::CommonDatapack::commonDatapack.monstersCollision.at(monstersCollisionValue.walkOn.at(index));
            if(monstersCollision.item==0 || items->find(monstersCollision.item)!=items->cend())
            {
                if(monstersCollision.tile!=lastTileset.toStdString())
                {
                    lastTileset=QString::fromStdString(monstersCollision.tile);
                    if(playerTilesetCache.contains(lastTileset))
                        playerTileset=playerTilesetCache.value(lastTileset);
                    else
                    {
                        if(lastTileset.isEmpty())
                            playerTileset=playerTilesetCache[defaultTileset];
                        else
                        {
                            const QString &imagePath=playerSkinPath+MapVisualiserPlayer::text_slash+lastTileset+MapVisualiserPlayer::text_dotpng;
                            QImage image(imagePath);
                            if(!image.isNull())
                            {
                                playerTileset = new Tiled::Tileset(lastTileset,16,24);
                                playerTileset->loadFromImage(image,imagePath);
                            }
                            else
                            {
                                qDebug() << "Unable to load the player tilset: "+imagePath;
                                playerTileset=playerTilesetCache[defaultTileset];
                            }
                        }
                        playerTilesetCache[lastTileset]=playerTileset;
                    }
                    {
                        Tiled::Cell cell=playerMapObject->cell();
                        int tileId=cell.tile->id();
                        cell.tile=playerTileset->tileAt(tileId);
                        playerMapObject->setCell(cell);
                    }
                }
                break;
            }
            index++;
        }
        if(index==monstersCollisionValue.walkOn.size())
        {
            lastTileset=defaultTileset;
            playerTileset=playerTilesetCache[defaultTileset];
            {
                Tiled::Cell cell=playerMapObject->cell();
                int tileId=cell.tile->id();
                cell.tile=playerTileset->tileAt(tileId);
                playerMapObject->setCell(cell);
            }
        }
    }
    //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
    playerMapObject->setPosition(QPoint(x,y+1));
    MapObjectItem::objectLink.value(playerMapObject)->setZValue(y);
    if(centerOnPlayer)
        centerOn(MapObjectItem::objectLink.value(playerMapObject));
    //stopGrassAnimation();

    if(haveStopTileAction())
        return;

    if(CatchChallenger::MoveOnTheMap::getLedge(current_map_pointer->logicalMap,x,y)!=CatchChallenger::ParsedLayerLedges_NoLedges)
    {
        switch(direction)
        {
            case CatchChallenger::Direction_look_at_left:
                direction=CatchChallenger::Direction_move_at_left;
            break;
            case CatchChallenger::Direction_look_at_right:
                direction=CatchChallenger::Direction_move_at_right;
            break;
            case CatchChallenger::Direction_look_at_top:
                direction=CatchChallenger::Direction_move_at_top;
            break;
            case CatchChallenger::Direction_look_at_bottom:
                direction=CatchChallenger::Direction_move_at_bottom;
            break;
            default:
                qDebug() << QStringLiteral("moveStepSlot(): direction: %1, wrong direction").arg(direction);
            return;
        }
        moveStep=0;
        moveTimer.start();
        //startGrassAnimation(direction);
        return;
    }

    //check if one arrow key is pressed to continue to move into this direction
    if(keyPressed.contains(Qt::Key_Left))
    {
        //can't go into this direction, then just look into this direction
        if(!canGoTo(CatchChallenger::Direction_move_at_left,current_map_pointer->logicalMap,x,y,true))
        {
            keyPressed.remove(Qt::Key_Left);
            direction=CatchChallenger::Direction_look_at_left;
            Tiled::Cell cell=playerMapObject->cell();
            cell.tile=playerTileset->tileAt(10);
            playerMapObject->setCell(cell);
            inMove=false;
            emit send_player_direction(direction);//see the top note
            parseStop();
        }
        //if can go, then do the move
        else
        {
            direction=CatchChallenger::Direction_move_at_left;
            moveStep=0;
            moveStepSlot();
            emit send_player_direction(direction);
            if(CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange && x==0)
                emit send_player_direction(CatchChallenger::Direction_look_at_left);
            //startGrassAnimation(direction);
        }
    }
    else if(keyPressed.contains(Qt::Key_Right))
    {
        //can't go into this direction, then just look into this direction
        if(!canGoTo(CatchChallenger::Direction_move_at_right,current_map_pointer->logicalMap,x,y,true))
        {
            keyPressed.remove(Qt::Key_Right);
            direction=CatchChallenger::Direction_look_at_right;
            Tiled::Cell cell=playerMapObject->cell();
            cell.tile=playerTileset->tileAt(4);
            playerMapObject->setCell(cell);
            inMove=false;
            emit send_player_direction(direction);//see the top note
            parseStop();
        }
        //if can go, then do the move
        else
        {
            direction=CatchChallenger::Direction_move_at_right;
            moveStep=0;
            moveStepSlot();
            emit send_player_direction(direction);
            if(CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange && x==(current_map_pointer->logicalMap.width-1))
                emit send_player_direction(CatchChallenger::Direction_look_at_right);
            //startGrassAnimation(direction);
        }
    }
    else if(keyPressed.contains(Qt::Key_Up))
    {
        //can't go into this direction, then just look into this direction
        if(!canGoTo(CatchChallenger::Direction_move_at_top,current_map_pointer->logicalMap,x,y,true))
        {
            keyPressed.remove(Qt::Key_Up);
            direction=CatchChallenger::Direction_look_at_top;
            Tiled::Cell cell=playerMapObject->cell();
            cell.tile=playerTileset->tileAt(1);
            playerMapObject->setCell(cell);
            inMove=false;
            emit send_player_direction(direction);//see the top note
            parseStop();
        }
        //if can go, then do the move
        else
        {
            direction=CatchChallenger::Direction_move_at_top;
            moveStep=0;
            moveStepSlot();
            emit send_player_direction(direction);
            if(CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange && y==0)
                emit send_player_direction(CatchChallenger::Direction_look_at_top);
            //startGrassAnimation(direction);
        }
    }
    else if(keyPressed.contains(Qt::Key_Down))
    {
        //can't go into this direction, then just look into this direction
        if(!canGoTo(CatchChallenger::Direction_move_at_bottom,current_map_pointer->logicalMap,x,y,true))
        {
            keyPressed.remove(Qt::Key_Down);
            direction=CatchChallenger::Direction_look_at_bottom;
            Tiled::Cell cell=playerMapObject->cell();
            cell.tile=playerTileset->tileAt(7);
            playerMapObject->setCell(cell);
            inMove=false;
            emit send_player_direction(direction);//see the top note
            parseStop();
        }
        //if can go, then do the move
        else
        {
            direction=CatchChallenger::Direction_move_at_bottom;
            moveStep=0;
            moveStepSlot();
            emit send_player_direction(direction);
            if(CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange && y==(current_map_pointer->logicalMap.height-1))
                emit send_player_direction(CatchChallenger::Direction_look_at_bottom);
            //startGrassAnimation(direction);
        }
    }
    //now stop walking, no more arrow key is pressed
    else
    {
        if(!nextPathStep())
        {
            if(inMove)
            {
                emit send_player_direction(direction);
                inMove=false;
                parseStop();
            }
        }
    }
}

bool MapVisualiserPlayer::haveStopTileAction()
{
    return false;
}

void MapVisualiserPlayer::parseStop()
{
    CatchChallenger::CommonMap * map=&all_map.value(current_map)->logicalMap;
    uint8_t x=this->x;
    uint8_t y=this->y;
    switch(direction)
    {
        case CatchChallenger::Direction_look_at_left:
        if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_left,*map,x,y,false))
        {
            if(!CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_left,&map,&x,&y,false))
                qDebug() << QStringLiteral("can't go at left at map %1 (%2,%3) when move have been checked").arg(QString::fromStdString(map->map_file)).arg(x).arg(y);
            else
                emit stopped_in_front_of(static_cast<CatchChallenger::Map_client *>(map),x,y);
        }
        break;
        case CatchChallenger::Direction_look_at_right:
        if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_right,*map,x,y,false))
        {
            if(!CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_right,&map,&x,&y,false))
                qDebug() << QStringLiteral("can't go at right at map %1 (%2,%3) when move have been checked").arg(QString::fromStdString(map->map_file)).arg(x).arg(y);
            else
                emit stopped_in_front_of(static_cast<CatchChallenger::Map_client *>(map),x,y);
        }
        break;
        case CatchChallenger::Direction_look_at_top:
        if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_top,*map,x,y,false))
        {
            if(!CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_top,&map,&x,&y,false))
                qDebug() << QStringLiteral("can't go at top at map %1 (%2,%3) when move have been checked").arg(QString::fromStdString(map->map_file)).arg(x).arg(y);
            else
                emit stopped_in_front_of(static_cast<CatchChallenger::Map_client *>(map),x,y);
        }
        break;
        case CatchChallenger::Direction_look_at_bottom:
        if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_bottom,*map,x,y,false))
        {
            if(!CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_bottom,&map,&x,&y,false))
                qDebug() << QStringLiteral("can't go at bottom at map %1 (%2,%3) when move have been checked").arg(QString::fromStdString(map->map_file)).arg(x).arg(y);
            else
                emit stopped_in_front_of(static_cast<CatchChallenger::Map_client *>(map),x,y);
        }
        break;
        default:
        break;
    }
}

void MapVisualiserPlayer::parseAction()
{
    CatchChallenger::CommonMap * map=&all_map.value(current_map)->logicalMap;
    uint8_t x=this->x;
    uint8_t y=this->y;
    switch(direction)
    {
        case CatchChallenger::Direction_look_at_left:
        if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_left,*map,x,y,false))
        {
            if(!CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_left,&map,&x,&y,false))
                qDebug() << QStringLiteral("can't go at left at map %1 (%2,%3) when move have been checked").arg(QString::fromStdString(map->map_file)).arg(x).arg(y);
            else
            {
                CatchChallenger::Map_client * map_client=static_cast<CatchChallenger::Map_client *>(map);
                if(map_client->botsDisplay.contains(QPair<uint8_t,uint8_t>(x,y)))
                {
                    CatchChallenger::BotDisplay *botDisplay=&map_client->botsDisplay[QPair<uint8_t,uint8_t>(x,y)];
                    Tiled::Cell cell=botDisplay->mapObject->cell();
                    cell.tile=botDisplay->tileset->tileAt(4);
                    botDisplay->mapObject->setCell(cell);
                }
                else if(map_client->itemsOnMap.contains(QPair<uint8_t,uint8_t>(x,y)))
                {
                    const CatchChallenger::Map_client::ItemOnMapForClient &item=map_client->itemsOnMap.value(QPair<uint8_t,uint8_t>(x,y));
                    if(item.tileObject!=NULL)
                    {
                        ObjectGroupItem::objectGroupLink[item.tileObject->objectGroup()]->removeObject(item.tileObject);
                        map_client->itemsOnMap[QPair<uint8_t,uint8_t>(x,y)].tileObject=NULL;
                    }
                }
                emit actionOn(map_client,x,y);
            }
        }
        break;
        case CatchChallenger::Direction_look_at_right:
        if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_right,*map,x,y,false))
        {
            if(!CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_right,&map,&x,&y,false))
                qDebug() << QStringLiteral("can't go at right at map %1 (%2,%3) when move have been checked").arg(QString::fromStdString(map->map_file)).arg(x).arg(y);
            else
            {
                CatchChallenger::Map_client * map_client=static_cast<CatchChallenger::Map_client *>(map);
                if(map_client->botsDisplay.contains(QPair<uint8_t,uint8_t>(x,y)))
                {
                    CatchChallenger::BotDisplay *botDisplay=&map_client->botsDisplay[QPair<uint8_t,uint8_t>(x,y)];
                    Tiled::Cell cell=botDisplay->mapObject->cell();
                    cell.tile=botDisplay->tileset->tileAt(10);
                    botDisplay->mapObject->setCell(cell);
                }
                else if(map_client->itemsOnMap.contains(QPair<uint8_t,uint8_t>(x,y)))
                {
                    const CatchChallenger::Map_client::ItemOnMapForClient &item=map_client->itemsOnMap.value(QPair<uint8_t,uint8_t>(x,y));
                    if(item.tileObject!=NULL)
                    {
                        ObjectGroupItem::objectGroupLink[item.tileObject->objectGroup()]->removeObject(item.tileObject);
                        map_client->itemsOnMap[QPair<uint8_t,uint8_t>(x,y)].tileObject=NULL;
                    }
                }
                emit actionOn(map_client,x,y);
            }
        }
        break;
        case CatchChallenger::Direction_look_at_top:
        if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_top,*map,x,y,false))
        {
            if(!CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_top,&map,&x,&y,false))
                qDebug() << QStringLiteral("can't go at bottom at map %1 (%2,%3) when move have been checked").arg(QString::fromStdString(map->map_file)).arg(x).arg(y);
            else
            {
                CatchChallenger::Map_client * map_client=static_cast<CatchChallenger::Map_client *>(map);
                if(map_client->botsDisplay.contains(QPair<uint8_t,uint8_t>(x,y)))
                {
                    CatchChallenger::BotDisplay *botDisplay=&map_client->botsDisplay[QPair<uint8_t,uint8_t>(x,y)];
                    Tiled::Cell cell=botDisplay->mapObject->cell();
                    cell.tile=botDisplay->tileset->tileAt(7);
                    botDisplay->mapObject->setCell(cell);
                }
                else if(map_client->itemsOnMap.contains(QPair<uint8_t,uint8_t>(x,y)))
                {
                    const CatchChallenger::Map_client::ItemOnMapForClient &item=map_client->itemsOnMap.value(QPair<uint8_t,uint8_t>(x,y));
                    if(item.tileObject!=NULL)
                    {
                        ObjectGroupItem::objectGroupLink[item.tileObject->objectGroup()]->removeObject(item.tileObject);
                        map_client->itemsOnMap[QPair<uint8_t,uint8_t>(x,y)].tileObject=NULL;
                    }
                }
                emit actionOn(map_client,x,y);
            }
        }
        break;
        case CatchChallenger::Direction_look_at_bottom:
        if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_bottom,*map,x,y,false))
        {
            if(!CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_bottom,&map,&x,&y,false))
                qDebug() << QStringLiteral("can't go at top at map %1 (%2,%3) when move have been checked").arg(QString::fromStdString(map->map_file)).arg(x).arg(y);
            else
            {
                CatchChallenger::Map_client * map_client=static_cast<CatchChallenger::Map_client *>(map);
                if(map_client->botsDisplay.contains(QPair<uint8_t,uint8_t>(x,y)))
                {
                    CatchChallenger::BotDisplay *botDisplay=&map_client->botsDisplay[QPair<uint8_t,uint8_t>(x,y)];
                    Tiled::Cell cell=botDisplay->mapObject->cell();
                    cell.tile=botDisplay->tileset->tileAt(1);
                    botDisplay->mapObject->setCell(cell);
                }
                else if(map_client->itemsOnMap.contains(QPair<uint8_t,uint8_t>(x,y)))
                {
                    const CatchChallenger::Map_client::ItemOnMapForClient &item=map_client->itemsOnMap.value(QPair<uint8_t,uint8_t>(x,y));
                    if(item.tileObject!=NULL)
                    {
                        ObjectGroupItem::objectGroupLink[item.tileObject->objectGroup()]->removeObject(item.tileObject);
                        map_client->itemsOnMap[QPair<uint8_t,uint8_t>(x,y)].tileObject=NULL;
                    }
                }
                emit actionOn(map_client,x,y);
            }
        }
        break;
        default:
            emit actionOnNothing();
        break;
    }
}

//have look into another direction, if the key remain pressed, apply like move
void MapVisualiserPlayer::transformLookToMove()
{
    if(inMove)
        return;

    switch(direction)
    {
        case CatchChallenger::Direction_look_at_left:
        if(keyPressed.contains(Qt::Key_Left) && canGoTo(CatchChallenger::Direction_move_at_left,all_map.value(current_map)->logicalMap,x,y,true))
        {
            direction=CatchChallenger::Direction_move_at_left;
            inMove=true;
            moveStep=1;
            moveStepSlot();
            emit send_player_direction(direction);
            //startGrassAnimation(direction);
        }
        break;
        case CatchChallenger::Direction_look_at_right:
        if(keyPressed.contains(Qt::Key_Right) && canGoTo(CatchChallenger::Direction_move_at_right,all_map.value(current_map)->logicalMap,x,y,true))
        {
            direction=CatchChallenger::Direction_move_at_right;
            inMove=true;
            moveStep=1;
            moveStepSlot();
            emit send_player_direction(direction);
            //startGrassAnimation(direction);
        }
        break;
        case CatchChallenger::Direction_look_at_top:
        if(keyPressed.contains(Qt::Key_Up) && canGoTo(CatchChallenger::Direction_move_at_top,all_map.value(current_map)->logicalMap,x,y,true))
        {
            direction=CatchChallenger::Direction_move_at_top;
            inMove=true;
            moveStep=1;
            moveStepSlot();
            emit send_player_direction(direction);
            //startGrassAnimation(direction);
        }
        break;
        case CatchChallenger::Direction_look_at_bottom:
        if(keyPressed.contains(Qt::Key_Down) && canGoTo(CatchChallenger::Direction_move_at_bottom,all_map.value(current_map)->logicalMap,x,y,true))
        {
            direction=CatchChallenger::Direction_move_at_bottom;
            inMove=true;
            moveStep=1;
            moveStepSlot();
            emit send_player_direction(direction);
            //startGrassAnimation(direction);
        }
        break;
        default:
        qDebug() << QStringLiteral("transformLookToMove(): wrong direction");
        return;
    }
}

void MapVisualiserPlayer::keyReleaseEvent(QKeyEvent * event)
{
    if(current_map.isEmpty() || !all_map.contains(current_map))
        return;

    //ignore the no arrow key
    if(!keyAccepted.contains(event->key()))
    {
        event->ignore();
        return;
    }

    //ignore the repeated event
    if(event->isAutoRepeat())
        return;

    //remove from the key list pressed
    keyPressed.remove(event->key());

    if(keyPressed.size()>0)//another key pressed, repeat
        keyPressParse();
}

QString MapVisualiserPlayer::lastLocation() const
{
    return mLastLocation;
}

QString MapVisualiserPlayer::currentMap() const
{
    return current_map;
}

MapVisualiserThread::Map_full * MapVisualiserPlayer::currentMapFull() const
{
    return all_map.value(current_map);
}

bool MapVisualiserPlayer::currentMapIsLoaded() const
{
    if(!all_map.contains(current_map))
        return false;
    return true;
}

QString MapVisualiserPlayer::currentMapType() const
{
    if(!all_map.contains(current_map))
        return QString();
    if(all_map.value(current_map)->tiledMap->properties().contains(MapVisualiserPlayer::text_type))
        if(!all_map.value(current_map)->tiledMap->properties().value(MapVisualiserPlayer::text_type).isEmpty())
            return all_map.value(current_map)->tiledMap->properties().value(MapVisualiserPlayer::text_type);
    if(all_map.value(current_map)->logicalMap.xmlRoot->Attribute(std::string("type"))==NULL)
        if(!all_map.value(current_map)->logicalMap.xmlRoot->Attribute(std::string("type"))->empty())
            return QString::fromStdString(*all_map.value(current_map)->logicalMap.xmlRoot->Attribute(std::string("type")));
    return QString();
}

QString MapVisualiserPlayer::currentZone() const
{
    if(!all_map.contains(current_map))
        return QString();
    if(all_map.value(current_map)->tiledMap->properties().contains(MapVisualiserPlayer::text_zone))
        if(!all_map.value(current_map)->tiledMap->properties().value(MapVisualiserPlayer::text_zone).isEmpty())
            return all_map.value(current_map)->tiledMap->properties().value(MapVisualiserPlayer::text_zone);
    if(all_map.value(current_map)->logicalMap.xmlRoot->Attribute(std::string("zone")))
        if(!all_map.value(current_map)->logicalMap.xmlRoot->Attribute(std::string("zone"))->empty())
            return QString::fromStdString(*all_map.value(current_map)->logicalMap.xmlRoot->Attribute(std::string("zone")));
    return QString();
}

QString MapVisualiserPlayer::currentBackgroundsound() const
{
    if(!all_map.contains(current_map))
        return QString();
    if(all_map.value(current_map)->tiledMap->properties().contains(MapVisualiserPlayer::text_backgroundsound))
        if(!all_map.value(current_map)->tiledMap->properties().value(MapVisualiserPlayer::text_backgroundsound).isEmpty())
            return all_map.value(current_map)->tiledMap->properties().value(MapVisualiserPlayer::text_backgroundsound);
    if(all_map.value(current_map)->logicalMap.xmlRoot->Attribute(std::string("backgroundsound")))
        if(!all_map.value(current_map)->logicalMap.xmlRoot->Attribute(std::string("backgroundsound"))->empty())
            return QString::fromStdString(*all_map.value(current_map)->logicalMap.xmlRoot->Attribute(std::string("backgroundsound")));
    return QString();
}

CatchChallenger::Direction MapVisualiserPlayer::getDirection()
{
    return direction;
}

void MapVisualiserPlayer::resetAll()
{
    //stopGrassAnimation();
    unloadPlayerFromCurrentMap();
    currentPlayerSpeed=250;
    moveTimer.setInterval(currentPlayerSpeed/5);
    moveTimer.setSingleShot(true);
    moveAnimationTimer.setInterval(currentPlayerSpeed/5);
    moveAnimationTimer.setSingleShot(true);
    timer.stop();
    moveTimer.stop();
    lookToMove.stop();
    keyPressed.clear();
    blocked=false;
    inMove=false;
    MapVisualiser::resetAll();
    mapVisualiserThread.stopIt=true;
    mapVisualiserThread.quit();
    mapVisualiserThread.wait();
    mapVisualiserThread.start(QThread::IdlePriority);

    //delete playerTileset;
    {
        QSet<Tiled::Tileset *> deletedTileset;
        QHashIterator<QString,Tiled::Tileset *> i(playerTilesetCache);
        while (i.hasNext()) {
            i.next();
            if(!deletedTileset.contains(i.value()))
            {
                deletedTileset << i.value();
                delete i.value();
            }
        }
        playerTilesetCache.clear();
    }
    lastTileset=defaultTileset;
    playerTileset = new Tiled::Tileset(QLatin1Literal("player"),16,24);
    playerTilesetCache[lastTileset]=playerTileset;
}

void MapVisualiserPlayer::setSpeed(const SPEED_TYPE &speed)
{
    currentPlayerSpeed=speed;
    moveTimer.setInterval(speed/5);
}

bool MapVisualiserPlayer::canGoTo(const CatchChallenger::Direction &direction, CatchChallenger::CommonMap map, uint8_t x, uint8_t y, const bool &checkCollision)
{
    CatchChallenger::CommonMap *mapPointer=&map;
    CatchChallenger::ParsedLayerLedges ledge;
    do
    {
        if(!CatchChallenger::MoveOnTheMap::canGoTo(direction,*mapPointer,x,y,checkCollision))
            return false;
        if(!CatchChallenger::MoveOnTheMap::move(direction,&mapPointer,&x,&y,checkCollision))
            return false;
        CatchChallenger::Map_client * map_client=static_cast<CatchChallenger::Map_client *>(&all_map.value(QString::fromStdString(map.map_file))->logicalMap);
        if(map_client->itemsOnMap.contains(QPair<uint8_t,uint8_t>(x,y)))
        {
            const CatchChallenger::Map_client::ItemOnMapForClient &item=map_client->itemsOnMap.value(QPair<uint8_t,uint8_t>(x,y));
            if(item.tileObject!=NULL)
                return false;
        }
        ledge=CatchChallenger::MoveOnTheMap::getLedge(map,x,y);
        if(ledge==CatchChallenger::ParsedLayerLedges_NoLedges)
            return true;
        switch(direction)
        {
            case CatchChallenger::Direction_move_at_bottom:
            if(ledge!=CatchChallenger::ParsedLayerLedges_LedgesBottom)
                return false;
            break;
            case CatchChallenger::Direction_move_at_top:
            if(ledge!=CatchChallenger::ParsedLayerLedges_LedgesTop)
                return false;
            break;
            case CatchChallenger::Direction_move_at_left:
            if(ledge!=CatchChallenger::ParsedLayerLedges_LedgesLeft)
                return false;
            break;
            case CatchChallenger::Direction_move_at_right:
            if(ledge!=CatchChallenger::ParsedLayerLedges_LedgesRight)
                return false;
            break;
            default:
            break;
        }
    } while(ledge!=CatchChallenger::ParsedLayerLedges_NoLedges);
    return true;
}

//call after enter on new map
void MapVisualiserPlayer::loadPlayerFromCurrentMap()
{
    if(!all_map.contains(current_map))
    {
        qDebug() << QStringLiteral("all_map have not the current map: %1").arg(current_map);
        return;
    }
    Tiled::ObjectGroup *currentGroup=playerMapObject->objectGroup();
    if(currentGroup!=NULL)
    {
        if(ObjectGroupItem::objectGroupLink.contains(currentGroup))
            ObjectGroupItem::objectGroupLink.value(currentGroup)->removeObject(playerMapObject);
        //currentGroup->removeObject(playerMapObject);
        if(currentGroup!=all_map.value(current_map)->objectGroup)
            qDebug() << QStringLiteral("loadPlayerFromCurrentMap(), the playerMapObject group is wrong: %1").arg(currentGroup->name());
    }
    if(ObjectGroupItem::objectGroupLink.contains(all_map.value(current_map)->objectGroup))
        ObjectGroupItem::objectGroupLink.value(all_map.value(current_map)->objectGroup)->addObject(playerMapObject);
    else
        qDebug() << QStringLiteral("loadPlayerFromCurrentMap(), ObjectGroupItem::objectGroupLink not contains current_map->objectGroup");
    mLastLocation=QString::fromStdString(all_map.value(current_map)->logicalMap.map_file);

    //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
    playerMapObject->setPosition(QPoint(x,y+1));
    MapObjectItem::objectLink.value(playerMapObject)->setZValue(y);
    if(centerOnPlayer)
        centerOn(MapObjectItem::objectLink.value(playerMapObject));
}

//call before leave the old map (and before loadPlayerFromCurrentMap())
void MapVisualiserPlayer::unloadPlayerFromCurrentMap()
{
    Tiled::ObjectGroup *currentGroup=playerMapObject->objectGroup();
    if(currentGroup==NULL)
        return;
    //unload the player sprite
    if(ObjectGroupItem::objectGroupLink.contains(playerMapObject->objectGroup()))
        ObjectGroupItem::objectGroupLink.value(playerMapObject->objectGroup())->removeObject(playerMapObject);
    else
        qDebug() << QStringLiteral("unloadPlayerFromCurrentMap(), ObjectGroupItem::objectGroupLink not contains playerMapObject->objectGroup()");
}

/*void MapVisualiserPlayer::startGrassAnimation(const CatchChallenger::Direction &direction)
{
    switch(direction)
    {
        case CatchChallenger::Direction_move_at_left:
        case CatchChallenger::Direction_move_at_right:
        case CatchChallenger::Direction_move_at_top:
        case CatchChallenger::Direction_move_at_bottom:
        break;
        default:
        return;
    }

    if(!haveGrassCurrentObject)
    {
        haveGrassCurrentObject=CatchChallenger::MoveOnTheMap::haveGrass(all_map.value(current_map)->logicalMap,x,y);
        if(haveGrassCurrentObject)
        {
            ObjectGroupItem::objectGroupLink.value(all_map.value(current_map)->objectGroup)->addObject(grassCurrentObject);
            grassCurrentObject->setPosition(QPoint(x,y+1));
            MapObjectItem::objectLink.value(playerMapObject)->setZValue(y);
            Tiled::Cell cell=grassCurrentObject->cell();
            cell.tile=animationTileset->tileAt(2);
            grassCurrentObject->setCell(cell);
        }
    }
    else
        qDebug() << "haveGrassCurrentObject true here, it's wrong!";

    if(!haveNextCurrentObject)
    {
        haveNextCurrentObject=false;
        CatchChallenger::Map * map_destination=&all_map.value(current_map)->logicalMap;
        COORD_TYPE x_destination=x;
        COORD_TYPE y_destination=y;
        if(CatchChallenger::MoveOnTheMap::move(direction,&map_destination,&x_destination,&y_destination))
            if(all_map.contains(map_destination->map_file))
                haveNextCurrentObject=CatchChallenger::MoveOnTheMap::haveGrass(*map_destination,x_destination,y_destination);
        if(haveNextCurrentObject)
        {
            ObjectGroupItem::objectGroupLink.value(all_map.value(map_destination->map_file)->objectGroup)->addObject(nextCurrentObject);
            nextCurrentObject->setPosition(QPoint(x_destination,y_destination+1));
            MapObjectItem::objectLink.value(playerMapObject)->setZValue(y_destination);
            Tiled::Cell cell=nextCurrentObject->cell();
            cell.tile=animationTileset->tileAt(1);
            nextCurrentObject->setCell(cell);
        }
    }
    else
        qDebug() << "haveNextCurrentObject true here, it's wrong!";
}

void MapVisualiserPlayer::stopGrassAnimation()
{
    if(haveGrassCurrentObject)
    {
        ObjectGroupItem::objectGroupLink.value(grassCurrentObject->objectGroup())->removeObject(grassCurrentObject);
        haveGrassCurrentObject=false;
    }
    if(haveNextCurrentObject)
    {
        ObjectGroupItem::objectGroupLink.value(nextCurrentObject->objectGroup())->removeObject(nextCurrentObject);
        haveNextCurrentObject=false;
    }
}*/

void MapVisualiserPlayer::loadGrassTile()
{
    if(haveGrassCurrentObject)
    {
        switch(moveStep)
        {
            case 0:
            case 1:
            break;
            case 2:
            {
                Tiled::Cell cell=grassCurrentObject->cell();
                cell.tile=animationTileset->tileAt(0);
                grassCurrentObject->setCell(cell);
            }
            break;
        }
    }
    if(haveNextCurrentObject)
    {
        switch(moveStep)
        {
            case 0:
            case 1:
            break;
            case 3:
            {
                Tiled::Cell cell=nextCurrentObject->cell();
                cell.tile=animationTileset->tileAt(2);
                nextCurrentObject->setCell(cell);
            }
            break;
        }
    }
}

void MapVisualiserPlayer::mapDisplayedSlot(const QString &fileName)
{
    if(current_map==fileName)
    {
        emit currentMapLoaded();
        loadPlayerFromCurrentMap();
    }
}

uint8_t MapVisualiserPlayer::getX()
{
    return x;
}

uint8_t MapVisualiserPlayer::getY()
{
    return y;
}

CatchChallenger::Map_client * MapVisualiserPlayer::getMapObject()
{
    if(all_map.contains(current_map))
        return &all_map.value(current_map)->logicalMap;
    else
        return NULL;
}

//the datapack
void MapVisualiserPlayer::setDatapackPath(const QString &path,const QString &mainDatapackCode)
{
    #ifdef DEBUG_CLIENT_LOAD_ORDER
    qDebug() << QStringLiteral("MapControllerMP::setDatapackPath()");
    #endif

    if(path.endsWith(MapVisualiserPlayer::text_slash) || path.endsWith(MapVisualiserPlayer::text_antislash))
        datapackPath=path;
    else
        datapackPath=path+MapVisualiserPlayer::text_slash;
    datapackMapPathBase=QFileInfo(datapackPath+DATAPACK_BASE_PATH_MAPBASE).absoluteFilePath();
    if(!datapackMapPathBase.endsWith(MapVisualiserPlayer::text_slash) && !datapackMapPathBase.endsWith(MapVisualiserPlayer::text_antislash))
        datapackMapPathBase+=MapVisualiserPlayer::text_slash;
    datapackMapPathSpec=QFileInfo(datapackPath+DATAPACK_BASE_PATH_MAPMAIN+mainDatapackCode+"/").absoluteFilePath();
    if(!datapackMapPathSpec.endsWith(MapVisualiserPlayer::text_slash) && !datapackMapPathSpec.endsWith(MapVisualiserPlayer::text_antislash))
        datapackMapPathSpec+=MapVisualiserPlayer::text_slash;
    mLastLocation.clear();
}

void MapVisualiserPlayer::datapackParsed()
{
}

void MapVisualiserPlayer::datapackParsedMainSub()
{
    #ifdef DEBUG_CLIENT_LOAD_ORDER
    qDebug() << QStringLiteral("MapControllerMP::datapackParsedMainSub()");
    #endif

    if(mHaveTheDatapack)
        return;
    mHaveTheDatapack=true;
}
