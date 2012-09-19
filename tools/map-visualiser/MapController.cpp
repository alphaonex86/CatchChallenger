#include "MapController.h"

MapController::MapController(const bool &centerOnPlayer,const bool &debugTags,const bool &useCache,const bool &OpenGL) :
    mapVisualiser(NULL,centerOnPlayer,debugTags,useCache,OpenGL)
{
    mapVisualiser.setWindowIcon(QIcon(":/icon.png"));
    inMove=false;
    xPerso=0;
    yPerso=0;

    lookToMove.setInterval(200);
    lookToMove.setSingleShot(true);
    connect(&lookToMove,SIGNAL(timeout()),this,SLOT(transformLookToMove()));

    moveTimer.setInterval(66);
    moveTimer.setSingleShot(true);
    connect(&moveTimer,SIGNAL(timeout()),this,SLOT(moveStepSlot()));

    connect(&mapVisualiser,SIGNAL(newKeyPressEvent(QKeyEvent*)),this,SLOT(keyPressEvent(QKeyEvent*)));
    connect(&mapVisualiser,SIGNAL(newKeyReleaseEvent(QKeyEvent*)),this,SLOT(keyReleaseEvent(QKeyEvent*)));

    playerTileset = new Tiled::Tileset("player",16,24);
    QString externalFile=QCoreApplication::applicationDirPath()+"/player_skin.png";
    if(QFile::exists(externalFile))
    {
        QImage externalImage(externalFile);
        if(!externalImage.isNull() && externalImage.width()==48 && externalImage.height()==96)
            playerTileset->loadFromImage(externalImage,externalFile);
        else
            playerTileset->loadFromImage(QImage(":/player_skin.png"),":/player_skin.png");
    }
    else
        playerTileset->loadFromImage(QImage(":/player_skin.png"),":/player_skin.png");
    botTileset = new Tiled::Tileset("bot",16,24);
    botTileset->loadFromImage(QImage(":/bot_skin.png"),":/bot_skin.png");
    playerMapObject = new Tiled::MapObject();
    botNumber = 0;

    //the direction
    direction=Pokecraft::Direction_look_at_bottom;
    playerMapObject->setTile(playerTileset->tileAt(7));

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

bool MapController::showFPS()
{
    return mapVisualiser.showFPS();
}

void MapController::setShowFPS(const bool &showFPS)
{
    mapVisualiser.setShowFPS(showFPS);
}

void MapController::setTargetFPS(int targetFPS)
{
    mapVisualiser.setTargetFPS(targetFPS);
}

void MapController::setScale(int scale)
{
    mapVisualiser.scale(scale,scale);
}

void MapController::setBotNumber(quint16 botNumber)
{
    this->botNumber=botNumber;
    botManagement();
}

void MapController::keyPressEvent(QKeyEvent * event)
{
    //ignore the repeated event
    if(event->isAutoRepeat())
        return;

    //add to pressed key list
    keyPressed << event->key();

    //apply the key
    keyPressParse();
}

void MapController::keyPressParse()
{
    //ignore is already in move
    if(inMove)
        return;

    if(keyPressed.contains(16777234))
    {
        //already turned on this direction, then try move into this direction
        if(direction==Pokecraft::Direction_look_at_left)
        {
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_left,mapVisualiser.current_map->logicalMap,xPerso,yPerso,true))
                return;//Can't do at the left!
            //the first step
            direction=Pokecraft::Direction_move_at_left;
            inMove=true;
            moveStep=1;
            moveStepSlot();
        }
        //look in this direction
        else
        {
            playerMapObject->setTile(playerTileset->tileAt(10));
            direction=Pokecraft::Direction_look_at_left;
            lookToMove.start();
        }
    }
    else if(keyPressed.contains(16777236))
    {
        //already turned on this direction, then try move into this direction
        if(direction==Pokecraft::Direction_look_at_right)
        {
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_right,mapVisualiser.current_map->logicalMap,xPerso,yPerso,true))
                return;//Can't do at the right!
            //the first step
            direction=Pokecraft::Direction_move_at_right;
            inMove=true;
            moveStep=1;
            moveStepSlot();
        }
        //look in this direction
        else
        {
            playerMapObject->setTile(playerTileset->tileAt(4));
            direction=Pokecraft::Direction_look_at_right;
            lookToMove.start();
        }
    }
    else if(keyPressed.contains(16777235))
    {
        //already turned on this direction, then try move into this direction
        if(direction==Pokecraft::Direction_look_at_top)
        {
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_top,mapVisualiser.current_map->logicalMap,xPerso,yPerso,true))
                return;//Can't do at the top!
            //the first step
            direction=Pokecraft::Direction_move_at_top;
            inMove=true;
            moveStep=1;
            moveStepSlot();
        }
        //look in this direction
        else
        {
            playerMapObject->setTile(playerTileset->tileAt(1));
            direction=Pokecraft::Direction_look_at_top;
            lookToMove.start();
        }
    }
    else if(keyPressed.contains(16777237))
    {
        //already turned on this direction, then try move into this direction
        if(direction==Pokecraft::Direction_look_at_bottom)
        {
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_bottom,mapVisualiser.current_map->logicalMap,xPerso,yPerso,true))
                return;//Can't do at the bottom!
            //the first step
            direction=Pokecraft::Direction_move_at_bottom;
            inMove=true;
            moveStep=1;
            moveStepSlot();
        }
        //look in this direction
        else
        {
            playerMapObject->setTile(playerTileset->tileAt(7));
            direction=Pokecraft::Direction_look_at_bottom;
            lookToMove.start();
        }
    }
}

