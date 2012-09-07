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
    qDebug() << QString("loadOtherMap: %1, tiledMap: %2").arg(resolvedFileName).arg(quint64(tempMapObject->tiledMap));

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
    tempMapObject->objectGroup = new ObjectGroup(QString("Dyna management %1").arg(fileInformations.fileName()),0,0,tempMapObject->tiledMap->width(),tempMapObject->tiledMap->height());

    //add a tags
    Tiled::MapObject * tagMapObject = new MapObject();
    tagMapObject->setTile(tagTileset->tileAt(tagTilesetIndex));
    tagMapObject->setPosition(QPoint(tempMapObject->logicalMap.width/2,tempMapObject->logicalMap.height/2+1));
    tempMapObject->objectGroup->addObject(tagMapObject);
    tagTilesetIndex++;
    if(tagTilesetIndex>=tagTileset->tileCount())
        tagTilesetIndex=0;

    qDebug() << QString("Add for %1 the layer: %2").arg(fileInformations.fileName()).arg((quint64)tempMapObject->objectGroup);
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

    //load the teleporter
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

    loadPlayerFromCurrentMap();
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

    unloadPlayerFromCurrentMap();
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

void MapVisualiserQt::blinkDynaLayer()
{
    current_map->objectGroup->setVisible(!current_map->objectGroup->isVisible());
    //do it here only because it's one player, then max 3 call by second
    viewport()->update();
}
