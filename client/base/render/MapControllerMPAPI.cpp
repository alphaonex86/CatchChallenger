#include "MapController.h"
#include "../DatapackClientLoader.h"
#include <iostream>

void MapControllerMP::insert_player(const CatchChallenger::Player_public_informations &player,const uint32_t &mapId,const uint16_t &x,const uint16_t &y,const CatchChallenger::Direction &direction)
{
    insert_player_final(player,mapId,x,y,direction,false);
}

void MapControllerMP::move_player(const uint16_t &id, const std::vector<std::pair<uint8_t,CatchChallenger::Direction> > &movement)
{
    move_player_final(id,movement,false);
}

void MapControllerMP::remove_player(const uint16_t &id)
{
    remove_player_final(id,false);
}

void MapControllerMP::reinsert_player(const uint16_t &id, const uint8_t &x, const uint8_t &y, const CatchChallenger::Direction &direction)
{
    reinsert_player_final(id,x,y,direction,false);
}

void MapControllerMP::full_reinsert_player(const uint16_t &id, const uint32_t &mapId, const uint8_t &x, const uint8_t &y, const CatchChallenger::Direction &direction)
{
    full_reinsert_player_final(id,mapId,x,y,direction,false);
}

void MapControllerMP::dropAllPlayerOnTheMap()
{
    dropAllPlayerOnTheMap_final(false);
}

