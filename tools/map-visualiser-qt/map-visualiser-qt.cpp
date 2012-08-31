/*
 * tmxviewer.cpp
 * Copyright 2010, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 *
 * This file is part of the TMX Viewer example.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "map-visualiser-qt.h"

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>
#include <QTime>
#include <QDebug>

#include "../../general/base/MoveOnTheMap.h"

using namespace Tiled;

/**
 * Item that represents a map object.
 */
class MapObjectItem : public QGraphicsItem
{
public:
    MapObjectItem(MapObject *mapObject, MapRenderer *renderer,
                  QGraphicsItem *parent = 0)
        : QGraphicsItem(parent)
        , mMapObject(mapObject)
        , mRenderer(renderer)
    {}

    QRectF boundingRect() const
    {
        return mRenderer->boundingRect(mMapObject);
    }

    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
    {
        const QColor &color = mMapObject->objectGroup()->color();
        mRenderer->drawMapObject(p, mMapObject,
                                 color.isValid() ? color : Qt::darkGray);
    }

private:
    MapObject *mMapObject;
    MapRenderer *mRenderer;
};

/**
 * Item that represents a tile layer.
 */
class TileLayerItem : public QGraphicsItem
{
public:
    TileLayerItem(TileLayer *tileLayer, MapRenderer *renderer,
                  QGraphicsItem *parent = 0)
        : QGraphicsItem(parent)
        , mTileLayer(tileLayer)
        , mRenderer(renderer)
    {
        setFlag(QGraphicsItem::ItemUsesExtendedStyleOption);
    }

    QRectF boundingRect() const
    {
        return mRenderer->boundingRect(mTileLayer->bounds());
    }

    void paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *)
    {
        mRenderer->drawTileLayer(p, mTileLayer, option->rect);
    }

private:
    TileLayer *mTileLayer;
    MapRenderer *mRenderer;
};

/**
 * Item that represents an object group.
 */
class ObjectGroupItem : public QGraphicsItem
{
public:
    ObjectGroupItem(ObjectGroup *objectGroup, MapRenderer *renderer,
                    QGraphicsItem *parent = 0)
        : QGraphicsItem(parent)
    {
        setFlag(QGraphicsItem::ItemHasNoContents);

        // Create a child item for each object
        foreach (MapObject *object, objectGroup->objects())
            new MapObjectItem(object, renderer, this);
    }

    QRectF boundingRect() const { return QRectF(); }
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) {}
};

/**
 * Item that represents a map.
 */
class MapItem : public QGraphicsItem
{
public:
    MapItem(QGraphicsItem *parent = 0)
        : QGraphicsItem(parent)
    {
        setFlag(QGraphicsItem::ItemHasNoContents);
    }

    void addMap(Map *map, MapRenderer *renderer)
    {
        //align zIndex to Collision Layer
        int index=0;
        while(index<map->layers().size())
        {
            if(map->layers().at(index)->name()=="Collisions")
                break;
            index++;
        }
        if(index==map->layers().size())
            index=-map->layers().size()-1;
        else
            index=-index;

        // Create a child item for each layer
        foreach (Layer *layer, map->layers()) {
            if (TileLayer *tileLayer = layer->asTileLayer()) {
                TileLayerItem *item=new TileLayerItem(tileLayer, renderer, this);
                item->setZValue(index++);
            } else if (ObjectGroup *objectGroup = layer->asObjectGroup()) {
                ObjectGroupItem *item=new ObjectGroupItem(objectGroup, renderer, this);
                item->setZValue(index++);
            }
        }
    }

    QRectF boundingRect() const { return QRectF(); }
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) {}
};

