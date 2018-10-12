#include "MapController.h"
#include <QtMath>
#include <iostream>

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
        CatchChallenger::CommonMap * map=&otherPlayer.presumed_map->logicalMap;
        uint8_t x=otherPlayer.presumed_x;
        uint8_t y=otherPlayer.presumed_y;
        //set the final value (direction, position, ...)
        switch(otherPlayer.presumed_direction)
        {
            case CatchChallenger::Direction_move_at_right:
            case CatchChallenger::Direction_move_at_top:
            case CatchChallenger::Direction_move_at_bottom:
            case CatchChallenger::Direction_move_at_left:
                CatchChallenger::MoveOnTheMap::move(otherPlayer.presumed_direction,&map,&x,&y);
            break;
            default:
            break;
        }
        if(all_map.find(map->map_file)!=all_map.cend())
            if(all_map.at(map->map_file)->doors.find(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)))!=
                    all_map.at(map->map_file)->doors.cend())
            {
                if(otherPlayerListByAnimationTimer.find(otherPlayer.moveAnimationTimer)!=otherPlayerListByAnimationTimer.cend())
                {
                    MapDoor* door=all_map.at(map->map_file)->doors.at(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)));
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
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
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
                    cell.tile=otherPlayer.monsterTileset->tileAt(baseTile+0);
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
                    cell.tile=otherPlayer.monsterTileset->tileAt(baseTile-2);
                    otherPlayer.monsterMapObject->setCell(cell);
                }
                break;
                //stopped step
                case 4:
                {
                    Tiled::Cell cell=otherPlayer.monsterMapObject->cell();
                    cell.tile=otherPlayer.monsterTileset->tileAt(baseTile+0);
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
            cell.tile=otherPlayer.playerTileset->tileAt(baseTile+0);
            otherPlayer.playerMapObject->setCell(cell);
        }
        break;
        case 1:
        MapObjectItem::objectLink.at(otherPlayer.playerMapObject)->setZValue(qCeil(otherPlayer.playerMapObject->y()));
        break;
        //transition step
        case 2:
        {
            Tiled::Cell cell=otherPlayer.playerMapObject->cell();
            if(otherPlayer.stepAlternance)
                cell.tile=otherPlayer.playerTileset->tileAt(baseTile-1);
            else
                cell.tile=otherPlayer.playerTileset->tileAt(baseTile+1);
            otherPlayer.playerMapObject->setCell(cell);
            otherPlayer.stepAlternance=!otherPlayer.stepAlternance;
        }
        break;
        //stopped step
        case 4:
        {
            Tiled::Cell cell=otherPlayer.playerMapObject->cell();
            cell.tile=otherPlayer.playerTileset->tileAt(baseTile+0);
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

            CatchChallenger::CommonMap * map=&all_map.at(otherPlayer.current_monster_map)->logicalMap;
            const CatchChallenger::CommonMap * old_map=map;
            //set the final value (direction, position, ...)
            switch(direction)
            {
                case CatchChallenger::Direction_move_at_left:
                case CatchChallenger::Direction_move_at_right:
                case CatchChallenger::Direction_move_at_top:
                case CatchChallenger::Direction_move_at_bottom:
                    if(!CatchChallenger::MoveOnTheMap::move(direction,&map,&otherPlayer.monster_x,&otherPlayer.monster_y))
                    {
                        std::cerr << "Bug at move for pendingMonsterMoves, unknown move: " << std::to_string(direction)
                                  << " from " << map->map_file << " (" << std::to_string(otherPlayer.monster_x) << "," << std::to_string(otherPlayer.monster_y) << ")"
                                  << std::endl;
                        resetMonsterTile();
                    }
                break;
                default:
                    qDebug() << QStringLiteral("moveStepSlot(): moveStep: %1, wrong direction (%2) when moveStep>2").arg(otherPlayer.moveStep).arg(direction);
                return;
            }
            //if the map have changed
            if(old_map!=map)
            {
                unloadOtherMonsterFromCurrentMap(otherPlayer);
                otherPlayer.current_monster_map=map->map_file;
                if(old_all_map.find(otherPlayer.current_monster_map)==old_all_map.cend())
                    std::cerr << "old_all_map.find(current_map)==old_all_map.cend() in monster follow" << std::endl;
                if(!vectorcontainsAtLeastOne(old_map->near_map,map))
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
        CatchChallenger::CommonMap * old_map=&otherPlayer.presumed_map->logicalMap;
        CatchChallenger::CommonMap * map=&otherPlayer.presumed_map->logicalMap;
        uint8_t x=otherPlayer.presumed_x;
        uint8_t y=otherPlayer.presumed_y;
        //set the final value (direction, position, ...)
        switch(otherPlayer.presumed_direction)
        {
            case CatchChallenger::Direction_move_at_right:
            case CatchChallenger::Direction_move_at_top:
            case CatchChallenger::Direction_move_at_bottom:
            case CatchChallenger::Direction_move_at_left:
                CatchChallenger::MoveOnTheMap::move(otherPlayer.presumed_direction,&map,&x,&y);
            break;
            default:
            qDebug() << QStringLiteral("moveStepSlot(): moveStep: %1, wrong direction when moveStep>2").arg(otherPlayer.moveStep);
            return;
        }
        otherPlayer.presumed_x=x;
        otherPlayer.presumed_y=y;
        //if the map have changed
        if(old_map!=map)
        {
            loadOtherMap(map->map_file);
            if(all_map.find(map->map_file)==all_map.cend())
                qDebug() << QStringLiteral("map changed not located: %1").arg(QString::fromStdString(map->map_file));
            else
            {
                unloadOtherPlayerFromMap(otherPlayer);
                otherPlayer.presumed_map=all_map.at(map->map_file);
                loadOtherPlayerFromMap(otherPlayer);
                if(!vectorcontainsAtLeastOne(old_map->near_map,map))
                    resetOtherMonsterTile(otherPlayer);
            }
        }
        //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
        otherPlayer.playerMapObject->setPosition(QPointF(x,y+1));
        if(otherPlayer.labelMapObject!=NULL)
            otherPlayer.labelMapObject->setPosition(QPointF(static_cast<qreal>(x)-static_cast<qreal>(otherPlayer.labelTileset->tileWidth())
                                                            /2/16+0.5,y+1-1.4));
        MapObjectItem::objectLink.at(otherPlayer.playerMapObject)->setZValue(y);

        //check if one arrow key is pressed to continue to move into this direction
        if(otherPlayer.presumed_direction==CatchChallenger::Direction_move_at_left)
        {
            //can't go into this direction, then just look into this direction
            if(!CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_left,otherPlayer.presumed_map->logicalMap,x,y,true))
            {
                otherPlayer.presumed_direction=CatchChallenger::Direction_look_at_left;
                Tiled::Cell cell=otherPlayer.playerMapObject->cell();
                cell.tile=otherPlayer.playerTileset->tileAt(10);
                otherPlayer.playerMapObject->setCell(cell);
                otherPlayer.inMove=false;
                otherPlayer.oneStepMore->stop();
                std::cout << "[debug], can't go left" << std::endl;
            }
            //if can go, then do the move
            else
            {
                otherPlayer.presumed_direction=CatchChallenger::Direction_move_at_left;
                otherPlayer.moveStep=0;
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(otherPlayer.pendingMonsterMoves.size()>1)
                    abort();
                #endif
                moveOtherPlayerStepSlot();
            }
        }
        else if(otherPlayer.presumed_direction==CatchChallenger::Direction_move_at_right)
        {
            //can't go into this direction, then just look into this direction
            if(!CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_right,otherPlayer.presumed_map->logicalMap,x,y,true))
            {
                otherPlayer.presumed_direction=CatchChallenger::Direction_look_at_right;
                Tiled::Cell cell=otherPlayer.playerMapObject->cell();
                cell.tile=otherPlayer.playerTileset->tileAt(4);
                otherPlayer.playerMapObject->setCell(cell);
                otherPlayer.inMove=false;
                otherPlayer.oneStepMore->stop();
                std::cout << "[debug], can't go right" << std::endl;
            }
            //if can go, then do the move
            else
            {
                otherPlayer.presumed_direction=CatchChallenger::Direction_move_at_right;
                otherPlayer.moveStep=0;
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(otherPlayer.pendingMonsterMoves.size()>1)
                    abort();
                #endif
                moveOtherPlayerStepSlot();
            }
        }
        else if(otherPlayer.presumed_direction==CatchChallenger::Direction_move_at_top)
        {
            //can't go into this direction, then just look into this direction
            if(!CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_top,otherPlayer.presumed_map->logicalMap,x,y,true))
            {
                otherPlayer.presumed_direction=CatchChallenger::Direction_look_at_top;

                Tiled::Cell cell=otherPlayer.playerMapObject->cell();
                cell.tile=otherPlayer.playerTileset->tileAt(1);
                otherPlayer.playerMapObject->setCell(cell);
                otherPlayer.inMove=false;
                otherPlayer.oneStepMore->stop();
                std::cout << "[debug], can't go top" << std::endl;
            }
            //if can go, then do the move
            else
            {
                otherPlayer.presumed_direction=CatchChallenger::Direction_move_at_top;
                otherPlayer.moveStep=0;
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(otherPlayer.pendingMonsterMoves.size()>1)
                    abort();
                #endif
                moveOtherPlayerStepSlot();
            }
        }
        else if(otherPlayer.presumed_direction==CatchChallenger::Direction_move_at_bottom)
        {
            //can't go into this direction, then just look into this direction
            if(!CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_bottom,otherPlayer.presumed_map->logicalMap,x,y,true))
            {
                otherPlayer.presumed_direction=CatchChallenger::Direction_look_at_bottom;
                Tiled::Cell cell=otherPlayer.playerMapObject->cell();
                cell.tile=otherPlayer.playerTileset->tileAt(7);
                otherPlayer.playerMapObject->setCell(cell);
                otherPlayer.inMove=false;
                otherPlayer.oneStepMore->stop();
                std::cout << "[debug], can't go bottom" << std::endl;
            }
            //if can go, then do the move
            else
            {
                otherPlayer.presumed_direction=CatchChallenger::Direction_move_at_bottom;
                otherPlayer.moveStep=0;
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(otherPlayer.pendingMonsterMoves.size()>1)
                    abort();
                #endif
                moveOtherPlayerStepSlot();
            }
        }
        //now stop walking, no more arrow key is pressed
        else
        {
            otherPlayer.moveStep=0;
            otherPlayer.inMove=false;
            otherPlayer.oneStepMore->stop();
        }
        finalOtherPlayerStep(otherPlayer);

        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(otherPlayer.pendingMonsterMoves.size()>1)
            abort();
        #endif
    }
    else
        otherPlayer.oneStepMore->start();
}
