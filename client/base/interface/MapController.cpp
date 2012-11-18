#include "MapController.h"
#include "../../general/base/MoveOnTheMap.h"
#include "../../general/base/FacilityLib.h"

#include <QMessageBox>

MapController::MapController(Pokecraft::Api_protocol *client,const bool &centerOnPlayer,const bool &debugTags,const bool &useCache,const bool &OpenGL) :
    MapVisualiserPlayer(centerOnPlayer,debugTags,useCache,OpenGL)
{
    qRegisterMetaType<Pokecraft::Direction>("Pokecraft::Direction");
    qRegisterMetaType<Pokecraft::Chat_type>("Pokecraft::Chat_type");
    qRegisterMetaType<Pokecraft::Player_public_informations>("Pokecraft::Player_public_informations");
    qRegisterMetaType<Pokecraft::Player_private_and_public_informations>("Pokecraft::Player_private_and_public_informations");

    botTileset = new Tiled::Tileset("bot",16,24);
    botTileset->loadFromImage(QImage(":/bot_skin.png"),":/bot_skin.png");
    botNumber = 0;

    this->client=client;
    player_informations_is_set=false;

    //the bot management
    connect(&timerBotMove,SIGNAL(timeout()),this,SLOT(botMove()));
    connect(&timerBotManagement,SIGNAL(timeout()),this,SLOT(botManagement()));
    timerBotMove.start(66);
    timerBotManagement.start(3000);

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
    delete botTileset;
}

void MapController::resetAll()
{
    if(!playerTileset->loadFromImage(QImage(":/images/player_default/trainer.png"),":/images/player_default/trainer.png"))
        qDebug() << "Unable the load the default tileset";

    mHaveTheDatapack=false;
}

void MapController::setScale(int scaleSize)
{
    scale(scaleSize,scaleSize);
}

void MapController::setBotNumber(quint16 botNumber)
{
    this->botNumber=botNumber;
    botManagement();
}

bool MapController::botMoveStepSlot(OtherPlayer *bot)
{
    int baseTile=1;
    //move the player for intermediate step and define the base tile (define the stopped step with direction)
    switch(bot->direction)
    {
        case Pokecraft::Direction_move_at_left:
        baseTile=10;
        switch(bot->moveStep)
        {
            case 1:
            case 2:
            bot->mapObject->setX(bot->mapObject->x()-0.33);
            break;
        }
        break;
        case Pokecraft::Direction_move_at_right:
        baseTile=4;
        switch(bot->moveStep)
        {
            case 1:
            case 2:
            bot->mapObject->setX(bot->mapObject->x()+0.33);
            break;
        }
        break;
        case Pokecraft::Direction_move_at_top:
        baseTile=1;
        switch(bot->moveStep)
        {
            case 1:
            case 2:
            bot->mapObject->setY(bot->mapObject->y()-0.33);
            break;
        }
        break;
        case Pokecraft::Direction_move_at_bottom:
        baseTile=7;
        switch(bot->moveStep)
        {
            case 1:
            case 2:
            bot->mapObject->setY(bot->mapObject->y()+0.33);
            break;
        }
        break;
        default:
        qDebug() << QString("botMoveStepSlot(): moveStep: %1, wrong direction").arg(bot->moveStep);
        return false;
    }

    //apply the right step of the base step defined previously by the direction
    switch(bot->moveStep)
    {
        //stopped step
        case 0:
        bot->mapObject->setTile(botTileset->tileAt(baseTile+0));
        break;
        //transition step
        case 1:
        bot->mapObject->setTile(botTileset->tileAt(baseTile-1));
        break;
        case 2:
        bot->mapObject->setTile(botTileset->tileAt(baseTile+1));
        break;
        //stopped step
        case 3:
        bot->mapObject->setTile(botTileset->tileAt(baseTile+0));
        break;
    }

    bot->moveStep++;

    //if have finish the step
    if(bot->moveStep>3)
    {
        Pokecraft::Map * old_map=&all_map[bot->map]->logicalMap;
        Pokecraft::Map * map=&all_map[bot->map]->logicalMap;
        //set the final value (direction, position, ...)
        switch(bot->direction)
        {
            case Pokecraft::Direction_move_at_left:
            bot->direction=Pokecraft::Direction_look_at_left;
            Pokecraft::MoveOnTheMap::move(Pokecraft::Direction_move_at_left,&map,&bot->x,&bot->y);
            break;
            case Pokecraft::Direction_move_at_right:
            bot->direction=Pokecraft::Direction_look_at_right;
            Pokecraft::MoveOnTheMap::move(Pokecraft::Direction_move_at_right,&map,&bot->x,&bot->y);
            break;
            case Pokecraft::Direction_move_at_top:
            bot->direction=Pokecraft::Direction_look_at_top;
            Pokecraft::MoveOnTheMap::move(Pokecraft::Direction_move_at_top,&map,&bot->x,&bot->y);
            break;
            case Pokecraft::Direction_move_at_bottom:
            bot->direction=Pokecraft::Direction_look_at_bottom;
            Pokecraft::MoveOnTheMap::move(Pokecraft::Direction_move_at_bottom,&map,&bot->x,&bot->y);
            break;
            default:
            qDebug() << QString("botMoveStepSlot(): bot->moveStep: %1, wrong direction when bot->moveStep>2").arg(bot->moveStep);
            return false;
        }
        //if the map have changed
        if(old_map!=map)
        {
            //remove bot
            if(ObjectGroupItem::objectGroupLink.contains(all_map[old_map->map_file]->objectGroup))
                ObjectGroupItem::objectGroupLink[all_map[old_map->map_file]->objectGroup]->removeObject(bot->mapObject);
            else
                qDebug() << QString("botMoveStepSlot(), ObjectGroupItem::objectGroupLink not contains bot->mapObject at remove to change the map");
            //add bot
            if(ObjectGroupItem::objectGroupLink.contains(all_map[map->map_file]->objectGroup))
                ObjectGroupItem::objectGroupLink[all_map[map->map_file]->objectGroup]->addObject(bot->mapObject);
            else
                return false;
            bot->map=map->map_file;
        }
        //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
        bot->mapObject->setPosition(QPoint(bot->x,bot->y+1));

        bot->inMove=false;
    }

    return true;
}

