#include "MapVisualiser.h"

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>
#include <QTime>
#include <QDebug>
#include <QFileInfo>
#include <QPointer>

#include "../../general/base/MoveOnTheMap.h"
#include "../../general/libtiled/tile.h"

void MapVisualiser::resetAll()
{
    QSet<QString>::const_iterator i = displayed_map.constBegin();
    while (i != displayed_map.constEnd()) {
        mapItem->removeMap(all_map[*i]->tiledMap);
        displayed_map.remove(*i);
        i = displayed_map.constBegin();
    }
    displayed_map.clear();

    all_map.clear();
    loadedNearMap.clear();
}

//open the file, and load it into the variables
QString MapVisualiser::loadOtherMap(const QString &fileName)
{
    QFileInfo fileInformations(fileName);
    QString resolvedFileName=fileInformations.absoluteFilePath();
    if(all_map.contains(resolvedFileName))
        return resolvedFileName;

    Map_full *tempMapObject=new Map_full();

    //load the map
    tempMapObject->tiledMap = reader.readMap(resolvedFileName);
    if (!tempMapObject->tiledMap)
    {
        mLastError=reader.errorString();
        qDebug() << QString("Unable to load the map: %1, error: %2").arg(resolvedFileName).arg(reader.errorString());
        delete tempMapObject;
        return QString();
    }
    Pokecraft::Map_loader map_loader;
    if(!map_loader.tryLoadMap(resolvedFileName))
    {
        mLastError=map_loader.errorString();
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
    tempMapObject->logicalMap.parsed_layer.grass                    = map_loader.map_to_send.parsed_layer.grass;
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
    tempMapObject->logicalMap.teleport_semi.clear();
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
    case Tiled::Map::Isometric:
        tempMapObject->tiledRender = new Tiled::IsometricRenderer(tempMapObject->tiledMap);
        break;
    case Tiled::Map::Orthogonal:
    default:
        tempMapObject->tiledRender = new Tiled::OrthogonalRenderer(tempMapObject->tiledMap);
        break;
    }
    tempMapObject->tiledRender->setObjectBorder(false);

    //do the object group to move the player on it
    tempMapObject->objectGroup = new Tiled::ObjectGroup("Dyna management",0,0,tempMapObject->tiledMap->width(),tempMapObject->tiledMap->height());

    //add a tags
    if(debugTags)
    {
        Tiled::MapObject * tagMapObject = new Tiled::MapObject();
        tagMapObject->setTile(tagTileset->tileAt(tagTilesetIndex));
        tagMapObject->setPosition(QPoint(tempMapObject->logicalMap.width/2,tempMapObject->logicalMap.height/2+1));
        tempMapObject->objectGroup->addObject(tagMapObject);
        tagTilesetIndex++;
        if(tagTilesetIndex>=tagTileset->tileCount())
            tagTilesetIndex=0;
    }
    else //remove the hidden tags, and unknow layer
    {
        index=0;
        while(index<tempMapObject->tiledMap->layerCount())
        {
            if(Tiled::ObjectGroup *objectGroup = tempMapObject->tiledMap->layerAt(index)->asObjectGroup())
            {
                //remove the unknow layer
                if(objectGroup->name()!="Moving")
                    delete tempMapObject->tiledMap->takeLayerAt(index);
                else
                {
                    QList<Tiled::MapObject*> objects=objectGroup->objects();
                    int index2=0;
                    while(index2<objects.size())
                    {
                        //remove the unknow object
                        if(objects.at(index2)->type()!="door")
                        {
                            objectGroup->removeObject(objects.at(index2));
                            delete objects.at(index2);
                        }
                        index2++;
                    }
                    index++;
                }
            }
            else
                index++;
        }
    }

    //search WalkBehind layer
    index=0;
    while(index<tempMapObject->tiledMap->layerCount())
    {
        if(tempMapObject->tiledMap->layerAt(index)->name()=="WalkBehind")
        {
            tempMapObject->objectGroupIndex=index;
            tempMapObject->tiledMap->insertLayer(index,tempMapObject->objectGroup);
            break;
        }
        index++;
    }
    if(index==tempMapObject->tiledMap->layerCount())
    {
        //search Collisions layer
        index=0;
        while(index<tempMapObject->tiledMap->layerCount())
        {
            if(tempMapObject->tiledMap->layerAt(index)->name()=="Collisions")
            {
                tempMapObject->objectGroupIndex=index+1;
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
    }

    //move the Moving layer on dyna layer
    index=0;
    while(index<tempMapObject->tiledMap->layerCount())
    {
        if(Tiled::ObjectGroup *objectGroup = tempMapObject->tiledMap->layerAt(index)->asObjectGroup())
        {
            if(objectGroup->name()=="Moving")
            {
                Tiled::Layer *layer = tempMapObject->tiledMap->takeLayerAt(index);
                if(tempMapObject->objectGroupIndex-1<=0)
                    tempMapObject->tiledMap->insertLayer(0,layer);
                else
                {
                    if(index>tempMapObject->objectGroupIndex)
                        tempMapObject->objectGroupIndex++;
                    tempMapObject->tiledMap->insertLayer(tempMapObject->objectGroupIndex-1,layer);
                }
            }
        }
        else if(Tiled::TileLayer *tileLayer = tempMapObject->tiledMap->layerAt(index)->asTileLayer())
        {
            if(tileLayer->name()=="Grass")
            {
                grass = tempMapObject->tiledMap->takeLayerAt(index);
                if(tempMapObject->objectGroupIndex-1<=0)
                    tempMapObject->tiledMap->insertLayer(0,grass);
                else
                {
                    if(index>tempMapObject->objectGroupIndex)
                        tempMapObject->objectGroupIndex++;
                    tempMapObject->tiledMap->insertLayer(tempMapObject->objectGroupIndex-1,grass);
                }
                grassOver = grass->clone();
                grassOver->setName("Grass over");
                tempMapObject->tiledMap->addLayer(grassOver);

                QSet<Tiled::Tileset*> tilesets=grassOver->usedTilesets();
                QSet<Tiled::Tileset*>::const_iterator i = tilesets.constBegin();
                while (i != tilesets.constEnd()) {
                     Tiled::Tileset* oldTileset=*i;
                     Tiled::MapReader mapReader;
                     QFile tsxFile(oldTileset->fileName());
                     if(tsxFile.open(QIODevice::ReadOnly))
                     {
                         Tiled::Tileset* newTileset=mapReader.readTileset(&tsxFile,QFileInfo(tsxFile).absoluteFilePath());
                         if(newTileset!=NULL)
                         {
                             Tiled::Tile * currentTile;
                             QSet<Tiled::Tile *> tileUsed;
                             int indexTile=0;
                             while(indexTile<=newTileset->tileCount())
                             {
                                 currentTile=newTileset->tileAt(indexTile);
                                 if(currentTile!=NULL)
                                     if(!tileUsed.contains(currentTile))
                                     {
                                         qDebug() << "New tile" << tileUsed;
                                         QPixmap pixmap=currentTile->image();
                                         pixmap.fill();
                                         currentTile->setImage(pixmap);
                                         tileUsed << currentTile;
                                     }
                                 indexTile++;
                             }
                             grassOver->replaceReferencesToTileset(*i,newTileset);
                         }
                         else
                             qDebug() << "Unable to load the tileset:" << oldTileset->fileName() << ", error:" << mapReader.errorString();
                     }
                     else
                         qDebug() << "Unable to open the tsx file:" << tsxFile.fileName() << ", error:" << tsxFile.errorString();
                     ++i;
                 }
            }
        }
        index++;
    }

    all_map[resolvedFileName]=tempMapObject;

    return resolvedFileName;
}

void MapVisualiser::loadCurrentMap()
{
    loadedNearMap.clear();

    QSet<QString> mapUsed;
    Map_full *tempMapObject=current_map;

    loadNearMap(tempMapObject->logicalMap.map_file);

    //load the teleporter
    int index=0;
    while(index<tempMapObject->logicalMap.teleport_semi.size())
    {
        QString mapIndex=loadOtherMap(tempMapObject->logicalMap.teleport_semi[index].map);
        //if is correctly loaded
        if(!mapIndex.isEmpty())
        {
            //if the teleporter is in range
            if(tempMapObject->logicalMap.teleport_semi[index].destination_x<all_map[mapIndex]->logicalMap.width && tempMapObject->logicalMap.teleport_semi[index].destination_y<all_map[mapIndex]->logicalMap.height)
            {
                int virtual_position=tempMapObject->logicalMap.teleport_semi[index].source_x+tempMapObject->logicalMap.teleport_semi[index].source_y*tempMapObject->logicalMap.width;
                tempMapObject->logicalMap.teleporter[virtual_position].map=&all_map[mapIndex]->logicalMap;
                tempMapObject->logicalMap.teleporter[virtual_position].x=tempMapObject->logicalMap.teleport_semi[index].destination_x;
                tempMapObject->logicalMap.teleporter[virtual_position].y=tempMapObject->logicalMap.teleport_semi[index].destination_y;

                mapUsed << mapIndex;
            }
            else
                qDebug() << QString("The teleporter is out of range: %1").arg(mapIndex);
        }
        index++;
    }

    //remove the not displayed map
    {
        QSet<QString>::const_iterator i = displayed_map.constBegin();
        while (i != displayed_map.constEnd()) {
            if(!loadedNearMap.contains(*i))
            {
                mapItem->removeMap(all_map[*i]->tiledMap);
                displayed_map.remove(*i);
                i = displayed_map.constBegin();
            }
            else
                ++i;
        }
    }
    //remove the not used map
    QHash<QString,Map_full *>::const_iterator i = all_map.constBegin();
    while (i != all_map.constEnd()) {
        if(!mapUsed.contains((*i)->logicalMap.map_file) && !loadedNearMap.contains((*i)->logicalMap.map_file))
        {
            if((*i)->logicalMap.parsed_layer.walkable!=NULL)
                delete (*i)->logicalMap.parsed_layer.walkable;
            if((*i)->logicalMap.parsed_layer.water!=NULL)
                delete (*i)->logicalMap.parsed_layer.water;
            if((*i)->logicalMap.parsed_layer.grass!=NULL)
                delete (*i)->logicalMap.parsed_layer.grass;
            qDeleteAll((*i)->tiledMap->tilesets());
            delete (*i)->tiledMap;
            delete (*i)->tiledRender;
            delete (*i);
            all_map.remove((*i)->logicalMap.map_file);
            i = all_map.constBegin();//needed
        }
        else
            ++i;
    }
}

void MapVisualiser::loadNearMap(const QString &fileName, const qint32 &x, const qint32 &y, const qint32 &x_pixel, const qint32 &y_pixel)
{
    if(loadedNearMap.contains(fileName))
        return;

    Map_full *tempMapObject;
    if(!all_map.contains(fileName))
    {
        qDebug() << QString("loadCurrentMap(): the current map is unable to load: %1").arg(fileName);
        return;
    }
    else
        tempMapObject=all_map[fileName];

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
    if(!displayed_map.contains(fileName))
    {
        mapItem->addMap(tempMapObject->tiledMap,tempMapObject->tiledRender,tempMapObject->objectGroupIndex);
        displayed_map << fileName;
    }
    mapItem->setMapPosition(tempMapObject->tiledMap,x_pixel,y_pixel);

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
                if(fileName==all_map[mapIndex]->logicalMap.border_semi.top.fileName && tempMapObject->logicalMap.border_semi.bottom.fileName==mapIndex)
                {
                    int offset=tempMapObject->logicalMap.border_semi.bottom.x_offset-all_map[mapIndex]->logicalMap.border_semi.top.x_offset;
                    const qint32 x_sub=x+offset;
                    const qint32 y_sub=y+tempMapObject->logicalMap.height;
                    QRect border_map_rect(x_sub,y_sub,all_map[mapIndex]->logicalMap.width,all_map[mapIndex]->logicalMap.height);
                    //if the new map touch the current map
                    if(RectTouch(current_map_rect,border_map_rect))
                    {
                        tempMapObject->logicalMap.border.bottom.map=&all_map[mapIndex]->logicalMap;
                        tempMapObject->logicalMap.border.bottom.x_offset=-offset;
                        all_map[mapIndex]->logicalMap.border.top.x_offset=offset;

                        loadNearMap(mapIndex,x_sub,y_sub,x_pixel+(x_sub-x)*tempMapObject->tiledMap->tileWidth(),y_pixel+(y_sub-y)*tempMapObject->tiledMap->tileHeight());
                    }
                }
                else
                    qDebug() << QString("loadNearMap(): bottom: have not mutual border %1").arg(fileName);
            }
            else
                qDebug() << QString("loadNearMap(): bottom: not correctly loaded %1").arg(fileName);
        }
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
                if(fileName==all_map[mapIndex]->logicalMap.border_semi.bottom.fileName && tempMapObject->logicalMap.border_semi.top.fileName==mapIndex)
                {
                    int offset=tempMapObject->logicalMap.border_semi.top.x_offset-all_map[mapIndex]->logicalMap.border_semi.bottom.x_offset;
                    const qint32 x_sub=x+offset;
                    const qint32 y_sub=y-all_map[mapIndex]->logicalMap.height;
                    QRect border_map_rect(x_sub,y_sub,all_map[mapIndex]->logicalMap.width,all_map[mapIndex]->logicalMap.height);
                    //if the new map touch the current map
                    if(RectTouch(current_map_rect,border_map_rect))
                    {
                        tempMapObject->logicalMap.border.top.map=&all_map[mapIndex]->logicalMap;
                        tempMapObject->logicalMap.border.top.x_offset=-offset;
                        all_map[mapIndex]->logicalMap.border.bottom.x_offset=offset;

                        loadNearMap(mapIndex,x_sub,y_sub,x_pixel+(x_sub-x)*tempMapObject->tiledMap->tileWidth(),y_pixel+(y_sub-y)*tempMapObject->tiledMap->tileHeight());
                    }
                }
                else
                    qDebug() << QString("loadNearMap(): top: have not mutual border %1").arg(fileName);
            }
            else
                qDebug() << QString("loadNearMap(): top: not correctly loaded %1").arg(fileName);
        }
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
                if(fileName==all_map[mapIndex]->logicalMap.border_semi.left.fileName && tempMapObject->logicalMap.border_semi.right.fileName==mapIndex)
                {
                    int offset=tempMapObject->logicalMap.border_semi.right.y_offset-all_map[mapIndex]->logicalMap.border_semi.left.y_offset;
                    const qint32 x_sub=x+tempMapObject->logicalMap.width;
                    const qint32 y_sub=y+offset;
                    QRect border_map_rect(x_sub,y_sub,all_map[mapIndex]->logicalMap.width,all_map[mapIndex]->logicalMap.height);
                    //if the new map touch the current map
                    if(RectTouch(current_map_rect,border_map_rect))
                    {
                        tempMapObject->logicalMap.border.right.map=&all_map[mapIndex]->logicalMap;
                        tempMapObject->logicalMap.border.right.y_offset=-offset;
                        all_map[mapIndex]->logicalMap.border.left.y_offset=offset;

                        loadNearMap(mapIndex,x_sub,y_sub,x_pixel+(x_sub-x)*tempMapObject->tiledMap->tileWidth(),y_pixel+(y_sub-y)*tempMapObject->tiledMap->tileHeight());
                    }
                }
                else
                    qDebug() << QString("loadNearMap(): right: have not mutual border %1").arg(fileName);
            }
            else
                qDebug() << QString("loadNearMap(): right: not correctly loaded %1").arg(fileName);
        }
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
                if(fileName==all_map[mapIndex]->logicalMap.border_semi.right.fileName && tempMapObject->logicalMap.border_semi.left.fileName==mapIndex)
                {
                    int offset=tempMapObject->logicalMap.border_semi.left.y_offset-all_map[mapIndex]->logicalMap.border_semi.right.y_offset;
                    const qint32 x_sub=x-all_map[mapIndex]->logicalMap.width;
                    const qint32 y_sub=y+offset;
                    QRect border_map_rect(x_sub,y_sub,all_map[mapIndex]->logicalMap.width,all_map[mapIndex]->logicalMap.height);
                    //if the new map touch the current map
                    if(RectTouch(current_map_rect,border_map_rect))
                    {
                        tempMapObject->logicalMap.border.left.map=&all_map[mapIndex]->logicalMap;
                        tempMapObject->logicalMap.border.left.y_offset=-offset;
                        all_map[mapIndex]->logicalMap.border.right.y_offset=offset;

                        loadNearMap(mapIndex,x_sub,y_sub,x_pixel+(x_sub-x)*tempMapObject->tiledMap->tileWidth(),y_pixel+(y_sub-y)*tempMapObject->tiledMap->tileHeight());
                    }
                }
                else
                    qDebug() << QString("loadNearMap(): left: have not mutual border %1").arg(fileName);
            }
            else
                qDebug() << QString("loadNearMap(): left: not correctly loaded %1").arg(fileName);
        }
    }
}
