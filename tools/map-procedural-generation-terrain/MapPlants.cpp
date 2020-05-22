#include "MapPlants.h"

#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>

#include "LoadMap.h"
#include "../../client/qt/tiled/tiled_tilelayer.hpp"
#include "../../client/qt/tiled/tiled_tile.hpp"

MapPlants::MapPlantsOptions MapPlants::mapPlantsOptions[5][6];

void MapPlants::loadTypeToMap(std::vector</*heigh*/std::vector</*moisure*/MapBrush::MapTemplate> > &templateResolver,
                   const unsigned int heigh/*heigh, starting at 0*/,
                   const unsigned int moisure/*moisure, starting at 0*/,
                   const MapBrush::MapTemplate &templateMap
                   )
{
    if(templateResolver.size()<4)
    {
        MapBrush::MapTemplate mapTemplate;
        mapTemplate.height=0;
        mapTemplate.width=0;
        mapTemplate.tiledMap=NULL;
        mapTemplate.x=0;
        mapTemplate.y=0;
        templateResolver.resize(4);
        for(int i=0;i<4;i++)
        {
            std::fill(templateResolver[i].begin(),templateResolver[i].end(),mapTemplate);
            templateResolver[i].resize(6);
        }
    }
    if(templateResolver.size()<(heigh+1))
        templateResolver.resize((heigh+1));
    if(templateResolver[heigh].size()<(moisure+1))
        templateResolver[heigh].resize((moisure+1));
    templateResolver[heigh][moisure]=templateMap;
}

void MapPlants::addVegetation(Tiled::Map &worldMap,const VoronioForTiledMapTmx::PolygonZoneMap &vd)
{
    if(MapBrush::mapMask==NULL)
    {
        std::cerr << "MapBrush::mapMask==NULL (abort) into MapPlants::addVegetation" << std::endl;
        abort();
    }
    std::vector<std::vector</*moisure*/MapBrush::MapTemplate> > templateResolver;
    for(int height=1;height<5;height++)
        for(int moisure=0;moisure<6;moisure++)
        {
            MapPlantsOptions &mapPlantsOption=mapPlantsOptions[height][moisure];
            if(!mapPlantsOption.tmx.isEmpty())
            {
                Tiled::Map *map=LoadMap::readMap("template/"+mapPlantsOption.tmx+".tmx");
                mapPlantsOption.mapTemplate=MapBrush::tiledMapToMapTemplate(map,worldMap);
                loadTypeToMap(templateResolver,height-1,moisure,mapPlantsOption.mapTemplate);
            }
        }

    Tiled::TileLayer * layerWalkable=LoadMap::searchTileLayerByName(worldMap,"Walkable");
    Tiled::TileLayer * layerCollisions=LoadMap::searchTileLayerByName(worldMap,"Collisions");
    {
        unsigned int y=0;
        while(y<(unsigned int)worldMap.height())
        {
            unsigned int x=0;
            while(x<(unsigned int)worldMap.width())
            {
                //unmask the zone walkable and not collision
                if(layerWalkable->cellAt(x,y).tile!=NULL)
                {
                    if(layerCollisions->cellAt(x,y).tile!=NULL)
                    {
                        const unsigned int &bitMask=x+y*worldMap.width();
                        const unsigned int maxMapSize=(worldMap.width()*worldMap.height()/8+1);
                        if(bitMask/8>=maxMapSize)
                            abort();
                        MapBrush::mapMask[bitMask/8]|=(1<<(7-bitMask%8));
                    }
                }
                else
                {
                    const unsigned int &bitMask=x+y*worldMap.width();
                    const unsigned int maxMapSize=(worldMap.width()*worldMap.height()/8+1);
                    if(bitMask/8>=maxMapSize)
                        abort();
                    MapBrush::mapMask[bitMask/8]|=(1<<(7-bitMask%8));
                }
                x++;
            }
            y++;
        }
    }
    unsigned int y=0;
    while(y<(unsigned int)worldMap.height())
    {
        unsigned int x=0;
        while(x<(unsigned int)worldMap.width())
        {
            //resolve into zone
            const VoronioForTiledMapTmx::PolygonZoneIndex &zoneIndex=vd.tileToPolygonZoneIndex[x+y*worldMap.width()];
            const VoronioForTiledMapTmx::PolygonZone &zone=vd.zones[zoneIndex.index];
            //resolve into MapTemplate
            if(zone.height>0)
            {
                const MapBrush::MapTemplate &selectedTemplate=templateResolver.at(zone.height-1).at(zone.moisure-1);
                if(selectedTemplate.tiledMap!=NULL)
                {
                    if(selectedTemplate.width==0 || selectedTemplate.height==0)
                        abort();
                    if(x%selectedTemplate.width==0 && y%selectedTemplate.height==0)
                    {
                        //check if all the collions layer is into the zone
                        const bool collionsIsIntoZone=MapBrush::brushHaveCollision(worldMap,selectedTemplate,x,y,MapBrush::mapMask);
                        if(collionsIsIntoZone)
                            MapBrush::brushTheMap(worldMap,selectedTemplate,x,y,MapBrush::mapMask);
                    }
                }
            }
            x++;
        }
        y++;
    }
}
