#include "MapController.hpp"
#include "../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../general/base/MoveOnTheMap.hpp"
#include <QtMath>
#include <iostream>
#include <QDebug>

void MapControllerMP::moveOtherPlayerStepSlot()
{
    QTimer *timer=qobject_cast<QTimer *>(QObject::sender());
    if(timer==NULL)
    {
        qDebug() << "moveOtherPlayerStepSlot() timer not located";
        return;
    }
    if(otherPlayerListByTimer.find(timer)==otherPlayerListByTimer.cend())
    {
        timer->stop();
        timer->deleteLater();
        return;
    }
    const uint16_t &simplifiedId=otherPlayerListByTimer.at(timer);
    OtherPlayer &otherPlayer=otherPlayerList[simplifiedId];
    switch(otherPlayer.presumed_direction)
    {
        case CatchChallenger::Direction_move_at_right:
        case CatchChallenger::Direction_move_at_top:
        case CatchChallenger::Direction_move_at_bottom:
        case CatchChallenger::Direction_move_at_left:
        break;
        default:
        return;
    }
//    std::cout << "MapControllerMP::moveOtherPlayerStepSlot(): " << std::to_string(otherPlayer.presumed_direction) << std::endl;
    moveOtherPlayerStepSlotWithPlayer(otherPlayerList[simplifiedId]);
}

