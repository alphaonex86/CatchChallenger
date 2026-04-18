#include "MapVisualiserPlayer.hpp"

#include "../../general/base/MoveOnTheMap.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../general/base/GeneralVariable.hpp"
#include "../libcatchchallenger/ClientVariable.hpp"
#include "QMap_client.hpp"

#include <qmath.h>
#include <QFileInfo>
#include <QMessageBox>
#include <QDebug>
#include <iostream>
#include <QCoreApplication>

/* why send the look at because blocked into the wall?
to be sync if connexion is stop, but use more bandwidth
To not send: store "is blocked but direction not send", cautch the close event, at close: if "is blocked but direction not send" then send it
*/

MapVisualiserPlayer::MapVisualiserPlayer(const bool &centerOnPlayer, const bool &debugTags, const bool &useCache, const bool &openGL) :
    MapVisualiser(debugTags,useCache,openGL)
{
    wasPathFindingUsed=false;
    blocked=false;
    inMove=false;
    teleportedOnPush=false;
    x=0;
    y=0;
    animationDisplayed=false;
    monsterTileset=nullptr;
    monsterMapObject=nullptr;
    monster_x=0;
    monster_y=0;
    clip=false;

    keyAccepted.insert(Qt::Key_Left);
    keyAccepted.insert(Qt::Key_Right);
    keyAccepted.insert(Qt::Key_Up);
    keyAccepted.insert(Qt::Key_Down);
    keyAccepted.insert(Qt::Key_Return);

    lookToMove.setInterval(200);
    lookToMove.setSingleShot(true);
    if(!connect(&lookToMove,&QTimer::timeout,this,&MapVisualiserPlayer::transformLookToMove))
        abort();
    if(!connect(this,&MapVisualiserPlayer::mapDisplayed,this,&MapVisualiserPlayer::mapDisplayedSlot))
        abort();

    moveTimer.setInterval(250/5);
    moveTimer.setSingleShot(true);
    if(!connect(&moveTimer,&QTimer::timeout,this,&MapVisualiserPlayer::moveStepSlot))
        abort();
    moveAnimationTimer.setInterval(250/5);
    moveAnimationTimer.setSingleShot(true);
    if(!connect(&moveAnimationTimer,&QTimer::timeout,this,&MapVisualiserPlayer::doMoveAnimation))
        abort();

    this->centerOnPlayer=centerOnPlayer;

    if(centerOnPlayer)
    {
        setSceneRect(-2000,-2000,4000,4000);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
    stepAlternance=false;
    animationTileset=Tiled::Tileset::create(QStringLiteral("animation"),16,16);
    nextCurrentObject=new Tiled::MapObject();
    grassCurrentObject=new Tiled::MapObject();
    grassCurrentObject->setName("grassCurrentObject");
    haveGrassCurrentObject=false;
    haveNextCurrentObject=false;

    defaultTileset="trainer";
    playerMapObject = new Tiled::MapObject();
    grassCurrentObject->setName("playerMapObject");

    lastTileset=defaultTileset;
    playerTileset=Tiled::Tileset::create(QStringLiteral("player"),16,24);
    playerTilesetCache[lastTileset]=playerTileset;

    lastAction.start();
}

void MapVisualiserPlayer::centerOnPlayerTile()
{
    if(!centerOnPlayer)
        return;
    //The current map is always drawn at scene origin (0,0) in pixel units
    //(see MapItem::setMapPosition, where border maps have relative pixel offset).
    //Prefer the sprite's actual tile-unit position so that the view follows the
    //player smoothly during intermediate animation steps (moveStepSlot() advances
    //playerMapObject by 0.20 tiles per step). At rest, playerMapObject is at
    //(x, y+1), so subtract 1 from y to match the tile center used when idle.
    qreal tile_x=static_cast<qreal>(x);
    qreal tile_y=static_cast<qreal>(y);
    if(playerMapObject!=NULL)
    {
        tile_x=playerMapObject->x();
        tile_y=playerMapObject->y()-1.0;
    }
    const qreal cx=tile_x*CLIENT_BASE_TILE_SIZE+static_cast<qreal>(CLIENT_BASE_TILE_SIZE)/2.0;
    const qreal cy=tile_y*CLIENT_BASE_TILE_SIZE+static_cast<qreal>(CLIENT_BASE_TILE_SIZE)/2.0;
    centerOn(cx,cy);
}

MapVisualiserPlayer::~MapVisualiserPlayer()
{
/*    if(animationTileset!=NULL)
        delete animationTileset;*/
    if(nextCurrentObject!=NULL)
        delete nextCurrentObject;
    if(grassCurrentObject!=NULL)
        delete grassCurrentObject;
    // playerMapObject may have been added to an ObjectGroup via addObject().
    // Must remove from ObjectGroup before deleting, otherwise ObjectGroup::~ObjectGroup()
    // will qDeleteAll(mObjects) on the already-freed pointer → double-free crash.
    if(playerMapObject!=NULL)
    {
        unloadPlayerFromCurrentMap();
        if(playerMapObject->objectGroup()==NULL)
            delete playerMapObject;
        // else: still owned by ObjectGroup, its destructor will qDeleteAll(mObjects)
        playerMapObject=NULL;
    }
    if(monsterMapObject!=nullptr)
    {
        unloadMonsterFromCurrentMap();
        if(monsterMapObject->objectGroup()==nullptr)
            delete monsterMapObject;
        // else: still owned by ObjectGroup, its destructor will qDeleteAll(mObjects)
        monsterMapObject=nullptr;
    }
}

bool MapVisualiserPlayer::haveMapInMemory(const CATCHCHALLENGER_TYPE_MAPID &mapIndex)
{
    return CatchChallenger::QMap_client::all_map.find(mapIndex)!=CatchChallenger::QMap_client::all_map.cend() ||
           CatchChallenger::QMap_client::old_all_map.find(mapIndex)!=CatchChallenger::QMap_client::old_all_map.cend();
}

void MapVisualiserPlayer::destroyMap(const CATCHCHALLENGER_TYPE_MAPID &mapIndex)
{
    CatchChallenger::QMap_client *map=NULL;
    if(CatchChallenger::QMap_client::all_map.find(mapIndex)!=CatchChallenger::QMap_client::all_map.cend())
        map=CatchChallenger::QMap_client::all_map.at(mapIndex);
    else if(CatchChallenger::QMap_client::old_all_map.find(mapIndex)!=CatchChallenger::QMap_client::old_all_map.cend())
        map=CatchChallenger::QMap_client::old_all_map.at(mapIndex);
    if(map!=NULL)
    {
        Tiled::ObjectGroup *group=map->objectGroup;
        if(group!=NULL)
        {
            if(playerMapObject!=nullptr && playerMapObject->objectGroup()==group)
                unloadPlayerFromCurrentMap();
            if(monsterMapObject!=nullptr && monsterMapObject->objectGroup()==group)
                unloadMonsterFromCurrentMap();
        }
    }
    MapVisualiser::destroyMap(mapIndex);
}

void MapVisualiserPlayer::keyPressEvent(QKeyEvent * event)
{
    if(current_map==65535 || CatchChallenger::QMap_client::all_map.find(current_map)==CatchChallenger::QMap_client::all_map.cend())
    {
        std::cerr << "MapVisualiserPlayer::keyPressEvent() ignored: current_map=" << current_map << std::endl;
        return;
    }

    //ignore the no arrow key
    if(keyAccepted.find(event->key())==keyAccepted.cend())
    {
        event->ignore();
        return;
    }

    //ignore the repeated event
    if(event->isAutoRepeat())
        return;

    //add to pressed key list
    keyPressed.insert(event->key());

    //apply the key
    keyPressParse();
}

void MapVisualiserPlayer::keyPressParse()
{
    //ignore is already in move
    if(blocked)
        return;
    if(inMove)
    {
        if(!moveTimer.isActive())
        {
            std::cerr << "!moveTimer.isActive() && inMove==true, reset, internal error" << std::endl;
            return;
        }
        else
            return;
    }

    if(keyPressed.size()==1 && keyPressed.find(Qt::Key_Return)!=keyPressed.cend())
    {
        keyPressed.erase(Qt::Key_Return);
        parseAction();
        return;
    }

    if(keyPressed.find(Qt::Key_Left)!=keyPressed.cend())
    {
        //already turned on this direction, then try move into this direction
        if(direction==CatchChallenger::Direction_look_at_left)
        {
            if(!canGoTo(CatchChallenger::Direction_move_at_left,current_map,x,y,true))
                return;//Can't do at the left!
            //the first step
            direction=CatchChallenger::Direction_move_at_left;
            inMove=true;
            moveStep=1;
            moveStepSlot();
            emit send_player_direction(direction);
        }
        //look in this direction
        else
        {
            Tiled::Cell cell=playerMapObject->cell();
            cell.setTile(playerTileset->tileAt(10));
            playerMapObject->setCell(cell);
            direction=CatchChallenger::Direction_look_at_left;
            lookToMove.start();
            emit send_player_direction(direction);
            parseStop();
        }
    }
    else if(keyPressed.find(Qt::Key_Right)!=keyPressed.cend())
    {
        //already turned on this direction, then try move into this direction
        if(direction==CatchChallenger::Direction_look_at_right)
        {
            if(!canGoTo(CatchChallenger::Direction_move_at_right,current_map,x,y,true))
                return;//Can't do at the right!
            //the first step
            direction=CatchChallenger::Direction_move_at_right;
            inMove=true;
            moveStep=1;
            moveStepSlot();
            emit send_player_direction(direction);
        }
        //look in this direction
        else
        {
            Tiled::Cell cell=playerMapObject->cell();
            cell.setTile(playerTileset->tileAt(4));
            playerMapObject->setCell(cell);
            direction=CatchChallenger::Direction_look_at_right;
            lookToMove.start();
            emit send_player_direction(direction);
            parseStop();
        }
    }
    else if(keyPressed.find(Qt::Key_Up)!=keyPressed.cend())
    {
        //already turned on this direction, then try move into this direction
        if(direction==CatchChallenger::Direction_look_at_top)
        {
            if(!canGoTo(CatchChallenger::Direction_move_at_top,current_map,x,y,true))
                return;//Can't do at the top!
            //the first step
            direction=CatchChallenger::Direction_move_at_top;
            inMove=true;
            moveStep=1;
            moveStepSlot();
            emit send_player_direction(direction);
        }
        //look in this direction
        else
        {
            Tiled::Cell cell=playerMapObject->cell();
            cell.setTile(playerTileset->tileAt(1));
            playerMapObject->setCell(cell);
            direction=CatchChallenger::Direction_look_at_top;
            lookToMove.start();
            emit send_player_direction(direction);
            parseStop();
        }
    }
    else if(keyPressed.find(Qt::Key_Down)!=keyPressed.cend())
    {
        //already turned on this direction, then try move into this direction
        if(direction==CatchChallenger::Direction_look_at_bottom)
        {
            if(!canGoTo(CatchChallenger::Direction_move_at_bottom,current_map,x,y,true))
                return;//Can't do at the bottom!
            //the first step
            direction=CatchChallenger::Direction_move_at_bottom;
            inMove=true;
            moveStep=1;
            moveStepSlot();
            emit send_player_direction(direction);
        }
        //look in this direction
        else
        {
            Tiled::Cell cell=playerMapObject->cell();
            cell.setTile(playerTileset->tileAt(7));
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
    QMap_client * map_full=CatchChallenger::QMap_client::all_map.at(current_map);
    if(!animationDisplayed)
    {
        //leave
        if(map_full->triggerAnimations.find(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)))!=map_full->triggerAnimations.cend())
        {
            TriggerAnimation* triggerAnimation=map_full->triggerAnimations.at(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)));
            triggerAnimation->startLeave();
        }
        animationDisplayed=true;
        //tiger the next tile
        const std::vector<CatchChallenger::CommonMap> &mapList=QtDatapackClientLoader::datapackLoader->get_mapList();
        CATCHCHALLENGER_TYPE_MAPID tempMapIndex=current_map;
        uint8_t nx=this->x;
        uint8_t ny=this->y;
        //set the final value (direction, position, ...)
        switch(direction)
        {
            case CatchChallenger::Direction_move_at_right:
            case CatchChallenger::Direction_move_at_top:
            case CatchChallenger::Direction_move_at_bottom:
            case CatchChallenger::Direction_move_at_left:
                CatchChallenger::MoveOnTheMap::move(mapList,direction,tempMapIndex,nx,ny,true);
            break;
            default:
            break;
        }
        //enter
        if(map_full->triggerAnimations.find(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(nx),static_cast<uint8_t>(ny)))!=map_full->triggerAnimations.cend())
        {
            TriggerAnimation* triggerAnimation=map_full->triggerAnimations.at(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(nx),static_cast<uint8_t>(ny)));
            triggerAnimation->startEnter();
        }
        //door
        if(map_full->doors.find(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(nx),static_cast<uint8_t>(ny)))!=map_full->doors.cend())
        {
            MapDoor* door=map_full->doors.at(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(nx),static_cast<uint8_t>(ny)));
            door->startOpen(250);
            {
                const int timerInterval=door->timeToOpen();
                if(timerInterval<0)
                    std::cerr << "QTimer negative interval at " << __FILE__ << ":" << __LINE__ << " value: " << timerInterval << std::endl;
                moveAnimationTimer.start(timerInterval);
            }

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
            cell.setTile(playerTileset->tileAt(baseTile+0));
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
    //monster
    if(monsterMapObject!=NULL)
    {
        if(inMove && moveStep==1)
            switch(direction)
            {
                case CatchChallenger::Direction_move_at_left:
                case CatchChallenger::Direction_move_at_right:
                case CatchChallenger::Direction_move_at_top:
                case CatchChallenger::Direction_move_at_bottom:
                    pendingMonsterMoves.push_back(direction);
                break;
                default:
                break;
            }
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(pendingMonsterMoves.size()>2)
            abort();
        #endif
        if(pendingMonsterMoves.size()>1)
        {
            //start move
            int baseTile=1;
            //move the player for intermediate step and define the base tile (define the stopped step with direction)
            switch(pendingMonsterMoves.front())
            {
                case CatchChallenger::Direction_move_at_left:
                baseTile=3;
                switch(moveStep)
                {
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    monsterMapObject->setX(monsterMapObject->x()-0.20);
                    break;
                }
                break;
                case CatchChallenger::Direction_move_at_right:
                baseTile=7;
                switch(moveStep)
                {
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    monsterMapObject->setX(monsterMapObject->x()+0.20);
                    break;
                }
                break;
                case CatchChallenger::Direction_move_at_top:
                baseTile=2;
                switch(moveStep)
                {
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    monsterMapObject->setY(monsterMapObject->y()-0.20);
                    break;
                }
                break;
                case CatchChallenger::Direction_move_at_bottom:
                baseTile=6;
                switch(moveStep)
                {
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    monsterMapObject->setY(monsterMapObject->y()+0.20);
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
                    Tiled::Cell cell=monsterMapObject->cell();
                    cell.setTile(monsterTileset->tileAt(baseTile+0));
                    monsterMapObject->setCell(cell);
                }
                break;
                case 1:
                MapObjectItem::objectLink.at(monsterMapObject)->setZValue(qCeil(monsterMapObject->y()));
                break;
                //transition step
                case 2:
                {
                    Tiled::Cell cell=monsterMapObject->cell();
                    cell.setTile(monsterTileset->tileAt(baseTile-2));
                    monsterMapObject->setCell(cell);
                }
                break;
                //stopped step
                case 4:
                {
                    Tiled::Cell cell=monsterMapObject->cell();
                    cell.setTile(monsterTileset->tileAt(baseTile+0));
                    monsterMapObject->setCell(cell);
                }
                break;
            }
        }
    }
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
            cell.setTile(playerTileset->tileAt(baseTile+0));
            playerMapObject->setCell(cell);
        }
        break;
        case 1:
        MapObjectItem::objectLink.at(playerMapObject)->setZValue(qCeil(playerMapObject->y()));
        break;
        //transition step
        case 2:
        {
            Tiled::Cell cell=playerMapObject->cell();
            if(stepAlternance)
                cell.setTile(playerTileset->tileAt(baseTile-1));
            else
                cell.setTile(playerTileset->tileAt(baseTile+1));
            playerMapObject->setCell(cell);
            stepAlternance=!stepAlternance;
        }
        break;
        //stopped step
        case 4:
        {
            Tiled::Cell cell=playerMapObject->cell();
            cell.setTile(playerTileset->tileAt(baseTile+0));
            playerMapObject->setCell(cell);
        }
        break;
    }

    centerOnPlayerTile();
    loadGrassTile();

    moveStep++;

    //if have finish the step
    if(moveStep>5)
    {
        if(monsterMapObject!=NULL)
            if(pendingMonsterMoves.size()>1)
            {
                const CatchChallenger::Direction direction=pendingMonsterMoves.front();
                pendingMonsterMoves.erase(pendingMonsterMoves.cbegin());

                const std::vector<CatchChallenger::CommonMap> &mapList=QtDatapackClientLoader::datapackLoader->get_mapList();
                CATCHCHALLENGER_TYPE_MAPID old_monster_map=current_monster_map;
                //set the final value (direction, position, ...)
                switch(direction)
                {
                    case CatchChallenger::Direction_move_at_left:
                    case CatchChallenger::Direction_move_at_right:
                    case CatchChallenger::Direction_move_at_top:
                    case CatchChallenger::Direction_move_at_bottom:
                        if(!CatchChallenger::MoveOnTheMap::move(mapList,direction,current_monster_map,monster_x,monster_y,true))
                        {
                            std::cerr << "Bug at move for pendingMonsterMoves, unknown move: " << std::to_string(direction)
                                      << " from map " << std::to_string(current_monster_map) << " (" << std::to_string(monster_x) << "," << std::to_string(monster_y) << ")"
                                      << std::endl;
                            resetMonsterTile();
                        }
                    break;
                    default:
                        qDebug() << QStringLiteral("moveStepSlot(): moveStep: %1, wrong direction (%2) when moveStep>2").arg(moveStep).arg(direction);
                    return;
                }
                //if the map have changed
                if(old_monster_map!=current_monster_map)
                {
                    unloadMonsterFromCurrentMap();
                    if(CatchChallenger::QMap_client::old_all_map.find(current_monster_map)==CatchChallenger::QMap_client::old_all_map.cend())
                        std::cerr << "old_all_map.find(current_monster_map)==old_all_map.cend() in monster follow" << std::endl;
                    loadMonsterFromCurrentMap();
                }

                monsterMapObject->setPosition(QPointF(monster_x-0.5,monster_y+1));
                MapObjectItem::objectLink.at(monsterMapObject)->setZValue(monster_y);
            }
        animationDisplayed=false;
        const std::vector<CatchChallenger::CommonMap> &mapList=QtDatapackClientLoader::datapackLoader->get_mapList();
        CATCHCHALLENGER_TYPE_MAPID old_map_index=current_map;
        //set the final value (direction, position, ...)
        //NOTE: use moveWithoutTeleport here so the player lands on the teleporter
        //source tile (e.g. a "teleport on push" wall). The actual cave->city swap
        //is then performed by finalPlayerStepTeleported() via MapControllerMP::finalPlayerStep.
        switch(direction)
        {
            case CatchChallenger::Direction_move_at_left:
            case CatchChallenger::Direction_move_at_right:
            case CatchChallenger::Direction_move_at_top:
            case CatchChallenger::Direction_move_at_bottom:
            {
                const uint8_t pre_x=x,pre_y=y;
                const CATCHCHALLENGER_TYPE_MAPID pre_map=current_map;
                if(!CatchChallenger::MoveOnTheMap::moveWithoutTeleport(mapList,direction,current_map,x,y,true))
                {
                    std::cerr << "Bug at move, unknown move: " << std::to_string(direction)
                              << " from map " << std::to_string(current_map) << " (" << std::to_string(x) << "," << std::to_string(y) << ")"
                              << std::endl;
                    return;
                }
                std::cerr << "moveStepSlot(): final step dir=" << std::to_string(direction)
                          << " from map=" << std::to_string(pre_map) << "(" << std::to_string(pre_x) << "," << std::to_string(pre_y) << ")"
                          << " -> map=" << std::to_string(current_map) << "(" << std::to_string(x) << "," << std::to_string(y) << ")"
                          << std::endl;
                direction=CatchChallenger::MoveOnTheMap::directionToDirectionLook(direction);
            }
            break;
            default:
                qDebug() << QStringLiteral("moveStepSlot(): moveStep: %1, wrong direction (%2) when moveStep>2").arg(moveStep).arg(direction);
            return;
        }
        //if the map have changed
        if(old_map_index!=current_map)
        {
            unloadPlayerFromCurrentMap();
            passMapIntoOld();
            if(CatchChallenger::QMap_client::old_all_map.find(current_map)==CatchChallenger::QMap_client::old_all_map.cend())
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

bool MapVisualiserPlayer::asyncMapLoaded(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,QMap_client * tempMapObject)
{
    std::cerr << "MapVisualiserPlayer::asyncMapLoaded() mapIndex=" << mapIndex << " current_map=" << current_map << std::endl;
    if(current_map==65535)
    {
        std::cerr << "MapVisualiserPlayer::asyncMapLoaded() current_map==65535, ignoring" << std::endl;
        return false;
    }
    if(MapVisualiser::asyncMapLoaded(mapIndex,tempMapObject))
    {
        if(tempMapObject!=NULL)
        {
            //item on map display logic removed - now handled via client/DatapackClientLoader

            if(mapIndex==current_map)
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

void MapVisualiserPlayer::unblock()
{
    blocked=false;
}

bool MapVisualiserPlayer::finalPlayerStepTeleported(bool &isTeleported)
{
    if(CatchChallenger::QMap_client::all_map.find(current_map)==CatchChallenger::QMap_client::all_map.cend())
    {
        qDebug() << "current map not loaded, unable to do finalPlayerStep()";
        return false;
    }
    const QMap_client * current_map_pointer=CatchChallenger::QMap_client::all_map.at(current_map);
    if(current_map_pointer==NULL)
    {
        qDebug() << "current map not loaded null pointer, unable to do finalPlayerStep()";
        return false;
    }
    if(!isTeleported)
    {
        const CatchChallenger::CommonMap &logicalMap=QtDatapackClientLoader::datapackLoader->getMap(current_map);
        int index=0;
        const int size=logicalMap.teleporters.size();
        std::cerr << "finalPlayerStepTeleported(): checking " << size << " teleporter(s) on map="
                  << std::to_string(current_map) << " pos=(" << std::to_string(x) << "," << std::to_string(y) << ")" << std::endl;
        while(index<size)
        {
            const CatchChallenger::Teleporter &current_teleport=logicalMap.teleporters.at(index);
            std::cerr << "  tp[" << index << "] src=(" << std::to_string(current_teleport.source_x) << "," << std::to_string(current_teleport.source_y)
                      << ") -> map=" << std::to_string(current_teleport.mapIndex)
                      << "(" << std::to_string(current_teleport.destination_x) << "," << std::to_string(current_teleport.destination_y) << ")"
                      << std::endl;
            //if need be teleported
            if(current_teleport.source_x==x && current_teleport.source_y==y)
            {
                std::cerr << "  -> MATCH: teleporting to map=" << std::to_string(current_teleport.mapIndex)
                          << "(" << std::to_string(current_teleport.destination_x) << "," << std::to_string(current_teleport.destination_y) << ")"
                          << std::endl;
                isTeleported=true;
                unloadPlayerFromCurrentMap();
                passMapIntoOld();
                //player coord
                current_map=current_teleport.mapIndex;
                x=current_teleport.destination_x;
                y=current_teleport.destination_y;
                //monster coord
                current_monster_map=current_teleport.mapIndex;
                monster_x=current_teleport.destination_x;
                monster_y=current_teleport.destination_y;

                if(!haveMapInMemory(current_map))
                    emit inWaitingOfMap();
                loadOtherMap(current_map);
                hideNotloadedMap();
                resetMonsterTile();
                return true;
            }
            index++;
        }
    }
    else
        isTeleported=false;
    return false;
}

void MapVisualiserPlayer::finalPlayerStep(bool parseKey)
{
    std::cout << "MapVisualiserPlayer::finalPlayerStep()" << std::endl;
    if(CatchChallenger::QMap_client::all_map.find(current_map)==CatchChallenger::QMap_client::all_map.cend())
    {
        qDebug() << "current map not loaded, unable to do finalPlayerStep()";
        return;
    }
    const QMap_client * current_map_pointer=CatchChallenger::QMap_client::all_map.at(current_map);
    if(current_map_pointer==NULL)
    {
        qDebug() << "current map not loaded null pointer, unable to do finalPlayerStep()";
        return;
    }
    const CatchChallenger::CommonMap &logicalMap=QtDatapackClientLoader::datapackLoader->getMap(current_map);

    /// \see into haveStopTileAction(), to NPC fight
    if(!CatchChallenger::CommonDatapack::commonDatapack.get_monstersCollision().empty())
    {
        //locate the right layer for monster
        if(monsterMapObject!=NULL)
        {
            const QMap_client * current_monster_map_pointer=CatchChallenger::QMap_client::all_map.at(current_monster_map);
            if(current_monster_map_pointer==NULL)
            {
                qDebug() << "current_monster_map_pointer not loaded null pointer, unable to do finalPlayerStep()";
                return;
            }
            const CatchChallenger::CommonMap &monsterLogicalMap=QtDatapackClientLoader::datapackLoader->getMap(current_monster_map);
            {
                const CatchChallenger::MonstersCollisionValue &monstersCollisionValue=
                        CatchChallenger::MoveOnTheMap::getZoneCollision(monsterLogicalMap,monster_x,monster_y);
                const CatchChallenger::ParsedLayerLedges &ledge=CatchChallenger::MoveOnTheMap::getLedge(monsterLogicalMap,monster_x,monster_y);
                if(ledge!=CatchChallenger::ParsedLayerLedges_NoLedges)
                    monsterMapObject->setVisible(true);
                else
                {
                    if(monstersCollisionValue.walkOn.empty())
                        monsterMapObject->setVisible(false);
                    else
                    {
                        bool visible=false;
                        unsigned int index=0;
                        while(index<monstersCollisionValue.walkOn.size())
                        {
                            const unsigned int &newIndex=monstersCollisionValue.walkOn.at(index);
                            if(newIndex<CatchChallenger::CommonDatapack::commonDatapack.get_monstersCollision().size())
                            {
                                const CatchChallenger::MonstersCollision &monstersCollision=
                                        CatchChallenger::CommonDatapack::commonDatapack.get_monstersCollision().at(newIndex);
                                const CatchChallenger::MonstersCollisionTemp &monstersCollisionTemp=
                                        CatchChallenger::CommonDatapack::commonDatapack.get_monstersCollisionTemp().at(newIndex);
                                if(monstersCollision.item==0)
                                {
                                    visible=(monstersCollisionTemp.tile.empty() && pendingMonsterMoves.size()>=1) ||
                                                                 (pendingMonsterMoves.size()==1 && !inMove)
                                                                 ;
                                }
                            }
                            index++;
                        }
                        monsterMapObject->setVisible(visible);
                    }
                }
            }
        }
        //locate the right layer
        const CatchChallenger::MonstersCollisionValue &monstersCollisionValue=
                CatchChallenger::MoveOnTheMap::getZoneCollision(logicalMap,x,y);
        unsigned int index=0;
        while(index<monstersCollisionValue.walkOn.size())
        {
            const unsigned int &newIndex=monstersCollisionValue.walkOn.at(index);
            if(newIndex<CatchChallenger::CommonDatapack::commonDatapack.get_monstersCollision().size())
            {
                const CatchChallenger::MonstersCollision &monstersCollision=
                        CatchChallenger::CommonDatapack::commonDatapack.get_monstersCollision().at(newIndex);
                const CatchChallenger::MonstersCollisionTemp &monstersCollisionTemp=
                        CatchChallenger::CommonDatapack::commonDatapack.get_monstersCollisionTemp().at(newIndex);
                if(monstersCollision.item==0)
                {
                    //change tile if needed (water to walk transition)
                    if(monstersCollisionTemp.tile!=lastTileset)
                    {
                        lastTileset=monstersCollisionTemp.tile;
                        if(playerTilesetCache.find(lastTileset)!=playerTilesetCache.cend())
                            playerTileset=playerTilesetCache.at(lastTileset);
                        else
                        {
                            if(lastTileset.empty())
                                playerTileset=playerTilesetCache[defaultTileset];
                            else
                            {
                                const std::string &imagePath=playerSkinPath+"/"+lastTileset+".png";
                                QImage image(QString::fromStdString(imagePath));
                                if(!image.isNull())
                                {
                                    playerTileset=Tiled::Tileset::create(QString::fromStdString(lastTileset),16,24);
                                    playerTileset->loadFromImage(image,QString::fromStdString(imagePath));
                                }
                                else
                                {
                                    qDebug() << "Unable to load the player tilset: "+QString::fromStdString(imagePath);
                                    playerTileset=playerTilesetCache[defaultTileset];
                                }
                            }
                            playerTilesetCache[lastTileset]=playerTileset;
                        }
                        if(playerTileset.isNull())
                        {
                            std::cerr << "ERROR playerTileset.isNull() (abort)" << std::endl;
                            abort();
                        }
                        {
                            Tiled::Cell cell=playerMapObject->cell();
                            if(cell.tile()!=nullptr)
                            {
                                int tileId=cell.tile()->id();
                                cell.setTile(playerTileset->tileAt(tileId));
                                playerMapObject->setCell(cell);
                            }
                            else
                            {
                                std::cerr << "ERROR Unable to load the player tilset, cell.tile=nullptr (abort)" << std::endl;
                                abort();
                            }
                        }
                    }
                    break;
                }
            }
            index++;
        }
        if(index==monstersCollisionValue.walkOn.size())
        {
            lastTileset=defaultTileset;
            playerTileset=playerTilesetCache[defaultTileset];
            {
                Tiled::Cell cell=playerMapObject->cell();
                int tileId=cell.tile()->id();
                cell.setTile(playerTileset->tileAt(tileId));
                playerMapObject->setCell(cell);
            }
        }
    }
    //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
    playerMapObject->setPosition(QPoint(x,y+1));
    MapObjectItem::objectLink.at(playerMapObject)->setZValue(y);
    centerOnPlayerTile();

    if(haveStopTileAction())
        return;

    if(CatchChallenger::MoveOnTheMap::getLedge(logicalMap,x,y)!=CatchChallenger::ParsedLayerLedges_NoLedges)
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
        return;
    }

    if(!parseKey)
        return;
    //check if one arrow key is pressed to continue to move into this direction
    if(keyPressed.find(Qt::Key_Left)!=keyPressed.cend())
    {
        //can't go into this direction, then just look into this direction
        if(!canGoTo(CatchChallenger::Direction_move_at_left,current_map,x,y,true))
        {
            keyPressed.erase(Qt::Key_Left);
            direction=CatchChallenger::Direction_look_at_left;
            Tiled::Cell cell=playerMapObject->cell();
            cell.setTile(playerTileset->tileAt(10));
            playerMapObject->setCell(cell);
            inMove=false;
            emit send_player_direction(direction);
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
        }
    }
    else if(keyPressed.find(Qt::Key_Right)!=keyPressed.cend())
    {
        if(!canGoTo(CatchChallenger::Direction_move_at_right,current_map,x,y,true))
        {
            keyPressed.erase(Qt::Key_Right);
            direction=CatchChallenger::Direction_look_at_right;
            Tiled::Cell cell=playerMapObject->cell();
            cell.setTile(playerTileset->tileAt(4));
            playerMapObject->setCell(cell);
            inMove=false;
            emit send_player_direction(direction);
            parseStop();
        }
        else
        {
            direction=CatchChallenger::Direction_move_at_right;
            moveStep=0;
            moveStepSlot();
            emit send_player_direction(direction);
            if(CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange && x==(logicalMap.width-1))
                emit send_player_direction(CatchChallenger::Direction_look_at_right);
        }
    }
    else if(keyPressed.find(Qt::Key_Up)!=keyPressed.cend())
    {
        if(!canGoTo(CatchChallenger::Direction_move_at_top,current_map,x,y,true))
        {
            keyPressed.erase(Qt::Key_Up);
            direction=CatchChallenger::Direction_look_at_top;
            Tiled::Cell cell=playerMapObject->cell();
            cell.setTile(playerTileset->tileAt(1));
            playerMapObject->setCell(cell);
            inMove=false;
            emit send_player_direction(direction);
            parseStop();
        }
        else
        {
            direction=CatchChallenger::Direction_move_at_top;
            moveStep=0;
            moveStepSlot();
            emit send_player_direction(direction);
            if(CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange && y==0)
                emit send_player_direction(CatchChallenger::Direction_look_at_top);
        }
    }
    else if(keyPressed.find(Qt::Key_Down)!=keyPressed.cend())
    {
        if(!canGoTo(CatchChallenger::Direction_move_at_bottom,current_map,x,y,true))
        {
            keyPressed.erase(Qt::Key_Down);
            direction=CatchChallenger::Direction_look_at_bottom;
            Tiled::Cell cell=playerMapObject->cell();
            cell.setTile(playerTileset->tileAt(7));
            playerMapObject->setCell(cell);
            inMove=false;
            emit send_player_direction(direction);
            parseStop();
        }
        else
        {
            direction=CatchChallenger::Direction_move_at_bottom;
            moveStep=0;
            moveStepSlot();
            emit send_player_direction(direction);
            if(CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange && y==(logicalMap.height-1))
                emit send_player_direction(CatchChallenger::Direction_look_at_bottom);
        }
    }
    //now stop walking, no more arrow key is pressed
    else
    {
        if(!nextPathStep())
        {
            if(inMove)
            {
                stopAndSend();
                parseStop();
            }
            if(wasPathFindingUsed)
            {
                if(keyPressed.empty())
                    parseAction();
                wasPathFindingUsed=false;
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
    const std::vector<CatchChallenger::CommonMap> &mapList=QtDatapackClientLoader::datapackLoader->get_mapList();
    const CatchChallenger::CommonMap &logicalMap=QtDatapackClientLoader::datapackLoader->getMap(current_map);
    CATCHCHALLENGER_TYPE_MAPID tempMapIndex=current_map;
    uint8_t lx=this->x;
    uint8_t ly=this->y;
    switch(direction)
    {
        case CatchChallenger::Direction_look_at_left:
        if(CatchChallenger::MoveOnTheMap::canGoTo(mapList,CatchChallenger::Direction_move_at_left,logicalMap,lx,ly,false))
        {
            if(!CatchChallenger::MoveOnTheMap::move(mapList,CatchChallenger::Direction_move_at_left,tempMapIndex,lx,ly,false))
                qDebug() << QStringLiteral("can't go at left at map %1 (%2,%3) when move have been checked").arg(current_map).arg(lx).arg(ly);
            else
            {
                const CatchChallenger::CommonMap &destMap=QtDatapackClientLoader::datapackLoader->getMap(tempMapIndex);
                emit stopped_in_front_of(const_cast<CatchChallenger::Map_client *>(static_cast<const CatchChallenger::Map_client *>(&destMap)),tempMapIndex,lx,ly);
            }
        }
        break;
        case CatchChallenger::Direction_look_at_right:
        if(CatchChallenger::MoveOnTheMap::canGoTo(mapList,CatchChallenger::Direction_move_at_right,logicalMap,lx,ly,false))
        {
            tempMapIndex=current_map; lx=this->x; ly=this->y;
            if(!CatchChallenger::MoveOnTheMap::move(mapList,CatchChallenger::Direction_move_at_right,tempMapIndex,lx,ly,false))
                qDebug() << QStringLiteral("can't go at right at map %1 (%2,%3) when move have been checked").arg(current_map).arg(lx).arg(ly);
            else
            {
                const CatchChallenger::CommonMap &destMap=QtDatapackClientLoader::datapackLoader->getMap(tempMapIndex);
                emit stopped_in_front_of(const_cast<CatchChallenger::Map_client *>(static_cast<const CatchChallenger::Map_client *>(&destMap)),tempMapIndex,lx,ly);
            }
        }
        break;
        case CatchChallenger::Direction_look_at_top:
        if(CatchChallenger::MoveOnTheMap::canGoTo(mapList,CatchChallenger::Direction_move_at_top,logicalMap,lx,ly,false))
        {
            tempMapIndex=current_map; lx=this->x; ly=this->y;
            if(!CatchChallenger::MoveOnTheMap::move(mapList,CatchChallenger::Direction_move_at_top,tempMapIndex,lx,ly,false))
                qDebug() << QStringLiteral("can't go at top at map %1 (%2,%3) when move have been checked").arg(current_map).arg(lx).arg(ly);
            else
            {
                const CatchChallenger::CommonMap &destMap=QtDatapackClientLoader::datapackLoader->getMap(tempMapIndex);
                emit stopped_in_front_of(const_cast<CatchChallenger::Map_client *>(static_cast<const CatchChallenger::Map_client *>(&destMap)),tempMapIndex,lx,ly);
            }
        }
        break;
        case CatchChallenger::Direction_look_at_bottom:
        if(CatchChallenger::MoveOnTheMap::canGoTo(mapList,CatchChallenger::Direction_move_at_bottom,logicalMap,lx,ly,false))
        {
            tempMapIndex=current_map; lx=this->x; ly=this->y;
            if(!CatchChallenger::MoveOnTheMap::move(mapList,CatchChallenger::Direction_move_at_bottom,tempMapIndex,lx,ly,false))
                qDebug() << QStringLiteral("can't go at bottom at map %1 (%2,%3) when move have been checked").arg(current_map).arg(lx).arg(ly);
            else
            {
                const CatchChallenger::CommonMap &destMap=QtDatapackClientLoader::datapackLoader->getMap(tempMapIndex);
                emit stopped_in_front_of(const_cast<CatchChallenger::Map_client *>(static_cast<const CatchChallenger::Map_client *>(&destMap)),tempMapIndex,lx,ly);
            }
        }
        break;
        default:
        break;
    }
}

void MapVisualiserPlayer::stopAndSend()
{
    inMove=false;
    switch(direction)
    {
        case CatchChallenger::Direction_move_at_bottom:
        case CatchChallenger::Direction_move_at_left:
        case CatchChallenger::Direction_move_at_right:
        case CatchChallenger::Direction_move_at_top:
            direction=(CatchChallenger::Direction)((uint8_t)direction-4);
        break;
        default:
        break;
    }
    emit send_player_direction(direction);
}

void MapVisualiserPlayer::parseAction()
{
    {//to prevent flood and kick from server, mostly with infinite item on map
        if(lastAction.elapsed()<400)
            return;
        lastAction.restart();
    }
    const std::vector<CatchChallenger::CommonMap> &mapList=QtDatapackClientLoader::datapackLoader->get_mapList();
    const CatchChallenger::CommonMap &logicalMap=QtDatapackClientLoader::datapackLoader->getMap(current_map);
    CATCHCHALLENGER_TYPE_MAPID tempMapIndex=current_map;
    uint8_t lx=this->x;
    uint8_t ly=this->y;
    switch(direction)
    {
        case CatchChallenger::Direction_look_at_left:
        if(CatchChallenger::MoveOnTheMap::canGoTo(mapList,CatchChallenger::Direction_move_at_left,logicalMap,lx,ly,false))
        {
            if(!CatchChallenger::MoveOnTheMap::move(mapList,CatchChallenger::Direction_move_at_left,tempMapIndex,lx,ly,false))
                qDebug() << QStringLiteral("can't go at left at map %1 (%2,%3) when move have been checked").arg(current_map).arg(lx).arg(ly);
            else
            {
                CatchChallenger::CommonMap &destMap=const_cast<CatchChallenger::CommonMap &>(QtDatapackClientLoader::datapackLoader->getMap(tempMapIndex));
                if(CatchChallenger::QMap_client::all_map.find(tempMapIndex)!=CatchChallenger::QMap_client::all_map.cend())
                {
                    QMap_client *destMapFull=CatchChallenger::QMap_client::all_map.at(tempMapIndex);
                    if(destMapFull->botsDisplay.find(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(lx),static_cast<uint8_t>(ly)))!=
                            destMapFull->botsDisplay.cend())
                    {
                        CatchChallenger::BotDisplay *botDisplay=&destMapFull->botsDisplay[
                                std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(lx),static_cast<uint8_t>(ly))
                                ];
                        Tiled::Cell cell=botDisplay->mapObject->cell();
                        cell.setTile(botDisplay->tileset->tileAt(4));
                        botDisplay->mapObject->setCell(cell);
                    }
                }
                emit actionOn(static_cast<CatchChallenger::Map_client *>(&destMap),tempMapIndex,lx,ly);
            }
        }
        break;
        case CatchChallenger::Direction_look_at_right:
        if(CatchChallenger::MoveOnTheMap::canGoTo(mapList,CatchChallenger::Direction_move_at_right,logicalMap,lx,ly,false))
        {
            tempMapIndex=current_map; lx=this->x; ly=this->y;
            if(!CatchChallenger::MoveOnTheMap::move(mapList,CatchChallenger::Direction_move_at_right,tempMapIndex,lx,ly,false))
                qDebug() << QStringLiteral("can't go at right at map %1 (%2,%3) when move have been checked").arg(current_map).arg(lx).arg(ly);
            else
            {
                CatchChallenger::CommonMap &destMap=const_cast<CatchChallenger::CommonMap &>(QtDatapackClientLoader::datapackLoader->getMap(tempMapIndex));
                if(CatchChallenger::QMap_client::all_map.find(tempMapIndex)!=CatchChallenger::QMap_client::all_map.cend())
                {
                    QMap_client *destMapFull=CatchChallenger::QMap_client::all_map.at(tempMapIndex);
                    if(destMapFull->botsDisplay.find(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(lx),static_cast<uint8_t>(ly)))!=
                            destMapFull->botsDisplay.cend())
                    {
                        CatchChallenger::BotDisplay *botDisplay=&destMapFull->botsDisplay[std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(lx),static_cast<uint8_t>(ly))];
                        Tiled::Cell cell=botDisplay->mapObject->cell();
                        cell.setTile(botDisplay->tileset->tileAt(10));
                        botDisplay->mapObject->setCell(cell);
                    }
                }
                emit actionOn(static_cast<CatchChallenger::Map_client *>(&destMap),tempMapIndex,lx,ly);
            }
        }
        break;
        case CatchChallenger::Direction_look_at_top:
        if(CatchChallenger::MoveOnTheMap::canGoTo(mapList,CatchChallenger::Direction_move_at_top,logicalMap,lx,ly,false))
        {
            tempMapIndex=current_map; lx=this->x; ly=this->y;
            if(!CatchChallenger::MoveOnTheMap::move(mapList,CatchChallenger::Direction_move_at_top,tempMapIndex,lx,ly,false))
                qDebug() << QStringLiteral("can't go at top at map %1 (%2,%3) when move have been checked").arg(current_map).arg(lx).arg(ly);
            else
            {
                CatchChallenger::CommonMap &destMap=const_cast<CatchChallenger::CommonMap &>(QtDatapackClientLoader::datapackLoader->getMap(tempMapIndex));
                if(CatchChallenger::QMap_client::all_map.find(tempMapIndex)!=CatchChallenger::QMap_client::all_map.cend())
                {
                    QMap_client *destMapFull=CatchChallenger::QMap_client::all_map.at(tempMapIndex);
                    if(destMapFull->botsDisplay.find(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(lx),static_cast<uint8_t>(ly)))!=
                            destMapFull->botsDisplay.cend())
                    {
                        CatchChallenger::BotDisplay *botDisplay=&destMapFull->botsDisplay[
                                std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(lx),static_cast<uint8_t>(ly))];
                        Tiled::Cell cell=botDisplay->mapObject->cell();
                        cell.setTile(botDisplay->tileset->tileAt(7));
                        botDisplay->mapObject->setCell(cell);
                    }
                }
                emit actionOn(static_cast<CatchChallenger::Map_client *>(&destMap),tempMapIndex,lx,ly);
            }
        }
        break;
        case CatchChallenger::Direction_look_at_bottom:
        if(CatchChallenger::MoveOnTheMap::canGoTo(mapList,CatchChallenger::Direction_move_at_bottom,logicalMap,lx,ly,false))
        {
            tempMapIndex=current_map; lx=this->x; ly=this->y;
            if(!CatchChallenger::MoveOnTheMap::move(mapList,CatchChallenger::Direction_move_at_bottom,tempMapIndex,lx,ly,false))
                qDebug() << QStringLiteral("can't go at bottom at map %1 (%2,%3) when move have been checked").arg(current_map).arg(lx).arg(ly);
            else
            {
                CatchChallenger::CommonMap &destMap=const_cast<CatchChallenger::CommonMap &>(QtDatapackClientLoader::datapackLoader->getMap(tempMapIndex));
                if(CatchChallenger::QMap_client::all_map.find(tempMapIndex)!=CatchChallenger::QMap_client::all_map.cend())
                {
                    QMap_client *destMapFull=CatchChallenger::QMap_client::all_map.at(tempMapIndex);
                    if(destMapFull->botsDisplay.find(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(lx),static_cast<uint8_t>(ly)))!=
                            destMapFull->botsDisplay.cend())
                    {
                        CatchChallenger::BotDisplay *botDisplay=&destMapFull->botsDisplay[std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(lx),static_cast<uint8_t>(ly))];
                        Tiled::Cell cell=botDisplay->mapObject->cell();
                        cell.setTile(botDisplay->tileset->tileAt(1));
                        botDisplay->mapObject->setCell(cell);
                    }
                }
                emit actionOn(static_cast<CatchChallenger::Map_client *>(&destMap),tempMapIndex,lx,ly);
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
        if(keyPressed.find(Qt::Key_Left)!=keyPressed.cend() &&
                canGoTo(CatchChallenger::Direction_move_at_left,current_map,x,y,true))
        {
            direction=CatchChallenger::Direction_move_at_left;
            inMove=true;
            moveStep=1;
            moveStepSlot();
            emit send_player_direction(direction);
        }
        break;
        case CatchChallenger::Direction_look_at_right:
        if(keyPressed.find(Qt::Key_Right)!=keyPressed.cend() &&
                canGoTo(CatchChallenger::Direction_move_at_right,current_map,x,y,true))
        {
            direction=CatchChallenger::Direction_move_at_right;
            inMove=true;
            moveStep=1;
            moveStepSlot();
            emit send_player_direction(direction);
        }
        break;
        case CatchChallenger::Direction_look_at_top:
        if(keyPressed.find(Qt::Key_Up)!=keyPressed.cend() &&
                canGoTo(CatchChallenger::Direction_move_at_top,current_map,x,y,true))
        {
            direction=CatchChallenger::Direction_move_at_top;
            inMove=true;
            moveStep=1;
            moveStepSlot();
            emit send_player_direction(direction);
        }
        break;
        case CatchChallenger::Direction_look_at_bottom:
        if(keyPressed.find(Qt::Key_Down)!=keyPressed.cend() &&
                canGoTo(CatchChallenger::Direction_move_at_bottom,current_map,x,y,true))
        {
            direction=CatchChallenger::Direction_move_at_bottom;
            inMove=true;
            moveStep=1;
            moveStepSlot();
            emit send_player_direction(direction);
        }
        break;
        default:
        qDebug() << QStringLiteral("transformLookToMove(): wrong direction");
        return;
    }
}

void MapVisualiserPlayer::keyReleaseEvent(QKeyEvent * event)
{
    if(current_map==65535 || CatchChallenger::QMap_client::all_map.find(current_map)==CatchChallenger::QMap_client::all_map.cend())
    {
        std::cerr << "MapVisualiserPlayer::keyReleaseEvent() ignored: current_map=" << current_map << std::endl;
        return;
    }

    //ignore the no arrow key
    if(keyAccepted.find(event->key())==keyAccepted.cend())
    {
        event->ignore();
        return;
    }

    //ignore the repeated event
    if(event->isAutoRepeat())
        return;

    //remove from the key list pressed
    keyPressed.erase(event->key());

    if(keyPressed.size()>0)//another key pressed, repeat
        keyPressParse();
}

CATCHCHALLENGER_TYPE_MAPID MapVisualiserPlayer::currentMap() const
{
    return current_map;
}

QMap_client * MapVisualiserPlayer::currentMapFull() const
{
    return CatchChallenger::QMap_client::all_map.at(current_map);
}

bool MapVisualiserPlayer::currentMapIsLoaded() const
{
    if(CatchChallenger::QMap_client::all_map.find(current_map)==CatchChallenger::QMap_client::all_map.cend())
        return false;
    return true;
}

std::string MapVisualiserPlayer::currentMapType() const
{
    if(CatchChallenger::QMap_client::all_map.find(current_map)==CatchChallenger::QMap_client::all_map.cend())
        return std::string();
    const QMap_client * const mapFull=CatchChallenger::QMap_client::all_map.at(current_map);
    const Tiled::Map * const tiledMap=mapFull->tiledMap.get();
    const Tiled::Properties &properties=tiledMap->properties();
    if(properties.find("type")!=properties.cend())
        if(!properties.value("type").toString().isEmpty())
            return properties.value("type").toString().toStdString();
    if(!mapFull->visualType.empty())
        return mapFull->visualType;
    return std::string();
}

std::string MapVisualiserPlayer::currentZone() const
{
    const QMap_client * const mapFull=CatchChallenger::QMap_client::all_map.at(current_map);
    const Tiled::Map * const tiledMap=mapFull->tiledMap.get();
    const Tiled::Properties &properties=tiledMap->properties();
    if(properties.find("zone")!=properties.cend())
        if(!properties.value("zone").toString().isEmpty())
            return properties.value("zone").toString().toStdString();
    if(!mapFull->zone.empty())
        return mapFull->zone;
    return std::string();
}

std::string MapVisualiserPlayer::currentBackgroundsound() const
{
    const QMap_client * const mapFull=CatchChallenger::QMap_client::all_map.at(current_map);
    const Tiled::Map * const tiledMap=mapFull->tiledMap.get();
    const Tiled::Properties &properties=tiledMap->properties();
    if(properties.find("backgroundsound")!=properties.cend())
        if(!properties.value("backgroundsound").toString().isEmpty())
            return properties.value("backgroundsound").toString().toStdString();
    if(!mapFull->backgroundsound.empty())
        return mapFull->backgroundsound;
    return std::string();
}

CatchChallenger::Direction MapVisualiserPlayer::getDirection()
{
    return direction;
}

bool MapVisualiserPlayer::loadPlayerMap(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const uint8_t &x,const uint8_t &y)
{
    this->x=x;
    this->y=y;
    this->monster_x=x;
    this->monster_y=y;
    current_map=mapIndex;
    current_monster_map=mapIndex;
    return true;
}

bool MapVisualiserPlayer::insert_player_internal(const CatchChallenger::Player_public_informations &player,
       const uint32_t &mapId,const uint16_t &x,const uint16_t &y,const CatchChallenger::Direction &direction,
                                              const std::vector<std::string> &skinFolderList)
{
    std::cout << "MapVisualiserPlayer::insert_player_internal()" << std::endl;
    if(!mHaveTheDatapack || !player_informations_is_set)
    {
        emit error("MapVisualiserPlayer::insert_player_final(): !mHaveTheDatapack || !player_informations_is_set");
        return false;
    }
    if(mapId>=(uint32_t)QtDatapackClientLoader::datapackLoader->get_maps().size())
    {
        /// \bug here pass after delete a party, create a new
        emit error("insert_player_internal(): mapId="+std::to_string(mapId)+" >= maps.size()="+
                   std::to_string(QtDatapackClientLoader::datapackLoader->get_maps().size()));
        return true;
    }
    //current player - use monsterId to identify (simplifiedId removed)
    {
        std::cout << "MapVisualiserPlayer::insert_player_internal() loading player" << std::endl;
        //ignore to improve the performance server because can reinsert all player of map using the overall client list
        if(current_map!=65535)
        {
            qDebug() << "Current player already loaded on the map (current_map=" << current_map << ")";
            return true;
        }
        /// \todo do a player cache here
        //the player skin
        std::cerr << "set playerTileset here from: " << std::to_string(player.skinId) << std::endl;
        if(player.skinId<skinFolderList.size())
        {
            playerSkinPath=datapackPath+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(player.skinId);
            const std::string &imagePath=playerSkinPath+"/trainer.png";
            QImage image(QString::fromStdString(imagePath));
            if(!image.isNull())
            {
                if(!playerTileset->loadFromImage(image,QString::fromStdString(imagePath)))
                    abort();
                if(playerTileset->tileCount()<12)
                {
                    std::cerr << "ERROR: playerTileset->tileCount()<12 at " << imagePath << " (abort)" << std::endl;
                    abort();
                }
                if(playerTileset->tileAt(1)!=nullptr)
                    std::cout << "correctly loaded at " << imagePath << std::endl;
                else
                {
                    std::cerr << "ERROR: playerTileset->tileCount()<12 at " << imagePath << " (abort)" << std::endl;
                    abort();
                }
            }
            else
                qDebug() << "ERROR: Unable to load the player tilset: "+QString::fromStdString(imagePath);
        }
        else
            qDebug() << "ERROR: The skin id: "+QString::number(player.skinId)+", into a list of: "+QString::number(skinFolderList.size())+" item(s) info MapControllerMP::insert_player()";

        //the direction
        this->direction=direction;
        switch(direction)
        {
            case CatchChallenger::Direction_move_at_top:
            case CatchChallenger::Direction_move_at_right:
            case CatchChallenger::Direction_move_at_bottom:
            case CatchChallenger::Direction_move_at_left:
            QMessageBox::critical(NULL,tr("Internal error")+", file: "+QString(__FILE__)+":"+QString::number(__LINE__),tr("The direction send by the server is wrong"));
            return true;
            default:
            break;
        }
        if(playerTileset==nullptr)
        {
            std::cerr << "set playerTileset nullptr into cell.tile (abort)" << std::endl;
            abort();
        }
        std::cout << "MapVisualiserPlayer::insert_player_internal() direction: " << std::to_string(direction) << " lastTileset: " << lastTileset << std::endl;
        switch(direction)
        {
            case CatchChallenger::Direction_look_at_top:
            case CatchChallenger::Direction_move_at_top:
            {
                Tiled::Cell cell=playerMapObject->cell();
                cell.setTile(playerTileset->tileAt(1));
                if(cell.tile()==nullptr)
                {
                    std::cerr << "set nullptr into cell.tile (abort)" << std::endl;
                    abort();
                }
                playerMapObject->setCell(cell);
            }
            break;
            case CatchChallenger::Direction_look_at_right:
            case CatchChallenger::Direction_move_at_right:
            {
                Tiled::Cell cell=playerMapObject->cell();
                cell.setTile(playerTileset->tileAt(4));
                if(cell.tile()==nullptr)
                {
                    std::cerr << "set nullptr into cell.tile (abort)" << std::endl;
                    abort();
                }
                playerMapObject->setCell(cell);
            }
            break;
            case CatchChallenger::Direction_look_at_bottom:
            case CatchChallenger::Direction_move_at_bottom:
            {
                Tiled::Cell cell=playerMapObject->cell();
                cell.setTile(playerTileset->tileAt(7));
                if(cell.tile()==nullptr)
                {
                    std::cerr << "set nullptr into cell.tile (abort)" << std::endl;
                    abort();
                }
                playerMapObject->setCell(cell);
            }
            break;
            case CatchChallenger::Direction_look_at_left:
            case CatchChallenger::Direction_move_at_left:
            {
                Tiled::Cell cell=playerMapObject->cell();
                cell.setTile(playerTileset->tileAt(10));
                if(cell.tile()==nullptr)
                {
                    std::cerr << "set nullptr into cell.tile (abort)" << std::endl;
                    abort();
                }
                playerMapObject->setCell(cell);
            }
            break;
            default:
            QMessageBox::critical(NULL,tr("Internal error")+", file: "+QString(__FILE__)+":"+QString::number(__LINE__),tr("The direction send by the server is wrong"));
            return true;
        }

        //monster
        updatePlayerMonsterTile(player.monsterId);

        current_map=static_cast<CATCHCHALLENGER_TYPE_MAPID>(mapId);
        if(datapackMapPathSpec.empty())
        {
            std::cout << "datapackMapPathSpec can't be empty at this point " << __FILE__ << ":" << __LINE__ << " MapVisualiserPlayer::setDatapackPath() was not called (abort)" << std::endl;
            abort();
        }
        loadPlayerMap(static_cast<CATCHCHALLENGER_TYPE_MAPID>(mapId),
                      static_cast<uint8_t>(x),static_cast<uint8_t>(y));
    }
    return true;
}

void MapVisualiserPlayer::resetAll()
{
    if(!playerTileset->loadFromImage(QImage(QStringLiteral(":/CC/images/player_default/trainer.png")),QStringLiteral(":/CC/images/player_default/trainer.png")))
        qDebug() << "Unable the load the default player tileset";
    current_monster_map=65535;
    unloadPlayerFromCurrentMap();
    moveTimer.setInterval(250/5);
    moveTimer.setSingleShot(true);
    moveAnimationTimer.setInterval(250/5);
    moveAnimationTimer.setSingleShot(true);
    timer.stop();
    moveTimer.stop();
    lookToMove.stop();
    keyPressed.clear();
    blocked=false;
    wasPathFindingUsed=false;
    inMove=false;
    MapVisualiser::resetAll();
    lastAction.restart();
    mapVisualiserThread.stopIt=true;
    #ifndef NOTHREADS
    mapVisualiserThread.quit();
    mapVisualiserThread.wait();
    mapVisualiserThread.start(QThread::IdlePriority);
    #endif

    lastTileset=defaultTileset;
    playerTileset=Tiled::Tileset::create(QStringLiteral("player"),16,24);
    playerTilesetCache[lastTileset]=playerTileset;
    playerMapObject = new Tiled::MapObject();
}

bool MapVisualiserPlayer::canGoTo(const CatchChallenger::Direction &direction, const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y, const bool &checkCollision)
{
    const std::vector<CatchChallenger::CommonMap> &mapList=QtDatapackClientLoader::datapackLoader->get_mapList();
    const CatchChallenger::CommonMap &logicalMap=QtDatapackClientLoader::datapackLoader->getMap(mapIndex);
    CATCHCHALLENGER_TYPE_MAPID tempMapIndex=mapIndex;
    COORD_TYPE lx=x,ly=y;
    CatchChallenger::ParsedLayerLedges ledge;
    do
    {
        if(!CatchChallenger::MoveOnTheMap::canGoTo(mapList,direction,logicalMap,lx,ly,checkCollision && !clip))
        {
            std::cerr << "MapVisualiserPlayer::canGoTo() MoveOnTheMap::canGoTo returned false dir=" << std::to_string(direction)
                      << " map=" << std::to_string(mapIndex) << " pos=(" << std::to_string(lx) << "," << std::to_string(ly) << ") checkCollision="
                      << (checkCollision && !clip) << std::endl;
            return false;
        }
        if(!CatchChallenger::MoveOnTheMap::move(mapList,direction,tempMapIndex,lx,ly,checkCollision && !clip))
            return false;
        if(CatchChallenger::QMap_client::all_map.find(tempMapIndex)==CatchChallenger::QMap_client::all_map.cend())
        {
            std::cerr << "MapVisualiserPlayer::canGoTo() destination map " << std::to_string(tempMapIndex)
                      << " not in all_map (pos=" << std::to_string(lx) << "," << std::to_string(ly) << "), blocking move" << std::endl;
            return false;
        }
        const CatchChallenger::CommonMap &destMap=QtDatapackClientLoader::datapackLoader->getMap(tempMapIndex);
        {
            const std::pair<uint8_t,uint8_t> pos(static_cast<uint8_t>(lx),static_cast<uint8_t>(ly));
            if(destMap.items.find(pos)!=destMap.items.cend())
                return false;
        }
        ledge=CatchChallenger::MoveOnTheMap::getLedge(destMap,lx,ly);
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
    std::cerr << "MapVisualiserPlayer::loadPlayerFromCurrentMap() current_map=" << current_map << " x=" << std::to_string(x) << " y=" << std::to_string(y) << " centerOnPlayer=" << centerOnPlayer << std::endl;
    std::cerr << "[CENTER DEBUG] centerOnPlayer=" << (centerOnPlayer ? "TRUE" : "FALSE") << std::endl;
    if(CatchChallenger::QMap_client::all_map.find(current_map)==CatchChallenger::QMap_client::all_map.cend())
    {
        std::cerr << "MapVisualiserPlayer::loadPlayerFromCurrentMap() all_map has no current_map=" << current_map << std::endl;
        return;
    }
    {
        Tiled::ObjectGroup *currentGroup=playerMapObject->objectGroup();
        if(currentGroup!=NULL)
        {
            if(ObjectGroupItem::objectGroupLink.find(currentGroup)!=ObjectGroupItem::objectGroupLink.cend())
                ObjectGroupItem::objectGroupLink.at(currentGroup)->removeObject(playerMapObject);
            if(currentGroup!=CatchChallenger::QMap_client::all_map.at(current_map)->objectGroup)
                qDebug() << QStringLiteral("loadPlayerFromCurrentMap(), the playerMapObject group is wrong: %1").arg(currentGroup->name());
        }
        Tiled::ObjectGroup * objectGroup=CatchChallenger::QMap_client::all_map.at(current_map)->objectGroup;
        if(ObjectGroupItem::objectGroupLink.find(objectGroup)!=ObjectGroupItem::objectGroupLink.cend())
            ObjectGroupItem::objectGroupLink.at(objectGroup)->addObject(playerMapObject);
        else
            qDebug() << QStringLiteral("loadPlayerFromCurrentMap(), ObjectGroupItem::objectGroupLink not contains current_map->objectGroup");

        //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
        playerMapObject->setPosition(QPoint(x,y+1));
        MapObjectItem::objectLink.at(playerMapObject)->setZValue(y);
        centerOnPlayerTile();
    }

    loadMonsterFromCurrentMap();
}

void MapVisualiserPlayer::loadMonsterFromCurrentMap()
{
    if(monsterMapObject==nullptr)
        return;
    //monster
    if(CatchChallenger::QMap_client::all_map.find(current_monster_map)==CatchChallenger::QMap_client::all_map.cend())
    {
        qDebug() << QStringLiteral("all_map have not the current monster map: %1").arg(current_monster_map);
        return;
    }
    {
        Tiled::ObjectGroup *currentGroup=monsterMapObject->objectGroup();
        if(currentGroup!=NULL)
        {
            if(ObjectGroupItem::objectGroupLink.find(currentGroup)!=ObjectGroupItem::objectGroupLink.cend())
                ObjectGroupItem::objectGroupLink.at(currentGroup)->removeObject(monsterMapObject);
            if(currentGroup!=CatchChallenger::QMap_client::all_map.at(current_monster_map)->objectGroup)
                qDebug() << QStringLiteral("loadPlayerFromCurrentMap(), the monsterMapObject group is wrong: %1").arg(currentGroup->name());
        }
        if(ObjectGroupItem::objectGroupLink.find(CatchChallenger::QMap_client::all_map.at(current_monster_map)->objectGroup)!=ObjectGroupItem::objectGroupLink.cend())
            ObjectGroupItem::objectGroupLink.at(CatchChallenger::QMap_client::all_map.at(current_monster_map)->objectGroup)->addObject(monsterMapObject);
        else
            qDebug() << QStringLiteral("loadPlayerFromCurrentMap(), ObjectGroupItem::objectGroupLink not contains current_monster_map->objectGroup");
        //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
        monsterMapObject->setPosition(QPointF(monster_x-0.5,monster_y+1));
        MapObjectItem::objectLink.at(monsterMapObject)->setZValue(monster_y);
    }
}

//call before leave the old map (and before loadPlayerFromCurrentMap())
void MapVisualiserPlayer::unloadPlayerFromCurrentMap()
{
    if(playerMapObject==nullptr)
        return;
    {
        Tiled::ObjectGroup *currentGroup=playerMapObject->objectGroup();
        if(currentGroup==NULL)
            return;
        //unload the player sprite
        if(ObjectGroupItem::objectGroupLink.find(playerMapObject->objectGroup())!=
                ObjectGroupItem::objectGroupLink.cend())
            ObjectGroupItem::objectGroupLink.at(playerMapObject->objectGroup())->removeObject(playerMapObject);
        else
            qDebug() << QStringLiteral("unloadPlayerFromCurrentMap(), ObjectGroupItem::objectGroupLink not contains playerMapObject->objectGroup()");
    }
}

void MapVisualiserPlayer::unloadMonsterFromCurrentMap()
{
    //monster
    {
        Tiled::ObjectGroup *currentGroup=monsterMapObject->objectGroup();
        if(currentGroup==NULL)
            return;
        //unload the player sprite
        if(ObjectGroupItem::objectGroupLink.find(monsterMapObject->objectGroup())!=
                ObjectGroupItem::objectGroupLink.cend())
            ObjectGroupItem::objectGroupLink.at(monsterMapObject->objectGroup())->removeObject(monsterMapObject);
        else
            qDebug() << QStringLiteral("unloadPlayerFromCurrentMap(), ObjectGroupItem::objectGroupLink not contains monsterMapObject->objectGroup()");
    }
}

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
                cell.setTile(animationTileset->tileAt(0));
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
                cell.setTile(animationTileset->tileAt(2));
                nextCurrentObject->setCell(cell);
            }
            break;
        }
    }
}

