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
#include <QFileInfo>

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
MapItem::MapItem(QGraphicsItem *parent)
    : QGraphicsItem(parent)
{
    setFlag(QGraphicsItem::ItemHasNoContents);
}

void MapItem::addMap(Map *map, MapRenderer *renderer)
{
    //align zIndex to Collision Layer
    int index=0;
    QList<Layer *> layers=map->layers();
    while(index<layers.size())
    {
        if(layers.at(index)->name()=="Collisions")
            break;
        index++;
    }
    if(index==map->layers().size())
        index=-map->layers().size()-1;
    else
        index=-index;

    // Create a child item for each layer
    foreach (Layer *layer, layers) {
        if (TileLayer *tileLayer = layer->asTileLayer()) {
            TileLayerItem *item=new TileLayerItem(tileLayer, renderer, this);
            item->setZValue(index++);
            displayed_layer.insert(map,item);
        } else if (ObjectGroup *objectGroup = layer->asObjectGroup()) {
            ObjectGroupItem *item=new ObjectGroupItem(objectGroup, renderer, this);
            item->setZValue(index++);
            displayed_layer.insert(map,item);
        }
    }
}

void MapItem::removeMap(Map *map)
{
    QList<QGraphicsItem *> values = displayed_layer.values(map);
    int index=0;
    while(index<values.size())
    {
        delete values.at(index);
        index++;
    }
    displayed_layer.remove(map);
}

void MapItem::setMapPosition(Tiled::Map *map,qint16 x,qint16 y)
{
    QList<QGraphicsItem *> values = displayed_layer.values(map);
    int index=0;
    while(index<values.size())
    {
        values.at(index)->setPos(x*16,y*16);
        index++;
    }
}

QRectF MapItem::boundingRect() const
{
    return QRectF();
}

void MapItem::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *)
{
}

MapVisualiserQt::MapVisualiserQt(QWidget *parent) :
    QGraphicsView(parent),
    mScene(new QGraphicsScene(this)),
    inMove(false)
{
    lookToMove.setInterval(200);
    lookToMove.setSingleShot(true);
    connect(&lookToMove,SIGNAL(timeout()),this,SLOT(transformLookToMove()));

    moveTimer.setInterval(66);
    moveTimer.setSingleShot(true);
    connect(&moveTimer,SIGNAL(timeout()),this,SLOT(moveStepSlot()));
    setWindowTitle(tr("map-visualiser-qt"));
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
/*    qDeleteAll(tiledMap->tilesets());
    delete tiledMap;
    delete tiledRender;*/
}

