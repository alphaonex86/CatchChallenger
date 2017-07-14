#include "LoadMapAll.h"

#include "../map-procedural-generation-terrain/LoadMap.h"
#include "../map-procedural-generation-terrain/MapBrush.h"

#include <iostream>

void LoadMapAll::addCityContent(Tiled::Map &worldMap, const unsigned int &mapXCount, const unsigned int &mapYCount)
{
    if(MapBrush::mapMask==NULL)
    {
        std::cerr << "MapBrush::mapMask==NULL (abort) into LoadMapAll::addCityContent" << std::endl;
        abort();
    }
    const unsigned int mapWidth=worldMap.width()/mapXCount;
    const unsigned int mapHeight=worldMap.height()/mapYCount;
    const Tiled::Map *map=LoadMap::readMap("template/city.tmx");
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
    MapBrush::MapTemplate mapTemplate=MapBrush::tiledMapToMapTemplate(map,worldMap);
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
    unsigned int index=0;
    while(index<cities.size())
    {
        const City &city=cities.at(index);

        const uint32_t x=city.x;
        const uint32_t y=city.y;
        const uint8_t xoffset=(mapWidth-map->width())/2;
        const uint8_t yoffset=(mapHeight-map->height())/2;
        MapBrush::brushTheMap(worldMap,mapTemplate,x*mapWidth+xoffset,y*mapHeight+yoffset,MapBrush::mapMask,true);
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
    delete map;
}