void MapControllerMP::moveOtherPlayerStepSlotWithPlayer(OtherPlayer &otherPlayer)
{
    #ifdef DEBUG_CLIENT_OTHER_PLAYER_MOVE_STEP
    qDebug() << QStringLiteral("moveOtherPlayerStepSlot() player: %1 (%2), moveStep: %3")
            .arg(otherPlayer.informations.pseudo)
            .arg(otherPlayer.informations.simplifiedId)
            .arg(otherPlayer.moveStep);
    #endif
    //tiger the next tile
    if(!otherPlayer.animationDisplayed)
    {
        otherPlayer.animationDisplayed=true;
        CATCHCHALLENGER_TYPE_MAPID mapIndex=otherPlayer.presumed_map;
        uint8_t x=otherPlayer.presumed_x;
        uint8_t y=otherPlayer.presumed_y;
        //set the final value (direction, position, ...)
        switch(otherPlayer.presumed_direction)
        {
            case CatchChallenger::Direction_move_at_right:
            case CatchChallenger::Direction_move_at_top:
            case CatchChallenger::Direction_move_at_bottom:
            case CatchChallenger::Direction_move_at_left:
                CatchChallenger::MoveOnTheMap::move(QtDatapackClientLoader::datapackLoader->get_mapList(),otherPlayer.presumed_direction,mapIndex,x,y,false);
            break;
            default:
            break;
        }
        if(CatchChallenger::QMap_client::all_map.find(mapIndex)!=CatchChallenger::QMap_client::all_map.cend())
            if(CatchChallenger::QMap_client::all_map.at(mapIndex)->doors.find(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)))!=
                    CatchChallenger::QMap_client::all_map.at(mapIndex)->doors.cend())
            {
                if(otherPlayerListByAnimationTimer.find(otherPlayer.moveAnimationTimer)!=otherPlayerListByAnimationTimer.cend())
                {
                    MapDoor* door=CatchChallenger::QMap_client::all_map.at(mapIndex)->doors.at(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)));
                    door->startOpen(static_cast<uint16_t>(otherPlayer.playerSpeed));
                    otherPlayer.moveAnimationTimer->start(door->timeToOpen());
                }
                return;
            }
    }

    switch(otherPlayer.presumed_direction)
    {
        case CatchChallenger::Direction_move_at_left:
        case CatchChallenger::Direction_move_at_right:
        case CatchChallenger::Direction_move_at_top:
        case CatchChallenger::Direction_move_at_bottom:
        otherPlayer.inMove=true;
        break;
        default:
        break;
    }

    //monster
    if(otherPlayer.monsterMapObject!=NULL)
    {
        if(otherPlayer.inMove && otherPlayer.moveStep==1)
            switch(otherPlayer.direction)
            {
                case CatchChallenger::Direction_move_at_left:
                case CatchChallenger::Direction_move_at_right:
                case CatchChallenger::Direction_move_at_top:
                case CatchChallenger::Direction_move_at_bottom:
                    otherPlayer.pendingMonsterMoves.push_back(otherPlayer.direction);
                break;
                default:
                break;
            }
        #ifdef CATCHCHALLENGER_HARDENED
        if(otherPlayer.pendingMonsterMoves.size()>2)
            abort();
        #endif
        if(otherPlayer.pendingMonsterMoves.size()>1)
        {
            //start move
            //moveTimer.stop();
            int baseTile=1;
            //move the player for intermediate step and define the base tile (define the stopped step with direction)
            switch(otherPlayer.pendingMonsterMoves.front())
            {
                case CatchChallenger::Direction_move_at_left:
                baseTile=3;
                switch(otherPlayer.moveStep)
                {
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    otherPlayer.monsterMapObject->setX(otherPlayer.monsterMapObject->x()-0.20);
                    break;
                }
                break;
                case CatchChallenger::Direction_move_at_right:
                baseTile=7;
                switch(otherPlayer.moveStep)
                {
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    otherPlayer.monsterMapObject->setX(otherPlayer.monsterMapObject->x()+0.20);
                    break;
                }
                break;
                case CatchChallenger::Direction_move_at_top:
                baseTile=2;
                switch(otherPlayer.moveStep)
                {
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    otherPlayer.monsterMapObject->setY(otherPlayer.monsterMapObject->y()-0.20);
                    break;
                }
                break;
                case CatchChallenger::Direction_move_at_bottom:
                baseTile=6;
                switch(otherPlayer.moveStep)
                {
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    otherPlayer.monsterMapObject->setY(otherPlayer.monsterMapObject->y()+0.20);
                    break;
                }
                break;
                default:
                qDebug() << QStringLiteral("moveStepSlot(): moveStep: %1, wrong direction").arg(otherPlayer.moveStep);
                return;
            }

            //apply the right step of the base step defined previously by the direction
            switch(otherPlayer.moveStep%5)
            {
                //stopped step
                case 0:
                {
                    Tiled::Cell cell=otherPlayer.monsterMapObject->cell();
                    cell.setTile(otherPlayer.monsterTileset->tileAt(baseTile+0));
                    otherPlayer.monsterMapObject->setCell(cell);
                }
                break;
                case 1:
                MapObjectItem::objectLink.at(otherPlayer.monsterMapObject)->setZValue(qCeil(otherPlayer.monsterMapObject->y()));
                break;
                //transition step
                case 2:
                {
                    Tiled::Cell cell=otherPlayer.monsterMapObject->cell();
                    cell.setTile(otherPlayer.monsterTileset->tileAt(baseTile-2));
                    otherPlayer.monsterMapObject->setCell(cell);
                }
                break;
                //stopped step
                case 4:
                {
                    Tiled::Cell cell=otherPlayer.monsterMapObject->cell();
                    cell.setTile(otherPlayer.monsterTileset->tileAt(baseTile+0));
                    otherPlayer.monsterMapObject->setCell(cell);
                }
                break;
            }
        }
    }

    int baseTile=1;
    //move the player for intermediate step and define the base tile (define the stopped step with direction)
    switch(otherPlayer.presumed_direction)
    {
        case CatchChallenger::Direction_move_at_left:
        baseTile=10;
        switch(otherPlayer.moveStep)
        {
            case 1:
            case 2:
            case 3:
            case 4:
            otherPlayer.playerMapObject->setX(otherPlayer.playerMapObject->x()-0.20);
            if(otherPlayer.labelMapObject!=NULL)
                otherPlayer.labelMapObject->setX(otherPlayer.labelMapObject->x()-0.20);
            break;
        }
        break;
        case CatchChallenger::Direction_move_at_right:
        baseTile=4;
        switch(otherPlayer.moveStep)
        {
            case 1:
            case 2:
            case 3:
            case 4:
            otherPlayer.playerMapObject->setX(otherPlayer.playerMapObject->x()+0.20);
            if(otherPlayer.labelMapObject!=NULL)
                otherPlayer.labelMapObject->setX(otherPlayer.labelMapObject->x()+0.20);
            break;
        }
        break;
        case CatchChallenger::Direction_move_at_top:
        baseTile=1;
        switch(otherPlayer.moveStep)
        {
            case 1:
            case 2:
            case 3:
            case 4:
            otherPlayer.playerMapObject->setY(otherPlayer.playerMapObject->y()-0.20);
            if(otherPlayer.labelMapObject!=NULL)
                otherPlayer.labelMapObject->setY(otherPlayer.labelMapObject->y()-0.20);
            break;
        }
        break;
        case CatchChallenger::Direction_move_at_bottom:
        baseTile=7;
        switch(otherPlayer.moveStep)
        {
            case 1:
            case 2:
            case 3:
            case 4:
            otherPlayer.playerMapObject->setY(otherPlayer.playerMapObject->y()+0.20);
            if(otherPlayer.labelMapObject!=NULL)
                otherPlayer.labelMapObject->setY(otherPlayer.labelMapObject->y()+0.20);
            break;
        }
        break;
        default:
        qDebug() << QStringLiteral("moveOtherPlayerStepSlot(): moveStep: %1, wrong direction").arg(otherPlayer.moveStep);
        otherPlayer.oneStepMore->stop();
        return;
    }

    //apply the right step of the base step defined previously by the direction
    switch(otherPlayer.moveStep)
    {
        //stopped step
        case 0:
        {
            Tiled::Cell cell=otherPlayer.playerMapObject->cell();
            cell.setTile(otherPlayer.playerTileset->tileAt(baseTile+0));
            otherPlayer.playerMapObject->setCell(cell);
        }
        break;
        case 1:
        if(MapObjectItem::objectLink.find(otherPlayer.playerMapObject)!=MapObjectItem::objectLink.cend())
            MapObjectItem::objectLink.at(otherPlayer.playerMapObject)->setZValue(qCeil(otherPlayer.playerMapObject->y()));
        break;
        //transition step
        case 2:
        {
            Tiled::Cell cell=otherPlayer.playerMapObject->cell();
            if(otherPlayer.stepAlternance)
                cell.setTile(otherPlayer.playerTileset->tileAt(baseTile-1));
            else
                cell.setTile(otherPlayer.playerTileset->tileAt(baseTile+1));
            otherPlayer.playerMapObject->setCell(cell);
            otherPlayer.stepAlternance=!otherPlayer.stepAlternance;
        }
        break;
        //stopped step
        case 4:
        {
            Tiled::Cell cell=otherPlayer.playerMapObject->cell();
            cell.setTile(otherPlayer.playerTileset->tileAt(baseTile+0));
            otherPlayer.playerMapObject->setCell(cell);
        }
        break;
    }

    otherPlayer.moveStep++;

    //if have finish the step
    if(otherPlayer.moveStep>5)
    {
        if(otherPlayer.pendingMonsterMoves.size()>1)
        {
            const CatchChallenger::Direction direction=otherPlayer.pendingMonsterMoves.front();
            otherPlayer.pendingMonsterMoves.erase(otherPlayer.pendingMonsterMoves.cbegin());

            CATCHCHALLENGER_TYPE_MAPID monsterMapIndex=otherPlayer.current_monster_map;
            const CATCHCHALLENGER_TYPE_MAPID oldMonsterMapIndex=monsterMapIndex;
            //set the final value (direction, position, ...)
            switch(direction)
            {
                case CatchChallenger::Direction_move_at_left:
                case CatchChallenger::Direction_move_at_right:
                case CatchChallenger::Direction_move_at_top:
                case CatchChallenger::Direction_move_at_bottom:
                    if(!CatchChallenger::MoveOnTheMap::move(QtDatapackClientLoader::datapackLoader->get_mapList(),direction,monsterMapIndex,otherPlayer.monster_x,otherPlayer.monster_y,false))
                    {
                        std::cerr << "Bug at move for pendingMonsterMoves, unknown move: " << std::to_string(direction)
                                  << " from " << std::to_string(monsterMapIndex) << " (" << std::to_string(otherPlayer.monster_x) << "," << std::to_string(otherPlayer.monster_y) << ")"
                                  << std::endl;
                        resetMonsterTile();
                    }
                break;
                default:
                    qDebug() << QStringLiteral("moveStepSlot(): moveStep: %1, wrong direction (%2) when moveStep>2").arg(otherPlayer.moveStep).arg(direction);
                return;
            }
            //if the map have changed
            if(oldMonsterMapIndex!=monsterMapIndex)
            {
                unloadOtherMonsterFromCurrentMap(otherPlayer);
                otherPlayer.current_monster_map=monsterMapIndex;
                if(CatchChallenger::QMap_client::old_all_map.find(otherPlayer.current_monster_map)==CatchChallenger::QMap_client::old_all_map.cend())
                    std::cerr << "old_all_map.find(current_map)==old_all_map.cend() in monster follow" << std::endl;
                const CatchChallenger::CommonMap &oldMonsterMap=QtDatapackClientLoader::datapackLoader->getMap(oldMonsterMapIndex);
                if(oldMonsterMap.border.top.mapIndex!=monsterMapIndex && oldMonsterMap.border.bottom.mapIndex!=monsterMapIndex &&
                   oldMonsterMap.border.left.mapIndex!=monsterMapIndex && oldMonsterMap.border.right.mapIndex!=monsterMapIndex)
                    resetOtherMonsterTile(otherPlayer);
                loadOtherMonsterFromCurrentMap(otherPlayer);
            }

            if(otherPlayer.monsterMapObject!=NULL)
            {
                otherPlayer.monsterMapObject->setPosition(QPointF((float)otherPlayer.monster_x-0.5,(float)otherPlayer.monster_y+1));
                MapObjectItem::objectLink.at(otherPlayer.monsterMapObject)->setZValue(otherPlayer.monster_y);
            }
            else
                otherPlayer.pendingMonsterMoves.clear();
        }
        otherPlayer.animationDisplayed=false;
        const CATCHCHALLENGER_TYPE_MAPID old_presumed_map=otherPlayer.presumed_map;
        CATCHCHALLENGER_TYPE_MAPID new_presumed_map=otherPlayer.presumed_map;
        uint8_t x=otherPlayer.presumed_x;
        uint8_t y=otherPlayer.presumed_y;
        //set the final value (direction, position, ...)
        switch(otherPlayer.presumed_direction)
        {
            case CatchChallenger::Direction_move_at_right:
            case CatchChallenger::Direction_move_at_top:
            case CatchChallenger::Direction_move_at_bottom:
            case CatchChallenger::Direction_move_at_left:
                CatchChallenger::MoveOnTheMap::move(QtDatapackClientLoader::datapackLoader->get_mapList(),otherPlayer.presumed_direction,new_presumed_map,x,y,false);
            break;
            default:
            qDebug() << QStringLiteral("moveStepSlot(): moveStep: %1, wrong direction when moveStep>2").arg(otherPlayer.moveStep);
            return;
        }
        otherPlayer.presumed_x=x;
        otherPlayer.presumed_y=y;
        //if the map have changed
        if(old_presumed_map!=new_presumed_map)
        {
            loadOtherMap(new_presumed_map);
            if(CatchChallenger::QMap_client::all_map.find(new_presumed_map)==CatchChallenger::QMap_client::all_map.cend())
                qDebug() << QStringLiteral("map changed not located: %1").arg(new_presumed_map);
            else
            {
                unloadOtherPlayerFromMap(otherPlayer);
                otherPlayer.presumed_map=new_presumed_map;
                loadOtherPlayerFromMap(otherPlayer);
                const CatchChallenger::CommonMap &oldMap=QtDatapackClientLoader::datapackLoader->getMap(old_presumed_map);
                if(oldMap.border.top.mapIndex!=new_presumed_map && oldMap.border.bottom.mapIndex!=new_presumed_map &&
                   oldMap.border.left.mapIndex!=new_presumed_map && oldMap.border.right.mapIndex!=new_presumed_map)
                    resetOtherMonsterTile(otherPlayer);
            }
        }
        //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
        otherPlayer.playerMapObject->setPosition(QPointF(x,y+1));
        if(otherPlayer.labelMapObject!=NULL)
            otherPlayer.labelMapObject->setPosition(QPointF(static_cast<qreal>(x)-static_cast<qreal>(otherPlayer.labelTileset->tileWidth())
                                                            /2/16+0.5,y+1-1.4));
        if(MapObjectItem::objectLink.find(otherPlayer.playerMapObject)!=MapObjectItem::objectLink.cend())
            MapObjectItem::objectLink.at(otherPlayer.playerMapObject)->setZValue(y);

        //check if one arrow key is pressed to continue to move into this direction
        if(otherPlayer.presumed_direction==CatchChallenger::Direction_move_at_left)
        {
            //can't go into this direction, then just look into this direction
            if(!CatchChallenger::MoveOnTheMap::canGoTo(QtDatapackClientLoader::datapackLoader->get_mapList(),CatchChallenger::Direction_move_at_left,QtDatapackClientLoader::datapackLoader->getMap(otherPlayer.presumed_map),x,y,true))
            {
                otherPlayer.presumed_direction=CatchChallenger::Direction_look_at_left;
                Tiled::Cell cell=otherPlayer.playerMapObject->cell();
                cell.setTile(otherPlayer.playerTileset->tileAt(10));
                otherPlayer.playerMapObject->setCell(cell);
                otherPlayer.inMove=false;
                otherPlayer.oneStepMore->stop();
                //std::cout << "[debug], can't go left" << std::endl;
                finalOtherPlayerStep(otherPlayer);
            }
            //if can go, then do the move
            else
            {
                otherPlayer.presumed_direction=CatchChallenger::Direction_move_at_left;
                otherPlayer.moveStep=0;
                #ifdef CATCHCHALLENGER_HARDENED
                if(otherPlayer.pendingMonsterMoves.size()>1)
                    abort();
                #endif
                finalOtherPlayerStep(otherPlayer);
                moveOtherPlayerStepSlotWithPlayer(otherPlayer);
            }
        }
        else if(otherPlayer.presumed_direction==CatchChallenger::Direction_move_at_right)
        {
            //can't go into this direction, then just look into this direction
            if(!CatchChallenger::MoveOnTheMap::canGoTo(QtDatapackClientLoader::datapackLoader->get_mapList(),CatchChallenger::Direction_move_at_right,QtDatapackClientLoader::datapackLoader->getMap(otherPlayer.presumed_map),x,y,true))
            {
                otherPlayer.presumed_direction=CatchChallenger::Direction_look_at_right;
                Tiled::Cell cell=otherPlayer.playerMapObject->cell();
                cell.setTile(otherPlayer.playerTileset->tileAt(4));
                otherPlayer.playerMapObject->setCell(cell);
                otherPlayer.inMove=false;
                otherPlayer.oneStepMore->stop();
                //std::cout << "[debug], can't go right" << std::endl;
                finalOtherPlayerStep(otherPlayer);
            }
            //if can go, then do the move
            else
            {
                otherPlayer.presumed_direction=CatchChallenger::Direction_move_at_right;
                otherPlayer.moveStep=0;
                #ifdef CATCHCHALLENGER_HARDENED
                if(otherPlayer.pendingMonsterMoves.size()>1)
                    abort();
                #endif
                finalOtherPlayerStep(otherPlayer);
                moveOtherPlayerStepSlotWithPlayer(otherPlayer);
            }
        }
        else if(otherPlayer.presumed_direction==CatchChallenger::Direction_move_at_top)
        {
            //can't go into this direction, then just look into this direction
            if(!CatchChallenger::MoveOnTheMap::canGoTo(QtDatapackClientLoader::datapackLoader->get_mapList(),CatchChallenger::Direction_move_at_top,QtDatapackClientLoader::datapackLoader->getMap(otherPlayer.presumed_map),x,y,true))
            {
                otherPlayer.presumed_direction=CatchChallenger::Direction_look_at_top;

                Tiled::Cell cell=otherPlayer.playerMapObject->cell();
                cell.setTile(otherPlayer.playerTileset->tileAt(1));
                otherPlayer.playerMapObject->setCell(cell);
                otherPlayer.inMove=false;
                otherPlayer.oneStepMore->stop();
                //std::cout << "[debug], can't go top" << std::endl;
                finalOtherPlayerStep(otherPlayer);
            }
            //if can go, then do the move
            else
            {
                otherPlayer.presumed_direction=CatchChallenger::Direction_move_at_top;
                otherPlayer.moveStep=0;
                #ifdef CATCHCHALLENGER_HARDENED
                if(otherPlayer.pendingMonsterMoves.size()>1)
                    abort();
                #endif
                finalOtherPlayerStep(otherPlayer);
                moveOtherPlayerStepSlotWithPlayer(otherPlayer);
            }
        }
        else if(otherPlayer.presumed_direction==CatchChallenger::Direction_move_at_bottom)
        {
            //can't go into this direction, then just look into this direction
            if(!CatchChallenger::MoveOnTheMap::canGoTo(QtDatapackClientLoader::datapackLoader->get_mapList(),CatchChallenger::Direction_move_at_bottom,QtDatapackClientLoader::datapackLoader->getMap(otherPlayer.presumed_map),x,y,true))
            {
                otherPlayer.presumed_direction=CatchChallenger::Direction_look_at_bottom;
                Tiled::Cell cell=otherPlayer.playerMapObject->cell();
                cell.setTile(otherPlayer.playerTileset->tileAt(7));
                otherPlayer.playerMapObject->setCell(cell);
                otherPlayer.inMove=false;
                otherPlayer.oneStepMore->stop();
                //std::cout << "[debug], can't go bottom" << std::endl;
                finalOtherPlayerStep(otherPlayer);
            }
            //if can go, then do the move
            else
            {
                otherPlayer.presumed_direction=CatchChallenger::Direction_move_at_bottom;
                otherPlayer.moveStep=0;
                #ifdef CATCHCHALLENGER_HARDENED
                if(otherPlayer.pendingMonsterMoves.size()>1)
                    abort();
                #endif
                finalOtherPlayerStep(otherPlayer);
                moveOtherPlayerStepSlotWithPlayer(otherPlayer);
            }
        }
        //now stop walking, no more arrow key is pressed
        else
        {
            otherPlayer.moveStep=0;
            otherPlayer.inMove=false;
            otherPlayer.oneStepMore->stop();
        }

        #ifdef CATCHCHALLENGER_HARDENED
        if(otherPlayer.pendingMonsterMoves.size()>1)
            abort();
        #endif
    }
    else
        otherPlayer.oneStepMore->start();
}
