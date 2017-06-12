#include "MapPlants.h"

#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>

#include "LoadMap.h"
#include "../../client/tiled/tiled_tilelayer.h"
#include "../../client/tiled/tiled_tile.h"

MapPlants::MapPlantsOptions MapPlants::mapPlantsOptions[5][6];

void MapPlants::loadTypeToMap(std::vector</*heigh*/std::vector</*moisure*/MapTemplate> > &templateResolver,
                   const unsigned int heigh/*heigh, starting at 0*/,
                   const unsigned int moisure/*moisure, starting at 0*/,
                   const MapTemplate &templateMap
                   )
{
    if(templateResolver.size()<4)
    {
        MapTemplate mapTemplate;
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

bool MapPlants::detectBorder(Tiled::TileLayer *layer,MapTemplate *templateMap)
{
    uint8_t min_x=layer->width(),min_y=layer->height(),max_x=0,max_y=0;
    uint8_t y=0;
    while(y<layer->height())
    {
        uint8_t x=0;
        while(x<layer->width())
        {
            if(!layer->cellAt(x,y).isEmpty())
            {
                if(x<min_x)
                    min_x=x;
                if(y<min_y)
                    min_y=y;
                if(x>max_x)
                    max_x=x;
                if(y>max_y)
                    max_y=y;
            }
            x++;
        }
        y++;
    }
    if(max_x<min_x)
        return false;
    templateMap->width=max_x-min_x+1;
    templateMap->height=max_y-min_y+1;
    templateMap->x=min_x;
    templateMap->y=min_y;
    return true;
}

MapPlants::MapTemplate MapPlants::tiledMapToMapTemplate(const Tiled::Map *templateMap,Tiled::Map &worldMap)
{
    uint8_t lastDimensionLayerUsed=99;
    MapTemplate returnedVar;
    returnedVar.tiledMap=templateMap;
    returnedVar.templateLayerNumberToMapLayerNumber.resize(templateMap->layerCount());
    returnedVar.baseLayerIndex=0;
    //returnedVar.templateTilesetToMapTileset.resize(templateMap->tilesetCount());

    {
        std::unordered_set<uint8_t> alreadyBlockedLayer;
        uint8_t templateLayerIndex=0;
        while(templateLayerIndex<templateMap->layerCount())
        {
            Tiled::Layer * layer=templateMap->layerAt(templateLayerIndex);
            if(layer->isTileLayer())
            {
                Tiled::TileLayer * tileLayer=static_cast<Tiled::TileLayer *>(layer);
                //search into the world map
                uint8_t worldLayerIndex=0;
                while(worldLayerIndex<worldMap.layerCount())
                {
                    Tiled::Layer * layer=worldMap.layerAt(worldLayerIndex);
                    if(layer->isTileLayer())
                    {
                        Tiled::TileLayer * worldTileLayer=static_cast<Tiled::TileLayer *>(layer);
                        //search into the world map
                        if(alreadyBlockedLayer.find(worldLayerIndex)==alreadyBlockedLayer.cend() && worldTileLayer->name()==tileLayer->name())
                        {
                            alreadyBlockedLayer.insert(worldLayerIndex);
                            returnedVar.templateLayerNumberToMapLayerNumber[templateLayerIndex]=worldLayerIndex;
                            break;
                        }
                    }
                    worldLayerIndex++;
                }
                if(worldLayerIndex>=worldMap.layerCount())
                {
                    std::cerr << "a template layer is not found: " << tileLayer->name().toStdString() << " (abort)" << std::endl;
                    abort();
                }
                if(tileLayer->name()=="Collisions")
                {
                    if(detectBorder(tileLayer,&returnedVar))
                    {
                        returnedVar.baseLayerIndex=templateLayerIndex;
                        lastDimensionLayerUsed=0;
                    }
                }
                else if(tileLayer->name()=="Grass" && lastDimensionLayerUsed>0)
                {
                    if(detectBorder(tileLayer,&returnedVar))
                    {
                        returnedVar.baseLayerIndex=templateLayerIndex;
                        lastDimensionLayerUsed=1;
                    }
                }
                else if(tileLayer->name()=="OnGrass" && lastDimensionLayerUsed>1)
                {
                    if(detectBorder(tileLayer,&returnedVar))
                    {
                        returnedVar.baseLayerIndex=templateLayerIndex;
                        lastDimensionLayerUsed=2;
                    }
                }
            }
            templateLayerIndex++;
        }
    }
    {
        uint8_t templateTilesetIndex=0;
        while(templateTilesetIndex<templateMap->tilesetCount())
        {
            Tiled::Tileset * tileset=templateMap->tilesetAt(templateTilesetIndex);
            QFileInfo tilesetFile(tileset->fileName());
            QString tilesetPath=tilesetFile.absoluteFilePath();
            uint8_t templateTilesetWorldIndex=0;
            while(templateTilesetWorldIndex<worldMap.tilesetCount())
            {
                Tiled::Tileset * tilesetWorld=worldMap.tilesetAt(templateTilesetWorldIndex);
                QFileInfo tilesetWorldFile(tilesetWorld->fileName());
                QString tilesetWorldPath=tilesetWorldFile.absoluteFilePath();
                if(tilesetPath==tilesetWorldPath)
                {
                    returnedVar.templateTilesetToMapTileset[tileset]=tilesetWorld;
                    break;
                }
                templateTilesetWorldIndex++;
            }
            if(templateTilesetWorldIndex>=templateMap->tilesetCount())
            {
                QDir templateDir(QCoreApplication::applicationDirPath()+"/dest/template/");
                const QString &tilesetTemplateRelativePath=templateDir.relativeFilePath(tilesetPath);
                returnedVar.templateTilesetToMapTileset[tileset]=LoadMap::readTileset("map/"+tilesetTemplateRelativePath,&worldMap);
            }
            templateTilesetIndex++;
        }
    }
    return returnedVar;
}

//brush down/right to the point
void MapPlants::brushTheMap(Tiled::Map &worldMap,const MapTemplate &selectedTemplate,const int x,const int y,uint8_t * const mapMask)
{
    const Tiled::Map *brush=selectedTemplate.tiledMap;
    unsigned int layerIndex=0;
    while(layerIndex<(unsigned int)brush->layerCount())
    {
        const Tiled::Layer * const layer=brush->layerAt(layerIndex);
        if(layer->isTileLayer())
        {
            const Tiled::TileLayer * const castedLayer=static_cast<const Tiled::TileLayer * const>(layer);
            Tiled::TileLayer * const worldLayer=static_cast<Tiled::TileLayer *>(worldMap.layerAt(selectedTemplate.templateLayerNumberToMapLayerNumber.at(layerIndex)));
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
                        {
                            const Tiled::Cell &cell=castedLayer->cellAt(index_x,index_y);
                            if(!cell.isEmpty())
                            {
                                const unsigned int &tileId=cell.tile->id();
                                Tiled::Tileset *worldTileset=selectedTemplate.templateTilesetToMapTileset.at(cell.tile->tileset());
                                Tiled::Cell cell;
                                cell.flippedHorizontally=false;
                                cell.flippedVertically=false;
                                cell.flippedAntiDiagonally=false;
                                cell.tile=worldTileset->tileAt(tileId);
                                worldLayer->setCell(x_world,y_world,cell);
                                if(layerIndex==selectedTemplate.baseLayerIndex)
                                {
                                    const unsigned int &bitMask=x_world+y_world*worldMap.width();
                                    const unsigned int maxMapSize=(worldMap.width()*worldMap.height()/8+1);
                                    if(bitMask/8>=maxMapSize)
                                        abort();
                                    mapMask[bitMask/8]|=(1<<(7-bitMask%8));
                                }
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
}

//brush down/right to the point
bool MapPlants::brushHaveCollision(Tiled::Map &worldMap,const MapTemplate &selectedTemplate,const int x,const int y,const uint8_t * const mapMask)
{
    const Tiled::Map *brush=selectedTemplate.tiledMap;
    unsigned int layerIndex=0;
    while(layerIndex<(unsigned int)brush->layerCount())
    {
        const Tiled::Layer * const layer=brush->layerAt(layerIndex);
        if(layer->isTileLayer())
        {
            const Tiled::TileLayer * const castedLayer=static_cast<const Tiled::TileLayer * const>(layer);
            Tiled::TileLayer * const worldLayer=static_cast<Tiled::TileLayer *>(worldMap.layerAt(selectedTemplate.templateLayerNumberToMapLayerNumber.at(layerIndex)));
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
                        {
                            const Tiled::Cell &cell=castedLayer->cellAt(index_x,index_y);
                            if(!cell.isEmpty())
                            {
                                if(!worldLayer->cellAt(x_world,y_world).isEmpty())
                                    return false;
                                if(layerIndex==selectedTemplate.baseLayerIndex)
                                {
                                    const unsigned int &bitMask=x_world+y_world*worldMap.width();
                                    if(mapMask[bitMask/8] & (1<<(7-bitMask%8)))
                                        return false;
                                }
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

void MapPlants::addVegetation(Tiled::Map &worldMap,const VoronioForTiledMapTmx::PolygonZoneMap &vd)
{
    std::vector<std::vector</*moisure*/MapTemplate> > templateResolver;
    for(int height=1;height<5;height++)
        for(int moisure=0;moisure<6;moisure++)
        {
            MapPlantsOptions &mapPlantsOption=mapPlantsOptions[height][moisure];
            if(!mapPlantsOption.tmx.isEmpty())
            {
                const Tiled::Map *map=LoadMap::readMap("template/"+mapPlantsOption.tmx+".tmx");
                mapPlantsOption.mapTemplate=tiledMapToMapTemplate(map,worldMap);
                loadTypeToMap(templateResolver,height-1,moisure,mapPlantsOption.mapTemplate);
            }
        }

    const unsigned int mapMaskSize=(worldMap.width()*worldMap.height()/8+1);
    uint8_t mapMask[mapMaskSize];
    memset(mapMask,0x00,mapMaskSize);
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
                const MapTemplate &selectedTemplate=templateResolver.at(zone.height-1).at(zone.moisure-1);
                if(selectedTemplate.tiledMap!=NULL)
                {
                    if(selectedTemplate.width==0 || selectedTemplate.height==0)
                        abort();
                    if(x%selectedTemplate.width==0 && y%selectedTemplate.height==0)
                    {
                        //check if all the collions layer is into the zone
                        bool collionsIsIntoZone=brushHaveCollision(worldMap,selectedTemplate,x,y,mapMask);
                        /*{
                            const Tiled::Map *brush=selectedTemplate.tiledMap;
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
                                        {
                                            const VoronioForTiledMapTmx::PolygonZoneIndex &exploredZoneIndex=vd.tileToPolygonZoneIndex[x_world+y_world*worldMap.height()];
                                            const VoronioForTiledMapTmx::PolygonZone &exploredZone=vd.zones[exploredZoneIndex.index];
                                            if(LoadMap::heightAndMoisureToZoneType(exploredZone.height,exploredZone.moisure)!=LoadMap::heightAndMoisureToZoneType(zone.height,zone.moisure))
                                            {
                                                collionsIsIntoZone=false;
                                                break;
                                            }
                                        }
                                        index_x++;
                                    }
                                    if(index_x<brush->width())
                                        break;
                                }
                                index_y++;
                            }
                        }*/
                        if(collionsIsIntoZone)
                            brushTheMap(worldMap,selectedTemplate,x,y,mapMask);
                    }
                }
            }
            x++;
        }
        y++;
    }
}
