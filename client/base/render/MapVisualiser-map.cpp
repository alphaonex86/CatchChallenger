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
#include "../../tiled/tiled_tile.h"
#include "../../general/base/CommonMap.h"
#include "../ClientVariable.h"

/// \warning all ObjectGroupItem destroyed into removeMap()
void MapVisualiser::destroyMap(MapVisualiserThread::Map_full *map)
{
    //logicalMap.plantList, delete plants useless, destroyed into removeMap()
    //logicalMap.botsDisplay, delete bot useless, destroyed into removeMap()
    //remove from the list
    QHashIterator<QPair<quint8,quint8>,MapDoor*> i(map->doors);
    while (i.hasNext()) {
        i.next();
        //object pointer removed by group
        i.value()->deleteLater();
    }
    map->doors.clear();
    if(map->tiledMap!=NULL)
        mapItem->removeMap(map->tiledMap);
    all_map.remove(map->logicalMap.map_file);
    old_all_map.remove(map->logicalMap.map_file);
    //delete common variables
    CatchChallenger::CommonMap::removeParsedLayer(map->logicalMap.parsed_layer);
    map->logicalMap.parsed_layer.dirt=NULL;
    map->logicalMap.parsed_layer.monstersCollisionMap=NULL;
    map->logicalMap.parsed_layer.ledges=NULL;
    map->logicalMap.parsed_layer.walkable=NULL;
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
    old_all_map_time.clear();
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
        MapVisualiserThread::Map_full * tempMapObject=old_all_map.value(resolvedFileName);
        tempMapObject->displayed=false;
        old_all_map.remove(resolvedFileName);
        old_all_map_time.remove(resolvedFileName);
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
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("file not found to async: %1").arg(resolvedFileName));
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
        current_map_rect=QRect(0,0,all_map.value(current_map)->logicalMap.width,all_map.value(current_map)->logicalMap.height);
    else
    {
        qDebug() << "The current map is not set, crash prevented";
        return;
    }
    QRect map_rect(tempMapObject->relative_x,tempMapObject->relative_y,tempMapObject->logicalMap.width,tempMapObject->logicalMap.height);
    //if the new map touch the current map
    if(CatchChallenger::FacilityLib::rectTouch(current_map_rect,map_rect))
    {
        //display a new map now visible
        if(!mapItem->haveMap(tempMapObject->tiledMap))
            mapItem->addMap(tempMapObject->tiledMap,tempMapObject->tiledRender,tempMapObject->objectGroupIndex);
        mapItem->setMapPosition(tempMapObject->tiledMap,tempMapObject->relative_x_pixel,tempMapObject->relative_y_pixel);
        emit mapDisplayed(tempMapObject->logicalMap.map_file);
        //display the bot
        QHashIterator<QPair<quint8,quint8>,CatchChallenger::Bot> i(tempMapObject->logicalMap.bots);
        while (i.hasNext()) {
            i.next();
            QString skin;
            if(i.value().properties.contains(QStringLiteral("skin")))
                skin=i.value().properties.value(QStringLiteral("skin"));
            else
                skin=QString();
            QString direction;
            if(i.value().properties.contains(QStringLiteral("lookAt")))
                direction=i.value().properties.value(QStringLiteral("lookAt"));
            else
            {
                if(!skin.isEmpty())
                    qDebug() << QStringLiteral("asyncDetectBorder(): lookAt: missing, fixed to bottom: %1").arg(tempMapObject->logicalMap.map_file);
                direction=QStringLiteral("bottom");
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
    if(current_map.isEmpty())
        return false;
    if(all_map.contains(fileName))
    {
        asyncMap.removeOne(fileName);
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("seam already loaded by sync call, internal bug on: %1").arg(fileName));
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
                animationFrame[i.key()];//creation
                connect(newTimer,&QTimer::timeout,this,&MapVisualiser::applyTheAnimationTimer);
                newTimer->start();
            }
            if(!animationFrame.value(i.key()).contains(i.value().count))
                animationFrame[i.key()][i.value().count]=0;
            else
            {
                const quint8 &count=animationFrame.value(i.key()).value(i.value().count);
                tempMapObject->animatedObject[i.key()].count=count;
                if(i.value().count!=0)
                {
                    int index=0;
                    while(index<i.value().animatedObjectList.size())
                    {
                        Tiled::MapObject * mapObject=i.value().animatedObjectList.at(index).animatedObject;
                        Tiled::Cell cell=mapObject->cell();
                        Tiled::Tile *tile=mapObject->cell().tile;
                        Tiled::Tile *newTile;
                        if((count+i.value().animatedObjectList.at(index).randomOffset)>=i.value().frameCountTotal)
                            newTile=tile->tileset()->tileAt(tile->id()+count-i.value().frameCountTotal);
                        else
                            newTile=tile->tileset()->tileAt(tile->id()+count);
                        cell.tile=newTile;
                        mapObject->setCell(cell);
                        index++;
                    }
                }
            }
            ++i;
        }
        //try locate and place it
        if(tempMapObject->logicalMap.map_file==current_map)
        {
            tempMapObject->displayed=true;
            tempMapObject->relative_x=0;
            tempMapObject->relative_y=0;
            tempMapObject->relative_x_pixel=0;
            tempMapObject->relative_y_pixel=0;
            loadTeleporter(tempMapObject);
            asyncDetectBorder(tempMapObject);
        }
        else
        {
            QRect current_map_rect;
            if(!current_map.isEmpty() && all_map.contains(current_map))
            {
                current_map_rect=QRect(0,0,all_map.value(current_map)->logicalMap.width,all_map.value(current_map)->logicalMap.height);
                if(all_map.contains(tempMapObject->logicalMap.border_semi.top.fileName))
                {
                    if(all_map.value(tempMapObject->logicalMap.border_semi.top.fileName)->displayed)
                    {
                        MapVisualiserThread::Map_full *border_map=all_map.value(tempMapObject->logicalMap.border_semi.top.fileName);
                        //if both border match
                        if(tempMapObject->logicalMap.map_file==border_map->logicalMap.border_semi.bottom.fileName)
                        {
                            const int &offset=border_map->logicalMap.border_semi.bottom.x_offset-tempMapObject->logicalMap.border_semi.top.x_offset;
                            const int &offset_pixel=border_map->logicalMap.border_semi.bottom.x_offset*border_map->tiledMap->tileWidth()-tempMapObject->logicalMap.border_semi.top.x_offset*tempMapObject->tiledMap->tileWidth();
                            tempMapObject->relative_x=border_map->relative_x+offset;
                            tempMapObject->relative_y=border_map->relative_y+border_map->logicalMap.height;
                            tempMapObject->relative_x_pixel=border_map->relative_x_pixel+offset_pixel;
                            tempMapObject->relative_y_pixel=border_map->relative_y_pixel+border_map->logicalMap.height*border_map->tiledMap->tileHeight();
                            tempMapObject->logicalMap.border.top.map=&border_map->logicalMap;
                            tempMapObject->logicalMap.border.top.x_offset=offset;
                            border_map->logicalMap.border.bottom.map=&tempMapObject->logicalMap;
                            border_map->logicalMap.border.bottom.x_offset=-offset;
                            QRect map_rect(tempMapObject->relative_x,tempMapObject->relative_y,tempMapObject->logicalMap.width,tempMapObject->logicalMap.height);
                            if(CatchChallenger::FacilityLib::rectTouch(current_map_rect,map_rect))
                            {
                                tempMapObject->displayed=true;
                                asyncDetectBorder(tempMapObject);
                            }
                        }
                        else
                            qDebug() << QStringLiteral("loadNearMap(): bottom: have not mutual border %1").arg(tempMapObject->logicalMap.map_file);
                    }
                }
                if(all_map.contains(tempMapObject->logicalMap.border_semi.bottom.fileName))
                {
                    if(all_map.value(tempMapObject->logicalMap.border_semi.bottom.fileName)->displayed)
                    {
                        MapVisualiserThread::Map_full *border_map=all_map.value(tempMapObject->logicalMap.border_semi.bottom.fileName);
                        //if both border match
                        if(tempMapObject->logicalMap.map_file==border_map->logicalMap.border_semi.top.fileName)
                        {
                            const int &offset=border_map->logicalMap.border_semi.top.x_offset-tempMapObject->logicalMap.border_semi.bottom.x_offset;
                            const int &offset_pixel=border_map->logicalMap.border_semi.top.x_offset*border_map->tiledMap->tileWidth()-tempMapObject->logicalMap.border_semi.bottom.x_offset*tempMapObject->tiledMap->tileWidth();
                            tempMapObject->relative_x=border_map->relative_x+offset;
                            tempMapObject->relative_y=border_map->relative_y-tempMapObject->logicalMap.height;
                            tempMapObject->relative_x_pixel=border_map->relative_x_pixel+offset_pixel;
                            tempMapObject->relative_y_pixel=border_map->relative_y_pixel-tempMapObject->logicalMap.height*tempMapObject->tiledMap->tileHeight();
                            tempMapObject->logicalMap.border.bottom.map=&border_map->logicalMap;
                            tempMapObject->logicalMap.border.bottom.x_offset=offset;
                            border_map->logicalMap.border.top.map=&tempMapObject->logicalMap;
                            border_map->logicalMap.border.top.x_offset=-offset;
                            QRect map_rect(tempMapObject->relative_x,tempMapObject->relative_y,tempMapObject->logicalMap.width,tempMapObject->logicalMap.height);
                            if(CatchChallenger::FacilityLib::rectTouch(current_map_rect,map_rect))
                            {
                                tempMapObject->displayed=true;
                                asyncDetectBorder(tempMapObject);
                            }
                        }
                        else
                            qDebug() << QStringLiteral("loadNearMap(): bottom: have not mutual border %1").arg(tempMapObject->logicalMap.map_file);
                    }
                }
                if(all_map.contains(tempMapObject->logicalMap.border_semi.right.fileName))
                {
                    if(all_map.value(tempMapObject->logicalMap.border_semi.right.fileName)->displayed)
                    {
                        MapVisualiserThread::Map_full *border_map=all_map.value(tempMapObject->logicalMap.border_semi.right.fileName);
                        //if both border match
                        if(tempMapObject->logicalMap.map_file==border_map->logicalMap.border_semi.left.fileName)
                        {
                            const int &offset=border_map->logicalMap.border_semi.left.y_offset-tempMapObject->logicalMap.border_semi.right.y_offset;
                            const int &offset_pixel=border_map->logicalMap.border_semi.left.y_offset*border_map->tiledMap->tileHeight()-tempMapObject->logicalMap.border_semi.right.y_offset*tempMapObject->tiledMap->tileHeight();
                            tempMapObject->relative_x=border_map->relative_x-tempMapObject->logicalMap.width;
                            tempMapObject->relative_y=border_map->relative_y+offset;
                            tempMapObject->relative_x_pixel=border_map->relative_x_pixel-tempMapObject->logicalMap.width*tempMapObject->tiledMap->tileWidth();
                            tempMapObject->relative_y_pixel=border_map->relative_y_pixel+offset_pixel;
                            tempMapObject->logicalMap.border.right.map=&border_map->logicalMap;
                            tempMapObject->logicalMap.border.right.y_offset=offset;
                            border_map->logicalMap.border.left.map=&tempMapObject->logicalMap;
                            border_map->logicalMap.border.left.y_offset=-offset;
                            QRect map_rect(tempMapObject->relative_x,tempMapObject->relative_y,tempMapObject->logicalMap.width,tempMapObject->logicalMap.height);
                            if(CatchChallenger::FacilityLib::rectTouch(current_map_rect,map_rect))
                            {
                                tempMapObject->displayed=true;
                                asyncDetectBorder(tempMapObject);
                            }
                        }
                        else
                            qDebug() << QStringLiteral("loadNearMap(): bottom: have not mutual border %1").arg(tempMapObject->logicalMap.map_file);
                    }
                }
                if(all_map.contains(tempMapObject->logicalMap.border_semi.left.fileName))
                {
                    if(all_map.value(tempMapObject->logicalMap.border_semi.left.fileName)->displayed)
                    {
                        MapVisualiserThread::Map_full *border_map=all_map.value(tempMapObject->logicalMap.border_semi.left.fileName);
                        //if both border match
                        if(tempMapObject->logicalMap.map_file==border_map->logicalMap.border_semi.right.fileName)
                        {
                            const int &offset=border_map->logicalMap.border_semi.right.y_offset-tempMapObject->logicalMap.border_semi.left.y_offset;
                            const int &offset_pixel=border_map->logicalMap.border_semi.right.y_offset*border_map->tiledMap->tileHeight()-tempMapObject->logicalMap.border_semi.left.y_offset*tempMapObject->tiledMap->tileHeight();
                            tempMapObject->relative_x=border_map->relative_x+border_map->logicalMap.width;
                            tempMapObject->relative_y=border_map->relative_y+offset;
                            tempMapObject->relative_x_pixel=border_map->relative_x_pixel+border_map->logicalMap.width*border_map->tiledMap->tileWidth();
                            tempMapObject->relative_y_pixel=border_map->relative_y_pixel+offset_pixel;
                            tempMapObject->logicalMap.border.left.map=&border_map->logicalMap;
                            tempMapObject->logicalMap.border.left.y_offset=offset;
                            border_map->logicalMap.border.right.map=&tempMapObject->logicalMap;
                            border_map->logicalMap.border.right.y_offset=-offset;
                            QRect map_rect(tempMapObject->relative_x,tempMapObject->relative_y,tempMapObject->logicalMap.width,tempMapObject->logicalMap.height);
                            if(CatchChallenger::FacilityLib::rectTouch(current_map_rect,map_rect))
                            {
                                tempMapObject->displayed=true;
                                asyncDetectBorder(tempMapObject);
                            }
                        }
                        else
                            qDebug() << QStringLiteral("loadNearMap(): bottom: have not mutual border %1").arg(tempMapObject->logicalMap.map_file);
                    }
                }
            }
            else
                qDebug() << "The current map is not set, crash prevented";
        }
    }
    if(asyncMap.removeOne(fileName))
        if(asyncMap.isEmpty())
        {
            hideNotloadedMap();//hide the mpa loaded by mistake
            removeUnusedMap();
        }
    return true;
}

