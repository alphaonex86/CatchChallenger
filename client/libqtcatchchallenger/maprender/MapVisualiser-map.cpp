#include "MapVisualiser.hpp"

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>
#include <QTime>
#include <QDebug>
#include <QFileInfo>
#include <QPointer>
#include <iostream>

#include "../../general/base/GeneralVariable.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/MoveOnTheMap.hpp"
#include <tiled/tile.h>
#include "../../general/base/CommonMap/CommonMap.hpp"
#include "../libcatchchallenger/ClientVariable.hpp"
#include "../libqtcatchchallenger/QtDatapackClientLoader.hpp"

bool MapVisualiser::rectTouch(QRect r1,QRect r2)
{
    if (r1.isNull() || r2.isNull())
        return false;

    if((r1.x()+r1.width())<r2.x())
        return false;
    if((r2.x()+r2.width())<r1.x())
        return false;

    if((r1.y()+r1.height())<r2.y())
        return false;
    if((r2.y()+r2.height())<r1.y())
        return false;

    return true;
}

/// \warning all ObjectGroupItem destroyed into removeMap()
void MapVisualiser::destroyMap(const CATCHCHALLENGER_TYPE_MAPID &mapIndex)
{
    CatchChallenger::QMap_client *map=NULL;
    if(CatchChallenger::QMap_client::all_map.find(mapIndex)!=CatchChallenger::QMap_client::all_map.cend())
        map=CatchChallenger::QMap_client::all_map.at(mapIndex);
    else if(CatchChallenger::QMap_client::old_all_map.find(mapIndex)!=CatchChallenger::QMap_client::old_all_map.cend())
        map=CatchChallenger::QMap_client::old_all_map.at(mapIndex);
    if(map==NULL)
        return;
    //logicalMap.plantList, delete plants useless, destroyed into removeMap()
    //logicalMap.botsDisplay, delete bot useless, destroyed into removeMap()
    //remove from the list
    for( const auto/*& crash with ref*/ n : map->doors ) {
        n.second->deleteLater();
    }
    map->doors.clear();
    if(map->tiledMap!=NULL)
        mapItem->removeMap(map->tiledMap.get());
    if(CatchChallenger::QMap_client::all_map.find(mapIndex)!=CatchChallenger::QMap_client::all_map.cend())
    {
        CatchChallenger::QMap_client::all_map[mapIndex]=NULL;
        CatchChallenger::QMap_client::all_map.erase(mapIndex);
    }
    if(CatchChallenger::QMap_client::old_all_map.find(mapIndex)!=CatchChallenger::QMap_client::old_all_map.cend())
    {
        CatchChallenger::QMap_client::old_all_map.erase(mapIndex);
    }
    //parsed_layer and removeParsedLayer no longer needed
    map->tiledMap=NULL;
    if(map->tiledRender!=NULL)
        delete map->tiledRender;
    map->tiledRender=NULL;
    delete map;
}

void MapVisualiser::resetAll()
{
    ///remove the not used map, then where no player is susceptible to switch (by border or teleporter)
    std::vector<CATCHCHALLENGER_TYPE_MAPID> mapListToDelete;
    for(auto /*& crash with ref*/n : CatchChallenger::QMap_client::old_all_map)
        mapListToDelete.push_back(n.first);
    for(auto /*& crash with ref*/n : CatchChallenger::QMap_client::all_map)
        mapListToDelete.push_back(n.first);
    unsigned int index=0;
    while(index<mapListToDelete.size())
    {
        destroyMap(mapListToDelete.at(index));
        index++;
    }
    CatchChallenger::QMap_client::old_all_map.clear();
    CatchChallenger::QMap_client::old_all_map_time_count.clear();
    CatchChallenger::QMap_client::all_map.clear();
    mapVisualiserThread.resetAll();
}

