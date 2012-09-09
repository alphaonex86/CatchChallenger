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

//open the file, and load it into the variables
QString MapVisualiserQt::loadOtherMap(const QString &fileName)
{
    QFileInfo fileInformations(fileName);
    QString resolvedFileName=fileInformations.absoluteFilePath();
    if(other_map.contains(resolvedFileName))
        return resolvedFileName;

    Map_full *tempMapObject=new Map_full();

    //load the map
    tempMapObject->tiledMap = reader.readMap(resolvedFileName);
    if (!tempMapObject->tiledMap)
    {
        qDebug() << QString("Unable to load the map: %1, error: %2").arg(resolvedFileName).arg(reader.errorString());
        delete tempMapObject;
        return QString();
    }
    Pokecraft::Map_loader map_loader;
    if(!map_loader.tryLoadMap(resolvedFileName))
    {
        qDebug() << QString("Unable to load the map: %1, error: %2").arg(resolvedFileName).arg(map_loader.errorString());
        int index=0;
        while(index<tempMapObject->tiledMap->tilesets().size())
        {
            delete tempMapObject->tiledMap->tilesets().at(index);
            index++;
        }
        delete tempMapObject->tiledMap;
        delete tempMapObject;
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
    tempMapObject->objectGroup = new ObjectGroup(QString("Dyna management %1").arg(fileInformations.fileName()),0,0,tempMapObject->tiledMap->width(),tempMapObject->tiledMap->height());

    //add a tags
    Tiled::MapObject * tagMapObject = new MapObject();
    tagMapObject->setTile(tagTileset->tileAt(tagTilesetIndex));
    tagMapObject->setPosition(QPoint(tempMapObject->logicalMap.width/2,tempMapObject->logicalMap.height/2+1));
    tempMapObject->objectGroup->addObject(tagMapObject);
    tagTilesetIndex++;
    if(tagTilesetIndex>=tagTileset->tileCount())
        tagTilesetIndex=0;

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
    if(index==tempMapObject->tiledMap->layerCount())
    {
        qDebug() << QString("Unable to locate the \"Collisions\" layer on the map: %1").arg(fileInformations.fileName());
        tempMapObject->tiledMap->addLayer(tempMapObject->objectGroup);
    }

    other_map[resolvedFileName]=tempMapObject;

    return resolvedFileName;
}

void MapVisualiserQt::loadCurrentMap()
{
    QSet<QString> mapUsed;
    Map_full *tempMapObject=current_map;

    mapUsed << tempMapObject->logicalMap.map_file;

    //load the teleporter
    int index=0;
    while(index<tempMapObject->logicalMap.teleport_semi.size())
    {
        QString mapIndex=loadOtherMap(tempMapObject->logicalMap.teleport_semi[index].map);
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
                //reset the other map
                tempMapObject->logicalMap.teleporter.clear();
                tempMapObject->logicalMap.border.bottom.map=NULL;
                tempMapObject->logicalMap.border.top.map=NULL;
                tempMapObject->logicalMap.border.left.map=NULL;
                tempMapObject->logicalMap.border.right.map=NULL;
                mapUsed << mapIndex;
            }
        }
        index++;
    }

    loadNearMap(tempMapObject->logicalMap.map_file);

    //remove the not used map
    QHash<QString,Map_full *>::const_iterator i = other_map.constBegin();
    while (i != other_map.constEnd()) {
        if(!mapUsed.contains((*i)->logicalMap.map_file) && !loadedNearMap.contains((*i)->logicalMap.map_file))
        {
            //if it's the last reference
            if(!displayed_map.contains(*i))
            {
                mapItem->removeMap((*i)->tiledMap);
                delete (*i)->logicalMap.parsed_layer.walkable;
                delete (*i)->logicalMap.parsed_layer.water;
                qDeleteAll((*i)->tiledMap->tilesets());
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

    loadPlayerFromCurrentMap();
    loadedNearMap.clear();
}

void MapVisualiserQt::loadNearMap(const QString &fileName, const qint32 &x, const qint32 &y)
{
    if(loadedNearMap.contains(fileName))
        return;
    qDebug() << QString("loadNearMap(): %1").arg(fileName);

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

    loadedNearMap << fileName;

    QString mapIndex;
    QRect current_map_rect(0,0,current_map->logicalMap.width,current_map->logicalMap.height);

    //reset the other map
    tempMapObject->logicalMap.teleporter.clear();
    tempMapObject->logicalMap.border.bottom.map=NULL;
    tempMapObject->logicalMap.border.top.map=NULL;
    tempMapObject->logicalMap.border.left.map=NULL;
    tempMapObject->logicalMap.border.right.map=NULL;

    //display the map
    mapItem->addMap(tempMapObject->tiledMap,tempMapObject->tiledRender);
    mapItem->setMapPosition(tempMapObject->tiledMap,x,y);

    //if have bottom border
    if(!tempMapObject->logicalMap.border_semi.bottom.fileName.isEmpty())
    {
        //if the position is good to have border in range
        if((y+tempMapObject->logicalMap.height)<=current_map->logicalMap.height)
        {
            mapIndex=loadOtherMap(tempMapObject->logicalMap.border_semi.bottom.fileName);
            //if is correctly loaded
            if(!mapIndex.isEmpty())
            {
                //if both border match
                if(fileName==other_map[mapIndex]->logicalMap.border_semi.top.fileName && tempMapObject->logicalMap.border_semi.bottom.fileName==mapIndex)
                {
                    int offset=tempMapObject->logicalMap.border_semi.bottom.x_offset-other_map[mapIndex]->logicalMap.border_semi.top.x_offset;
                    const quint32 x_sub=x+offset;
                    const quint32 y_sub=y+tempMapObject->logicalMap.height;
                    QRect border_map_rect(x_sub,y_sub,other_map[mapIndex]->logicalMap.width,other_map[mapIndex]->logicalMap.height);
                    //if the new map touch the current map
                    if(RectTouch(current_map_rect,border_map_rect))
                    {
                        tempMapObject->logicalMap.border.bottom.map=&other_map[mapIndex]->logicalMap;
                        tempMapObject->logicalMap.border.bottom.x_offset=-offset;
                        other_map[mapIndex]->logicalMap.border.top.x_offset=offset;

                        loadNearMap(mapIndex,x_sub,y_sub);
                    }
                    else
                    {
                        qDebug() << current_map_rect;
                        qDebug() << border_map_rect;
                        qDebug() << QString("loadNearMap(): bottom: the map not touch %1").arg(fileName);
                    }
                }
                else
                    qDebug() << QString("loadNearMap(): bottom: have not mutual border %1").arg(fileName);
            }
            else
                qDebug() << QString("loadNearMap(): bottom: not correctly loaded %1").arg(fileName);
        }
        else
            qDebug() << QString("loadNearMap(): bottom: the next map is out of range %1").arg(fileName);
    }
    //if have top border
    if(!tempMapObject->logicalMap.border_semi.top.fileName.isEmpty())
    {
        //if the position is good to have border in range
        if(y>=0)
        {
            mapIndex=loadOtherMap(tempMapObject->logicalMap.border_semi.top.fileName);
            //if is correctly loaded
            if(!mapIndex.isEmpty())
            {
                //if both border match
                if(fileName==other_map[mapIndex]->logicalMap.border_semi.bottom.fileName && tempMapObject->logicalMap.border_semi.top.fileName==mapIndex)
                {
                    int offset=tempMapObject->logicalMap.border_semi.top.x_offset-other_map[mapIndex]->logicalMap.border_semi.bottom.x_offset;
                    const quint32 x_sub=x+offset;
                    const quint32 y_sub=y-other_map[mapIndex]->logicalMap.height;
                    QRect border_map_rect(x_sub,y_sub,other_map[mapIndex]->logicalMap.width,other_map[mapIndex]->logicalMap.height);
                    //if the new map touch the current map
                    if(RectTouch(current_map_rect,border_map_rect))
                    {
                        tempMapObject->logicalMap.border.top.map=&other_map[mapIndex]->logicalMap;
                        tempMapObject->logicalMap.border.top.x_offset=-offset;
                        other_map[mapIndex]->logicalMap.border.bottom.x_offset=offset;

                        loadNearMap(mapIndex,x_sub,y_sub);
                    }
                    else
                    {
                        qDebug() << current_map_rect;
                        qDebug() << border_map_rect;
                        qDebug() << QString("loadNearMap(): top: the map not touch %1").arg(fileName);
                    }
                }
                else
                    qDebug() << QString("loadNearMap(): top: have not mutual border %1").arg(fileName);
            }
            else
                qDebug() << QString("loadNearMap(): top: not correctly loaded %1").arg(fileName);
        }
        else
            qDebug() << QString("loadNearMap(): top: the next map is out of range %1").arg(fileName);
    }
    //if have right border
    if(!tempMapObject->logicalMap.border_semi.right.fileName.isEmpty())
    {
        //if the position is good to have border in range
        if((x+tempMapObject->logicalMap.width)<=current_map->logicalMap.width)
        {
            mapIndex=loadOtherMap(tempMapObject->logicalMap.border_semi.right.fileName);
            //if is correctly loaded
            if(!mapIndex.isEmpty())
            {
                //if both border match
                if(fileName==other_map[mapIndex]->logicalMap.border_semi.left.fileName && tempMapObject->logicalMap.border_semi.right.fileName==mapIndex)
                {
                    int offset=tempMapObject->logicalMap.border_semi.right.y_offset-other_map[mapIndex]->logicalMap.border_semi.left.y_offset;
                    const quint32 x_sub=x+tempMapObject->logicalMap.width;
                    const quint32 y_sub=y+offset;
                    QRect border_map_rect(x_sub,y_sub,other_map[mapIndex]->logicalMap.width,other_map[mapIndex]->logicalMap.height);
                    //if the new map touch the current map
                    if(RectTouch(current_map_rect,border_map_rect))
                    {
                        tempMapObject->logicalMap.border.right.map=&other_map[mapIndex]->logicalMap;
                        tempMapObject->logicalMap.border.right.y_offset=-offset;
                        other_map[mapIndex]->logicalMap.border.left.y_offset=offset;

                        loadNearMap(mapIndex,x_sub,y_sub);
                    }
                    else
                    {
                        qDebug() << current_map_rect;
                        qDebug() << border_map_rect;
                        qDebug() << QString("loadNearMap(): right: the map not touch %1").arg(fileName);
                    }
                }
                else
                    qDebug() << QString("loadNearMap(): right: have not mutual border %1").arg(fileName);
            }
            else
                qDebug() << QString("loadNearMap(): right: not correctly loaded %1").arg(fileName);
        }
        else
            qDebug() << QString("loadNearMap(): right: the next map is out of range %1").arg(fileName);
    }
    //if have left border
    if(!tempMapObject->logicalMap.border_semi.left.fileName.isEmpty())
    {
        //if the position is good to have border in range
        if(x>=0)
        {
            mapIndex=loadOtherMap(tempMapObject->logicalMap.border_semi.left.fileName);
            //if is correctly loaded
            if(!mapIndex.isEmpty())
            {
                //if both border match
                if(fileName==other_map[mapIndex]->logicalMap.border_semi.right.fileName && tempMapObject->logicalMap.border_semi.left.fileName==mapIndex)
                {
                    int offset=tempMapObject->logicalMap.border_semi.left.y_offset-other_map[mapIndex]->logicalMap.border_semi.right.y_offset;
                    const quint32 x_sub=x-other_map[mapIndex]->logicalMap.width;
                    const quint32 y_sub=y+offset;
                    QRect border_map_rect(x_sub,y_sub,other_map[mapIndex]->logicalMap.width,other_map[mapIndex]->logicalMap.height);
                    //if the new map touch the current map
                    if(RectTouch(current_map_rect,border_map_rect))
                    {
                        tempMapObject->logicalMap.border.left.map=&other_map[mapIndex]->logicalMap;
                        tempMapObject->logicalMap.border.left.y_offset=-offset;
                        other_map[mapIndex]->logicalMap.border.right.y_offset=offset;

                        loadNearMap(mapIndex,x_sub,y_sub);
                    }
                    else
                    {
                        qDebug() << current_map_rect;
                        qDebug() << border_map_rect;
                        qDebug() << QString("loadNearMap(): left: the map not touch %1").arg(fileName);
                    }
                }
                else
                    qDebug() << QString("loadNearMap(): left: have not mutual border %1").arg(fileName);
            }
            else
                qDebug() << QString("loadNearMap(): left: not correctly loaded %1").arg(fileName);
        }
        else
            qDebug() << QString("loadNearMap(): left: the next map is out of range %1").arg(fileName);
    }
}

void MapVisualiserQt::unloadCurrentMap()
{
    unloadPlayerFromCurrentMap();
}

void MapVisualiserQt::blinkDynaLayer()
{
    current_map->objectGroup->setVisible(!current_map->objectGroup->isVisible());
    //do it here only because it's one player, then max 3 call by second
    viewport()->update();
}
