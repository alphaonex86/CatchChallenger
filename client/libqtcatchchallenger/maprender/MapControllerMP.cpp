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
#include "../LocalListener.hpp"
#include <iostream>
#include <QDebug>
#include <QFileInfo>
#include <QCoreApplication>
#include <QApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QStringList>
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
    remoteActionInProgress=false;

    signSelfTestStarted=false;
    signTestMap=0;
    signTestX=0;
    signTestY=0;
    signSelfTestTimeoutTimer.setSingleShot(true);
    if(!connect(&signSelfTestTimeoutTimer,&QTimer::timeout,this,&MapControllerMP::signSelfTestTimeout))
        abort();

    doorTestPhase=0;
    doorOrigMap=0;
    doorDestMap=0;
    doorTestTimeoutTimer.setSingleShot(true);
    if(!connect(&doorTestTimeoutTimer,&QTimer::timeout,this,&MapControllerMP::doorTestTimeout))
        abort();

    kbPhase=0;
    kbOrigMap=0;
    kbIndoorMap=0;
    kbNavMap=0;
    kbSignX=0;
    kbSignY=0;
    kbSignOpened=false;
    kbPathStep=0;
    kbTimer.setSingleShot(true);
    if(!connect(&kbTimer,&QTimer::timeout,this,&MapControllerMP::kbTick))
        abort();
    kbTimeoutTimer.setSingleShot(true);
    if(!connect(&kbTimeoutTimer,&QTimer::timeout,this,&MapControllerMP::keyboardTestTimeout))
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
    //Null our cached `client` the instant the Api_protocol_Qt is deleted. A
    //newError() (e.g. a fight action sent out of fight) calls
    //client->tryDisconnect(), which can tear the autosolo session down and delete
    //the client while the map controller is still alive; without this, the
    //automation channel (and any null-checked path) would dereference a freed
    //vtable. Default (direct) connection -> runs synchronously inside ~QObject,
    //before the next command is processed.
    if(!QObject::connect(client,&QObject::destroyed,this,&MapControllerMP::clientDestroyedSlot))
        abort();
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
    //QLocalServer automation channel (stage 2): buffer every chat / system line the
    //server delivers so a controller can drain them with GETCHAT. Independent of the
    //real chat UI's own connections; harmless when no controller is attached.
    if(!QObject::connect(client,&CatchChallenger::Api_protocol_Qt::Qtnew_chat_text,   this,&MapControllerMP::remoteChatReceived,Qt::QueuedConnection))
        abort();
    if(!QObject::connect(client,&CatchChallenger::Api_protocol_Qt::Qtnew_system_text, this,&MapControllerMP::remoteSystemReceived,Qt::QueuedConnection))
        abort();
    //QLocalServer automation channel (stage 3): buffer trade events for GETTRADE
    if(!QObject::connect(client,&CatchChallenger::Api_protocol_Qt::QttradeRequested,        this,&MapControllerMP::remoteTradeRequested,Qt::QueuedConnection))
        abort();
    if(!QObject::connect(client,&CatchChallenger::Api_protocol_Qt::QttradeAcceptedByOther,  this,&MapControllerMP::remoteTradeAcceptedByOther,Qt::QueuedConnection))
        abort();
    if(!QObject::connect(client,&CatchChallenger::Api_protocol_Qt::QttradeCanceledByOther,  this,&MapControllerMP::remoteTradeCanceledByOther,Qt::QueuedConnection))
        abort();
    if(!QObject::connect(client,&CatchChallenger::Api_protocol_Qt::QttradeFinishedByOther,  this,&MapControllerMP::remoteTradeFinishedByOther,Qt::QueuedConnection))
        abort();
    if(!QObject::connect(client,&CatchChallenger::Api_protocol_Qt::QttradeValidatedByTheServer,this,&MapControllerMP::remoteTradeValidatedByTheServer,Qt::QueuedConnection))
        abort();
    if(!QObject::connect(client,&CatchChallenger::Api_protocol_Qt::QttradeAddTradeCash,     this,&MapControllerMP::remoteTradeAddTradeCash,Qt::QueuedConnection))
        abort();
    if(!QObject::connect(client,&CatchChallenger::Api_protocol_Qt::QttradeAddTradeObject,   this,&MapControllerMP::remoteTradeAddTradeObject,Qt::QueuedConnection))
        abort();
    if(!QObject::connect(client,&CatchChallenger::Api_protocol_Qt::QttradeAddTradeMonster,  this,&MapControllerMP::remoteTradeAddTradeMonster,Qt::QueuedConnection))
        abort();
    //QLocalServer automation channel (stage 4): buffer fight events for GETFIGHT
    if(!QObject::connect(client,&CatchChallenger::Api_protocol_Qt::QtbattleRequested,       this,&MapControllerMP::remoteBattleRequested,Qt::QueuedConnection))
        abort();
    if(!QObject::connect(client,&CatchChallenger::Api_protocol_Qt::QtbattleAcceptedByOther, this,&MapControllerMP::remoteBattleAcceptedByOther,Qt::QueuedConnection))
        abort();
    if(!QObject::connect(client,&CatchChallenger::Api_protocol_Qt::QtbattleCanceledByOther, this,&MapControllerMP::remoteBattleCanceledByOther,Qt::QueuedConnection))
        abort();
    if(!QObject::connect(client,&CatchChallenger::Api_protocol_Qt::QtsendBattleReturn,      this,&MapControllerMP::remoteSendBattleReturn,Qt::QueuedConnection))
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
                //"teleport on push" exit: the clicked tile is a teleporter source
                //sitting on a COLLISION tile (you LEAVE by pushing into it). The
                //pathfinder can never stand on it, so arm the push and walk to a
                //reachable neighbour; on arrival (or right now when already next
                //to it) pushTeleportIfAdjacent() steps in and the map swaps.
                //A walkable DOOR teleporter stays on the normal flow below: the
                //pathfinder routes straight onto it.
                bool isPushTeleporter=false;
                {
                    size_t ti=0;
                    while(ti<lm.teleporters.size())
                    {
                        if(lm.teleporters.at(ti).source_x==x && lm.teleporters.at(ti).source_y==y)
                        {
                            //direction only matters for ledges, never a teleporter source
                            if(!CatchChallenger::MoveOnTheMap::isWalkableWithDirection(lm,x,y,CatchChallenger::Direction_move_at_bottom))
                                isPushTeleporter=true;
                            break;
                        }
                        ti++;
                    }
                }
                if(isPushTeleporter)
                {
                    clickInteractPushValid=true;
                    clickInteractPushMap=current_map;
                    clickInteractPushX=x;
                    clickInteractPushY=y;
                    int nX=0,nY=0;
                    if(reachableClickNeighbor(current_map,x,y,thisx,thisy,nX,nY))
                    {
                        targetX=static_cast<COORD_TYPE>(nX);
                        targetY=static_cast<COORD_TYPE>(nY);
                    }
                }
                else if(!tileStandable(current_map,x,y,mw,mh))
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
                //pathfinder no-ops on a same-tile request). Push into the clicked
                //teleport-on-push exit if one was armed above, else face the sign
                //and open it like Enter was pressed.
                if(keyPressed.empty())
                {
                    if(!pushTeleportIfAdjacent())
                    {
                        faceClickInteractTargetIfAdjacent();
                        parseAction();
                    }
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
    if(CliClientOptions::clickDoorTest)
    {
        //re-entrant: the door round-trip advances on every current-map display
        //(initial spawn, then after each teleport). Settle before each step.
        QTimer::singleShot(500,this,&MapControllerMP::runClickDoorSelfTest);
        return;
    }
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
    else if(CliClientOptions::keyboardTest)
    {
        //start once; the kbTick() state machine then drives itself through the
        //map changes (it survives the door/return teleports).
        signSelfTestStarted=true;
        QTimer::singleShot(800,this,&MapControllerMP::runKeyboardSelfTest);
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
    const QPoint vpPt=mapFromScene(tileCenterScenePos(tileX,tileY));
    postClickAtPixel(vpPt.x(),vpPt.y());
}

void MapControllerMP::postClickAtPixel(const int &px,const int &py)
{
    //A press is needed before the release so QGraphicsView sets the item under
    //the cursor as the mouse grabber for the release.
    const QPointF vpPt(px,py);
    const QPointF globalPt=QPointF(viewport()->mapToGlobal(QPoint(px,py)));
    QMouseEvent press(QEvent::MouseButtonPress,vpPt,globalPt,
                      Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(viewport(),&press);
    QMouseEvent release(QEvent::MouseButtonRelease,vpPt,globalPt,
                        Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
    QApplication::sendEvent(viewport(),&release);
}

int MapControllerMP::remoteKeyNameToQt(const QString &name) const
{
    const QString n=name.toUpper();
    if(n==QStringLiteral("UP"))     return Qt::Key_Up;
    if(n==QStringLiteral("DOWN"))   return Qt::Key_Down;
    if(n==QStringLiteral("LEFT"))   return Qt::Key_Left;
    if(n==QStringLiteral("RIGHT"))  return Qt::Key_Right;
    if(n==QStringLiteral("RETURN") || n==QStringLiteral("ENTER")) return Qt::Key_Return;
    if(n==QStringLiteral("ESCAPE") || n==QStringLiteral("ESC"))   return Qt::Key_Escape;
    if(n==QStringLiteral("SPACE"))  return Qt::Key_Space;
    return 0;
}

void MapControllerMP::remoteAction(const QString &line)
{
    //Re-entrancy guard. A command can spin a NESTED event loop — e.g. a fight
    //action sent out of fight -> Api_protocol::useSkill() -> newError() ->
    //client->tryDisconnect() + a modal QMessageBox::critical(). While that modal
    //loop runs, the next buffered socket command would re-enter remoteAction
    //against a client that is mid-teardown (its deleteLater() fires when the
    //nested loop yields) -> use-after-free / call through a freed vtable.
    //Serialise instead: queue every command and run them one at a time from the
    //outermost call. The drain runs synchronously, before control returns to the
    //event loop, so a pending deleteLater() cannot free the client between
    //commands.
    remoteActionPending.push_back(line);
    if(remoteActionInProgress)
        return;
    remoteActionInProgress=true;
    while(!remoteActionPending.empty())
    {
        const QString next=remoteActionPending.front();
        remoteActionPending.erase(remoteActionPending.begin());
        remoteActionExecute(next);
    }
    remoteActionInProgress=false;
}

void MapControllerMP::remoteActionExecute(const QString &line)
{
    const QStringList parts=line.trimmed().split(QLatin1Char(' '),Qt::SkipEmptyParts);
    if(parts.isEmpty())
        return;
    const QString verb=parts.at(0).toUpper();
    if(verb==QStringLiteral("KEY") && parts.size()>=2)
    {
        const int key=remoteKeyNameToQt(parts.at(1));
        if(key==0)
            emit remoteReply(QStringLiteral("ERROR unknown key ")+parts.at(1));
        else
        {
            synthKey(key);
            emit remoteReply(QStringLiteral("OK KEY ")+parts.at(1).toUpper());
        }
    }
    else if(verb==QStringLiteral("CLICKTILE") && parts.size()>=3)
    {
        bool ok1=false,ok2=false;
        const int x=parts.at(1).toInt(&ok1);
        const int y=parts.at(2).toInt(&ok2);
        if(!ok1 || !ok2)
            emit remoteReply(QStringLiteral("ERROR bad CLICKTILE args"));
        else
        {
            postClickAtTile(x,y);
            emit remoteReply(QStringLiteral("OK CLICKTILE %1 %2").arg(x).arg(y));
        }
    }
    else if(verb==QStringLiteral("CLICKPIXEL") && parts.size()>=3)
    {
        bool ok1=false,ok2=false;
        const int px=parts.at(1).toInt(&ok1);
        const int py=parts.at(2).toInt(&ok2);
        if(!ok1 || !ok2)
            emit remoteReply(QStringLiteral("ERROR bad CLICKPIXEL args"));
        else
        {
            postClickAtPixel(px,py);
            emit remoteReply(QStringLiteral("OK CLICKPIXEL %1 %2").arg(px).arg(py));
        }
    }
    else if(verb==QStringLiteral("GETSTATE"))
    {
        const std::pair<COORD_TYPE,COORD_TYPE> p(getPos());
        emit remoteReply(QStringLiteral("STATE map=%1 x=%2 y=%3 dir=%4")
                         .arg(static_cast<int>(current_map))
                         .arg(static_cast<int>(p.first))
                         .arg(static_cast<int>(p.second))
                         .arg(static_cast<int>(getDirection())));
    }
    else if(verb==QStringLiteral("GETINVENTORY"))
    {
        QString out=QStringLiteral("INVENTORY");
        if(client!=nullptr)
        {
            const std::map<CATCHCHALLENGER_TYPE_ITEM,CATCHCHALLENGER_TYPE_ITEM_QUANTITY> &items=client->get_player_informations().items;
            std::map<CATCHCHALLENGER_TYPE_ITEM,CATCHCHALLENGER_TYPE_ITEM_QUANTITY>::const_iterator it=items.cbegin();
            while(it!=items.cend())
            {
                out+=QStringLiteral(" %1:%2").arg(static_cast<int>(it->first)).arg(static_cast<qulonglong>(it->second));
                ++it;
            }
        }
        emit remoteReply(out);
    }
    else if(verb==QStringLiteral("SENDCHAT") && parts.size()>=3)
    {
        const int chatType=remoteChatTypeFromName(parts.at(1));
        if(chatType<0)
            emit remoteReply(QStringLiteral("ERROR unknown chat channel ")+parts.at(1));
        else if(client==nullptr)
            emit remoteReply(QStringLiteral("ERROR not connected"));
        else
        {
            //text = everything after "SENDCHAT <channel> " (keep inner spaces)
            const int sp1=line.indexOf(QLatin1Char(' '));
            const int sp2=line.indexOf(QLatin1Char(' '),sp1+1);
            const QString text=line.mid(sp2+1);
            client->sendChatText(static_cast<CatchChallenger::Chat_type>(chatType),text.toStdString());
            emit remoteReply(QStringLiteral("OK SENDCHAT ")+parts.at(1).toLower());
        }
    }
    else if(verb==QStringLiteral("GETCHAT"))
    {
        std::size_t i=0;
        while(i<remoteChatLog.size())
        {
            emit remoteReply(QStringLiteral("CHATMSG ")+QString::fromStdString(remoteChatLog.at(i)));
            ++i;
        }
        emit remoteReply(QStringLiteral("OK GETCHAT %1").arg(static_cast<qulonglong>(remoteChatLog.size())));
        remoteChatLog.clear();
    }
    else if(verb==QStringLiteral("TRADEREQUEST") && parts.size()>=2)
    {
        if(client==nullptr)
            emit remoteReply(QStringLiteral("ERROR not connected"));
        else
        {
            //a trade is initiated by the "/trade <pseudo>" chat command, which the
            //server parses (ClientNetworkReadMessage); send it as a local message.
            client->sendChatText(CatchChallenger::Chat_type_local,std::string("/trade ")+parts.at(1).toStdString());
            emit remoteReply(QStringLiteral("OK TRADEREQUEST ")+parts.at(1));
        }
    }
    else if(verb==QStringLiteral("TRADEACCEPT"))
    {
        if(client==nullptr) emit remoteReply(QStringLiteral("ERROR not connected"));
        else { client->tradeAccepted(); emit remoteReply(QStringLiteral("OK TRADEACCEPT")); }
    }
    else if(verb==QStringLiteral("TRADEREFUSE"))
    {
        if(client==nullptr) emit remoteReply(QStringLiteral("ERROR not connected"));
        else { client->tradeRefused(); emit remoteReply(QStringLiteral("OK TRADEREFUSE")); }
    }
    else if(verb==QStringLiteral("TRADEADDCASH") && parts.size()>=2)
    {
        bool ok=false;
        const qulonglong cash=parts.at(1).toULongLong(&ok);
        if(!ok) emit remoteReply(QStringLiteral("ERROR bad TRADEADDCASH arg"));
        else if(client==nullptr) emit remoteReply(QStringLiteral("ERROR not connected"));
        else { client->addTradeCash(static_cast<uint64_t>(cash)); emit remoteReply(QStringLiteral("OK TRADEADDCASH %1").arg(cash)); }
    }
    else if(verb==QStringLiteral("TRADEADDITEM") && parts.size()>=3)
    {
        bool ok1=false,ok2=false;
        const uint item=parts.at(1).toUInt(&ok1);
        const uint qty=parts.at(2).toUInt(&ok2);
        if(!ok1 || !ok2) emit remoteReply(QStringLiteral("ERROR bad TRADEADDITEM args"));
        else if(client==nullptr) emit remoteReply(QStringLiteral("ERROR not connected"));
        else { client->addObject(static_cast<CATCHCHALLENGER_TYPE_ITEM>(item),qty); emit remoteReply(QStringLiteral("OK TRADEADDITEM %1 %2").arg(item).arg(qty)); }
    }
    else if(verb==QStringLiteral("TRADEADDMONSTER") && parts.size()>=2)
    {
        bool ok=false;
        const uint pos=parts.at(1).toUInt(&ok);
        if(!ok) emit remoteReply(QStringLiteral("ERROR bad TRADEADDMONSTER arg"));
        else if(client==nullptr) emit remoteReply(QStringLiteral("ERROR not connected"));
        else { client->addMonsterByPosition(static_cast<uint8_t>(pos)); emit remoteReply(QStringLiteral("OK TRADEADDMONSTER %1").arg(pos)); }
    }
    else if(verb==QStringLiteral("TRADEFINISH"))
    {
        if(client==nullptr) emit remoteReply(QStringLiteral("ERROR not connected"));
        else { client->tradeFinish(); emit remoteReply(QStringLiteral("OK TRADEFINISH")); }
    }
    else if(verb==QStringLiteral("TRADECANCEL"))
    {
        if(client==nullptr) emit remoteReply(QStringLiteral("ERROR not connected"));
        else { client->tradeCanceled(); emit remoteReply(QStringLiteral("OK TRADECANCEL")); }
    }
    else if(verb==QStringLiteral("GETTRADE"))
    {
        std::size_t i=0;
        while(i<remoteTradeLog.size())
        {
            emit remoteReply(QStringLiteral("TRADEEVENT ")+QString::fromStdString(remoteTradeLog.at(i)));
            ++i;
        }
        emit remoteReply(QStringLiteral("OK GETTRADE %1").arg(static_cast<qulonglong>(remoteTradeLog.size())));
        remoteTradeLog.clear();
    }
    else if(verb==QStringLiteral("FIGHTREQUEST") && parts.size()>=2)
    {
        if(client==nullptr)
            emit remoteReply(QStringLiteral("ERROR not connected"));
        else
        {
            //a PvP fight is initiated by the "/battle <pseudo>" chat command
            client->sendChatText(CatchChallenger::Chat_type_local,std::string("/battle ")+parts.at(1).toStdString());
            emit remoteReply(QStringLiteral("OK FIGHTREQUEST ")+parts.at(1));
        }
    }
    else if(verb==QStringLiteral("FIGHTACCEPT"))
    {
        if(client==nullptr) emit remoteReply(QStringLiteral("ERROR not connected"));
        else { client->battleAccepted(); emit remoteReply(QStringLiteral("OK FIGHTACCEPT")); }
    }
    else if(verb==QStringLiteral("FIGHTREFUSE"))
    {
        if(client==nullptr) emit remoteReply(QStringLiteral("ERROR not connected"));
        else { client->battleRefused(); emit remoteReply(QStringLiteral("OK FIGHTREFUSE")); }
    }
    else if(verb==QStringLiteral("FIGHTSKILL") && parts.size()>=2)
    {
        bool ok=false;
        const uint skill=parts.at(1).toUInt(&ok);
        if(!ok) emit remoteReply(QStringLiteral("ERROR bad FIGHTSKILL arg"));
        else if(client==nullptr) emit remoteReply(QStringLiteral("ERROR not connected"));
        else { client->useSkill(static_cast<CATCHCHALLENGER_TYPE_SKILL>(skill)); emit remoteReply(QStringLiteral("OK FIGHTSKILL %1").arg(skill)); }
    }
    else if(verb==QStringLiteral("FIGHTCHANGEMONSTER") && parts.size()>=2)
    {
        bool ok=false;
        const uint pos=parts.at(1).toUInt(&ok);
        if(!ok) emit remoteReply(QStringLiteral("ERROR bad FIGHTCHANGEMONSTER arg"));
        else if(client==nullptr) emit remoteReply(QStringLiteral("ERROR not connected"));
        else { client->changeOfMonsterInFightByPosition(static_cast<uint8_t>(pos)); emit remoteReply(QStringLiteral("OK FIGHTCHANGEMONSTER %1").arg(pos)); }
    }
    else if(verb==QStringLiteral("FIGHTITEM") && parts.size()>=2)
    {
        bool ok1=false;
        const uint item=parts.at(1).toUInt(&ok1);
        if(!ok1) emit remoteReply(QStringLiteral("ERROR bad FIGHTITEM arg"));
        else if(client==nullptr) emit remoteReply(QStringLiteral("ERROR not connected"));
        else if(parts.size()>=3)
        {
            //use the item on a specific team monster (capture ball, potion, ...)
            bool ok2=false;
            const uint pos=parts.at(2).toUInt(&ok2);
            if(!ok2) emit remoteReply(QStringLiteral("ERROR bad FIGHTITEM monster arg"));
            else { client->useObjectOnMonsterByPosition(static_cast<CATCHCHALLENGER_TYPE_ITEM>(item),static_cast<uint8_t>(pos)); emit remoteReply(QStringLiteral("OK FIGHTITEM %1 %2").arg(item).arg(pos)); }
        }
        else { client->useObject(static_cast<CATCHCHALLENGER_TYPE_ITEM>(item)); emit remoteReply(QStringLiteral("OK FIGHTITEM %1").arg(item)); }
    }
    else if(verb==QStringLiteral("FIGHTESCAPE"))
    {
        if(client==nullptr) emit remoteReply(QStringLiteral("ERROR not connected"));
        else { client->sendTryEscape(); emit remoteReply(QStringLiteral("OK FIGHTESCAPE")); }
    }
    else if(verb==QStringLiteral("GETFIGHT"))
    {
        std::size_t i=0;
        while(i<remoteFightLog.size())
        {
            emit remoteReply(QStringLiteral("FIGHTEVENT ")+QString::fromStdString(remoteFightLog.at(i)));
            ++i;
        }
        const int inFight=(client!=nullptr && client->isInFight())?1:0;
        emit remoteReply(QStringLiteral("OK GETFIGHT inFight=%1 count=%2").arg(inFight).arg(static_cast<qulonglong>(remoteFightLog.size())));
        remoteFightLog.clear();
    }
    else
        emit remoteReply(QStringLiteral("ERROR unknown command ")+verb);
}

int MapControllerMP::remoteChatTypeFromName(const QString &name) const
{
    const QString n=name.toLower();
    if(n==QStringLiteral("local"))            return CatchChallenger::Chat_type_local;
    if(n==QStringLiteral("all"))              return CatchChallenger::Chat_type_all;
    if(n==QStringLiteral("clan"))             return CatchChallenger::Chat_type_clan;
    if(n==QStringLiteral("pm"))               return CatchChallenger::Chat_type_pm;
    if(n==QStringLiteral("system"))           return CatchChallenger::Chat_type_system;
    if(n==QStringLiteral("system_important")) return CatchChallenger::Chat_type_system_important;
    return -1;
}

QString MapControllerMP::remoteChatTypeName(const uint8_t &chatType) const
{
    switch(chatType)
    {
        case CatchChallenger::Chat_type_local:            return QStringLiteral("local");
        case CatchChallenger::Chat_type_all:              return QStringLiteral("all");
        case CatchChallenger::Chat_type_clan:             return QStringLiteral("clan");
        case CatchChallenger::Chat_type_pm:               return QStringLiteral("pm");
        case CatchChallenger::Chat_type_system:           return QStringLiteral("system");
        case CatchChallenger::Chat_type_system_important: return QStringLiteral("system_important");
        default:                                          return QStringLiteral("unknown");
    }
}

void MapControllerMP::remoteChatReceived(const CatchChallenger::Chat_type &chat_type,const std::string &text,const std::string &pseudo,const CatchChallenger::Player_type &player_type)
{
    (void)player_type;
    //format "<channel> <pseudo> <text>"; '-' pseudo when empty
    std::string line=remoteChatTypeName(chat_type).toStdString();
    line+=' ';
    line+=pseudo.empty()?std::string("-"):pseudo;
    line+=' ';
    line+=text;
    rememberRemoteEvent(remoteChatLog,line);
}

void MapControllerMP::remoteSystemReceived(const CatchChallenger::Chat_type &chat_type,const std::string &text)
{
    std::string line=remoteChatTypeName(chat_type).toStdString();
    line+=" - ";
    line+=text;
    rememberRemoteEvent(remoteChatLog,line);
}

void MapControllerMP::clientDestroyedSlot(QObject *obj)
{
    //only null if the destroyed object IS our current client (a replaced client
    //from a new character selection must not clear the new pointer)
    if(static_cast<QObject*>(client)==obj)
        client=nullptr;
}

void MapControllerMP::rememberRemoteEvent(std::vector<std::string> &log,const std::string &line)
{
    //bound the buffer so a long un-drained session can't grow without limit
    if(log.size()>=256)
        log.erase(log.begin());
    log.push_back(line);
}

void MapControllerMP::remoteTradeRequested(const std::string &pseudo,const uint8_t &skinInt)
{
    (void)skinInt;
    rememberRemoteEvent(remoteTradeLog,std::string("REQUESTED ")+pseudo);
}

void MapControllerMP::remoteTradeAcceptedByOther(const std::string &pseudo,const uint8_t &skinInt)
{
    (void)skinInt;
    rememberRemoteEvent(remoteTradeLog,std::string("ACCEPTED ")+pseudo);
}

void MapControllerMP::remoteTradeCanceledByOther()
{
    rememberRemoteEvent(remoteTradeLog,std::string("CANCELED"));
}

void MapControllerMP::remoteTradeFinishedByOther()
{
    rememberRemoteEvent(remoteTradeLog,std::string("FINISHED"));
}

void MapControllerMP::remoteTradeValidatedByTheServer()
{
    rememberRemoteEvent(remoteTradeLog,std::string("VALIDATED"));
}

void MapControllerMP::remoteTradeAddTradeCash(const uint64_t &cash)
{
    rememberRemoteEvent(remoteTradeLog,std::string("ADDCASH ")+std::to_string(cash));
}

void MapControllerMP::remoteTradeAddTradeObject(const CATCHCHALLENGER_TYPE_ITEM &item,const uint32_t &quantity)
{
    rememberRemoteEvent(remoteTradeLog,std::string("ADDITEM ")+std::to_string(item)+" "+std::to_string(quantity));
}

void MapControllerMP::remoteTradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster)
{
    rememberRemoteEvent(remoteTradeLog,std::string("ADDMONSTER ")+std::to_string(monster.monster)+" level "+std::to_string(monster.level));
}

void MapControllerMP::remoteBattleRequested(const std::string &pseudo,const uint8_t &skinInt)
{
    (void)skinInt;
    rememberRemoteEvent(remoteFightLog,std::string("REQUESTED ")+pseudo);
}

void MapControllerMP::remoteBattleAcceptedByOther(const std::string &pseudo,const uint8_t &skinId,const std::vector<uint8_t> &stat,const uint8_t &monsterPlace,const CatchChallenger::PublicPlayerMonster &publicPlayerMonster)
{
    (void)skinId;(void)stat;(void)monsterPlace;
    rememberRemoteEvent(remoteFightLog,std::string("ACCEPTED ")+pseudo+" monster "+std::to_string(publicPlayerMonster.monster));
}

void MapControllerMP::remoteBattleCanceledByOther()
{
    rememberRemoteEvent(remoteFightLog,std::string("CANCELED"));
}

void MapControllerMP::remoteSendBattleReturn(const std::vector<CatchChallenger::Skill::AttackReturn> &attackReturn)
{
    rememberRemoteEvent(remoteFightLog,std::string("ATTACKRETURN ")+std::to_string(attackReturn.size()));
}

void MapControllerMP::wireRemoteControl(LocalListener *localListener)
{
    if(localListener==nullptr)
        return;
    if(!connect(localListener,&LocalListener::actionReceived,this,&MapControllerMP::remoteAction,Qt::UniqueConnection))
        abort();
    if(!connect(this,&MapControllerMP::remoteReply,localListener,&LocalListener::sendReply,Qt::UniqueConnection))
        abort();
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

void MapControllerMP::computeReachableSet(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const int &px,const int &py,std::vector<bool> &reach,int &mw,int &mh)
{
    const CatchChallenger::CommonMap &lm=QtDatapackClientLoader::datapackLoader->getMap(mapIndex);
    mw=lm.width;
    mh=lm.height;
    reach.assign(static_cast<size_t>(mw>0?mw:0)*static_cast<size_t>(mh>0?mh:0),false);
    if(mw<=0 || mh<=0 || px<0 || px>=mw || py<0 || py>=mh)
        return;
    const CatchChallenger::Direction mv[4]={CatchChallenger::Direction_move_at_left,
                                            CatchChallenger::Direction_move_at_right,
                                            CatchChallenger::Direction_move_at_top,
                                            CatchChallenger::Direction_move_at_bottom};
    const int dxx[4]={-1,1,0,0};
    const int dyy[4]={0,0,-1,1};
    //probe SILENTLY: never pop "you can't enter ..." while mapping reachability
    const bool prevSilent=canGoToSilent;
    canGoToSilent=true;
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
    canGoToSilent=prevSilent;
}

bool MapControllerMP::pickReachableTeleporter(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const int &px,const int &py,const CATCHCHALLENGER_TYPE_MAPID &requireDest,uint8_t &srcX,uint8_t &srcY,CATCHCHALLENGER_TYPE_MAPID &destMap,bool &isPush,int &neighX,int &neighY)
{
    const CatchChallenger::CommonMap &lm=QtDatapackClientLoader::datapackLoader->getMap(mapIndex);
    int mw=0,mh=0;
    std::vector<bool> reach;
    computeReachableSet(mapIndex,px,py,reach,mw,mh);
    if(mw<=0 || mh<=0)
        return false;
    const int dxx[4]={-1,1,0,0};
    const int dyy[4]={0,0,-1,1};
    bool found=false;
    int bestDist=0;
    size_t ti=0;
    while(ti<lm.teleporters.size())
    {
        const CatchChallenger::Teleporter &tp=lm.teleporters.at(ti);
        const int sx=tp.source_x;
        const int sy=tp.source_y;
        const bool destOk=(requireDest==65535 || tp.mapIndex==requireDest);
        if(destOk && sx>=0 && sx<mw && sy>=0 && sy<mh)
        {
            const bool srcReach=reach[static_cast<size_t>(sx)+static_cast<size_t>(sy)*static_cast<size_t>(mw)];
            //nearest reachable orthogonal neighbour (so we can stand next to it
            //and push in even when we don't want to click the source directly)
            int nX=-1,nY=-1,nBest=0;
            int k=0;
            while(k<4)
            {
                const int nx=sx+dxx[k];
                const int ny=sy+dyy[k];
                if(nx>=0 && nx<mw && ny>=0 && ny<mh
                   && reach[static_cast<size_t>(nx)+static_cast<size_t>(ny)*static_cast<size_t>(mw)])
                {
                    const int nd=(nx>px?nx-px:px-nx)+(ny>py?ny-py:py-ny);
                    if(nX<0 || nd<nBest)
                    {
                        nBest=nd;
                        nX=nx;
                        nY=ny;
                    }
                }
                k++;
            }
            std::cerr << "[DOORTEST]  teleporter src=(" << sx << "," << sy << ") -> map " << (int)tp.mapIndex
                      << " srcReachable=" << (srcReach?"yes":"no") << " reachNeighbour="
                      << (nX>=0?"yes":"no") << std::endl;
            if(srcReach || nX>=0)
            {
                const int dist=(sx>px?sx-px:px-sx)+(sy>py?sy-py:py-sy);
                if(!found || dist<bestDist)
                {
                    found=true;
                    bestDist=dist;
                    srcX=static_cast<uint8_t>(sx);
                    srcY=static_cast<uint8_t>(sy);
                    destMap=tp.mapIndex;
                    //push when we can't (or won't) stand on the source; a reachable
                    //neighbour is required to push
                    isPush=(!srcReach && nX>=0);
                    neighX=nX;
                    neighY=nY;
                }
            }
        }
        ti++;
    }
    return found;
}

void MapControllerMP::clickTeleporter(const uint8_t &srcX,const uint8_t &srcY,const bool &isPush,const int &neighX,const int &neighY)
{
    if(!isPush)
    {
        //DOOR: click straight on the teleporter tile; the pathfinder routes the
        //player onto it (the source is walkable) and finalPlayerStepTeleported()
        //swaps the map.
        std::cerr << "[DOORTEST] clicking ON the door tile (" << (int)srcX << "," << (int)srcY << ")" << std::endl;
        postClickAtTile(srcX,srcY);
    }
    else
    {
        //PUSH wall: arm the pending push onto the teleporter source.
        clickInteractPushValid=true;
        clickInteractPushMap=current_map;
        clickInteractPushX=srcX;
        clickInteractPushY=srcY;
        const std::pair<uint8_t,uint8_t> pos(getPos());
        const int adx=(srcX>pos.first?srcX-pos.first:pos.first-srcX);
        const int ady=(srcY>pos.second?srcY-pos.second:pos.second-srcY);
        if((adx==1 && ady==0) || (adx==0 && ady==1))
        {
            //the door dropped us right next to the return teleport: push in NOW
            //(clicking our own tile would hit the player sprite, not the map).
            std::cerr << "[DOORTEST] already next to the teleport (" << (int)srcX << "," << (int)srcY
                      << "): pushing in" << std::endl;
            pushTeleportIfAdjacent();
        }
        else
        {
            //walk up to the reachable neighbour, then push onto the source on arrival
            std::cerr << "[DOORTEST] clicking NEXT TO the teleport at neighbour (" << neighX << "," << neighY
                      << ") then pushing onto (" << (int)srcX << "," << (int)srcY << ")" << std::endl;
            postClickAtTile(neighX,neighY);
        }
    }
}

void MapControllerMP::runClickDoorSelfTest()
{
    if(CatchChallenger::QMap_client::all_map.find(current_map)==CatchChallenger::QMap_client::all_map.cend())
        return;
    const std::pair<COORD_TYPE,COORD_TYPE> pos(getPos());
    const int px=pos.first;
    const int py=pos.second;
    if(doorTestPhase==0)
    {
        //── Phase 0: on the spawn map, click a DOOR and pass to the other map.
        doorTestPhase=1;
        doorOrigMap=current_map;
        std::cerr << "[DOORTEST] phase0 on map " << (int)current_map << " player (" << px << "," << py
                  << "): looking for a reachable door" << std::endl;
        uint8_t sx=0,sy=0;
        CATCHCHALLENGER_TYPE_MAPID dest=65535;
        bool isPush=false;
        int nX=-1,nY=-1;
        if(!pickReachableTeleporter(current_map,px,py,65535,sx,sy,dest,isPush,nX,nY))
        {
            std::cerr << "[DOORTEST] FAIL no reachable door/teleporter on the spawn map " << (int)current_map << std::endl;
            QCoreApplication::exit(3);
            return;
        }
        doorDestMap=dest;
        std::cerr << "[DOORTEST] door at (" << (int)sx << "," << (int)sy << ") -> map " << (int)dest << std::endl;
        doorTestTimeoutTimer.start(20000);
        //forward leg uses the DOOR interaction (click straight on it)
        clickTeleporter(sx,sy,false,nX,nY);
    }
    else if(doorTestPhase==1)
    {
        //wait until we actually arrived on the door's destination map
        if(current_map==doorOrigMap)
            return;
        if(current_map!=doorDestMap)
        {
            std::cerr << "[DOORTEST] FAIL door led to map " << (int)current_map << " not the expected " << (int)doorDestMap << std::endl;
            QCoreApplication::exit(3);
            return;
        }
        std::cerr << "[DOORTEST] PASS-1 clicked the door and passed from map " << (int)doorOrigMap
                  << " to map " << (int)doorDestMap << std::endl;
        //── Phase 1: find the RETURN teleport (back to the original map) and click
        //   NEXT TO it, pushing in, to come back.
        doorTestPhase=2;
        uint8_t sx=0,sy=0;
        CATCHCHALLENGER_TYPE_MAPID dest=65535;
        bool isPush=false;
        int nX=-1,nY=-1;
        bool ok=pickReachableTeleporter(current_map,px,py,doorOrigMap,sx,sy,dest,isPush,nX,nY);
        if(!ok)
            //no direct return found: accept ANY reachable teleporter as the way back
            ok=pickReachableTeleporter(current_map,px,py,65535,sx,sy,dest,isPush,nX,nY);
        if(!ok)
        {
            std::cerr << "[DOORTEST] FAIL no return teleport on map " << (int)current_map << std::endl;
            QCoreApplication::exit(3);
            return;
        }
        //force the PUSH interaction (click next to it) when a neighbour is reachable
        if(nX>=0)
            isPush=true;
        std::cerr << "[DOORTEST] return teleport at (" << (int)sx << "," << (int)sy << ") -> map " << (int)dest
                  << " via " << (isPush?"PUSH (click next to)":"DOOR (click on)") << std::endl;
        doorTestTimeoutTimer.start(20000);
        clickTeleporter(sx,sy,isPush,nX,nY);
    }
    else if(doorTestPhase==2)
    {
        //wait until we actually left the destination map
        if(current_map==doorDestMap)
            return;
        doorTestTimeoutTimer.stop();
        if(current_map==doorOrigMap)
        {
            std::cerr << "[DOORTEST] PASS came back to the original map " << (int)doorOrigMap
                      << " by pushing into the return teleport" << std::endl;
            QCoreApplication::exit(0);
        }
        else
        {
            std::cerr << "[DOORTEST] FAIL return teleport led to map " << (int)current_map
                      << " not the original " << (int)doorOrigMap << std::endl;
            QCoreApplication::exit(3);
        }
    }
}

void MapControllerMP::doorTestTimeout()
{
    std::cerr << "[DOORTEST] FAIL timeout in phase " << doorTestPhase
              << " (current map " << (int)current_map << ", orig " << (int)doorOrigMap
              << ", dest " << (int)doorDestMap << ")" << std::endl;
    QCoreApplication::exit(3);
}

//──────────────────────── --test-keyboard ────────────────────────
//The keyboard run is a tick-driven state machine; these are its phases.
enum KbPhase : int {
    Kb_FindSign=0, Kb_NavSign, Kb_FaceSign, Kb_Enter, Kb_WaitSign, Kb_Escape,
    Kb_FindDoor, Kb_NavDoor, Kb_FindReturn, Kb_NavBack
};

int MapControllerMP::dirToKey(const CatchChallenger::Direction &d) const
{
    switch(d)
    {
        case CatchChallenger::Direction_move_at_left:  return Qt::Key_Left;
        case CatchChallenger::Direction_move_at_right: return Qt::Key_Right;
        case CatchChallenger::Direction_move_at_top:   return Qt::Key_Up;
        case CatchChallenger::Direction_move_at_bottom:return Qt::Key_Down;
        default: return Qt::Key_Up;
    }
}

void MapControllerMP::synthKey(const int &key)
{
    //feed the key straight into the shared keyPressEvent path (arrows move, Enter
    //opens a sign via parseAction(), Escape emits escapePressed()); press+release
    //so a held-key continuous move never starts — one tile per arrow.
    QKeyEvent press(QEvent::KeyPress,key,Qt::NoModifier);
    QKeyEvent release(QEvent::KeyRelease,key,Qt::NoModifier);
    keyPressEvent(&press);
    keyReleaseEvent(&release);
}

bool MapControllerMP::computeKeyboardPath(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const int &fromX,const int &fromY,const int &toX,const int &toY,std::vector<CatchChallenger::Direction> &dirs,std::vector<std::pair<uint8_t,uint8_t> > &pos)
{
    dirs.clear();
    pos.clear();
    const CatchChallenger::CommonMap &lm=QtDatapackClientLoader::datapackLoader->getMap(mapIndex);
    const int mw=lm.width;
    const int mh=lm.height;
    if(mw<=0 || mh<=0 || fromX<0 || fromX>=mw || fromY<0 || fromY>=mh || toX<0 || toX>=mw || toY<0 || toY>=mh)
        return false;
    pos.push_back(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(fromX),static_cast<uint8_t>(fromY)));
    if(fromX==toX && fromY==toY)
        return true;
    const CatchChallenger::Direction mv[4]={CatchChallenger::Direction_move_at_left,
                                            CatchChallenger::Direction_move_at_right,
                                            CatchChallenger::Direction_move_at_top,
                                            CatchChallenger::Direction_move_at_bottom};
    const int dxx[4]={-1,1,0,0};
    const int dyy[4]={0,0,-1,1};
    const bool prevSilent=canGoToSilent;
    canGoToSilent=true;
    const size_t total=static_cast<size_t>(mw)*static_cast<size_t>(mh);
    std::vector<int> parent(total,-1);
    std::vector<CatchChallenger::Direction> parentDir(total,CatchChallenger::Direction_move_at_top);
    std::vector<bool> visited(total,false);
    std::vector<std::pair<int,int> > queue;
    queue.push_back(std::pair<int,int>(fromX,fromY));
    visited[static_cast<size_t>(fromX)+static_cast<size_t>(fromY)*static_cast<size_t>(mw)]=true;
    size_t head=0;
    bool reached=false;
    while(head<queue.size() && !reached)
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
                if(!visited[idx] && canGoTo(mv[k],mapIndex,static_cast<COORD_TYPE>(qx),static_cast<COORD_TYPE>(qy),true))
                {
                    visited[idx]=true;
                    parent[idx]=qx+qy*mw;
                    parentDir[idx]=mv[k];
                    if(tx==toX && ty==toY)
                    {
                        reached=true;
                        break;
                    }
                    queue.push_back(std::pair<int,int>(tx,ty));
                }
            }
            k++;
        }
    }
    canGoToSilent=prevSilent;
    if(!reached)
        return false;
    //walk parents back from the target, collecting directions, then reverse
    std::vector<CatchChallenger::Direction> rev;
    int cur=toX+toY*mw;
    while(cur!=fromX+fromY*mw)
    {
        rev.push_back(parentDir[static_cast<size_t>(cur)]);
        cur=parent[static_cast<size_t>(cur)];
    }
    int i=static_cast<int>(rev.size())-1;
    int cx=fromX,cy=fromY;
    while(i>=0)
    {
        const CatchChallenger::Direction d=rev.at(static_cast<size_t>(i));
        dirs.push_back(d);
        if(d==CatchChallenger::Direction_move_at_left) cx--;
        else if(d==CatchChallenger::Direction_move_at_right) cx++;
        else if(d==CatchChallenger::Direction_move_at_top) cy--;
        else if(d==CatchChallenger::Direction_move_at_bottom) cy++;
        pos.push_back(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(cx),static_cast<uint8_t>(cy)));
        i--;
    }
    return true;
}

