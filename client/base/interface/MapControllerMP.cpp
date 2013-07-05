#include "MapController.h"
#include "../../general/base/MoveOnTheMap.h"
#include "../../general/base/FacilityLib.h"
#include "DatapackClientLoader.h"
#include "../ClientVariable.h"
#include "../Api_client_real.h"

#include <QMessageBox>
#include <QMessageBox>
#include <qmath.h>

MapControllerMP::MapControllerMP(const bool &centerOnPlayer,const bool &debugTags,const bool &useCache,const bool &OpenGL) :
    MapVisualiserPlayerWithFight(centerOnPlayer,debugTags,useCache,OpenGL)
{
    qRegisterMetaType<CatchChallenger::Direction>("CatchChallenger::Direction");
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_public_informations>("CatchChallenger::Player_public_informations");
    qRegisterMetaType<CatchChallenger::Player_private_and_public_informations>("CatchChallenger::Player_private_and_public_informations");
    qRegisterMetaType<QList<QPair<quint8,CatchChallenger::Direction> > >("QList<QPair<quint8,CatchChallenger::Direction> >");

    player_informations_is_set=false;

    resetAll();

    //connect the map controler
    connect(CatchChallenger::Api_client_real::client,SIGNAL(have_current_player_info(CatchChallenger::Player_private_and_public_informations)),this,SLOT(have_current_player_info(CatchChallenger::Player_private_and_public_informations)),Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,SIGNAL(insert_player(CatchChallenger::Player_public_informations,quint32,quint16,quint16,CatchChallenger::Direction)),this,SLOT(insert_player(CatchChallenger::Player_public_informations,quint32,quint16,quint16,CatchChallenger::Direction)),Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,SIGNAL(remove_player(quint16)),this,SLOT(remove_player(quint16)),Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,SIGNAL(move_player(quint16,QList<QPair<quint8,CatchChallenger::Direction> >)),this,SLOT(move_player(quint16,QList<QPair<quint8,CatchChallenger::Direction> >)),Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,SIGNAL(reinsert_player(quint16,quint8,quint8,CatchChallenger::Direction)),this,SLOT(reinsert_player(quint16,quint8,quint8,CatchChallenger::Direction)),Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,SIGNAL(reinsert_player(quint16,quint32,quint8,quint8,CatchChallenger::Direction)),this,SLOT(reinsert_player(quint16,quint32,quint8,quint8,CatchChallenger::Direction)),Qt::QueuedConnection);
    connect(this,SIGNAL(send_player_direction(CatchChallenger::Direction)),CatchChallenger::Api_client_real::client,SLOT(send_player_direction(CatchChallenger::Direction)),Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,SIGNAL(teleportTo(quint32,quint16,quint16,CatchChallenger::Direction)),this,SLOT(teleportTo(quint32,quint16,quint16,CatchChallenger::Direction)),Qt::QueuedConnection);

    scaleSize=1;
}

MapControllerMP::~MapControllerMP()
{
}

void MapControllerMP::resetAll()
{
    if(!playerTileset->loadFromImage(QImage(":/images/player_default/trainer.png"),":/images/player_default/trainer.png"))
        qDebug() << "Unable the load the default player tileset";

    unloadPlayerFromCurrentMap();
    current_map.clear();

    delayedActions.clear();
    skinFolderList.clear();

    QHashIterator<quint16,OtherPlayer> i(otherPlayerList);
    while (i.hasNext()) {
        i.next();
        unloadOtherPlayerFromMap(i.value());
        delete i.value().playerTileset;
    }
    otherPlayerList.clear();
    otherPlayerListByTimer.clear();
    mapUsedByOtherPlayer.clear();

    mHaveTheDatapack=false;
    player_informations_is_set=false;

    MapVisualiserPlayer::resetAll();
}

void MapControllerMP::setScale(const float &scaleSize)
{
    scale(scaleSize/this->scaleSize,scaleSize/this->scaleSize);
    this->scaleSize=scaleSize;
}

bool MapControllerMP::loadPlayerMap(const QString &fileName,const quint8 &x,const quint8 &y)
{
    //position
    this->x=x;
    this->y=y;
    QFileInfo fileInformations(fileName);
    current_map=fileInformations.absoluteFilePath();
    loadOtherMap(current_map);
    return true;
}

