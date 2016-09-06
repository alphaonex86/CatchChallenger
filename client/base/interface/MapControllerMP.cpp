#include "MapController.h"
#include "../../../general/base/MoveOnTheMap.h"
#include "../../../general/base/FacilityLib.h"
#include "../../../general/base/FacilityLibGeneral.h"
#include "../../../general/base/CommonSettingsServer.h"
#include "DatapackClientLoader.h"
#include "../ClientVariable.h"
#include "../FacilityLibClient.h"
#include "../Api_client_real.h"

#include <QMessageBox>
#include <QLabel>
#include <QPixmap>
#include <qmath.h>

QFont MapControllerMP::playerpseudofont;
QPixmap * MapControllerMP::imgForPseudoAdmin=NULL;
QPixmap * MapControllerMP::imgForPseudoDev=NULL;
QPixmap * MapControllerMP::imgForPseudoPremium=NULL;

MapControllerMP::MapControllerMP(const bool &centerOnPlayer,const bool &debugTags,const bool &useCache,const bool &OpenGL) :
    MapVisualiserPlayerWithFight(centerOnPlayer,debugTags,useCache,OpenGL)
{
    qRegisterMetaType<CatchChallenger::Direction>("CatchChallenger::Direction");
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_public_informations>("CatchChallenger::Player_public_informations");
    qRegisterMetaType<CatchChallenger::Player_private_and_public_informations>("CatchChallenger::Player_private_and_public_informations");
    qRegisterMetaType<QList<QPair<uint8_t,CatchChallenger::Direction> > >("QList<QPair<uint8_t,CatchChallenger::Direction> >");
    qRegisterMetaType<QList<MapVisualiserThread::Map_full> >("QList<MapVisualiserThread::Map_full>");
    qRegisterMetaType<QList<QPair<CatchChallenger::Direction,uint8_t> > >("QList<QPair<CatchChallenger::Direction,uint8_t> >");
    qRegisterMetaType<QList<QPair<CatchChallenger::Orientation,uint8_t> > >("QList<QPair<CatchChallenger::Orientation,uint8_t> >");
    connect(&pathFinding,&PathFinding::result,this,&MapControllerMP::pathFindingResult);

    playerpseudofont=QFont(QStringLiteral("Arial"));
    playerpseudofont.setPixelSize(14);
    player_informations_is_set=false;

    resetAll();

    scaleSize=1;
}

MapControllerMP::~MapControllerMP()
{
    pathFinding.cancel();
}

void MapControllerMP::connectAllSignals()
{
    //connect the map controler
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::have_current_player_info,   this,&MapControllerMP::have_current_player_info,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::insert_player,              this,&MapControllerMP::insert_player,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::remove_player,              this,&MapControllerMP::remove_player,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::move_player,                this,&MapControllerMP::move_player,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_protocol::reinsert_player,               this,&MapControllerMP::reinsert_player,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_protocol::reinsert_player,               this,&MapControllerMP::reinsert_player,Qt::QueuedConnection);
    connect(this,&MapControllerMP::send_player_direction,CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::send_player_direction);//,Qt::QueuedConnection
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::teleportTo,                 this,&MapControllerMP::teleportTo,Qt::QueuedConnection);
}

void MapControllerMP::resetAll()
{
    if(!playerTileset->loadFromImage(QImage(QStringLiteral(":/images/player_default/trainer.png")),QStringLiteral(":/images/player_default/trainer.png")))
        qDebug() << "Unable the load the default player tileset";

    unloadPlayerFromCurrentMap();
    current_map.clear();
    pathList.clear();
    delayedActions.clear();
    skinFolderList.clear();

    QHashIterator<uint16_t,OtherPlayer> i(otherPlayerList);
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
    isTeleported=true;

    MapVisualiserPlayer::resetAll();
}

void MapControllerMP::setScale(const float &scaleSize)
{
    if(scaleSize<1 || scaleSize>4)
    {
        qDebug() << QStringLiteral("scaleSize out of range 1-4: %1").arg(scaleSize);
        return;
    }
    scale(scaleSize/this->scaleSize,scaleSize/this->scaleSize);
    this->scaleSize=scaleSize;
}

bool MapControllerMP::loadPlayerMap(const QString &fileName,const uint8_t &x,const uint8_t &y)
{
    //position
    this->x=x;
    this->y=y;
    QFileInfo fileInformations(fileName);
    current_map=fileInformations.absoluteFilePath();
    mapVisualiserThread.stopIt=false;
    loadOtherMap(current_map);
    return true;
}

void MapControllerMP::insert_player(const CatchChallenger::Player_public_informations &player,const uint32_t &mapId,const uint16_t &x,const uint16_t &y,const CatchChallenger::Direction &direction)
{
    insert_player_final(player,mapId,x,y,direction,false);
}

