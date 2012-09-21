#include "MapController.h"
#include "../../general/base/MoveOnTheMap.h"

MapController::MapController(QWidget *parent,const bool &centerOnPlayer,const bool &debugTags,const bool &useCache,const bool &OpenGL) :
    MapVisualiserPlayer(parent,centerOnPlayer,debugTags,useCache,OpenGL)
{
    setWindowIcon(QIcon(":/icon.png"));

    botTileset = new Tiled::Tileset("bot",16,24);
    botTileset->loadFromImage(QImage(":/bot_skin.png"),":/bot_skin.png");
    botNumber = 0;

    //the bot management
    connect(&timerBotMove,SIGNAL(timeout()),this,SLOT(botMove()));
    connect(&timerBotManagement,SIGNAL(timeout()),this,SLOT(botManagement()));
    timerBotMove.start(66);
    timerBotManagement.start(3000);
}

MapController::~MapController()
{
    delete playerTileset;
    delete botTileset;
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

bool MapController::botMoveStepSlot(Bot *bot)
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
    int index;
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
        if(index>=botNumber /* if to much bot */ || rand()%100<20)
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

            Bot bot;
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
    }
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