//map move
void MapControllerMP::insert_player(const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction)
{
    if(!mHaveTheDatapack || !player_informations_is_set)
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
        delayedActions << multiplex;
        #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
        qDebug() << QString("delayed: insert_player(%1->%2,%3,%4,%5,%6)").arg(player.pseudo).arg(player.simplifiedId).arg(mapId).arg(x).arg(y).arg(CatchChallenger::MoveOnTheMap::directionToString(direction));
        #endif
        return;
    }
    if(mapId>=(quint32)DatapackClientLoader::datapackLoader.maps.size())
    {
        /// \bug here pass after delete a party, create a new
        emit error("mapId greater than DatapackClientLoader::datapackLoader.maps.size(): "+QString::number(DatapackClientLoader::datapackLoader.maps.size()));
        return;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    qDebug() << QString("insert_player(%1->%2,%3,%4,%5,%6)").arg(player.pseudo).arg(player.simplifiedId).arg(DatapackClientLoader::datapackLoader.maps[mapId]).arg(x).arg(y).arg(CatchChallenger::MoveOnTheMap::directionToString(direction));
    #endif
    if(player.simplifiedId==player_informations.public_informations.simplifiedId)
    {
        if(current_map!=NULL)
        {
            qDebug() << "Current player already loaded on the map";
            return;
        }
        //the player skin
        if(player.skinId<skinFolderList.size())
        {
            QImage image(datapackPath+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(player.skinId)+"/trainer.png");
            if(!image.isNull())
                playerTileset->loadFromImage(image,datapackPath+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(player.skinId)+"/trainer.png");
            else
                qDebug() << "Unable to load the player tilset: "+datapackPath+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(player.skinId)+"/trainer.png";
        }
        else
            qDebug() << "The skin id: "+QString::number(player.skinId)+", into a list of: "+QString::number(skinFolderList.size())+" item(s) info MapControllerMP::insert_player()";

        //the direction
        this->direction=direction;
        switch(direction)
        {
            case CatchChallenger::Direction_look_at_top:
            case CatchChallenger::Direction_move_at_top:
                playerMapObject->setTile(playerTileset->tileAt(1));
            break;
            case CatchChallenger::Direction_look_at_right:
            case CatchChallenger::Direction_move_at_right:
                playerMapObject->setTile(playerTileset->tileAt(4));
            break;
            case CatchChallenger::Direction_look_at_bottom:
            case CatchChallenger::Direction_move_at_bottom:
                playerMapObject->setTile(playerTileset->tileAt(7));
            break;
            case CatchChallenger::Direction_look_at_left:
            case CatchChallenger::Direction_move_at_left:
                playerMapObject->setTile(playerTileset->tileAt(10));
            break;
            default:
            QMessageBox::critical(NULL,tr("Internal error"),tr("The direction send by the server is wrong"));
            return;
        }

        loadPlayerMap(datapackMapPath+DatapackClientLoader::datapackLoader.maps[mapId],x,y);
        setSpeed(player.speed);
    }
    else
    {
        if(otherPlayerList.contains(player.simplifiedId))
        {
            qDebug() << QString("Other player (%1) already loaded on the map").arg(player.simplifiedId);
            return;
        }
        OtherPlayer tempPlayer;
        tempPlayer.x=x;
        tempPlayer.y=y;
        tempPlayer.direction=direction;
        tempPlayer.moveStep=0;
        tempPlayer.inMove=false;
        tempPlayer.stepAlternance=false;

        QString mapPath=QFileInfo(datapackMapPath+DatapackClientLoader::datapackLoader.maps[mapId]).absoluteFilePath();
        if(!all_map.contains(mapPath))
        {
            qDebug() << "MapControllerMP::insert_player(): current map " << mapPath << " not loaded";
            return;
        }
        //the player skin
        if(player.skinId<skinFolderList.size())
        {
            QImage image(datapackPath+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(player.skinId)+"/trainer.png");
            if(!image.isNull())
            {
                tempPlayer.playerMapObject = new Tiled::MapObject();
                tempPlayer.playerTileset = new Tiled::Tileset(skinFolderList.at(player.skinId),16,24);
                tempPlayer.playerTileset->loadFromImage(image,datapackPath+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(player.skinId)+"/trainer.png");
            }
            else
            {
                qDebug() << "Unable to load the player tilset: "+datapackPath+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(player.skinId)+"/trainer.png";
                return;
            }
        }
        else
        {
            qDebug() << "The skin id: "+QString::number(player.skinId)+", into a list of: "+QString::number(skinFolderList.size())+" item(s) info MapControllerMP::insert_player()";
            return;
        }
        tempPlayer.current_map=mapPath;
        tempPlayer.presumed_map=all_map[mapPath];
        tempPlayer.presumed_x=x;
        tempPlayer.presumed_y=y;
        switch(direction)
        {
            case CatchChallenger::Direction_look_at_top:
            case CatchChallenger::Direction_move_at_top:
                tempPlayer.playerMapObject->setTile(tempPlayer.playerTileset->tileAt(1));
            break;
            case CatchChallenger::Direction_look_at_right:
            case CatchChallenger::Direction_move_at_right:
                tempPlayer.playerMapObject->setTile(tempPlayer.playerTileset->tileAt(4));
            break;
            case CatchChallenger::Direction_look_at_bottom:
            case CatchChallenger::Direction_move_at_bottom:
                tempPlayer.playerMapObject->setTile(tempPlayer.playerTileset->tileAt(7));
            break;
            case CatchChallenger::Direction_look_at_left:
            case CatchChallenger::Direction_move_at_left:
                tempPlayer.playerMapObject->setTile(tempPlayer.playerTileset->tileAt(10));
            break;
            default:
                delete tempPlayer.playerMapObject;
                delete tempPlayer.playerTileset;
                qDebug() << "The direction send by the server is wrong";
            return;
        }

        loadOtherPlayerFromMap(tempPlayer,false);

        tempPlayer.informations=player;
        tempPlayer.oneStepMore=new QTimer();
        tempPlayer.oneStepMore->setSingleShot(true);
        otherPlayerListByTimer[tempPlayer.oneStepMore]=player.simplifiedId;
        connect(tempPlayer.oneStepMore,SIGNAL(timeout()),this,SLOT(moveOtherPlayerStepSlot()));
        otherPlayerList[player.simplifiedId]=tempPlayer;

        switch(direction)
        {
            case CatchChallenger::Direction_move_at_top:
            case CatchChallenger::Direction_move_at_right:
            case CatchChallenger::Direction_move_at_bottom:
            case CatchChallenger::Direction_move_at_left:
            {
                QList<QPair<quint8, CatchChallenger::Direction> > movement;
                QPair<quint8, CatchChallenger::Direction> move;
                move.first=0;
                move.second=direction;
                movement << move;
                move_player(player.simplifiedId,movement);
            }
            break;
            default:
            break;
        }
        return;
    }
}