void MapController::moveStepSlot()
{
    int baseTile=1;
    //move the player for intermediate step and define the base tile (define the stopped step with direction)
    switch(direction)
    {
        case Pokecraft::Direction_move_at_left:
        baseTile=10;
        switch(moveStep)
        {
            case 1:
            case 2:
            playerMapObject->setX(playerMapObject->x()-0.33);
            break;
        }
        break;
        case Pokecraft::Direction_move_at_right:
        baseTile=4;
        switch(moveStep)
        {
            case 1:
            case 2:
            playerMapObject->setX(playerMapObject->x()+0.33);
            break;
        }
        break;
        case Pokecraft::Direction_move_at_top:
        baseTile=1;
        switch(moveStep)
        {
            case 1:
            case 2:
            playerMapObject->setY(playerMapObject->y()-0.33);
            break;
        }
        break;
        case Pokecraft::Direction_move_at_bottom:
        baseTile=7;
        switch(moveStep)
        {
            case 1:
            case 2:
            playerMapObject->setY(playerMapObject->y()+0.33);
            break;
        }
        break;
        default:
        qDebug() << QString("moveStepSlot(): moveStep: %1, wrong direction").arg(moveStep);
        return;
    }

    //apply the right step of the base step defined previously by the direction
    switch(moveStep)
    {
        //stopped step
        case 0:
        playerMapObject->setTile(playerTileset->tileAt(baseTile+0));
        break;
        //transition step
        case 1:
        playerMapObject->setTile(playerTileset->tileAt(baseTile-1));
        break;
        case 2:
        playerMapObject->setTile(playerTileset->tileAt(baseTile+1));
        break;
        //stopped step
        case 3:
        playerMapObject->setTile(playerTileset->tileAt(baseTile+0));
        break;
    }

    moveStep++;

    //if have finish the step
    if(moveStep>3)
    {
        Pokecraft::Map * old_map=&mapVisualiser.current_map->logicalMap;
        Pokecraft::Map * map=&mapVisualiser.current_map->logicalMap;
        //set the final value (direction, position, ...)
        switch(direction)
        {
            case Pokecraft::Direction_move_at_left:
            direction=Pokecraft::Direction_look_at_left;
            Pokecraft::MoveOnTheMap::move(Pokecraft::Direction_move_at_left,&map,&xPerso,&yPerso);
            break;
            case Pokecraft::Direction_move_at_right:
            direction=Pokecraft::Direction_look_at_right;
            Pokecraft::MoveOnTheMap::move(Pokecraft::Direction_move_at_right,&map,&xPerso,&yPerso);
            break;
            case Pokecraft::Direction_move_at_top:
            direction=Pokecraft::Direction_look_at_top;
            Pokecraft::MoveOnTheMap::move(Pokecraft::Direction_move_at_top,&map,&xPerso,&yPerso);
            break;
            case Pokecraft::Direction_move_at_bottom:
            direction=Pokecraft::Direction_look_at_bottom;
            Pokecraft::MoveOnTheMap::move(Pokecraft::Direction_move_at_bottom,&map,&xPerso,&yPerso);
            break;
            default:
            qDebug() << QString("moveStepSlot(): moveStep: %1, wrong direction when moveStep>2").arg(moveStep);
            return;
        }
        //if the map have changed
        if(old_map!=map)
        {
            mapVisualiser.loadOtherMap(map->map_file);
            if(!mapVisualiser.all_map.contains(map->map_file))
                qDebug() << QString("map changed not located: %1").arg(map->map_file);
            else
            {
                unloadPlayerFromCurrentMap();
                mapVisualiser.all_map[mapVisualiser.current_map->logicalMap.map_file]=mapVisualiser.current_map;
                mapVisualiser.current_map=mapVisualiser.all_map[map->map_file];
                mapVisualiser.loadCurrentMap();
                loadPlayerFromCurrentMap();
            }
        }
        //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
        playerMapObject->setPosition(QPoint(xPerso,yPerso+1));

        //check if one arrow key is pressed to continue to move into this direction
        if(keyPressed.contains(16777234))
        {
            //if can go, then do the move
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_left,mapVisualiser.current_map->logicalMap,xPerso,yPerso,true))
            {
                direction=Pokecraft::Direction_look_at_left;
                playerMapObject->setTile(playerTileset->tileAt(10));
                inMove=false;
            }
            //can go into this direction, then just look into this direction
            else
            {
                direction=Pokecraft::Direction_move_at_left;
                moveStep=0;
                moveStepSlot();
            }
        }
        else if(keyPressed.contains(16777236))
        {
            //if can go, then do the move
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_right,mapVisualiser.current_map->logicalMap,xPerso,yPerso,true))
            {
                direction=Pokecraft::Direction_look_at_right;
                playerMapObject->setTile(playerTileset->tileAt(4));
                inMove=false;
            }
            //can go into this direction, then just look into this direction
            else
            {
                direction=Pokecraft::Direction_move_at_right;
                moveStep=0;
                moveStepSlot();
            }
        }
        else if(keyPressed.contains(16777235))
        {
            //if can go, then do the move
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_top,mapVisualiser.current_map->logicalMap,xPerso,yPerso,true))
            {
                direction=Pokecraft::Direction_look_at_top;
                playerMapObject->setTile(playerTileset->tileAt(1));
                inMove=false;
            }
            //can go into this direction, then just look into this direction
            else
            {
                direction=Pokecraft::Direction_move_at_top;
                moveStep=0;
                moveStepSlot();
            }
        }
        else if(keyPressed.contains(16777237))
        {
            //if can go, then do the move
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_bottom,mapVisualiser.current_map->logicalMap,xPerso,yPerso,true))
            {
                direction=Pokecraft::Direction_look_at_bottom;
                playerMapObject->setTile(playerTileset->tileAt(7));
                inMove=false;
            }
            //can go into this direction, then just look into this direction
            else
            {
                direction=Pokecraft::Direction_move_at_bottom;
                moveStep=0;
                moveStepSlot();
            }
        }
        //now stop walking, no more arrow key is pressed
        else
            inMove=false;
    }
    else
        moveTimer.start();
}