//map move
bool MapControllerMP::insert_player_final(const CatchChallenger::Player_public_informations &player,
             const uint32_t &mapId,const uint16_t &x,const uint16_t &y,const CatchChallenger::Direction &direction,bool inReplayMode)
{
    if(!mHaveTheDatapack || !player_informations_is_set)
    {
        if(!inReplayMode)
        {
            DelayedInsert tempItem;
            tempItem.player=player;
            tempItem.mapId=mapId;
            tempItem.x=x;
            tempItem.y=y;
            tempItem.direction=direction;

            DelayedMultiplex multiplex;
            multiplex.insert=tempItem;
            multiplex.type=DelayedType_Insert;
            delayedActions.push_back(multiplex);
        }
        #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
        qDebug() << QStringLiteral("delayed: insert_player(%1->%2,%3,%4,%5,%6)").arg(player.pseudo).arg(player.simplifiedId).arg(mapId).arg(x).arg(y).arg(CatchChallenger::MoveOnTheMap::directionToString(direction));
        #endif
        return false;
    }
    if(mapId>=(uint32_t)DatapackClientLoader::datapackLoader.maps.size())
    {
        /// \bug here pass after delete a party, create a new
        emit error("mapId greater than DatapackClientLoader::datapackLoader.maps.size(): "+
                   std::to_string(DatapackClientLoader::datapackLoader.maps.size()));
        return true;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    qDebug() << QStringLiteral("insert_player(%1->%2,%3,%4,%5,%6)").arg(player.pseudo).arg(player.simplifiedId).arg(DatapackClientLoader::datapackLoader.maps.value(mapId)).arg(x).arg(y).arg(CatchChallenger::MoveOnTheMap::directionToString(direction));
    #endif
    //current player
    if(player.simplifiedId==player_informations.public_informations.simplifiedId)
        MapVisualiserPlayer::insert_player_internal(player,mapId,x,y,direction,skinFolderList);
    //other player
    else
    {
        if(otherPlayerList.find(player.simplifiedId)!=otherPlayerList.cend())
        {
            qDebug() << QStringLiteral("Other player (%1) already loaded on the map").arg(player.simplifiedId);
            //return true;-> ignored to fix temporally, but need remove at map unload
        }

        OtherPlayer tempPlayer;
        tempPlayer.playerMapObject=nullptr;
        tempPlayer.playerTileset=nullptr;
        tempPlayer.informations.monsterId=0;
        tempPlayer.informations.simplifiedId=0;
        tempPlayer.informations.skinId=0;
        tempPlayer.informations.speed=0;
        tempPlayer.informations.type=CatchChallenger::Player_type_normal;
        tempPlayer.labelMapObject=nullptr;
        tempPlayer.labelTileset=nullptr;
        tempPlayer.playerSpeed=0;
        tempPlayer.animationDisplayed=false;
        tempPlayer.monsterMapObject=nullptr;
        tempPlayer.monsterTileset=nullptr;
        tempPlayer.monster_x=0;
        tempPlayer.monster_y=0;
        tempPlayer.presumed_map=nullptr;
        tempPlayer.presumed_x=0;
        tempPlayer.presumed_y=0;

        tempPlayer.x=static_cast<uint8_t>(x);
        tempPlayer.y=static_cast<uint8_t>(y);
        tempPlayer.presumed_x=static_cast<uint8_t>(x);
        tempPlayer.presumed_y=static_cast<uint8_t>(y);
        tempPlayer.monster_x=static_cast<uint8_t>(x);
        tempPlayer.monster_y=static_cast<uint8_t>(y);
        tempPlayer.direction=direction;
        tempPlayer.moveStep=0;
        tempPlayer.inMove=false;
        tempPlayer.stepAlternance=false;

        const std::string &mapPath=QFileInfo(QString::fromStdString(datapackMapPathSpec+DatapackClientLoader::datapackLoader.maps.at(mapId)))
                .absoluteFilePath().toStdString();
        if(all_map.find(mapPath)==all_map.cend())
        {
            qDebug() << "MapControllerMP::insert_player(): current map " << QString::fromStdString(mapPath) << " not loaded, delayed: ";
            for (const auto &n : all_map)
                std::cout << n.first << std::endl;
            qDebug() << "List end";
            if(!inReplayMode)
            {
                DelayedInsert tempItem;
                tempItem.player=player;
                tempItem.mapId=mapId;
                tempItem.x=x;
                tempItem.y=y;
                tempItem.direction=direction;

                DelayedMultiplex multiplex;
                multiplex.insert=tempItem;
                multiplex.type=DelayedType_Insert;
                delayedActions.push_back(multiplex);
            }
            return false;
        }
        /// \todo do a player cache here
        //the player skin
        if(player.skinId<skinFolderList.size())
        {
            QImage image(QString::fromStdString(datapackPath+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(player.skinId)+"/trainer.png"));
            if(!image.isNull())
            {
                tempPlayer.playerMapObject = new Tiled::MapObject();
                tempPlayer.playerMapObject->setName("Other player");
                tempPlayer.playerTileset = new Tiled::Tileset(QString::fromStdString(skinFolderList.at(player.skinId)),16,24);
                if(!tempPlayer.playerTileset->loadFromImage(image,QString::fromStdString(datapackPath+
                     DATAPACK_BASE_PATH_SKIN+skinFolderList.at(player.skinId)+"/trainer.png")))
                    abort();
            }
            else
            {
                qDebug() << "Unable to load the player tilset: "+QString::fromStdString(datapackPath+DATAPACK_BASE_PATH_SKIN+
                                                   skinFolderList.at(player.skinId)+"/trainer.png");
                return true;
            }
        }
        else
        {
            qDebug() << QStringLiteral("The skin id: ")+QString::number(player.skinId)+QStringLiteral(", into a list of: ")+
                        QString::number(skinFolderList.size())+QStringLiteral(" item(s) info MapControllerMP::insert_player()");
            return true;
        }
        {
            QPixmap pix;
            if(!player.pseudo.empty())
            {
                QRectF destRect;
                {
                    QPixmap pix(50,10);
                    QPainter painter(&pix);
                    painter.setFont(playerpseudofont);
                    QRectF sourceRec(0.0,0.0,50.0,10.0);
                    destRect=painter.boundingRect(sourceRec,Qt::TextSingleLine,QString::fromStdString(player.pseudo));
                }
                int more=0;
                if(player.type!=CatchChallenger::Player_type_normal)
                    more=40;
                pix=QPixmap(destRect.width()+more,destRect.height());
                //draw the text & image
                {
                    pix.fill(Qt::transparent);
                    QPainter painter(&pix);
                    painter.setFont(playerpseudofont);
                    painter.drawText(QRectF(0.0,0.0,destRect.width(),destRect.height()),Qt::TextSingleLine,QString::fromStdString(player.pseudo));
                    if(player.type==CatchChallenger::Player_type_gm)
                    {
                        if(imgForPseudoAdmin==NULL)
                            imgForPseudoAdmin=new QPixmap(":/images/chat/admin.png");
                        painter.drawPixmap(destRect.width(),(destRect.height()-14)/2,40,14,*imgForPseudoAdmin);
                    }
                    if(player.type==CatchChallenger::Player_type_dev)
                    {
                        if(imgForPseudoDev==NULL)
                            imgForPseudoDev=new QPixmap(":/images/chat/developer.png");
                        painter.drawPixmap(destRect.width(),(destRect.height()-14)/2,40,14,*imgForPseudoDev);
                    }
                    if(player.type==CatchChallenger::Player_type_premium)
                    {
                        if(imgForPseudoPremium==NULL)
                            imgForPseudoPremium=new QPixmap(":/images/chat/premium.png");
                        painter.drawPixmap(destRect.width(),(destRect.height()-14)/2,40,14,*imgForPseudoPremium);
                    }
                }
            }
            else
            {
                if(player.type==CatchChallenger::Player_type_gm)
                {
                    if(imgForPseudoAdmin==NULL)
                        imgForPseudoAdmin=new QPixmap(":/images/chat/admin.png");
                    pix=*imgForPseudoAdmin;
                }
                if(player.type==CatchChallenger::Player_type_dev)
                {
                    if(imgForPseudoDev==NULL)
                        imgForPseudoDev=new QPixmap(":/images/chat/developer.png");
                    pix=*imgForPseudoDev;
                }
                if(player.type==CatchChallenger::Player_type_premium)
                {
                    if(imgForPseudoPremium==NULL)
                        imgForPseudoPremium=new QPixmap(":/images/chat/premium.png");
                    pix=*imgForPseudoPremium;
                }
            }
            if(!pix.isNull())
            {
                tempPlayer.labelMapObject = new Tiled::MapObject();
                tempPlayer.labelMapObject->setName("labelMapObject");
                tempPlayer.labelTileset = new Tiled::Tileset(QString(),pix.width(),pix.height());
                tempPlayer.labelTileset->addTile(pix);
                Tiled::Cell cell=tempPlayer.labelMapObject->cell();
                cell.tile=tempPlayer.labelTileset->tileAt(0);
                tempPlayer.labelMapObject->setCell(cell);
            }
            else
            {
                tempPlayer.labelMapObject=NULL;
                tempPlayer.labelTileset=NULL;
            }
        }

        tempPlayer.current_map=mapPath;
        tempPlayer.presumed_map=all_map.at(mapPath);
        tempPlayer.current_monster_map=mapPath;

        switch(direction)
        {
            case CatchChallenger::Direction_look_at_top:
            case CatchChallenger::Direction_move_at_top:
            {
                Tiled::Cell cell=tempPlayer.playerMapObject->cell();
                cell.tile=tempPlayer.playerTileset->tileAt(1);
                tempPlayer.playerMapObject->setCell(cell);
            }
            break;
            case CatchChallenger::Direction_look_at_right:
            case CatchChallenger::Direction_move_at_right:
            {
                Tiled::Cell cell=tempPlayer.playerMapObject->cell();
                cell.tile=tempPlayer.playerTileset->tileAt(4);
                tempPlayer.playerMapObject->setCell(cell);
            }
            break;
            case CatchChallenger::Direction_look_at_bottom:
            case CatchChallenger::Direction_move_at_bottom:
            {
                Tiled::Cell cell=tempPlayer.playerMapObject->cell();
                cell.tile=tempPlayer.playerTileset->tileAt(7);
                tempPlayer.playerMapObject->setCell(cell);
            }
            break;
            case CatchChallenger::Direction_look_at_left:
            case CatchChallenger::Direction_move_at_left:
            {
                Tiled::Cell cell=tempPlayer.playerMapObject->cell();
                cell.tile=tempPlayer.playerTileset->tileAt(10);
                tempPlayer.playerMapObject->setCell(cell);
            }
            break;
            default:
                delete tempPlayer.playerMapObject;
                delete tempPlayer.playerTileset;
                qDebug() << QStringLiteral("The direction send by the server is wrong");
            return true;
        }

        updateOtherPlayerMonsterTile(tempPlayer,player.monsterId);
        loadOtherPlayerFromMap(tempPlayer,false);

        tempPlayer.animationDisplayed=false;
        tempPlayer.informations=player;
        tempPlayer.oneStepMore=new QTimer();
        tempPlayer.oneStepMore->setSingleShot(true);
        tempPlayer.moveAnimationTimer=new QTimer();
        tempPlayer.moveAnimationTimer->setSingleShot(true);
        tempPlayer.playerSpeed=player.speed;
        otherPlayerListByTimer[tempPlayer.oneStepMore]=player.simplifiedId;
        otherPlayerListByAnimationTimer[tempPlayer.moveAnimationTimer]=player.simplifiedId;
        connect(tempPlayer.oneStepMore,&QTimer::timeout,this,&MapControllerMP::moveOtherPlayerStepSlot);
        connect(tempPlayer.moveAnimationTimer,&QTimer::timeout,this,&MapControllerMP::doMoveOtherAnimation);
        otherPlayerList[player.simplifiedId]=tempPlayer;

        switch(direction)
        {
            case CatchChallenger::Direction_move_at_top:
            case CatchChallenger::Direction_move_at_right:
            case CatchChallenger::Direction_move_at_bottom:
            case CatchChallenger::Direction_move_at_left:
            {
                std::vector<std::pair<uint8_t, CatchChallenger::Direction> > movement;
                std::pair<uint8_t, CatchChallenger::Direction> move;
                move.first=0;
                move.second=direction;
                movement.push_back(move);
                move_player_final(player.simplifiedId,movement,inReplayMode);
            }
            break;
            default:
            break;
        }
        finalOtherPlayerStep(tempPlayer);
        return true;
    }
    return true;
}

bool MapControllerMP::move_player_final(const uint16_t &id, const std::vector<std::pair<uint8_t, CatchChallenger::Direction> > &movement,bool inReplayMode)
{
    if(!mHaveTheDatapack || !player_informations_is_set)
    {
        if(!inReplayMode)
        {
            DelayedMove tempItem;
            tempItem.id=id;
            tempItem.movement=movement;

            DelayedMultiplex multiplex;
            multiplex.move=tempItem;
            multiplex.type=DelayedType_Move;
            delayedActions.push_back(multiplex);
        }
        return false;
    }
    if(id==player_informations.public_informations.simplifiedId)
    {
        qDebug() << "The current player can't be moved (only teleported)";
        return true;
    }
    if(otherPlayerList.find(id)==otherPlayerList.cend())
    {
        qDebug() << QStringLiteral("Other player (%1) not loaded on the map").arg(id);
        return true;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    QStringList moveString;
    int index_temp=0;
    while(index_temp<movement.size())
    {
        std::pair<uint8_t, CatchChallenger::Direction> move=movement.at(index_temp);
        moveString << QStringLiteral("{%1,%2}").arg(move.first).arg(CatchChallenger::MoveOnTheMap::directionToString(move.second));
        index_temp++;
    }

    qDebug() << QStringLiteral("move_player(%1,%2), previous direction: %3").arg(id).arg(moveString.join(";")).arg(CatchChallenger::MoveOnTheMap::directionToString(otherPlayerList.value(id).direction));
    #endif


    //reset to the good position
    OtherPlayer &otherPlayer=otherPlayerList[id];
    otherPlayer.oneStepMore->stop();
    otherPlayer.inMove=false;
    otherPlayer.moveStep=0;
    if(otherPlayer.current_map!=otherPlayer.presumed_map->logicalMap.map_file)
    {
        unloadOtherPlayerFromMap(otherPlayer);
        std::string mapPath=otherPlayer.current_map;
        if(!haveMapInMemory(mapPath))
        {
            qDebug() << QStringLiteral("move_player(%1), map not already loaded").arg(id).arg(QString::fromStdString(otherPlayer.current_map));
            if(!inReplayMode)
            {
                DelayedMove tempItem;
                tempItem.id=id;
                tempItem.movement=movement;

                DelayedMultiplex multiplex;
                multiplex.move=tempItem;
                multiplex.type=DelayedType_Move;
                delayedActions.push_back(multiplex);
            }
            return false;
        }
        loadOtherMap(mapPath);
        otherPlayer.presumed_map=all_map.at(mapPath);
        loadOtherPlayerFromMap(otherPlayer);
    }
    uint8_t x=otherPlayer.x;
    uint8_t y=otherPlayer.y;
    otherPlayer.presumed_x=x;
    otherPlayer.presumed_y=y;
    otherPlayer.presumed_direction=otherPlayer.direction;
    CatchChallenger::CommonMap * map=&otherPlayer.presumed_map->logicalMap;
    CatchChallenger::CommonMap * old_map;

    //move to have the new position if needed
    unsigned int index=0;
    while(index<movement.size())
    {
        std::pair<uint8_t, CatchChallenger::Direction> move=movement.at(index);
        int index2=0;
        while(index2<move.first)
        {
            old_map=map;
            //set the final value (direction, position, ...)
            switch(otherPlayer.presumed_direction)
            {
                case CatchChallenger::Direction_move_at_left:
                case CatchChallenger::Direction_move_at_right:
                case CatchChallenger::Direction_move_at_top:
                case CatchChallenger::Direction_move_at_bottom:
                if(CatchChallenger::MoveOnTheMap::canGoTo(otherPlayer.presumed_direction,*map,x,y,true))
                    CatchChallenger::MoveOnTheMap::move(otherPlayer.presumed_direction,&map,&x,&y);
                else
                {
                    qDebug() << QStringLiteral("move_player(): at %1(%2,%3) can't go to %4")
                                .arg(QString::fromStdString(map->map_file)).arg(x).arg(y)
                                .arg(QString::fromStdString(
                                         CatchChallenger::MoveOnTheMap::directionToString(
                                             otherPlayer.presumed_direction)));
                    return true;
                }
                break;
                default:
                qDebug() << QStringLiteral("move_player(): moveStep: %1, wrong direction: %2")
                            .arg(move.first)
                            .arg(QString::fromStdString(
                                     CatchChallenger::MoveOnTheMap::directionToString(
                                         otherPlayer.presumed_direction)));
                return true;
            }
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
                }
            }
            index2++;
        }
        otherPlayer.direction=move.second;
        index++;
    }


    //set the new variables
    otherPlayer.current_map=map->map_file;
    otherPlayer.x=x;
    otherPlayer.y=y;

    otherPlayer.presumed_map=all_map.at(otherPlayer.current_map);
    otherPlayer.presumed_x=otherPlayer.x;
    otherPlayer.presumed_y=otherPlayer.y;
    otherPlayer.presumed_direction=otherPlayer.direction;

    //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
    otherPlayer.playerMapObject->setPosition(QPoint(otherPlayer.presumed_x,otherPlayer.presumed_y+1));
    if(otherPlayer.labelMapObject!=NULL)
    {
        otherPlayerList.at(id).labelMapObject->setPosition(QPointF(static_cast<qreal>(otherPlayer.presumed_x)-
             static_cast<qreal>(otherPlayer.labelTileset->tileWidth())/2/16+0.5,
             static_cast<qreal>(otherPlayer.presumed_y)+1-1.4));
        MapObjectItem::objectLink.at(otherPlayer.labelMapObject)->setZValue(otherPlayer.presumed_y);
    }
    MapObjectItem::objectLink.at(otherPlayer.playerMapObject)->setZValue(otherPlayer.presumed_y);

    //start moving into the right direction
    switch(otherPlayer.presumed_direction)
    {
        case CatchChallenger::Direction_look_at_top:
        case CatchChallenger::Direction_move_at_top:
        {
            Tiled::Cell cell=otherPlayer.playerMapObject->cell();
            cell.tile=otherPlayer.playerTileset->tileAt(1);
            otherPlayer.playerMapObject->setCell(cell);
        }
        break;
        case CatchChallenger::Direction_look_at_right:
        case CatchChallenger::Direction_move_at_right:
        {
            Tiled::Cell cell=otherPlayer.playerMapObject->cell();
            cell.tile=otherPlayer.playerTileset->tileAt(4);
            otherPlayer.playerMapObject->setCell(cell);
        }
        break;
        case CatchChallenger::Direction_look_at_bottom:
        case CatchChallenger::Direction_move_at_bottom:
        {
            Tiled::Cell cell=otherPlayer.playerMapObject->cell();
            cell.tile=otherPlayer.playerTileset->tileAt(7);
            otherPlayer.playerMapObject->setCell(cell);
        }
        break;
        case CatchChallenger::Direction_look_at_left:
        case CatchChallenger::Direction_move_at_left:
        {
            Tiled::Cell cell=otherPlayer.playerMapObject->cell();
            cell.tile=otherPlayer.playerTileset->tileAt(10);
            otherPlayer.playerMapObject->setCell(cell);
        }
        break;
        default:
            qDebug() << QStringLiteral("move_player(): player: %1 (%2), wrong direction: %3")
                        .arg(QString::fromStdString(otherPlayer.informations.pseudo))
                        .arg(id).arg(otherPlayer.presumed_direction);
        return true;
    }
    switch(otherPlayer.presumed_direction)
    {
        case CatchChallenger::Direction_move_at_top:
        case CatchChallenger::Direction_move_at_right:
        case CatchChallenger::Direction_move_at_bottom:
        case CatchChallenger::Direction_move_at_left:
            otherPlayer.oneStepMore->start(otherPlayer.informations.speed/5);
        break;
        default:
        break;
    }
    finalOtherPlayerStep(otherPlayer);
    return true;
}