void MapController::botMove()
{
    int index;
    QSet<int> continuedMove;
    //continue the move
    index=0;
    while(index<botList.size())
    {
        if(botList.at(index).inMove)
        {
            if(!botMoveStepSlot(&botList[index]))
            {
                delete botList.at(index).mapObject;
                botList.removeAt(index);
                index--;
            }
            continuedMove << index;
        }
        index++;
    }
    //start move
    index=0;
    while(index<botList.size())
    {
        if(!botList.at(index).inMove && !continuedMove.contains(index))
        {
            QList<Pokecraft::Direction> directions_allowed;
            if(Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_left,all_map[botList.at(index).map]->logicalMap,botList.at(index).x,botList.at(index).y,true))
                directions_allowed << Pokecraft::Direction_move_at_left;
            if(Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_right,all_map[botList.at(index).map]->logicalMap,botList.at(index).x,botList.at(index).y,true))
                directions_allowed << Pokecraft::Direction_move_at_right;
            if(Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_top,all_map[botList.at(index).map]->logicalMap,botList.at(index).x,botList.at(index).y,true))
                directions_allowed << Pokecraft::Direction_move_at_top;
            if(Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_bottom,all_map[botList.at(index).map]->logicalMap,botList.at(index).x,botList.at(index).y,true))
                directions_allowed << Pokecraft::Direction_move_at_bottom;
            if(directions_allowed.size()>0)
            {
                int random = rand()%directions_allowed.size();
                Pokecraft::Direction final_direction=directions_allowed.at(random);

                botList[index].direction=final_direction;
                botList[index].inMove=true;
                botList[index].moveStep=1;
                switch(final_direction)
                {
                    case Pokecraft::Direction_move_at_left:
                        botMoveStepSlot(&botList[index]);
                    break;
                    case Pokecraft::Direction_move_at_right:
                        botMoveStepSlot(&botList[index]);
                    break;
                    case Pokecraft::Direction_move_at_top:
                        botMoveStepSlot(&botList[index]);
                    break;
                    case Pokecraft::Direction_move_at_bottom:
                        botMoveStepSlot(&botList[index]);
                    break;
                    default:
                    qDebug() << QString("transformLookToMove(): wrong direction");
                    return;
                }
            }
        }
        index++;
    }
}

