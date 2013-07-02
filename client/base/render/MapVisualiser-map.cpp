#include "MapVisualiser.h"

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>
#include <QTime>
#include <QDebug>
#include <QFileInfo>
#include <QPointer>

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
    all_map.remove(map->logicalMap.map_file);
    //delete common variables
    CatchChallenger::Map::removeParsedLayer(map->logicalMap.parsed_layer);
    qDeleteAll(map->tiledMap->tilesets());
    delete map->tiledMap;
    delete map->tiledRender;
    delete map;
}

void MapVisualiser::resetAll()
{
    QSet<QString>::const_iterator i = displayed_map.constBegin();
    while (i != displayed_map.constEnd()) {
        mapItem->removeMap(all_map[*i]->tiledMap);
        displayed_map.remove(*i);
        i = displayed_map.constBegin();
    }
    ///remove the not used map, then where no player is susceptible to switch (by border or teleporter)
    QHash<QString,MapVisualiserThread::Map_full *>::const_iterator j = all_map.constBegin();
    while (j != all_map.constEnd()) {
        destroyMap(*j);
        j = all_map.constBegin();//needed
    }
    displayed_map.clear();
    all_map.clear();
    mapVisualiserThread.resetAll();
}

//open the file, and load it into the variables
QString MapVisualiser::loadOtherMap(const QString &resolvedFileName)
{
    if(all_map.contains(resolvedFileName))
        return resolvedFileName;

    if(asyncMap.contains(resolvedFileName))
        return resolvedFileName;
    CatchChallenger::DebugClass::debugConsole(QString("need async this load: %1").arg(resolvedFileName));
    asyncMap << resolvedFileName;
    emit loadOtherMapAsync(resolvedFileName);
    return resolvedFileName;
}

void MapVisualiser::asyncDetectBorder(MapVisualiserThread::Map_full * tempMapObject)
{
    if(tempMapObject==NULL)
    {
        qDebug() << "Map is NULL, can't load more at MapVisualiser::asyncDetectBorder()";
        return;
    }
    all_map[tempMapObject->logicalMap.map_file]=tempMapObject;
    QRect current_map_rect;
    if(current_map!=NULL)
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
        if(!displayed_map.contains(tempMapObject->logicalMap.map_file))
        {
            mapItem->addMap(tempMapObject->tiledMap,tempMapObject->tiledRender,tempMapObject->objectGroupIndex);
            displayed_map << tempMapObject->logicalMap.map_file;
            emit mapDisplayed(tempMapObject->logicalMap.map_file);
        }
        mapItem->setMapPosition(tempMapObject->tiledMap,tempMapObject->x_pixel,tempMapObject->y_pixel);
        if(!tempMapObject->logicalMap.border_semi.bottom.fileName.isEmpty())
            if(!asyncMap.contains(tempMapObject->logicalMap.border_semi.bottom.fileName) && !all_map.contains(tempMapObject->logicalMap.border_semi.bottom.fileName))
            {
                asyncMap << tempMapObject->logicalMap.border_semi.bottom.fileName;
                emit loadOtherMapAsync(tempMapObject->logicalMap.border_semi.bottom.fileName);
            }
        if(!tempMapObject->logicalMap.border_semi.top.fileName.isEmpty())
            if(!asyncMap.contains(tempMapObject->logicalMap.border_semi.top.fileName) && !all_map.contains(tempMapObject->logicalMap.border_semi.top.fileName))
            {
                asyncMap << tempMapObject->logicalMap.border_semi.top.fileName;
                emit loadOtherMapAsync(tempMapObject->logicalMap.border_semi.top.fileName);
            }
        if(!tempMapObject->logicalMap.border_semi.left.fileName.isEmpty())
            if(!asyncMap.contains(tempMapObject->logicalMap.border_semi.left.fileName) && !all_map.contains(tempMapObject->logicalMap.border_semi.left.fileName))
            {
                asyncMap << tempMapObject->logicalMap.border_semi.left.fileName;
                emit loadOtherMapAsync(tempMapObject->logicalMap.border_semi.left.fileName);
            }
        if(!tempMapObject->logicalMap.border_semi.right.fileName.isEmpty())
            if(!asyncMap.contains(tempMapObject->logicalMap.border_semi.right.fileName) && !all_map.contains(tempMapObject->logicalMap.border_semi.right.fileName))
            {
                asyncMap << tempMapObject->logicalMap.border_semi.right.fileName;
                emit loadOtherMapAsync(tempMapObject->logicalMap.border_semi.right.fileName);
            }
    }
}