void MapControllerMP::move_player(const uint16_t &id, const QList<QPair<uint8_t,CatchChallenger::Direction> > &movement)
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
bool MapControllerMP::insert_player_final(const CatchChallenger::Player_public_informations &player,const uint32_t &mapId,const uint16_t &x,const uint16_t &y,const CatchChallenger::Direction &direction,bool inReplayMode)
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
            delayedActions << multiplex;
        }
        #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
        qDebug() << QStringLiteral("delayed: insert_player(%1->%2,%3,%4,%5,%6)").arg(player.pseudo).arg(player.simplifiedId).arg(mapId).arg(x).arg(y).arg(CatchChallenger::MoveOnTheMap::directionToString(direction));
        #endif
        return false;
    }
    if(mapId>=(uint32_t)DatapackClientLoader::datapackLoader.maps.size())
    {
        /// \bug here pass after delete a party, create a new
        emit error("mapId greater than DatapackClientLoader::datapackLoader.maps.size(): "+QString::number(DatapackClientLoader::datapackLoader.maps.size()));
        return true;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    qDebug() << QStringLiteral("insert_player(%1->%2,%3,%4,%5,%6)").arg(player.pseudo).arg(player.simplifiedId).arg(DatapackClientLoader::datapackLoader.maps.value(mapId)).arg(x).arg(y).arg(CatchChallenger::MoveOnTheMap::directionToString(direction));
    #endif
    if(player.simplifiedId==player_informations.public_informations.simplifiedId)
    {
        //ignore to improve the performance server because can reinsert all player of map using the overall client list
        if(!current_map.isEmpty())
        {
            qDebug() << "Current player already loaded on the map";
            return true;
        }
        //the player skin
        if(player.skinId<skinFolderList.size())
        {
            playerSkinPath=datapackPath+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(player.skinId);
            const QString &imagePath=playerSkinPath+MapControllerMP::text_slashtrainerpng;
            QImage image(imagePath);
            if(!image.isNull())
                playerTileset->loadFromImage(image,imagePath);
            else
                qDebug() << "Unable to load the player tilset: "+imagePath;
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

        loadPlayerMap(datapackMapPathSpec+DatapackClientLoader::datapackLoader.maps.value(mapId),x,y);
        setSpeed(player.speed);
    }
    else
    {
        if(otherPlayerList.contains(player.simplifiedId))
        {
            qDebug() << QStringLiteral("Other player (%1) already loaded on the map").arg(player.simplifiedId);
            //return true;-> ignored to fix temporally, but need remove at map unload
        }
        OtherPlayer tempPlayer;
        tempPlayer.x=x;
        tempPlayer.y=y;
        tempPlayer.direction=direction;
        tempPlayer.moveStep=0;
        tempPlayer.inMove=false;
        tempPlayer.stepAlternance=false;

        const QString &mapPath=QFileInfo(datapackMapPathSpec+DatapackClientLoader::datapackLoader.maps.value(mapId)).absoluteFilePath();
        if(!all_map.contains(mapPath))
        {
            qDebug() << "MapControllerMP::insert_player(): current map " << mapPath << " not loaded, delayed: ";
            QHash<QString,MapVisualiserThread::Map_full *>::const_iterator i = all_map.constBegin();
            while (i != all_map.constEnd()) {
                qDebug() << i.key();
                ++i;
            }
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
                delayedActions << multiplex;
            }
            return false;
        }
        //the player skin
        if(player.skinId<skinFolderList.size())
        {
            QImage image(datapackPath+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(player.skinId)+MapControllerMP::text_slashtrainerpng);
            if(!image.isNull())
            {
                tempPlayer.playerMapObject = new Tiled::MapObject();
                tempPlayer.playerMapObject->setName("Other player");
                tempPlayer.playerTileset = new Tiled::Tileset(skinFolderList.at(player.skinId),16,24);
                tempPlayer.playerTileset->loadFromImage(image,datapackPath+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(player.skinId)+MapControllerMP::text_slashtrainerpng);
            }
            else
            {
                qDebug() << "Unable to load the player tilset: "+datapackPath+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(player.skinId)+MapControllerMP::text_slashtrainerpng;
                return true;
            }
        }
        else
        {
            qDebug() << QStringLiteral("The skin id: ")+QString::number(player.skinId)+QStringLiteral(", into a list of: ")+QString::number(skinFolderList.size())+QStringLiteral(" item(s) info MapControllerMP::insert_player()");
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
        tempPlayer.presumed_map=all_map.value(mapPath);
        tempPlayer.presumed_x=x;
        tempPlayer.presumed_y=y;
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
                QList<QPair<uint8_t, CatchChallenger::Direction> > movement;
                QPair<uint8_t, CatchChallenger::Direction> move;
                move.first=0;
                move.second=direction;
                movement << move;
                move_player_final(player.simplifiedId,movement,inReplayMode);
            }
            break;
            default:
            break;
        }
        return true;
    }
    return true;
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
        {
            ObjectGroupItem::objectGroupLink.value(currentGroup)->removeObject(otherPlayer.playerMapObject);
            if(otherPlayer.labelMapObject!=NULL)
                ObjectGroupItem::objectGroupLink.value(currentGroup)->removeObject(otherPlayer.labelMapObject);
        }
        if(currentGroup!=otherPlayer.presumed_map->objectGroup)
            qDebug() << QStringLiteral("loadOtherPlayerFromMap(), the playerMapObject group is wrong: %1").arg(currentGroup->name());
    }

    //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
    otherPlayer.playerMapObject->setPosition(QPoint(otherPlayer.x,otherPlayer.y+1));
    if(otherPlayer.labelMapObject!=NULL)
        otherPlayer.labelMapObject->setPosition(QPointF((float)otherPlayer.x-(float)otherPlayer.labelTileset->tileWidth()/2/16+0.5,otherPlayer.y+1-1.4));

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
    centerOn(MapObjectItem::objectLink.value(playerMapObject));

    if(ObjectGroupItem::objectGroupLink.contains(otherPlayer.presumed_map->objectGroup))
    {
        ObjectGroupItem::objectGroupLink.value(otherPlayer.presumed_map->objectGroup)->addObject(otherPlayer.playerMapObject);
        if(otherPlayer.labelMapObject!=NULL)
            ObjectGroupItem::objectGroupLink.value(otherPlayer.presumed_map->objectGroup)->addObject(otherPlayer.labelMapObject);
        if(!MapObjectItem::objectLink.contains(otherPlayer.playerMapObject))
            qDebug() << QStringLiteral("loadOtherPlayerFromMap(), MapObjectItem::objectLink don't have otherPlayer.playerMapObject");
        else
        {
            if(MapObjectItem::objectLink.value(otherPlayer.playerMapObject)==NULL)
                qDebug() << QStringLiteral("loadOtherPlayerFromMap(), MapObjectItem::objectLink[otherPlayer.playerMapObject]==NULL");
            else
            {
                MapObjectItem::objectLink.value(otherPlayer.playerMapObject)->setZValue(otherPlayer.y);
                if(otherPlayer.labelMapObject!=NULL)
                    MapObjectItem::objectLink.value(otherPlayer.labelMapObject)->setZValue(otherPlayer.y);
            }
        }
    }
    else
        qDebug() << QStringLiteral("loadOtherPlayerFromMap(), ObjectGroupItem::objectGroupLink not contains current_map->objectGroup");
}