QString MapVisualiserQt::loadOtherMap(const QString &fileName)
{
    Map_full *tempMapObject=new Map_full();
    QFileInfo fileInformations(fileName);
    QString resolvedFileName=fileInformations.absoluteFilePath();
    if(other_map.contains(resolvedFileName))
        return resolvedFileName;

    //load the map
    tempMapObject->tiledMap = reader.readMap(resolvedFileName);
    if (!tempMapObject->tiledMap)
    {
        qDebug() << QString("Unable to load the map: %1, error: %2").arg(resolvedFileName).arg(reader.errorString());
        return QString();
    }
    Pokecraft::Map_loader map_loader;
    if(!map_loader.tryLoadMap(resolvedFileName))
    {
        qDebug() << QString("Unable to load the map: %1, error: %2").arg(resolvedFileName).arg(map_loader.errorString());
        delete tempMapObject->tiledMap;
        return QString();
    }

    //copy the variables
    tempMapObject->logicalMap.width                                 = map_loader.map_to_send.width;
    tempMapObject->logicalMap.height                                = map_loader.map_to_send.height;
    tempMapObject->logicalMap.parsed_layer.walkable                 = map_loader.map_to_send.parsed_layer.walkable;
    tempMapObject->logicalMap.parsed_layer.water                    = map_loader.map_to_send.parsed_layer.water;
    tempMapObject->logicalMap.map_file                              = resolvedFileName;
    tempMapObject->logicalMap.border.bottom.map                     = NULL;
    tempMapObject->logicalMap.border.top.map                        = NULL;
    tempMapObject->logicalMap.border.right.map                      = NULL;
    tempMapObject->logicalMap.border.left.map                       = NULL;

    //load the string
    tempMapObject->logicalMap.border_semi                = map_loader.map_to_send.border;
    if(!map_loader.map_to_send.border.bottom.fileName.isEmpty())
        tempMapObject->logicalMap.border_semi.bottom.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+tempMapObject->logicalMap.border_semi.bottom.fileName).absoluteFilePath();
    if(!map_loader.map_to_send.border.top.fileName.isEmpty())
        tempMapObject->logicalMap.border_semi.top.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+tempMapObject->logicalMap.border_semi.top.fileName).absoluteFilePath();
    if(!map_loader.map_to_send.border.right.fileName.isEmpty())
        tempMapObject->logicalMap.border_semi.right.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+tempMapObject->logicalMap.border_semi.right.fileName).absoluteFilePath();
    if(!map_loader.map_to_send.border.left.fileName.isEmpty())
        tempMapObject->logicalMap.border_semi.left.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+tempMapObject->logicalMap.border_semi.left.fileName).absoluteFilePath();

    //load the string
    int index=0;
    while(index<map_loader.map_to_send.teleport.size())
    {
        tempMapObject->logicalMap.teleport_semi << map_loader.map_to_send.teleport.at(index);
        tempMapObject->logicalMap.teleport_semi[index].map                      = QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+tempMapObject->logicalMap.teleport_semi.at(index).map).absoluteFilePath();
        //qDebug() << QString("moveStepSlot(): resolvedFileName: '%1'").arg(tempMapObject->logicalMap.teleport_semi.at(index).map);
        index++;
    }

    tempMapObject->logicalMap.rescue_points            = map_loader.map_to_send.rescue_points;
    tempMapObject->logicalMap.bot_spawn_points         = map_loader.map_to_send.bot_spawn_points;

    //load the render
    switch (tempMapObject->tiledMap->orientation()) {
    case Map::Isometric:
        tempMapObject->tiledRender = new IsometricRenderer(tempMapObject->tiledMap);
        break;
    case Map::Orthogonal:
    default:
        tempMapObject->tiledRender = new OrthogonalRenderer(tempMapObject->tiledMap);
        break;
    }
    tempMapObject->tiledRender->setObjectBorder(false);

    //do the object group to move the player on it
    tempMapObject->objectGroup = new ObjectGroup();
    index=0;
    while(index<tempMapObject->tiledMap->layerCount())
    {
        if(tempMapObject->tiledMap->layerAt(index)->name()=="Collisions")
        {
            tempMapObject->tiledMap->insertLayer(index+1,tempMapObject->objectGroup);
            break;
        }
        index++;
    }

    other_map[resolvedFileName]=tempMapObject;

    return resolvedFileName;
}