void MapVisualiser::asyncMapLoaded(MapVisualiserThread::Map_full * tempMapObject)
{
    asyncMap.removeOne(tempMapObject->logicalMap.map_file);
    if(all_map.contains(tempMapObject->logicalMap.map_file))
    {
        CatchChallenger::DebugClass::debugConsole(QString("seam already loaded by sync call, then skip: %1").arg(tempMapObject->logicalMap.map_file));
        return;
    }
    //CatchChallenger::DebugClass::debugConsole(QString("todo, place the loaded map: %1").arg(tempMapObject->logicalMap.map_file));
    //destroyMap(tempMapObject);
    //try locate and place it
    if(tempMapObject->logicalMap.map_file==current_map)
    {
        tempMapObject->x=0;
        tempMapObject->y=0;
        tempMapObject->x_pixel=0;
        tempMapObject->y_pixel=0;
        asyncDetectBorder(tempMapObject);
    }
    else if(all_map.contains(tempMapObject->logicalMap.border_semi.top.fileName))
    {
        MapVisualiserThread::Map_full *border_map=all_map[tempMapObject->logicalMap.border_semi.top.fileName];
        //if both border match
        if(tempMapObject->logicalMap.map_file==border_map->logicalMap.border_semi.bottom.fileName)
        {
            int offset=tempMapObject->logicalMap.border_semi.bottom.x_offset-border_map->logicalMap.border_semi.top.x_offset;
            int offset_pixel=tempMapObject->logicalMap.border_semi.bottom.x_offset*tempMapObject->tiledMap->tileWidth()-border_map->logicalMap.border_semi.top.x_offset*border_map->tiledMap->tileWidth();
            tempMapObject->x=border_map->x+offset;
            tempMapObject->y=border_map->y+border_map->logicalMap.height;
            tempMapObject->x_pixel=border_map->x_pixel+offset_pixel;
            tempMapObject->y_pixel=border_map->y_pixel+border_map->logicalMap.height;
            asyncDetectBorder(tempMapObject);
        }
        else
            qDebug() << QString("loadNearMap(): bottom: have not mutual border %1").arg(tempMapObject->logicalMap.map_file);
    }
    else if(all_map.contains(tempMapObject->logicalMap.border_semi.bottom.fileName))
    {
        MapVisualiserThread::Map_full *border_map=all_map[tempMapObject->logicalMap.border_semi.bottom.fileName];
        //if both border match
        if(tempMapObject->logicalMap.map_file==border_map->logicalMap.border_semi.top.fileName)
        {
            int offset=tempMapObject->logicalMap.border_semi.top.x_offset-border_map->logicalMap.border_semi.bottom.x_offset;
            int offset_pixel=tempMapObject->logicalMap.border_semi.top.x_offset*tempMapObject->tiledMap->tileWidth()-border_map->logicalMap.border_semi.bottom.x_offset*border_map->tiledMap->tileWidth();
            tempMapObject->x=border_map->x+offset;
            tempMapObject->y=border_map->y-tempMapObject->logicalMap.height;
            tempMapObject->x_pixel=border_map->x_pixel+offset_pixel;
            tempMapObject->y_pixel=border_map->y_pixel-tempMapObject->logicalMap.height;
            asyncDetectBorder(tempMapObject);
        }
        else
            qDebug() << QString("loadNearMap(): bottom: have not mutual border %1").arg(tempMapObject->logicalMap.map_file);
    }
    else if(all_map.contains(tempMapObject->logicalMap.border_semi.right.fileName))
    {
        MapVisualiserThread::Map_full *border_map=all_map[tempMapObject->logicalMap.border_semi.right.fileName];
        //if both border match
        if(tempMapObject->logicalMap.map_file==border_map->logicalMap.border_semi.left.fileName)
        {
            int offset=tempMapObject->logicalMap.border_semi.left.y_offset-border_map->logicalMap.border_semi.right.y_offset;
            int offset_pixel=tempMapObject->logicalMap.border_semi.left.y_offset*tempMapObject->tiledMap->tileWidth()-border_map->logicalMap.border_semi.right.y_offset*border_map->tiledMap->tileWidth();
            tempMapObject->x=border_map->x+border_map->logicalMap.width;
            tempMapObject->y=border_map->y+offset;
            tempMapObject->x_pixel=border_map->x_pixel+border_map->logicalMap.width;
            tempMapObject->y_pixel=border_map->y_pixel+offset_pixel;
            asyncDetectBorder(tempMapObject);
        }
        else
            qDebug() << QString("loadNearMap(): bottom: have not mutual border %1").arg(tempMapObject->logicalMap.map_file);
    }
    else if(all_map.contains(tempMapObject->logicalMap.border_semi.left.fileName))
    {
        MapVisualiserThread::Map_full *border_map=all_map[tempMapObject->logicalMap.border_semi.left.fileName];
        //if both border match
        if(tempMapObject->logicalMap.map_file==border_map->logicalMap.border_semi.bottom.fileName)
        {
            int offset=tempMapObject->logicalMap.border_semi.right.y_offset-border_map->logicalMap.border_semi.left.y_offset;
            int offset_pixel=tempMapObject->logicalMap.border_semi.right.y_offset*tempMapObject->tiledMap->tileWidth()-border_map->logicalMap.border_semi.left.y_offset*border_map->tiledMap->tileWidth();
            tempMapObject->x=border_map->x-tempMapObject->logicalMap.width;
            tempMapObject->y=border_map->y+offset;
            tempMapObject->x_pixel=border_map->x_pixel-tempMapObject->logicalMap.width;
            tempMapObject->y_pixel=border_map->y_pixel+offset_pixel;
            asyncDetectBorder(tempMapObject);
        }
        else
            qDebug() << QString("loadNearMap(): bottom: have not mutual border %1").arg(tempMapObject->logicalMap.map_file);
    }
    else
    {
        CatchChallenger::DebugClass::debugConsole(QString("map not located to place it then skip: %1").arg(tempMapObject->logicalMap.map_file));
        destroyMap(tempMapObject);
        return;
    }
    //drop the old map (not loaded into this cycle)
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

QSet<QString> MapVisualiser::loadMap(MapVisualiserThread::Map_full *map,const bool &display)
{
    if(map==NULL)
    {
        qDebug() << "Map is NULL, can't load more at MapVisualiser::loadMap()";
        return QSet<QString>();
    }
    QSet<QString> loadedNearMap;
    loadedNearMap=loadNearMap(map->logicalMap.map_file,true,0,0,0,0,loadedNearMap);
    QSet<QString> loadedTeleporter=loadTeleporter(map);

    //add the new map visible directly done into loadNearMap() to have position in pixel
    //remove the not displayed map
    if(display)
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
    return loadedTeleporter+loadedNearMap;
}

void MapVisualiser::removeUnusedMap()
{
    ///remove the not used map, then where no player is susceptible to switch (by border or teleporter)
    QHash<QString,MapVisualiserThread::Map_full *>::const_iterator i = all_map.constBegin();
    while (i != all_map.constEnd()) {
        if(!mapUsed.contains((*i)->logicalMap.map_file))
        {
            destroyMap(*i);
            i = all_map.constBegin();//needed
        }
        else
            ++i;
    }
}

QSet<QString> MapVisualiser::loadTeleporter(MapVisualiserThread::Map_full *map)
{
    QSet<QString> mapUsed;
    //load the teleporter
    int index=0;
    while(index<map->logicalMap.teleport_semi.size())
    {
        QString mapIndex=loadOtherMap(map->logicalMap.teleport_semi[index].map);
        //if is correctly loaded
        if(!mapIndex.isEmpty())
        {
            //if the teleporter is in range
            if(map->logicalMap.teleport_semi[index].destination_x<all_map[mapIndex]->logicalMap.width && map->logicalMap.teleport_semi[index].destination_y<all_map[mapIndex]->logicalMap.height)
            {
                int virtual_position=map->logicalMap.teleport_semi[index].source_x+map->logicalMap.teleport_semi[index].source_y*map->logicalMap.width;
                map->logicalMap.teleporter[virtual_position].map=&all_map[mapIndex]->logicalMap;
                map->logicalMap.teleporter[virtual_position].x=map->logicalMap.teleport_semi[index].destination_x;
                map->logicalMap.teleporter[virtual_position].y=map->logicalMap.teleport_semi[index].destination_y;

                mapUsed << mapIndex;
            }
            else
                qDebug() << QString("The teleporter is out of range: %1").arg(mapIndex);
        }
        index++;
    }
    return mapUsed;
}

QSet<QString> MapVisualiser::loadNearMap(const QString &fileName, const bool &display, const qint32 &x, const qint32 &y, const qint32 &x_pixel, const qint32 &y_pixel,const QSet<QString> &previousLoadedNearMap)
{
    if(previousLoadedNearMap.contains(fileName))
        return QSet<QString>();

    MapVisualiserThread::Map_full *tempMapObject;
    if(!all_map.contains(fileName))
    {
        qDebug() << QString("loadNearMap(): the current map is unable to load: %1").arg(fileName);
        return QSet<QString>();
    }
    else
        tempMapObject=all_map[fileName];

    QSet<QString> loadedNearMap;
    loadedNearMap << fileName;

    QString mapIndex;
    QRect current_map_rect;
    if(current_map!=NULL)
        current_map_rect=QRect(0,0,all_map[current_map]->logicalMap.width,all_map[current_map]->logicalMap.height);
    else
        qDebug() << "The current map is not set, crash prevented";

    //reset the other map
    tempMapObject->logicalMap.teleporter.clear();
    tempMapObject->logicalMap.border.bottom.map=NULL;
    tempMapObject->logicalMap.border.top.map=NULL;
    tempMapObject->logicalMap.border.left.map=NULL;
    tempMapObject->logicalMap.border.right.map=NULL;
    tempMapObject->x=x;
    tempMapObject->y=y;
    tempMapObject->x_pixel=x_pixel;
    tempMapObject->y_pixel=y_pixel;

    //display a new map now visible
    if(display && !displayed_map.contains(fileName))
    {
        mapItem->addMap(tempMapObject->tiledMap,tempMapObject->tiledRender,tempMapObject->objectGroupIndex);
        displayed_map << fileName;
    }
    mapItem->setMapPosition(tempMapObject->tiledMap,x_pixel,y_pixel);

    //if have bottom border
    if(!tempMapObject->logicalMap.border_semi.bottom.fileName.isEmpty())
    {
        //if the position is good to have border in range
        if((y+tempMapObject->logicalMap.height)<=all_map[current_map]->logicalMap.height)
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
                    if(current_map!=NULL && CatchChallenger::FacilityLib::rectTouch(current_map_rect,border_map_rect))
                    {
                        tempMapObject->logicalMap.border.bottom.map=&all_map[mapIndex]->logicalMap;
                        tempMapObject->logicalMap.border.bottom.x_offset=-offset;
                        all_map[mapIndex]->logicalMap.border.top.x_offset=offset;

                        loadedNearMap+=loadNearMap(mapIndex,display,x_sub,y_sub,x_pixel+(x_sub-x)*tempMapObject->tiledMap->tileWidth(),y_pixel+(y_sub-y)*tempMapObject->tiledMap->tileHeight(),previousLoadedNearMap+loadedNearMap);
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
                    if(current_map!=NULL && CatchChallenger::FacilityLib::rectTouch(current_map_rect,border_map_rect))
                    {
                        tempMapObject->logicalMap.border.top.map=&all_map[mapIndex]->logicalMap;
                        tempMapObject->logicalMap.border.top.x_offset=-offset;
                        all_map[mapIndex]->logicalMap.border.bottom.x_offset=offset;

                        loadedNearMap+=loadNearMap(mapIndex,display,x_sub,y_sub,x_pixel+(x_sub-x)*tempMapObject->tiledMap->tileWidth(),y_pixel+(y_sub-y)*tempMapObject->tiledMap->tileHeight(),previousLoadedNearMap+loadedNearMap);
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
        if((x+tempMapObject->logicalMap.width)<=all_map[current_map]->logicalMap.width)
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
                    if(current_map!=NULL && CatchChallenger::FacilityLib::rectTouch(current_map_rect,border_map_rect))
                    {
                        tempMapObject->logicalMap.border.right.map=&all_map[mapIndex]->logicalMap;
                        tempMapObject->logicalMap.border.right.y_offset=-offset;
                        all_map[mapIndex]->logicalMap.border.left.y_offset=offset;

                        loadedNearMap+=loadNearMap(mapIndex,display,x_sub,y_sub,x_pixel+(x_sub-x)*tempMapObject->tiledMap->tileWidth(),y_pixel+(y_sub-y)*tempMapObject->tiledMap->tileHeight(),previousLoadedNearMap+loadedNearMap);
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
                    if(current_map!=NULL && CatchChallenger::FacilityLib::rectTouch(current_map_rect,border_map_rect))
                    {
                        tempMapObject->logicalMap.border.left.map=&all_map[mapIndex]->logicalMap;
                        tempMapObject->logicalMap.border.left.y_offset=-offset;
                        all_map[mapIndex]->logicalMap.border.right.y_offset=offset;

                        loadedNearMap+=loadNearMap(mapIndex,display,x_sub,y_sub,x_pixel+(x_sub-x)*tempMapObject->tiledMap->tileWidth(),y_pixel+(y_sub-y)*tempMapObject->tiledMap->tileHeight(),previousLoadedNearMap+loadedNearMap);
                    }
                }
                else
                    qDebug() << QString("loadNearMap(): left: have not mutual border %1").arg(fileName);
            }
            else
                qDebug() << QString("loadNearMap(): left: not correctly loaded %1").arg(fileName);
        }
    }

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
                qDebug() << QString("loadNearMap(): lookAt: missing, fixed to bottom").arg(fileName);
            direction="bottom";
        }
        loadBotOnTheMap(tempMapObject,i.value().botId,i.key().first,i.key().second,direction,skin);
    }

    return loadedNearMap;
}
