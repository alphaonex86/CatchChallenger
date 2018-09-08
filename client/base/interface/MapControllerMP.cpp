#include "MapController.h"
#include "../../../general/base/MoveOnTheMap.h"
#include "../../../general/base/FacilityLib.h"
#include "../../../general/base/FacilityLibGeneral.h"
#include "../../../general/base/CommonSettingsServer.h"
#include "../../../general/base/CommonDatapack.h"
#include "DatapackClientLoader.h"
#include "../ClientVariable.h"
#include "../FacilityLibClient.h"
#include "../Api_client_real.h"

#include <QMessageBox>
#include <QLabel>
#include <QPixmap>
#include <qmath.h>
#include <iostream>
#include <cmath>

QFont MapControllerMP::playerpseudofont;
QPixmap * MapControllerMP::imgForPseudoAdmin=NULL;
QPixmap * MapControllerMP::imgForPseudoDev=NULL;
QPixmap * MapControllerMP::imgForPseudoPremium=NULL;

MapControllerMP::MapControllerMP(const bool &centerOnPlayer, const bool &debugTags, const bool &useCache) :
    MapVisualiserPlayerWithFight(centerOnPlayer,debugTags,useCache)
{
    qRegisterMetaType<CatchChallenger::Direction>("CatchChallenger::Direction");
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_public_informations>("CatchChallenger::Player_public_informations");
    qRegisterMetaType<CatchChallenger::Player_private_and_public_informations>("CatchChallenger::Player_private_and_public_informations");
    qRegisterMetaType<std::vector<std::pair<uint8_t,CatchChallenger::Direction> > >("std::vector<std::pair<uint8_t,CatchChallenger::Direction> >");
    qRegisterMetaType<std::vector<MapVisualiserThread::Map_full> >("std::vector<MapVisualiserThread::Map_full>");
    qRegisterMetaType<std::vector<std::pair<CatchChallenger::Direction,uint8_t> > >("std::vector<std::pair<CatchChallenger::Direction,uint8_t> >");
    qRegisterMetaType<std::vector<std::pair<CatchChallenger::Orientation,uint8_t> > >("std::vector<std::pair<CatchChallenger::Orientation,uint8_t> >");
    if(!connect(&pathFinding,&PathFinding::result,this,&MapControllerMP::pathFindingResult))
        abort();

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

void MapControllerMP::connectAllSignals(CatchChallenger::Api_protocol *client)
{
    this->client=client;
    //connect the map controler
    if(!QObject::connect(client,&CatchChallenger::Api_client_real::Qthave_current_player_info,   this,&MapControllerMP::have_current_player_info,Qt::QueuedConnection))
        abort();
    if(!QObject::connect(client,&CatchChallenger::Api_client_real::Qtinsert_player,              this,&MapControllerMP::insert_player,Qt::QueuedConnection))
        abort();
    if(!QObject::connect(client,&CatchChallenger::Api_client_real::Qtremove_player,              this,&MapControllerMP::remove_player,Qt::QueuedConnection))
        abort();
    if(!QObject::connect(client,&CatchChallenger::Api_client_real::Qtmove_player,                this,&MapControllerMP::move_player,Qt::QueuedConnection))
        abort();
    if(!QObject::connect(client,&CatchChallenger::Api_client_real::Qtreinsert_player,               this,&MapControllerMP::reinsert_player,Qt::QueuedConnection))
        abort();
    if(!QObject::connect(client,&CatchChallenger::Api_client_real::Qtreinsert_player,               this,&MapControllerMP::reinsert_player,Qt::QueuedConnection))
        abort();
    if(!QObject::connect(this,&MapControllerMP::send_player_direction,client,&CatchChallenger::Api_client_real::send_player_direction))
        abort();
    if(!QObject::connect(client,&CatchChallenger::Api_client_real::QtteleportTo,                 this,&MapControllerMP::teleportTo,Qt::QueuedConnection))
        abort();
}

void MapControllerMP::resetAll()
{
    unloadPlayerFromCurrentMap();
    current_map.clear();
    pathList.clear();
    delayedActions.clear();
    skinFolderList.clear();

    for(const auto &n : otherPlayerList) {
        unloadOtherPlayerFromMap(n.second);
        delete n.second.playerTileset;
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
    double scaleSizeDouble=static_cast<double>(scaleSize);
    double scaleSizeQReal=static_cast<qreal>(scaleSizeDouble);
    if(scaleSize<1 || scaleSize>4)
    {
        qDebug() << QStringLiteral("scaleSize out of range 1-4: %1").arg(scaleSizeQReal);
        return;
    }
    scale(scaleSizeQReal/static_cast<double>(this->scaleSize),scaleSizeQReal/static_cast<double>(this->scaleSize));
    this->scaleSize=scaleSize;
}

const std::unordered_map<uint16_t,MapControllerMP::OtherPlayer> &MapControllerMP::getOtherPlayerList() const
{
    return otherPlayerList;
}

bool MapControllerMP::loadPlayerMap(const std::string &fileName,const uint8_t &x,const uint8_t &y)
{
    //position
    mapVisualiserThread.stopIt=false;
    if(current_map.empty())
    {
        error("MapControllerMP::loadPlayerMap() map empty");
        return false;
    }
    if(fileName.empty())
    {
        error("MapControllerMP::loadPlayerMap() fileName empty");
        return false;
    }
    if(stringEndsWith(fileName,"/"))
    {
        error("MapControllerMP::loadPlayerMap() map is directory");
        return false;
    }
    loadOtherMap(fileName);
    return MapVisualiserPlayer::loadPlayerMap(fileName,x,y);
}

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

void MapControllerMP::updateOtherPlayerMonsterTile(OtherPlayer &tempPlayer,const uint16_t &monster)
{
    bool resetMonster=false;
    if(tempPlayer.monsterMapObject!=NULL)
    {
        unloadOtherMonsterFromCurrentMap(tempPlayer);
        delete tempPlayer.monsterMapObject;
        tempPlayer.monsterMapObject=NULL;
        resetMonster=true;
    }
    tempPlayer.monsterTileset=NULL;
    tempPlayer.informations.monsterId=monster;
    const std::string &imagePath=datapackPath+DATAPACK_BASE_PATH_MONSTERS+std::to_string(monster)+"/overworld.png";
    if(monsterTilesetCache.find(imagePath)!=monsterTilesetCache.cend())
        tempPlayer.monsterTileset=monsterTilesetCache.at(imagePath);
    else
    {
        QImage image(QString::fromStdString(imagePath));
        if(!image.isNull())
        {
            tempPlayer.monsterTileset = new Tiled::Tileset(QString::fromStdString(lastTileset),32,32);
            if(!tempPlayer.monsterTileset->loadFromImage(image,QString::fromStdString(imagePath)))
                abort();
            monsterTilesetCache[imagePath]=tempPlayer.monsterTileset;
        }
        else
            tempPlayer.monsterTileset=NULL;
    }
    if(tempPlayer.monsterTileset!=NULL)
    {
        tempPlayer.monsterMapObject = new Tiled::MapObject();
        tempPlayer.monsterMapObject->setName("Other player monster");

        Tiled::Cell cell=tempPlayer.monsterMapObject->cell();
        switch(tempPlayer.direction)
        {
            case CatchChallenger::Direction_look_at_top:
            case CatchChallenger::Direction_move_at_top:
                cell.tile=tempPlayer.monsterTileset->tileAt(2);
            break;
            case CatchChallenger::Direction_look_at_right:
            case CatchChallenger::Direction_move_at_right:
                cell.tile=tempPlayer.monsterTileset->tileAt(7);
            break;
            case CatchChallenger::Direction_look_at_bottom:
            case CatchChallenger::Direction_move_at_bottom:
                cell.tile=tempPlayer.monsterTileset->tileAt(6);
            break;
            case CatchChallenger::Direction_look_at_left:
            case CatchChallenger::Direction_move_at_left:
                cell.tile=tempPlayer.monsterTileset->tileAt(3);
            break;
            default:
            break;
        }
        tempPlayer.monsterMapObject->setCell(cell);
    }
    if(resetMonster)
        loadOtherMonsterFromCurrentMap(tempPlayer);
    resetOtherMonsterTile(tempPlayer);
}

void MapControllerMP::resetOtherMonsterTile(OtherPlayer &tempPlayer)
{
    if(tempPlayer.monsterMapObject==nullptr)
        return;
    tempPlayer.monster_x=tempPlayer.x;
    tempPlayer.monster_y=tempPlayer.y;
    tempPlayer.pendingMonsterMoves.clear();
    tempPlayer.monsterMapObject->setVisible(false);
}

//call after enter on new map
void MapControllerMP::loadOtherPlayerFromMap(const OtherPlayer &otherPlayer,const bool &display)
{
    Q_UNUSED(display);
    //remove the player tile if needed
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
    }

    //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
    otherPlayer.playerMapObject->setPosition(QPoint(otherPlayer.x,otherPlayer.y+1));
    if(otherPlayer.labelMapObject!=NULL)
        otherPlayer.labelMapObject->setPosition(QPointF(static_cast<qreal>(otherPlayer.x)-
                static_cast<qreal>(otherPlayer.labelTileset->tileWidth())/2/16+0.5,
                                                        static_cast<qreal>(otherPlayer.y)+1-1.4));

    /*if(display)
    {
        QSet<QString> mapUsed=loadMap(otherPlayer.presumed_map,display);
        QSetIterator<QString> i(mapUsed);
        while (i.hasNext())
        {
            std::string map = i.next();
            if(mapUsedByOtherPlayer.contains(map))
                mapUsedByOtherPlayer[map]++;
            else
                mapUsedByOtherPlayer[map]=1;
        }
        removeUnusedMap();
    }*/
    /// \todo temp fix, do a better fix
    const Tiled::MapObject * playerMapObject=getPlayerMapObject();
    if(MapObjectItem::objectLink.find(const_cast<Tiled::MapObject *>(playerMapObject))!=MapObjectItem::objectLink.cend())
        centerOn(MapObjectItem::objectLink.at(const_cast<Tiled::MapObject *>(playerMapObject)));

    if(ObjectGroupItem::objectGroupLink.find(otherPlayer.presumed_map->objectGroup)!=ObjectGroupItem::objectGroupLink.cend())
    {
        ObjectGroupItem::objectGroupLink.at(otherPlayer.presumed_map->objectGroup)->addObject(otherPlayer.playerMapObject);
        if(otherPlayer.labelMapObject!=NULL)
            ObjectGroupItem::objectGroupLink.at(otherPlayer.presumed_map->objectGroup)->addObject(otherPlayer.labelMapObject);
        if(MapObjectItem::objectLink.find(otherPlayer.playerMapObject)==MapObjectItem::objectLink.cend())
            qDebug() << QStringLiteral("loadOtherPlayerFromMap(), MapObjectItem::objectLink don't have otherPlayer.playerMapObject");
        else
        {
            if(MapObjectItem::objectLink.at(otherPlayer.playerMapObject)==NULL)
                qDebug() << QStringLiteral("loadOtherPlayerFromMap(), MapObjectItem::objectLink[otherPlayer.playerMapObject]==NULL");
            else
            {
                MapObjectItem::objectLink.at(otherPlayer.playerMapObject)->setZValue(otherPlayer.y);
                if(otherPlayer.labelMapObject!=NULL)
                    MapObjectItem::objectLink.at(otherPlayer.labelMapObject)->setZValue(otherPlayer.y);
            }
        }
    }
    else
        qDebug() << QStringLiteral("loadOtherPlayerFromMap(), ObjectGroupItem::objectGroupLink not contains current_map->objectGroup");

    loadOtherMonsterFromCurrentMap(otherPlayer);
}

void MapControllerMP::loadOtherMonsterFromCurrentMap(const OtherPlayer &tempPlayer)
{
    if(tempPlayer.monsterMapObject==nullptr)
        return;
    //monster
    if(all_map.find(tempPlayer.current_monster_map)==all_map.cend())
    {
        qDebug() << QStringLiteral("all_map have not the current map: %1").arg(QString::fromStdString(tempPlayer.current_monster_map));
        return;
    }
    {
        Tiled::ObjectGroup *currentGroup=tempPlayer.monsterMapObject->objectGroup();
        if(currentGroup!=NULL)
        {
            if(ObjectGroupItem::objectGroupLink.find(currentGroup)!=ObjectGroupItem::objectGroupLink.cend())
                ObjectGroupItem::objectGroupLink.at(currentGroup)->removeObject(tempPlayer.monsterMapObject);
            //currentGroup->removeObject(monsterMapObject);
            if(currentGroup!=all_map.at(tempPlayer.current_map)->objectGroup)
                qDebug() << QStringLiteral("loadPlayerFromCurrentMap(), the monsterMapObject group is wrong: %1").arg(currentGroup->name());
        }
        if(ObjectGroupItem::objectGroupLink.find(all_map.at(tempPlayer.current_map)->objectGroup)!=ObjectGroupItem::objectGroupLink.cend())
            ObjectGroupItem::objectGroupLink.at(all_map.at(tempPlayer.current_map)->objectGroup)->addObject(tempPlayer.monsterMapObject);
        else
            qDebug() << QStringLiteral("loadPlayerFromCurrentMap(), ObjectGroupItem::objectGroupLink not contains current_map->objectGroup");
        //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
        tempPlayer.monsterMapObject->setPosition(QPointF(tempPlayer.x-0.5,tempPlayer.y+1));
        MapObjectItem::objectLink.at(tempPlayer.monsterMapObject)->setZValue(tempPlayer.y);
    }
}

//call before leave the old map (and before loadPlayerFromCurrentMap())
void MapControllerMP::unloadOtherPlayerFromMap(const OtherPlayer &otherPlayer)
{
    //unload the player sprite
    if(ObjectGroupItem::objectGroupLink.find(otherPlayer.playerMapObject->objectGroup())!=ObjectGroupItem::objectGroupLink.cend())
        ObjectGroupItem::objectGroupLink.at(otherPlayer.playerMapObject->objectGroup())->removeObject(otherPlayer.playerMapObject);
    else
        qDebug() << QStringLiteral("unloadOtherPlayerFromMap(), ObjectGroupItem::objectGroupLink not contains otherPlayer.playerMapObject->objectGroup()");
    if(otherPlayer.labelMapObject!=NULL)
    {
        //unload the player sprite
        if(ObjectGroupItem::objectGroupLink.find(otherPlayer.labelMapObject->objectGroup())!=ObjectGroupItem::objectGroupLink.cend())
            ObjectGroupItem::objectGroupLink.at(otherPlayer.labelMapObject->objectGroup())->removeObject(otherPlayer.labelMapObject);
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

    for (const auto &n : otherPlayer.mapUsed) {
        std::string map = n;
        if(mapUsedByOtherPlayer.find(map)!=mapUsedByOtherPlayer.cend())
        {
            mapUsedByOtherPlayer[map]--;
            if(mapUsedByOtherPlayer.at(map)==0)
                mapUsedByOtherPlayer.erase(map);
        }
        else
            qDebug() << QStringLiteral("map not found into mapUsedByOtherPlayer for player: %1, map: %2")
                        .arg(otherPlayer.informations.simplifiedId).arg(QString::fromStdString(map));
    }
}

void MapControllerMP::unloadOtherMonsterFromCurrentMap(const MapControllerMP::OtherPlayer &tempPlayer)
{
    //monster
    {
        Tiled::ObjectGroup *currentGroup=tempPlayer.monsterMapObject->objectGroup();
        if(currentGroup==NULL)
            return;
        //unload the player sprite
        if(ObjectGroupItem::objectGroupLink.find(tempPlayer.monsterMapObject->objectGroup())!=
                ObjectGroupItem::objectGroupLink.cend())
            ObjectGroupItem::objectGroupLink.at(tempPlayer.monsterMapObject->objectGroup())->removeObject(tempPlayer.monsterMapObject);
        else
            qDebug() << QStringLiteral("unloadPlayerFromCurrentMap(), ObjectGroupItem::objectGroupLink not contains monsterMapObject->objectGroup()");
    }
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

void MapControllerMP::teleportTo(const uint32_t &mapId,const uint16_t &x,const uint16_t &y,const CatchChallenger::Direction &direction)
{
    if(!mHaveTheDatapack || !player_informations_is_set)
    {
        DelayedTeleportTo tempItem;
        tempItem.mapId=mapId;
        tempItem.x=x;
        tempItem.y=y;
        tempItem.direction=direction;
        delayedTeleportTo.push_back(tempItem);
        #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
        qDebug() << QStringLiteral("delayed teleportTo(%1,%2,%3,%4)").arg(DatapackClientLoader::datapackLoader.maps.value(mapId)).arg(x).arg(y).arg(CatchChallenger::MoveOnTheMap::directionToString(direction));
        #endif
        return;
    }
    if(mapId>=(uint32_t)DatapackClientLoader::datapackLoader.maps.size())
    {
        emit error("mapId greater than DatapackClientLoader::datapackLoader.maps.size(): "+
                   std::to_string(DatapackClientLoader::datapackLoader.maps.size()));
        return;
    }
    #ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    qDebug() << QStringLiteral("teleportTo(%1,%2,%3,%4)").arg(DatapackClientLoader::datapackLoader.maps.value(mapId)).arg(x).arg(y).arg(CatchChallenger::MoveOnTheMap::directionToString(direction));
    qDebug() << QStringLiteral("currently on: %1 (%2,%3)").arg(current_map).arg(this->x).arg(this->y);
    #endif
    client->teleportDone();
    MapVisualiserPlayer::unloadPlayerFromCurrentMap();
    current_map=QFileInfo(QString::fromStdString(
                    datapackMapPathSpec+DatapackClientLoader::datapackLoader.maps.at(mapId)))
            .absoluteFilePath().toStdString();
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
    finalPlayerStepTeleported(isTeleported);
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

    skinFolderList=CatchChallenger::FacilityLibGeneral::skinIdList(datapackPath+DATAPACK_BASE_PATH_SKIN);

    if(player_informations_is_set)
        reinject_signals();
}

void MapControllerMP::reinject_signals()
{
    #ifdef DEBUG_CLIENT_LOAD_ORDER
    qDebug() << QStringLiteral("MapControllerMP::reinject_signals()");
    #endif
    unsigned int index=0;

    if(mHaveTheDatapack && player_informations_is_set)
    {
        #ifdef DEBUG_CLIENT_LOAD_ORDER
        qDebug() << QStringLiteral("MapControllerMP::reinject_signals(): mHaveTheDatapack && player_informations_is_set");
        #endif
        index=0;
        const unsigned int delayedActions_size=delayedActions.size();
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
                    if(insert_player_final(delayedActions.at(index).insert.player,
                                           delayedActions.at(index).insert.mapId,delayedActions.at(index).insert.x,
                                           delayedActions.at(index).insert.y,delayedActions.at(index).insert.direction,true))
                        delayedActions.erase(delayedActions.cbegin()+index);
                break;
                case DelayedType_Move:
                    if(move_player_final(delayedActions.at(index).move.id,delayedActions.at(index).move.movement,true))
                        delayedActions.erase(delayedActions.cbegin()+index);
                break;
                case DelayedType_Remove:
                    if(remove_player_final(delayedActions.at(index).remove,true))
                        delayedActions.erase(delayedActions.cbegin()+index);
                break;
                case DelayedType_Reinsert_single:
                    if(reinsert_player_final(delayedActions.at(index).reinsert_single.id,
                                             delayedActions.at(index).reinsert_single.x,delayedActions.at(index).reinsert_single.y,
                                             delayedActions.at(index).reinsert_single.direction,true))
                        delayedActions.erase(delayedActions.cbegin()+index);
                break;
                case DelayedType_Reinsert_full:
                    if(full_reinsert_player_final(delayedActions.at(index).reinsert_full.id,
                                                  delayedActions.at(index).reinsert_full.mapId,delayedActions.at(index).reinsert_full.x,
                                                  delayedActions.at(index).reinsert_full.y,delayedActions.at(index).reinsert_full.direction,true))
                        delayedActions.erase(delayedActions.cbegin()+index);
                break;
                case DelayedType_Drop_all:
                    if(dropAllPlayerOnTheMap_final(true))
                        delayedActions.erase(delayedActions.cbegin()+index);
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
    unsigned int index;

    if(mHaveTheDatapack && player_informations_is_set)
    {
        #ifdef DEBUG_CLIENT_LOAD_ORDER
        qDebug() << QStringLiteral("MapControllerMP::reinject_signals_on_valid_map(): mHaveTheDatapack && player_informations_is_set");
        #endif
        index=0;
        const unsigned int delayedActions_size=delayedActions.size();
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
                    const std::string &mapPath=QFileInfo(QString::fromStdString(datapackMapPathSpec+DatapackClientLoader::datapackLoader.maps.at(
                                                             delayedActions.at(index).insert.mapId))).absoluteFilePath().toStdString();
                    if(all_map.find(mapPath)!=all_map.cend())
                    {
                        if(insert_player_final(delayedActions.at(index).insert.player,delayedActions.at(index).insert.mapId,
                                               delayedActions.at(index).insert.x,delayedActions.at(index).insert.y,
                                               delayedActions.at(index).insert.direction,true))
                            delayedActions.erase(delayedActions.cbegin()+index);
                        index--;
                    }
                }
                break;
                case DelayedType_Move:
                    if(otherPlayerList.find(delayedActions.at(index).move.id)!=otherPlayerList.cend())
                    {
                        if(move_player_final(delayedActions.at(index).move.id,delayedActions.at(index).move.movement,true))
                            delayedActions.erase(delayedActions.cbegin()+index);
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

bool MapControllerMP::asyncMapLoaded(const std::string &fileName,MapVisualiserThread::Map_full * tempMapObject)
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
    const uint16_t &simplifiedId=otherPlayerListByAnimationTimer.at(timer);
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
    const uint16_t &simplifiedId=otherPlayerListByTimer.at(timer);
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
                MapDoor* door=all_map.at(map->map_file)->doors.at(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)));
                door->startOpen(static_cast<uint16_t>(otherPlayer.playerSpeed));
                otherPlayer.moveAnimationTimer->start(door->timeToOpen());
                return;
            }
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
        otherPlayer.inMove=true;
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
        otherPlayer.inMove=true;
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
        otherPlayer.inMove=true;
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
        otherPlayer.inMove=true;
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
        if(otherPlayer.monsterMapObject!=NULL)
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

                otherPlayer.monsterMapObject->setPosition(QPointF(otherPlayer.monster_x-0.5,otherPlayer.monster_y+1));
                MapObjectItem::objectLink.at(otherPlayer.monsterMapObject)->setZValue(otherPlayer.monster_y);
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
        otherPlayer.playerMapObject->setPosition(QPoint(x,y+1));
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
        finalOtherPlayerStep(otherPlayer);
    }
    else
        otherPlayer.oneStepMore->start();
}

void MapControllerMP::finalOtherPlayerStep(OtherPlayer &otherPlayer)
{
    const MapVisualiserThread::Map_full * current_map_pointer=otherPlayer.presumed_map;
    if(current_map_pointer==NULL)
    {
        qDebug() << "current map not loaded null pointer, unable to do finalPlayerStep()";
        return;
    }

    if(!CatchChallenger::CommonDatapack::commonDatapack.monstersCollision.empty())
    {
        //locate the right layer for monster
        if(otherPlayer.monsterMapObject!=NULL)
        {
            const MapVisualiserThread::Map_full * current_monster_map_pointer=all_map.at(otherPlayer.current_monster_map);
            if(current_monster_map_pointer==NULL)
            {
                qDebug() << "current_monster_map_pointer not loaded null pointer, unable to do finalPlayerStep()";
                return;
            }
            {
                const CatchChallenger::MonstersCollisionValue &monstersCollisionValue=
                        CatchChallenger::MoveOnTheMap::getZoneCollision(current_monster_map_pointer->logicalMap,otherPlayer.monster_x,otherPlayer.monster_y);
                otherPlayer.monsterMapObject->setVisible(false);
                unsigned int index=0;
                while(index<monstersCollisionValue.walkOn.size())
                {
                    const unsigned int &newIndex=monstersCollisionValue.walkOn.at(index);
                    if(newIndex<CatchChallenger::CommonDatapack::commonDatapack.monstersCollision.size())
                    {
                        const CatchChallenger::MonstersCollision &monstersCollision=
                                CatchChallenger::CommonDatapack::commonDatapack.monstersCollision.at(newIndex);
                            otherPlayer.monsterMapObject->setVisible((monstersCollision.tile.empty() && otherPlayer.pendingMonsterMoves.size()>=1) ||
                                                         (otherPlayer.pendingMonsterMoves.size()==1 && !otherPlayer.inMove)
                                                         );
                    }
                    index++;
                }
            }
        }
        //locate the right layer
        const CatchChallenger::MonstersCollisionValue &monstersCollisionValue=
                CatchChallenger::MoveOnTheMap::getZoneCollision(current_map_pointer->logicalMap,otherPlayer.presumed_x,otherPlayer.presumed_y);
        unsigned int index=0;
        while(index<monstersCollisionValue.walkOn.size())
        {
            const unsigned int &newIndex=monstersCollisionValue.walkOn.at(index);
            if(newIndex<CatchChallenger::CommonDatapack::commonDatapack.monstersCollision.size())
            {
                const CatchChallenger::MonstersCollision &monstersCollision=
                        CatchChallenger::CommonDatapack::commonDatapack.monstersCollision.at(newIndex);
                //change tile if needed (water to walk transition)
                if(monstersCollision.tile!=otherPlayer.lastTileset)
                {
                    otherPlayer.lastTileset=monstersCollision.tile;
                    if(playerTilesetCache.find(otherPlayer.lastTileset)!=playerTilesetCache.cend())
                        otherPlayer.playerTileset=playerTilesetCache.at(otherPlayer.lastTileset);
                    else
                    {
                        if(otherPlayer.lastTileset.empty())
                            otherPlayer.playerTileset=playerTilesetCache[defaultTileset];
                        else
                        {
                            const std::string &playerSkinPath=datapackPath+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(otherPlayer.informations.skinId);
                            const std::string &imagePath=playerSkinPath+"/"+otherPlayer.lastTileset+".png";
                            QImage image(QString::fromStdString(imagePath));
                            if(!image.isNull())
                            {
                                otherPlayer.playerTileset = new Tiled::Tileset(QString::fromStdString(lastTileset),16,24);
                                otherPlayer.playerTileset->loadFromImage(image,QString::fromStdString(imagePath));
                            }
                            else
                            {
                                qDebug() << "Unable to load the player tilset: "+QString::fromStdString(imagePath);
                                otherPlayer.playerTileset=playerTilesetCache[defaultTileset];
                            }
                        }
                        playerTilesetCache[lastTileset]=otherPlayer.playerTileset;
                    }
                    {
                        Tiled::Cell cell=otherPlayer.playerMapObject->cell();
                        int tileId=cell.tile->id();
                        cell.tile=otherPlayer.playerTileset->tileAt(tileId);
                        otherPlayer.playerMapObject->setCell(cell);
                    }
                }
                break;
            }
            index++;
        }
        if(index==monstersCollisionValue.walkOn.size())
        {
            lastTileset=defaultTileset;
            otherPlayer.playerTileset=playerTilesetCache[defaultTileset];
            {
                Tiled::Cell cell=otherPlayer.playerMapObject->cell();
                int tileId=cell.tile->id();
                cell.tile=otherPlayer.playerTileset->tileAt(tileId);
                otherPlayer.playerMapObject->setCell(cell);
            }
        }
    }
}

/// \warning all ObjectGroupItem destroyed into removeMap()
void MapControllerMP::destroyMap(MapVisualiserThread::Map_full *map)
{
    //remove the other player
    std::unordered_map<uint16_t,OtherPlayer> otherPlayerList2=otherPlayerList;
    for (const auto &n : otherPlayerList2) {
        if(n.second.presumed_map==map)
            remove_player_final(n.first,true);
    }
    MapVisualiser::destroyMap(map);
}

CatchChallenger::Direction MapControllerMP::moveFromPath()
{
    const std::pair<uint8_t,uint8_t> pos(getPos());
    const uint8_t &x=pos.first;
    const uint8_t &y=pos.second;
    CatchChallenger::Orientation orientation;
    if(pathList.size()>1)
    {
        PathResolved::StartPoint startPoint=pathList.back().startPoint;
        if(startPoint.map==currentMap() && startPoint.x==x && startPoint.y==y)
        {
            while(pathList.size()>1)
                pathList.erase(pathList.cbegin());
        }

        PathResolved &pathFirst=pathList.front();
        std::pair<CatchChallenger::Orientation,uint8_t> &pathFirstUnit=pathFirst.path.front();
        orientation=pathFirstUnit.first;
        pathFirstUnit.second--;
        if(pathFirstUnit.second==0)
        {
            pathFirst.path.erase(pathFirst.path.cbegin());
            if(pathFirst.path.empty())
            {
                pathList.erase(pathList.cbegin());
                if(!pathList.empty())
                {
                    PathResolved::StartPoint startPoint=pathList.back().startPoint;
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
        PathResolved &pathFirst=pathList.front();
        std::pair<CatchChallenger::Orientation,uint8_t> &pathFirstUnit=pathFirst.path.front();
        orientation=pathFirstUnit.first;
        pathFirstUnit.second--;
        if(pathFirstUnit.second==0)
        {
            pathFirst.path.erase(pathFirst.path.cbegin());
            if(pathFirst.path.empty())
            {
                pathList.erase(pathList.cbegin());
                if(!pathList.empty())
                {
                    PathResolved::StartPoint startPoint=pathList.back().startPoint;
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
    const std::pair<uint8_t,uint8_t> pos(getPos());
    const uint8_t &thisx=pos.first;
    const uint8_t &thisy=pos.second;
    if(event==CatchChallenger::MapEvent_SimpleClick)
    {
        if(keyAccepted.empty() || (keyAccepted.find(Qt::Key_Return)!=keyAccepted.cend() && keyAccepted.size()))
        {
            MapVisualiser::eventOnMap(event,tempMapObject,x,y);
            pathFinding.searchPath(all_map,tempMapObject->logicalMap.map_file,x,y,current_map,thisx,thisy,*items);
            if(pathList.empty())
            {
                while(pathList.size()>1)
                    pathList.pop_back();
                //pathList.clear();
            }
        }
    }
}

bool MapControllerMP::nextPathStep()//true if have step
{
    if(!pathList.empty())
    {
        const CatchChallenger::Direction &direction=MapControllerMP::moveFromPath();
        MapVisualiserPlayer::nextPathStepInternal(pathList,direction);
    }
    return false;
}

void MapControllerMP::pathFindingResult(const std::string &current_map,const uint8_t &x,const uint8_t &y,const std::vector<std::pair<CatchChallenger::Orientation,uint8_t> > &path)
{
    if(!path.empty())
    {
        if(path.front().second==0)
        {
            std::cerr << "MapControllerMP::pathFindingResult(): path.first().second==0" << std::endl;
            pathFindingNotFound();
            return;
        }
        MapVisualiserPlayer::pathFindingResultInternal(pathList,current_map,x,y,path);
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
