#include "MapController.h"
#include "../../general/base/MoveOnTheMap.h"
#include "../../general/base/FacilityLib.h"
#include "DatapackClientLoader.h"

#include <QMessageBox>

MapController::MapController(Pokecraft::Api_protocol *client,const bool &centerOnPlayer,const bool &debugTags,const bool &useCache,const bool &OpenGL) :
    MapVisualiserPlayer(centerOnPlayer,debugTags,useCache,OpenGL)
{
    qRegisterMetaType<Pokecraft::Direction>("Pokecraft::Direction");
    qRegisterMetaType<Pokecraft::Chat_type>("Pokecraft::Chat_type");
    qRegisterMetaType<Pokecraft::Player_public_informations>("Pokecraft::Player_public_informations");
    qRegisterMetaType<Pokecraft::Player_private_and_public_informations>("Pokecraft::Player_private_and_public_informations");

    this->client=client;
    player_informations_is_set=false;

    playerMapObject = new Tiled::MapObject();
    playerTileset = new Tiled::Tileset("player",16,24);

    resetAll();

    //connect the map controler
    connect(client,SIGNAL(have_current_player_info(Pokecraft::Player_private_and_public_informations)),this,SLOT(have_current_player_info(Pokecraft::Player_private_and_public_informations)),Qt::QueuedConnection);
    connect(client,SIGNAL(insert_player(Pokecraft::Player_public_informations,quint32,quint16,quint16,Pokecraft::Direction)),this,SLOT(insert_player(Pokecraft::Player_public_informations,quint32,quint16,quint16,Pokecraft::Direction)),Qt::QueuedConnection);
    connect(this,SIGNAL(send_player_direction(Pokecraft::Direction)),client,SLOT(send_player_direction(Pokecraft::Direction)),Qt::QueuedConnection);
}

MapController::~MapController()
{
    delete playerTileset;
}

void MapController::resetAll()
{
    if(!playerTileset->loadFromImage(QImage(":/images/player_default/trainer.png"),":/images/player_default/trainer.png"))
        qDebug() << "Unable the load the default tileset";

    delayedInsert.clear();
    delayedMove.clear();
    delayedRemove.clear();
    skinFolderList.clear();

    int index=0;
    while(index<otherPlayerList.size())
    {
        unloadOtherPlayerFromMap(otherPlayerList.at(index));
        delete otherPlayerList.at(index).playerTileset;
        index++;
    }
    otherPlayerList.clear();

    mHaveTheDatapack=false;

    MapVisualiserPlayer::resetAll();
}

void MapController::setScale(int scaleSize)
{
    scale(scaleSize,scaleSize);
}

bool MapController::loadMap(const QString &fileName,const quint8 &x,const quint8 &y)
{
    //position
    this->x=x;
    this->y=y;

    current_map=NULL;

    QString current_map_fileName=loadOtherMap(fileName);
    if(current_map_fileName.isEmpty())
    {
        QMessageBox::critical(NULL,"Error",mLastError);
        return false;
    }
    current_map=all_map[current_map_fileName];

    render();

    loadCurrentMap();
    loadPlayerFromCurrentMap();

    show();

    return true;
}

//map move
void MapController::insert_player(const Pokecraft::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const Pokecraft::Direction &direction)
{
    if(!mHaveTheDatapack || !player_informations_is_set)
    {
        DelayedInsert tempItem;
        tempItem.player=player;
        tempItem.mapId=mapId;
        tempItem.x=x;
        tempItem.y=y;
        tempItem.direction=direction;
        delayedInsert << tempItem;
        return;
    }
    if(player.simplifiedId==player_informations.public_informations.simplifiedId)
    {
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
            qDebug() << "The skin id: "+QString::number(player.skinId)+", into a list of: "+QString::number(skinFolderList.size())+" item(s) info MapController::insert_player()";

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

        loadMap(datapackPath+DATAPACK_BASE_PATH_MAP+DatapackClientLoader::datapackLoader.maps[mapId],x,y);
    }
    else
    {
        OtherPlayer tempPlayer;
        tempPlayer.x=x;
        tempPlayer.y=y;
        tempPlayer.direction=direction;
        tempPlayer.moveStep=0;
        tempPlayer.inMove=false;
        tempPlayer.stepAlternance=false;

        QString current_map_fileName=loadOtherMap(datapackPath+DATAPACK_BASE_PATH_MAP+DatapackClientLoader::datapackLoader.maps[mapId]);
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
            qDebug() << "The skin id: "+QString::number(player.skinId)+", into a list of: "+QString::number(skinFolderList.size())+" item(s) info MapController::insert_player()";
            return;
        }
        tempPlayer.current_map=all_map[current_map_fileName];
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

        loadCurrentMap();
        loadOtherPlayerFromMap(tempPlayer);

        otherPlayerList << tempPlayer;
        return;
    }
}

