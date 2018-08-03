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
#include <iostream>

std::string MapVisualiserPlayer::text_slashtrainerpng="/trainer.png";
std::string MapVisualiserPlayer::text_slashtrainerMonsterpng="/following.png";
std::string MapVisualiserPlayer::text_slashMonsterpng="/overworld.png";
std::string MapVisualiserPlayer::text_slash="/";
std::string MapVisualiserPlayer::text_antislash="\\";
std::string MapVisualiserPlayer::text_dotpng=".png";
std::string MapVisualiserPlayer::text_type="type";
std::string MapVisualiserPlayer::text_zone="zone";
std::string MapVisualiserPlayer::text_backgroundsound="backgroundsound";

/* why send the look at because blocked into the wall?
to be sync if connexion is stop, but use more bandwith
To not send: store "is blocked but direction not send", cautch the close event, at close: if "is blocked but direction not send" then send it
*/

MapVisualiserPlayer::MapVisualiserPlayer(const bool &centerOnPlayer, const bool &debugTags, const bool &useCache) :
    MapVisualiser(debugTags,useCache)
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

    lastTileset = defaultTileset = "trainer";
    lastMonsterTileset = defaultMonsterTileset = "following";

    playerMapObject = new Tiled::MapObject();
    followingMonsterMapObject = new Tiled::MapObject();
    grassCurrentObject->setName("playerMapObject");

    playerTileset = new Tiled::Tileset(QStringLiteral("player"),16,24);
    followingMonsterTileset = new Tiled::Tileset(QStringLiteral("followingmonster"), 32, 32);

    playerTilesetCache[lastTileset] = playerTileset;
    playerTilesetCache[defaultMonsterTileset] = followingMonsterTileset;

    followingMonsterInformation.pseudo = "followingmonster";
    followingMonsterInformation.simplifiedId = 0;
    followingMonsterInformation.skinId = 1;
    followingMonsterInformation.type = CatchChallenger::Player_type::Player_type_normal;

    lastAction.restart();
}

MapVisualiserPlayer::~MapVisualiserPlayer()
{
    delete animationTileset;
    delete nextCurrentObject;
    delete grassCurrentObject;
    delete playerMapObject;
    delete followingMonsterMapObject;
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
    if(inMove || blocked)
        return;

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
            updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Left);
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
            updateFollowingMonsterPosition();
            updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Left);
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
            updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Right);
            moveStepSlot();
            emit send_player_direction(direction);
            //startGrassAnimation(direction);
        }
        //look in this direction
        else
        {
            Tiled::Cell cell = playerMapObject->cell();
            cell.tile = playerTileset->tileAt(4);
            playerMapObject->setCell(cell);
            direction = CatchChallenger::Direction_look_at_right;
            lookToMove.start();
            updateFollowingMonsterPosition();
            updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Right);
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
            updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Top);
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
            updateFollowingMonsterPosition();
            updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Top);
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
            updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Bottom);
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
            updateFollowingMonsterPosition();
            updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Bottom);
            emit send_player_direction(direction);
            parseStop();
        }
    }
}

void MapVisualiserPlayer::updateFollowingMonster(int tiledPos) {
    if (followingMonsterMapObject != nullptr) {
        Tiled::Cell cell = followingMonsterMapObject->cell();
        if (followingMonsterTileset != nullptr) {
            cell.tile = followingMonsterTileset->tileAt(tiledPos);
            followingMonsterMapObject->setCell(cell);
            monsterLastTileset = tiledPos;
        }
    }
}

void MapVisualiserPlayer::updateFollowingMonsterPosition() {
    //Update before the direction variable change
    if (keyPressed.find(Qt::Key_Left) != keyPressed.cend()) {
        direction = CatchChallenger::Direction_look_at_left;
    } else if (keyPressed.find(Qt::Key_Right) != keyPressed.cend()) {
        direction = CatchChallenger::Direction_look_at_right;
    } else if (keyPressed.find(Qt::Key_Up) != keyPressed.cend()) {
        direction = CatchChallenger::Direction_look_at_top;
    } else if (keyPressed.find(Qt::Key_Down) != keyPressed.cend()) {
        direction = CatchChallenger::Direction_look_at_bottom;
    }
    //follow the player direction
    switch (direction) {
        case CatchChallenger::Direction_look_at_left:
            followingMonsterMapObject->setPosition(QPoint(x + 1, y + 1));
            break;
        case CatchChallenger::Direction_look_at_right:
            followingMonsterMapObject->setPosition(QPoint(x - 2, y + 1));
            break;
        case CatchChallenger::Direction_look_at_top:
            followingMonsterMapObject->setPosition(QPoint(x, y + 2));
            break;
        case CatchChallenger::Direction_look_at_bottom:
            followingMonsterMapObject->setPosition(QPoint(x, y));
            break;
        default:
            std::cerr << "direction " << static_cast<unsigned int>(direction) << " cannot be handled." << std::endl;
            break;
    }
}

