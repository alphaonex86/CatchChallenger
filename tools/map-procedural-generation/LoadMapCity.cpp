#include "LoadMapAll.h"

#include "../map-procedural-generation-terrain/LoadMap.h"
#include "../map-procedural-generation-terrain/MapBrush.h"

#include "../../client/tiled/tiled_tileset.h"
#include "../../client/tiled/tiled_tile.h"
#include "../../client/tiled/tiled_objectgroup.h"
#include "../../client/tiled/tiled_mapobject.h"

#include <iostream>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>

Tiled::Map * LoadMapAll::loadMapTemplate(MapBrush::MapTemplate &mapTemplate, const char * fileName, const unsigned int mapWidth, const unsigned int mapHeight, Tiled::Map &worldMap)
{
    Tiled::Map *map=LoadMap::readMap(fileName);
    if((unsigned int)map->width()>mapWidth)
    {
        std::cout << "map->width()>mapWitdh for city" << std::endl;
        abort();
    }
    if((unsigned int)map->height()>mapHeight)
    {
        std::cout << "map->height()>mapHeight for city" << std::endl;
        abort();
    }
    mapTemplate=MapBrush::tiledMapToMapTemplate(map,worldMap);
    //reset the auto detection to grab ALL
    mapTemplate.x=0;
    mapTemplate.y=0;
    mapTemplate.width=map->width();
    mapTemplate.height=map->height();
    //force collision layer
    if(LoadMap::haveTileLayer(*map,"Walkable"))
        mapTemplate.baseLayerIndex=LoadMap::searchTileIndexByName(*map,"Walkable");
    else if(LoadMap::haveTileLayer(*map,"OnGrass"))
        mapTemplate.baseLayerIndex=LoadMap::searchTileIndexByName(*map,"OnGrass");
    //else don nothing

    if(mapTemplate.templateTilesetToMapTileset.empty())
    {
        std::cerr << "LoadMapAll::addCityContent(): mapTemplate.templateTilesetToMapTileset.empty()" << std::endl;
        abort();
    }

    return map;
}

