#include "MapVisualiserPlayer.h"

#include "../../../general/base/MoveOnTheMap.h"
#include "../../../general/base/CommonDatapack.h"
#include "../../../general/base/CommonSettings.h"
#include "../interface/DatapackClientLoader.h"
#include "../../../general/base/GeneralVariable.h"

#include <qmath.h>
#include <QFileInfo>
#include <QMessageBox>

QString MapVisualiserPlayer::text_DATAPACK_BASE_PATH_SKIN=QLatin1Literal(DATAPACK_BASE_PATH_SKIN);
QString MapVisualiserPlayer::text_DATAPACK_BASE_PATH_MAP=QLatin1Literal(DATAPACK_BASE_PATH_MAP);
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
    inMove=false;
    teleportedOnPush=false;
    x=0;
    y=0;
    events=NULL;
    items=NULL;
    quests=NULL;

    keyAccepted << Qt::Key_Left << Qt::Key_Right << Qt::Key_Up << Qt::Key_Down << Qt::Key_Return;

    lookToMove.setInterval(200);
    lookToMove.setSingleShot(true);
    connect(&lookToMove,&QTimer::timeout,this,&MapVisualiserPlayer::transformLookToMove);
    connect(this,&MapVisualiserPlayer::mapDisplayed,this,&MapVisualiserPlayer::mapDisplayedSlot);

    moveTimer.setInterval(250/5);
    moveTimer.setSingleShot(true);
    connect(&moveTimer,&QTimer::timeout,this,&MapVisualiserPlayer::moveStepSlot);

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

    defaultTileset=QLatin1Literal("trainer");
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
    if(inMove)
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
            //tiger the next tile
            {
                CatchChallenger::CommonMap * map=&all_map.value(current_map)->logicalMap;
                quint8 x=this->x;
                quint8 y=this->y;
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
                if(all_map.contains(map->map_file))
                    if(all_map.value(map->map_file)->doors.contains(QPair<quint8,quint8>(x,y)))
                        all_map.value(map->map_file)->doors.value(QPair<quint8,quint8>(x,y))->startOpen();
            }
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
            //tiger the next tile
            {
                CatchChallenger::CommonMap * map=&all_map.value(current_map)->logicalMap;
                quint8 x=this->x;
                quint8 y=this->y;
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
                if(all_map.contains(map->map_file))
                    if(all_map.value(map->map_file)->doors.contains(QPair<quint8,quint8>(x,y)))
                        all_map.value(map->map_file)->doors.value(QPair<quint8,quint8>(x,y))->startOpen();
            }
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
            //tiger the next tile
            {
                CatchChallenger::CommonMap * map=&all_map.value(current_map)->logicalMap;
                quint8 x=this->x;
                quint8 y=this->y;
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
                if(all_map.contains(map->map_file))
                    if(all_map.value(map->map_file)->doors.contains(QPair<quint8,quint8>(x,y)))
                        all_map.value(map->map_file)->doors.value(QPair<quint8,quint8>(x,y))->startOpen();
            }
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
            //tiger the next tile
            {
                CatchChallenger::CommonMap * map=&all_map.value(current_map)->logicalMap;
                quint8 x=this->x;
                quint8 y=this->y;
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
                if(all_map.contains(map->map_file))
                    if(all_map.value(map->map_file)->doors.contains(QPair<quint8,quint8>(x,y)))
                        all_map.value(map->map_file)->doors.value(QPair<quint8,quint8>(x,y))->startOpen();
            }
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

void MapVisualiserPlayer::moveStepSlot()
{
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
            current_map=map->map_file;
            if(!old_all_map.contains(map->map_file))
                emit inWaitingOfMap();
            loadOtherMap(map->map_file);
            hideNotloadedMap();
            return;
        }
        else
            finalPlayerStep();
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