std::string MapVisualiserPlayer::StepToSTring(int step) {
    switch(step) {
            case CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Top: return "top leftfoot";
            case CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Left: return "left leftfoot";
            case CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Top:return "top rightfoot";
            case CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Left:return "left rightfoot";
            case CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Bottom:return "down leftfoot";
            case CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Right:return "right leftfoot";
            case CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Bottom:return "down rightfoot";
            case CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Right:return "right rightfoot";
    }
    return "unknown";
}

void MapVisualiserPlayer::doMoveAnimation()
{
    moveStepSlot();
}

void MapVisualiserPlayer::moveStepSlot()
{
    MapVisualiserThread::Map_full * map_full=all_map.at(current_map);
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
            uint8_t baseMonster;
            switch(direction)
            {
                case CatchChallenger::Direction_move_at_left:
                baseTile=10;
                baseMonster = CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Left;
                break;
                case CatchChallenger::Direction_move_at_right:
                baseTile=4;
                baseMonster = CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Right;
                break;
                case CatchChallenger::Direction_move_at_top:
                baseTile=1;
                baseMonster = CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Top;
                break;
                case CatchChallenger::Direction_move_at_bottom:
                baseTile=7;
                baseMonster = CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Bottom;
                break;
                default:
                qDebug() << QStringLiteral("moveStepSlot(): moveStep: %1, wrong direction").arg(moveStep);
                return;
            }
            Tiled::Cell cell=playerMapObject->cell();
            cell.tile=playerTileset->tileAt(baseTile+0);
            playerMapObject->setCell(cell);
            updateFollowingMonster(baseMonster);
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
    int baseMonster = CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Bottom;
    //move the player for intermediate step and define the base tile (define the stopped step with direction)
    switch(direction)
    {
        case CatchChallenger::Direction_move_at_left:
        baseTile=10;
        baseMonster = CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Left;
        switch(moveStep)
        {
            case 1:
            case 2:
            case 3:
            case 4:
            playerMapObject->setX(playerMapObject->x()-0.20);
            followingMonsterMapObject->setX(followingMonsterMapObject->x() - 0.20);
            break;
        }
        break;
        case CatchChallenger::Direction_move_at_right:
        baseMonster = CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Right;
        baseTile=4;
        switch(moveStep)
        {
            case 1:
            case 2:
            case 3:
            case 4:
            playerMapObject->setX(playerMapObject->x()+0.20);
            followingMonsterMapObject->setX(followingMonsterMapObject->x() + 0.20);
            break;
        }
        break;
        case CatchChallenger::Direction_move_at_top:
        baseMonster = CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Top;
        baseTile=1;
        switch(moveStep)
        {
            case 1:
            case 2:
            case 3:
            case 4:
            playerMapObject->setY(playerMapObject->y()-0.20);
            followingMonsterMapObject->setY(followingMonsterMapObject->y()-0.20);
            break;
        }
        break;
        case CatchChallenger::Direction_move_at_bottom:
        baseMonster = CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Bottom;
        baseTile=7;
        switch(moveStep)
        {
            case 1:
            case 2:
            case 3:
            case 4:
            playerMapObject->setY(playerMapObject->y()+0.20);
            followingMonsterMapObject->setY(followingMonsterMapObject->y() + 0.20);
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
            updateFollowingMonster(baseMonster);
        }
        break;
        case 1:
        MapObjectItem::objectLink.at(playerMapObject)->setZValue(qCeil(playerMapObject->y()));
        break;
        //transition step
        case 2:
        {
            Tiled::Cell cell=playerMapObject->cell();
            if(stepAlternance) {
                cell.tile=playerTileset->tileAt(baseTile-1);
            } else {
                cell.tile=playerTileset->tileAt(baseTile+1);
            }
            transitionMonster(baseMonster);
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
            updateFollowingMonster(baseMonster);
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

void MapVisualiserPlayer::transitionMonster(int baseMonster) {
    if (baseMonster == CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Left) {
        updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Left);
    } else if (baseMonster == CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Left) {
        updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Left);
    }
    if (baseMonster == CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Right) {
        updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Right);
    } else if (baseMonster == CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Right) {
        updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Right);
    }
    if (baseMonster == CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Top) {
        updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Top);
    } else if (baseMonster == CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Top) {
        updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Top);
    }
    if (baseMonster == CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Bottom) {
        updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Bottom);
    } else if (baseMonster == CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Bottom) {
        updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Bottom);
    }
}

