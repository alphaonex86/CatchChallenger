#include "MapVisualiser.h"

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>
#include <QTime>
#include <QDebug>
#include <QFileInfo>
#include <QPointer>

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/MoveOnTheMap.h"
#include "../../general/base/DebugClass.h"
#include "../../general/libtiled/tile.h"
#include "../../general/base/Map.h"

/// \warning all ObjectGroupItem destroyed into removeMap()
void MapVisualiser::destroyMap(MapVisualiserThread::Map_full *map)
{
    //logicalMap.plantList, delete plants useless, destroyed into removeMap()
    //logicalMap.botsDisplay, delete bot useless, destroyed into removeMap()
    //remove from the list
    if(map->tiledMap!=NULL)
        mapItem->removeMap(map->tiledMap);
    all_map.remove(map->logicalMap.map_file);
    old_all_map.remove(map->logicalMap.map_file);
    //delete common variables
    CatchChallenger::Map::removeParsedLayer(map->logicalMap.parsed_layer);
    map->logicalMap.parsed_layer.dirt=NULL;
    map->logicalMap.parsed_layer.grass=NULL;
    map->logicalMap.parsed_layer.ledges=NULL;
    map->logicalMap.parsed_layer.walkable=NULL;
    map->logicalMap.parsed_layer.water=NULL;
    qDeleteAll(map->tiledMap->tilesets());
    if(map->tiledMap!=NULL)
        delete map->tiledMap;
    map->tiledMap=NULL;
    if(map->tiledRender!=NULL)
        delete map->tiledRender;
    map->tiledRender=NULL;
    delete map;
}

void MapVisualiser::resetAll()
{
    ///remove the not used map, then where no player is susceptible to switch (by border or teleporter)
    QHash<QString,MapVisualiserThread::Map_full *>::const_iterator i = old_all_map.constBegin();
    while (i != old_all_map.constEnd()) {
        destroyMap(i.value());
        i = old_all_map.constBegin();//needed
    }

    QHash<QString,MapVisualiserThread::Map_full *>::const_iterator j = all_map.constBegin();
    while (j != all_map.constEnd()) {
        destroyMap(j.value());
        j = all_map.constBegin();//needed
    }
    old_all_map.clear();
    all_map.clear();
    mapVisualiserThread.resetAll();
}