void MapVisualiserQt::loadCurrentMap(const QString &fileName)
{
    QStringList mapUsed;
    qDebug() << QString("loadCurrentMap(%1)").arg(fileName);
    Map_full *tempMapObject;
    if(!other_map.contains(fileName))
    {
        if(current_map->logicalMap.map_file!=fileName)
        {
            qDebug() << QString("loadCurrentMap(): the current map is unable to load: %1").arg(fileName);
            return;
        }
        else
            tempMapObject=current_map;
    }
    else
        tempMapObject=other_map[fileName];

    mapUsed<<tempMapObject->logicalMap.map_file;

    QString mapIndex;

    //if have border
    if(!tempMapObject->logicalMap.border_semi.bottom.fileName.isEmpty())
    {
        mapIndex=loadOtherMap(tempMapObject->logicalMap.border_semi.bottom.fileName);
        //if is correctly loaded
        if(!mapIndex.isEmpty())
        {
            //if both border match
            if(fileName==other_map[mapIndex]->logicalMap.border_semi.top.fileName && current_map->logicalMap.border_semi.bottom.fileName==mapIndex)
            {
                current_map->logicalMap.border.bottom.map=&other_map[mapIndex]->logicalMap;
                int offset=current_map->logicalMap.border.bottom.x_offset-other_map[mapIndex]->logicalMap.border.top.x_offset;
                current_map->logicalMap.border.bottom.x_offset=offset;
                other_map[mapIndex]->logicalMap.border.top.x_offset=-offset;
                mapUsed<<mapIndex;
            }
        }
    }

    //if have border
    if(!tempMapObject->logicalMap.border_semi.top.fileName.isEmpty())
    {
        mapIndex=loadOtherMap(tempMapObject->logicalMap.border_semi.top.fileName);
        //if is correctly loaded
        if(!mapIndex.isEmpty())
        {
            //if both border match
            if(fileName==other_map[mapIndex]->logicalMap.border_semi.bottom.fileName && current_map->logicalMap.border_semi.top.fileName==mapIndex)
            {
                current_map->logicalMap.border.top.map=&other_map[mapIndex]->logicalMap;
                int offset=current_map->logicalMap.border.top.x_offset-other_map[mapIndex]->logicalMap.border.bottom.x_offset;
                current_map->logicalMap.border.top.x_offset=offset;
                other_map[mapIndex]->logicalMap.border.bottom.x_offset=-offset;
                mapUsed<<mapIndex;
            }
        }
    }

    //if have border
    if(!tempMapObject->logicalMap.border_semi.left.fileName.isEmpty())
    {
        mapIndex=loadOtherMap(tempMapObject->logicalMap.border_semi.left.fileName);
        //if is correctly loaded
        if(!mapIndex.isEmpty())
        {
            //if both border match
            if(fileName==other_map[mapIndex]->logicalMap.border_semi.right.fileName && current_map->logicalMap.border_semi.left.fileName==mapIndex)
            {
                current_map->logicalMap.border.left.map=&other_map[mapIndex]->logicalMap;
                int offset=current_map->logicalMap.border.left.y_offset-other_map[mapIndex]->logicalMap.border.right.y_offset;
                current_map->logicalMap.border.left.y_offset=offset;
                other_map[mapIndex]->logicalMap.border.right.y_offset=-offset;
                mapUsed<<mapIndex;
            }
        }
    }

    //if have border
    if(!tempMapObject->logicalMap.border_semi.right.fileName.isEmpty())
    {
        mapIndex=loadOtherMap(tempMapObject->logicalMap.border_semi.right.fileName);
        //if is correctly loaded
        if(!mapIndex.isEmpty())
        {
            //if both border match
            if(fileName==other_map[mapIndex]->logicalMap.border_semi.left.fileName && current_map->logicalMap.border_semi.right.fileName==mapIndex)
            {
                current_map->logicalMap.border.right.map=&other_map[mapIndex]->logicalMap;
                int offset=current_map->logicalMap.border.right.y_offset-other_map[mapIndex]->logicalMap.border.left.y_offset;
                current_map->logicalMap.border.right.y_offset=offset;
                other_map[mapIndex]->logicalMap.border.left.y_offset=-offset;
                mapUsed<<mapIndex;
            }
        }
    }

    int index=0;
    while(index<tempMapObject->logicalMap.teleport_semi.size())
    {
        mapIndex=loadOtherMap(tempMapObject->logicalMap.teleport_semi[index].map);
        //if is correctly loaded
        if(!mapIndex.isEmpty())
        {
            //if both border match
            if(tempMapObject->logicalMap.teleport_semi[index].destination_x<other_map[mapIndex]->logicalMap.width && tempMapObject->logicalMap.teleport_semi[index].destination_y<other_map[mapIndex]->logicalMap.height)
            {
                int virtual_position=tempMapObject->logicalMap.teleport_semi[index].source_x+tempMapObject->logicalMap.teleport_semi[index].source_y*tempMapObject->logicalMap.width;
                tempMapObject->logicalMap.teleporter[virtual_position].map=&other_map[mapIndex]->logicalMap;
                tempMapObject->logicalMap.teleporter[virtual_position].x=tempMapObject->logicalMap.teleport_semi[index].destination_x;
                tempMapObject->logicalMap.teleporter[virtual_position].y=tempMapObject->logicalMap.teleport_semi[index].destination_y;
                mapUsed<<mapIndex;
            }
        }
        index++;
    }

    //load the player sprite
    qDebug() << QString("loadCurrentMap(), add player on: %1").arg(tempMapObject->logicalMap.map_file);
    tempMapObject->objectGroup->addObject(playerMapObject);

    //remove the not used map
    QHash<QString,Map_full *>::const_iterator i = other_map.constBegin();
    while (i != other_map.constEnd()) {
        if(!mapUsed.contains((*i)->logicalMap.map_file))
        {
            //if it's the last reference
            if(!displayed_map.contains(*i))
            {
                delete (*i)->logicalMap.parsed_layer.walkable;
                delete (*i)->logicalMap.parsed_layer.water;
                delete (*i)->tiledMap;
                delete (*i)->tiledRender;
                delete (*i);
            }
            other_map.remove((*i)->logicalMap.map_file);
            i = other_map.constBegin();//needed
        }
        else
            ++i;
    }
}