bool MapControllerMP::remove_player_final(const uint16_t &id,bool inReplayMode)
{
    if(id==player_informations.public_informations.simplifiedId)
    {
        qDebug() << "The current player can't be removed";
        return true;
    }
    if(!mHaveTheDatapack || !player_informations_is_set)
    {
        #ifdef DEBUG_CLIENT_LOAD_ORDER
        qDebug() << QStringLiteral("delayed: MapControllerMP::remove_player(%1)").arg(id);
        #endif
        if(!inReplayMode)
        {
            DelayedMultiplex multiplex;
            multiplex.remove=id;
            multiplex.type=DelayedType_Remove;
            delayedActions.push_back(multiplex);
        }
        return false;
    }
    if(otherPlayerList.find(id)==otherPlayerList.cend())
    {
        qDebug() << QStringLiteral("Other player (%1) not exists").arg(id);
        return true;
    }
    {
        unsigned int index=0;
        while(index<delayedActions.size())
        {
            switch(delayedActions.at(index).type)
            {
                case DelayedType_Insert:
                    if(delayedActions.at(index).insert.player.simplifiedId==id)
                        delayedActions.erase(delayedActions.cbegin()+index);
                    else
                        index++;
                break;
                case DelayedType_Move:
                    if(delayedActions.at(index).move.id==id)
                        delayedActions.erase(delayedActions.cbegin()+index);
                    else
                        index++;
                break;
                default:
                    index++;
                break;
            }
        }
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    qDebug() << QStringLiteral("remove_player(%1)").arg(id);
    #endif
    const OtherPlayer &otherPlayer=otherPlayerList.at(id);
    unloadOtherPlayerFromMap(otherPlayer);

    otherPlayerListByTimer.erase(otherPlayer.oneStepMore);
    otherPlayerListByAnimationTimer.erase(otherPlayer.moveAnimationTimer);

    Tiled::ObjectGroup *currentGroup=otherPlayer.playerMapObject->objectGroup();
    if(currentGroup!=NULL)
    {
        if(ObjectGroupItem::objectGroupLink.find(currentGroup)!=ObjectGroupItem::objectGroupLink.cend())
        {
            ObjectGroupItem::objectGroupLink.at(currentGroup)->removeObject(otherPlayer.playerMapObject);
            if(otherPlayer.labelMapObject!=NULL)
                ObjectGroupItem::objectGroupLink.at(currentGroup)->removeObject(otherPlayer.labelMapObject);
        }
        if(currentGroup!=otherPlayer.presumed_map->objectGroup)
            qDebug() << QStringLiteral("loadOtherPlayerFromMap(), the playerMapObject group is wrong: %1").arg(currentGroup->name());
        currentGroup->removeObject(otherPlayer.playerMapObject);
        if(otherPlayer.labelMapObject!=NULL)
            currentGroup->removeObject(otherPlayer.labelMapObject);
    }

    /*delete otherPlayer.playerMapObject;
    delete otherPlayer.playerTileset;*/
    delete otherPlayer.oneStepMore;
    delete otherPlayer.moveAnimationTimer;
    if(otherPlayer.labelMapObject!=NULL)
        delete otherPlayer.labelMapObject;
    if(otherPlayer.labelTileset!=NULL)
        delete otherPlayer.labelTileset;

    otherPlayerList.erase(id);
    return true;
}

bool MapControllerMP::reinsert_player_final(const uint16_t &id,const uint8_t &x,const uint8_t &y,
                                            const CatchChallenger::Direction &direction,bool inReplayMode)
{
    if(!mHaveTheDatapack || !player_informations_is_set)
    {
        #ifdef DEBUG_CLIENT_LOAD_ORDER
        qDebug() << QStringLiteral("delayed: MapControllerMP::reinsert_player(%1)").arg(id);
        #endif
        if(!inReplayMode)
        {
            DelayedReinsertSingle tempItem;
            tempItem.id=id;
            tempItem.x=x;
            tempItem.y=y;
            tempItem.direction=direction;

            DelayedMultiplex multiplex;
            multiplex.reinsert_single=tempItem;
            multiplex.type=DelayedType_Reinsert_single;
            delayedActions.push_back(multiplex);
        }
        return false;
    }
    if(id==player_informations.public_informations.simplifiedId)
    {
        qDebug() << "The current player can't be removed";
        return true;
    }
    if(otherPlayerList.find(id)==otherPlayerList.cend())
    {
        qDebug() << QStringLiteral("Other player (%1) not exists").arg(id);
        return true;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    qDebug() << QStringLiteral("reinsert_player(%1)").arg(id);
    #endif

    CatchChallenger::Player_public_informations informations=otherPlayerList.at(id).informations;
    /// \warning search by loop because otherPlayerList.value(id).current_map is the full path, DatapackClientLoader::datapackLoader.maps relative path
    std::string tempCurrentMap=otherPlayerList.at(id).current_map;
    //if not found, search into sub
    if(all_map.find(tempCurrentMap)==all_map.cend() && !client->subDatapackCode().empty())
    {
        std::string tempCurrentMapSub=tempCurrentMap;
        stringreplaceOne(tempCurrentMapSub,client->datapackPathSub(),"");
        if(all_map.find(tempCurrentMapSub)!=all_map.cend())
            tempCurrentMap=tempCurrentMapSub;
    }
    //if not found, search into main
    if(all_map.find(tempCurrentMap)==all_map.cend())
    {
        std::string tempCurrentMapMain=tempCurrentMap;
        stringreplaceOne(tempCurrentMapMain,client->datapackPathMain(),"");
        if(all_map.find(tempCurrentMapMain)!=all_map.cend())
            tempCurrentMap=tempCurrentMapMain;
    }
    //if remain not found
    if(all_map.find(tempCurrentMap)==all_map.cend())
    {
        qDebug() << "internal problem, revert map (" << QString::fromStdString(otherPlayerList.at(id).current_map)
                 << ") index is wrong (" << QString::fromStdString(stringimplode(DatapackClientLoader::datapackLoader.maps,";")) << ")";
        if(!inReplayMode)
        {
            DelayedReinsertSingle tempItem;
            tempItem.id=id;
            tempItem.x=x;
            tempItem.y=y;
            tempItem.direction=direction;

            DelayedMultiplex multiplex;
            multiplex.reinsert_single=tempItem;
            multiplex.type=DelayedType_Reinsert_single;
            delayedActions.push_back(multiplex);
        }
        return false;
    }
    const uint32_t mapId=(uint32_t)all_map.at(tempCurrentMap)->logicalMap.id;
    if(mapId==0)
        qDebug() << QStringLiteral("supected NULL map then error");

    OtherPlayer &tempPlayer=otherPlayerList[id];
    if(tempPlayer.x==x || tempPlayer.y==y)
    {
        /// \warning no path finding because too many player update can overflow the cpu
        int moveOffset=0;
        if(tempPlayer.x==x)
            moveOffset=abs((int)y-(int)tempPlayer.y);
        else if(tempPlayer.y==y)
            moveOffset=abs((int)x-(int)tempPlayer.x);
        else
            abort();

        std::vector<std::pair<uint8_t, CatchChallenger::Direction> > movement;
        std::pair<uint8_t, CatchChallenger::Direction> t(moveOffset,direction);
        movement.push_back(t);
        move_player_final(id,movement,false);
    }
    else
    {
        tempPlayer.presumed_x=static_cast<uint8_t>(x);
        tempPlayer.presumed_y=static_cast<uint8_t>(y);

        //monster
        tempPlayer.monster_x=static_cast<uint8_t>(x);
        tempPlayer.monster_y=static_cast<uint8_t>(y);

        switch(direction)
        {
            case CatchChallenger::Direction_look_at_top:
            case CatchChallenger::Direction_move_at_top:
            {
                Tiled::Cell cell=tempPlayer.playerMapObject->cell();
                cell.tile=tempPlayer.playerTileset->tileAt(1);
                tempPlayer.playerMapObject->setCell(cell);
            }
            break;
            case CatchChallenger::Direction_look_at_right:
            case CatchChallenger::Direction_move_at_right:
            {
                Tiled::Cell cell=tempPlayer.playerMapObject->cell();
                cell.tile=tempPlayer.playerTileset->tileAt(4);
                tempPlayer.playerMapObject->setCell(cell);
            }
            break;
            case CatchChallenger::Direction_look_at_bottom:
            case CatchChallenger::Direction_move_at_bottom:
            {
                Tiled::Cell cell=tempPlayer.playerMapObject->cell();
                cell.tile=tempPlayer.playerTileset->tileAt(7);
                tempPlayer.playerMapObject->setCell(cell);
            }
            break;
            case CatchChallenger::Direction_look_at_left:
            case CatchChallenger::Direction_move_at_left:
            {
                Tiled::Cell cell=tempPlayer.playerMapObject->cell();
                cell.tile=tempPlayer.playerTileset->tileAt(10);
                tempPlayer.playerMapObject->setCell(cell);
            }
            break;
            default:
                delete tempPlayer.playerMapObject;
                delete tempPlayer.playerTileset;
                qDebug() << QStringLiteral("The direction send by the server is wrong");
            return true;
        }

        switch(direction)
        {
            case CatchChallenger::Direction_move_at_top:
            case CatchChallenger::Direction_move_at_right:
            case CatchChallenger::Direction_move_at_bottom:
            case CatchChallenger::Direction_move_at_left:
            {
                std::vector<std::pair<uint8_t, CatchChallenger::Direction> > movement;
                std::pair<uint8_t, CatchChallenger::Direction> move;
                move.first=0;
                move.second=direction;
                movement.push_back(move);
                move_player_final(id,movement,inReplayMode);
            }
            break;
            default:
            break;
        }
        finalOtherPlayerStep(tempPlayer);

        if(tempPlayer.playerMapObject!=NULL)
        {
            tempPlayer.playerMapObject->setPosition(QPointF(tempPlayer.x-0.5,tempPlayer.y+1));
            MapObjectItem::objectLink.at(tempPlayer.playerMapObject)->setZValue(tempPlayer.y);
        }
        if(tempPlayer.monsterMapObject!=NULL)
        {
            tempPlayer.monsterMapObject->setPosition(QPointF(tempPlayer.x-0.5,tempPlayer.y+1));
            MapObjectItem::objectLink.at(tempPlayer.monsterMapObject)->setZValue(tempPlayer.y);
        }
    }

    return true;
}

bool MapControllerMP::full_reinsert_player_final(const uint16_t &id,const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction,bool inReplayMode)
{
    if(!mHaveTheDatapack || !player_informations_is_set)
    {
        #ifdef DEBUG_CLIENT_LOAD_ORDER
        qDebug() << QStringLiteral("delayed: MapControllerMP::reinsert_player(%1)").arg(id);
        #endif
        if(!inReplayMode)
        {
            DelayedReinsertFull tempItem;
            tempItem.id=id;
            tempItem.mapId=mapId;
            tempItem.x=x;
            tempItem.y=y;
            tempItem.direction=direction;

            DelayedMultiplex multiplex;
            multiplex.reinsert_full=tempItem;
            multiplex.type=DelayedType_Reinsert_full;
            delayedActions.push_back(multiplex);
        }
        return false;
    }
    if(id==player_informations.public_informations.simplifiedId)
    {
        qDebug() << "The current player can't be removed";
        return true;
    }
    if(otherPlayerList.find(id)==otherPlayerList.cend())
    {
        qDebug() << QStringLiteral("Other player (%1) not exists").arg(id);
        return true;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    qDebug() << QStringLiteral("reinsert_player(%1)").arg(id);
    #endif

    CatchChallenger::Player_public_informations informations=otherPlayerList.at(id).informations;
    remove_player_final(id,inReplayMode);
    insert_player_final(informations,mapId,x,y,direction,inReplayMode);
    return true;
}

bool MapControllerMP::dropAllPlayerOnTheMap_final(bool inReplayMode)
{
    if(!mHaveTheDatapack || !player_informations_is_set)
    {
        #ifdef DEBUG_CLIENT_LOAD_ORDER
        qDebug() << QStringLiteral("delayed: MapControllerMP::dropAllPlayerOnTheMap()");
        #endif
        if(!inReplayMode)
        {
            DelayedMultiplex multiplex;
            multiplex.type=DelayedType_Drop_all;
            delayedActions.push_back(multiplex);
        }
        return false;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    qDebug() << QStringLiteral("dropAllPlayerOnTheMap()");
    #endif
    std::vector<uint16_t> temIdList;
    for (const auto &n : otherPlayerList)
        temIdList.push_back(n.first);
    unsigned int index=0;
    while(index<temIdList.size())
    {
        remove_player_final(temIdList.at(index),inReplayMode);
        index++;
    }
    return true;
}