//call before leave the old map (and before loadPlayerFromCurrentMap())
void MapControllerMP::unloadOtherPlayerFromMap(OtherPlayer otherPlayer)
{
    //unload the player sprite
    if(ObjectGroupItem::objectGroupLink.contains(otherPlayer.playerMapObject->objectGroup()))
        ObjectGroupItem::objectGroupLink.value(otherPlayer.playerMapObject->objectGroup())->removeObject(otherPlayer.playerMapObject);
    else
        qDebug() << QStringLiteral("unloadOtherPlayerFromMap(), ObjectGroupItem::objectGroupLink not contains otherPlayer.playerMapObject->objectGroup()");
    if(otherPlayer.labelMapObject!=NULL)
    {
        //unload the player sprite
        if(ObjectGroupItem::objectGroupLink.contains(otherPlayer.labelMapObject->objectGroup()))
            ObjectGroupItem::objectGroupLink.value(otherPlayer.labelMapObject->objectGroup())->removeObject(otherPlayer.labelMapObject);
        else
            qDebug() << QStringLiteral("unloadOtherPlayerFromMap(), ObjectGroupItem::objectGroupLink not contains otherPlayer.labelMapObject->objectGroup()");
    }
    /* done into remove_player_final()
    {
        const uint16_t &simplifiedId=otherPlayer.informations.simplifiedId;
        if(otherPlayerList.contains(simplifiedId))
            otherPlayerList.remove(simplifiedId);
        else
            qDebug() << QStringLiteral("simplifiedId: %1 not found into otherPlayerList").arg(simplifiedId);
    }*/

    QSetIterator<QString> i(otherPlayer.mapUsed);
    while (i.hasNext())
    {
        QString map = i.next();
        if(mapUsedByOtherPlayer.contains(map))
        {
            mapUsedByOtherPlayer[map]--;
            if(mapUsedByOtherPlayer.value(map)==0)
                mapUsedByOtherPlayer.remove(map);
        }
        else
            qDebug() << QStringLiteral("map not found into mapUsedByOtherPlayer for player: %1, map: %2").arg(otherPlayer.informations.simplifiedId).arg(map);
    }
}