bool MapControllerMP::kbStartNavTo(const int &toX,const int &toY)
{
    const std::pair<COORD_TYPE,COORD_TYPE> p(getPos());
    kbNavMap=current_map;
    kbPathStep=0;
    return computeKeyboardPath(current_map,p.first,p.second,toX,toY,kbPath,kbPathPos);
}

bool MapControllerMP::kbNavStep()
{
    if(kbPathStep>=kbPath.size())
        return true;
    const std::pair<COORD_TYPE,COORD_TYPE> p(getPos());
    const std::pair<uint8_t,uint8_t> &expected=kbPathPos.at(kbPathStep+1);
    if(p.first==expected.first && p.second==expected.second)
    {
        kbPathStep++;
        return kbPathStep>=kbPath.size();
    }
    //turn toward / move one tile in the step direction (one synthKey == turn OR
    //move; the next tick takes the other half)
    synthKey(dirToKey(kbPath.at(kbPathStep)));
    return false;
}

bool MapControllerMP::findNearestSign(int &signX,int &signY,int &neighX,int &neighY)
{
    if(CatchChallenger::QMap_client::all_map.find(current_map)==CatchChallenger::QMap_client::all_map.cend())
        return false;
    const QMap_client * const mapFull=CatchChallenger::QMap_client::all_map.at(current_map);
    const CatchChallenger::CommonMap &lm=QtDatapackClientLoader::datapackLoader->getMap(current_map);
    const int mw=lm.width;
    const int mh=lm.height;
    const std::pair<COORD_TYPE,COORD_TYPE> p(getPos());
    const int px=p.first;
    const int py=p.second;
    Q_UNUSED(mapFull);
    bool found=false;
    int bestDist=0;
    //A Sign/NPC sits on a NON-standable tile; facing it and pressing Enter makes
    //parseAction() emit actionOn (the emit is unconditional for any in-map faced
    //tile). Same interactables as the click-on-sign test; pick the NEAREST so the
    //keyboard walk is short.
    int by=0;
    while(by<mh)
    {
        int bx=0;
        while(bx<mw)
        {
            if(!tileStandable(current_map,bx,by,mw,mh))
            {
                int nX=0,nY=0;
                if(reachableClickNeighbor(current_map,bx,by,px,py,nX,nY))
                {
                    const int dist=(nX>px?nX-px:px-nX)+(nY>py?nY-py:py-nY);
                    if(!found || dist<bestDist)
                    {
                        found=true;
                        bestDist=dist;
                        signX=bx;
                        signY=by;
                        neighX=nX;
                        neighY=nY;
                    }
                }
            }
            bx++;
        }
        by++;
    }
    return found;
}