//call after enter on new map
void MapControllerMP::loadOtherPlayerFromMap(OtherPlayer otherPlayer,const bool &display)
{
    Q_UNUSED(display);
    //remove the player tile if needed
    Tiled::ObjectGroup *currentGroup=otherPlayer.playerMapObject->objectGroup();
    if(currentGroup!=NULL)
    {
        if(ObjectGroupItem::objectGroupLink.contains(currentGroup))
            ObjectGroupItem::objectGroupLink[currentGroup]->removeObject(otherPlayer.playerMapObject);
        if(currentGroup!=otherPlayer.presumed_map->objectGroup)
            qDebug() << QString("loadOtherPlayerFromMap(), the playerMapObject group is wrong: %1").arg(currentGroup->name());
    }

    //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
    otherPlayer.playerMapObject->setPosition(QPoint(otherPlayer.x,otherPlayer.y+1));

    /*if(display)
    {
        QSet<QString> mapUsed=loadMap(otherPlayer.presumed_map,display);
        QSetIterator<QString> i(mapUsed);
        while (i.hasNext())
        {
            QString map = i.next();
            if(mapUsedByOtherPlayer.contains(map))
                mapUsedByOtherPlayer[map]++;
            else
                mapUsedByOtherPlayer[map]=1;
        }
        removeUnusedMap();
    }*/
    /// \todo temp fix, do a better fix
    centerOn(MapObjectItem::objectLink[playerMapObject]);

    if(ObjectGroupItem::objectGroupLink.contains(otherPlayer.presumed_map->objectGroup))
    {
        ObjectGroupItem::objectGroupLink[otherPlayer.presumed_map->objectGroup]->addObject(otherPlayer.playerMapObject);
        if(!MapObjectItem::objectLink.contains(otherPlayer.playerMapObject))
            qDebug() << QString("loadOtherPlayerFromMap(), MapObjectItem::objectLink don't have otherPlayer.playerMapObject");
        else
        {
            if(MapObjectItem::objectLink[otherPlayer.playerMapObject]==NULL)
                qDebug() << QString("loadOtherPlayerFromMap(), MapObjectItem::objectLink[otherPlayer.playerMapObject]==NULL");
            else
                MapObjectItem::objectLink[otherPlayer.playerMapObject]->setZValue(otherPlayer.y);
        }
    }
    else
        qDebug() << QString("loadOtherPlayerFromMap(), ObjectGroupItem::objectGroupLink not contains current_map->objectGroup");
}

//call before leave the old map (and before loadPlayerFromCurrentMap())
void MapControllerMP::unloadOtherPlayerFromMap(OtherPlayer otherPlayer)
{
    //unload the player sprite
    if(ObjectGroupItem::objectGroupLink.contains(otherPlayer.playerMapObject->objectGroup()))
        ObjectGroupItem::objectGroupLink[otherPlayer.playerMapObject->objectGroup()]->removeObject(otherPlayer.playerMapObject);
    else
        qDebug() << QString("unloadOtherPlayerFromMap(), ObjectGroupItem::objectGroupLink not contains otherPlayer.playerMapObject->objectGroup()");

    QSetIterator<QString> i(otherPlayer.mapUsed);
    while (i.hasNext())
    {
        QString map = i.next();
        if(mapUsedByOtherPlayer.contains(map))
        {
            mapUsedByOtherPlayer[map]--;
            if(mapUsedByOtherPlayer[map]==0)
                mapUsedByOtherPlayer.remove(map);
        }
        else
            qDebug() << QString("map not found into mapUsedByOtherPlayer for player: %1, map: %2").arg(otherPlayer.informations.simplifiedId).arg(map);
    }
}

