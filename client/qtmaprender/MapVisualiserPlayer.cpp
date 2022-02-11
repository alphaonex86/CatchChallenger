#include "MapVisualiserPlayer.hpp"

#include "../../../general/base/MoveOnTheMap.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include "../../../general/base/CommonDatapackServerSpec.hpp"
#include "../../../general/base/CommonSettingsCommon.hpp"
#include "../../../general/base/CommonSettingsServer.hpp"
#include "../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../general/base/GeneralVariable.hpp"

#include <qmath.h>
#include <QFileInfo>
#include <QMessageBox>
#include <QDebug>
#include <iostream>
#include <QCoreApplication>

/* why send the look at because blocked into the wall?
to be sync if connexion is stop, but use more bandwith
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
    events=NULL;
    items=NULL;
    quests=NULL;
    itemOnMap=NULL;
    plantOnMap=NULL;
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

    currentPlayerSpeed=250;
    moveTimer.setInterval(currentPlayerSpeed/5);
    moveTimer.setSingleShot(true);
    if(!connect(&moveTimer,&QTimer::timeout,this,&MapVisualiserPlayer::moveStepSlot))
        abort();
    moveAnimationTimer.setInterval(currentPlayerSpeed/5);
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
    animationTileset=new Tiled::Tileset(QStringLiteral("animation"),16,16);
    nextCurrentObject=new Tiled::MapObject();
    grassCurrentObject=new Tiled::MapObject();
    grassCurrentObject->setName("grassCurrentObject");
    haveGrassCurrentObject=false;
    haveNextCurrentObject=false;

    defaultTileset="trainer";
    playerMapObject = new Tiled::MapObject();
    grassCurrentObject->setName("playerMapObject");

    lastTileset=defaultTileset;
    playerTileset = new Tiled::Tileset(QStringLiteral("player"),16,24);
    playerTilesetCache[lastTileset]=playerTileset;

    lastAction.restart();
}

MapVisualiserPlayer::~MapVisualiserPlayer()
{
    if(animationTileset!=NULL)
        delete animationTileset;
    if(nextCurrentObject!=NULL)
        delete nextCurrentObject;
    if(grassCurrentObject!=NULL)
        delete grassCurrentObject;
    if(playerMapObject!=NULL)
        delete playerMapObject;
    if(monsterMapObject!=nullptr)
    {
        //delete monsterMapObject;-> do a clean fix
        monsterMapObject=nullptr;
    }
    //delete playerTileset;
    std::unordered_set<Tiled::Tileset *> deletedTileset;
    for(auto i : playerTilesetCache) {
            Tiled::Tileset * cur = i.second;
            if(deletedTileset.find(cur)==deletedTileset.cend())
            {
                deletedTileset.insert(cur);
                delete cur;
            }
        }
    for(auto i : monsterTilesetCache) {
            Tiled::Tileset * cur = i.second;
            if(deletedTileset.find(cur)==deletedTileset.cend())
            {
                deletedTileset.insert(cur);
                delete cur;
            }
        }
}

bool MapVisualiserPlayer::haveMapInMemory(const std::string &mapPath)
{
    return all_map.find(mapPath)!=all_map.cend() || old_all_map.find(mapPath)!=old_all_map.cend();
}

void MapVisualiserPlayer::keyPressEvent(QKeyEvent * event)
{
    if(current_map.empty() || all_map.find(current_map)==all_map.cend())
        return;

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
            if(!canGoTo(CatchChallenger::Direction_move_at_left,all_map.at(current_map)->logicalMap,x,y,true))
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
    else if(keyPressed.find(Qt::Key_Right)!=keyPressed.cend())
    {
        //already turned on this direction, then try move into this direction
        if(direction==CatchChallenger::Direction_look_at_right)
        {
            if(!canGoTo(CatchChallenger::Direction_move_at_right,all_map.at(current_map)->logicalMap,x,y,true))
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
    else if(keyPressed.find(Qt::Key_Up)!=keyPressed.cend())
    {
        //already turned on this direction, then try move into this direction
        if(direction==CatchChallenger::Direction_look_at_top)
        {
            if(!canGoTo(CatchChallenger::Direction_move_at_top,all_map.at(current_map)->logicalMap,x,y,true))
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
    else if(keyPressed.find(Qt::Key_Down)!=keyPressed.cend())
    {
        //already turned on this direction, then try move into this direction
        if(direction==CatchChallenger::Direction_look_at_bottom)
        {
            if(!canGoTo(CatchChallenger::Direction_move_at_bottom,all_map.at(current_map)->logicalMap,x,y,true))
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
    Map_full * map_full=all_map.at(current_map);
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
        if(map_full->triggerAnimations.find(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)))!=map_full->triggerAnimations.cend())
        {
            TriggerAnimation* triggerAnimation=map_full->triggerAnimations.at(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)));
            triggerAnimation->startEnter();
        }
        //door
        if(map_full->doors.find(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)))!=map_full->doors.cend())
        {
            MapDoor* door=map_full->doors.at(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)));
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
            //moveTimer.stop();
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
                    cell.tile=monsterTileset->tileAt(baseTile+0);
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
                    cell.tile=monsterTileset->tileAt(baseTile-2);
                    monsterMapObject->setCell(cell);
                }
                break;
                //stopped step
                case 4:
                {
                    Tiled::Cell cell=monsterMapObject->cell();
                    cell.tile=monsterTileset->tileAt(baseTile+0);
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
            cell.tile=playerTileset->tileAt(baseTile+0);
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
        centerOn(MapObjectItem::objectLink.at(playerMapObject));
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

                CatchChallenger::CommonMap * map=&all_map.at(current_monster_map)->logicalMap;
                const CatchChallenger::CommonMap * old_map=map;
                //set the final value (direction, position, ...)
                switch(direction)
                {
                    case CatchChallenger::Direction_move_at_left:
                    case CatchChallenger::Direction_move_at_right:
                    case CatchChallenger::Direction_move_at_top:
                    case CatchChallenger::Direction_move_at_bottom:
                        if(!CatchChallenger::MoveOnTheMap::move(direction,&map,&monster_x,&monster_y))
                        {
                            std::cerr << "Bug at move for pendingMonsterMoves, unknown move: " << std::to_string(direction)
                                      << " from " << map->map_file << " (" << std::to_string(monster_x) << "," << std::to_string(monster_y) << ")"
                                      << std::endl;
                            resetMonsterTile();
                        }
                    break;
                    default:
                        qDebug() << QStringLiteral("moveStepSlot(): moveStep: %1, wrong direction (%2) when moveStep>2").arg(moveStep).arg(direction);
                    return;
                }
                //if the map have changed
                if(old_map!=map)
                {
                    unloadMonsterFromCurrentMap();
                    current_monster_map=map->map_file;
                    if(old_all_map.find(current_monster_map)==old_all_map.cend())
                        std::cerr << "old_all_map.find(current_map)==old_all_map.cend() in monster follow" << std::endl;
                    if(!vectorcontainsAtLeastOne(static_cast<const CatchChallenger::Map_client *>(old_map)->near_map,map))
                        resetMonsterTile();
                    loadMonsterFromCurrentMap();
                }

                monsterMapObject->setPosition(QPointF(monster_x-0.5,monster_y+1));
                MapObjectItem::objectLink.at(monsterMapObject)->setZValue(monster_y);
            }
        animationDisplayed=false;
        CatchChallenger::CommonMap * map=&all_map.at(current_map)->logicalMap;
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
                    std::cerr << "Bug at move, unknown move: " << std::to_string(direction)
                              << " from " << map->map_file << " (" << std::to_string(x) << "," << std::to_string(y) << ")"
                              << std::endl;
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
            if(old_all_map.find(current_map)==old_all_map.cend())
                emit inWaitingOfMap();
            loadOtherMap(current_map);
            hideNotloadedMap();
            if(!vectorcontainsAtLeastOne(static_cast<const CatchChallenger::Map_client *>(old_map)->near_map,map))
                resetMonsterTile();
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

bool MapVisualiserPlayer::asyncMapLoaded(const std::string &fileName,Map_full * tempMapObject)
{
    if(current_map.empty())
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
                    if(objectGroup->name()=="Object")
                    {
                        QList<Tiled::MapObject*> objects=objectGroup->objects();
                        int index2=0;
                        while(index2<objects.size())
                        {
                            Tiled::MapObject* object=objects.at(index2);
                            const uint32_t &x=object->x();
                            const uint32_t &y=object->y()-1;

                            if(object->type()=="object")
                            {
                                //found into the logical map
                                if(tempMapObject->logicalMap.itemsOnMap.find(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)))!=
                                        tempMapObject->logicalMap.itemsOnMap.cend())
                                {
                                    if(object->property("visible")=="false")
                                    {
                                        //The tiled object not exist on this layer
                                        if(ObjectGroupItem::objectGroupLink.find(objectGroup)!=ObjectGroupItem::objectGroupLink.cend())
                                        {
                                            ObjectGroupItem::objectGroupLink.at(objectGroup)->removeObject(object);
                                            tempMapObject->logicalMap.itemsOnMap[std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))].tileObject=NULL;
                                        }
                                        else
                                            std::cerr << "Try removeObject(object) on not existant ObjectGroupItem::objectGroupLink.at(objectGroup)" << std::endl;
                                        objects.removeAt(index2);
                                    }
                                    else
                                    {
                                        const std::string tempMap=tempMapObject->logicalMap.map_file;
                                        if(QtDatapackClientLoader::datapackLoader->get_itemOnMap().find(tempMap)!=
                                                QtDatapackClientLoader::datapackLoader->get_itemOnMap().cend())
                                        {
                                            const std::unordered_map<std::pair<uint8_t,uint8_t>,uint16_t,pairhash> &tempIndexItem=
                                                    QtDatapackClientLoader::datapackLoader->get_itemOnMap().at(tempMap);
                                            if(tempIndexItem.find(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)))!=
                                                    tempIndexItem.cend())
                                            {
                                                const uint16_t &itemIndex=tempIndexItem.at(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)));
                                                if(itemOnMap->find(itemIndex)!=itemOnMap->cend())
                                                {
                                                    if(ObjectGroupItem::objectGroupLink.find(objectGroup)!=ObjectGroupItem::objectGroupLink.cend())
                                                    {
                                                        ObjectGroupItem * objectGroupItem=ObjectGroupItem::objectGroupLink.at(objectGroup);
                                                        objectGroupItem->removeObject(object);
                                                        objects.removeAt(index2);
                                                    }
                                                    else
                                                    {
                                                        std::cerr << "ObjectGroupItem::objectGroupLink.find(objectGroup)!=ObjectGroupItem::objectGroupLink.cend(), map mixed?" << std::endl;
                                                        index2++;
                                                    }
                                                }
                                                else
                                                {
                                                    tempMapObject->logicalMap.itemsOnMap[std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))].tileObject=object;
                                                    index2++;
                                                }
                                            }
                                            else
                                            {
                                                tempMapObject->logicalMap.itemsOnMap[std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))].tileObject=object;
                                                index2++;
                                            }
                                        }
                                        else
                                        {
                                            tempMapObject->logicalMap.itemsOnMap[std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))].tileObject=object;
                                            index2++;
                                        }
                                    }
                                }
                                else
                                {
                                    //The tiled object not exist on this layer
                                    if(ObjectGroupItem::objectGroupLink.find(objectGroup)!=ObjectGroupItem::objectGroupLink.cend())
                                    {
                                        ObjectGroupItem::objectGroupLink.at(objectGroup)->removeObject(object);
                                        tempMapObject->logicalMap.itemsOnMap[std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))].tileObject=NULL;
                                    }
                                    else
                                        std::cerr << "Try removeObject(object) on not existant ObjectGroupItem::objectGroupLink.at(objectGroup)" << std::endl;
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

void MapVisualiserPlayer::setInformations(std::unordered_map<uint16_t, uint32_t> *items, std::unordered_map<CATCHCHALLENGER_TYPE_QUEST, CatchChallenger::PlayerQuest> *quests, std::vector<uint8_t> *events, std::unordered_set<uint16_t> *itemOnMap, std::unordered_map<uint16_t, CatchChallenger::PlayerPlant> *plantOnMap)
{
    this->events=events;
    this->items=items;
    this->quests=quests;
    this->itemOnMap=itemOnMap;
    this->plantOnMap=plantOnMap;
    if(plantOnMap->size()>65535)
        abort();
    if(items->size()>65535)
        abort();
    if(quests->size()>65535)
        abort();
    if(itemOnMap->size()>65535)
        abort();
}

void MapVisualiserPlayer::unblock()
{
    blocked=false;
}

bool MapVisualiserPlayer::finalPlayerStepTeleported(bool &isTeleported)
{
    if(all_map.find(current_map)==all_map.cend())
    {
        qDebug() << "current map not loaded, unable to do finalPlayerStep()";
        return false;
    }
    const Map_full * current_map_pointer=all_map.at(current_map);
    if(current_map_pointer==NULL)
    {
        qDebug() << "current map not loaded null pointer, unable to do finalPlayerStep()";
        return false;
    }
    if(!isTeleported)
    {
        int index=0;
        const int size=current_map_pointer->logicalMap.teleport_semi.size();
        while(index<size)
        {
            const CatchChallenger::Map_semi_teleport &current_teleport=current_map_pointer->logicalMap.teleport_semi.at(index);
            //if need be teleported
            if(current_teleport.source_x==x && current_teleport.source_y==y)
            {
                isTeleported=true;
                unloadPlayerFromCurrentMap();
                passMapIntoOld();
                //player coord
                current_map=current_teleport.map;
                x=current_teleport.destination_x;
                y=current_teleport.destination_y;
                //monster coord
                current_monster_map=current_teleport.map;
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
    if(all_map.find(current_map)==all_map.cend())
    {
        qDebug() << "current map not loaded, unable to do finalPlayerStep()";
        return;
    }
    const Map_full * current_map_pointer=all_map.at(current_map);
    if(current_map_pointer==NULL)
    {
        qDebug() << "current map not loaded null pointer, unable to do finalPlayerStep()";
        return;
    }

    /// \see into haveStopTileAction(), to NPC fight: std::vector<std::pair<uint8_t,uint8_t> > botFightRemotePointList=all_map.value(current_map)->logicalMap.botsFightTriggerExtra.values(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)));
    if(!CatchChallenger::CommonDatapack::commonDatapack.get_monstersCollision().empty())
    {
        //locate the right layer for monster
        if(monsterMapObject!=NULL)
        {
            const Map_full * current_monster_map_pointer=all_map.at(current_monster_map);
            if(current_monster_map_pointer==NULL)
            {
                qDebug() << "current_monster_map_pointer not loaded null pointer, unable to do finalPlayerStep()";
                return;
            }
            {
                const CatchChallenger::MonstersCollisionValue &monstersCollisionValue=
                        CatchChallenger::MoveOnTheMap::getZoneCollision(current_monster_map_pointer->logicalMap,monster_x,monster_y);
                monsterMapObject->setVisible(false);
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
                        if(monstersCollision.item==0 || items->find(monstersCollision.item)!=items->cend())
                        {
                            monsterMapObject->setVisible((monstersCollisionTemp.tile.empty() && pendingMonsterMoves.size()>=1) ||
                                                         (pendingMonsterMoves.size()==1 && !inMove)
                                                         );
                        }
                    }
                    index++;
                }
            }
        }
        //locate the right layer
        const CatchChallenger::MonstersCollisionValue &monstersCollisionValue=
                CatchChallenger::MoveOnTheMap::getZoneCollision(current_map_pointer->logicalMap,x,y);
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
                if(monstersCollision.item==0 || items->find(monstersCollision.item)!=items->cend())
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
                                    playerTileset = new Tiled::Tileset(QString::fromStdString(lastTileset),16,24);
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
                        {
                            Tiled::Cell cell=playerMapObject->cell();
                            int tileId=cell.tile->id();
                            cell.tile=playerTileset->tileAt(tileId);
                            playerMapObject->setCell(cell);
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
                int tileId=cell.tile->id();
                cell.tile=playerTileset->tileAt(tileId);
                playerMapObject->setCell(cell);
            }
        }
    }
    //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
    playerMapObject->setPosition(QPoint(x,y+1));
    MapObjectItem::objectLink.at(playerMapObject)->setZValue(y);
    if(centerOnPlayer)
        centerOn(MapObjectItem::objectLink.at(playerMapObject));
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

    if(!parseKey)
        return;
    //check if one arrow key is pressed to continue to move into this direction
    if(keyPressed.find(Qt::Key_Left)!=keyPressed.cend())
    {
        //can't go into this direction, then just look into this direction
        if(!canGoTo(CatchChallenger::Direction_move_at_left,current_map_pointer->logicalMap,x,y,true))
        {
            keyPressed.erase(Qt::Key_Left);
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
    else if(keyPressed.find(Qt::Key_Right)!=keyPressed.cend())
    {
        //can't go into this direction, then just look into this direction
        if(!canGoTo(CatchChallenger::Direction_move_at_right,current_map_pointer->logicalMap,x,y,true))
        {
            keyPressed.erase(Qt::Key_Right);
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
    else if(keyPressed.find(Qt::Key_Up)!=keyPressed.cend())
    {
        //can't go into this direction, then just look into this direction
        if(!canGoTo(CatchChallenger::Direction_move_at_top,current_map_pointer->logicalMap,x,y,true))
        {
            keyPressed.erase(Qt::Key_Up);
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
    else if(keyPressed.find(Qt::Key_Down)!=keyPressed.cend())
    {
        //can't go into this direction, then just look into this direction
        if(!canGoTo(CatchChallenger::Direction_move_at_bottom,current_map_pointer->logicalMap,x,y,true))
        {
            keyPressed.erase(Qt::Key_Down);
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
    CatchChallenger::CommonMap * map=&all_map.at(current_map)->logicalMap;
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
    CatchChallenger::CommonMap * map=&all_map.at(current_map)->logicalMap;
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
                if(map_client->botsDisplay.find(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)))!=
                        map_client->botsDisplay.cend())
                {
                    CatchChallenger::BotDisplay *botDisplay=&map_client->botsDisplay[
                            std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))
                            ];
                    Tiled::Cell cell=botDisplay->mapObject->cell();
                    cell.tile=botDisplay->tileset->tileAt(4);
                    botDisplay->mapObject->setCell(cell);
                }
                else if(map_client->itemsOnMap.find(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)))!=
                        map_client->itemsOnMap.cend())
                {
                    const CatchChallenger::Map_client::ItemOnMapForClient &item=map_client->itemsOnMap.at(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)));
                    if(item.tileObject!=NULL && !item.infinite)
                    {
                        ObjectGroupItem::objectGroupLink[item.tileObject->objectGroup()]->removeObject(item.tileObject);
                        map_client->itemsOnMap[std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))].tileObject=NULL;
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
                if(map_client->botsDisplay.find(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)))!=
                        map_client->botsDisplay.cend())
                {
                    CatchChallenger::BotDisplay *botDisplay=&map_client->botsDisplay[std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))];
                    Tiled::Cell cell=botDisplay->mapObject->cell();
                    cell.tile=botDisplay->tileset->tileAt(10);
                    botDisplay->mapObject->setCell(cell);
                }
                else if(map_client->itemsOnMap.find(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)))!=
                        map_client->itemsOnMap.cend())
                {
                    const CatchChallenger::Map_client::ItemOnMapForClient &item=map_client->itemsOnMap.at(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)));
                    if(item.tileObject!=NULL && !item.infinite)
                    {
                        ObjectGroupItem::objectGroupLink[item.tileObject->objectGroup()]->removeObject(item.tileObject);
                        map_client->itemsOnMap[std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))].tileObject=NULL;
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
                if(map_client->botsDisplay.find(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)))!=
                        map_client->botsDisplay.cend())
                {
                    CatchChallenger::BotDisplay *botDisplay=&map_client->botsDisplay[
                            std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))];
                    Tiled::Cell cell=botDisplay->mapObject->cell();
                    cell.tile=botDisplay->tileset->tileAt(7);
                    botDisplay->mapObject->setCell(cell);
                }
                else if(map_client->itemsOnMap.find(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)))!=
                        map_client->itemsOnMap.cend())
                {
                    const CatchChallenger::Map_client::ItemOnMapForClient &item=map_client->itemsOnMap.at(
                                std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)));
                    if(item.tileObject!=NULL && !item.infinite)
                    {
                        ObjectGroupItem::objectGroupLink[item.tileObject->objectGroup()]->removeObject(item.tileObject);
                        map_client->itemsOnMap[std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))].tileObject=NULL;
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
                if(map_client->botsDisplay.find(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)))!=
                        map_client->botsDisplay.cend())
                {
                    CatchChallenger::BotDisplay *botDisplay=&map_client->botsDisplay[std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))];
                    Tiled::Cell cell=botDisplay->mapObject->cell();
                    cell.tile=botDisplay->tileset->tileAt(1);
                    botDisplay->mapObject->setCell(cell);
                }
                else if(map_client->itemsOnMap.find(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)))!=
                        map_client->itemsOnMap.cend())
                {
                    const CatchChallenger::Map_client::ItemOnMapForClient &item=map_client->itemsOnMap.at(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)));
                    if(item.tileObject!=NULL && !item.infinite)
                    {
                        ObjectGroupItem::objectGroupLink[item.tileObject->objectGroup()]->removeObject(item.tileObject);
                        map_client->itemsOnMap[std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))].tileObject=NULL;
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
        if(keyPressed.find(Qt::Key_Left)!=keyPressed.cend() &&
                canGoTo(CatchChallenger::Direction_move_at_left,all_map.at(current_map)->logicalMap,x,y,true))
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
        if(keyPressed.find(Qt::Key_Right)!=keyPressed.cend() &&
                canGoTo(CatchChallenger::Direction_move_at_right,all_map.at(current_map)->logicalMap,x,y,true))
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
        if(keyPressed.find(Qt::Key_Up)!=keyPressed.cend() &&
                canGoTo(CatchChallenger::Direction_move_at_top,all_map.at(current_map)->logicalMap,x,y,true))
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
        if(keyPressed.find(Qt::Key_Down)!=keyPressed.cend() &&
                canGoTo(CatchChallenger::Direction_move_at_bottom,all_map.at(current_map)->logicalMap,x,y,true))
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
    if(current_map.empty() || all_map.find(current_map)==all_map.cend())
        return;

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

std::string MapVisualiserPlayer::lastLocation() const
{
    return mLastLocation;
}

std::string MapVisualiserPlayer::currentMap() const
{
    return current_map;
}

Map_full * MapVisualiserPlayer::currentMapFull() const
{
    return all_map.at(current_map);
}

bool MapVisualiserPlayer::currentMapIsLoaded() const
{
    if(all_map.find(current_map)==all_map.cend())
        return false;
    return true;
}

std::string MapVisualiserPlayer::currentMapType() const
{
    if(all_map.find(current_map)==all_map.cend())
        return std::string();
    const Map_full * const mapFull=all_map.at(current_map);
    const Tiled::Map * const tiledMap=mapFull->tiledMap;
    const Tiled::Properties &properties=tiledMap->properties();
    if(properties.find("type")!=properties.cend())
        if(!properties.value("type").isEmpty())
            return properties.value("type").toStdString();
    if(mapFull->logicalMap.xmlRoot==NULL)
        return std::string();
    if(mapFull->logicalMap.xmlRoot->Attribute("type")!=NULL)
        //if(!std::string(all_map.value(current_map)->logicalMap.xmlRoot->Attribute("type")->empty()) if empty return empty or empty?
            return mapFull->logicalMap.xmlRoot->Attribute("type");
    return std::string();
}

std::string MapVisualiserPlayer::currentZone() const
{
    const Map_full * const mapFull=all_map.at(current_map);
    const Tiled::Map * const tiledMap=mapFull->tiledMap;
    const Tiled::Properties &properties=tiledMap->properties();
    if(properties.find("zone")!=properties.cend())
        if(!properties.value("zone").isEmpty())
            return properties.value("zone").toStdString();
    if(mapFull->logicalMap.xmlRoot==NULL)
        return std::string();
    if(mapFull->logicalMap.xmlRoot->Attribute("zone")!=NULL)
        //if(!std::string(all_map.value(current_map)->logicalMap.xmlRoot->Attribute("zone")->empty()) if empty return empty or empty?
            return mapFull->logicalMap.xmlRoot->Attribute("zone");
    return std::string();
}

std::string MapVisualiserPlayer::currentBackgroundsound() const
{
    const Map_full * const mapFull=all_map.at(current_map);
    const Tiled::Map * const tiledMap=mapFull->tiledMap;
    const Tiled::Properties &properties=tiledMap->properties();
    if(properties.find("backgroundsound")!=properties.cend())
        if(!properties.value("backgroundsound").isEmpty())
            return properties.value("backgroundsound").toStdString();
    if(mapFull->logicalMap.xmlRoot==NULL)
        return std::string();
    if(mapFull->logicalMap.xmlRoot->Attribute("backgroundsound")!=NULL)
        //if(!std::string(all_map.value(current_map)->logicalMap.xmlRoot->Attribute("backgroundsound")->empty()) if empty return empty or empty?
            return mapFull->logicalMap.xmlRoot->Attribute("backgroundsound");
    return std::string();
}

CatchChallenger::Direction MapVisualiserPlayer::getDirection()
{
    return direction;
}

bool MapVisualiserPlayer::loadPlayerMap(const std::string &fileName,const uint8_t &x,const uint8_t &y)
{
    this->x=x;
    this->y=y;
    this->monster_x=x;
    this->monster_y=y;
    QFileInfo fileInformations(QString::fromStdString(fileName));
    current_map=fileInformations.absoluteFilePath().toStdString();
    current_monster_map=fileInformations.absoluteFilePath().toStdString();
    return true;
}

bool MapVisualiserPlayer::insert_player_internal(const CatchChallenger::Player_public_informations &player,
       const uint32_t &mapId,const uint16_t &x,const uint16_t &y,const CatchChallenger::Direction &direction,
                                              const std::vector<std::string> &skinFolderList)
{
    if(!mHaveTheDatapack || !player_informations_is_set)
    {
        emit error("MapVisualiserPlayer::insert_player_final(): !mHaveTheDatapack || !player_informations_is_set");
        return false;
    }
    if(mapId>=(uint32_t)QtDatapackClientLoader::datapackLoader->get_maps().size())
    {
        /// \bug here pass after delete a party, create a new
        emit error("mapId greater than QtDatapackClientLoader::datapackLoader->maps.size(): "+
                   std::to_string(QtDatapackClientLoader::datapackLoader->get_maps().size()));
        return true;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    qDebug() << QStringLiteral("insert_player(%1->%2,%3,%4,%5,%6)").arg(player.pseudo).arg(player.simplifiedId).arg(QtDatapackClientLoader::datapackLoader->maps.value(mapId)).arg(x).arg(y).arg(CatchChallenger::MoveOnTheMap::directionToString(direction));
    #endif
    //current player
    if(player.simplifiedId==player_informations.public_informations.simplifiedId)
    {
        //ignore to improve the performance server because can reinsert all player of map using the overall client list
        if(!current_map.empty())
        {
            qDebug() << "Current player already loaded on the map";
            return true;
        }
        /// \todo do a player cache here
        //the player skin
        if(player.skinId<skinFolderList.size())
        {
            playerSkinPath=datapackPath+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(player.skinId);
            const std::string &imagePath=playerSkinPath+"/trainer.png";
            QImage image(QString::fromStdString(imagePath));
            if(!image.isNull())
            {
                if(!playerTileset->loadFromImage(image,QString::fromStdString(imagePath)))
                    abort();
            }
            else
                qDebug() << "Unable to load the player tilset: "+QString::fromStdString(imagePath);
        }
        else
            qDebug() << "The skin id: "+QString::number(player.skinId)+", into a list of: "+QString::number(skinFolderList.size())+" item(s) info MapControllerMP::insert_player()";

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
        switch(direction)
        {
            case CatchChallenger::Direction_look_at_top:
            case CatchChallenger::Direction_move_at_top:
            {
                Tiled::Cell cell=playerMapObject->cell();
                cell.tile=playerTileset->tileAt(1);
                playerMapObject->setCell(cell);
            }
            break;
            case CatchChallenger::Direction_look_at_right:
            case CatchChallenger::Direction_move_at_right:
            {
                Tiled::Cell cell=playerMapObject->cell();
                cell.tile=playerTileset->tileAt(4);
                playerMapObject->setCell(cell);
            }
            break;
            case CatchChallenger::Direction_look_at_bottom:
            case CatchChallenger::Direction_move_at_bottom:
            {
                Tiled::Cell cell=playerMapObject->cell();
                cell.tile=playerTileset->tileAt(7);
                playerMapObject->setCell(cell);
            }
            break;
            case CatchChallenger::Direction_look_at_left:
            case CatchChallenger::Direction_move_at_left:
            {
                Tiled::Cell cell=playerMapObject->cell();
                cell.tile=playerTileset->tileAt(10);
                playerMapObject->setCell(cell);
            }
            break;
            default:
            QMessageBox::critical(NULL,tr("Internal error")+", file: "+QString(__FILE__)+":"+QString::number(__LINE__),tr("The direction send by the server is wrong"));
            return true;
        }

        //monster
        updatePlayerMonsterTile(player.monsterId);

        current_map=QtDatapackClientLoader::datapackLoader->get_maps().at(mapId);
        loadPlayerMap(datapackMapPathSpec+QtDatapackClientLoader::datapackLoader->get_maps().at(mapId),
                      static_cast<uint8_t>(x),static_cast<uint8_t>(y));
        setSpeed(player.speed);
    }
    return true;
}

void MapVisualiserPlayer::resetAll()
{
    if(!playerTileset->loadFromImage(QImage(QStringLiteral(":/CC/images/player_default/trainer.png")),QStringLiteral(":/CC/images/player_default/trainer.png")))
        qDebug() << "Unable the load the default player tileset";
    current_monster_map.clear();
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

    //delete playerTileset;
    {
        std::unordered_set<Tiled::Tileset *> deletedTileset;
        for(auto iter = playerTilesetCache.begin(); iter != playerTilesetCache.end(); ++iter){
                if(deletedTileset.find(iter->second)==deletedTileset.cend())
                {
                    deletedTileset.insert(iter->second);
                    delete iter->second;
                }
            }
        playerTilesetCache.clear();
    }
    lastTileset=defaultTileset;
    playerTileset = new Tiled::Tileset(QStringLiteral("player"),16,24);
    playerTilesetCache[lastTileset]=playerTileset;
    playerMapObject = new Tiled::MapObject();
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
        if(!CatchChallenger::MoveOnTheMap::canGoTo(direction,*mapPointer,x,y,checkCollision && !clip))
            return false;
        if(!CatchChallenger::MoveOnTheMap::move(direction,&mapPointer,&x,&y,checkCollision && !clip))
            return false;
        CatchChallenger::Map_client * map_client=static_cast<CatchChallenger::Map_client *>(&all_map.at(map.map_file)->logicalMap);
        if(map_client->itemsOnMap.find(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)))!=
                map_client->itemsOnMap.cend())
        {
            const CatchChallenger::Map_client::ItemOnMapForClient &item=map_client->itemsOnMap.at(
                        std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)));
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
    if(all_map.find(current_map)==all_map.cend())
    {
        qDebug() << QStringLiteral("all_map have not the current map: %1").arg(QString::fromStdString(current_map));
        return;
    }
    {
        Tiled::ObjectGroup *currentGroup=playerMapObject->objectGroup();
        if(currentGroup!=NULL)
        {
            if(ObjectGroupItem::objectGroupLink.find(currentGroup)!=ObjectGroupItem::objectGroupLink.cend())
                ObjectGroupItem::objectGroupLink.at(currentGroup)->removeObject(playerMapObject);
            //currentGroup->removeObject(playerMapObject);
            if(currentGroup!=all_map.at(current_map)->objectGroup)
                qDebug() << QStringLiteral("loadPlayerFromCurrentMap(), the playerMapObject group is wrong: %1").arg(currentGroup->name());
        }
        Tiled::ObjectGroup * objectGroup=all_map.at(current_map)->objectGroup;
        if(ObjectGroupItem::objectGroupLink.find(objectGroup)!=ObjectGroupItem::objectGroupLink.cend())
            ObjectGroupItem::objectGroupLink.at(objectGroup)->addObject(playerMapObject);
        else
            qDebug() << QStringLiteral("loadPlayerFromCurrentMap(), ObjectGroupItem::objectGroupLink not contains current_map->objectGroup");
        mLastLocation=all_map.at(current_map)->logicalMap.map_file;

        //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
        playerMapObject->setPosition(QPoint(x,y+1));
        MapObjectItem::objectLink.at(playerMapObject)->setZValue(y);
        if(centerOnPlayer)
            centerOn(MapObjectItem::objectLink.at(playerMapObject));
    }

    loadMonsterFromCurrentMap();
}

void MapVisualiserPlayer::loadMonsterFromCurrentMap()
{
    if(monsterMapObject==nullptr)
        return;
    //monster
    if(all_map.find(current_monster_map)==all_map.cend())
    {
        qDebug() << QStringLiteral("all_map have not the current map: %1").arg(QString::fromStdString(current_monster_map));
        return;
    }
    {
        Tiled::ObjectGroup *currentGroup=monsterMapObject->objectGroup();
        if(currentGroup!=NULL)
        {
            if(ObjectGroupItem::objectGroupLink.find(currentGroup)!=ObjectGroupItem::objectGroupLink.cend())
                ObjectGroupItem::objectGroupLink.at(currentGroup)->removeObject(monsterMapObject);
            //currentGroup->removeObject(monsterMapObject);
            if(currentGroup!=all_map.at(current_map)->objectGroup)
                qDebug() << QStringLiteral("loadPlayerFromCurrentMap(), the monsterMapObject group is wrong: %1").arg(currentGroup->name());
        }
        if(ObjectGroupItem::objectGroupLink.find(all_map.at(current_map)->objectGroup)!=ObjectGroupItem::objectGroupLink.cend())
            ObjectGroupItem::objectGroupLink.at(all_map.at(current_map)->objectGroup)->addObject(monsterMapObject);
        else
            qDebug() << QStringLiteral("loadPlayerFromCurrentMap(), ObjectGroupItem::objectGroupLink not contains current_map->objectGroup");
        //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
        monsterMapObject->setPosition(QPointF(monster_x-0.5,monster_y+1));
        MapObjectItem::objectLink.at(monsterMapObject)->setZValue(y);
    }
}

//call before leave the old map (and before loadPlayerFromCurrentMap())
void MapVisualiserPlayer::unloadPlayerFromCurrentMap()
{
    if(monsterMapObject==nullptr)
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

void MapVisualiserPlayer::mapDisplayedSlot(const std::string &fileName)
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
    if(all_map.find(current_map)!=all_map.cend())
        return &all_map.at(current_map)->logicalMap;
    else
        return NULL;
}

//the datapack
void MapVisualiserPlayer::setDatapackPath(const std::string &path,const std::string &mainDatapackCode)
{
    #ifdef DEBUG_CLIENT_LOAD_ORDER
    qDebug() << QStringLiteral("MapControllerMP::setDatapackPath()");
    #endif

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
    bool resetMonster=false;
    if(monsterMapObject!=NULL)
    {
        unloadMonsterFromCurrentMap();
        //delete monsterMapObject;
        monsterMapObject=NULL;
        resetMonster=true;
    }
    monsterTileset=NULL;
    player_informations.public_informations.monsterId=monster;
    const std::string &imagePath=datapackPath+DATAPACK_BASE_PATH_MONSTERS+std::to_string(monster)+"/overworld.png";
    if(monsterTilesetCache.find(imagePath)!=monsterTilesetCache.cend())
        monsterTileset=monsterTilesetCache.at(imagePath);
    else
    {
        QImage image(QString::fromStdString(imagePath));
        if(!image.isNull())
        {
            monsterTileset = new Tiled::Tileset(QString::fromStdString(lastTileset),32,32);
            if(!monsterTileset->loadFromImage(image,QString::fromStdString(imagePath)))
                abort();
            monsterTilesetCache[imagePath]=monsterTileset;
        }
        else
            monsterTileset=NULL;
    }
    if(monsterTileset!=NULL)
    {
        monsterMapObject = new Tiled::MapObject();
        monsterMapObject->setName("Current player monster");

        Tiled::Cell cell=monsterMapObject->cell();
        switch(direction)
        {
            case CatchChallenger::Direction_look_at_top:
            case CatchChallenger::Direction_move_at_top:
                cell.tile=monsterTileset->tileAt(2);
            break;
            case CatchChallenger::Direction_look_at_right:
            case CatchChallenger::Direction_move_at_right:
                cell.tile=monsterTileset->tileAt(7);
            break;
            case CatchChallenger::Direction_look_at_bottom:
            case CatchChallenger::Direction_move_at_bottom:
                cell.tile=monsterTileset->tileAt(6);
            break;
            case CatchChallenger::Direction_look_at_left:
            case CatchChallenger::Direction_move_at_left:
                cell.tile=monsterTileset->tileAt(3);
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

bool MapVisualiserPlayer::teleportTo(const uint32_t &mapId,const uint16_t &x,const uint16_t &y,const CatchChallenger::Direction &direction)
{
    if(mapId>=(uint32_t)QtDatapackClientLoader::datapackLoader->get_maps().size())
    {
        emit error("mapId greater than QtDatapackClientLoader::datapackLoader->maps.size(): "+
                   std::to_string(QtDatapackClientLoader::datapackLoader->get_maps().size()));
        return false;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    qDebug() << QStringLiteral("teleportTo(%1,%2,%3,%4)").arg(QtDatapackClientLoader::datapackLoader->maps.value(mapId)).arg(x).arg(y).arg(CatchChallenger::MoveOnTheMap::directionToString(direction));
    qDebug() << QStringLiteral("currently on: %1 (%2,%3)").arg(current_map).arg(this->x).arg(this->y);
    #endif

    current_map=QFileInfo(QString::fromStdString(
                    datapackMapPathSpec+QtDatapackClientLoader::datapackLoader->get_maps().at(mapId)))
            .absoluteFilePath().toStdString();
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
            cell.tile=playerTileset->tileAt(1);
            playerMapObject->setCell(cell);
        }
        break;
        case CatchChallenger::Direction_look_at_right:
        case CatchChallenger::Direction_move_at_right:
        {
            Tiled::Cell cell=playerMapObject->cell();
            cell.tile=playerTileset->tileAt(4);
            playerMapObject->setCell(cell);
        }
        break;
        case CatchChallenger::Direction_look_at_bottom:
        case CatchChallenger::Direction_move_at_bottom:
        {
            Tiled::Cell cell=playerMapObject->cell();
            cell.tile=playerTileset->tileAt(7);
            playerMapObject->setCell(cell);
        }
        break;
        case CatchChallenger::Direction_look_at_left:
        case CatchChallenger::Direction_move_at_left:
        {
            Tiled::Cell cell=playerMapObject->cell();
            cell.tile=playerTileset->tileAt(10);
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
    const std::pair<uint8_t,uint8_t> pos(getPos());
    const uint8_t &x=pos.first;
    const uint8_t &y=pos.second;
    //if(!pathList.empty()) -> wrong, to get direction, get from direction
    {
        if(!inMove)
        {
            std::cerr << "inMove=false into MapControllerMP::nextPathStep(), fixed" << std::endl;
            inMove=true;
        }
        if(canGoTo(direction,all_map.at(current_map)->logicalMap,x,y,true))
        {
            this->direction=direction;
            moveStep=0;
            moveStepSlot();
            if(CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange)
            {
                if(direction==CatchChallenger::Direction_move_at_bottom)
                {
                    if(y==(all_map.at(current_map)->logicalMap.height-1))
                        emit send_player_direction(CatchChallenger::Direction_look_at_bottom);
                }
                else if(direction==CatchChallenger::Direction_move_at_top)
                {
                    if(y==0)
                        emit send_player_direction(CatchChallenger::Direction_look_at_top);
                }
                else if(direction==CatchChallenger::Direction_move_at_right)
                {
                    if(x==(all_map.at(current_map)->logicalMap.width-1))
                        emit send_player_direction(CatchChallenger::Direction_look_at_right);
                }
                else if(direction==CatchChallenger::Direction_move_at_left)
                {
                    if(x==0)
                        emit send_player_direction(CatchChallenger::Direction_look_at_left);
                }
            }
            emit send_player_direction(direction);
            //startGrassAnimation(direction);
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
            //emit send_player_direction(this->direction);

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
            cell.tile=playerTileset->tileAt(baseTile+0);
            playerMapObject->setCell(cell);

            std::cerr << "Error at path found, collision detected" << std::endl;
            pathList.clear();
            return false;
        }
    }
    return false;
}

void MapVisualiserPlayer::pathFindingResultInternal(std::vector<PathResolved> &pathList,const std::string &current_map,const uint8_t &x,const uint8_t &y,
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
    cell.tile=playerTileset->tileAt(7);
    playerMapObject->setCell(cell);
}