bool MapVisualiserPlayer::asyncMapLoaded(const std::string &fileName,MapVisualiserThread::Map_full * tempMapObject)
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
                                        if(DatapackClientLoader::datapackLoader.itemOnMap.find(tempMap)!=
                                                DatapackClientLoader::datapackLoader.itemOnMap.cend())
                                        {
                                            const std::unordered_map<std::pair<uint8_t,uint8_t>,uint16_t,pairhash> &tempIndexItem=
                                                    DatapackClientLoader::datapackLoader.itemOnMap.at(tempMap);
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
                                    ObjectGroupItem::objectGroupLink.at(objectGroup)->removeObject(object);
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

void MapVisualiserPlayer::setInformations(std::unordered_map<uint16_t, uint32_t> *items, std::unordered_map<uint16_t, CatchChallenger::PlayerQuest> *quests, std::vector<uint8_t> *events, std::unordered_set<uint16_t> *itemOnMap, std::unordered_map<uint16_t, CatchChallenger::PlayerPlant> *plantOnMap)
{
    this->events=events;
    this->items=items;
    this->quests=quests;
    this->itemOnMap=itemOnMap;
    this->plantOnMap=plantOnMap;
    if(plantOnMap->size()>USHRT_MAX)
        abort();
    if(items->size()>USHRT_MAX)
        abort();
    if(quests->size()>USHRT_MAX)
        abort();
    if(itemOnMap->size()>USHRT_MAX)
        abort();
}

void MapVisualiserPlayer::unblock()
{
    blocked=false;
}

void MapVisualiserPlayer::fetchPlayer() {
    if (playerTilesetCache.find(lastTileset) != playerTilesetCache.cend())
    {
        //found in playerTilesetCache
        playerTileset = playerTilesetCache.at(lastTileset);
    }
    else
    {
        if (!lastTileset.empty())
        //if the id string was not initializated?
        {
            //take the default one.
            playerTileset = playerTilesetCache[defaultTileset];
        }
        else
        //load again, should no happenend, this
        {
            const std::string &imagePath = playerSkinPath + MapVisualiserPlayer::text_slash + lastTileset + MapVisualiserPlayer::text_dotpng;
            QImage image(QString::fromStdString(imagePath));
            if(!image.isNull())
            {
                playerTileset = new Tiled::Tileset(QString::fromStdString(lastTileset), 16, 24);
                playerTileset->loadFromImage(image,QString::fromStdString(imagePath));
            }
            else
            {
                qDebug() << "Unable to load the player tilset: "+QString::fromStdString(imagePath);
                playerTileset=playerTilesetCache[defaultTileset];
            }
        }
        //save in cache
        playerTilesetCache[lastTileset] = playerTileset;
    }
}

void MapVisualiserPlayer::fetchFollowingMonster() {
    if(playerTilesetCache.find(defaultMonsterTileset) != playerTilesetCache.cend())
    {
        followingMonsterTileset = playerTilesetCache.at(defaultMonsterTileset);
    }
    else
    {
        if (!defaultMonsterTileset.empty())
        {
            followingMonsterTileset = playerTilesetCache[defaultMonsterTileset];
        }
        else
        {
            //Should not happened
            if (followingMonsterTileset) {
                delete followingMonsterTileset;
                followingMonsterTileset = nullptr;
            }
            followingMonsterTileset = new Tiled::Tileset(QStringLiteral("followingmonster"), 32, 32);
            //The main following monster character selected by ID.
            followingMonsterSkinPath = datapackPath + "monsters/" + std::to_string(followingMonsterInformation.monsterId);
            std::string imagePath = followingMonsterSkinPath + MapVisualiserPlayer::text_slashMonsterpng;
            if (!followingMonsterTileset->loadFromImage(imagePath)) {
                //The default following monster character selected from skinFolderList (hardcoded, cannot access MapControllerMP::skinFolderList)
                imagePath = ":/images/followingMonster_default" + MapVisualiserPlayer::text_slashtrainerMonsterpng;
                if (!followingMonsterTileset->loadFromImage(imagePath))
                {
                    qDebug() << "Unable to load the player tilset: " + QString::fromStdString(imagePath);
                    followingMonsterTileset = playerTilesetCache[defaultMonsterTileset];
                }
            }
            const_cast<Tiled::Cell&>(followingMonsterMapObject->cell()).tile = followingMonsterTileset->tileAt(CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Top);
        }

        playerTilesetCache[defaultMonsterTileset] = followingMonsterTileset;
    }
}

void MapVisualiserPlayer::updateTilesetForNewTerrain()
{
    Tiled::Cell cell = playerMapObject->cell();
    int tileId = cell.tile->id();
    cell.tile = playerTileset->tileAt(tileId);//new contents of playerTileset according terrain
    playerMapObject->setCell(cell);

    updateFollowingMonster();
}

void MapVisualiserPlayer::finalPlayerStep()
{
    if(all_map.find(current_map)==all_map.cend())
    {
        qDebug() << "current map not loaded, unable to do finalPlayerStep()";
        return;
    }
    const MapVisualiserThread::Map_full * current_map_pointer=all_map.at(current_map);
    if(current_map_pointer==NULL)
    {
        qDebug() << "current map not loaded null pointer, unable to do finalPlayerStep()";
        return;
    }

    /// \see into haveStopTileAction(), to NPC fight: std::vector<std::pair<uint8_t,uint8_t> > botFightRemotePointList=all_map.value(current_map)->logicalMap.botsFightTriggerExtra.values(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)));
    if(!CatchChallenger::CommonDatapack::commonDatapack.monstersCollision.empty())
    {
        const CatchChallenger::MonstersCollisionValue &monstersCollisionValue=CatchChallenger::MoveOnTheMap::getZoneCollision(current_map_pointer->logicalMap,x,y);
        unsigned int index=0;
        while(index<monstersCollisionValue.walkOn.size())
        {
            const unsigned int &newIndex=monstersCollisionValue.walkOn.at(index);
            if(newIndex<CatchChallenger::CommonDatapack::commonDatapack.monstersCollision.size())
            {
                const CatchChallenger::MonstersCollision &monstersCollision = CatchChallenger::CommonDatapack::commonDatapack.monstersCollision.at(newIndex);
                if(monstersCollision.item==0 || items->find(monstersCollision.item)!=items->cend())
                {
                    if(monstersCollision.tile!=lastTileset)
                    {
                        lastTileset = monstersCollision.tile;
                        fetchPlayer();
                        fetchFollowingMonster();
                        updateTilesetForNewTerrain();
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
            updateTilesetForNewTerrain();
        }
    }
    //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
    playerMapObject->setPosition(QPoint(x,y+1));
    updateFollowingMonsterPosition();
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
            updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Left);
            inMove=false;
            emit send_player_direction(direction);//see the top note
            parseStop();
        }
        //if can go, then do the move
        else
        {
            direction=CatchChallenger::Direction_move_at_left;
            moveStep=0;
            updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Left);
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
            updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Right);
            inMove=false;
            emit send_player_direction(direction);//see the top note
            parseStop();
        }
        //if can go, then do the move
        else
        {
            direction=CatchChallenger::Direction_move_at_right;
            moveStep=0;
            updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Right);
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
            updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Top);
            inMove=false;
            emit send_player_direction(direction);//see the top note
            parseStop();
        }
        //if can go, then do the move
        else
        {
            direction=CatchChallenger::Direction_move_at_top;
            moveStep=0;
            updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Top);
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
            updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkRightFoot_Bottom);
            inMove=false;
            emit send_player_direction(direction);//see the top note
            parseStop();
        }
        //if can go, then do the move
        else
        {
            direction=CatchChallenger::Direction_move_at_bottom;
            moveStep=0;
            updateFollowingMonster(CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Bottom);
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
    updateFollowingMonster(monsterLastTileset);
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
    {
        keyPressParse();
    }
}

