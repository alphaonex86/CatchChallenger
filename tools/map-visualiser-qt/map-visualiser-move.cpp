#include "map-visualiser-qt.h"

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>
#include <QTime>
#include <QDebug>
#include <QFileInfo>
#include <QPointer>

#include "../../general/base/MoveOnTheMap.h"

using namespace Tiled;

void MapVisualiserQt::keyPressEvent(QKeyEvent * event)
{
    //ignore the repeated event
    if(event->isAutoRepeat())
        return;

    //add to pressed key list
    keyPressed << event->key();

    //apply the key
    keyPressParse();
}

void MapVisualiserQt::keyPressParse()
{
    //ignore is already in move
    if(inMove)
        return;

    /*int y=0;
    while(y<logicalMap.height)
    {
        QString line;
        int x=0;
        while(x<logicalMap.width)
        {
            if(x==xPerso && y==yPerso)
                line+="P";
            else if(logicalMap.parsed_layer.walkable[x+y*logicalMap.width])
                line+="_";
            else
                line+="X";
            x++;
        }
        qDebug() << line;
        y++;
    }*/

    if(keyPressed.contains(16777234))
    {
        //already turned on this direction, then try move into this direction
        if(direction==Pokecraft::Direction_look_at_left)
        {
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_left,&current_map->logicalMap,xPerso,yPerso,true))
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
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_right,&current_map->logicalMap,xPerso,yPerso,true))
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
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_top,&current_map->logicalMap,xPerso,yPerso,true))
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
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_bottom,&current_map->logicalMap,xPerso,yPerso,true))
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

    //do it here only because it's one player, then max 3 call by second
    viewport()->update();
}

void MapVisualiserQt::moveStepSlot(bool justUpdateTheTile)
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
        qDebug() << QString("moveStepSlot(): moveStep: %1, justUpdateTheTile: %2, wrong direction").arg(moveStep).arg(justUpdateTheTile);
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
        Pokecraft::Map * old_map=&current_map->logicalMap;
        Pokecraft::Map * map=&current_map->logicalMap;
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
            qDebug() << QString("moveStepSlot(): moveStep: %1, justUpdateTheTile: %2, wrong direction when moveStep>2").arg(moveStep).arg(justUpdateTheTile);
            return;
        }
        //if the map have changed
        if(old_map!=map)
        {
            loadOtherMap(map->map_file);
            if(!other_map.contains(map->map_file))
                qDebug() << QString("map changed not located: %1").arg(map->map_file);
            else
            {
                qDebug() << QString("map changed located: %1").arg(map->map_file);
                unloadCurrentMap(current_map->logicalMap.map_file);
                other_map[current_map->logicalMap.map_file]=current_map;
                current_map=other_map[map->map_file];
                loadCurrentMap(current_map->logicalMap.map_file);
                displayMap();
            }
        }
        //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
        playerMapObject->setPosition(QPoint(xPerso,yPerso+1));

        //check if one arrow key is pressed to continue to move into this direction
        if(keyPressed.contains(16777234))
        {
            //if can go, then do the move
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_left,&current_map->logicalMap,xPerso,yPerso,true))
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
                moveStepSlot(true);
            }
        }
        else if(keyPressed.contains(16777236))
        {
            //if can go, then do the move
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_right,&current_map->logicalMap,xPerso,yPerso,true))
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
                moveStepSlot(true);
            }
        }
        else if(keyPressed.contains(16777235))
        {
            //if can go, then do the move
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_top,&current_map->logicalMap,xPerso,yPerso,true))
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
                moveStepSlot(true);
            }
        }
        else if(keyPressed.contains(16777237))
        {
            //if can go, then do the move
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_bottom,&current_map->logicalMap,xPerso,yPerso,true))
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
                moveStepSlot(true);
            }
        }
        //now stop walking, no more arrow key is pressed
        else
        {
            playerMapObject->setPosition(QPoint(xPerso,yPerso+1));
            inMove=false;
        }
        //qDebug() << QString("xPerso: %1, yPerso: %2, map: %3").arg(xPerso).arg(yPerso).arg(current_map->logicalMap.map_file);
    }
    else
        moveTimer.start();

    /*
    qDebug() << QString("xPerso: %1, yPerso: %2, map: %3").arg(xPerso).arg(yPerso).arg(current_map->logicalMap.map_file);
    int y=0;
    while(y<logicalMap.height)
    {
        QString line;
        int x=0;
        while(x<logicalMap.width)
        {
            if(x==xPerso && y==yPerso)
                line+="P";
            else if(logicalMap.parsed_layer.walkable[x+y*logicalMap.width])
                line+="_";
            else
                line+="X";
            x++;
        }
        qDebug() << line;
        y++;
    }*/

    //do it here only because it's one player, then max 3 call by second
    if(!justUpdateTheTile)
        viewport()->update();
}

//have look into another direction, if the key remain pressed, apply like move
void MapVisualiserQt::transformLookToMove()
{
    if(inMove)
        return;

    switch(direction)
    {
        case Pokecraft::Direction_look_at_left:
        if(keyPressed.contains(16777234) && Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_left,&current_map->logicalMap,xPerso,yPerso,true))
        {
            direction=Pokecraft::Direction_move_at_left;
            inMove=true;
            moveStep=1;
            moveStepSlot();
        }
        break;
        case Pokecraft::Direction_look_at_right:
        if(keyPressed.contains(16777236) && Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_right,&current_map->logicalMap,xPerso,yPerso,true))
        {
            direction=Pokecraft::Direction_move_at_right;
            inMove=true;
            moveStep=1;
            moveStepSlot();
        }
        break;
        case Pokecraft::Direction_look_at_top:
        if(keyPressed.contains(16777235) && Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_top,&current_map->logicalMap,xPerso,yPerso,true))
        {
            direction=Pokecraft::Direction_move_at_top;
            inMove=true;
            moveStep=1;
            moveStepSlot();
        }
        break;
        case Pokecraft::Direction_look_at_bottom:
        if(keyPressed.contains(16777237) && Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_bottom,&current_map->logicalMap,xPerso,yPerso,true))
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

void MapVisualiserQt::keyReleaseEvent(QKeyEvent * event)
{
    //ignore the repeated event
    if(event->isAutoRepeat())
        return;

    //remove from the key list pressed
    keyPressed.remove(event->key());

    if(keyPressed.size()>0)//another key pressed, repeat
        keyPressParse();
}