bool MapControllerMP::move_player_final(const uint16_t &id, const QList<QPair<uint8_t, CatchChallenger::Direction> > &movement,bool inReplayMode)
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
            delayedActions << multiplex;
        }
        return false;
    }
    if(id==player_informations.public_informations.simplifiedId)
    {
        qDebug() << "The current player can't be moved (only teleported)";
        return true;
    }
    if(!otherPlayerList.contains(id))
    {
        qDebug() << QStringLiteral("Other player (%1) not loaded on the map").arg(id);
        return true;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    QStringList moveString;
    int index_temp=0;
    while(index_temp<movement.size())
    {
        QPair<uint8_t, CatchChallenger::Direction> move=movement.at(index_temp);
        moveString << QStringLiteral("{%1,%2}").arg(move.first).arg(CatchChallenger::MoveOnTheMap::directionToString(move.second));
        index_temp++;
    }

    qDebug() << QStringLiteral("move_player(%1,%2), previous direction: %3").arg(id).arg(moveString.join(";")).arg(CatchChallenger::MoveOnTheMap::directionToString(otherPlayerList.value(id).direction));
    #endif


    //reset to the good position
    otherPlayerList.value(id).oneStepMore->stop();
    otherPlayerList[id].inMove=false;
    otherPlayerList[id].moveStep=0;
    if(otherPlayerList.value(id).current_map!=QString::fromStdString(otherPlayerList.value(id).presumed_map->logicalMap.map_file))
    {
        unloadOtherPlayerFromMap(otherPlayerList.value(id));
        QString mapPath=otherPlayerList.value(id).current_map;
        if(!haveMapInMemory(mapPath))
        {
            qDebug() << QStringLiteral("move_player(%1), map not already loaded").arg(id).arg(otherPlayerList.value(id).current_map);
            if(!inReplayMode)
            {
                DelayedMove tempItem;
                tempItem.id=id;
                tempItem.movement=movement;

                DelayedMultiplex multiplex;
                multiplex.move=tempItem;
                multiplex.type=DelayedType_Move;
                delayedActions << multiplex;
            }
            return false;
        }
        loadOtherMap(mapPath);
        otherPlayerList[id].presumed_map=all_map.value(mapPath);
        loadOtherPlayerFromMap(otherPlayerList.value(id));
    }
    uint8_t x=otherPlayerList.value(id).x;
    uint8_t y=otherPlayerList.value(id).y;
    otherPlayerList[id].presumed_x=x;
    otherPlayerList[id].presumed_y=y;
    otherPlayerList[id].presumed_direction=otherPlayerList.value(id).direction;
    CatchChallenger::CommonMap * map=&otherPlayerList.value(id).presumed_map->logicalMap;
    CatchChallenger::CommonMap * old_map;

    //move to have the new position if needed
    int index=0;
    while(index<movement.size())
    {
        QPair<uint8_t, CatchChallenger::Direction> move=movement.at(index);
        int index2=0;
        while(index2<move.first)
        {
            old_map=map;
            //set the final value (direction, position, ...)
            switch(otherPlayerList.value(id).presumed_direction)
            {
                case CatchChallenger::Direction_move_at_left:
                case CatchChallenger::Direction_move_at_right:
                case CatchChallenger::Direction_move_at_top:
                case CatchChallenger::Direction_move_at_bottom:
                if(CatchChallenger::MoveOnTheMap::canGoTo(otherPlayerList.value(id).presumed_direction,*map,x,y,true))
                    CatchChallenger::MoveOnTheMap::move(otherPlayerList.value(id).presumed_direction,&map,&x,&y);
                else
                {
                    qDebug() << QStringLiteral("move_player(): at %1(%2,%3) can't go to %4").arg(QString::fromStdString(map->map_file)).arg(x).arg(y).arg(QString::fromStdString(CatchChallenger::MoveOnTheMap::directionToString(otherPlayerList.value(id).presumed_direction)));
                    return true;
                }
                break;
                default:
                qDebug() << QStringLiteral("move_player(): moveStep: %1, wrong direction: %2").arg(move.first).arg(QString::fromStdString(CatchChallenger::MoveOnTheMap::directionToString(otherPlayerList.value(id).presumed_direction)));
                return true;
            }
            //if the map have changed
            if(old_map!=map)
            {
                loadOtherMap(QString::fromStdString(map->map_file));
                if(!all_map.contains(QString::fromStdString(map->map_file)))
                    qDebug() << QStringLiteral("map changed not located: %1").arg(QString::fromStdString(map->map_file));
                else
                {
                    unloadOtherPlayerFromMap(otherPlayerList.value(id));
                    otherPlayerList[id].presumed_map=all_map.value(QString::fromStdString(map->map_file));
                    loadOtherPlayerFromMap(otherPlayerList.value(id));
                }
            }
            index2++;
        }
        otherPlayerList[id].direction=move.second;
        index++;
    }


    //set the new variables
    otherPlayerList[id].current_map=QString::fromStdString(map->map_file);
    otherPlayerList[id].x=x;
    otherPlayerList[id].y=y;

    otherPlayerList[id].presumed_map=all_map.value(otherPlayerList.value(id).current_map);
    otherPlayerList[id].presumed_x=otherPlayerList.value(id).x;
    otherPlayerList[id].presumed_y=otherPlayerList.value(id).y;
    otherPlayerList[id].presumed_direction=otherPlayerList.value(id).direction;

    //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
    otherPlayerList.value(id).playerMapObject->setPosition(QPoint(otherPlayerList.value(id).presumed_x,otherPlayerList.value(id).presumed_y+1));
    if(otherPlayerList.value(id).labelMapObject!=NULL)
    {
        otherPlayerList.value(id).labelMapObject->setPosition(QPointF((float)otherPlayerList.value(id).presumed_x-(float)otherPlayerList.value(id).labelTileset->tileWidth()/2/16+0.5,otherPlayerList.value(id).presumed_y+1-1.4));
        MapObjectItem::objectLink.value(otherPlayerList.value(id).labelMapObject)->setZValue(otherPlayerList.value(id).presumed_y);
    }
    MapObjectItem::objectLink.value(otherPlayerList.value(id).playerMapObject)->setZValue(otherPlayerList.value(id).presumed_y);

    //start moving into the right direction
    switch(otherPlayerList.value(id).presumed_direction)
    {
        case CatchChallenger::Direction_look_at_top:
        case CatchChallenger::Direction_move_at_top:
        {
            Tiled::Cell cell=otherPlayerList.value(id).playerMapObject->cell();
            cell.tile=otherPlayerList.value(id).playerTileset->tileAt(1);
            otherPlayerList.value(id).playerMapObject->setCell(cell);
        }
        break;
        case CatchChallenger::Direction_look_at_right:
        case CatchChallenger::Direction_move_at_right:
        {
            Tiled::Cell cell=otherPlayerList.value(id).playerMapObject->cell();
            cell.tile=otherPlayerList.value(id).playerTileset->tileAt(4);
            otherPlayerList.value(id).playerMapObject->setCell(cell);
        }
        break;
        case CatchChallenger::Direction_look_at_bottom:
        case CatchChallenger::Direction_move_at_bottom:
        {
            Tiled::Cell cell=otherPlayerList.value(id).playerMapObject->cell();
            cell.tile=otherPlayerList.value(id).playerTileset->tileAt(7);
            otherPlayerList.value(id).playerMapObject->setCell(cell);
        }
        break;
        case CatchChallenger::Direction_look_at_left:
        case CatchChallenger::Direction_move_at_left:
        {
            Tiled::Cell cell=otherPlayerList.value(id).playerMapObject->cell();
            cell.tile=otherPlayerList.value(id).playerTileset->tileAt(10);
            otherPlayerList.value(id).playerMapObject->setCell(cell);
        }
        break;
        default:
            qDebug() << QStringLiteral("move_player(): player: %1 (%2), wrong direction: %3").arg(QString::fromStdString(otherPlayerList.value(id).informations.pseudo)).arg(id).arg(otherPlayerList.value(id).presumed_direction);
        return true;
    }
    switch(otherPlayerList.value(id).presumed_direction)
    {
        case CatchChallenger::Direction_move_at_top:
        case CatchChallenger::Direction_move_at_right:
        case CatchChallenger::Direction_move_at_bottom:
        case CatchChallenger::Direction_move_at_left:
            otherPlayerList.value(id).oneStepMore->start(otherPlayerList.value(id).informations.speed/5);
        break;
        default:
        break;
    }
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
            delayedActions << multiplex;
        }
        return false;
    }
    if(!otherPlayerList.contains(id))
    {
        qDebug() << QStringLiteral("Other player (%1) not exists").arg(id);
        return true;
    }
    {
        int index=0;
        while(index<delayedActions.size())
        {
            switch(delayedActions.at(index).type)
            {
                case DelayedType_Insert:
                    if(delayedActions.at(index).insert.player.simplifiedId==id)
                        delayedActions.removeAt(index);
                    else
                        index++;
                break;
                case DelayedType_Move:
                    if(delayedActions.at(index).move.id==id)
                        delayedActions.removeAt(index);
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
    unloadOtherPlayerFromMap(otherPlayerList.value(id));

    otherPlayerListByTimer.remove(otherPlayerList.value(id).oneStepMore);
    otherPlayerListByAnimationTimer.remove(otherPlayerList.value(id).moveAnimationTimer);

    Tiled::ObjectGroup *currentGroup=otherPlayerList.value(id).playerMapObject->objectGroup();
    if(currentGroup!=NULL)
    {
        if(ObjectGroupItem::objectGroupLink.contains(currentGroup))
        {
            ObjectGroupItem::objectGroupLink.value(currentGroup)->removeObject(otherPlayerList.value(id).playerMapObject);
            if(otherPlayerList.value(id).labelMapObject!=NULL)
                ObjectGroupItem::objectGroupLink.value(currentGroup)->removeObject(otherPlayerList.value(id).labelMapObject);
        }
        if(currentGroup!=otherPlayerList.value(id).presumed_map->objectGroup)
            qDebug() << QStringLiteral("loadOtherPlayerFromMap(), the playerMapObject group is wrong: %1").arg(currentGroup->name());
        currentGroup->removeObject(otherPlayerList.value(id).playerMapObject);
        if(otherPlayerList.value(id).labelMapObject!=NULL)
            currentGroup->removeObject(otherPlayerList.value(id).labelMapObject);
    }

    delete otherPlayerList.value(id).playerMapObject;
    delete otherPlayerList.value(id).playerTileset;
    delete otherPlayerList.value(id).oneStepMore;
    delete otherPlayerList.value(id).moveAnimationTimer;
    if(otherPlayerList.value(id).labelMapObject!=NULL)
        delete otherPlayerList.value(id).labelMapObject;
    if(otherPlayerList.value(id).labelTileset!=NULL)
        delete otherPlayerList.value(id).labelTileset;

    otherPlayerList.remove(id);
    return true;
}

bool MapControllerMP::reinsert_player_final(const uint16_t &id,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction,bool inReplayMode)
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
            delayedActions << multiplex;
        }
        return false;
    }
    if(id==player_informations.public_informations.simplifiedId)
    {
        qDebug() << "The current player can't be removed";
        return true;
    }
    if(!otherPlayerList.contains(id))
    {
        qDebug() << QStringLiteral("Other player (%1) not exists").arg(id);
        return true;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    qDebug() << QStringLiteral("reinsert_player(%1)").arg(id);
    #endif

    CatchChallenger::Player_public_informations informations=otherPlayerList.value(id).informations;
    /// \warning search by loop because otherPlayerList.value(id).current_map is the full path, DatapackClientLoader::datapackLoader.maps relative path
    QString tempCurrentMap=otherPlayerList.value(id).current_map;
    //if not found, search into sub
    if(!all_map.contains(tempCurrentMap) && !CatchChallenger::Api_client_real::client->subDatapackCode().isEmpty())
    {
        QString tempCurrentMapSub=tempCurrentMap;
        tempCurrentMapSub.remove(CatchChallenger::Api_client_real::client->datapackPathSub());
        if(all_map.contains(tempCurrentMapSub))
            tempCurrentMap=tempCurrentMapSub;
    }
    //if not found, search into main
    if(!all_map.contains(tempCurrentMap))
    {
        QString tempCurrentMapMain=tempCurrentMap;
        tempCurrentMapMain.remove(CatchChallenger::Api_client_real::client->datapackPathMain());
        if(all_map.contains(tempCurrentMapMain))
            tempCurrentMap=tempCurrentMapMain;
    }
    //if remain not found
    if(!all_map.contains(tempCurrentMap))
    {
        qDebug() << "internal problem, revert map (" << otherPlayerList.value(id).current_map << ") index is wrong (" << DatapackClientLoader::datapackLoader.maps.join(";") << ")";
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
            delayedActions << multiplex;
        }
        return false;
    }
    uint32_t mapId=(uint32_t)all_map.value(tempCurrentMap)->logicalMap.id;
    if(mapId==0)
        qDebug() << QStringLiteral("supected NULL map then error");
    remove_player_final(id,inReplayMode);
    insert_player_final(informations,mapId,x,y,direction,inReplayMode);
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
            delayedActions << multiplex;
        }
        return false;
    }
    if(id==player_informations.public_informations.simplifiedId)
    {
        qDebug() << "The current player can't be removed";
        return true;
    }
    if(!otherPlayerList.contains(id))
    {
        qDebug() << QStringLiteral("Other player (%1) not exists").arg(id);
        return true;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    qDebug() << QStringLiteral("reinsert_player(%1)").arg(id);
    #endif

    CatchChallenger::Player_public_informations informations=otherPlayerList.value(id).informations;
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
            delayedActions << multiplex;
        }
        return false;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    qDebug() << QStringLiteral("dropAllPlayerOnTheMap()");
    #endif
    QList<uint16_t> temIdList;
    QHashIterator<uint16_t,OtherPlayer> i(otherPlayerList);
    while (i.hasNext()) {
        i.next();
        temIdList << i.key();
    }
    int index=0;
    while(index<temIdList.size())
    {
        remove_player_final(temIdList.at(index),inReplayMode);
        index++;
    }
    return true;
}