std::string MapVisualiserPlayer::lastLocation() const
{
    return mLastLocation;
}

std::string MapVisualiserPlayer::currentMap() const
{
    return current_map;
}

MapVisualiserThread::Map_full * MapVisualiserPlayer::currentMapFull() const
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
    const MapVisualiserThread::Map_full * const mapFull=all_map.at(current_map);
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
    const MapVisualiserThread::Map_full * const mapFull=all_map.at(current_map);
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
    const MapVisualiserThread::Map_full * const mapFull=all_map.at(current_map);
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
    wasPathFindingUsed=false;
    inMove=false;
    MapVisualiser::resetAll();
    lastAction.restart();
    mapVisualiserThread.stopIt=true;
    mapVisualiserThread.quit();
    mapVisualiserThread.wait();
    mapVisualiserThread.start(QThread::IdlePriority);

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
    lastTileset = defaultTileset;
    lastMonsterTileset = defaultMonsterTileset;
    playerTileset = new Tiled::Tileset(QStringLiteral("player"),16,24);
    followingMonsterTileset = new Tiled::Tileset(QStringLiteral("followingmonster"), 32, 32);
    playerTilesetCache[lastTileset] = playerTileset;
    playerTilesetCache[lastMonsterTileset] = followingMonsterTileset;
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
//TODO: separate following monster from player
void MapVisualiserPlayer::loadPlayerFromCurrentMap()
{
    if (all_map.find(current_map) == all_map.cend())
    {
        qDebug() << QStringLiteral("all_map have not the current map: %1").arg(QString::fromStdString(current_map));
        return;
    }
    Tiled::ObjectGroup *currentGroup = playerMapObject->objectGroup();
    Tiled::ObjectGroup *othercurrentGroup = followingMonsterMapObject->objectGroup();
    if (currentGroup != NULL)
    {
        if (ObjectGroupItem::objectGroupLink.find(currentGroup) != ObjectGroupItem::objectGroupLink.cend()) {
            ObjectGroupItem::objectGroupLink.at(currentGroup)->removeObject(playerMapObject);
        }

        if (currentGroup!=all_map.at(current_map)->objectGroup) {
            qDebug() << QStringLiteral("loadPlayerFromCurrentMap(), the playerMapObject group is wrong: %1").arg(currentGroup->name());
        }
    }
    if (othercurrentGroup != NULL)
    {
        if (ObjectGroupItem::objectGroupLink.find(othercurrentGroup) != ObjectGroupItem::objectGroupLink.cend()) {
            ObjectGroupItem::objectGroupLink.at(othercurrentGroup)->removeObject(followingMonsterMapObject);
        }

        if (othercurrentGroup!=all_map.at(current_map)->objectGroup) {
            qDebug() << QStringLiteral("loadPlayerFromCurrentMap(), the followingmonster group is wrong: %1").arg(othercurrentGroup->name());
        }
    }

    Tiled::ObjectGroup *currentMapGroup = all_map.at(current_map)->objectGroup;
    if (ObjectGroupItem::objectGroupLink.find(currentMapGroup) != ObjectGroupItem::objectGroupLink.cend()) {
        ObjectGroupItem::objectGroupLink.at(currentMapGroup)->addObject(playerMapObject);
        ObjectGroupItem::objectGroupLink.at(currentMapGroup)->addObject(followingMonsterMapObject);
    } else {
        qDebug() << QStringLiteral("loadPlayerFromCurrentMap(), ObjectGroupItem::objectGroupLink not contains current_map->objectGroup");
    }
    mLastLocation = all_map.at(current_map)->logicalMap.map_file;

    //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
    playerMapObject->setPosition(QPoint(x, y + 1));
    MapObjectItem::objectLink.at(playerMapObject)->setZValue(y);
    //move following to the final position (integer), x + 2 because the tile lib start x behind the player
    updateFollowingMonsterPosition();
    MapObjectItem::objectLink.at(followingMonsterMapObject)->setZValue(y);

    if (centerOnPlayer) {
        centerOn(MapObjectItem::objectLink.at(playerMapObject));
    }
    centerOn(MapObjectItem::objectLink.at(followingMonsterMapObject));
}

