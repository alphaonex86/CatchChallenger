#include "MapController.h"
#include "../../general/base/MoveOnTheMap.h"
#include "../../general/base/FacilityLib.h"
#include "DatapackClientLoader.h"
#include "../ClientVariable.h"

#include <QMessageBox>
#include <QMessageBox>
#include <qmath.h>

MapControllerMP::MapControllerMP(Pokecraft::Api_protocol *client,const bool &centerOnPlayer,const bool &debugTags,const bool &useCache,const bool &OpenGL) :
    MapVisualiserPlayer(centerOnPlayer,debugTags,useCache,OpenGL)
{
    qRegisterMetaType<Pokecraft::Direction>("Pokecraft::Direction");
    qRegisterMetaType<Pokecraft::Chat_type>("Pokecraft::Chat_type");
    qRegisterMetaType<Pokecraft::Player_public_informations>("Pokecraft::Player_public_informations");
    qRegisterMetaType<Pokecraft::Player_private_and_public_informations>("Pokecraft::Player_private_and_public_informations");
    qRegisterMetaType<QList<QPair<quint8,Pokecraft::Direction> > >("QList<QPair<quint8,Pokecraft::Direction> >");

    this->client=client;
    player_informations_is_set=false;

    resetAll();

    //connect the map controler
    connect(client,SIGNAL(have_current_player_info(Pokecraft::Player_private_and_public_informations)),this,SLOT(have_current_player_info(Pokecraft::Player_private_and_public_informations)),Qt::QueuedConnection);
    connect(client,SIGNAL(insert_player(Pokecraft::Player_public_informations,quint32,quint16,quint16,Pokecraft::Direction)),this,SLOT(insert_player(Pokecraft::Player_public_informations,quint32,quint16,quint16,Pokecraft::Direction)),Qt::QueuedConnection);
    connect(client,SIGNAL(remove_player(quint16)),this,SLOT(remove_player(quint16)),Qt::QueuedConnection);
    connect(client,SIGNAL(move_player(quint16,QList<QPair<quint8,Pokecraft::Direction> >)),this,SLOT(move_player(quint16,QList<QPair<quint8,Pokecraft::Direction> >)),Qt::QueuedConnection);
    connect(client,SIGNAL(reinsert_player(quint16,quint8,quint8,Pokecraft::Direction)),this,SLOT(reinsert_player(quint16,quint8,quint8,Pokecraft::Direction)),Qt::QueuedConnection);
    connect(client,SIGNAL(reinsert_player(quint16,quint32,quint8,quint8,Pokecraft::Direction)),this,SLOT(reinsert_player(quint16,quint32,quint8,quint8,Pokecraft::Direction)),Qt::QueuedConnection);
    connect(this,SIGNAL(send_player_direction(Pokecraft::Direction)),client,SLOT(send_player_direction(Pokecraft::Direction)),Qt::QueuedConnection);
    connect(client,SIGNAL(teleportTo(quint32,quint16,quint16,Pokecraft::Direction)),this,SLOT(teleportTo(quint32,quint16,quint16,Pokecraft::Direction)),Qt::QueuedConnection);
    connect(this,SIGNAL(teleportDone()),client,SLOT(teleportDone()),Qt::QueuedConnection);

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
    current_map=NULL;

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

    QString current_map_fileName=loadOtherMap(fileName);
    if(current_map_fileName.isEmpty())
    {
        QMessageBox::critical(NULL,"Error",mLastError);
        return false;
    }
    current_map=all_map[current_map_fileName];

    render();

    mapUsed=loadMap(current_map,true);
    removeUnusedMap();
    loadPlayerFromCurrentMap();

    QStringList map_list;
    QSetIterator<QString> i(displayed_map);
    while (i.hasNext())
        map_list << i.next();
    qDebug() << QString("MapControllerMP::loadPlayerMap(): displayed_map: %1").arg(map_list.join(";"));

    show();

    return true;
}

void MapControllerMP::removeUnusedMap()
{
    ///remove the not used map, then where no player is susceptible to switch (by border or teleporter)
    QHash<QString,Map_full *>::const_iterator i = all_map.constBegin();
    while (i != all_map.constEnd()) {
        if(!mapUsed.contains((*i)->logicalMap.map_file) && !mapUsedByOtherPlayer.contains((*i)->logicalMap.map_file))
        {
            destroyMap(*i);
            i = all_map.constBegin();//needed
        }
        else
            ++i;
    }
}