void MapControllerMP::teleportTo(const uint32_t &mapId,const uint16_t &x,const uint16_t &y,const CatchChallenger::Direction &direction)
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
        qDebug() << QStringLiteral("delayed teleportTo(%1,%2,%3,%4)").arg(DatapackClientLoader::datapackLoader.maps.value(mapId)).arg(x).arg(y).arg(CatchChallenger::MoveOnTheMap::directionToString(direction));
        #endif
        return;
    }
    if(mapId>=(uint32_t)DatapackClientLoader::datapackLoader.maps.size())
    {
        emit error("mapId greater than DatapackClientLoader::datapackLoader.maps.size(): "+QString::number(DatapackClientLoader::datapackLoader.maps.size()));
        return;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    qDebug() << QStringLiteral("teleportTo(%1,%2,%3,%4)").arg(DatapackClientLoader::datapackLoader.maps.value(mapId)).arg(x).arg(y).arg(CatchChallenger::MoveOnTheMap::directionToString(direction));
    qDebug() << QStringLiteral("currently on: %1 (%2,%3)").arg(current_map).arg(this->x).arg(this->y);
    #endif
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
        return;
    }

    CatchChallenger::Api_client_real::client->teleportDone();

    //position
    this->x=x;
    this->y=y;

    unloadPlayerFromCurrentMap();
    current_map=QFileInfo(datapackMapPathSpec+DatapackClientLoader::datapackLoader.maps.value(mapId)).absoluteFilePath();
    passMapIntoOld();
    if(!haveMapInMemory(current_map))
    {
        emit inWaitingOfMap();
        loadOtherMap(current_map);
        return;//because the rest is wrong
    }
    loadOtherMap(current_map);
    hideNotloadedMap();
    removeUnusedMap();
    loadPlayerFromCurrentMap();
}