void LoadMapAll::addCityContent(Tiled::Map &worldMap, const unsigned int &mapXCount, const unsigned int &mapYCount,bool full)
{
    if(MapBrush::mapMask==NULL)
    {
        std::cerr << "MapBrush::mapMask==NULL (abort) into LoadMapAll::addCityContent" << std::endl;
        abort();
    }
    const unsigned int mapWidth=worldMap.width()/mapXCount;
    const unsigned int mapHeight=worldMap.height()/mapYCount;
    MapBrush::MapTemplate mapTemplateBig;
    MapBrush::MapTemplate mapTemplateMedium;
    MapBrush::MapTemplate mapTemplateSmall;
    Tiled::Map *mapBig=loadMapTemplate(mapTemplateBig,"template/city-big.tmx",mapWidth,mapHeight,worldMap);
    Tiled::Map *mapMedium=loadMapTemplate(mapTemplateMedium,"template/city-medium.tmx",mapWidth,mapHeight,worldMap);
    Tiled::Map *mapSmall=loadMapTemplate(mapTemplateSmall,"template/city-small.tmx",mapWidth,mapHeight,worldMap);

    MapBrush::MapTemplate mapTemplatebuildingshop;
    MapBrush::MapTemplate mapTemplatebuildingheal;
    MapBrush::MapTemplate mapTemplatebuilding1;
    MapBrush::MapTemplate mapTemplatebuilding2;
    Tiled::Map *mapbuildingshop=NULL;
    Tiled::Map *mapbuildingheal=NULL;
    Tiled::Map *mapbuilding1=NULL;
    Tiled::Map *mapbuilding2=NULL;
    if(full)
    {
        mapbuildingshop=loadMapTemplate(mapTemplatebuildingshop,"template/building-shop.tmx",mapWidth,mapHeight,worldMap);
        mapbuildingheal=loadMapTemplate(mapTemplatebuildingheal,"template/building-heal.tmx",mapWidth,mapHeight,worldMap);
        mapbuilding1=loadMapTemplate(mapTemplatebuilding1,"template/building-1.tmx",mapWidth,mapHeight,worldMap);
        mapbuilding2=loadMapTemplate(mapTemplatebuilding2,"template/building-2.tmx",mapWidth,mapHeight,worldMap);
    }

    unsigned int index=0;
    while(index<cities.size())
    {
        const City &city=cities.at(index);

        const uint32_t x=city.x;
        const uint32_t y=city.y;
        Tiled::Map *map=NULL;
        MapBrush::MapTemplate mapTemplate;
        std::vector<std::pair<uint8_t,uint8_t> > positionBuilding;
        switch(city.type) {
        case CityType_small:
            map=mapSmall;
            mapTemplate=mapTemplateSmall;
            if(full)
            {
                positionBuilding.push_back(std::pair<uint8_t,uint8_t>(15,15));
                positionBuilding.push_back(std::pair<uint8_t,uint8_t>(24,15));
                positionBuilding.push_back(std::pair<uint8_t,uint8_t>(24,22));
                positionBuilding.push_back(std::pair<uint8_t,uint8_t>(15,22));
            }
            break;
        case CityType_medium:
            map=mapMedium;
            mapTemplate=mapTemplateMedium;
            break;
        default:
            map=mapBig;
            mapTemplate=mapTemplateBig;
            break;
        }
        const uint8_t xoffset=(mapWidth-map->width())/2;
        const uint8_t yoffset=(mapHeight-map->height())/2;
        MapBrush::brushTheMap(worldMap,mapTemplate,x*mapWidth+xoffset,y*mapHeight+yoffset,MapBrush::mapMask,true);
        bool haveHeal=false;
        bool haveShop=false;
        while(!positionBuilding.empty())
        {
            unsigned int randomIndex=rand()%positionBuilding.size();
            std::pair<uint8_t,uint8_t> pos=positionBuilding.at(randomIndex);
            positionBuilding.erase(positionBuilding.cbegin()+randomIndex);
            if(!haveHeal)
            {
                haveHeal=true;
                MapBrush::brushTheMap(worldMap,mapTemplatebuildingheal,x*mapWidth+pos.first,y*mapHeight+pos.second,MapBrush::mapMask,true);
            }
            else if(!haveShop)
            {
                haveShop=true;
                MapBrush::brushTheMap(worldMap,mapTemplatebuildingshop,x*mapWidth+pos.first,y*mapHeight+pos.second,MapBrush::mapMask,true);
            }
            else
            {
                haveHeal=true;
                switch(rand()%2)
                {
                case 0:
                    MapBrush::brushTheMap(worldMap,mapTemplatebuilding1,x*mapWidth+pos.first,y*mapHeight+pos.second,MapBrush::mapMask,true);
                    break;
                default:
                    MapBrush::brushTheMap(worldMap,mapTemplatebuilding2,x*mapWidth+pos.first,y*mapHeight+pos.second,MapBrush::mapMask,true);
                    break;
                }
            }
        }
        index++;
    }

    //do the path
    const unsigned int w=worldMap.width()/mapWidth;
    const unsigned int h=worldMap.height()/mapHeight;
    {
        unsigned int y=0;
        while(y<h)
        {
            unsigned int x=0;
            while(x<w)
            {
                const uint8_t &zoneOrientation=mapPathDirection[x+y*w];
                if(zoneOrientation!=0)
                {
                    if(zoneOrientation&Orientation_bottom)
                    {
                        const unsigned int minX=x*mapWidth+mapWidth/2-2;
                        const unsigned int maxX=x*mapWidth+mapWidth/2+2;
                        unsigned int yTile=y*mapHeight+mapHeight/2;
                        while(yTile<(y+1)*mapHeight)
                        {
                            unsigned int xTile=minX;
                            while(xTile<maxX)
                            {
                                const unsigned int &bitMask=xTile+yTile*worldMap.width();
                                const unsigned int maxMapSize=(worldMap.width()*worldMap.height()/8+1);
                                if(bitMask/8>=maxMapSize)
                                    abort();
                                MapBrush::mapMask[bitMask/8]|=(1<<(7-bitMask%8));
                                xTile++;
                            }
                            yTile++;
                        }
                    }
                    if(zoneOrientation&Orientation_top)
                    {
                        const unsigned int minX=x*mapWidth+mapWidth/2-2;
                        const unsigned int maxX=x*mapWidth+mapWidth/2+2;
                        unsigned int yTile=y*mapHeight;
                        while(yTile<y*mapHeight+mapHeight/2)
                        {
                            unsigned int xTile=minX;
                            while(xTile<maxX)
                            {
                                const unsigned int &bitMask=xTile+yTile*worldMap.width();
                                const unsigned int maxMapSize=(worldMap.width()*worldMap.height()/8+1);
                                if(bitMask/8>=maxMapSize)
                                    abort();
                                MapBrush::mapMask[bitMask/8]|=(1<<(7-bitMask%8));
                                xTile++;
                            }
                            yTile++;
                        }
                    }
                    if(zoneOrientation&Orientation_left)
                    {
                        const unsigned int minX=x*mapWidth;
                        const unsigned int maxX=x*mapWidth+mapWidth/2;
                        unsigned int yTile=y*mapHeight+mapHeight/2-2;
                        while(yTile<y*mapHeight+mapHeight/2+2)
                        {
                            unsigned int xTile=minX;
                            while(xTile<maxX)
                            {
                                const unsigned int &bitMask=xTile+yTile*worldMap.width();
                                const unsigned int maxMapSize=(worldMap.width()*worldMap.height()/8+1);
                                if(bitMask/8>=maxMapSize)
                                    abort();
                                MapBrush::mapMask[bitMask/8]|=(1<<(7-bitMask%8));
                                xTile++;
                            }
                            yTile++;
                        }
                    }
                    if(zoneOrientation&Orientation_right)
                    {
                        const unsigned int minX=x*mapWidth+mapWidth/2;
                        const unsigned int maxX=(x+1)*mapWidth;
                        unsigned int yTile=y*mapHeight+mapHeight/2-2;
                        while(yTile<y*mapHeight+mapHeight/2+2)
                        {
                            unsigned int xTile=minX;
                            while(xTile<maxX)
                            {
                                const unsigned int &bitMask=xTile+yTile*worldMap.width();
                                const unsigned int maxMapSize=(worldMap.width()*worldMap.height()/8+1);
                                if(bitMask/8>=maxMapSize)
                                    abort();
                                MapBrush::mapMask[bitMask/8]|=(1<<(7-bitMask%8));
                                xTile++;
                            }
                            yTile++;
                        }
                    }
                }
                x++;
            }
            y++;
        }
    }
    delete mapBig;
    delete mapMedium;
    delete mapSmall;
    if(full)
    {
        delete mapbuildingshop;
        delete mapbuildingheal;
        delete mapbuilding1;
        delete mapbuilding2;
    }
}