//have look into another direction, if the key remain pressed, apply like move
void MapController::transformLookToMove()
{
    if(inMove)
        return;

    switch(direction)
    {
        case Pokecraft::Direction_look_at_left:
        if(keyPressed.contains(16777234) && Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_left,mapVisualiser.current_map->logicalMap,xPerso,yPerso,true))
        {
            direction=Pokecraft::Direction_move_at_left;
            inMove=true;
            moveStep=1;
            moveStepSlot();
        }
        break;
        case Pokecraft::Direction_look_at_right:
        if(keyPressed.contains(16777236) && Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_right,mapVisualiser.current_map->logicalMap,xPerso,yPerso,true))
        {
            direction=Pokecraft::Direction_move_at_right;
            inMove=true;
            moveStep=1;
            moveStepSlot();
        }
        break;
        case Pokecraft::Direction_look_at_top:
        if(keyPressed.contains(16777235) && Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_top,mapVisualiser.current_map->logicalMap,xPerso,yPerso,true))
        {
            direction=Pokecraft::Direction_move_at_top;
            inMove=true;
            moveStep=1;
            moveStepSlot();
        }
        break;
        case Pokecraft::Direction_look_at_bottom:
        if(keyPressed.contains(16777237) && Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_bottom,mapVisualiser.current_map->logicalMap,xPerso,yPerso,true))
        {
            direction=Pokecraft::Direction_move_at_bottom;
            inMove=true;
            moveStep=1;
            moveStepSlot();
        }
        break;
        default:
        qDebug() << QString("transformLookToMove(): wrong direction");
        return;
    }
}