void MapControllerMP::finalPlayerStep()
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
                current_map=QString::fromStdString(current_teleport.map);
                x=current_teleport.destination_x;
                y=current_teleport.destination_y;
                if(!haveMapInMemory(current_map))
                    emit inWaitingOfMap();
                loadOtherMap(current_map);
                hideNotloadedMap();
                return;
            }
            index++;
        }
    }
    else
        isTeleported=false;
    MapVisualiserPlayer::finalPlayerStep();
}

//player info
void MapControllerMP::have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations)
{
    #ifdef DEBUG_CLIENT_LOAD_ORDER
    qDebug() << QStringLiteral("MapControllerMP::have_current_player_info()");
    #endif

    if(player_informations_is_set)
    {
        //qDebug() << "player information already set";
        return;
    }
    this->player_informations=informations;
    player_informations_is_set=true;

    if(mHaveTheDatapack)
        reinject_signals();
}

void MapControllerMP::datapackParsed()
{
    MapVisualiserPlayer::datapackParsed();
}

void MapControllerMP::datapackParsedMainSub()
{
    MapVisualiserPlayer::datapackParsedMainSub();

    skinFolderList=CatchChallenger::FacilityLibClient::stdvectorstringToQStringList(CatchChallenger::FacilityLibGeneral::skinIdList((datapackPath+DATAPACK_BASE_PATH_SKIN).toStdString()));

    if(player_informations_is_set)
        reinject_signals();
}

void MapControllerMP::reinject_signals()
{
    #ifdef DEBUG_CLIENT_LOAD_ORDER
    qDebug() << QStringLiteral("MapControllerMP::reinject_signals()");
    #endif
    int index=0;

    if(mHaveTheDatapack && player_informations_is_set)
    {
        #ifdef DEBUG_CLIENT_LOAD_ORDER
        qDebug() << QStringLiteral("MapControllerMP::reinject_signals(): mHaveTheDatapack && player_informations_is_set");
        #endif
        index=0;
        const int delayedActions_size=delayedActions.size();
        while(index<delayedActions.size())
        {
            if(index>50000)
            {
                qDebug() << QStringLiteral("Too hight delayedActions");
                abort();
            }
            switch(delayedActions.at(index).type)
            {
                case DelayedType_Insert:
                    if(insert_player_final(delayedActions.at(index).insert.player,delayedActions.at(index).insert.mapId,delayedActions.at(index).insert.x,delayedActions.at(index).insert.y,delayedActions.at(index).insert.direction,true))
                        delayedActions.removeAt(index);
                break;
                case DelayedType_Move:
                    if(move_player_final(delayedActions.at(index).move.id,delayedActions.at(index).move.movement,true))
                        delayedActions.removeAt(index);
                break;
                case DelayedType_Remove:
                    if(remove_player_final(delayedActions.at(index).remove,true))
                        delayedActions.removeAt(index);
                break;
                case DelayedType_Reinsert_single:
                    if(reinsert_player_final(delayedActions.at(index).reinsert_single.id,delayedActions.at(index).reinsert_single.x,delayedActions.at(index).reinsert_single.y,delayedActions.at(index).reinsert_single.direction,true))
                        delayedActions.removeAt(index);
                break;
                case DelayedType_Reinsert_full:
                    if(full_reinsert_player_final(delayedActions.at(index).reinsert_full.id,delayedActions.at(index).reinsert_full.mapId,delayedActions.at(index).reinsert_full.x,delayedActions.at(index).reinsert_full.y,delayedActions.at(index).reinsert_full.direction,true))
                        delayedActions.removeAt(index);
                break;
                case DelayedType_Drop_all:
                    if(dropAllPlayerOnTheMap_final(true))
                        delayedActions.removeAt(index);
                break;
                default:
                break;
            }
            if(delayedActions.size()>delayedActions_size)
            {
                qDebug() << QStringLiteral("MapControllerMP::reinject_signals(): can't grow delayedActions");
                abort();
            }
            index++;
        }

        index=0;
        while(index<delayedTeleportTo.size())
        {
            teleportTo(delayedTeleportTo.at(index).mapId,delayedTeleportTo.at(index).x,delayedTeleportTo.at(index).y,delayedTeleportTo.at(index).direction);
            index++;
        }
        delayedTeleportTo.clear();
    }
    else
        qDebug() << QStringLiteral("MapControllerMP::reinject_signals(): should not pass here because all is not previously loaded");
}