MapVisualiserQt::MapVisualiserQt(QWidget *parent) :
    QGraphicsView(parent),
    mScene(new QGraphicsScene(this)),
    mMap(0),
    mRenderer(0),
    inMove(false)
{
    lookToMove.setInterval(200);
    lookToMove.setSingleShot(true);
    connect(&lookToMove,SIGNAL(timeout()),this,SLOT(transformLookToMove()));

    moveTimer.setInterval(66);
    moveTimer.setSingleShot(true);
    connect(&moveTimer,SIGNAL(timeout()),this,SLOT(moveStepSlot()));
    setWindowTitle(tr("TMX Viewer"));
    //scale(2,2);

    setScene(mScene);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing
                         | QGraphicsView::DontSavePainterState);
    setBackgroundBrush(Qt::black);
    setFrameStyle(QFrame::NoFrame);

    //viewport()->setAttribute(Qt::WA_StaticContents);
    //setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
}



MapVisualiserQt::~MapVisualiserQt()
{
    qDeleteAll(mMap->tilesets());
    delete mMap;
    delete mRenderer;
}

void MapVisualiserQt::viewMap(const QString &fileName)
{
    playerTileset = new Tileset("player",16,24);
    playerTileset->loadFromImage(QImage(":/player_skin.png"),":/player_skin.png");
    playerMapObject = new MapObject();

    delete mRenderer;
    mRenderer = 0;

    mScene->clear();
    centerOn(0, 0);

    QTime startTime;
    startTime.restart();

    //load the map
    MapReader reader;
    mMap = reader.readMap(fileName);
    if (!mMap)
        return; // TODO: Add error handling
    Pokecraft::Map_loader map_loader;
    if(!map_loader.tryLoadMap(fileName))
        return;

    //the direction
    direction=Pokecraft::Direction_look_at_bottom;
    playerMapObject->setTile(playerTileset->tileAt(7));

    //position
    if(!map_loader.map_to_send.rescue_points.empty())
    {
        xPerso=map_loader.map_to_send.rescue_points.first().x;
        yPerso=map_loader.map_to_send.rescue_points.first().y;
    }
    else if(!map_loader.map_to_send.bot_spawn_points.empty())
    {
        xPerso=map_loader.map_to_send.bot_spawn_points.first().x;
        yPerso=map_loader.map_to_send.bot_spawn_points.first().y;
    }
    else
    {
        xPerso=2;
        yPerso=2;
    }
    playerMapObject->setPosition(QPoint(xPerso,yPerso+1));

    logicalMap.width                    = map_loader.map_to_send.width;
    logicalMap.height                   = map_loader.map_to_send.height;
    logicalMap.parsed_layer.walkable	= map_loader.map_to_send.parsed_layer.walkable;
    logicalMap.parsed_layer.water		= map_loader.map_to_send.parsed_layer.water;
    logicalMap.map_file                 = fileName;
    logicalMap.border.bottom.map        = NULL;
    logicalMap.border.top.map           = NULL;
    logicalMap.border.right.map         = NULL;
    logicalMap.border.left.map          = NULL;
    logicalMap.border_semi              = map_loader.map_to_send.border;
    logicalMap.teleport_semi            = map_loader.map_to_send.teleport;

    switch (mMap->orientation()) {
    case Map::Isometric:
        mRenderer = new IsometricRenderer(mMap);
        break;
    case Map::Orthogonal:
    default:
        mRenderer = new OrthogonalRenderer(mMap);
        break;
    }
    mRenderer->setObjectBorder(false);

    MapItem* mapItem=new MapItem();
    int index=0;
    while(index<fileName.size())
    {
        if(mMap->layerAt(index)->name()=="Collisions")
        {
            Tiled::ObjectGroup * objectGroup = new ObjectGroup();
            mMap->insertLayer(index+1,objectGroup);
            objectGroup->addObject(playerMapObject);
            break;
        }
        index++;
    }

    mapItem->addMap(mMap,mRenderer);
    qDebug() << startTime.elapsed();
    mScene->addItem(mapItem);

    connect(&timer,SIGNAL(timeout()),this,SLOT(moveTile()));
    timer.setSingleShot(true);
    timer.setInterval(30);
    timer.start();
}

void MapVisualiserQt::moveTile()
{
    /*QTime startTime;
    startTime.restart();
    int index=0;
    while(index<mapObject.size())
    {
        mapObject.at(index)->setPosition(QPoint(rand()%32,rand()%32));
        index++;
    }
    viewport()->update();
    qDebug() << startTime.elapsed();
    timer.start();*/
}