void MapVisualiserQt::unloadCurrentMap(const QString &fileName)
{
    qDebug() << QString("unloadCurrentMap(%1)").arg(fileName);
    Map_full *tempMapObject;
    if(!other_map.contains(fileName))
    {
        if(current_map->logicalMap.map_file!=fileName)
        {
            qDebug() << QString("unloadCurrentMap(): the current map is unable to load: %1").arg(fileName);
            return;
        }
        else
            tempMapObject=current_map;
    }
    else
        tempMapObject=other_map[fileName];

    tempMapObject->logicalMap.border.bottom.map=NULL;
    tempMapObject->logicalMap.border.top.map=NULL;
    tempMapObject->logicalMap.border.left.map=NULL;
    tempMapObject->logicalMap.border.right.map=NULL;
    tempMapObject->logicalMap.teleporter.clear();

    //load the player sprite
    int index=tempMapObject->objectGroup->removeObject(playerMapObject);
    qDebug() << QString("unloadCurrentMap(): after remove the player: %1").arg(index);
}

void MapVisualiserQt::viewMap(const QString &fileName)
{
    QTime startTime;
    startTime.restart();

    mapItem=new MapItem();

    playerTileset = new Tileset("player",16,24);
    playerTileset->loadFromImage(QImage(":/player_skin.png"),":/player_skin.png");
    playerMapObject = new MapObject();

    mScene->clear();
    centerOn(0, 0);

    QString current_map_fileName=loadOtherMap(fileName);
    if(current_map_fileName.isEmpty())
        return;
    current_map=other_map[current_map_fileName];
    other_map.remove(current_map_fileName);
    loadCurrentMap(current_map->logicalMap.map_file);

    //the direction
    direction=Pokecraft::Direction_look_at_bottom;
    playerMapObject->setTile(playerTileset->tileAt(7));

    //position
    if(!current_map->logicalMap.rescue_points.empty())
    {
        xPerso=current_map->logicalMap.rescue_points.first().x;
        yPerso=current_map->logicalMap.rescue_points.first().y;
    }
    else if(!current_map->logicalMap.bot_spawn_points.empty())
    {
        xPerso=current_map->logicalMap.bot_spawn_points.first().x;
        yPerso=current_map->logicalMap.bot_spawn_points.first().y;
    }
    else
    {
        xPerso=current_map->logicalMap.width/2;
        yPerso=current_map->logicalMap.height/2;
    }
    playerMapObject->setPosition(QPoint(xPerso,yPerso+1));

    displayMap();
    qDebug() << startTime.elapsed();
    mScene->addItem(mapItem);
}