void MapControllerMP::reinject_signals_on_valid_map()
{
    #ifdef DEBUG_CLIENT_LOAD_ORDER
    qDebug() << QStringLiteral("MapControllerMP::reinject_signals_on_valid_map()");
    #endif
    int index;

    if(mHaveTheDatapack && player_informations_is_set)
    {
        #ifdef DEBUG_CLIENT_LOAD_ORDER
        qDebug() << QStringLiteral("MapControllerMP::reinject_signals_on_valid_map(): mHaveTheDatapack && player_informations_is_set");
        #endif
        index=0;
        const int delayedActions_size=delayedActions.size();
        while(index<delayedActions.size())
        {
            if(index>50000)
            {
                qDebug() << QStringLiteral("Too hight delayedActions");
                abort();
            }
            switch(delayedActions.at(index).type)
            {
                case DelayedType_Insert:
                if(delayedActions.at(index).insert.player.simplifiedId!=player_informations.public_informations.simplifiedId)
                {
                    const QString &mapPath=QFileInfo(datapackMapPathSpec+DatapackClientLoader::datapackLoader.maps.value(delayedActions.at(index).insert.mapId)).absoluteFilePath();
                    if(all_map.contains(mapPath))
                    {
                        if(insert_player_final(delayedActions.at(index).insert.player,delayedActions.at(index).insert.mapId,delayedActions.at(index).insert.x,delayedActions.at(index).insert.y,delayedActions.at(index).insert.direction,true))
                            delayedActions.removeAt(index);
                        index--;
                    }
                }
                break;
                case DelayedType_Move:
                    if(otherPlayerList.contains(delayedActions.at(index).move.id))
                    {
                        if(move_player_final(delayedActions.at(index).move.id,delayedActions.at(index).move.movement,true))
                            delayedActions.removeAt(index);
                        index--;
                    }
                break;
                default:
                break;
            }
            if(delayedActions.size()>delayedActions_size)
            {
                qDebug() << QStringLiteral("MapControllerMP::reinject_signals(): can't grow delayedActions");
                abort();
            }
            index++;
        }
    }
    else
        qDebug() << QStringLiteral("MapControllerMP::reinject_signals_on_valid_map(): should not pass here because all is not previously loaded");
}

bool MapControllerMP::asyncMapLoaded(const QString &fileName,MapVisualiserThread::Map_full * tempMapObject)
{
    if(!mHaveTheDatapack)
        return false;
    if(!player_informations_is_set)
        return false;
    const bool &result=MapVisualiserPlayer::asyncMapLoaded(fileName,tempMapObject);
    reinject_signals_on_valid_map();
    return result;
}

void MapControllerMP::doMoveOtherAnimation()
{
    QTimer *timer=qobject_cast<QTimer *>(QObject::sender());
    if(timer==NULL)
    {
        qDebug() << "moveOtherPlayerStepSlot() timer not located";
        return;
    }
    const uint16_t &simplifiedId=otherPlayerListByAnimationTimer.value(timer);
    moveOtherPlayerStepSlotWithPlayer(otherPlayerList[simplifiedId]);
}

void MapControllerMP::moveOtherPlayerStepSlot()
{
    QTimer *timer=qobject_cast<QTimer *>(QObject::sender());
    if(timer==NULL)
    {
        qDebug() << "moveOtherPlayerStepSlot() timer not located";
        return;
    }
    const uint16_t &simplifiedId=otherPlayerListByTimer.value(timer);
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
        if(all_map.contains(QString::fromStdString(map->map_file)))
            if(all_map.value(QString::fromStdString(map->map_file))->doors.contains(QPair<uint8_t,uint8_t>(x,y)))
            {
                MapDoor* door=all_map.value(QString::fromStdString(map->map_file))->doors.value(QPair<uint8_t,uint8_t>(x,y));
                door->startOpen(otherPlayer.playerSpeed);
                otherPlayer.moveAnimationTimer->start(door->timeToOpen());
                return;
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
        MapObjectItem::objectLink.value(otherPlayer.playerMapObject)->setZValue(qCeil(otherPlayer.playerMapObject->y()));
        break;
        //transition step
        case 2:
        {
            Tiled::Cell cell=otherPlayer.playerMapObject->cell();
            if(stepAlternance)
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
            loadOtherMap(QString::fromStdString(map->map_file));
            if(!all_map.contains(QString::fromStdString(map->map_file)))
                qDebug() << QStringLiteral("map changed not located: %1").arg(QString::fromStdString(map->map_file));
            else
            {
                unloadOtherPlayerFromMap(otherPlayer);
                otherPlayer.presumed_map=all_map.value(QString::fromStdString(map->map_file));
                loadOtherPlayerFromMap(otherPlayer);
            }
        }
        //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
        otherPlayer.playerMapObject->setPosition(QPoint(x,y+1));
        if(otherPlayer.labelMapObject!=NULL)
            otherPlayer.labelMapObject->setPosition(QPointF((float)x-(float)otherPlayer.labelTileset->tileWidth()/2/16+0.5,y+1-1.4));
        MapObjectItem::objectLink.value(otherPlayer.playerMapObject)->setZValue(y);

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
            }
            //if can go, then do the move
            else
            {
                otherPlayer.presumed_direction=CatchChallenger::Direction_move_at_left;
                otherPlayer.moveStep=0;
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
            }
            //if can go, then do the move
            else
            {
                otherPlayer.presumed_direction=CatchChallenger::Direction_move_at_right;
                otherPlayer.moveStep=0;
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
            }
            //if can go, then do the move
            else
            {
                otherPlayer.presumed_direction=CatchChallenger::Direction_move_at_top;
                otherPlayer.moveStep=0;
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
            }
            //if can go, then do the move
            else
            {
                otherPlayer.presumed_direction=CatchChallenger::Direction_move_at_bottom;
                otherPlayer.moveStep=0;
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
    }
    else
        otherPlayer.oneStepMore->start();
}