void MapVisualiserPlayer::mapDisplayedSlot(const CATCHCHALLENGER_TYPE_MAPID &mapIndex)
{
    std::cerr << "MapVisualiserPlayer::mapDisplayedSlot() mapIndex=" << mapIndex << " current_map=" << current_map << std::endl;
    if(current_map==mapIndex)
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
    if(CatchChallenger::QMap_client::all_map.find(current_map)!=CatchChallenger::QMap_client::all_map.cend())
        return const_cast<CatchChallenger::Map_client *>(static_cast<const CatchChallenger::Map_client *>(&QtDatapackClientLoader::datapackLoader->getMap(current_map)));
    else
        return NULL;
}

//the datapack
void MapVisualiserPlayer::setDatapackPath(const std::string &path,const std::string &mainDatapackCode)
{
    std::cout << "MapVisualiserPlayer::setDatapackPath(" << path << "," << mainDatapackCode << ")" << std::endl;
    #ifdef DEBUG_CLIENT_LOAD_ORDER
    qDebug() << QStringLiteral("MapControllerMP::setDatapackPath()");
    #endif
    if(mainDatapackCode.find("/")!=std::string::npos)
    {
        std::cerr << "mainDatapackCode is not Path, forbiden / detected (abort)" << std::endl;
        abort();
    }
    if(mainDatapackCode=="[main]")
    {
        std::cerr << "mainDatapackCode is the default value, forbiden [main] detected (abort)" << std::endl;
        abort();
    }

    if(stringEndsWith(path,'/') || stringEndsWith(path,'\\'))
        datapackPath=path;
    else
        datapackPath=path+"/";
    datapackMapPathBase=QFileInfo(QString::fromStdString(datapackPath)+DATAPACK_BASE_PATH_MAPBASE).absoluteFilePath().toStdString();
    if(!stringEndsWith(datapackMapPathBase,'/') && !stringEndsWith(datapackMapPathBase,'\\'))
        datapackMapPathBase+="/";
    datapackMapPathSpec=QFileInfo(QString::fromStdString(datapackPath)+DATAPACK_BASE_PATH_MAPMAIN+QString::fromStdString(mainDatapackCode)+"/").absoluteFilePath().toStdString();
    if(!stringEndsWith(datapackMapPathSpec,'/') && !stringEndsWith(datapackMapPathSpec,'\\'))
        datapackMapPathSpec+="/";
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

void MapVisualiserPlayer::resetMonsterTile()
{
    if(monsterMapObject==nullptr)
        return;
    this->monster_x=x;
    this->monster_y=y;
    pendingMonsterMoves.clear();
    monsterMapObject->setVisible(false);
}

void MapVisualiserPlayer::updatePlayerMonsterTile(const uint16_t &monster)
{
    std::cerr << "MapVisualiserPlayer::updatePlayerMonsterTile() monster=" << monster << std::endl;
    bool resetMonster=false;
    if(monsterMapObject!=NULL)
    {
        unloadMonsterFromCurrentMap();
        monsterMapObject=NULL;
        resetMonster=true;
    }
    monsterTileset=NULL;
    //player_informations removed - monsterId tracked via client
    const std::string &imagePath=datapackPath+DATAPACK_BASE_PATH_MONSTERS+std::to_string(monster)+"/overworld.png";
    if(monsterTilesetCache.find(imagePath)!=monsterTilesetCache.cend())
        monsterTileset=monsterTilesetCache.at(imagePath);
    else
    {
        QImage image(QString::fromStdString(imagePath));
        if(!image.isNull())
        {
            monsterTileset=Tiled::Tileset::create(QString::fromStdString(lastTileset),32,32);
            if(!monsterTileset->loadFromImage(image,QString::fromStdString(imagePath)))
                abort();
            monsterTilesetCache[imagePath]=monsterTileset;
        }
        else
        {
            std::cerr << "MapVisualiserPlayer::updatePlayerMonsterTile() image not found: " << imagePath << std::endl;
            monsterTileset=NULL;
        }
    }
    if(monsterTileset!=NULL)
    {
        std::cerr << "MapVisualiserPlayer::updatePlayerMonsterTile() monsterMapObject created" << std::endl;
        monsterMapObject = new Tiled::MapObject();
        monsterMapObject->setName("Current player monster");

        Tiled::Cell cell=monsterMapObject->cell();
        switch(direction)
        {
            case CatchChallenger::Direction_look_at_top:
            case CatchChallenger::Direction_move_at_top:
                cell.setTile(monsterTileset->tileAt(2));
            break;
            case CatchChallenger::Direction_look_at_right:
            case CatchChallenger::Direction_move_at_right:
                cell.setTile(monsterTileset->tileAt(7));
            break;
            case CatchChallenger::Direction_look_at_bottom:
            case CatchChallenger::Direction_move_at_bottom:
                cell.setTile(monsterTileset->tileAt(6));
            break;
            case CatchChallenger::Direction_look_at_left:
            case CatchChallenger::Direction_move_at_left:
                cell.setTile(monsterTileset->tileAt(3));
            break;
            default:
            break;
        }
        monsterMapObject->setCell(cell);
    }
    if(resetMonster)
        loadMonsterFromCurrentMap();
    resetMonsterTile();
}

void MapVisualiserPlayer::setClip(const bool &clip)
{
    this->clip=clip;
}

const Tiled::MapObject * MapVisualiserPlayer::getPlayerMapObject() const
{
    return playerMapObject;
}

std::pair<uint8_t,uint8_t> MapVisualiserPlayer::getPos() const
{
    return std::pair<uint8_t,uint8_t>(x,y);
}

bool MapVisualiserPlayer::isInMove() const
{
    return inMove;
}

CatchChallenger::Direction MapVisualiserPlayer::getDirection() const
{
    return direction;
}

void MapVisualiserPlayer::stopMove()
{
    inMove=false;
}

bool MapVisualiserPlayer::teleportTo(const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const CatchChallenger::Direction &direction)
{
    std::cout << "MapVisualiserPlayer::teleportTo()" << std::endl;
    if(mapId>=(CATCHCHALLENGER_TYPE_MAPID)QtDatapackClientLoader::datapackLoader->get_maps().size())
    {
        emit error("mapId greater than QtDatapackClientLoader::datapackLoader->maps.size(): "+
                   std::to_string(QtDatapackClientLoader::datapackLoader->get_maps().size()));
        return false;
    }

    current_map=mapId;
    this->x=x;
    this->y=y;

    if(monsterMapObject!=nullptr)
        monsterMapObject->setVisible(false);
    pendingMonsterMoves.clear();
    current_monster_map=current_map;
    monster_x=x;
    monster_y=y;

    //the direction
    this->direction=direction;
    switch(direction)
    {
        case CatchChallenger::Direction_look_at_top:
        case CatchChallenger::Direction_move_at_top:
        {
            Tiled::Cell cell=playerMapObject->cell();
            cell.setTile(playerTileset->tileAt(1));
            playerMapObject->setCell(cell);
        }
        break;
        case CatchChallenger::Direction_look_at_right:
        case CatchChallenger::Direction_move_at_right:
        {
            Tiled::Cell cell=playerMapObject->cell();
            cell.setTile(playerTileset->tileAt(4));
            playerMapObject->setCell(cell);
        }
        break;
        case CatchChallenger::Direction_look_at_bottom:
        case CatchChallenger::Direction_move_at_bottom:
        {
            Tiled::Cell cell=playerMapObject->cell();
            cell.setTile(playerTileset->tileAt(7));
            playerMapObject->setCell(cell);
        }
        break;
        case CatchChallenger::Direction_look_at_left:
        case CatchChallenger::Direction_move_at_left:
        {
            Tiled::Cell cell=playerMapObject->cell();
            cell.setTile(playerTileset->tileAt(10));
            playerMapObject->setCell(cell);
        }
        break;
        default:
        QMessageBox::critical(NULL,tr("Internal error")+", file: "+QString(__FILE__)+":"+QString::number(__LINE__),tr("The direction send by the server is wrong"));
        return false;
    }

    //position
    this->x=static_cast<uint8_t>(x);
    this->y=static_cast<uint8_t>(y);

    return true;
}

bool MapVisualiserPlayer::nextPathStepInternal(std::vector<PathResolved> &pathList,const CatchChallenger::Direction &direction)//true if have step
{
    std::cout << "MapVisualiserPlayer::nextPathStepInternal()" << std::endl;
    const std::pair<uint8_t,uint8_t> pos(getPos());
    const uint8_t &x=pos.first;
    const uint8_t &y=pos.second;
    {
        if(!inMove)
        {
            std::cerr << "inMove=false into MapControllerMP::nextPathStep(), fixed" << std::endl;
            inMove=true;
        }
        if(canGoTo(direction,current_map,x,y,true))
        {
            this->direction=direction;
            moveStep=0;
            moveStepSlot();
            const CatchChallenger::CommonMap &logicalMap=QtDatapackClientLoader::datapackLoader->getMap(current_map);
            if(CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange)
            {
                if(direction==CatchChallenger::Direction_move_at_bottom)
                {
                    if(y==(logicalMap.height-1))
                        emit send_player_direction(CatchChallenger::Direction_look_at_bottom);
                }
                else if(direction==CatchChallenger::Direction_move_at_top)
                {
                    if(y==0)
                        emit send_player_direction(CatchChallenger::Direction_look_at_top);
                }
                else if(direction==CatchChallenger::Direction_move_at_right)
                {
                    if(x==(logicalMap.width-1))
                        emit send_player_direction(CatchChallenger::Direction_look_at_right);
                }
                else if(direction==CatchChallenger::Direction_move_at_left)
                {
                    if(x==0)
                        emit send_player_direction(CatchChallenger::Direction_look_at_left);
                }
            }
            emit send_player_direction(direction);
            return true;
        }
        else
        {
            if(direction==CatchChallenger::Direction_move_at_bottom)
                this->direction=CatchChallenger::Direction_look_at_bottom;
            else if(direction==CatchChallenger::Direction_move_at_top)
                this->direction=CatchChallenger::Direction_look_at_top;
            else if(direction==CatchChallenger::Direction_move_at_right)
                this->direction=CatchChallenger::Direction_look_at_right;
            else if(direction==CatchChallenger::Direction_move_at_left)
                this->direction=CatchChallenger::Direction_look_at_left;

            moveStep=0;
            uint8_t baseTile;
            switch(this->direction)
            {
                case CatchChallenger::Direction_look_at_left:
                baseTile=10;
                break;
                case CatchChallenger::Direction_look_at_right:
                baseTile=4;
                break;
                case CatchChallenger::Direction_look_at_top:
                baseTile=1;
                break;
                case CatchChallenger::Direction_look_at_bottom:
                baseTile=7;
                break;
                default:
                qDebug() << QStringLiteral("moveStepSlot(): nextPathStep: %1, wrong direction").arg(moveStep);
                pathList.clear();
                return false;
            }
            Tiled::Cell cell=playerMapObject->cell();
            cell.setTile(playerTileset->tileAt(baseTile+0));
            playerMapObject->setCell(cell);

            std::cerr << "Error at path found, collision detected" << std::endl;
            pathList.clear();
            return false;
        }
    }
    return false;
}

void MapVisualiserPlayer::pathFindingResultInternal(std::vector<PathResolved> &pathList, const CATCHCHALLENGER_TYPE_MAPID &current_map, const uint8_t &x, const uint8_t &y,
                                                    const std::vector<std::pair<CatchChallenger::Orientation,uint8_t> > &path)
{
    if(keyAccepted.empty() || keyAccepted.find(Qt::Key_Return)!=keyAccepted.cend())
    {
        //take care of the returned data
        PathResolved pathResolved;
        pathResolved.startPoint.map=current_map;
        pathResolved.startPoint.x=x;
        pathResolved.startPoint.y=y;
        pathResolved.path=path;

        if(!inMove)
        {
            inMove=true;
            if(this->current_map==current_map && this->x==x && this->y==y)
            {
                std::cout << "this->current_map==current_map && this->x==x && this->y==y" << std::endl;
                if(pathList.size()>1)
                    pathList.pop_back();
                pathList.push_back(pathResolved);
                if(!nextPathStep())
                {
                    std::cout << "this->current_map==current_map && this->x==x && this->y==y stopAndSend" << std::endl;
                    stopAndSend();
                    parseStop();
                }
                if(keyPressed.empty())
                {
                    std::cout << "this->current_map==current_map && this->x==x && this->y==y !keyPressed" << std::endl;
                    parseAction();
                }
            }
            else
                std::cerr << "Wrong start point to start the path finding" << std::endl;
        }
        wasPathFindingUsed=true;
        return;
    }
}

// for /tools/map-visualiser/
void MapVisualiserPlayer::forcePlayerTileset(QString path)
{
    std::cout << "MapVisualiserPlayer::forcePlayerTileset(): " << path.toStdString() << std::endl;
    QString externalFile=QCoreApplication::applicationDirPath()+"/"+path;
    if(QFile::exists(externalFile))
    {
        QImage externalImage(externalFile);
        if(!externalImage.isNull() && externalImage.width()==48 && externalImage.height()==96)
            playerTileset->loadFromImage(externalImage,externalFile);
        else
            playerTileset->loadFromImage(QImage(":/"+path),":/"+path);
    }
    else
        playerTileset->loadFromImage(QImage(":/"+path),":/"+path);

    //the direction
    direction=CatchChallenger::Direction_look_at_bottom;
    Tiled::Cell cell=playerMapObject->cell();
    cell.setTile(playerTileset->tileAt(7));
    playerMapObject->setCell(cell);
}