void MapVisualiserQt::displayMap()
{
    qDebug() << QString("displayMap(): %1").arg(current_map->logicalMap.map_file);

    QSet<Map_full *> temp_displayed_map;
    //the main map
    if(!displayed_map.contains(current_map))
    {
        mapItem->addMap(current_map->tiledMap,current_map->tiledRender);
        displayed_map << current_map;
    }
    temp_displayed_map << current_map;
    //the border
    if(current_map->logicalMap.border.bottom.map!=NULL)
    {
        if(!displayed_map.contains(other_map[current_map->logicalMap.border.bottom.map->map_file]))
        {
            mapItem->addMap(other_map[current_map->logicalMap.border.bottom.map->map_file]->tiledMap,other_map[current_map->logicalMap.border.bottom.map->map_file]->tiledRender);
            displayed_map << other_map[current_map->logicalMap.border.bottom.map->map_file];
        }
        temp_displayed_map << other_map[current_map->logicalMap.border.bottom.map->map_file];
    }
    if(current_map->logicalMap.border.top.map!=NULL)
    {
        if(!displayed_map.contains(other_map[current_map->logicalMap.border.top.map->map_file]))
        {
            mapItem->addMap(other_map[current_map->logicalMap.border.top.map->map_file]->tiledMap,other_map[current_map->logicalMap.border.top.map->map_file]->tiledRender);
            displayed_map << other_map[current_map->logicalMap.border.top.map->map_file];
        }
        temp_displayed_map << other_map[current_map->logicalMap.border.top.map->map_file];
    }
    if(current_map->logicalMap.border.left.map!=NULL)
    {
        if(!displayed_map.contains(other_map[current_map->logicalMap.border.left.map->map_file]))
        {
            mapItem->addMap(other_map[current_map->logicalMap.border.left.map->map_file]->tiledMap,other_map[current_map->logicalMap.border.left.map->map_file]->tiledRender);
            displayed_map << other_map[current_map->logicalMap.border.left.map->map_file];
        }
        temp_displayed_map << other_map[current_map->logicalMap.border.left.map->map_file];
    }
    if(current_map->logicalMap.border.right.map!=NULL)
    {
        if(!displayed_map.contains(other_map[current_map->logicalMap.border.right.map->map_file]))
        {
            mapItem->addMap(other_map[current_map->logicalMap.border.right.map->map_file]->tiledMap,other_map[current_map->logicalMap.border.right.map->map_file]->tiledRender);
            displayed_map << other_map[current_map->logicalMap.border.right.map->map_file];
        }
        temp_displayed_map << other_map[current_map->logicalMap.border.right.map->map_file];
    }
    //the map to remove
    QSet<Map_full *>::const_iterator i = displayed_map.constBegin();
    while (i != displayed_map.constEnd()) {
        if(!temp_displayed_map.contains(*i))
        {
            mapItem->removeMap((*i)->tiledMap);
            //if it's the last reference
            if(!other_map.contains((*i)->logicalMap.map_file))
            {
                delete (*i)->logicalMap.parsed_layer.walkable;
                delete (*i)->logicalMap.parsed_layer.water;
                delete (*i)->tiledMap;
                delete (*i)->tiledRender;
                delete (*i);
            }
            displayed_map.remove(*i);
            i = displayed_map.constBegin();//needed
        }
        else
           ++i;
    }
    //set the position
    mapItem->setMapPosition(current_map->tiledMap,0,0);
    if(current_map->logicalMap.border.left.map!=NULL)
        mapItem->setMapPosition(other_map[current_map->logicalMap.border_semi.left.fileName]->tiledMap,
                                -(quint32)current_map->logicalMap.border.left.map->width,-(quint32)current_map->logicalMap.border.left.y_offset);
    if(current_map->logicalMap.border.right.map!=NULL)
        mapItem->setMapPosition(other_map[current_map->logicalMap.border_semi.right.fileName]->tiledMap,
                                (quint32)current_map->logicalMap.width,-(quint32)current_map->logicalMap.border.right.y_offset);
    if(current_map->logicalMap.border.top.map!=NULL)
        mapItem->setMapPosition(other_map[current_map->logicalMap.border_semi.top.fileName]->tiledMap,
                                -(quint32)current_map->logicalMap.border.top.x_offset,-(quint32)current_map->logicalMap.border.top.map->height);
    if(current_map->logicalMap.border.bottom.map!=NULL)
        mapItem->setMapPosition(other_map[current_map->logicalMap.border_semi.bottom.fileName]->tiledMap,
                                -(quint32)current_map->logicalMap.border.bottom.x_offset,(quint32)current_map->logicalMap.height);
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
        qDebug() << QString("xPerso: %1, yPerso: %2, map: %3").arg(xPerso).arg(yPerso).arg(current_map->logicalMap.map_file);
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