/// \warning all ObjectGroupItem destroyed into removeMap()
void MapControllerMP::destroyMap(MapVisualiserThread::Map_full *map)
{
    //remove the other player
    QHash<uint16_t,OtherPlayer>::const_iterator i = otherPlayerList.constBegin();
    while (i != otherPlayerList.constEnd()) {
        if(i.value().presumed_map==map)
        {
            //unloadOtherPlayerFromMap(i.value());-> seam useless for the bug
            remove_player_final(i.key(),true);
            i = otherPlayerList.constBegin();
        }
        else
            ++i;
    }
    MapVisualiser::destroyMap(map);
}

CatchChallenger::Direction MapControllerMP::moveFromPath()
{
    CatchChallenger::Orientation orientation;
    if(pathList.size()>1)
    {
        PathResolved::StartPoint startPoint=pathList.last().startPoint;
        if(startPoint.map==currentMap() && startPoint.x==x && startPoint.y==y)
        {
            while(pathList.size()>1)
                pathList.removeFirst();
        }

        PathResolved &pathFirst=pathList.first();
        QPair<CatchChallenger::Orientation,uint8_t> &pathFirstUnit=pathFirst.path.first();
        orientation=pathFirstUnit.first;
        pathFirstUnit.second--;
        if(pathFirstUnit.second==0)
        {
            pathFirst.path.removeFirst();
            if(pathFirst.path.isEmpty())
            {
                pathList.removeFirst();
                if(!pathList.isEmpty())
                {
                    PathResolved::StartPoint startPoint=pathList.last().startPoint;
                    if(startPoint.map==currentMap() && startPoint.x==x && startPoint.y==y)
                    {
                    }
                    else
                        pathList.clear();
                }
            }
        }
    }
    else
    {
        PathResolved &pathFirst=pathList.first();
        QPair<CatchChallenger::Orientation,uint8_t> &pathFirstUnit=pathFirst.path.first();
        orientation=pathFirstUnit.first;
        pathFirstUnit.second--;
        if(pathFirstUnit.second==0)
        {
            pathFirst.path.removeFirst();
            if(pathFirst.path.isEmpty())
            {
                pathList.removeFirst();
                if(!pathList.isEmpty())
                {
                    PathResolved::StartPoint startPoint=pathList.last().startPoint;
                    if(startPoint.map==currentMap() && startPoint.x==x && startPoint.y==y)
                    {
                    }
                    else
                        pathList.clear();
                }
            }
        }
    }

    if(orientation==CatchChallenger::Orientation_bottom)
        return CatchChallenger::Direction_move_at_bottom;
    if(orientation==CatchChallenger::Orientation_top)
        return CatchChallenger::Direction_move_at_top;
    if(orientation==CatchChallenger::Orientation_left)
        return CatchChallenger::Direction_move_at_left;
    if(orientation==CatchChallenger::Orientation_right)
        return CatchChallenger::Direction_move_at_right;
    return CatchChallenger::Direction_move_at_bottom;
}

void MapControllerMP::eventOnMap(CatchChallenger::MapEvent event,MapVisualiserThread::Map_full * tempMapObject,uint8_t x,uint8_t y)
{
    if(event==CatchChallenger::MapEvent_SimpleClick)
    {
        if(keyAccepted.isEmpty() || (keyAccepted.contains(Qt::Key_Return) && keyAccepted.size()))
        {
            MapVisualiser::eventOnMap(event,tempMapObject,x,y);
            pathFinding.searchPath(all_map,QString::fromStdString(tempMapObject->logicalMap.map_file),x,y,current_map,this->x,this->y,CatchChallenger::stdmapToQHash(*items));
            if(pathList.isEmpty())
            {
                while(pathList.size()>1)
                    pathList.removeLast();
                //pathList.clear();
            }
        }
    }
}

bool MapControllerMP::nextPathStep()//true if have step
{
    if(!pathList.isEmpty())
    {
        if(!inMove)
        {
            std::cerr << "inMove=false into MapControllerMP::nextPathStep(), fixed" << std::endl;
            inMove=true;
        }
        const CatchChallenger::Direction &direction=MapControllerMP::moveFromPath();
        if(canGoTo(direction,all_map.value(current_map)->logicalMap,x,y,true))
        {
            this->direction=direction;
            moveStep=0;
            moveStepSlot();
            if(CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange)
            {
                if(direction==CatchChallenger::Direction_move_at_bottom)
                {
                    if(y==(all_map.value(current_map)->logicalMap.height-1))
                        emit send_player_direction(CatchChallenger::Direction_look_at_bottom);
                }
                else if(direction==CatchChallenger::Direction_move_at_top)
                {
                    if(y==0)
                        emit send_player_direction(CatchChallenger::Direction_look_at_top);
                }
                else if(direction==CatchChallenger::Direction_move_at_right)
                {
                    if(x==(all_map.value(current_map)->logicalMap.width-1))
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

void MapControllerMP::pathFindingResult(const QString &current_map,const uint8_t &x,const uint8_t &y,const QList<QPair<CatchChallenger::Orientation,uint8_t> > &path)
{
    if(!path.isEmpty())
    {
        if(path.first().second==0)
        {
            std::cerr << "MapControllerMP::pathFindingResult(): path.first().second==0" << std::endl;
            pathFindingNotFound();
            return;
        }
        if(keyAccepted.isEmpty() || keyAccepted.contains(Qt::Key_Return))
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
                    if(pathList.size()>1)
                        pathList.removeLast();
                    pathList.append(pathResolved);
                    if(!nextPathStep())
                    {
                        stopAndSend();
                        parseStop();
                    }
                    if(keyPressed.empty())
                        parseAction();
                }
                else
                    std::cerr << "Wrong start point to start the path finding" << std::endl;
            }
            wasPathFindingUsed=true;
            return;
        }
    }
    else
        pathFindingNotFound();
}

void MapControllerMP::keyPressParse()
{
    pathFinding.cancel();
    pathList.clear();
    MapVisualiserPlayerWithFight::keyPressParse();
}