void MapController::botManagement()
{
/*    int index;
    //remove bot where the map is not displayed
    index=0;
    while(index<botList.size())
    {
        if(!loadedNearMap.contains(botList.at(index).map))
        {
            if(all_map.contains(botList.at(index).map))
            {
                ObjectGroupItem::objectGroupLink[all_map[botList.at(index).map]->objectGroup]->removeObject(botList.at(index).mapObject);
                delete botList.at(index).mapObject;
            }
            else
            {
                //the map have been removed then the map object too
            }
            botList.removeAt(index);
        }
        else
            index++;
    }
    //remove random bot
    index=0;
    while(index<botList.size())
    {
        if(index>=botNumber  rand()%100<20)
        {
            if(all_map.contains(botList.at(index).map))
            {
                if(ObjectGroupItem::objectGroupLink.contains(all_map[botList.at(index).map]->objectGroup))
                    ObjectGroupItem::objectGroupLink[all_map[botList.at(index).map]->objectGroup]->removeObject(botList.at(index).mapObject);
                else
                    qDebug() << QString("botManagement(), ObjectGroupItem::objectGroupLink not contains botList.at(index).mapObject at remove random bot");
                delete botList.at(index).mapObject;
            }
            else
            {
                //the map have been removed then the map object too
            }
            botList.removeAt(index);
        }
        else
            index++;
    }
    //add bot
    if(!botSpawnPointList.isEmpty())
    {
        index=botList.size();
        while(index<botNumber)//do botNumber bot
        {
            BotSpawnPoint point=botSpawnPointList[rand()%botSpawnPointList.size()];

            OtherPlayer bot;
            bot.map=point.map->logicalMap.map_file;
            bot.mapObject=new Tiled::MapObject();
            bot.x=point.x;
            bot.y=point.y;
            bot.direction=Pokecraft::Direction_look_at_bottom;
            bot.inMove=false;
            bot.moveStep=0;

            if(ObjectGroupItem::objectGroupLink.contains(all_map[bot.map]->objectGroup))
                ObjectGroupItem::objectGroupLink[all_map[bot.map]->objectGroup]->addObject(bot.mapObject);
            else
                qDebug() << QString("botManagement(), ObjectGroupItem::objectGroupLink not contains bot.map->objectGroup");

            //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
            bot.mapObject->setPosition(QPoint(bot.x,bot.y+1));

            bot.mapObject->setTile(botTileset->tileAt(7));
            botList << bot;
            index++;
        }
    }
    else
    {
        qDebug() << "Bot spawn list is empty";
    }*/
}

//call after enter on new map
void MapController::loadPlayerFromCurrentMap()
{
    MapVisualiserPlayer::loadPlayerFromCurrentMap();

    //list bot spawn
    botSpawnPointList.clear();
    {
        QSet<QString>::const_iterator i = loadedNearMap.constBegin();
        while (i != loadedNearMap.constEnd()) {
            MapVisualiser::Map_full * map=getMap(*i);
            if(map!=NULL)
            {
                int index=0;
                while(index<map->logicalMap.bot_spawn_points.size())
                {
                    BotSpawnPoint point;
                    point.map=map;
                    point.x=map->logicalMap.bot_spawn_points.at(index).x;
                    point.y=map->logicalMap.bot_spawn_points.at(index).y;
                    botSpawnPointList << point;
                    index++;
                }
            }
            ++i;
        }
    }

    botManagement();
}

//call before leave the old map (and before loadPlayerFromCurrentMap())
void MapController::unloadPlayerFromCurrentMap()
{
    MapVisualiserPlayer::unloadPlayerFromCurrentMap();

    botSpawnPointList.clear();
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

        loadMap(datapackPath+DATAPACK_BASE_PATH_MAP+datapackLoader.maps[mapId],x,y);
    }
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
    index=0;
    while(index<delayedMove.size())
    {
        move_player(delayedMove.at(index).id,delayedMove.at(index).movement);
        index++;
    }
    index=0;
    while(index<delayedRemove.size())
    {
        remove_player(delayedRemove.at(index));
        index++;
    }
}