void MapVisualiserPlayer::setInformations(QHash<quint16,quint32> *items,QHash<quint16, CatchChallenger::PlayerQuest> *quests,QList<quint8> *events)
{
    this->events=events;
    this->items=items;
    this->quests=quests;
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
        int index=0;
        while(index<monstersCollisionValue.walkOn.size())
        {
            const CatchChallenger::MonstersCollision &monstersCollision=CatchChallenger::CommonDatapack::commonDatapack.monstersCollision.at(monstersCollisionValue.walkOn.at(index));
            if(monstersCollision.item==0 || items->contains(monstersCollision.item))
            {
                if(monstersCollision.tile!=lastTileset)
                {
                    lastTileset=monstersCollision.tile;
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
            //tiger the next tile
            {
                CatchChallenger::CommonMap * map=&all_map.value(current_map)->logicalMap;
                quint8 x=this->x;
                quint8 y=this->y;
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
                if(all_map.contains(map->map_file))
                    if(all_map.value(map->map_file)->doors.contains(QPair<quint8,quint8>(x,y)))
                        all_map.value(map->map_file)->doors.value(QPair<quint8,quint8>(x,y))->startOpen();
            }
            moveStep=0;
            moveStepSlot();
            emit send_player_direction(direction);
            if(CommonSettings::commonSettings.forceClientToSendAtMapChange && x==0)
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
            //tiger the next tile
            {
                CatchChallenger::CommonMap * map=&all_map.value(current_map)->logicalMap;
                quint8 x=this->x;
                quint8 y=this->y;
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
                if(all_map.contains(map->map_file))
                    if(all_map.value(map->map_file)->doors.contains(QPair<quint8,quint8>(x,y)))
                        all_map.value(map->map_file)->doors.value(QPair<quint8,quint8>(x,y))->startOpen();
            }
            moveStep=0;
            moveStepSlot();
            emit send_player_direction(direction);
            if(CommonSettings::commonSettings.forceClientToSendAtMapChange && x==(current_map_pointer->logicalMap.width-1))
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
            //tiger the next tile
            {
                CatchChallenger::CommonMap * map=&all_map.value(current_map)->logicalMap;
                quint8 x=this->x;
                quint8 y=this->y;
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
                if(all_map.contains(map->map_file))
                    if(all_map.value(map->map_file)->doors.contains(QPair<quint8,quint8>(x,y)))
                        all_map.value(map->map_file)->doors.value(QPair<quint8,quint8>(x,y))->startOpen();
            }
            moveStep=0;
            moveStepSlot();
            emit send_player_direction(direction);
            if(CommonSettings::commonSettings.forceClientToSendAtMapChange && y==0)
                emit send_player_direction(CatchChallenger::Direction_look_at_right);
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
            //tiger the next tile
            {
                CatchChallenger::CommonMap * map=&all_map.value(current_map)->logicalMap;
                quint8 x=this->x;
                quint8 y=this->y;
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
                if(all_map.contains(map->map_file))
                    if(all_map.value(map->map_file)->doors.contains(QPair<quint8,quint8>(x,y)))
                        all_map.value(map->map_file)->doors.value(QPair<quint8,quint8>(x,y))->startOpen();
            }
            moveStep=0;
            moveStepSlot();
            emit send_player_direction(direction);
            if(CommonSettings::commonSettings.forceClientToSendAtMapChange && y==(current_map_pointer->logicalMap.height-1))
                emit send_player_direction(CatchChallenger::Direction_look_at_right);
            //startGrassAnimation(direction);
        }
    }
    //now stop walking, no more arrow key is pressed
    else
    {
        if(inMove)
        {
            inMove=false;
            emit send_player_direction(direction);
            parseStop();
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
    quint8 x=this->x;
    quint8 y=this->y;
    switch(direction)
    {
        case CatchChallenger::Direction_look_at_left:
        if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_left,*map,x,y,false))
        {
            if(!CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_left,&map,&x,&y,false))
                qDebug() << QStringLiteral("can't go at left at map %1 (%2,%3) when move have been checked").arg(map->map_file).arg(x).arg(y);
            else
                emit stopped_in_front_of(static_cast<CatchChallenger::Map_client *>(map),x,y);
        }
        break;
        case CatchChallenger::Direction_look_at_right:
        if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_right,*map,x,y,false))
        {
            if(!CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_right,&map,&x,&y,false))
                qDebug() << QStringLiteral("can't go at right at map %1 (%2,%3) when move have been checked").arg(map->map_file).arg(x).arg(y);
            else
                emit stopped_in_front_of(static_cast<CatchChallenger::Map_client *>(map),x,y);
        }
        break;
        case CatchChallenger::Direction_look_at_top:
        if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_top,*map,x,y,false))
        {
            if(!CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_top,&map,&x,&y,false))
                qDebug() << QStringLiteral("can't go at top at map %1 (%2,%3) when move have been checked").arg(map->map_file).arg(x).arg(y);
            else
                emit stopped_in_front_of(static_cast<CatchChallenger::Map_client *>(map),x,y);
        }
        break;
        case CatchChallenger::Direction_look_at_bottom:
        if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_bottom,*map,x,y,false))
        {
            if(!CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_bottom,&map,&x,&y,false))
                qDebug() << QStringLiteral("can't go at bottom at map %1 (%2,%3) when move have been checked").arg(map->map_file).arg(x).arg(y);
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
    quint8 x=this->x;
    quint8 y=this->y;
    switch(direction)
    {
        case CatchChallenger::Direction_look_at_left:
        if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_left,*map,x,y,false))
        {
            if(!CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_left,&map,&x,&y,false))
                qDebug() << QStringLiteral("can't go at left at map %1 (%2,%3) when move have been checked").arg(map->map_file).arg(x).arg(y);
            else
            {
                CatchChallenger::Map_client * map_client=static_cast<CatchChallenger::Map_client *>(map);
                if(map_client->botsDisplay.contains(QPair<quint8,quint8>(x,y)))
                {
                    CatchChallenger::BotDisplay *botDisplay=&map_client->botsDisplay[QPair<quint8,quint8>(x,y)];
                    Tiled::Cell cell=botDisplay->mapObject->cell();
                    cell.tile=botDisplay->tileset->tileAt(4);
                    botDisplay->mapObject->setCell(cell);
                }
                emit actionOn(map_client,x,y);
            }
        }
        break;
        case CatchChallenger::Direction_look_at_right:
        if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_right,*map,x,y,false))
        {
            if(!CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_right,&map,&x,&y,false))
                qDebug() << QStringLiteral("can't go at right at map %1 (%2,%3) when move have been checked").arg(map->map_file).arg(x).arg(y);
            else
            {
                CatchChallenger::Map_client * map_client=static_cast<CatchChallenger::Map_client *>(map);
                if(map_client->botsDisplay.contains(QPair<quint8,quint8>(x,y)))
                {
                    CatchChallenger::BotDisplay *botDisplay=&map_client->botsDisplay[QPair<quint8,quint8>(x,y)];
                    Tiled::Cell cell=botDisplay->mapObject->cell();
                    cell.tile=botDisplay->tileset->tileAt(10);
                    botDisplay->mapObject->setCell(cell);
                }
                emit actionOn(map_client,x,y);
            }
        }
        break;
        case CatchChallenger::Direction_look_at_top:
        if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_top,*map,x,y,false))
        {
            if(!CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_top,&map,&x,&y,false))
                qDebug() << QStringLiteral("can't go at bottom at map %1 (%2,%3) when move have been checked").arg(map->map_file).arg(x).arg(y);
            else
            {
                CatchChallenger::Map_client * map_client=static_cast<CatchChallenger::Map_client *>(map);
                if(map_client->botsDisplay.contains(QPair<quint8,quint8>(x,y)))
                {
                    CatchChallenger::BotDisplay *botDisplay=&map_client->botsDisplay[QPair<quint8,quint8>(x,y)];
                    Tiled::Cell cell=botDisplay->mapObject->cell();
                    cell.tile=botDisplay->tileset->tileAt(7);
                    botDisplay->mapObject->setCell(cell);
                }
                emit actionOn(map_client,x,y);
            }
        }
        break;
        case CatchChallenger::Direction_look_at_bottom:
        if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_bottom,*map,x,y,false))
        {
            if(!CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_bottom,&map,&x,&y,false))
                qDebug() << QStringLiteral("can't go at top at map %1 (%2,%3) when move have been checked").arg(map->map_file).arg(x).arg(y);
            else
            {
                CatchChallenger::Map_client * map_client=static_cast<CatchChallenger::Map_client *>(map);
                if(map_client->botsDisplay.contains(QPair<quint8,quint8>(x,y)))
                {
                    CatchChallenger::BotDisplay *botDisplay=&map_client->botsDisplay[QPair<quint8,quint8>(x,y)];
                    Tiled::Cell cell=botDisplay->mapObject->cell();
                    cell.tile=botDisplay->tileset->tileAt(1);
                    botDisplay->mapObject->setCell(cell);
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
    if(all_map.value(current_map)->logicalMap.xmlRoot.hasAttribute(MapVisualiserPlayer::text_type))
        if(!all_map.value(current_map)->logicalMap.xmlRoot.attribute(MapVisualiserPlayer::text_type).isEmpty())
            return all_map.value(current_map)->logicalMap.xmlRoot.attribute(MapVisualiserPlayer::text_type);
    return QString();
}

QString MapVisualiserPlayer::currentZone() const
{
    if(!all_map.contains(current_map))
        return QString();
    if(all_map.value(current_map)->tiledMap->properties().contains(MapVisualiserPlayer::text_zone))
        if(!all_map.value(current_map)->tiledMap->properties().value(MapVisualiserPlayer::text_zone).isEmpty())
            return all_map.value(current_map)->tiledMap->properties().value(MapVisualiserPlayer::text_zone);
    if(all_map.value(current_map)->logicalMap.xmlRoot.hasAttribute(MapVisualiserPlayer::text_zone))
        if(!all_map.value(current_map)->logicalMap.xmlRoot.attribute(MapVisualiserPlayer::text_zone).isEmpty())
            return all_map.value(current_map)->logicalMap.xmlRoot.attribute(MapVisualiserPlayer::text_zone);
    return QString();
}

QString MapVisualiserPlayer::currentBackgroundsound() const
{
    if(!all_map.contains(current_map))
        return QString();
    if(all_map.value(current_map)->tiledMap->properties().contains(MapVisualiserPlayer::text_backgroundsound))
        if(!all_map.value(current_map)->tiledMap->properties().value(MapVisualiserPlayer::text_backgroundsound).isEmpty())
            return all_map.value(current_map)->tiledMap->properties().value(MapVisualiserPlayer::text_backgroundsound);
    if(all_map.value(current_map)->logicalMap.xmlRoot.hasAttribute(MapVisualiserPlayer::text_backgroundsound))
        if(!all_map.value(current_map)->logicalMap.xmlRoot.attribute(MapVisualiserPlayer::text_backgroundsound).isEmpty())
            return all_map.value(current_map)->logicalMap.xmlRoot.attribute(MapVisualiserPlayer::text_backgroundsound);
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
    timer.stop();
    moveTimer.stop();
    lookToMove.stop();
    keyPressed.clear();
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
    moveTimer.setInterval(speed/5);
}

bool MapVisualiserPlayer::canGoTo(const CatchChallenger::Direction &direction, CatchChallenger::CommonMap map, quint8 x, quint8 y, const bool &checkCollision)
{
    CatchChallenger::CommonMap *mapPointer=&map;
    CatchChallenger::ParsedLayerLedges ledge;
    do
    {
        if(!CatchChallenger::MoveOnTheMap::canGoTo(direction,*mapPointer,x,y,checkCollision))
            return false;
        if(!CatchChallenger::MoveOnTheMap::move(direction,&mapPointer,&x,&y,checkCollision))
            return false;
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
    mLastLocation=all_map.value(current_map)->logicalMap.map_file;

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

quint8 MapVisualiserPlayer::getX()
{
    return x;
}

quint8 MapVisualiserPlayer::getY()
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
void MapVisualiserPlayer::setDatapackPath(const QString &path)
{
    #ifdef DEBUG_CLIENT_LOAD_ORDER
    qDebug() << QStringLiteral("MapControllerMP::setDatapackPath()");
    #endif

    if(path.endsWith(MapVisualiserPlayer::text_slash) || path.endsWith(MapVisualiserPlayer::text_antislash))
        datapackPath=path;
    else
        datapackPath=path+MapVisualiserPlayer::text_slash;
    datapackMapPath=QFileInfo(datapackPath+MapVisualiserPlayer::text_DATAPACK_BASE_PATH_MAP).absoluteFilePath();
    if(!datapackMapPath.endsWith(MapVisualiserPlayer::text_slash) && !datapackMapPath.endsWith(MapVisualiserPlayer::text_antislash))
        datapackMapPath+=MapVisualiserPlayer::text_slash;
    mLastLocation.clear();
}

void MapVisualiserPlayer::datapackParsed()
{
    #ifdef DEBUG_CLIENT_LOAD_ORDER
    qDebug() << QStringLiteral("MapControllerMP::datapackParsed()");
    #endif

    if(mHaveTheDatapack)
        return;
    mHaveTheDatapack=true;
}
