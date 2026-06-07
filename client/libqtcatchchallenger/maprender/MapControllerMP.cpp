#include "MapController.hpp"
#include "MapItem.hpp"
#include "../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../libqtcatchchallenger/Api_client_real.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/MoveOnTheMap.hpp"
#include "QMap_client.hpp"
#include "../libcatchchallenger/ClientVariable.hpp"
#include "../CliClientOptions.hpp"
#include <iostream>
#include <QDebug>
#include <QFileInfo>
#include <QCoreApplication>
#include <QApplication>
#include <QMouseEvent>
#include <QtMath>

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
    qRegisterMetaType<CatchChallenger::QMap_client*>("CatchChallenger::QMap_client*");
    qRegisterMetaType<std::vector<std::pair<CatchChallenger::Direction,uint8_t> > >("std::vector<std::pair<CatchChallenger::Direction,uint8_t> >");
    qRegisterMetaType<std::vector<std::pair<CatchChallenger::Orientation,uint8_t> > >("std::vector<std::pair<CatchChallenger::Orientation,uint8_t> >");
    if(!connect(&pathFinding,&PathFinding::result,this,&MapControllerMP::pathFindingResult))
        abort();

    playerpseudofont=QFont(QStringLiteral("Arial"));
    playerpseudofont.setPixelSize(14);
    player_informations_is_set=false;
    pendingInitialPlayerLoad=false;
    chatHelloSent=false;

    signSelfTestStarted=false;
    signTestMap=0;
    signTestX=0;
    signTestY=0;
    signSelfTestTimeoutTimer.setSingleShot(true);
    if(!connect(&signSelfTestTimeoutTimer,&QTimer::timeout,this,&MapControllerMP::signSelfTestTimeout))
        abort();

    resetAll();

    scaleSize=1;
    requestedScaleSize=1;
}

MapControllerMP::~MapControllerMP()
{
    pathFinding.cancel();
}

void MapControllerMP::connectAllSignals(CatchChallenger::Api_protocol_Qt *client)
{
    this->client=client;
    #if ! defined (CATCHCHALLENGER_ONLYMAPRENDER)
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
    if(!QObject::connect(client,&CatchChallenger::Api_client_real::QthaveCharacter,              this,&MapControllerMP::loadCurrentPlayer,Qt::QueuedConnection))
        abort();
    #endif
}

void MapControllerMP::resetAll()
{
    std::cout << "MapControllerMP::resetAll()" << std::endl;
    unloadPlayerFromCurrentMap();
    current_map=65535;
    pathList.clear();
    delayedActions.clear();
    skinFolderList.clear();

    for(const std::pair<const SIMPLIFIED_PLAYER_ID_FOR_MAP,OtherPlayer> &n : otherPlayerList) {
        unloadOtherPlayerFromMap(n.second);
//        delete n.second.playerTileset;mem leak to prevent crash for now
    }
    otherPlayerList.clear();
    otherPlayerListByTimer.clear();
    mapUsedByOtherPlayer.clear();

    mHaveTheDatapack=false;
    player_informations_is_set=false;
    isTeleported=true;
    pendingInitialPlayerLoad=false;

    MapVisualiserPlayer::resetAll();
}

void MapControllerMP::setScale(float scaleSize)
{
    if(scaleSize<1)
    {
        qDebug() << QStringLiteral("scaleSize lower than 1: %1").arg(scaleSize);
        scaleSize=1;
    }
    if(scaleSize>4)
    {
        qDebug() << QStringLiteral("scaleSize greater than 4: %1").arg(scaleSize);
        scaleSize=4;
    }
    requestedScaleSize=scaleSize;
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
    setScale(requestedScaleSize);
}

void MapControllerMP::resizeEvent(QResizeEvent *event)
{
    MapVisualiserPlayerWithFight::resizeEvent(event);
    updateScale();
    centerOnPlayerTile();
}

const std::unordered_map<uint8_t,MapControllerMP::OtherPlayer> &MapControllerMP::getOtherPlayerList() const
{
    return otherPlayerList;
}