void MapControllerMP::move_player(const quint16 &id, const QList<QPair<quint8, CatchChallenger::Direction> > &movement)
{
    if(!mHaveTheDatapack || !player_informations_is_set)
    {
        DelayedMove tempItem;
        tempItem.id=id;
        tempItem.movement=movement;
        DelayedMultiplex multiplex;
        multiplex.move=tempItem;
        multiplex.type=DelayedType_Move;
        delayedActions << multiplex;
        return;
    }
    if(id==player_informations.public_informations.simplifiedId)
    {
        qDebug() << "The current player can't be moved (only teleported)";
        return;
    }
    if(!otherPlayerList.contains(id))
    {
        qDebug() << QString("Other player (%1) not loaded on the map").arg(id);
        return;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    QStringList moveString;
    int index_temp=0;
    while(index_temp<movement.size())
    {
        QPair<quint8, CatchChallenger::Direction> move=movement.at(index_temp);
        moveString << QString("{%1,%2}").arg(move.first).arg(CatchChallenger::MoveOnTheMap::directionToString(move.second));
        index_temp++;
    }

    qDebug() << QString("move_player(%1,%2), previous direction: %3").arg(id).arg(moveString.join(";")).arg(CatchChallenger::MoveOnTheMap::directionToString(otherPlayerList[id].direction));
    #endif


    //reset to the good position
    otherPlayerList[id].oneStepMore->stop();
    otherPlayerList[id].inMove=false;
    otherPlayerList[id].moveStep=0;
    if(otherPlayerList[id].current_map!=otherPlayerList[id].presumed_map->logicalMap.map_file)
    {
        unloadOtherPlayerFromMap(otherPlayerList[id]);
        QString mapPath=otherPlayerList[id].current_map;
        if(!haveMapInMemory(mapPath))
        {
            /// \todo this case
            qDebug() << QString("move_player(%1), map not already loaded").arg(id).arg(otherPlayerList[id].current_map);
            return;
        }
        loadOtherMap(mapPath);
        otherPlayerList[id].presumed_map=all_map[mapPath];
        loadOtherPlayerFromMap(otherPlayerList[id]);
    }
    quint8 x=otherPlayerList[id].x;
    quint8 y=otherPlayerList[id].y;
    otherPlayerList[id].presumed_x=x;
    otherPlayerList[id].presumed_y=y;
    otherPlayerList[id].presumed_direction=otherPlayerList[id].direction;
    CatchChallenger::Map * map=&otherPlayerList[id].presumed_map->logicalMap;
    CatchChallenger::Map * old_map;

    //move to have the new position if needed
    int index=0;
    while(index<movement.size())
    {
        QPair<quint8, CatchChallenger::Direction> move=movement.at(index);
        int index2=0;
        while(index2<move.first)
        {
            old_map=map;
            //set the final value (direction, position, ...)
            switch(otherPlayerList[id].presumed_direction)
            {
                case CatchChallenger::Direction_move_at_left:
                case CatchChallenger::Direction_move_at_right:
                case CatchChallenger::Direction_move_at_top:
                case CatchChallenger::Direction_move_at_bottom:
                if(CatchChallenger::MoveOnTheMap::canGoTo(otherPlayerList[id].presumed_direction,*map,x,y,true))
                    CatchChallenger::MoveOnTheMap::move(otherPlayerList[id].presumed_direction,&map,&x,&y);
                else
                {
                    qDebug() << QString("move_player(): at %1(%2,%3) can't go to %4").arg(map->map_file).arg(x).arg(y).arg(CatchChallenger::MoveOnTheMap::directionToString(otherPlayerList[id].presumed_direction));
                    return;
                }
                break;
                default:
                qDebug() << QString("move_player(): moveStep: %1, wrong direction: %2").arg(move.first).arg(CatchChallenger::MoveOnTheMap::directionToString(otherPlayerList[id].presumed_direction));
                return;
            }
            //if the map have changed
            if(old_map!=map)
            {
                loadOtherMap(map->map_file);
                if(!all_map.contains(map->map_file))
                    qDebug() << QString("map changed not located: %1").arg(map->map_file);
                else
                {
                    unloadOtherPlayerFromMap(otherPlayerList[id]);
                    otherPlayerList[id].presumed_map=all_map[map->map_file];
                    loadOtherPlayerFromMap(otherPlayerList[id]);
                }
            }
            index2++;
        }
        otherPlayerList[id].direction=move.second;
        index++;
    }


    //set the new variables
    otherPlayerList[id].current_map=map->map_file;
    otherPlayerList[id].x=x;
    otherPlayerList[id].y=y;

    otherPlayerList[id].presumed_map=all_map[otherPlayerList[id].current_map];
    otherPlayerList[id].presumed_x=otherPlayerList[id].x;
    otherPlayerList[id].presumed_y=otherPlayerList[id].y;
    otherPlayerList[id].presumed_direction=otherPlayerList[id].direction;

    //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
    otherPlayerList[id].playerMapObject->setPosition(QPoint(otherPlayerList[id].presumed_x,otherPlayerList[id].presumed_y+1));
    MapObjectItem::objectLink[otherPlayerList[id].playerMapObject]->setZValue(otherPlayerList[id].presumed_y);

    //start moving into the right direction
    switch(otherPlayerList[id].presumed_direction)
    {
        case CatchChallenger::Direction_look_at_top:
        case CatchChallenger::Direction_move_at_top:
            otherPlayerList[id].playerMapObject->setTile(otherPlayerList[id].playerTileset->tileAt(1));
        break;
        case CatchChallenger::Direction_look_at_right:
        case CatchChallenger::Direction_move_at_right:
            otherPlayerList[id].playerMapObject->setTile(otherPlayerList[id].playerTileset->tileAt(4));
        break;
        case CatchChallenger::Direction_look_at_bottom:
        case CatchChallenger::Direction_move_at_bottom:
            otherPlayerList[id].playerMapObject->setTile(otherPlayerList[id].playerTileset->tileAt(7));
        break;
        case CatchChallenger::Direction_look_at_left:
        case CatchChallenger::Direction_move_at_left:
            otherPlayerList[id].playerMapObject->setTile(otherPlayerList[id].playerTileset->tileAt(10));
        break;
        default:
            qDebug() << QString("move_player(): player: %1 (%2), wrong direction: %3").arg(otherPlayerList[id].informations.pseudo).arg(id).arg(otherPlayerList[id].presumed_direction);
        return;
    }
    switch(otherPlayerList[id].presumed_direction)
    {
        case CatchChallenger::Direction_move_at_top:
        case CatchChallenger::Direction_move_at_right:
        case CatchChallenger::Direction_move_at_bottom:
        case CatchChallenger::Direction_move_at_left:
            otherPlayerList[id].oneStepMore->start(otherPlayerList[id].informations.speed/5);
        break;
        default:
        break;
    }
}

void MapControllerMP::remove_player(const quint16 &id)
{
    if(!mHaveTheDatapack || !player_informations_is_set)
    {
        #ifdef DEBUG_CLIENT_LOAD_ORDER
        qDebug() << QString("delayed: MapControllerMP::remove_player(%1)").arg(id);
        #endif
        DelayedMultiplex multiplex;
        multiplex.remove=id;
        multiplex.type=DelayedType_Remove;
        delayedActions << multiplex;
        return;
    }
    if(id==player_informations.public_informations.simplifiedId)
    {
        qDebug() << "The current player can't be removed";
        return;
    }
    if(!otherPlayerList.contains(id))
    {
        qDebug() << QString("Other player (%1) not exists").arg(id);
        return;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    qDebug() << QString("remove_player(%1)").arg(id);
    #endif
    unloadOtherPlayerFromMap(otherPlayerList[id]);

    otherPlayerListByTimer.remove(otherPlayerList[id].oneStepMore);

    Tiled::ObjectGroup *currentGroup=otherPlayerList[id].playerMapObject->objectGroup();
    if(currentGroup!=NULL)
    {
        if(ObjectGroupItem::objectGroupLink.contains(currentGroup))
            ObjectGroupItem::objectGroupLink[currentGroup]->removeObject(otherPlayerList[id].playerMapObject);
        if(currentGroup!=otherPlayerList[id].presumed_map->objectGroup)
            qDebug() << QString("loadOtherPlayerFromMap(), the playerMapObject group is wrong: %1").arg(currentGroup->name());
        currentGroup->removeObject(otherPlayerList[id].playerMapObject);
    }

    delete otherPlayerList[id].playerMapObject;
    delete otherPlayerList[id].playerTileset;
    delete otherPlayerList[id].oneStepMore;

    otherPlayerList.remove(id);
}

void MapControllerMP::reinsert_player(const quint16 &id,const quint8 &x,const quint8 &y,const CatchChallenger::Direction &direction)
{
    if(!mHaveTheDatapack || !player_informations_is_set)
    {
        #ifdef DEBUG_CLIENT_LOAD_ORDER
        qDebug() << QString("delayed: MapControllerMP::reinsert_player(%1)").arg(id);
        #endif
        DelayedReinsertSingle tempItem;
        tempItem.id=id;
        tempItem.x=x;
        tempItem.y=y;
        tempItem.direction=direction;
        DelayedMultiplex multiplex;
        multiplex.reinsert_single=tempItem;
        multiplex.type=DelayedType_Reinsert_single;
        delayedActions << multiplex;
        return;
    }
    if(id==player_informations.public_informations.simplifiedId)
    {
        qDebug() << "The current player can't be removed";
        return;
    }
    if(!otherPlayerList.contains(id))
    {
        qDebug() << QString("Other player (%1) not exists").arg(id);
        return;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    qDebug() << QString("reinsert_player(%1)").arg(id);
    #endif

    CatchChallenger::Player_public_informations informations=otherPlayerList[id].informations;
    /// \warning search by loop because otherPlayerList[id].current_map is the full path, DatapackClientLoader::datapackLoader.maps relative path
    if(!all_map.contains(otherPlayerList[id].current_map))
    {
        qDebug() << "internal problem, revert map (" << otherPlayerList[id].current_map << ") index is wrong (" << DatapackClientLoader::datapackLoader.maps.join(";") << ")";
        return;
    }
    quint32 mapId=(quint32)all_map[otherPlayerList[id].current_map]->logicalMap.id;
    if(mapId==0)
        qDebug() << QString("supected NULL map then error");
    remove_player(id);
    insert_player(informations,mapId,x,y,direction);
}

void MapControllerMP::reinsert_player(const quint16 &id,const quint32 &mapId,const quint8 &x,const quint8 &y,const CatchChallenger::Direction &direction)
{
    if(!mHaveTheDatapack || !player_informations_is_set)
    {
        #ifdef DEBUG_CLIENT_LOAD_ORDER
        qDebug() << QString("delayed: MapControllerMP::reinsert_player(%1)").arg(id);
        #endif
        DelayedReinsertFull tempItem;
        tempItem.id=id;
        tempItem.mapId=mapId;
        tempItem.x=x;
        tempItem.y=y;
        tempItem.direction=direction;
        DelayedMultiplex multiplex;
        multiplex.reinsert_full=tempItem;
        multiplex.type=DelayedType_Reinsert_full;
        delayedActions << multiplex;
        return;
    }
    if(id==player_informations.public_informations.simplifiedId)
    {
        qDebug() << "The current player can't be removed";
        return;
    }
    if(!otherPlayerList.contains(id))
    {
        qDebug() << QString("Other player (%1) not exists").arg(id);
        return;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    qDebug() << QString("reinsert_player(%1)").arg(id);
    #endif

    CatchChallenger::Player_public_informations informations=otherPlayerList[id].informations;
    remove_player(id);
    insert_player(informations,mapId,x,y,direction);
}

void MapControllerMP::dropAllPlayerOnTheMap()
{
    if(!mHaveTheDatapack || !player_informations_is_set)
    {
        #ifdef DEBUG_CLIENT_LOAD_ORDER
        qDebug() << QString("delayed: MapControllerMP::dropAllPlayerOnTheMap()");
        #endif
        DelayedMultiplex multiplex;
        multiplex.type=DelayedType_Drop_all;
        delayedActions << multiplex;
        return;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    qDebug() << QString("dropAllPlayerOnTheMap()");
    #endif
    QList<quint16> temIdList;
    QHashIterator<quint16,OtherPlayer> i(otherPlayerList);
    while (i.hasNext()) {
        i.next();
        temIdList << i.key();
    }
    int index=0;
    while(index<temIdList.size())
    {
        remove_player(temIdList.at(index));
        index++;
    }
}

void MapControllerMP::teleportTo(const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction)
{
    if(!mHaveTheDatapack || !player_informations_is_set)
    {
        DelayedTeleportTo tempItem;
        tempItem.mapId=mapId;
        tempItem.x=x;
        tempItem.y=y;
        tempItem.direction=direction;
        delayedTeleportTo << tempItem;
        #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
        qDebug() << QString("delayed teleportTo(%1,%2,%3,%4)").arg(DatapackClientLoader::datapackLoader.maps[mapId]).arg(x).arg(y).arg(CatchChallenger::MoveOnTheMap::directionToString(direction));
        #endif
        return;
    }
    if(mapId>=(quint32)DatapackClientLoader::datapackLoader.maps.size())
    {
        emit error("mapId greater than DatapackClientLoader::datapackLoader.maps.size(): "+QString::number(DatapackClientLoader::datapackLoader.maps.size()));
        return;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    qDebug() << QString("teleportTo(%1,%2,%3,%4)").arg(DatapackClientLoader::datapackLoader.maps[mapId]).arg(x).arg(y).arg(CatchChallenger::MoveOnTheMap::directionToString(direction));
    qDebug() << QString("currently on: %1 (%2,%3)").arg(current_map).arg(this->x).arg(this->y);
    #endif
    //the direction
    this->direction=direction;
    switch(direction)
    {
        case CatchChallenger::Direction_look_at_top:
        case CatchChallenger::Direction_move_at_top:
            playerMapObject->setTile(playerTileset->tileAt(1));
        break;
        case CatchChallenger::Direction_look_at_right:
        case CatchChallenger::Direction_move_at_right:
            playerMapObject->setTile(playerTileset->tileAt(4));
        break;
        case CatchChallenger::Direction_look_at_bottom:
        case CatchChallenger::Direction_move_at_bottom:
            playerMapObject->setTile(playerTileset->tileAt(7));
        break;
        case CatchChallenger::Direction_look_at_left:
        case CatchChallenger::Direction_move_at_left:
            playerMapObject->setTile(playerTileset->tileAt(10));
        break;
        default:
        QMessageBox::critical(NULL,tr("Internal error"),tr("The direction send by the server is wrong"));
        return;
    }

    //position
    this->x=x;
    this->y=y;

    unloadPlayerFromCurrentMap();
    QString mapPath=QFileInfo(datapackMapPath+DatapackClientLoader::datapackLoader.maps[mapId]).absoluteFilePath();
    current_map=mapPath;
    if(!haveMapInMemory(mapPath))
        emit inWaitingOfMap();
    loadOtherMap(mapPath);
    CatchChallenger::Api_client_real::client->teleportDone();
}

void MapControllerMP::finalPlayerStep()
{
    if(!all_map.contains(current_map))
    {
        qDebug() << "current map not loaded, unable to do finalPlayerStep()";
        return;
    }
    const MapVisualiserThread::Map_full * current_map_pointer=all_map[current_map];
    int index=0;
    const int size=current_map_pointer->logicalMap.teleport_semi.size();
    while(index<size)
    {
        const CatchChallenger::Map_semi_teleport &current_teleport=current_map_pointer->logicalMap.teleport_semi.at(index);
        //if need be teleported
        if(current_teleport.source_x==x && current_teleport.source_y==y)
        {
            unloadPlayerFromCurrentMap();
            QString mapPath=QFileInfo(datapackMapPath+current_teleport.map).absoluteFilePath();
            current_map=mapPath;
            x=current_teleport.destination_x;
            y=current_teleport.destination_y;
            if(!haveMapInMemory(current_map))
                emit inWaitingOfMap();
            loadOtherMap(current_map);
            return;
        }
        index++;
    }
    MapVisualiserPlayer::finalPlayerStep();
}

//player info
void MapControllerMP::have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations)
{
    #ifdef DEBUG_CLIENT_LOAD_ORDER
    qDebug() << QString("MapControllerMP::have_current_player_info()");
    #endif

    if(player_informations_is_set)
    {
        qDebug() << "player information already set";
        return;
    }
    this->player_informations=informations;
    player_informations_is_set=true;

    if(mHaveTheDatapack)
        reinject_signals();
}

//the datapack
void MapControllerMP::setDatapackPath(const QString &path)
{
    #ifdef DEBUG_CLIENT_LOAD_ORDER
    qDebug() << QString("MapControllerMP::setDatapackPath()");
    #endif

    if(path.endsWith("/") || path.endsWith("\\"))
        datapackPath=path;
    else
        datapackPath=path+"/";
    datapackMapPath=QFileInfo(datapackPath+DATAPACK_BASE_PATH_MAP).absoluteFilePath();
    if(!datapackMapPath.endsWith("/") && !datapackMapPath.endsWith("\\"))
        datapackMapPath+="/";
    mLastLocation.clear();
}

void MapControllerMP::datapackParsed()
{
    #ifdef DEBUG_CLIENT_LOAD_ORDER
    qDebug() << QString("MapControllerMP::datapackParsed()");
    #endif

    if(mHaveTheDatapack)
        return;
    mHaveTheDatapack=true;

    skinFolderList=CatchChallenger::FacilityLib::skinIdList(datapackPath+DATAPACK_BASE_PATH_SKIN);

    setAnimationTilset(datapackPath+DATAPACK_BASE_PATH_ANIMATION+"animation.png");

    if(player_informations_is_set)
        reinject_signals();
}

void MapControllerMP::reinject_signals()
{
    #ifdef DEBUG_CLIENT_LOAD_ORDER
    qDebug() << QString("MapControllerMP::reinject_signals()");
    #endif
    int index;

    if(mHaveTheDatapack && player_informations_is_set)
    {
        #ifdef DEBUG_CLIENT_LOAD_ORDER
        qDebug() << QString("MapControllerMP::reinject_signals(): mHaveTheDatapack && player_informations_is_set");
        #endif
        index=0;
        while(index<delayedActions.size())
        {
            switch(delayedActions.at(index).type)
            {
                case DelayedType_Insert:
                    insert_player(delayedActions.at(index).insert.player,delayedActions.at(index).insert.mapId,delayedActions.at(index).insert.x,delayedActions.at(index).insert.y,delayedActions.at(index).insert.direction);
                break;
                case DelayedType_Move:
                    move_player(delayedActions.at(index).move.id,delayedActions.at(index).move.movement);
                break;
                case DelayedType_Remove:
                    remove_player(delayedActions.at(index).remove);
                break;
                case DelayedType_Reinsert_single:
                    reinsert_player(delayedActions.at(index).reinsert_single.id,delayedActions.at(index).reinsert_single.x,delayedActions.at(index).reinsert_single.y,delayedActions.at(index).reinsert_single.direction);
                break;
                case DelayedType_Reinsert_full:
                    reinsert_player(delayedActions.at(index).reinsert_full.id,delayedActions.at(index).reinsert_full.mapId,delayedActions.at(index).reinsert_full.x,delayedActions.at(index).reinsert_full.y,delayedActions.at(index).reinsert_full.direction);
                break;
                case DelayedType_Drop_all:
                    dropAllPlayerOnTheMap();
                break;
                default:
                break;
            }
            index++;
        }
        delayedActions.clear();

        index=0;
        while(index<delayedTeleportTo.size())
        {
            teleportTo(delayedTeleportTo.at(index).mapId,delayedTeleportTo.at(index).x,delayedTeleportTo.at(index).y,delayedTeleportTo.at(index).direction);
            index++;
        }
        delayedTeleportTo.clear();
    }
    else
        qDebug() << QString("MapControllerMP::reinject_signals(): should not pass here because all is not previously loaded");
}

void MapControllerMP::moveOtherPlayerStepSlot()
{
    QTimer *timer=qobject_cast<QTimer *>(QObject::sender());
    if(timer==NULL)
    {
        qDebug() << "moveOtherPlayerStepSlot() timer not located";
        return;
    }
    #ifdef DEBUG_CLIENT_OTHER_PLAYER_MOVE_STEP
    qDebug() << QString("moveOtherPlayerStepSlot() player: %1 (%2), moveStep: %3")
            .arg(otherPlayerList[otherPlayerListByTimer[timer]].informations.pseudo)
            .arg(otherPlayerList[otherPlayerListByTimer[timer]].informations.simplifiedId)
            .arg(otherPlayerList[otherPlayerListByTimer[timer]].moveStep);
    #endif
    int baseTile=1;
    //move the player for intermediate step and define the base tile (define the stopped step with direction)
    switch(otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction)
    {
        case CatchChallenger::Direction_move_at_left:
        baseTile=10;
        switch(otherPlayerList[otherPlayerListByTimer[timer]].moveStep)
        {
            case 1:
            case 2:
            case 3:
            case 4:
            otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject->setX(otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject->x()-0.20);
            break;
        }
        break;
        case CatchChallenger::Direction_move_at_right:
        baseTile=4;
        switch(otherPlayerList[otherPlayerListByTimer[timer]].moveStep)
        {
            case 1:
            case 2:
            case 3:
            case 4:
            otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject->setX(otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject->x()+0.20);
            break;
        }
        break;
        case CatchChallenger::Direction_move_at_top:
        baseTile=1;
        switch(otherPlayerList[otherPlayerListByTimer[timer]].moveStep)
        {
            case 1:
            case 2:
            case 3:
            case 4:
            otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject->setY(otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject->y()-0.20);
            break;
        }
        break;
        case CatchChallenger::Direction_move_at_bottom:
        baseTile=7;
        switch(otherPlayerList[otherPlayerListByTimer[timer]].moveStep)
        {
            case 1:
            case 2:
            case 3:
            case 4:
            otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject->setY(otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject->y()+0.20);
            break;
        }
        break;
        default:
        qDebug() << QString("moveOtherPlayerStepSlot(): moveStep: %1, wrong direction").arg(otherPlayerList[otherPlayerListByTimer[timer]].moveStep);
        timer->stop();
        return;
    }

    //apply the right step of the base step defined previously by the direction
    switch(otherPlayerList[otherPlayerListByTimer[timer]].moveStep)
    {
        //stopped step
        case 0:
        otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject->setTile(otherPlayerList[otherPlayerListByTimer[timer]].playerTileset->tileAt(baseTile+0));
        break;
        case 1:
        MapObjectItem::objectLink[otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject]->setZValue(qCeil(otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject->y()));
        break;
        //transition step
        case 2:
        if(stepAlternance)
            otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject->setTile(otherPlayerList[otherPlayerListByTimer[timer]].playerTileset->tileAt(baseTile-1));
        else
            otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject->setTile(otherPlayerList[otherPlayerListByTimer[timer]].playerTileset->tileAt(baseTile+1));
        otherPlayerList[otherPlayerListByTimer[timer]].stepAlternance=!otherPlayerList[otherPlayerListByTimer[timer]].stepAlternance;
        break;
        //stopped step
        case 4:
        otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject->setTile(otherPlayerList[otherPlayerListByTimer[timer]].playerTileset->tileAt(baseTile+0));
        break;
    }

    otherPlayerList[otherPlayerListByTimer[timer]].moveStep++;

    //if have finish the step
    if(otherPlayerList[otherPlayerListByTimer[timer]].moveStep>5)
    {
        CatchChallenger::Map * old_map=&otherPlayerList[otherPlayerListByTimer[timer]].presumed_map->logicalMap;
        CatchChallenger::Map * map=&otherPlayerList[otherPlayerListByTimer[timer]].presumed_map->logicalMap;
        quint8 x=otherPlayerList[otherPlayerListByTimer[timer]].presumed_x;
        quint8 y=otherPlayerList[otherPlayerListByTimer[timer]].presumed_y;
        //set the final value (direction, position, ...)
        switch(otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction)
        {
            case CatchChallenger::Direction_move_at_right:
            case CatchChallenger::Direction_move_at_top:
            case CatchChallenger::Direction_move_at_bottom:
            case CatchChallenger::Direction_move_at_left:
                CatchChallenger::MoveOnTheMap::move(otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction,&map,&x,&y);
            break;
            default:
            qDebug() << QString("moveStepSlot(): moveStep: %1, wrong direction when moveStep>2").arg(otherPlayerList[otherPlayerListByTimer[timer]].moveStep);
            return;
        }
        otherPlayerList[otherPlayerListByTimer[timer]].presumed_x=x;
        otherPlayerList[otherPlayerListByTimer[timer]].presumed_y=y;
        //if the map have changed
        if(old_map!=map)
        {
            loadOtherMap(map->map_file);
            if(!all_map.contains(map->map_file))
                qDebug() << QString("map changed not located: %1").arg(map->map_file);
            else
            {
                unloadOtherPlayerFromMap(otherPlayerList[otherPlayerListByTimer[timer]]);
                otherPlayerList[otherPlayerListByTimer[timer]].presumed_map=all_map[map->map_file];
                loadOtherPlayerFromMap(otherPlayerList[otherPlayerListByTimer[timer]]);
            }
        }
        //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
        otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject->setPosition(QPoint(x,y+1));
        MapObjectItem::objectLink[otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject]->setZValue(y);

        //check if one arrow key is pressed to continue to move into this direction
        if(otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction==CatchChallenger::Direction_move_at_left)
        {
            //can't go into this direction, then just look into this direction
            if(!CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_left,otherPlayerList[otherPlayerListByTimer[timer]].presumed_map->logicalMap,x,y,true))
            {
                otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction=CatchChallenger::Direction_look_at_left;
                otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject->setTile(otherPlayerList[otherPlayerListByTimer[timer]].playerTileset->tileAt(10));
                otherPlayerList[otherPlayerListByTimer[timer]].inMove=false;
                timer->stop();
            }
            //if can go, then do the move
            else
            {
                otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction=CatchChallenger::Direction_move_at_left;
                otherPlayerList[otherPlayerListByTimer[timer]].moveStep=0;
                moveOtherPlayerStepSlot();
            }
        }
        else if(otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction==CatchChallenger::Direction_move_at_right)
        {
            //can't go into this direction, then just look into this direction
            if(!CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_right,otherPlayerList[otherPlayerListByTimer[timer]].presumed_map->logicalMap,x,y,true))
            {
                otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction=CatchChallenger::Direction_look_at_right;
                otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject->setTile(otherPlayerList[otherPlayerListByTimer[timer]].playerTileset->tileAt(4));
                otherPlayerList[otherPlayerListByTimer[timer]].inMove=false;
                timer->stop();
            }
            //if can go, then do the move
            else
            {
                otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction=CatchChallenger::Direction_move_at_right;
                otherPlayerList[otherPlayerListByTimer[timer]].moveStep=0;
                moveOtherPlayerStepSlot();
            }
        }
        else if(otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction==CatchChallenger::Direction_move_at_top)
        {
            //can't go into this direction, then just look into this direction
            if(!CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_top,otherPlayerList[otherPlayerListByTimer[timer]].presumed_map->logicalMap,x,y,true))
            {
                otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction=CatchChallenger::Direction_look_at_top;
                otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject->setTile(otherPlayerList[otherPlayerListByTimer[timer]].playerTileset->tileAt(1));
                otherPlayerList[otherPlayerListByTimer[timer]].inMove=false;
                timer->stop();
            }
            //if can go, then do the move
            else
            {
                otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction=CatchChallenger::Direction_move_at_top;
                otherPlayerList[otherPlayerListByTimer[timer]].moveStep=0;
                moveOtherPlayerStepSlot();
            }
        }
        else if(otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction==CatchChallenger::Direction_move_at_bottom)
        {
            //can't go into this direction, then just look into this direction
            if(!CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_bottom,otherPlayerList[otherPlayerListByTimer[timer]].presumed_map->logicalMap,x,y,true))
            {
                otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction=CatchChallenger::Direction_look_at_bottom;
                otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject->setTile(otherPlayerList[otherPlayerListByTimer[timer]].playerTileset->tileAt(7));
                otherPlayerList[otherPlayerListByTimer[timer]].inMove=false;
                timer->stop();
            }
            //if can go, then do the move
            else
            {
                otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction=CatchChallenger::Direction_move_at_bottom;
                otherPlayerList[otherPlayerListByTimer[timer]].moveStep=0;
                moveOtherPlayerStepSlot();
            }
        }
        //now stop walking, no more arrow key is pressed
        else
        {
            otherPlayerList[otherPlayerListByTimer[timer]].moveStep=0;
            otherPlayerList[otherPlayerListByTimer[timer]].inMove=false;
            timer->stop();
        }
    }
    else
        timer->start();
}

/// \warning all ObjectGroupItem destroyed into removeMap()
void MapControllerMP::destroyMap(MapVisualiserThread::Map_full *map)
{
    //remove the other player
    QHash<quint16,OtherPlayer>::const_iterator i = otherPlayerList.constBegin();
    while (i != otherPlayerList.constEnd()) {
        if(i.value().presumed_map==map)
        {
            //unloadOtherPlayerFromMap(i.value());-> seam useless for the bug
            remove_player(i.key());
            i = otherPlayerList.constBegin();
        }
        else
            ++i;
    }
    MapVisualiser::destroyMap(map);
}
