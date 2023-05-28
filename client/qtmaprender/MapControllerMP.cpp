#include "MapController.hpp"
#include "../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../libqtcatchchallenger/Api_client_real.hpp"
#include "../../../general/base/FacilityLibGeneral.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include "../../../general/base/MoveOnTheMap.hpp"
#include <iostream>
#include <QDebug>
#include <QFileInfo>

QFont MapControllerMP::playerpseudofont;
QPixmap * MapControllerMP::imgForPseudoAdmin=NULL;
QPixmap * MapControllerMP::imgForPseudoDev=NULL;
QPixmap * MapControllerMP::imgForPseudoPremium=NULL;

MapControllerMP::MapControllerMP(const bool &centerOnPlayer, const bool &debugTags, const bool &useCache, const bool &openGL) :
    MapVisualiserPlayerWithFight(centerOnPlayer,debugTags,useCache,openGL)
{
    qRegisterMetaType<CatchChallenger::Direction>("CatchChallenger::Direction");
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_public_informations>("CatchChallenger::Player_public_informations");
    qRegisterMetaType<CatchChallenger::Player_private_and_public_informations>("CatchChallenger::Player_private_and_public_informations");
    qRegisterMetaType<std::vector<std::pair<uint8_t,CatchChallenger::Direction> > >("std::vector<std::pair<uint8_t,CatchChallenger::Direction> >");
    qRegisterMetaType<std::vector<Map_full> >("std::vector<Map_full>");
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

void MapControllerMP::connectAllSignals(CatchChallenger::Api_protocol_Qt *client)
{
    this->client=client;
    #if ! defined (ONLYMAPRENDER)
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
    if(!QObject::connect(client,&CatchChallenger::Api_client_real::Qtfull_reinsert_player,               this,&MapControllerMP::full_reinsert_player,Qt::QueuedConnection))
        abort();
    if(!QObject::connect(client,&CatchChallenger::Api_client_real::QtdropAllPlayerOnTheMap,               this,&MapControllerMP::dropAllPlayerOnTheMap,Qt::QueuedConnection))
        abort();
    if(!QObject::connect(this,&MapControllerMP::send_player_direction,client,&CatchChallenger::Api_protocol_Qt::send_player_direction))
        abort();
    if(!QObject::connect(client,&CatchChallenger::Api_client_real::QtteleportTo,                 this,&MapControllerMP::teleportTo,Qt::QueuedConnection))
        abort();
    #endif
}

void MapControllerMP::resetAll()
{
    std::cout << "MapControllerMP::resetAll()" << std::endl;
    unloadPlayerFromCurrentMap();
    current_map.clear();
    pathList.clear();
    delayedActions.clear();
    skinFolderList.clear();

    for(const auto &n : otherPlayerList) {
        unloadOtherPlayerFromMap(n.second);
//        delete n.second.playerTileset;mem leak to prevent crash for now
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
    //scaleSize 1: 32*16px = 512px
    //scaleSize 4: 8*16px = 128px
    const int w=width();
    const int h=height();
    double scaleSizeW=(double)w*scaleSize/512;
    double scaleSizeH=(double)h*scaleSize/512;
    double scaleSizeMax=scaleSizeW;
    if(scaleSizeH>scaleSizeMax)
        scaleSizeMax=scaleSizeH;
    scaleSizeMax=(int)scaleSizeMax;
    if(scaleSizeMax<1.0)
        scaleSizeMax=1.0;
    scale(scaleSizeMax/static_cast<double>(this->scaleSize),scaleSizeMax/static_cast<double>(this->scaleSize));
    this->scaleSize=scaleSizeMax;
    //update to real zoom
}

void MapControllerMP::updateScale()
{
    setScale(this->scaleSize);
}

void MapControllerMP::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    updateScale();
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
    tempPlayer.monster_x=tempPlayer.presumed_x;
    tempPlayer.monster_y=tempPlayer.presumed_y;
    tempPlayer.pendingMonsterMoves.clear();
    tempPlayer.monsterMapObject->setVisible(false);
}

//call after enter on new map
void MapControllerMP::loadOtherPlayerFromMap(const OtherPlayer &otherPlayer,const bool &display)
{
    std::cout << "MapControllerMP::loadOtherPlayerFromMap();" << std::endl;
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
    otherPlayer.playerMapObject->setPosition(QPointF(otherPlayer.x,otherPlayer.y+1));
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
    std::cout << "playerMapObject=getPlayerMapObject();" << std::endl;
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
        tempPlayer.monsterMapObject->setPosition(QPointF(tempPlayer.monster_x-0.5,tempPlayer.monster_y+1));
        MapObjectItem::objectLink.at(tempPlayer.monsterMapObject)->setZValue(tempPlayer.monster_y);
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

bool MapControllerMP::teleportTo(const uint32_t &mapId,const uint16_t &x,const uint16_t &y,const CatchChallenger::Direction &direction)
{
    #if defined (ONLYMAPRENDER)
    (void)mapId;
    (void)x;
    (void)y;
    (void)direction;
    #else
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
        return true;
    }
    client->teleportDone();
    MapVisualiserPlayer::unloadPlayerFromCurrentMap();
    if(!MapVisualiserPlayer::teleportTo(mapId,x,y,direction))
        return false;

    passMapIntoOld();
    if(!haveMapInMemory(current_map))
    {
        emit inWaitingOfMap();
        loadOtherMap(current_map);
        return true;//because the rest is wrong
    }
    loadOtherMap(current_map);
    hideNotloadedMap();
    removeUnusedMap();
    loadPlayerFromCurrentMap();
    #endif
    return true;
}

void MapControllerMP::finalPlayerStep(bool parseKey)
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
    const bool newlyTeleported=finalPlayerStepTeleported(isTeleported);
    MapVisualiserPlayer::finalPlayerStep(!newlyTeleported && parseKey);
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
    std::cout << "MapControllerMP::datapackParsedMainSub()" << std::endl;
    setDatapackPath(client->datapackPathBase(),client->mainDatapackCode());
    /*datapackParsed();
    datapackParsedMainSub();*/

    if(datapackPath.empty())
    {
        std::cout << "MapControllerMP::datapackParsedMainSub() datapackPath can't be empty! You never call MapVisualiserPlayer::setDatapackPath()? (abort)" << std::endl;
        abort();
    }
    MapVisualiserPlayer::datapackParsedMainSub();

    skinFolderList=CatchChallenger::FacilityLibGeneral::skinIdList(datapackPath+DATAPACK_BASE_PATH_SKIN);
    if(skinFolderList.empty())
    {
        std::cout << "MapControllerMP::datapackParsedMainSub() skinFolderList can't be empty at " << datapackPath << "+" << DATAPACK_BASE_PATH_SKIN << "! (abort)" << std::endl;
        abort();
    }

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
                    const std::string &mapPath=QFileInfo(QString::fromStdString(datapackMapPathSpec+QtDatapackClientLoader::datapackLoader->get_maps().at(
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

bool MapControllerMP::asyncMapLoaded(const std::string &fileName,Map_full * tempMapObject)
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
    if(otherPlayerListByAnimationTimer.find(timer)==otherPlayerListByAnimationTimer.cend())
    {
        timer->stop();
        return;
    }
    const uint16_t &simplifiedId=otherPlayerListByAnimationTimer.at(timer);
    moveOtherPlayerStepSlotWithPlayer(otherPlayerList[simplifiedId]);
}

void MapControllerMP::finalOtherPlayerStep(OtherPlayer &otherPlayer)
{
    const Map_full * current_map_pointer=otherPlayer.presumed_map;
    if(current_map_pointer==NULL)
    {
        qDebug() << "current map not loaded null pointer, unable to do finalPlayerStep()";
        return;
    }

    if(!CatchChallenger::CommonDatapack::commonDatapack.get_monstersCollision().empty())
    {
        //locate the right layer for monster
        if(otherPlayer.monsterMapObject!=NULL)
        {
            const Map_full * current_monster_map_pointer=all_map.at(otherPlayer.current_monster_map);
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
                    if(newIndex<CatchChallenger::CommonDatapack::commonDatapack.get_monstersCollision().size())
                    {
                        /*const CatchChallenger::MonstersCollision &monstersCollision=
                                CatchChallenger::CommonDatapack::commonDatapack.monstersCollision.at(newIndex);*/
                        const CatchChallenger::MonstersCollisionTemp &monstersCollisionTemp=
                                CatchChallenger::CommonDatapack::commonDatapack.get_monstersCollisionTemp().at(newIndex);
                        const bool needBeVisible=(monstersCollisionTemp.tile.empty() && otherPlayer.pendingMonsterMoves.size()>=1) ||
                                (otherPlayer.pendingMonsterMoves.size()==1 && !otherPlayer.inMove);
                        const bool wasVisible=otherPlayer.monsterMapObject->isVisible();
                        otherPlayer.monsterMapObject->setVisible(needBeVisible);
                        if(wasVisible && !needBeVisible)
                            resetOtherMonsterTile(otherPlayer);
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
            if(newIndex<CatchChallenger::CommonDatapack::commonDatapack.get_monstersCollision().size())
            {
                /*const CatchChallenger::MonstersCollision &monstersCollision=
                        CatchChallenger::CommonDatapack::commonDatapack.monstersCollision.at(newIndex);*/
                const CatchChallenger::MonstersCollisionTemp &monstersCollisionTemp=
                        CatchChallenger::CommonDatapack::commonDatapack.get_monstersCollisionTemp().at(newIndex);
                //change tile if needed (water to walk transition)
                if(monstersCollisionTemp.tile!=otherPlayer.lastTileset)
                {
                    otherPlayer.lastTileset=monstersCollisionTemp.tile;
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
void MapControllerMP::destroyMap(Map_full *map)
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
    CatchChallenger::Orientation orientation=CatchChallenger::Orientation::Orientation_bottom;
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
        if(pathFirstUnit.second<=0)
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
            return CatchChallenger::Direction_look_at_bottom;
        }
        else
        {
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
    }
    else
    {
        PathResolved &pathFirst=pathList.front();
        std::pair<CatchChallenger::Orientation,uint8_t> &pathFirstUnit=pathFirst.path.front();
        if(pathFirstUnit.second<=0)
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
            return CatchChallenger::Direction_look_at_bottom;
        }
        else
        {
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
    }

    if(orientation==CatchChallenger::Orientation_bottom)
        return CatchChallenger::Direction_move_at_bottom;
    if(orientation==CatchChallenger::Orientation_top)
        return CatchChallenger::Direction_move_at_top;
    if(orientation==CatchChallenger::Orientation_left)
        return CatchChallenger::Direction_move_at_left;
    if(orientation==CatchChallenger::Orientation_right)
        return CatchChallenger::Direction_move_at_right;
    return CatchChallenger::Direction_look_at_bottom;
}

void MapControllerMP::eventOnMap(CatchChallenger::MapEvent event,Map_full * tempMapObject,uint8_t x,uint8_t y)
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
        switch (direction) {
        case CatchChallenger::Direction::Direction_move_at_bottom:
        case CatchChallenger::Direction::Direction_move_at_left:
        case CatchChallenger::Direction::Direction_move_at_right:
        case CatchChallenger::Direction::Direction_move_at_top:
            return MapVisualiserPlayer::nextPathStepInternal(pathList,direction);
            break;
        default:
            return false;
            break;
        }
    }
    return false;
}

void MapControllerMP::pathFindingResult(const std::string &current_map,const uint8_t &x,const uint8_t &y,const std::vector<std::pair<CatchChallenger::Orientation,uint8_t> > &path, const PathFinding::PathFinding_status &status)
{
    switch (status) {
    case PathFinding::PathFinding_status_OK:
        MapVisualiserPlayer::pathFindingResultInternal(pathList,current_map,x,y,path);
        break;
    case PathFinding::PathFinding_status_PathNotFound:
        pathFindingNotFound();
        break;
    case PathFinding::PathFinding_status_Canceled:
        break;
    case PathFinding::PathFinding_status_InternalError:
        pathFindingInternalError();
        break;
    default:
        break;
    }
}

void MapControllerMP::keyPressParse()
{
    pathFinding.cancel();
    pathList.clear();
    MapVisualiserPlayerWithFight::keyPressParse();
}