bool MapControllerMP::loadPlayerMap(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const COORD_TYPE &x,const COORD_TYPE &y)
{
    //position
    mapVisualiserThread.stopIt=false;
    if(current_map==65535)
    {
        error("MapControllerMP::loadPlayerMap() map empty");
        return false;
    }
    if(mapIndex==65535)
    {
        error("MapControllerMP::loadPlayerMap() fileName empty");
        return false;
    }
    loadOtherMap(mapIndex);
    return MapVisualiserPlayer::loadPlayerMap(mapIndex,x,y);
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
            // Erase the OLD tileset's raw ptr from validTilesets_ before
            // the SharedTileset assignment below drops its last ref and
            // frees the underlying Tileset.  Without this, validTilesets_
            // accumulates stale pointers that cellTilesetIsValid() will
            // happily accept on the next paint, and Cell::tile() then
            // crashes inside the freed Tileset's mTilesById tree.
            if(tempPlayer.monsterTileset)
                MapItem::validTilesets_.erase(tempPlayer.monsterTileset.data());
            tempPlayer.monsterTileset=Tiled::Tileset::create(QString::fromStdString(lastTileset),32,32);
            MapItem::validTilesets_.emplace(tempPlayer.monsterTileset.data(), tempPlayer.monsterTileset);  // see MapObjectItem.cpp cellTilesetIsValid
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
                cell.setTile(tempPlayer.monsterTileset->tileAt(2));
            break;
            case CatchChallenger::Direction_look_at_right:
            case CatchChallenger::Direction_move_at_right:
                cell.setTile(tempPlayer.monsterTileset->tileAt(7));
            break;
            case CatchChallenger::Direction_look_at_bottom:
            case CatchChallenger::Direction_move_at_bottom:
                cell.setTile(tempPlayer.monsterTileset->tileAt(6));
            break;
            case CatchChallenger::Direction_look_at_left:
            case CatchChallenger::Direction_move_at_left:
                cell.setTile(tempPlayer.monsterTileset->tileAt(3));
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
    if(CatchChallenger::QMap_client::all_map.find(otherPlayer.presumed_map)==CatchChallenger::QMap_client::all_map.cend())
    {
        std::cout << "MapControllerMP::loadOtherPlayerFromMap(); not found otherPlayer.presumed_map" << std::endl;
        return;
    }
    const CatchChallenger::QMap_client *presumed_map_pointer=CatchChallenger::QMap_client::all_map.at(otherPlayer.presumed_map);
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
        if(currentGroup!=presumed_map_pointer->objectGroup)
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
    centerOnPlayerTile();

    if(ObjectGroupItem::objectGroupLink.find(presumed_map_pointer->objectGroup)!=ObjectGroupItem::objectGroupLink.cend())
    {
        ObjectGroupItem::objectGroupLink.at(presumed_map_pointer->objectGroup)->addObject(otherPlayer.playerMapObject);
        if(otherPlayer.labelMapObject!=NULL)
            ObjectGroupItem::objectGroupLink.at(presumed_map_pointer->objectGroup)->addObject(otherPlayer.labelMapObject);
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
    if(CatchChallenger::QMap_client::all_map.find(tempPlayer.current_monster_map)==CatchChallenger::QMap_client::all_map.cend())
    {
        qDebug() << QStringLiteral("all_map have not the current map: %1").arg(QString::number(tempPlayer.current_monster_map));
        return;
    }
    {
        Tiled::ObjectGroup *currentGroup=tempPlayer.monsterMapObject->objectGroup();
        if(currentGroup!=NULL)
        {
            if(ObjectGroupItem::objectGroupLink.find(currentGroup)!=ObjectGroupItem::objectGroupLink.cend())
                ObjectGroupItem::objectGroupLink.at(currentGroup)->removeObject(tempPlayer.monsterMapObject);
            //currentGroup->removeObject(monsterMapObject);
            if(currentGroup!=CatchChallenger::QMap_client::all_map.at(tempPlayer.current_map)->objectGroup)
                qDebug() << QStringLiteral("loadPlayerFromCurrentMap(), the monsterMapObject group is wrong: %1").arg(currentGroup->name());
        }
        if(ObjectGroupItem::objectGroupLink.find(CatchChallenger::QMap_client::all_map.at(tempPlayer.current_map)->objectGroup)!=ObjectGroupItem::objectGroupLink.cend())
            ObjectGroupItem::objectGroupLink.at(CatchChallenger::QMap_client::all_map.at(tempPlayer.current_map)->objectGroup)->addObject(tempPlayer.monsterMapObject);
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
    if(otherPlayer.playerMapObject!=nullptr)
    {
        if(ObjectGroupItem::objectGroupLink.find(otherPlayer.playerMapObject->objectGroup())!=ObjectGroupItem::objectGroupLink.cend())
            ObjectGroupItem::objectGroupLink.at(otherPlayer.playerMapObject->objectGroup())->removeObject(otherPlayer.playerMapObject);
        else
            qDebug() << QStringLiteral("unloadOtherPlayerFromMap(), ObjectGroupItem::objectGroupLink not contains otherPlayer.playerMapObject->objectGroup()");
    }
    else
        qDebug() << QStringLiteral("unloadOtherPlayerFromMap(), otherPlayer.playerMapObject is null");
    if(otherPlayer.labelMapObject!=nullptr)
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

    for (const CATCHCHALLENGER_TYPE_MAPID &n : otherPlayer.mapUsed) {
        CATCHCHALLENGER_TYPE_MAPID mapIndex = n;
        if(mapUsedByOtherPlayer.find(mapIndex)!=mapUsedByOtherPlayer.cend())
        {
            mapUsedByOtherPlayer[mapIndex]--;
            if(mapUsedByOtherPlayer.at(mapIndex)==0)
                mapUsedByOtherPlayer.erase(mapIndex);
        }
        else
            qDebug() << QStringLiteral("map not found into mapUsedByOtherPlayer for this player: map: %1").arg(QString::number(mapIndex));
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

void MapControllerMP::loadCurrentPlayer(const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const CatchChallenger::Direction &direction)
{
    std::cerr << "MapControllerMP::loadCurrentPlayer() mapId=" << mapId << " x=" << std::to_string(x) << " y=" << std::to_string(y)
              << " mHaveTheDatapack=" << mHaveTheDatapack << " player_informations_is_set=" << player_informations_is_set << std::endl;
    initialPlayerPosition.mapId=mapId;
    initialPlayerPosition.x=x;
    initialPlayerPosition.y=y;
    initialPlayerPosition.direction=direction;
    pendingInitialPlayerLoad=true;
    if(mHaveTheDatapack && player_informations_is_set)
        reinject_signals();
}

bool MapControllerMP::teleportTo(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const COORD_TYPE &x,const COORD_TYPE &y,const CatchChallenger::Direction &direction)
{
    std::cerr << "MapControllerMP::teleportTo() mapIndex=" << mapIndex << " x=" << std::to_string(x) << " y=" << std::to_string(y) << " mHaveTheDatapack=" << mHaveTheDatapack << " player_informations_is_set=" << player_informations_is_set << std::endl;
    #if defined (CATCHCHALLENGER_ONLYMAPRENDER)
    (void)mapIndex;
    (void)x;
    (void)y;
    (void)direction;
    #else
    if(!mHaveTheDatapack || !player_informations_is_set)
    {
        DelayedTeleportTo tempItem;
        tempItem.mapId=mapIndex;
        tempItem.x=x;
        tempItem.y=y;
        tempItem.direction=direction;
        delayedTeleportTo.push_back(tempItem);
        std::cerr << "MapControllerMP::teleportTo() delayed (datapack/player not ready)" << std::endl;
        return true;
    }
    client->teleportDone();
    MapVisualiserPlayer::unloadPlayerFromCurrentMap();
    if(!MapVisualiserPlayer::teleportTo(mapIndex,x,y,direction))
    {
        std::cerr << "MapControllerMP::teleportTo() MapVisualiserPlayer::teleportTo failed" << std::endl;
        return false;
    }

    passMapIntoOld();
    std::cerr << "MapControllerMP::teleportTo() current_map=" << current_map << " haveMapInMemory=" << haveMapInMemory(current_map) << std::endl;
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
    if(CatchChallenger::QMap_client::all_map.find(current_map)==CatchChallenger::QMap_client::all_map.cend())
    {
        qDebug() << "current map not loaded, unable to do finalPlayerStep()";
        return;
    }
    const CatchChallenger::QMap_client * current_map_pointer=CatchChallenger::QMap_client::all_map.at(current_map);
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
        //load current player onto the map (initial position from QthaveCharacter)
        if(pendingInitialPlayerLoad && current_map==65535)
        {
            pendingInitialPlayerLoad=false;
            const std::vector<std::string> &maps=QtDatapackClientLoader::datapackLoader->get_maps();
            std::cerr << "MapControllerMP::reinject_signals() loading current player on mapId="
                      << initialPlayerPosition.mapId << " x=" << std::to_string(initialPlayerPosition.x)
                      << " y=" << std::to_string(initialPlayerPosition.y)
                      << " maps.size()=" << maps.size() << std::endl;
            if(initialPlayerPosition.mapId>=(CATCHCHALLENGER_TYPE_MAPID)maps.size())
            {
                std::cerr << "MapControllerMP::reinject_signals() ABORT initial player load: mapId="
                          << initialPlayerPosition.mapId << " >= maps.size()=" << maps.size() << std::endl;
            }
            else
            {
                const CatchChallenger::Player_public_informations &public_info=
                        client->get_player_informations().public_informations;
                if(!haveMapInMemory(initialPlayerPosition.mapId))
                    emit inWaitingOfMap();
                //insert_player_internal sets current_map and calls loadPlayerMap which triggers loadOtherMap
                MapVisualiserPlayer::insert_player_internal(public_info,
                        initialPlayerPosition.mapId,initialPlayerPosition.x,initialPlayerPosition.y,
                        initialPlayerPosition.direction,skinFolderList);
            }
        }
        index=0;
        const unsigned int delayedActions_size=delayedActions.size();
        while(index<delayedActions.size())
        {
            if(index>50000)
            {
                qDebug() << QStringLiteral("Too hight delayedActions");
                abort();
            }
            const DelayedMultiplex &a=delayedActions.at(index);
            switch(a.type)
            {
                case DelayedType_Insert:
                    if(insert_player_final(a.insert.id,a.insert.player,
                                           a.insert.mapId,a.insert.x,
                                           a.insert.y,a.insert.direction,true))
                        delayedActions.erase(delayedActions.cbegin()+index);
                break;
                case DelayedType_Move:
                    if(move_player_final(a.move.id,a.move.movement,true))
                        delayedActions.erase(delayedActions.cbegin()+index);
                break;
                case DelayedType_Remove:
                    if(remove_player_final(a.remove,true))
                        delayedActions.erase(delayedActions.cbegin()+index);
                break;
                case DelayedType_Reinsert_single:
                    if(reinsert_player_final(a.reinsert_single.id,
                                             a.reinsert_single.x,a.reinsert_single.y,
                                             a.reinsert_single.direction,true))
                        delayedActions.erase(delayedActions.cbegin()+index);
                break;
                case DelayedType_Reinsert_full:
                    if(full_reinsert_player_final(a.reinsert_full.id,
                                                  a.reinsert_full.mapId,a.reinsert_full.x,
                                                  a.reinsert_full.y,a.reinsert_full.direction,true))
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
        std::vector<DelayedMultiplex> delayedActions=this->delayedActions;//work on copy fix cash for corruption
        std::vector<size_t> toDelete;
        const unsigned int delayedActions_size=this->delayedActions.size();
        while(index<delayedActions.size())
        {
            if(index>50000)
            {
                qDebug() << QStringLiteral("Too hight delayedActions");
                abort();
            }
            const DelayedMultiplex &a=delayedActions.at(index);
            switch(a.type)
            {
                case DelayedType_Insert:
                {
                    if(delayedActions.size()>delayedActions_size)
                    {
                        size_t sizenow=delayedActions.size();
                        qDebug() << QStringLiteral("MapControllerMP::reinject_signals(): can't grow delayedActions") << sizenow;
                        abort();
                    }
                    if(delayedActions.size()>delayedActions_size)
                    {
                        size_t sizenow=delayedActions.size();
                        qDebug() << QStringLiteral("MapControllerMP::reinject_signals(): can't grow delayedActions") << sizenow;
                        abort();
                    }
                    if(CatchChallenger::QMap_client::all_map.find(a.insert.mapId)!=CatchChallenger::QMap_client::all_map.cend())
                    {
                        if(delayedActions.size()>delayedActions_size)
                        {
                            size_t sizenow=delayedActions.size();
                            qDebug() << QStringLiteral("MapControllerMP::reinject_signals(): can't grow delayedActions") << sizenow;
                            abort();
                        }
                        if(insert_player_final(a.insert.id,a.insert.player,a.insert.mapId,
                                               a.insert.x,a.insert.y,
                                               a.insert.direction,true))
                        {
                            if(delayedActions.size()>delayedActions_size)
                            {
                                size_t sizenow=delayedActions.size();
                                qDebug() << QStringLiteral("MapControllerMP::reinject_signals(): can't grow delayedActions") << sizenow;
                                abort();
                            }
                            delayedActions.erase(delayedActions.cbegin()+index);
                            toDelete.push_back(index);
                            if(delayedActions.size()>delayedActions_size)
                            {
                                size_t sizenow=delayedActions.size();
                                qDebug() << QStringLiteral("MapControllerMP::reinject_signals(): can't grow delayedActions") << sizenow;
                                abort();
                            }
                        }
                        index--;
                    }
                }
                break;
                case DelayedType_Move:
                    if(otherPlayerList.find(a.move.id)!=otherPlayerList.cend())
                    {
                        if(delayedActions.size()>delayedActions_size)
                        {
                            size_t sizenow=delayedActions.size();
                            qDebug() << QStringLiteral("MapControllerMP::reinject_signals(): can't grow delayedActions") << sizenow;
                            abort();
                        }
                        if(move_player_final(a.move.id,a.move.movement,true))
                        {
                            if(delayedActions.size()>delayedActions_size)
                            {
                                size_t sizenow=delayedActions.size();
                                qDebug() << QStringLiteral("MapControllerMP::reinject_signals(): can't grow delayedActions") << sizenow;
                                abort();
                            }
                            delayedActions.erase(delayedActions.cbegin()+index);
                            toDelete.push_back(index);
                            if(delayedActions.size()>delayedActions_size)
                            {
                                size_t sizenow=delayedActions.size();
                                qDebug() << QStringLiteral("MapControllerMP::reinject_signals(): can't grow delayedActions") << sizenow;
                                abort();
                            }
                        }
                        index--;
                    }
                break;
                default:
                break;
            }
            if(delayedActions.size()>delayedActions_size)
            {
                size_t sizenow=delayedActions.size();
                qDebug() << QStringLiteral("MapControllerMP::reinject_signals(): can't grow delayedActions") << sizenow;
                abort();
            }
            index++;
        }
        if(delayedActions_size<this->delayedActions.size())
            delayedActions.insert(delayedActions.end(),this->delayedActions.cbegin()+(this->delayedActions.size()-delayedActions_size),this->delayedActions.cend());
        this->delayedActions=delayedActions;
    }
    else
        qDebug() << QStringLiteral("MapControllerMP::reinject_signals_on_valid_map(): should not pass here because all is not previously loaded");
}

bool MapControllerMP::asyncMapLoaded(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,QMap_client * tempMapObject)
{
    if(!mHaveTheDatapack)
        return false;
    if(!player_informations_is_set)
        return false;
    const bool &result=MapVisualiserPlayer::asyncMapLoaded(mapIndex,tempMapObject);
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
    if(CatchChallenger::QMap_client::all_map.find(otherPlayer.presumed_map)==CatchChallenger::QMap_client::all_map.cend())
    {
        qDebug() << "current map not loaded null pointer, unable to do finalPlayerStep()";
        return;
    }
    const CatchChallenger::QMap_client * current_map_pointer=CatchChallenger::QMap_client::all_map.at(otherPlayer.presumed_map);
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
            if(CatchChallenger::QMap_client::all_map.find(otherPlayer.current_monster_map)==CatchChallenger::QMap_client::all_map.cend())
            {
                qDebug() << "current map not loaded null pointer, unable to do finalPlayerStep()";
                return;
            }
            const CatchChallenger::QMap_client * current_monster_map_pointer=CatchChallenger::QMap_client::all_map.at(otherPlayer.current_monster_map);
            if(current_monster_map_pointer==NULL)
            {
                qDebug() << "current_monster_map_pointer not loaded null pointer, unable to do finalPlayerStep()";
                return;
            }
            {
                const CatchChallenger::MonstersCollisionValue &monstersCollisionValue=CatchChallenger::MoveOnTheMap::getZoneCollision(QtDatapackClientLoader::datapackLoader->getMap(otherPlayer.current_monster_map),otherPlayer.monster_x,otherPlayer.monster_y);
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
        const CatchChallenger::MonstersCollisionValue &monstersCollisionValue=CatchChallenger::MoveOnTheMap::getZoneCollision(QtDatapackClientLoader::datapackLoader->getMap(otherPlayer.presumed_map),otherPlayer.presumed_x,otherPlayer.presumed_y);
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
                                // Erase OLD tileset before reassign — see comment in updateOtherPlayerMonsterTile above.
                                if(otherPlayer.playerTileset)
                                    MapItem::validTilesets_.erase(otherPlayer.playerTileset.data());
                                otherPlayer.playerTileset=Tiled::Tileset::create(QString::fromStdString(lastTileset),16,24);
                                MapItem::validTilesets_.emplace(otherPlayer.playerTileset.data(), otherPlayer.playerTileset);  // see MapObjectItem.cpp cellTilesetIsValid
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
                        int tileId=cell.tile()->id();
                        cell.setTile(otherPlayer.playerTileset->tileAt(tileId));
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
                int tileId=cell.tile()->id();
                cell.setTile(otherPlayer.playerTileset->tileAt(tileId));
                otherPlayer.playerMapObject->setCell(cell);
            }
        }
    }
}

/// \warning all ObjectGroupItem destroyed into removeMap()
void MapControllerMP::destroyMap(const CATCHCHALLENGER_TYPE_MAPID &mapIndex)
{
    //remove the other player
    std::unordered_map<SIMPLIFIED_PLAYER_ID_FOR_MAP,OtherPlayer> otherPlayerList2=otherPlayerList;
    for (const std::pair<const SIMPLIFIED_PLAYER_ID_FOR_MAP,OtherPlayer> &n : otherPlayerList2) {
        if(n.second.presumed_map==mapIndex)
            remove_player_final(n.first,true);
    }
    MapVisualiserPlayer::destroyMap(mapIndex);
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

bool MapControllerMP::tileStandable(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const int &x,const int &y,const int &mw,const int &mh)
{
    if(x<0 || x>=mw || y<0 || y>=mh)
        return false;
    //A tile is standable if some orthogonal neighbour can MOVE INTO it. canGoTo()
    //checks the destination's enterability (collisions, ledges, water/lava needing
    //an item), so a Sign/NPC/water/wall tile returns false from every neighbour.
    //Probe SILENTLY: canGoTo() otherwise fires blockedOn("you can't enter without
    //the correct item") for every water/lava tile we test, flooding the player
    //with spurious tips on a single click.
    const bool prevSilent=canGoToSilent;
    canGoToSilent=true;
    bool standable=false;
    if(x+1<mw && canGoTo(CatchChallenger::Direction_move_at_left,mapIndex,static_cast<COORD_TYPE>(x+1),static_cast<COORD_TYPE>(y),true))
        standable=true;
    else if(x-1>=0 && canGoTo(CatchChallenger::Direction_move_at_right,mapIndex,static_cast<COORD_TYPE>(x-1),static_cast<COORD_TYPE>(y),true))
        standable=true;
    else if(y+1<mh && canGoTo(CatchChallenger::Direction_move_at_top,mapIndex,static_cast<COORD_TYPE>(x),static_cast<COORD_TYPE>(y+1),true))
        standable=true;
    else if(y-1>=0 && canGoTo(CatchChallenger::Direction_move_at_bottom,mapIndex,static_cast<COORD_TYPE>(x),static_cast<COORD_TYPE>(y-1),true))
        standable=true;
    canGoToSilent=prevSilent;
    return standable;
}

bool MapControllerMP::reachableClickNeighbor(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const int &cx,const int &cy,const int &px,const int &py,int &outX,int &outY)
{
    const CatchChallenger::CommonMap &lm=QtDatapackClientLoader::datapackLoader->getMap(mapIndex);
    const int mw=lm.width;
    const int mh=lm.height;
    if(mw<=0 || mh<=0 || px<0 || px>=mw || py<0 || py>=mh)
        return false;
    const CatchChallenger::Direction mv[4]={CatchChallenger::Direction_move_at_left,
                                            CatchChallenger::Direction_move_at_right,
                                            CatchChallenger::Direction_move_at_top,
                                            CatchChallenger::Direction_move_at_bottom};
    const int dxx[4]={-1,1,0,0};
    const int dyy[4]={0,0,-1,1};
    //BFS the tiles the player can WALK to (same per-step check as real movement),
    //but SILENTLY: probing water/lava tiles with canGoTo() would otherwise fire a
    //blockedOn("you can't enter without the correct item") tip for each one.
    const bool prevSilent=canGoToSilent;
    canGoToSilent=true;
    std::vector<bool> reach(static_cast<size_t>(mw)*static_cast<size_t>(mh),false);
    std::vector<std::pair<int,int> > queue;
    queue.push_back(std::pair<int,int>(px,py));
    reach[static_cast<size_t>(px)+static_cast<size_t>(py)*static_cast<size_t>(mw)]=true;
    size_t head=0;
    while(head<queue.size())
    {
        const int qx=queue.at(head).first;
        const int qy=queue.at(head).second;
        head++;
        int k=0;
        while(k<4)
        {
            const int tx=qx+dxx[k];
            const int ty=qy+dyy[k];
            if(tx>=0 && tx<mw && ty>=0 && ty<mh)
            {
                const size_t idx=static_cast<size_t>(tx)+static_cast<size_t>(ty)*static_cast<size_t>(mw);
                if(!reach[idx] && canGoTo(mv[k],mapIndex,static_cast<COORD_TYPE>(qx),static_cast<COORD_TYPE>(qy),true))
                {
                    reach[idx]=true;
                    queue.push_back(std::pair<int,int>(tx,ty));
                }
            }
            k++;
        }
    }
    //among the clicked tile's 4 neighbours, the reachable one nearest the player —
    //but never a teleporter tile (the player would warp away instead of standing
    //next to the sign to read it)
    bool found=false;
    int bestDist=0;
    int k=0;
    while(k<4)
    {
        const int nx=cx+dxx[k];
        const int ny=cy+dyy[k];
        if(nx>=0 && nx<mw && ny>=0 && ny<mh
           && reach[static_cast<size_t>(nx)+static_cast<size_t>(ny)*static_cast<size_t>(mw)])
        {
            bool isTeleporter=false;
            size_t ti=0;
            while(ti<lm.teleporters.size())
            {
                if(lm.teleporters.at(ti).source_x==nx && lm.teleporters.at(ti).source_y==ny)
                {
                    isTeleporter=true;
                    break;
                }
                ti++;
            }
            if(!isTeleporter)
            {
                const int dist=(nx>px?nx-px:px-nx)+(ny>py?ny-py:py-ny);
                if(!found || dist<bestDist)
                {
                    found=true;
                    bestDist=dist;
                    outX=nx;
                    outY=ny;
                }
            }
        }
        k++;
    }
    canGoToSilent=prevSilent;
    return found;
}

void MapControllerMP::eventOnMap(CatchChallenger::MapEvent event, const CATCHCHALLENGER_TYPE_MAPID &mapIndex, COORD_TYPE x, COORD_TYPE y)
{
    const std::pair<uint8_t,uint8_t> pos(getPos());
    const uint8_t &thisx=pos.first;
    const uint8_t &thisy=pos.second;
    if(event==CatchChallenger::MapEvent_SimpleClick)
    {
        if(keyAccepted.empty() || (keyAccepted.find(Qt::Key_Return)!=keyAccepted.cend() && keyAccepted.size()))
        {
            MapVisualiser::eventOnMap(event,mapIndex,x,y);
            //Remember the clicked tile so that, on arrival,
            //faceClickInteractTargetIfAdjacent() turns the player toward it and
            //parseAction() opens it like Enter was pressed.
            clickInteractTargetValid=true;
            clickInteractTargetMap=mapIndex;
            clickInteractTargetX=x;
            clickInteractTargetY=y;
            //A Sign/NPC sits on a NON-enterable tile, and the pathfinder cannot
            //route onto a blocked tile, so clicking the sign itself would find no
            //path. When the clicked tile is not standable, walk to the foot-
            //REACHABLE orthogonal neighbour nearest the player instead;
            //faceClickInteractTargetIfAdjacent() then turns the player to face the
            //clicked sign on arrival. Using reachability (not just nearest by
            //distance) is essential: the closest standable side may be across
            //water/walls, which would send the player to a disconnected tile.
            COORD_TYPE targetX=x,targetY=y;
            if(mapIndex==current_map)
            {
                const CatchChallenger::CommonMap &lm=QtDatapackClientLoader::datapackLoader->getMap(current_map);
                const int mw=lm.width;
                const int mh=lm.height;
                if(!tileStandable(current_map,x,y,mw,mh))
                {
                    int nX=0,nY=0;
                    if(reachableClickNeighbor(current_map,x,y,thisx,thisy,nX,nY))
                    {
                        targetX=static_cast<COORD_TYPE>(nX);
                        targetY=static_cast<COORD_TYPE>(nY);
                    }
                }
            }
            if(mapIndex==current_map && targetX==thisx && targetY==thisy)
            {
                //Already standing next to the clicked sign: no walk needed (the
                //pathfinder no-ops on a same-tile request). Face the sign and open
                //it like Enter was pressed.
                if(keyPressed.empty())
                {
                    faceClickInteractTargetIfAdjacent();
                    parseAction();
                }
            }
            else
            {
                pathFinding.searchPath(QtDatapackClientLoader::datapackLoader->get_mapList(),mapIndex,targetX,targetY,current_map,thisx,thisy,std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,CATCHCHALLENGER_TYPE_ITEM_QUANTITY>());
                if(pathList.empty())
                {
                    while(pathList.size()>1)
                        pathList.pop_back();
                    //pathList.clear();
                }
            }
        }
    }
}

void MapControllerMP::afterMapDisplayed()
{
    if(signSelfTestStarted)
        return;
    //let the player fully settle on the map before injecting the click
    if(CliClientOptions::clickSignTest)
    {
        signSelfTestStarted=true;
        QTimer::singleShot(500,this,&MapControllerMP::runClickSignSelfTest);
    }
    else if(CliClientOptions::autosoloClick)
    {
        signSelfTestStarted=true;
        QTimer::singleShot(500,this,&MapControllerMP::runAutosoloClick);
    }
}

void MapControllerMP::runAutosoloClick()
{
    if(CatchChallenger::QMap_client::all_map.find(current_map)==CatchChallenger::QMap_client::all_map.cend())
    {
        std::cerr << "[CLICKTEST] current map not loaded" << std::endl;
        return;
    }
    const CatchChallenger::CommonMap &lm=QtDatapackClientLoader::datapackLoader->getMap(current_map);
    const int mw=lm.width;
    const int mh=lm.height;
    const std::pair<COORD_TYPE,COORD_TYPE> pos(getPos());
    const int px=pos.first;
    const int py=pos.second;
    int tx=px+CliClientOptions::autosoloClickDx;
    int ty=py+CliClientOptions::autosoloClickDy;
    if(tx<0)
        tx=0;
    if(tx>=mw)
        tx=mw-1;
    if(ty<0)
        ty=0;
    if(ty>=mh)
        ty=mh-1;
    signTestMap=current_map;
    signTestX=static_cast<uint8_t>(tx);
    signTestY=static_cast<uint8_t>(ty);
    const CatchChallenger::Bot * const bot=QtDatapackClientLoader::datapackLoader->getBot(current_map,signTestX,signTestY);
    std::cerr << "[CLICKTEST] map " << current_map << ": click offset (" << CliClientOptions::autosoloClickDx << ","
              << CliClientOptions::autosoloClickDy << ") -> tile (" << tx << "," << ty << ")"
              << (bot!=nullptr?" (sign/NPC)":"") << " from player (" << px << "," << py << ")" << std::endl;
    if(!connect(this,&MapVisualiserPlayer::actionOn,this,&MapControllerMP::autosoloClickActionOn,Qt::UniqueConnection))
        abort();
    //inject the click exactly as PreparedLayer::mouseReleaseEvent() would
    eventOnMap(CatchChallenger::MapEvent_SimpleClick,current_map,signTestX,signTestY);
}

void MapControllerMP::autosoloClickActionOn(CatchChallenger::Map_client *map, const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y)
{
    Q_UNUSED(map);
    const std::pair<COORD_TYPE,COORD_TYPE> pos(getPos());
    const bool onClicked=(mapIndex==signTestMap && x==signTestX && y==signTestY);
    std::cerr << "[CLICKTEST] result: player now (" << (int)pos.first << "," << (int)pos.second
              << "), faced/acted on (" << (int)x << "," << (int)y << ") -> "
              << (onClicked?"walked up to the clicked tile, ORIENTED toward it and TRIGGERED it (like Enter)"
                           :"acted on a different tile (the clicked tile was walkable / not a sign)")
              << std::endl;
}

QPointF MapControllerMP::tileCenterScenePos(const int &tileX,const int &tileY) const
{
    //The current map is drawn at scene origin (0,0), tile pixel size =
    //CLIENT_BASE_TILE_SIZE; the centre of tile (tx,ty) is the click target.
    const double half=static_cast<double>(CLIENT_BASE_TILE_SIZE)/2.0;
    return QPointF(static_cast<double>(tileX)*CLIENT_BASE_TILE_SIZE+half,
                   static_cast<double>(tileY)*CLIENT_BASE_TILE_SIZE+half);
}

bool MapControllerMP::resolveTileAtZoom(const int &tileX,const int &tileY,int &outX,int &outY)
{
    //Reproduce the REAL on-device click maths: the sign's tile centre is mapped
    //to a viewport pixel through the ZOOMED QGraphicsView transform, then mapped
    //back to the scene, then turned into a tile the SAME way PreparedLayer does
    //(qCeil(pos/tileSize)-1). If the zoom transform is off, the round trip lands
    //on a neighbouring tile — typically the water/lava tile next to the sign,
    //which is why the device says "you can't enter without the correct item".
    const QPoint vpPt=mapFromScene(tileCenterScenePos(tileX,tileY));
    const QPointF back=mapToScene(vpPt);
    outX=static_cast<int>(qCeil(back.x()/static_cast<double>(CLIENT_BASE_TILE_SIZE))-1);
    outY=static_cast<int>(qCeil(back.y()/static_cast<double>(CLIENT_BASE_TILE_SIZE))-1);
    return true;
}

void MapControllerMP::postClickAtTile(const int &tileX,const int &tileY)
{
    //Inject a REAL left-click at the viewport pixel the sign occupies under the
    //current zoom, so the event travels the full device path
    //(pixel -> zoomed view -> scene -> PreparedLayer item -> tile) instead of
    //the old tile-coordinate shortcut. A press is needed before the release so
    //QGraphicsView sets the item as the mouse grabber for the release.
    const QPointF vpPt=QPointF(mapFromScene(tileCenterScenePos(tileX,tileY)));
    const QPointF globalPt=QPointF(viewport()->mapToGlobal(vpPt.toPoint()));
    QMouseEvent press(QEvent::MouseButtonPress,vpPt,globalPt,
                      Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(viewport(),&press);
    QMouseEvent release(QEvent::MouseButtonRelease,vpPt,globalPt,
                        Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
    QApplication::sendEvent(viewport(),&release);
}

void MapControllerMP::runClickSignSelfTest()
{
    if(CatchChallenger::QMap_client::all_map.find(current_map)==CatchChallenger::QMap_client::all_map.cend())
    {
        std::cerr << "[SIGNTEST] FAIL current map not loaded" << std::endl;
        QCoreApplication::exit(3);
        return;
    }
    const CatchChallenger::CommonMap &logicalMap=QtDatapackClientLoader::datapackLoader->getMap(current_map);
    const int mw=logicalMap.width;
    const int mh=logicalMap.height;
    const std::pair<COORD_TYPE,COORD_TYPE> pos(getPos());
    const int px=pos.first;
    const int py=pos.second;
    if(mw<=0 || mh<=0 || px<0 || px>=mw || py<0 || py>=mh)
    {
        std::cerr << "[SIGNTEST] FAIL bad map size / player out of range" << std::endl;
        QCoreApplication::exit(3);
        return;
    }
    //Find a Sign/NPC-like tile to click: a NON-standable tile whose foot-reachable
    //non-teleporter stop tile (chosen by the SAME reachableClickNeighbor() the
    //click handler uses) is FARTHEST from the player, so the click exercises the
    //longest walk this map allows. NOTE: a cramped autosolo spawn may only reach
    //signs that are already adjacent (a 0-tile walk); the multi-tile walk is
    //exercised by real gameplay on roomier maps.
    bool found=false;
    int bestDist=0;
    int bestX=0,bestY=0;
    int by2=0;
    while(by2<mh)
    {
        int bx2=0;
        while(bx2<mw)
        {
            if(!tileStandable(current_map,bx2,by2,mw,mh))
            {
                int nX=0,nY=0;
                if(reachableClickNeighbor(current_map,bx2,by2,px,py,nX,nY))
                {
                    const int dist=(nX>px?nX-px:px-nX)+(nY>py?nY-py:py-nY);
                    if(!found || dist>bestDist)
                    {
                        found=true;
                        bestDist=dist;
                        bestX=bx2;
                        bestY=by2;
                    }
                }
            }
            bx2++;
        }
        by2++;
    }
    if(!found)
    {
        std::cerr << "[SIGNTEST] FAIL no reachable sign on map " << current_map << std::endl;
        QCoreApplication::exit(3);
        return;
    }
    signTestMap=current_map;
    signTestX=static_cast<uint8_t>(bestX);
    signTestY=static_cast<uint8_t>(bestY);
    const CatchChallenger::Bot * const bot=QtDatapackClientLoader::datapackLoader->getBot(current_map,signTestX,signTestY);
    std::cerr << "[SIGNTEST] clicking interactable at (" << bestX << "," << bestY << ")"
              << (bot!=nullptr?" (sign/NPC)":"") << " from player (" << px << "," << py
              << "), expected walk distance=" << bestDist << std::endl;
    //── Zoom robustness: the on-device tap goes through the ZOOMED view, so the
    //   sign's pixel must map back to the sign's tile at EVERY zoom level. A
    //   wrong mapping lands the tap on the water/lava tile beside the sign and
    //   the engine answers "you can't enter without the correct item".
    {
        bool zoomOk=true;
        int zoom=1;
        while(zoom<=4)
        {
            setScale(static_cast<float>(zoom));
            centerOnPlayerTile();
            int rx=-1,ry=-1;
            resolveTileAtZoom(bestX,bestY,rx,ry);
            const bool match=(rx==bestX && ry==bestY);
            std::cerr << "[SIGNTEST] zoom=" << zoom << ": sign tile (" << bestX << "," << bestY
                      << ") on-screen pixel resolves to (" << rx << "," << ry << ")"
                      << (match?" MATCH":" MISMATCH") << std::endl;
            if(!match)
                zoomOk=false;
            zoom++;
        }
        if(!zoomOk)
        {
            std::cerr << "[SIGNTEST] FAIL zoom-dependent click mis-resolves: the sign's pixel "
                         "lands on the wrong tile, which on-device triggers \"you can't enter "
                         "without the correct item\"" << std::endl;
            QCoreApplication::exit(3);
            return;
        }
    }
    //── Full interaction at a fixed mid zoom, driven by a REAL dispatched click
    //   (the device path: pixel -> zoomed view -> scene -> tile), not the tile-
    //   coordinate shortcut: walk to the sign, FACE it, open it like Enter.
    setScale(2.0f);
    centerOnPlayerTile();
    //verify the outcome via actionOn (emitted when parseAction() acts on the tile
    //the player faces) plus a fallback timeout covering the walk
    if(!connect(this,&MapVisualiserPlayer::actionOn,this,&MapControllerMP::signSelfTestActionOn,Qt::UniqueConnection))
        abort();
    //catch "you can't enter ..." the instant a walk step is refused, rather than
    //waiting out the whole timeout
    if(!connect(this,&MapVisualiserPlayer::blockedOn,this,&MapControllerMP::signSelfTestBlockedOn,Qt::UniqueConnection))
        abort();
    signSelfTestTimeoutTimer.start(20000);
    postClickAtTile(bestX,bestY);
}

void MapControllerMP::signSelfTestBlockedOn(const MapVisualiserPlayer::BlockedOn &blockOnVar)
{
    signSelfTestTimeoutTimer.stop();
    const char *why="the zone";
    if(blockOnVar==MapVisualiserPlayer::BlockedOn_ZoneItem)
        why="a zone needing an item (water/lava) next to the sign";
    std::cerr << "[SIGNTEST] FAIL blocked walking to the sign: you can't enter " << why
              << " — the click routed the player into a blocked tile" << std::endl;
    QCoreApplication::exit(3);
}

void MapControllerMP::signSelfTestActionOn(CatchChallenger::Map_client *map, const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y)
{
    Q_UNUSED(map);
    signSelfTestTimeoutTimer.stop();
    const std::pair<COORD_TYPE,COORD_TYPE> pos(getPos());
    const bool ok=(mapIndex==signTestMap && x==signTestX && y==signTestY);
    if(ok)
        std::cerr << "[SIGNTEST] PASS player walked to (" << (int)pos.first << "," << (int)pos.second
                  << "), faced and acted on the clicked tile (" << (int)x << "," << (int)y
                  << ") like Enter was pressed" << std::endl;
    else
        std::cerr << "[SIGNTEST] FAIL acted on (" << (int)x << "," << (int)y << ") not the clicked tile ("
                  << (int)signTestX << "," << (int)signTestY << ")" << std::endl;
    QCoreApplication::exit(ok?0:3);
}

void MapControllerMP::signSelfTestTimeout()
{
    std::cerr << "[SIGNTEST] FAIL timeout: never reached/opened the sign at ("
              << (int)signTestX << "," << (int)signTestY << ")" << std::endl;
    QCoreApplication::exit(3);
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

void MapControllerMP::pathFindingResult(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y, const std::vector<std::pair<CatchChallenger::Orientation, uint8_t> > &path, const PathFinding::PathFinding_status &status)
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
    //manual keyboard movement cancels any pending click-to-interact target
    clickInteractTargetValid=false;
    MapVisualiserPlayerWithFight::keyPressParse();
}