//map move
void MapControllerMP::insert_player(const Pokecraft::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const Pokecraft::Direction &direction)
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
        qDebug() << QString("delayed: insert_player(%1->%2,%3,%4,%5,%6)").arg(player.pseudo).arg(player.simplifiedId).arg(mapId).arg(x).arg(y).arg(Pokecraft::MoveOnTheMap::directionToString(direction));
        #endif
        return;
    }
    if(mapId>=(quint32)DatapackClientLoader::datapackLoader.maps.size())
    {
        qDebug() << "mapId greater than DatapackClientLoader::datapackLoader.maps.size()";
        return;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    qDebug() << QString("insert_player(%1->%2,%3,%4,%5,%6)").arg(player.pseudo).arg(player.simplifiedId).arg(DatapackClientLoader::datapackLoader.maps[mapId]).arg(x).arg(y).arg(Pokecraft::MoveOnTheMap::directionToString(direction));
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
            case Pokecraft::Direction_look_at_top:
            case Pokecraft::Direction_move_at_top:
                playerMapObject->setTile(playerTileset->tileAt(1));
            break;
            case Pokecraft::Direction_look_at_right:
            case Pokecraft::Direction_move_at_right:
                playerMapObject->setTile(playerTileset->tileAt(4));
            break;
            case Pokecraft::Direction_look_at_bottom:
            case Pokecraft::Direction_move_at_bottom:
                playerMapObject->setTile(playerTileset->tileAt(7));
            break;
            case Pokecraft::Direction_look_at_left:
            case Pokecraft::Direction_move_at_left:
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

        QString current_map_fileName=loadOtherMap(datapackMapPath+DatapackClientLoader::datapackLoader.maps[mapId]);
        if(current_map_fileName.isEmpty())
        {
            qDebug() << QString("unable to insert player: %1 on map: %2").arg(player.pseudo).arg(DatapackClientLoader::datapackLoader.maps[mapId]);
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
        tempPlayer.current_map=current_map_fileName;
        tempPlayer.presumed_map=all_map[current_map_fileName];
        tempPlayer.presumed_x=x;
        tempPlayer.presumed_y=y;
        switch(direction)
        {
            case Pokecraft::Direction_look_at_top:
            case Pokecraft::Direction_move_at_top:
                tempPlayer.playerMapObject->setTile(tempPlayer.playerTileset->tileAt(1));
            break;
            case Pokecraft::Direction_look_at_right:
            case Pokecraft::Direction_move_at_right:
                tempPlayer.playerMapObject->setTile(tempPlayer.playerTileset->tileAt(4));
            break;
            case Pokecraft::Direction_look_at_bottom:
            case Pokecraft::Direction_move_at_bottom:
                tempPlayer.playerMapObject->setTile(tempPlayer.playerTileset->tileAt(7));
            break;
            case Pokecraft::Direction_look_at_left:
            case Pokecraft::Direction_move_at_left:
                tempPlayer.playerMapObject->setTile(tempPlayer.playerTileset->tileAt(10));
            break;
            default:
                delete tempPlayer.playerMapObject;
                delete tempPlayer.playerTileset;
                qDebug() << "The direction send by the server is wrong";
            return;
        }

        loadOtherPlayerFromMap(tempPlayer);

        tempPlayer.informations=player;
        tempPlayer.oneStepMore=new QTimer();
        tempPlayer.oneStepMore->setSingleShot(true);
        otherPlayerListByTimer[tempPlayer.oneStepMore]=player.simplifiedId;
        connect(tempPlayer.oneStepMore,SIGNAL(timeout()),this,SLOT(moveOtherPlayerStepSlot()));
        otherPlayerList[player.simplifiedId]=tempPlayer;

        switch(direction)
        {
            case Pokecraft::Direction_move_at_top:
            case Pokecraft::Direction_move_at_right:
            case Pokecraft::Direction_move_at_bottom:
            case Pokecraft::Direction_move_at_left:
            {
                QList<QPair<quint8, Pokecraft::Direction> > movement;
                QPair<quint8, Pokecraft::Direction> move;
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
void MapControllerMP::loadOtherPlayerFromMap(OtherPlayer otherPlayer)
{
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

    QSet<QString> mapUsed=loadMap(otherPlayer.presumed_map,true);
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

    if(ObjectGroupItem::objectGroupLink.contains(otherPlayer.presumed_map->objectGroup))
        ObjectGroupItem::objectGroupLink[otherPlayer.presumed_map->objectGroup]->addObject(otherPlayer.playerMapObject);
    else
        qDebug() << QString("loadOtherPlayerFromMap(), ObjectGroupItem::objectGroupLink not contains current_map->objectGroup");
    MapObjectItem::objectLink[otherPlayer.playerMapObject]->setZValue(otherPlayer.y);
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

void MapControllerMP::move_player(const quint16 &id, const QList<QPair<quint8, Pokecraft::Direction> > &movement)
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
        QPair<quint8, Pokecraft::Direction> move=movement.at(index_temp);
        moveString << QString("{%1,%2}").arg(move.first).arg(Pokecraft::MoveOnTheMap::directionToString(move.second));
        index_temp++;
    }

    qDebug() << QString("move_player(%1,%2), previous direction: %3").arg(id).arg(moveString.join(";")).arg(Pokecraft::MoveOnTheMap::directionToString(otherPlayerList[id].direction));
    #endif


    //reset to the good position
    otherPlayerList[id].oneStepMore->stop();
    otherPlayerList[id].inMove=false;
    otherPlayerList[id].moveStep=0;
    if(otherPlayerList[id].current_map!=otherPlayerList[id].presumed_map->logicalMap.map_file)
    {
        unloadOtherPlayerFromMap(otherPlayerList[id]);
        QString current_map_fileName=loadOtherMap(otherPlayerList[id].current_map);
        if(current_map_fileName.isEmpty())
        {
            qDebug() << QString("move_player(%1), unable to load the map: %2").arg(id).arg(otherPlayerList[id].current_map);
            return;
        }
        otherPlayerList[id].presumed_map=all_map[current_map_fileName];
        loadOtherPlayerFromMap(otherPlayerList[id]);
    }
    quint8 x=otherPlayerList[id].x;
    quint8 y=otherPlayerList[id].y;
    otherPlayerList[id].presumed_x=x;
    otherPlayerList[id].presumed_y=y;
    otherPlayerList[id].presumed_direction=otherPlayerList[id].direction;
    Pokecraft::Map * map=&otherPlayerList[id].presumed_map->logicalMap;
    Pokecraft::Map * old_map;

    //move to have the new position if needed
    int index=0;
    while(index<movement.size())
    {
        QPair<quint8, Pokecraft::Direction> move=movement.at(index);
        int index2=0;
        while(index2<move.first)
        {
            old_map=map;
            //set the final value (direction, position, ...)
            switch(otherPlayerList[id].presumed_direction)
            {
                case Pokecraft::Direction_move_at_left:
                case Pokecraft::Direction_move_at_right:
                case Pokecraft::Direction_move_at_top:
                case Pokecraft::Direction_move_at_bottom:
                if(Pokecraft::MoveOnTheMap::canGoTo(otherPlayerList[id].presumed_direction,*map,x,y,true))
                    Pokecraft::MoveOnTheMap::move(otherPlayerList[id].presumed_direction,&map,&x,&y);
                else
                {
                    qDebug() << QString("move_player(): at %1(%2,%3) can't go to %4").arg(map->map_file).arg(x).arg(y).arg(Pokecraft::MoveOnTheMap::directionToString(otherPlayerList[id].presumed_direction));
                    return;
                }
                break;
                default:
                qDebug() << QString("move_player(): moveStep: %1, wrong direction: %2").arg(move.first).arg(Pokecraft::MoveOnTheMap::directionToString(otherPlayerList[id].presumed_direction));
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
        case Pokecraft::Direction_look_at_top:
        case Pokecraft::Direction_move_at_top:
            otherPlayerList[id].playerMapObject->setTile(otherPlayerList[id].playerTileset->tileAt(1));
        break;
        case Pokecraft::Direction_look_at_right:
        case Pokecraft::Direction_move_at_right:
            otherPlayerList[id].playerMapObject->setTile(otherPlayerList[id].playerTileset->tileAt(4));
        break;
        case Pokecraft::Direction_look_at_bottom:
        case Pokecraft::Direction_move_at_bottom:
            otherPlayerList[id].playerMapObject->setTile(otherPlayerList[id].playerTileset->tileAt(7));
        break;
        case Pokecraft::Direction_look_at_left:
        case Pokecraft::Direction_move_at_left:
            otherPlayerList[id].playerMapObject->setTile(otherPlayerList[id].playerTileset->tileAt(10));
        break;
        default:
            qDebug() << QString("move_player(): player: %1 (%2), wrong direction: %3").arg(otherPlayerList[id].informations.pseudo).arg(id).arg(otherPlayerList[id].presumed_direction);
        return;
    }
    switch(otherPlayerList[id].presumed_direction)
    {
        case Pokecraft::Direction_move_at_top:
        case Pokecraft::Direction_move_at_right:
        case Pokecraft::Direction_move_at_bottom:
        case Pokecraft::Direction_move_at_left:
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

    delete otherPlayerList[id].playerMapObject;
    delete otherPlayerList[id].playerTileset;
    delete otherPlayerList[id].oneStepMore;

    otherPlayerList.remove(id);
}

void MapControllerMP::reinsert_player(const quint16 &id,const quint8 &x,const quint8 &y,const Pokecraft::Direction &direction)
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

    Pokecraft::Player_public_informations informations=otherPlayerList[id].informations;
    qint32 mapIndex=DatapackClientLoader::datapackLoader.maps.indexOf(otherPlayerList[id].current_map);
    if(mapIndex==-1)
    {
        qDebug() << "internal problem, revert map index is wrong";
        return;
    }
    quint32 mapId=(quint32)mapIndex;
    remove_player(id);
    insert_player(informations,mapId,x,y,direction);
}

void MapControllerMP::reinsert_player(const quint16 &id,const quint32 &mapId,const quint8 &x,const quint8 &y,const Pokecraft::Direction &direction)
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

    Pokecraft::Player_public_informations informations=otherPlayerList[id].informations;
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

void MapControllerMP::teleportTo(const quint32 &mapId,const quint16 &x,const quint16 &y,const Pokecraft::Direction &direction)
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
        qDebug() << QString("delayed teleportTo(%1,%2,%3,%4)").arg(DatapackClientLoader::datapackLoader.maps[mapId]).arg(x).arg(y).arg(Pokecraft::MoveOnTheMap::directionToString(direction));
        #endif
        return;
    }
    if(mapId>=(quint32)DatapackClientLoader::datapackLoader.maps.size())
    {
        qDebug() << "mapId greater than DatapackClientLoader::datapackLoader.maps.size()";
        return;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    qDebug() << QString("teleportTo(%1,%2,%3,%4)").arg(DatapackClientLoader::datapackLoader.maps[mapId]).arg(x).arg(y).arg(Pokecraft::MoveOnTheMap::directionToString(direction));
    #endif
    if(current_map==NULL)
    {
        qDebug() << "Current player not loaded on the map";
        return;
    }

    //the direction
    this->direction=direction;
    switch(direction)
    {
        case Pokecraft::Direction_look_at_top:
        case Pokecraft::Direction_move_at_top:
            playerMapObject->setTile(playerTileset->tileAt(1));
        break;
        case Pokecraft::Direction_look_at_right:
        case Pokecraft::Direction_move_at_right:
            playerMapObject->setTile(playerTileset->tileAt(4));
        break;
        case Pokecraft::Direction_look_at_bottom:
        case Pokecraft::Direction_move_at_bottom:
            playerMapObject->setTile(playerTileset->tileAt(7));
        break;
        case Pokecraft::Direction_look_at_left:
        case Pokecraft::Direction_move_at_left:
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
    QString current_map_fileName=loadOtherMap(datapackMapPath+DatapackClientLoader::datapackLoader.maps[mapId]);
    if(current_map_fileName.isEmpty())
    {
        qDebug() << QString("Unable to open: %1").arg(datapackMapPath+DatapackClientLoader::datapackLoader.maps[mapId]);
        QMessageBox::critical(NULL,"Error",mLastError);
        return;
    }
    current_map=all_map[current_map_fileName];

    mapUsed=loadMap(current_map,true);
    removeUnusedMap();
    loadPlayerFromCurrentMap();

    QStringList map_list;
    QSetIterator<QString> i(displayed_map);
    while (i.hasNext())
        map_list << i.next();
    qDebug() << QString("MapControllerMP::teleportTo(): displayed_map: %1").arg(map_list.join(";"));

    emit teleportDone();
}

//player info
void MapControllerMP::have_current_player_info(const Pokecraft::Player_private_and_public_informations &informations)
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

    skinFolderList=Pokecraft::FacilityLib::skinIdList(datapackPath+DATAPACK_BASE_PATH_SKIN);

    setAnimationTilset(datapackPath+DATAPACK_BASE_PATH_ANIMATION+"animation.png");

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
        case Pokecraft::Direction_move_at_left:
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
        case Pokecraft::Direction_move_at_right:
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
        case Pokecraft::Direction_move_at_top:
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
        case Pokecraft::Direction_move_at_bottom:
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
        Pokecraft::Map * old_map=&otherPlayerList[otherPlayerListByTimer[timer]].presumed_map->logicalMap;
        Pokecraft::Map * map=&otherPlayerList[otherPlayerListByTimer[timer]].presumed_map->logicalMap;
        quint8 x=otherPlayerList[otherPlayerListByTimer[timer]].presumed_x;
        quint8 y=otherPlayerList[otherPlayerListByTimer[timer]].presumed_y;
        //set the final value (direction, position, ...)
        switch(otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction)
        {
            case Pokecraft::Direction_move_at_right:
            case Pokecraft::Direction_move_at_top:
            case Pokecraft::Direction_move_at_bottom:
            case Pokecraft::Direction_move_at_left:
                Pokecraft::MoveOnTheMap::move(otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction,&map,&x,&y);
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
        if(otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction==Pokecraft::Direction_move_at_left)
        {
            //can't go into this direction, then just look into this direction
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_left,otherPlayerList[otherPlayerListByTimer[timer]].presumed_map->logicalMap,x,y,true))
            {
                otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction=Pokecraft::Direction_look_at_left;
                otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject->setTile(otherPlayerList[otherPlayerListByTimer[timer]].playerTileset->tileAt(10));
                otherPlayerList[otherPlayerListByTimer[timer]].inMove=false;
                timer->stop();
            }
            //if can go, then do the move
            else
            {
                otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction=Pokecraft::Direction_move_at_left;
                otherPlayerList[otherPlayerListByTimer[timer]].moveStep=0;
                moveOtherPlayerStepSlot();
            }
        }
        else if(otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction==Pokecraft::Direction_move_at_right)
        {
            //can't go into this direction, then just look into this direction
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_right,otherPlayerList[otherPlayerListByTimer[timer]].presumed_map->logicalMap,x,y,true))
            {
                otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction=Pokecraft::Direction_look_at_right;
                otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject->setTile(otherPlayerList[otherPlayerListByTimer[timer]].playerTileset->tileAt(4));
                otherPlayerList[otherPlayerListByTimer[timer]].inMove=false;
                timer->stop();
            }
            //if can go, then do the move
            else
            {
                otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction=Pokecraft::Direction_move_at_right;
                otherPlayerList[otherPlayerListByTimer[timer]].moveStep=0;
                moveOtherPlayerStepSlot();
            }
        }
        else if(otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction==Pokecraft::Direction_move_at_top)
        {
            //can't go into this direction, then just look into this direction
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_top,otherPlayerList[otherPlayerListByTimer[timer]].presumed_map->logicalMap,x,y,true))
            {
                otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction=Pokecraft::Direction_look_at_top;
                otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject->setTile(otherPlayerList[otherPlayerListByTimer[timer]].playerTileset->tileAt(1));
                otherPlayerList[otherPlayerListByTimer[timer]].inMove=false;
                timer->stop();
            }
            //if can go, then do the move
            else
            {
                otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction=Pokecraft::Direction_move_at_top;
                otherPlayerList[otherPlayerListByTimer[timer]].moveStep=0;
                moveOtherPlayerStepSlot();
            }
        }
        else if(otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction==Pokecraft::Direction_move_at_bottom)
        {
            //can't go into this direction, then just look into this direction
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_bottom,otherPlayerList[otherPlayerListByTimer[timer]].presumed_map->logicalMap,x,y,true))
            {
                otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction=Pokecraft::Direction_look_at_bottom;
                otherPlayerList[otherPlayerListByTimer[timer]].playerMapObject->setTile(otherPlayerList[otherPlayerListByTimer[timer]].playerTileset->tileAt(7));
                otherPlayerList[otherPlayerListByTimer[timer]].inMove=false;
                timer->stop();
            }
            //if can go, then do the move
            else
            {
                otherPlayerList[otherPlayerListByTimer[timer]].presumed_direction=Pokecraft::Direction_move_at_bottom;
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