/*    int index=0;
while(index<fileName.size())
{

    MapReader reader;
    mMap = reader.readMap(fileName.at(index));
    if (!mMap)
        return; // TODO: Add error handling
    //Map *map, MapRenderer *renderer
    // Create a child item for each layer
    switch (mMap->orientation()) {
    case Map::Isometric:
        mRenderer = new IsometricRenderer(mMap);
        break;
    case Map::Orthogonal:
    default:
        mRenderer = new OrthogonalRenderer(mMap);
        break;
    }

int index2=0;
    foreach (Layer *layer, mMap->layers()) {
        if (TileLayer *tileLayer = layer->asTileLayer()) {
            TileLayerItem *item=new TileLayerItem(tileLayer, mRenderer);
    item->setZValue(index2++);
        } else if (ObjectGroup *objectGroup = layer->asObjectGroup()) {
            ObjectGroupItem *item=new ObjectGroupItem(objectGroup, mRenderer);
    item->setZValue(index2++);
        }
    }
    //    mScene->addItem(new MapItem(mMap, mRenderer));
    index++;
}*/















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
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_left,&logicalMap,xPerso,yPerso,true))
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
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_right,&logicalMap,xPerso,yPerso,true))
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
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_top,&logicalMap,xPerso,yPerso,true))
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
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_bottom,&logicalMap,xPerso,yPerso,true))
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
        //set the final value (direction, position, ...)
        switch(direction)
        {
            case Pokecraft::Direction_move_at_left:
            direction=Pokecraft::Direction_look_at_left;
            xPerso-=1;
            break;
            case Pokecraft::Direction_move_at_right:
            direction=Pokecraft::Direction_look_at_right;
            xPerso+=1;
            break;
            case Pokecraft::Direction_move_at_top:
            direction=Pokecraft::Direction_look_at_top;
            yPerso-=1;
            break;
            case Pokecraft::Direction_move_at_bottom:
            direction=Pokecraft::Direction_look_at_bottom;
            yPerso+=1;
            break;
            default:
            qDebug() << QString("moveStepSlot(): moveStep: %1, justUpdateTheTile: %2, wrong direction when moveStep>2").arg(moveStep).arg(justUpdateTheTile);
            return;
        }
        //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
        playerMapObject->setPosition(QPoint(xPerso,yPerso+1));

	//check if one arrow key is pressed to continue to move into this direction
        if(keyPressed.contains(16777234))
        {
            //if can go, then do the move
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_left,&logicalMap,xPerso,yPerso,true))
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
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_right,&logicalMap,xPerso,yPerso,true))
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
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_top,&logicalMap,xPerso,yPerso,true))
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
            if(!Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_bottom,&logicalMap,xPerso,yPerso,true))
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
    }
    else
        moveTimer.start();

    /*qDebug() << QString("xPerso: %1, yPerso: %2").arg(xPerso).arg(yPerso);
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
        if(keyPressed.contains(16777234) && Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_left,&logicalMap,xPerso,yPerso,true))
        {
            direction=Pokecraft::Direction_move_at_left;
            inMove=true;
            moveStep=1;
            moveStepSlot();
        }
        break;
        case Pokecraft::Direction_look_at_right:
        if(keyPressed.contains(16777236) && Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_right,&logicalMap,xPerso,yPerso,true))
        {
            direction=Pokecraft::Direction_move_at_right;
            inMove=true;
            moveStep=1;
            moveStepSlot();
        }
        break;
        case Pokecraft::Direction_look_at_top:
        if(keyPressed.contains(16777235) && Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_top,&logicalMap,xPerso,yPerso,true))
        {
            direction=Pokecraft::Direction_move_at_top;
            inMove=true;
            moveStep=1;
            moveStepSlot();
        }
        break;
        case Pokecraft::Direction_look_at_bottom:
        if(keyPressed.contains(16777237) && Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_bottom,&logicalMap,xPerso,yPerso,true))
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