//call before leave the old map (and before loadPlayerFromCurrentMap())
void MapVisualiserPlayer::unloadPlayerFromCurrentMap()
{
    Tiled::ObjectGroup* currentGroup = playerMapObject->objectGroup();
    if (currentGroup == NULL) {
        return;
    }
    //unload the player sprite
    if(ObjectGroupItem::objectGroupLink.find(currentGroup) != ObjectGroupItem::objectGroupLink.cend()) {
        ObjectGroupItem::objectGroupLink.at(currentGroup)->removeObject(playerMapObject);
    } else {
        qDebug() << QStringLiteral("unloadPlayerFromCurrentMap(), ObjectGroupItem::objectGroupLink not contains playerMapObject->objectGroup()");
    }
}


void MapVisualiserPlayer::unloadFollowingMonsterFromCurrentMap()
{
    Tiled::ObjectGroup *currentGroup = followingMonsterMapObject->objectGroup();
    if (currentGroup == NULL) {
        return;
    }
    //unload the following monster sprite
    if (ObjectGroupItem::objectGroupLink.find(currentGroup) != ObjectGroupItem::objectGroupLink.cend()) {
        ObjectGroupItem::objectGroupLink.at(currentGroup)->removeObject(followingMonsterMapObject);
    } else {
        qDebug() << QStringLiteral("unloadFollowingMonsterFromCurrentMap(), ObjectGroupItem::objectGroupLink not contains followingMonsterMapObject->objectGroup()");
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