void MapController::keyReleaseEvent(QKeyEvent * event)
{
    //ignore the repeated event
    if(event->isAutoRepeat())
        return;

    //remove from the key list pressed
    keyPressed.remove(event->key());

    if(keyPressed.size()>0)//another key pressed, repeat
        keyPressParse();
}

bool MapController::viewMap(const QString &fileName)
{
    if(!mapVisualiser.viewMap(fileName))
        return false;

    //position
    if(!mapVisualiser.current_map->logicalMap.rescue_points.empty())
    {
        xPerso=mapVisualiser.current_map->logicalMap.rescue_points.first().x;
        yPerso=mapVisualiser.current_map->logicalMap.rescue_points.first().y;
    }
    else if(!mapVisualiser.current_map->logicalMap.bot_spawn_points.empty())
    {
        xPerso=mapVisualiser.current_map->logicalMap.bot_spawn_points.first().x;
        yPerso=mapVisualiser.current_map->logicalMap.bot_spawn_points.first().y;
    }
    else
    {
        xPerso=mapVisualiser.current_map->logicalMap.width/2;
        yPerso=mapVisualiser.current_map->logicalMap.height/2;
    }

    mapVisualiser.loadCurrentMap();
    loadPlayerFromCurrentMap();

    playerMapObject->setPosition(QPoint(xPerso,yPerso+1));
    mapVisualiser.show();

    return true;
}