//open the file, and load it into the variables
void MapVisualiser::loadOtherMap(const CATCHCHALLENGER_TYPE_MAPID &mapIndex)
{
    if(current_map==0)
    {
        std::cerr << "MapVisualiser::loadOtherMap() map empty" << std::endl;
        return;
    }
    //already loaded
    if(CatchChallenger::QMap_client::all_map.find(mapIndex)!=CatchChallenger::QMap_client::all_map.cend())
        return;
    //already in progress
    if(vectorcontainsAtLeastOne(asyncMap,mapIndex))
        return;
    //previously loaded
    if(CatchChallenger::QMap_client::old_all_map.find(mapIndex)!=CatchChallenger::QMap_client::old_all_map.cend())
    {
        CatchChallenger::QMap_client * tempMapObject=CatchChallenger::QMap_client::old_all_map.at(mapIndex);
        tempMapObject->displayed=false;
        CatchChallenger::QMap_client::old_all_map.erase(mapIndex);
        CatchChallenger::QMap_client::old_all_map_time_count.erase(mapIndex);
        asyncMap.push_back(mapIndex);
        asyncMapLoaded(mapIndex,tempMapObject);
        return;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(mapIndex>=QtDatapackClientLoader::datapackLoader->get_maps().size())
        qDebug() << QStringLiteral("file not found to async: mapIndex %1").arg(mapIndex);
    #endif
    asyncMap.push_back(mapIndex);
    emit loadOtherMapAsync(mapIndex);
}

void MapVisualiser::asyncDetectBorder(const CATCHCHALLENGER_TYPE_MAPID &mapIndex)
{
    if(CatchChallenger::QMap_client::all_map.find(mapIndex)==CatchChallenger::QMap_client::all_map.cend())
    {
        qDebug() << "Map is NULL, can't load more at MapVisualiser::asyncDetectBorder()";
        return;
    }
    CatchChallenger::QMap_client *tempMapObject=CatchChallenger::QMap_client::all_map.at(mapIndex);
    const CatchChallenger::CommonMap &logicalMap=QtDatapackClientLoader::datapackLoader->getMap(mapIndex);
    QRect current_map_rect;
    if(current_map!=0 && CatchChallenger::QMap_client::all_map.find(current_map)!=CatchChallenger::QMap_client::all_map.cend())
    {
        const CatchChallenger::CommonMap &currentLogical=QtDatapackClientLoader::datapackLoader->getMap(current_map);
        current_map_rect=QRect(0,0,currentLogical.width,currentLogical.height);
    }
    else
    {
        qDebug() << "The current map is not set, crash prevented";
        return;
    }
    QRect map_rect(tempMapObject->relative_x,tempMapObject->relative_y,logicalMap.width,logicalMap.height);
    //if the new map touch the current map
    if(MapVisualiser::rectTouch(current_map_rect,map_rect))
    {
        //display a new map now visible
        if(!mapItem->haveMap(tempMapObject->tiledMap.get()))
            mapItem->addMap(mapIndex,tempMapObject->tiledMap.get(),tempMapObject->tiledRender,tempMapObject->objectGroupIndex);
        mapItem->setMapPosition(
                    tempMapObject->tiledMap.get(),
                                static_cast<uint16_t>(tempMapObject->relative_x_pixel),
                                static_cast<uint16_t>(tempMapObject->relative_y_pixel)
                                );
        emit mapDisplayed(mapIndex);
        /*      162 -        //display the bot                                                                                                                                                  
      163 -        for( const auto& n : tempMapObject->botsDisplay ) {                                                                                                                
      164 -            std::string skin;                                                                                                                                              
      165 -            if(n.second.properties.find("skin")!=n.second.properties.cend())                                                                                                      
      166 -                skin=n.second.properties.at("skin");                                                                                                                       
      167 -            std::string direction;                                                                                                                                                
      168 -            if(n.second.properties.find("lookAt")!=n.second.properties.cend())                                                                                                  
      169 -                direction=n.second.properties.at("lookAt");                                                                                                                       
      170 -            else                                                                                                                                                                
      171 -            {                                                                                                                                                                     
      172 -                if(!skin.empty())                                                                                                                                               
      173 -                    qDebug() << QStringLiteral("asyncDetectBorder(): lookAt: missing, fixed to bottom for mapIndex %1").arg(mapIndex);                                          
      174 -                direction="bottom";                                                                                                                                        
      175 -            }                                                                                                                                                              
      176 -            loadBotOnTheMap(mapIndex,n.second.botId,n.first.first,n.first.second,direction,skin);                                                                          
      177 -        }*/
        //bot display loading now handled separately via DatapackClientLoader
        //load adjacent maps via resolved borders
        const CatchChallenger::Map_Border &border=logicalMap.border;
        if(border.bottom.mapIndex!=65535)
            if(!vectorcontainsAtLeastOne(asyncMap,border.bottom.mapIndex) &&
                    CatchChallenger::QMap_client::all_map.find(border.bottom.mapIndex)==CatchChallenger::QMap_client::all_map.cend())
                loadOtherMap(border.bottom.mapIndex);
        if(border.top.mapIndex!=65535)
            if(!vectorcontainsAtLeastOne(asyncMap,border.top.mapIndex) &&
                    CatchChallenger::QMap_client::all_map.find(border.top.mapIndex)==CatchChallenger::QMap_client::all_map.cend())
                loadOtherMap(border.top.mapIndex);
        if(border.left.mapIndex!=65535)
            if(!vectorcontainsAtLeastOne(asyncMap,border.left.mapIndex) &&
                    CatchChallenger::QMap_client::all_map.find(border.left.mapIndex)==CatchChallenger::QMap_client::all_map.cend())
                loadOtherMap(border.left.mapIndex);
        if(border.right.mapIndex!=65535)
            if(!vectorcontainsAtLeastOne(asyncMap,border.right.mapIndex) &&
                    CatchChallenger::QMap_client::all_map.find(border.right.mapIndex)==CatchChallenger::QMap_client::all_map.cend())
                loadOtherMap(border.right.mapIndex);
    }
}

bool MapVisualiser::asyncMapLoaded(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, QMap_client * tempMapObject)
{
    if(current_map==0)
        return false;
    if(CatchChallenger::QMap_client::all_map.find(mapIndex)!=CatchChallenger::QMap_client::all_map.cend())
    {
        vectorremoveOne(asyncMap,mapIndex);
        qDebug() << QStringLiteral("seam already loaded by sync call, internal bug on mapIndex: %1").arg(mapIndex);
        return false;
    }
    if(tempMapObject!=NULL)
    {
        tempMapObject->displayed=false;
        CatchChallenger::QMap_client::all_map[mapIndex]=tempMapObject;
        for( auto& n : tempMapObject->animatedObject ) {
            const uint16_t &interval=static_cast<uint16_t>(n.first);
            if(animationTimer.find(interval)==animationTimer.cend())
            {
                QTimer *newTimer=new QTimer();
                newTimer->setInterval(interval);
                animationTimer[interval]=newTimer;
                if(!connect(newTimer,&QTimer::timeout,this,&MapVisualiser::applyTheAnimationTimer))
                    abort();
                newTimer->start();
            }
        }
        //try locate and place it
        if(mapIndex==current_map)
        {
            tempMapObject->displayed=true;
            tempMapObject->relative_x=0;
            tempMapObject->relative_y=0;
            tempMapObject->relative_x_pixel=0;
            tempMapObject->relative_y_pixel=0;
            loadTeleporter(mapIndex);
            asyncDetectBorder(mapIndex);
        }
        else
        {
            const CatchChallenger::CommonMap &logicalMap=QtDatapackClientLoader::datapackLoader->getMap(mapIndex);
            QRect current_map_rect;
            if(current_map!=0 && CatchChallenger::QMap_client::all_map.find(current_map)!=CatchChallenger::QMap_client::all_map.cend())
            {
                const CatchChallenger::CommonMap &currentLogical=QtDatapackClientLoader::datapackLoader->getMap(current_map);
                current_map_rect=QRect(0,0,currentLogical.width,currentLogical.height);
                const CatchChallenger::Map_Border &border=logicalMap.border;
                //check top border: this map's top connects to a displayed map's bottom
                if(border.top.mapIndex!=65535 &&
                   CatchChallenger::QMap_client::all_map.find(border.top.mapIndex)!=CatchChallenger::QMap_client::all_map.cend())
                {
                    CatchChallenger::QMap_client *border_map=CatchChallenger::QMap_client::all_map.at(border.top.mapIndex);
                    if(border_map->displayed)
                    {
                        const CatchChallenger::CommonMap &borderLogical=QtDatapackClientLoader::datapackLoader->getMap(border.top.mapIndex);
                        const int offset=border.top.x_offset;
                        const int offset_pixel=border.top.x_offset*tempMapObject->tiledMap->tileWidth();
                        tempMapObject->relative_x=border_map->relative_x+offset;
                        tempMapObject->relative_y=border_map->relative_y+borderLogical.height;
                        tempMapObject->relative_x_pixel=border_map->relative_x_pixel+offset_pixel;
                        tempMapObject->relative_y_pixel=border_map->relative_y_pixel+borderLogical.height*border_map->tiledMap->tileHeight();
                        QRect map_rect(tempMapObject->relative_x,tempMapObject->relative_y,logicalMap.width,logicalMap.height);
                        if(MapVisualiser::rectTouch(current_map_rect,map_rect))
                        {
                            tempMapObject->displayed=true;
                            asyncDetectBorder(mapIndex);
                        }
                    }
                }
                //check bottom border
                if(!tempMapObject->displayed && border.bottom.mapIndex!=65535 &&
                   CatchChallenger::QMap_client::all_map.find(border.bottom.mapIndex)!=CatchChallenger::QMap_client::all_map.cend())
                {
                    CatchChallenger::QMap_client *border_map=CatchChallenger::QMap_client::all_map.at(border.bottom.mapIndex);
                    if(border_map->displayed)
                    {
                        const int offset=border.bottom.x_offset;
                        const int offset_pixel=border.bottom.x_offset*tempMapObject->tiledMap->tileWidth();
                        tempMapObject->relative_x=border_map->relative_x+offset;
                        tempMapObject->relative_y=border_map->relative_y-logicalMap.height;
                        tempMapObject->relative_x_pixel=border_map->relative_x_pixel+offset_pixel;
                        tempMapObject->relative_y_pixel=border_map->relative_y_pixel-logicalMap.height*tempMapObject->tiledMap->tileHeight();
                        QRect map_rect(tempMapObject->relative_x,tempMapObject->relative_y,logicalMap.width,logicalMap.height);
                        if(MapVisualiser::rectTouch(current_map_rect,map_rect))
                        {
                            tempMapObject->displayed=true;
                            asyncDetectBorder(mapIndex);
                        }
                    }
                }
                //check right border
                if(!tempMapObject->displayed && border.right.mapIndex!=65535 &&
                   CatchChallenger::QMap_client::all_map.find(border.right.mapIndex)!=CatchChallenger::QMap_client::all_map.cend())
                {
                    CatchChallenger::QMap_client *border_map=CatchChallenger::QMap_client::all_map.at(border.right.mapIndex);
                    if(border_map->displayed)
                    {
                        const int offset=border.right.y_offset;
                        const int offset_pixel=border.right.y_offset*tempMapObject->tiledMap->tileHeight();
                        tempMapObject->relative_x=border_map->relative_x-logicalMap.width;
                        tempMapObject->relative_y=border_map->relative_y+offset;
                        tempMapObject->relative_x_pixel=border_map->relative_x_pixel-logicalMap.width*tempMapObject->tiledMap->tileWidth();
                        tempMapObject->relative_y_pixel=border_map->relative_y_pixel+offset_pixel;
                        QRect map_rect(tempMapObject->relative_x,tempMapObject->relative_y,logicalMap.width,logicalMap.height);
                        if(MapVisualiser::rectTouch(current_map_rect,map_rect))
                        {
                            tempMapObject->displayed=true;
                            asyncDetectBorder(mapIndex);
                        }
                    }
                }
                //check left border
                if(!tempMapObject->displayed && border.left.mapIndex!=65535 &&
                   CatchChallenger::QMap_client::all_map.find(border.left.mapIndex)!=CatchChallenger::QMap_client::all_map.cend())
                {
                    CatchChallenger::QMap_client *border_map=CatchChallenger::QMap_client::all_map.at(border.left.mapIndex);
                    if(border_map->displayed)
                    {
                        const CatchChallenger::CommonMap &borderLogical=QtDatapackClientLoader::datapackLoader->getMap(border.left.mapIndex);
                        const int offset=border.left.y_offset;
                        const int offset_pixel=border.left.y_offset*tempMapObject->tiledMap->tileHeight();
                        tempMapObject->relative_x=border_map->relative_x+borderLogical.width;
                        tempMapObject->relative_y=border_map->relative_y+offset;
                        tempMapObject->relative_x_pixel=border_map->relative_x_pixel+borderLogical.width*border_map->tiledMap->tileWidth();
                        tempMapObject->relative_y_pixel=border_map->relative_y_pixel+offset_pixel;
                        QRect map_rect(tempMapObject->relative_x,tempMapObject->relative_y,logicalMap.width,logicalMap.height);
                        if(MapVisualiser::rectTouch(current_map_rect,map_rect))
                        {
                            tempMapObject->displayed=true;
                            asyncDetectBorder(mapIndex);
                        }
                    }
                }
            }
            else
                qDebug() << "The current map is not set, crash prevented";
        }
    }
    if(vectorremoveOne(asyncMap,mapIndex))
        if(asyncMap.empty())
        {
            hideNotloadedMap();//hide the map loaded by mistake
            removeUnusedMap();
        }
    if(tempMapObject!=NULL)
        return true;
    else
        return false;
}

void MapVisualiser::applyTheAnimationTimer()
{
    QTimer *timer=qobject_cast<QTimer *>(QObject::sender());
    const uint16_t &interval=static_cast<uint16_t>(timer->interval());
    bool isUsed=false;
    for( const auto& n : CatchChallenger::QMap_client::all_map ) {
        CatchChallenger::QMap_client * tempMap=n.second;
        if(tempMap->displayed)
        {
            if(tempMap->animatedObject.find(interval)!=tempMap->animatedObject.cend())
            {
                for(auto& n:tempMap->animatedObject[interval])
                {
                    MapVisualiserOrder::Map_animation &map_animation=n.second;
                    isUsed=true;
                    std::vector<MapVisualiserThread::Map_animation_object> &animatedObject=map_animation.animatedObjectList;
                    unsigned int index=0;
                    while(index<animatedObject.size())
                    {
                        MapVisualiserThread::Map_animation_object &animation_object=animatedObject[index];

                        Tiled::MapObject * mapObject=animation_object.animatedObject;
                        Tiled::Cell cell=mapObject->cell();
                        Tiled::Tile *tile=mapObject->cell().tile();
                        Tiled::Tile *newTile=tile->tileset()->tileAt(tile->id()+1);
                        if(newTile->id()>=map_animation.maxId)
                            newTile=tile->tileset()->tileAt(map_animation.minId);
                        cell.setTile(newTile);
                        mapObject->setCell(cell);

                        index++;
                    }
                }
            }
        }
    }
    if(!isUsed)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cout << "MapVisualiser::applyTheAnimationTimer() : !isUsed for timer: " << std::to_string(interval) << std::endl;
        #endif
        animationTimer.erase(interval);
        timer->stop();
        delete timer;
    }
}

