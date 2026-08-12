#include "MapPlants.h"

#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>

#include "LoadMap.h"
#include <libtiled/tilelayer.h>
#include <libtiled/tile.h>

MapPlants::MapPlantsOptions MapPlants::mapPlantsOptions[5][6];
std::vector<MapPlants::PlantInstance> MapPlants::plantInstances;
std::vector<int32_t> MapPlants::plantInstanceOfTile;
std::vector<std::vector<MapBrush::MapTemplate> > MapPlants::plantTemplateResolver;

//mark every world cell a plant wrote as belonging to it, so the cell that blocks
//a corridor can name the whole tree it is part of
static void recordPlantCells(Tiled::Map &worldMap,const MapBrush::MapTemplate &selectedTemplate,
                             const int x,const int y,const int32_t instance)
{
    const Tiled::Map * const brush=selectedTemplate.tiledMap;
    unsigned int layerIndex=0;
    while(layerIndex<(unsigned int)brush->layerCount())
    {
        const Tiled::Layer * const layer=brush->layerAt(layerIndex);
        if(layer->isTileLayer())
        {
            const Tiled::TileLayer * const castedLayer=static_cast<const Tiled::TileLayer *>(layer);
            int index_y=0;
            while(index_y<brush->height())
            {
                const int y_world=index_y-selectedTemplate.y+y;
                if(y_world>=0 && y_world<worldMap.height())
                {
                    int index_x=0;
                    while(index_x<brush->width())
                    {
                        const int x_world=index_x-selectedTemplate.x+x;
                        if(x_world>=0 && x_world<worldMap.width())
                            if(!castedLayer->cellAt(index_x,index_y).isEmpty())
                                MapPlants::plantInstanceOfTile[(unsigned int)x_world
                                        +(unsigned int)y_world*(unsigned int)worldMap.width()]=instance;
                        index_x++;
                    }
                }
                index_y++;
            }
        }
        layerIndex++;
    }
}

bool MapPlants::removePlantAt(Tiled::Map &worldMap,const unsigned int &x,const unsigned int &y)
{
    if(plantInstanceOfTile.size()!=(unsigned int)(worldMap.width()*worldMap.height()))
        return false;
    const int32_t instance=plantInstanceOfTile.at(x+y*(unsigned int)worldMap.width());
    if(instance<0)
        return false;
    const PlantInstance plant=plantInstances.at((unsigned int)instance);
    if(plant.height>=plantTemplateResolver.size())
        return false;
    if(plant.moisure>=plantTemplateResolver.at(plant.height).size())
        return false;
    const MapBrush::MapTemplate &selectedTemplate=plantTemplateResolver.at(plant.height).at(plant.moisure);
    if(selectedTemplate.tiledMap==NULL)
        return false;
    const Tiled::Map * const brush=selectedTemplate.tiledMap;
    unsigned int layerIndex=0;
    while(layerIndex<(unsigned int)brush->layerCount())
    {
        const Tiled::Layer * const layer=brush->layerAt(layerIndex);
        if(layer->isTileLayer())
        {
            const Tiled::TileLayer * const castedLayer=static_cast<const Tiled::TileLayer *>(layer);
            Tiled::TileLayer * const worldLayer=static_cast<Tiled::TileLayer *>(
                        worldMap.layerAt(selectedTemplate.templateLayerNumberToMapLayerNumber.at(layerIndex)));
            int index_y=0;
            while(index_y<brush->height())
            {
                const int y_world=index_y-selectedTemplate.y+(int)plant.y;
                if(y_world>=0 && y_world<worldMap.height())
                {
                    int index_x=0;
                    while(index_x<brush->width())
                    {
                        const int x_world=index_x-selectedTemplate.x+(int)plant.x;
                        if(x_world>=0 && x_world<worldMap.width())
                        {
                            if(!castedLayer->cellAt(index_x,index_y).isEmpty())
                            {
                                worldLayer->setCell(x_world,y_world,Tiled::Cell());
                                plantInstanceOfTile[(unsigned int)x_world
                                        +(unsigned int)y_world*(unsigned int)worldMap.width()]=-1;
                            }
                        }
                        index_x++;
                    }
                }
                index_y++;
            }
        }
        layerIndex++;
    }
    return true;
}

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
    //the resolver is KEPT (plantTemplateResolver): removePlantAt needs the very
    //template a plant was brushed with to erase every cell of it
    std::vector<std::vector</*moisure*/MapBrush::MapTemplate> > &templateResolver=plantTemplateResolver;
    templateResolver.clear();
    plantInstances.clear();
    plantInstanceOfTile.assign((unsigned int)(worldMap.width()*worldMap.height()),-1);
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
                if(layerWalkable->cellAt(x,y).tile()!=NULL)
                {
                    if(layerCollisions->cellAt(x,y).tile()!=NULL)
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
                        {
                            MapBrush::brushTheMap(worldMap,selectedTemplate,x,y,MapBrush::mapMask);
                            PlantInstance plant;
                            plant.x=(uint16_t)x;
                            plant.y=(uint16_t)y;
                            plant.height=(uint8_t)(zone.height-1);
                            plant.moisure=(uint8_t)(zone.moisure-1);
                            plantInstances.push_back(plant);
                            recordPlantCells(worldMap,selectedTemplate,(int)x,(int)y,
                                             (int32_t)(plantInstances.size()-1));
                        }
                    }
                }
            }
            x++;
        }
        y++;
    }
}