//open the file, and load it into the variables
void MapVisualiser::loadOtherMap(const QString &resolvedFileName)
{
    //already loaded
    if(all_map.contains(resolvedFileName))
        return;
    //already in progress
    if(asyncMap.contains(resolvedFileName))
        return;
    //previously loaded
    if(old_all_map.contains(resolvedFileName))
    {
        MapVisualiserThread::Map_full * tempMapObject=old_all_map[resolvedFileName];
        tempMapObject->displayed=false;
        old_all_map.remove(resolvedFileName);
        tempMapObject->logicalMap.border.bottom.map=NULL;
        tempMapObject->logicalMap.border.top.map=NULL;
        tempMapObject->logicalMap.border.left.map=NULL;
        tempMapObject->logicalMap.border.right.map=NULL;
        tempMapObject->logicalMap.teleporter.clear();
        asyncMap << resolvedFileName;
        asyncMapLoaded(resolvedFileName,tempMapObject);
        return;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(!QFileInfo(resolvedFileName).exists())
        CatchChallenger::DebugClass::debugConsole(QString("file not found to async: %1").arg(resolvedFileName));
    #endif
    asyncMap << resolvedFileName;
    emit loadOtherMapAsync(resolvedFileName);
}

void MapVisualiser::asyncDetectBorder(MapVisualiserThread::Map_full * tempMapObject)
{
    if(tempMapObject==NULL)
    {
        qDebug() << "Map is NULL, can't load more at MapVisualiser::asyncDetectBorder()";
        return;
    }
    QRect current_map_rect;
    if(!current_map.isEmpty() && all_map.contains(current_map))
        current_map_rect=QRect(0,0,all_map[current_map]->logicalMap.width,all_map[current_map]->logicalMap.height);
    else
    {
        qDebug() << "The current map is not set, crash prevented";
        return;
    }
    QRect map_rect(tempMapObject->x,tempMapObject->y,tempMapObject->logicalMap.width,tempMapObject->logicalMap.height);
    //if the new map touch the current map
    if(CatchChallenger::FacilityLib::rectTouch(current_map_rect,map_rect))
    {
        //display a new map now visible
        if(!mapItem->haveMap(tempMapObject->tiledMap))
            mapItem->addMap(tempMapObject->tiledMap,tempMapObject->tiledRender,tempMapObject->objectGroupIndex);
        mapItem->setMapPosition(tempMapObject->tiledMap,tempMapObject->x_pixel,tempMapObject->y_pixel);
        emit mapDisplayed(tempMapObject->logicalMap.map_file);
        //display the bot
        QHashIterator<QPair<quint8,quint8>,CatchChallenger::Bot> i(tempMapObject->logicalMap.bots);
        while (i.hasNext()) {
            i.next();
            QString skin;
            if(i.value().properties.contains("skin"))
                skin=i.value().properties["skin"];
            else
                skin="empty";
            QString direction;
            if(i.value().properties.contains("lookAt"))
                direction=i.value().properties["lookAt"];
            else
            {
                if(!skin.isEmpty())
                    qDebug() << QString("asyncDetectBorder(): lookAt: missing, fixed to bottom: %1").arg(tempMapObject->logicalMap.map_file);
                direction="bottom";
            }
            loadBotOnTheMap(tempMapObject,i.value().botId,i.key().first,i.key().second,direction,skin);
        }
        if(!tempMapObject->logicalMap.border_semi.bottom.fileName.isEmpty())
            if(!asyncMap.contains(tempMapObject->logicalMap.border_semi.bottom.fileName) && !all_map.contains(tempMapObject->logicalMap.border_semi.bottom.fileName))
                loadOtherMap(tempMapObject->logicalMap.border_semi.bottom.fileName);
        if(!tempMapObject->logicalMap.border_semi.top.fileName.isEmpty())
            if(!asyncMap.contains(tempMapObject->logicalMap.border_semi.top.fileName) && !all_map.contains(tempMapObject->logicalMap.border_semi.top.fileName))
                loadOtherMap(tempMapObject->logicalMap.border_semi.top.fileName);
        if(!tempMapObject->logicalMap.border_semi.left.fileName.isEmpty())
            if(!asyncMap.contains(tempMapObject->logicalMap.border_semi.left.fileName) && !all_map.contains(tempMapObject->logicalMap.border_semi.left.fileName))
                loadOtherMap(tempMapObject->logicalMap.border_semi.left.fileName);
        if(!tempMapObject->logicalMap.border_semi.right.fileName.isEmpty())
            if(!asyncMap.contains(tempMapObject->logicalMap.border_semi.right.fileName) && !all_map.contains(tempMapObject->logicalMap.border_semi.right.fileName))
                loadOtherMap(tempMapObject->logicalMap.border_semi.right.fileName);
    }
}

bool MapVisualiser::asyncMapLoaded(const QString &fileName, MapVisualiserThread::Map_full * tempMapObject)
{
    if(all_map.contains(fileName))
    {
        asyncMap.removeOne(fileName);
        CatchChallenger::DebugClass::debugConsole(QString("seam already loaded by sync call, internal bug on: %1").arg(fileName));
        return false;
    }
    if(tempMapObject!=NULL)
    {
        tempMapObject->displayed=false;
        all_map[tempMapObject->logicalMap.map_file]=tempMapObject;
        QHash<quint16,MapVisualiserThread::Map_animation>::const_iterator i = tempMapObject->animatedObject.constBegin();
        while (i != tempMapObject->animatedObject.constEnd()) {
            if(!animationTimer.contains(i.key()))
            {
                QTimer *newTimer=new QTimer();
                newTimer->setInterval(i.key());
                animationTimer[i.key()]=newTimer;
                connect(newTimer,SIGNAL(timeout()),this,SLOT(applyTheAnimationTimer()));
                /// \todo syncro all the timer start, with frame offset to align with the other timer
                newTimer->start();
            }
            ++i;
        }
        //try locate and place it
        if(tempMapObject->logicalMap.map_file==current_map)
        {
            tempMapObject->displayed=true;
            tempMapObject->x=0;
            tempMapObject->y=0;
            tempMapObject->x_pixel=0;
            tempMapObject->y_pixel=0;
            loadTeleporter(tempMapObject);
            asyncDetectBorder(tempMapObject);
        }
        else
        {
            QRect current_map_rect;
            if(!current_map.isEmpty() && all_map.contains(current_map))
            {
                current_map_rect=QRect(0,0,all_map[current_map]->logicalMap.width,all_map[current_map]->logicalMap.height);
                if(all_map.contains(tempMapObject->logicalMap.border_semi.top.fileName))
                {
                    if(all_map[tempMapObject->logicalMap.border_semi.top.fileName]->displayed)
                    {
                        MapVisualiserThread::Map_full *border_map=all_map[tempMapObject->logicalMap.border_semi.top.fileName];
                        //if both border match
                        if(tempMapObject->logicalMap.map_file==border_map->logicalMap.border_semi.bottom.fileName)
                        {
                            int offset=tempMapObject->logicalMap.border_semi.top.x_offset-border_map->logicalMap.border_semi.bottom.x_offset;
                            int offset_pixel=tempMapObject->logicalMap.border_semi.top.x_offset*tempMapObject->tiledMap->tileWidth()-border_map->logicalMap.border_semi.bottom.x_offset*border_map->tiledMap->tileWidth();
                            tempMapObject->x=border_map->x+offset;
                            tempMapObject->y=border_map->y+border_map->logicalMap.height;
                            tempMapObject->x_pixel=border_map->x_pixel+offset_pixel;
                            tempMapObject->y_pixel=border_map->y_pixel+border_map->logicalMap.height*border_map->tiledMap->tileHeight();
                            tempMapObject->logicalMap.border.top.map=&border_map->logicalMap;
                            tempMapObject->logicalMap.border.top.x_offset=offset;
                            border_map->logicalMap.border.bottom.map=&tempMapObject->logicalMap;
                            border_map->logicalMap.border.bottom.x_offset=-offset;
                            QRect map_rect(tempMapObject->x,tempMapObject->y,tempMapObject->logicalMap.width,tempMapObject->logicalMap.height);
                            if(CatchChallenger::FacilityLib::rectTouch(current_map_rect,map_rect))
                            {
                                tempMapObject->displayed=true;
                                asyncDetectBorder(tempMapObject);
                            }
                        }
                        else
                            qDebug() << QString("loadNearMap(): bottom: have not mutual border %1").arg(tempMapObject->logicalMap.map_file);
                    }
                }
                if(all_map.contains(tempMapObject->logicalMap.border_semi.bottom.fileName))
                {
                    if(all_map[tempMapObject->logicalMap.border_semi.bottom.fileName]->displayed)
                    {
                        MapVisualiserThread::Map_full *border_map=all_map[tempMapObject->logicalMap.border_semi.bottom.fileName];
                        //if both border match
                        if(tempMapObject->logicalMap.map_file==border_map->logicalMap.border_semi.top.fileName)
                        {
                            int offset=tempMapObject->logicalMap.border_semi.bottom.x_offset-border_map->logicalMap.border_semi.top.x_offset;
                            int offset_pixel=tempMapObject->logicalMap.border_semi.bottom.x_offset*tempMapObject->tiledMap->tileWidth()-border_map->logicalMap.border_semi.top.x_offset*border_map->tiledMap->tileWidth();
                            tempMapObject->x=border_map->x+offset;
                            tempMapObject->y=border_map->y-tempMapObject->logicalMap.height;
                            tempMapObject->x_pixel=border_map->x_pixel+offset_pixel;
                            tempMapObject->y_pixel=border_map->y_pixel-tempMapObject->logicalMap.height*tempMapObject->tiledMap->tileHeight();
                            tempMapObject->logicalMap.border.bottom.map=&border_map->logicalMap;
                            tempMapObject->logicalMap.border.bottom.x_offset=offset;
                            border_map->logicalMap.border.top.map=&tempMapObject->logicalMap;
                            border_map->logicalMap.border.top.x_offset=-offset;
                            QRect map_rect(tempMapObject->x,tempMapObject->y,tempMapObject->logicalMap.width,tempMapObject->logicalMap.height);
                            if(CatchChallenger::FacilityLib::rectTouch(current_map_rect,map_rect))
                            {
                                tempMapObject->displayed=true;
                                asyncDetectBorder(tempMapObject);
                            }
                        }
                        else
                            qDebug() << QString("loadNearMap(): bottom: have not mutual border %1").arg(tempMapObject->logicalMap.map_file);
                    }
                }
                if(all_map.contains(tempMapObject->logicalMap.border_semi.right.fileName))
                {
                    if(all_map[tempMapObject->logicalMap.border_semi.right.fileName]->displayed)
                    {
                        MapVisualiserThread::Map_full *border_map=all_map[tempMapObject->logicalMap.border_semi.right.fileName];
                        //if both border match
                        if(tempMapObject->logicalMap.map_file==border_map->logicalMap.border_semi.left.fileName)
                        {
                            int offset=tempMapObject->logicalMap.border_semi.right.y_offset-border_map->logicalMap.border_semi.left.y_offset;
                            int offset_pixel=tempMapObject->logicalMap.border_semi.right.y_offset*tempMapObject->tiledMap->tileHeight()-border_map->logicalMap.border_semi.left.y_offset*border_map->tiledMap->tileHeight();
                            tempMapObject->x=border_map->x-tempMapObject->logicalMap.width;
                            tempMapObject->y=border_map->y+offset;
                            tempMapObject->x_pixel=border_map->x_pixel-tempMapObject->logicalMap.width*tempMapObject->tiledMap->tileWidth();
                            tempMapObject->y_pixel=border_map->y_pixel+offset_pixel;
                            tempMapObject->logicalMap.border.right.map=&border_map->logicalMap;
                            tempMapObject->logicalMap.border.right.y_offset=offset;
                            border_map->logicalMap.border.left.map=&tempMapObject->logicalMap;
                            border_map->logicalMap.border.left.y_offset=-offset;
                            QRect map_rect(tempMapObject->x,tempMapObject->y,tempMapObject->logicalMap.width,tempMapObject->logicalMap.height);
                            if(CatchChallenger::FacilityLib::rectTouch(current_map_rect,map_rect))
                            {
                                tempMapObject->displayed=true;
                                asyncDetectBorder(tempMapObject);
                            }
                        }
                        else
                            qDebug() << QString("loadNearMap(): bottom: have not mutual border %1").arg(tempMapObject->logicalMap.map_file);
                    }
                }
                if(all_map.contains(tempMapObject->logicalMap.border_semi.left.fileName))
                {
                    if(all_map[tempMapObject->logicalMap.border_semi.left.fileName]->displayed)
                    {
                        MapVisualiserThread::Map_full *border_map=all_map[tempMapObject->logicalMap.border_semi.left.fileName];
                        //if both border match
                        if(tempMapObject->logicalMap.map_file==border_map->logicalMap.border_semi.right.fileName)
                        {
                            int offset=tempMapObject->logicalMap.border_semi.left.y_offset-border_map->logicalMap.border_semi.right.y_offset;
                            int offset_pixel=tempMapObject->logicalMap.border_semi.left.y_offset*tempMapObject->tiledMap->tileHeight()-border_map->logicalMap.border_semi.right.y_offset*border_map->tiledMap->tileHeight();
                            tempMapObject->x=border_map->x+border_map->logicalMap.width;
                            tempMapObject->y=border_map->y+offset;
                            tempMapObject->x_pixel=border_map->x_pixel+border_map->logicalMap.width*border_map->tiledMap->tileWidth();
                            tempMapObject->y_pixel=border_map->y_pixel+offset_pixel;
                            tempMapObject->logicalMap.border.left.map=&border_map->logicalMap;
                            tempMapObject->logicalMap.border.left.y_offset=offset;
                            border_map->logicalMap.border.right.map=&tempMapObject->logicalMap;
                            border_map->logicalMap.border.right.y_offset=-offset;
                            QRect map_rect(tempMapObject->x,tempMapObject->y,tempMapObject->logicalMap.width,tempMapObject->logicalMap.height);
                            if(CatchChallenger::FacilityLib::rectTouch(current_map_rect,map_rect))
                            {
                                tempMapObject->displayed=true;
                                asyncDetectBorder(tempMapObject);
                            }
                        }
                        else
                            qDebug() << QString("loadNearMap(): bottom: have not mutual border %1").arg(tempMapObject->logicalMap.map_file);
                    }
                }
            }
            else
                qDebug() << "The current map is not set, crash prevented";
        }
    }
    if(asyncMap.removeOne(fileName))
        if(asyncMap.isEmpty())
            removeUnusedMap();
    return true;
}

void MapVisualiser::applyTheAnimationTimer()
{
    QTimer *timer=qobject_cast<QTimer *>(QObject::sender());
    quint16 interval=timer->interval();
    bool isUsed=false;
    QHash<QString,MapVisualiserThread::Map_full *>::const_iterator i = all_map.constBegin();
    while (i != all_map.constEnd()) {
        MapVisualiserThread::Map_full * tempMap=i.value();
        if(tempMap->displayed)
        {
            if(tempMap->animatedObject.contains(interval))
            {
                if(tempMap->animatedObject[interval].frames>1)
                {
                    isUsed=true;
                    tempMap->animatedObject[interval].count++;
                    qint8 frameOffset=1;
                    if(tempMap->animatedObject[interval].count>=tempMap->animatedObject[interval].frames)
                    {
                        frameOffset+=-tempMap->animatedObject[interval].frames;
                        tempMap->animatedObject[interval].count=0;
                    }
                    QList<Tiled::MapObject *> animatedObject=tempMap->animatedObject[interval].animatedObject;
                    int index=0;
                    while(index<animatedObject.size())
                    {
                        Tiled::MapObject * mapObject=animatedObject.at(index);
                        Tiled::Tile *tile=mapObject->tile();
                        Tiled::Tile *newTile=tile->tileset()->tileAt(tile->id()+frameOffset);
                        mapObject->setTile(newTile);
                        index++;
                    }
                }
            }
        }
        ++i;
    }
    if(!isUsed)
    {
        animationTimer.remove(interval);
        timer->stop();
        delete timer;
    }
}

void MapVisualiser::loadBotOnTheMap(MapVisualiserThread::Map_full *parsedMap,const quint32 &botId,const quint8 &x,const quint8 &y,const QString &lookAt,const QString &skin)
{
    Q_UNUSED(botId);
    Q_UNUSED(parsedMap);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(lookAt);
    Q_UNUSED(skin);
}

void MapVisualiser::removeUnusedMap()
{
    //undisplay the unused map
    QHash<QString,MapVisualiserThread::Map_full *>::const_iterator i = old_all_map.constBegin();
    while (i != old_all_map.constEnd()) {
        destroyMap(i.value());
        i = old_all_map.constBegin();
    }
    //undisplay the map not in dirrect contact with the current_map
    QHash<QString,MapVisualiserThread::Map_full *>::const_iterator j = all_map.constBegin();
    while (j != all_map.constEnd()) {
        if(j.value()->logicalMap.map_file!=current_map)
            if(!j.value()->displayed)
                mapItem->removeMap(j.value()->tiledMap);
        ++j;
    }
}

void MapVisualiser::passMapIntoOld()
{
    if(old_all_map.isEmpty())
        old_all_map=all_map;
    else
    {
        QHash<QString,MapVisualiserThread::Map_full *>::const_iterator i = all_map.constBegin();
        while (i != all_map.constEnd()) {
            old_all_map[i.key()]=i.value();
            ++i;
        }
    }
    all_map.clear();
}

void MapVisualiser::loadTeleporter(MapVisualiserThread::Map_full *map)
{
    //load the teleporter
    int index=0;
    while(index<map->logicalMap.teleport_semi.size())
    {
        loadOtherMap(map->logicalMap.teleport_semi[index].map);
        index++;
    }
}