void LoadMapAll::addMapChange(Tiled::Map &worldMap, const unsigned int &mapXCount, const unsigned int &mapYCount)
{
    Tiled::ObjectGroup * const movingLayer=LoadMap::searchObjectGroupByName(worldMap,"Moving");
    const Tiled::Tileset * const invisibleTileset=LoadMap::searchTilesetByName(worldMap,"invisible");

    Tiled::Cell newCell;
    newCell.flippedAntiDiagonally=false;
    newCell.flippedHorizontally=false;
    newCell.flippedVertically=false;
    newCell.tile=invisibleTileset->tileAt(3);

    const unsigned int mapWidth=worldMap.width()/mapXCount;
    const unsigned int mapHeight=worldMap.height()/mapYCount;
    unsigned int y=0;
    while(y<mapYCount)
    {
        unsigned int x=0;
        while(x<mapXCount)
        {
            const uint8_t &zoneOrientation=LoadMapAll::mapPathDirection[x+y*mapXCount];
            if(zoneOrientation!=0)
            {
                QDir mapDir(QFileInfo(QString::fromStdString(LoadMapAll::getMapFile(x,y))).absoluteDir());
                if(zoneOrientation&Orientation_left)
                {
                    Tiled::MapObject* newobject=new Tiled::MapObject("","border-left",QPointF(x*mapWidth,y*mapHeight+mapHeight/2),QSizeF(1,1));
                    const QString nextMap(mapDir.relativeFilePath(QString::fromStdString(LoadMapAll::getMapFile(x-1,y))));
                    newobject->setProperty("map",nextMap);
                    newobject->setCell(newCell);
                    movingLayer->addObject(newobject);
                }
                if(zoneOrientation&Orientation_right)
                {
                    Tiled::MapObject* newobject=new Tiled::MapObject("","border-right",QPointF(x*mapWidth+mapWidth-1,y*mapHeight+mapHeight/2),QSizeF(1,1));
                    const QString nextMap(mapDir.relativeFilePath(QString::fromStdString(LoadMapAll::getMapFile(x+1,y))));
                    newobject->setProperty("map",nextMap);
                    newobject->setCell(newCell);
                    movingLayer->addObject(newobject);
                }
                if(zoneOrientation&Orientation_top)
                {
                    Tiled::MapObject* newobject=new Tiled::MapObject("","border-top",QPointF(x*mapWidth+mapWidth/2,-0.001/*not exact float representation correction*/+y*mapHeight+1),QSizeF(1,1));
                    const QString nextMap(mapDir.relativeFilePath(QString::fromStdString(LoadMapAll::getMapFile(x,y-1))));
                    newobject->setProperty("map",nextMap);
                    newobject->setCell(newCell);
                    movingLayer->addObject(newobject);
                }
                if(zoneOrientation&Orientation_bottom)
                {
                    Tiled::MapObject* newobject=new Tiled::MapObject("","border-bottom",QPointF(x*mapWidth+mapWidth/2,-0.001/*not exact float representation correction*/+y*mapHeight+mapHeight),QSizeF(1,1));
                    const QString nextMap(mapDir.relativeFilePath(QString::fromStdString(LoadMapAll::getMapFile(x,y+1))));
                    newobject->setProperty("map",nextMap);
                    newobject->setCell(newCell);
                    movingLayer->addObject(newobject);
                }
            }
            x++;
        }
        y++;
    }
}