void MapVisualiser::loadBotOnTheMap(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const CATCHCHALLENGER_TYPE_BOTID &botId,
                                    const COORD_TYPE &x, const COORD_TYPE &y, const std::string &lookAt, const std::string &skin)
{
    Q_UNUSED(mapIndex);
    Q_UNUSED(botId);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(lookAt);
    Q_UNUSED(skin);
}

void MapVisualiser::removeUnusedMap()
{
    //undisplay the unused map
    std::vector<CATCHCHALLENGER_TYPE_MAPID> toDelete;
    for( const auto& n : CatchChallenger::QMap_client::old_all_map ) {
        if(CatchChallenger::QMap_client::old_all_map_time_count.find(n.first)==CatchChallenger::QMap_client::old_all_map_time_count.cend() ||
                CatchChallenger::QMap_client::old_all_map_time_count.at(n.first)>CATCHCHALLENGER_CLIENT_MAP_CACHE_TIMEOUT ||
                CatchChallenger::QMap_client::old_all_map.size()>CATCHCHALLENGER_CLIENT_MAP_CACHE_SIZE)
            toDelete.push_back(n.first);
    }
    for(const auto &id : toDelete)
        destroyMap(id);
}

void MapVisualiser::hideNotloadedMap()
{
    //undisplay the map not in direct contact with the current_map
    for( const auto& n : CatchChallenger::QMap_client::all_map )
        if(n.first!=current_map)
            if(!n.second->displayed)
                mapItem->removeMap(n.second->tiledMap.get());
    for( const auto& n : CatchChallenger::QMap_client::old_all_map )
        mapItem->removeMap(n.second->tiledMap.get());
}

void MapVisualiser::passMapIntoOld()
{
    for( const auto& n : CatchChallenger::QMap_client::all_map ) {
        CatchChallenger::QMap_client::old_all_map[n.first]=n.second;
        CatchChallenger::QMap_client::old_all_map_time_count[n.first]=0;
    }
    CatchChallenger::QMap_client::all_map.clear();
}

void MapVisualiser::loadTeleporter(const CATCHCHALLENGER_TYPE_MAPID &mapIndex)
{
    //load the teleporter
    const CatchChallenger::CommonMap &logicalMap=QtDatapackClientLoader::datapackLoader->getMap(mapIndex);
    unsigned int index=0;
    while(index<logicalMap.teleporters.size())
    {
        if(logicalMap.teleporters.at(index).mapIndex!=65535)
            loadOtherMap(logicalMap.teleporters.at(index).mapIndex);
        index++;
    }
}