//call after enter on new map
void MapController::loadPlayerFromCurrentMap()
{
    Tiled::ObjectGroup *currentGroup=playerMapObject->objectGroup();
    if(currentGroup!=NULL)
    {
        if(ObjectGroupItem::objectGroupLink.contains(currentGroup))
        {
            qDebug() << "MapController::loadPlayerFromCurrentMap(): removeObject" << playerMapObject;
            ObjectGroupItem::objectGroupLink[currentGroup]->removeObject(playerMapObject);
        }
        //currentGroup->removeObject(playerMapObject);
        if(currentGroup!=mapVisualiser.current_map->objectGroup)
            qDebug() << QString("loadPlayerFromCurrentMap(), the playerMapObject group is wrong: %1").arg(currentGroup->name());
    }
    if(ObjectGroupItem::objectGroupLink.contains(mapVisualiser.current_map->objectGroup))
        ObjectGroupItem::objectGroupLink[mapVisualiser.current_map->objectGroup]->addObject(playerMapObject);
    else
        qDebug() << QString("loadPlayerFromCurrentMap(), ObjectGroupItem::objectGroupLink not contains current_map->objectGroup");

    //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
    playerMapObject->setPosition(QPoint(xPerso,yPerso+1));

    //list bot spawn
    botSpawnPointList.clear();
    {
        QSet<QString>::const_iterator i = mapVisualiser.loadedNearMap.constBegin();
        while (i != mapVisualiser.loadedNearMap.constEnd()) {
            MapVisualiser::Map_full * map=mapVisualiser.getMap(*i);
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
    //unload the player sprite
    if(ObjectGroupItem::objectGroupLink.contains(playerMapObject->objectGroup()))
    {
        qDebug() << "MapController::unloadPlayerFromCurrentMap(): removeObject" << playerMapObject;
        ObjectGroupItem::objectGroupLink[playerMapObject->objectGroup()]->removeObject(playerMapObject);
    }
    else
        qDebug() << QString("unloadPlayerFromCurrentMap(), ObjectGroupItem::objectGroupLink not contains playerMapObject->objectGroup()");

    botSpawnPointList.clear();
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
        Pokecraft::Map * old_map=&mapVisualiser.all_map[bot->map]->logicalMap;
        Pokecraft::Map * map=&mapVisualiser.all_map[bot->map]->logicalMap;
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
            if(ObjectGroupItem::objectGroupLink.contains(mapVisualiser.all_map[old_map->map_file]->objectGroup))
                ObjectGroupItem::objectGroupLink[mapVisualiser.all_map[old_map->map_file]->objectGroup]->removeObject(bot->mapObject);
            else
                qDebug() << QString("botMoveStepSlot(), ObjectGroupItem::objectGroupLink not contains bot->mapObject at remove to change the map");
            //add bot
            if(ObjectGroupItem::objectGroupLink.contains(mapVisualiser.all_map[map->map_file]->objectGroup))
                ObjectGroupItem::objectGroupLink[mapVisualiser.all_map[map->map_file]->objectGroup]->addObject(bot->mapObject);
            else
                return false;
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
            if(Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_left,mapVisualiser.all_map[botList.at(index).map]->logicalMap,botList.at(index).x,botList.at(index).y,true))
                directions_allowed << Pokecraft::Direction_move_at_left;
            if(Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_right,mapVisualiser.all_map[botList.at(index).map]->logicalMap,botList.at(index).x,botList.at(index).y,true))
                directions_allowed << Pokecraft::Direction_move_at_right;
            if(Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_top,mapVisualiser.all_map[botList.at(index).map]->logicalMap,botList.at(index).x,botList.at(index).y,true))
                directions_allowed << Pokecraft::Direction_move_at_top;
            if(Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_bottom,mapVisualiser.all_map[botList.at(index).map]->logicalMap,botList.at(index).x,botList.at(index).y,true))
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
        if(!mapVisualiser.loadedNearMap.contains(botList.at(index).map))
        {
            if(mapVisualiser.all_map.contains(botList.at(index).map))
            {
                mapVisualiser.all_map[botList.at(index).map]->objectGroup->removeObject(botList.at(index).mapObject);
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
            if(mapVisualiser.all_map.contains(botList.at(index).map))
            {
                if(ObjectGroupItem::objectGroupLink.contains(mapVisualiser.all_map[botList.at(index).map]->objectGroup))
                    ObjectGroupItem::objectGroupLink[mapVisualiser.all_map[botList.at(index).map]->objectGroup]->removeObject(botList.at(index).mapObject);
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

            if(ObjectGroupItem::objectGroupLink.contains(mapVisualiser.all_map[bot.map]->objectGroup))
                ObjectGroupItem::objectGroupLink[mapVisualiser.all_map[bot.map]->objectGroup]->addObject(bot.mapObject);
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