//call after enter on new map
void MapController::loadOtherPlayerFromMap(OtherPlayer otherPlayer)
{
    Tiled::ObjectGroup *currentGroup=otherPlayer.playerMapObject->objectGroup();
    if(currentGroup!=NULL)
    {
        if(ObjectGroupItem::objectGroupLink.contains(currentGroup))
            ObjectGroupItem::objectGroupLink[currentGroup]->removeObject(otherPlayer.playerMapObject);
        if(currentGroup!=otherPlayer.current_map->objectGroup)
            qDebug() << QString("loadOtherPlayerFromMap(), the playerMapObject group is wrong: %1").arg(currentGroup->name());
    }
    if(ObjectGroupItem::objectGroupLink.contains(otherPlayer.current_map->objectGroup))
        ObjectGroupItem::objectGroupLink[otherPlayer.current_map->objectGroup]->addObject(otherPlayer.playerMapObject);
    else
        qDebug() << QString("loadOtherPlayerFromMap(), ObjectGroupItem::objectGroupLink not contains current_map->objectGroup");

    //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
    otherPlayer.playerMapObject->setPosition(QPoint(otherPlayer.x,otherPlayer.y+1));
}

//call before leave the old map (and before loadPlayerFromCurrentMap())
void MapController::unloadOtherPlayerFromMap(OtherPlayer otherPlayer)
{
    //unload the player sprite
    if(ObjectGroupItem::objectGroupLink.contains(otherPlayer.playerMapObject->objectGroup()))
        ObjectGroupItem::objectGroupLink[otherPlayer.playerMapObject->objectGroup()]->removeObject(otherPlayer.playerMapObject);
    else
        qDebug() << QString("unloadOtherPlayerFromMap(), ObjectGroupItem::objectGroupLink not contains playerMapObject->objectGroup()");
}

void MapController::move_player(const quint16 &id, const QList<QPair<quint8, Pokecraft::Direction> > &movement)
{
    if(!mHaveTheDatapack || !player_informations_is_set)
    {
        DelayedMove tempItem;
        tempItem.id=id;
        tempItem.movement=movement;
        delayedMove << tempItem;
        return;
    }
}

void MapController::remove_player(const quint16 &id)
{
    if(!mHaveTheDatapack || !player_informations_is_set)
    {
        delayedRemove << id;
        return;
    }
}

void MapController::reinsert_player(const quint16 &,const quint8 &,const quint8 &,const Pokecraft::Direction &)
{
}

void MapController::reinsert_player(const quint16 &, const quint32 &, const quint8 &, const quint8 &, const Pokecraft::Direction &)
{
}

void MapController::dropAllPlayerOnTheMap()
{
}

//player info
void MapController::have_current_player_info(const Pokecraft::Player_private_and_public_informations &informations)
{
    if(player_informations_is_set)
    {
        qDebug() << "player information already set";
        return;
    }
    this->player_informations=informations;
    player_informations_is_set=true;
}

//the datapack
void MapController::setDatapackPath(const QString &path)
{
    if(path.endsWith("/") || path.endsWith("\\"))
        datapackPath=path;
    else
        datapackPath=path+"/";
    mLastLocation.clear();
}

void MapController::datapackParsed()
{
    if(mHaveTheDatapack)
        return;
    mHaveTheDatapack=true;

    int index;
    skinFolderList=Pokecraft::FacilityLib::skinIdList(datapackPath+DATAPACK_BASE_PATH_SKIN);

    setAnimationTilset(datapackPath+DATAPACK_BASE_PATH_ANIMATION+"animation.png");

    index=0;
    while(index<delayedInsert.size())
    {
        insert_player(delayedInsert.at(index).player,delayedInsert.at(index).mapId,delayedInsert.at(index).x,delayedInsert.at(index).y,delayedInsert.at(index).direction);
        index++;
    }
    delayedInsert.clear();

    index=0;
    while(index<delayedMove.size())
    {
        move_player(delayedMove.at(index).id,delayedMove.at(index).movement);
        index++;
    }
    delayedMove.clear();

    index=0;
    while(index<delayedRemove.size())
    {
        remove_player(delayedRemove.at(index));
        index++;
    }
    delayedRemove.clear();
}