std::string LoadMapAll::getMapFile(const unsigned int &x, const unsigned int &y)
{
    if(haveCityEntry(citiesCoordToIndex,x,y))
    {
        const LoadMapAll::City &city=LoadMapAll::cities.at(LoadMapAll::citiesCoordToIndex.at(x).at(y));
        return QCoreApplication::applicationDirPath().toStdString()+"/dest/main/official/"+LoadMapAll::lowerCase(city.name)+"/"+LoadMapAll::lowerCase(city.name);
    }

    const RoadIndex &roadIndex=roadCoordToIndex.at(x).at(y);
    const LoadMapAll::Road &road=LoadMapAll::roads.at(roadIndex.roadIndex);
    if(road.haveOnlySegmentNearCity)
    {
        if(roadIndex.cityIndex.empty())
        {
            std::cerr << "road.haveOnlySegmentNearCity and roadIndex.cityIndex.empty()" << std::endl;
            abort();
        }
        const LoadMapAll::RoadToCity &cityIndex=roadIndex.cityIndex.front();
        return QCoreApplication::applicationDirPath().toStdString()+"/dest/main/official/"+
                LoadMapAll::lowerCase(LoadMapAll::cities.at(cityIndex.cityIndex).name)+"/road-"+std::to_string(roadIndex.roadIndex+1)+
                "-"+LoadMapAll::orientationToString(LoadMapAll::reverseOrientation(cityIndex.orientation));
    }
    else
    {
        const unsigned int &indexCoord=vectorindexOf(road.coords,std::pair<uint16_t,uint16_t>(x,y));
        return QCoreApplication::applicationDirPath().toStdString()+"/dest/main/official/road-"+
                std::to_string(roadIndex.roadIndex+1)+"/"+std::to_string(indexCoord+1);
    }
}

std::string LoadMapAll::lowerCase(std::string str)
{
    std::transform(str.begin(),str.end(),str.begin(),::tolower);
    return str;
}
