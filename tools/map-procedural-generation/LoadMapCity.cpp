#include "LoadMapAll.h"

#include "../map-procedural-generation-terrain/LoadMap.h"
#include "../map-procedural-generation-terrain/MapBrush.h"

#include <iostream>

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