void MapVisualiser::applyTheAnimationTimer()
{
    QTimer *timer=qobject_cast<QTimer *>(QObject::sender());
    const quint16 &interval=timer->interval();
    if(animationFrame.contains(interval))
    {
        QHash<quint8/*frame total*/,quint8/*actual frame*/> countList=animationFrame.value(interval);
        QHashIterator<quint8/*frame total*/,quint8/*actual frame*/> i(countList);
        while (i.hasNext()) {
            i.next();
            countList[i.key()]++;
            if(countList.value(i.key())>=i.key())
                countList[i.key()]=0;
        }
        animationFrame[interval]=countList;
    }
    bool isUsed=false;
    QHash<QString,MapVisualiserThread::Map_full *>::const_iterator i = all_map.constBegin();
    while (i != all_map.constEnd()) {
        MapVisualiserThread::Map_full * tempMap=i.value();
        if(tempMap->displayed)
        {
            if(tempMap->animatedObject.contains(interval))
            {
                if(tempMap->animatedObject.value(interval).frameCountTotal>1)
                {
                    isUsed=true;
                    tempMap->animatedObject[interval].count++;
                    const QList<MapVisualiserThread::Map_animation_object> &animatedObject=tempMap->animatedObject.value(interval).animatedObjectList;
                    int index=0;
                    while(index<animatedObject.size())
                    {
                        qint8 frameOffset=1;
                        if((tempMap->animatedObject.value(interval).count+animatedObject.at(index).randomOffset)==tempMap->animatedObject.value(interval).frameCountTotal)
                            frameOffset-=tempMap->animatedObject.value(interval).frameCountTotal;
                        Tiled::MapObject * mapObject=animatedObject.at(index).animatedObject;
                        Tiled::Cell cell=mapObject->cell();
                        Tiled::Tile *tile=mapObject->cell().tile;
                        Tiled::Tile *newTile=tile->tileset()->tileAt(tile->id()+frameOffset);
                        cell.tile=newTile;
                        mapObject->setCell(cell);
                        index++;
                    }
                    if(tempMap->animatedObject.value(interval).count>=tempMap->animatedObject.value(interval).frameCountTotal)
                        tempMap->animatedObject[interval].count=0;
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
    const QDateTime &currentTime=QDateTime::currentDateTime();
    //undisplay the unused map
    QHash<QString,MapVisualiserThread::Map_full *>::const_iterator i = old_all_map.constBegin();
    while (i != old_all_map.constEnd()) {
        if(!old_all_map_time.contains(i.key()) || (currentTime.toTime_t()-old_all_map_time.value(i.key()).toTime_t())>CATCHCHALLENGER_CLIENT_MAP_CACHE_TIMEOUT || old_all_map.size()>CATCHCHALLENGER_CLIENT_MAP_CACHE_SIZE)
        {
            destroyMap(i.value());
            i = old_all_map.constBegin();
        }
        else
            ++i;
    }
}

void MapVisualiser::hideNotloadedMap()
{
    //undisplay the map not in dirrect contact with the current_map
    QHash<QString,MapVisualiserThread::Map_full *>::const_iterator i = all_map.constBegin();
    while (i != all_map.constEnd()) {
        if(i.value()->logicalMap.map_file!=current_map)
            if(!i.value()->displayed)
                mapItem->removeMap(i.value()->tiledMap);
        ++i;
    }
    QHash<QString,MapVisualiserThread::Map_full *>::const_iterator j = old_all_map.constBegin();
    while (j != old_all_map.constEnd()) {
        mapItem->removeMap(j.value()->tiledMap);
        ++j;
    }
}

void MapVisualiser::passMapIntoOld()
{
    const QDateTime &currentTime=QDateTime::currentDateTime();
/*    if(old_all_map.isEmpty())
        old_all_map=all_map;
    else
    {*/
        QHash<QString,MapVisualiserThread::Map_full *>::const_iterator i = all_map.constBegin();
        while (i != all_map.constEnd()) {
            old_all_map[i.key()]=i.value();
            old_all_map_time[i.key()]=currentTime;
            ++i;
        }
    //}
    all_map.clear();
}

void MapVisualiser::loadTeleporter(MapVisualiserThread::Map_full *map)
{
    //load the teleporter
    int index=0;
    while(index<map->logicalMap.teleport_semi.size())
    {
        loadOtherMap(map->logicalMap.teleport_semi.value(index).map);
        index++;
    }
}