void MapControllerMP::runKeyboardSelfTest()
{
    if(CatchChallenger::QMap_client::all_map.find(current_map)==CatchChallenger::QMap_client::all_map.cend())
    {
        std::cerr << "[KEYBOARDTEST] FAIL current map not loaded" << std::endl;
        QCoreApplication::exit(3);
        return;
    }
    if(!connect(this,&MapVisualiserPlayer::actionOn,this,&MapControllerMP::keyboardSignActionOn,Qt::UniqueConnection))
        abort();
    kbPhase=Kb_FindSign;
    kbTimeoutTimer.start(60000);
    std::cerr << "[KEYBOARDTEST] start on map " << (int)current_map << std::endl;
    kbTick();
}

void MapControllerMP::keyboardSignActionOn(CatchChallenger::Map_client *map, const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y)
{
    Q_UNUSED(map);
    if(mapIndex==current_map && x==kbSignX && y==kbSignY)
        kbSignOpened=true;
}

void MapControllerMP::kbTick()
{
    //wait out any running step animation before issuing the next key
    if(playerIsMoving())
    {
        kbTimer.start(40);
        return;
    }
    const std::pair<COORD_TYPE,COORD_TYPE> p(getPos());
    bool reschedule=true;
    switch(kbPhase)
    {
        case Kb_FindSign:
        {
            int sx=0,sy=0,nX=0,nY=0;
            if(!findNearestSign(sx,sy,nX,nY))
            {
                std::cerr << "[KEYBOARDTEST] FAIL no reachable sign on map " << (int)current_map << std::endl;
                QCoreApplication::exit(3);
                return;
            }
            kbSignX=static_cast<uint8_t>(sx);
            kbSignY=static_cast<uint8_t>(sy);
            std::cerr << "[KEYBOARDTEST] arrow-walking to the sign at (" << sx << "," << sy
                      << "), foot tile (" << nX << "," << nY << ") from (" << (int)p.first << "," << (int)p.second << ")" << std::endl;
            if(!kbStartNavTo(nX,nY))
            {
                std::cerr << "[KEYBOARDTEST] FAIL no keyboard path to the sign" << std::endl;
                QCoreApplication::exit(3);
                return;
            }
            kbPhase=Kb_NavSign;
        }
        break;
        case Kb_NavSign:
            if(kbNavStep())
                kbPhase=Kb_FaceSign;
        break;
        case Kb_FaceSign:
        {
            //turn to FACE the sign (it is blocked, so the arrow only turns)
            const int dx=static_cast<int>(kbSignX)-static_cast<int>(p.first);
            const int dy=static_cast<int>(kbSignY)-static_cast<int>(p.second);
            CatchChallenger::Direction toward=CatchChallenger::Direction_move_at_top;
            if(dx==1 && dy==0) toward=CatchChallenger::Direction_move_at_right;
            else if(dx==-1 && dy==0) toward=CatchChallenger::Direction_move_at_left;
            else if(dx==0 && dy==1) toward=CatchChallenger::Direction_move_at_bottom;
            else if(dx==0 && dy==-1) toward=CatchChallenger::Direction_move_at_top;
            synthKey(dirToKey(toward));
            kbPhase=Kb_Enter;
        }
        break;
        case Kb_Enter:
            kbSignOpened=false;
            synthKey(Qt::Key_Return);//parseAction() -> actionOn on the faced sign
            kbPhase=Kb_WaitSign;
        break;
        case Kb_WaitSign:
            if(kbSignOpened)
            {
                std::cerr << "[KEYBOARDTEST] PASS-sign: walked to the sign with arrows and opened it (" << (int)kbSignX
                          << "," << (int)kbSignY << ") with Enter" << std::endl;
                kbPhase=Kb_Escape;
            }
        break;
        case Kb_Escape:
            synthKey(Qt::Key_Escape);//emit escapePressed() -> client closes the dialog
            std::cerr << "[KEYBOARDTEST] pressed Escape to close the dialog" << std::endl;
            kbPhase=Kb_FindDoor;
        break;
        case Kb_FindDoor:
        {
            uint8_t sx=0,sy=0;
            CATCHCHALLENGER_TYPE_MAPID dest=65535;
            bool isPush=false;
            int nX=-1,nY=-1;
            if(!pickReachableTeleporter(current_map,p.first,p.second,65535,sx,sy,dest,isPush,nX,nY))
            {
                std::cerr << "[KEYBOARDTEST] FAIL no reachable door on map " << (int)current_map << std::endl;
                QCoreApplication::exit(3);
                return;
            }
            kbOrigMap=current_map;
            std::cerr << "[KEYBOARDTEST] arrow-walking into the door at (" << (int)sx << "," << (int)sy
                      << ") -> map " << (int)dest << std::endl;
            if(!kbStartNavTo(sx,sy))
            {
                std::cerr << "[KEYBOARDTEST] FAIL no keyboard path to the door" << std::endl;
                QCoreApplication::exit(3);
                return;
            }
            kbPhase=Kb_NavDoor;
        }
        break;
        case Kb_NavDoor:
            if(current_map!=kbNavMap)
            {
                kbIndoorMap=current_map;
                std::cerr << "[KEYBOARDTEST] PASS-door: walked into the door with arrows, now indoors (map "
                          << (int)kbOrigMap << " -> " << (int)kbIndoorMap << ")" << std::endl;
                kbPhase=Kb_FindReturn;
            }
            else
                kbNavStep();
        break;
        case Kb_FindReturn:
        {
            uint8_t sx=0,sy=0;
            CATCHCHALLENGER_TYPE_MAPID dest=65535;
            bool isPush=false;
            int nX=-1,nY=-1;
            bool ok=pickReachableTeleporter(current_map,p.first,p.second,kbOrigMap,sx,sy,dest,isPush,nX,nY);
            if(!ok)
                ok=pickReachableTeleporter(current_map,p.first,p.second,65535,sx,sy,dest,isPush,nX,nY);
            if(!ok)
            {
                std::cerr << "[KEYBOARDTEST] FAIL no return teleport on map " << (int)current_map << std::endl;
                QCoreApplication::exit(3);
                return;
            }
            std::cerr << "[KEYBOARDTEST] arrow-walking back to the city via teleport (" << (int)sx << "," << (int)sy
                      << ") -> map " << (int)dest << std::endl;
            if(!kbStartNavTo(sx,sy))
            {
                std::cerr << "[KEYBOARDTEST] FAIL no keyboard path to the return teleport" << std::endl;
                QCoreApplication::exit(3);
                return;
            }
            kbPhase=Kb_NavBack;
        }
        break;
        case Kb_NavBack:
            if(current_map!=kbNavMap)
            {
                kbTimeoutTimer.stop();
                if(current_map==kbOrigMap)
                {
                    std::cerr << "[KEYBOARDTEST] PASS came back from indoor to the city (map "
                              << (int)kbIndoorMap << " -> " << (int)kbOrigMap << ") with the arrow keys" << std::endl;
                    QCoreApplication::exit(0);
                }
                else
                    std::cerr << "[KEYBOARDTEST] FAIL return led to map " << (int)current_map
                              << " not the city " << (int)kbOrigMap << std::endl;
                if(current_map!=kbOrigMap)
                    QCoreApplication::exit(3);
                return;
            }
            else
                kbNavStep();
        break;
        default:
        break;
    }
    if(reschedule)
        kbTimer.start(40);
}

void MapControllerMP::keyboardTestTimeout()
{
    std::cerr << "[KEYBOARDTEST] FAIL timeout in phase " << kbPhase
              << " (current map " << (int)current_map << ", sign opened " << (kbSignOpened?"yes":"no") << ")" << std::endl;
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
