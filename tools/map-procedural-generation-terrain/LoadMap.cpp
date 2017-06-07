#include "LoadMap.h"

#include <QDir>
#include <QCoreApplication>
#include <iostream>

#include "../../client/tiled/tiled_mapreader.h"
#include "../../client/tiled/tiled_tileset.h"
#include "../../client/tiled/tiled_objectgroup.h"
#include "../../client/tiled/tiled_mapobject.h"

#include "../../general/base/cpp11addition.h"

unsigned int LoadMap::floatToHigh(const float f)
{
    if(f<-0.1)
        return 0;
    else if(f<0.2)
        return 1;
    else if(f<0.4)
        return 2;
    else if(f<0.6)
        return 3;
    else
        return 4;
}

unsigned int LoadMap::floatToMoisure(const float f)
{
    if(f<-0.6)
        return 1;
    else if(f<-0.3)
        return 2;
    else if(f<0.0)
        return 3;
    else if(f<0.3)
        return 4;
    else if(f<0.6)
        return 5;
    else
        return 6;
}

Tiled::Tileset *LoadMap::readTileset(const QString &tsx,Tiled::Map *tiledMap)
{
    QDir mapDir(QCoreApplication::applicationDirPath()+"/dest/map/");

    Tiled::MapReader reader;
    Tiled::Tileset *tilesetBase=reader.readTileset(QCoreApplication::applicationDirPath()+"/dest/"+tsx);
    if(tilesetBase==NULL)
    {
        std::cerr << "File not found: " << QCoreApplication::applicationDirPath().toStdString() << "/" << tsx.toStdString() << std::endl;
        abort();
    }
    if(tilesetBase->tileWidth()!=tiledMap->tileWidth())
    {
        std::cerr << "tile Width not match with map tile Width" << std::endl;
        abort();
    }
    if(tilesetBase->tileHeight()!=tiledMap->tileHeight())
    {
        std::cerr << "tile height not match with map tile height" << std::endl;
        abort();
    }
    tiledMap->addTileset(tilesetBase);
    tilesetBase->setFileName(mapDir.relativeFilePath(tilesetBase->fileName()));
    return tilesetBase;
}

Tiled::Tileset *LoadMap::readTileset(const uint32_t &tile,const QString &tsx,Tiled::Map *tiledMap)
{
    Tiled::Tileset *tilesetBase=readTileset(tsx,tiledMap);
    if(tilesetBase!=NULL)
    {
        if(tile>=(uint32_t)tilesetBase->tileCount())
        {
            std::cerr << "tile: " << tile << " too high number for: " << tsx.toStdString() << std::endl;
            abort();
        }
    }
    return tilesetBase;
}

void LoadMap::loadTileset(Terrain &terrain,QHash<QString,Tiled::Tileset *> &cachedTileset,Tiled::Map &tiledMap)
{
    if(cachedTileset.contains(terrain.tsx))
        terrain.tileset=cachedTileset.value(terrain.tsx);
    else
    {
        Tiled::Tileset *tilesetBase=readTileset(terrain.tileId,terrain.tsx,&tiledMap);
        terrain.tileset=tilesetBase;
        cachedTileset[terrain.tsx]=tilesetBase;
    }
}

Tiled::ObjectGroup *LoadMap::addDebugLayer(Tiled::Map &tiledMap,std::vector<std::vector<Tiled::ObjectGroup *> > &arrayTerrain,bool polygon)
{
    QString addText;
    if(polygon==true)
        addText=" (Polygon)";
    else
        addText=" (Tile)";
    Tiled::ObjectGroup *layerZoneWater=new Tiled::ObjectGroup("WaterZone"+addText,0,0,tiledMap.width(),tiledMap.height());
    layerZoneWater->setColor(QColor("#6273cc"));
    tiledMap.addLayer(layerZoneWater);

    Tiled::ObjectGroup *layerZoneSnow=new Tiled::ObjectGroup("Snow"+addText,0,0,tiledMap.width(),tiledMap.height());
    layerZoneSnow->setColor(QColor("#ffffff"));
    tiledMap.addLayer(layerZoneSnow);
    Tiled::ObjectGroup *layerZoneTundra=new Tiled::ObjectGroup("Tundra"+addText,0,0,tiledMap.width(),tiledMap.height());
    layerZoneTundra->setColor(QColor("#ddddbb"));
    tiledMap.addLayer(layerZoneTundra);
    Tiled::ObjectGroup *layerZoneBare=new Tiled::ObjectGroup("Bare"+addText,0,0,tiledMap.width(),tiledMap.height());
    layerZoneBare->setColor(QColor("#bbbbbb"));
    tiledMap.addLayer(layerZoneBare);
    Tiled::ObjectGroup *layerZoneScorched=new Tiled::ObjectGroup("Scorched"+addText,0,0,tiledMap.width(),tiledMap.height());
    layerZoneScorched->setColor(QColor("#999999"));
    tiledMap.addLayer(layerZoneScorched);
    Tiled::ObjectGroup *layerZoneTaiga=new Tiled::ObjectGroup("Taiga"+addText,0,0,tiledMap.width(),tiledMap.height());
    layerZoneTaiga->setColor(QColor("#ccd4bb"));
    tiledMap.addLayer(layerZoneTaiga);
    Tiled::ObjectGroup *layerZoneShrubland=new Tiled::ObjectGroup("Shrubland"+addText,0,0,tiledMap.width(),tiledMap.height());
    layerZoneShrubland->setColor(QColor("#c4ccbb"));
    tiledMap.addLayer(layerZoneShrubland);
    Tiled::ObjectGroup *layerZoneTemperateDesert=new Tiled::ObjectGroup("Temperate Desert"+addText,0,0,tiledMap.width(),tiledMap.height());
    layerZoneTemperateDesert->setColor(QColor("#e4e8ca"));
    tiledMap.addLayer(layerZoneTemperateDesert);
    Tiled::ObjectGroup *layerZoneTemperateRainForest=new Tiled::ObjectGroup("Temperate Rain Forest"+addText,0,0,tiledMap.width(),tiledMap.height());
    layerZoneTemperateRainForest->setColor(QColor("#a4c4a8"));
    tiledMap.addLayer(layerZoneTemperateRainForest);
    Tiled::ObjectGroup *layerZoneTemperateDeciduousForest=new Tiled::ObjectGroup("Temperate Deciduous Forest"+addText,0,0,tiledMap.width(),tiledMap.height());
    layerZoneTemperateDeciduousForest->setColor(QColor("#b4c9a9"));
    tiledMap.addLayer(layerZoneTemperateDeciduousForest);
    Tiled::ObjectGroup *layerZoneGrassland=new Tiled::ObjectGroup("Grassland"+addText,0,0,tiledMap.width(),tiledMap.height());
    layerZoneGrassland->setColor(QColor("#c4d4aa"));
    tiledMap.addLayer(layerZoneGrassland);
    Tiled::ObjectGroup *layerZoneTropicalRainForest=new Tiled::ObjectGroup("Tropical Rain Forest"+addText,0,0,tiledMap.width(),tiledMap.height());
    layerZoneTropicalRainForest->setColor(QColor("#9cbba9"));
    tiledMap.addLayer(layerZoneTropicalRainForest);
    Tiled::ObjectGroup *layerZoneTropicalSeasonalForest=new Tiled::ObjectGroup("Tropical Seasonal Forest"+addText,0,0,tiledMap.width(),tiledMap.height());
    layerZoneTropicalSeasonalForest->setColor(QColor("#a9cca4"));
    tiledMap.addLayer(layerZoneTropicalSeasonalForest);
    Tiled::ObjectGroup *layerZoneSubtropicalDesert=new Tiled::ObjectGroup("Subtropical Desert"+addText,0,0,tiledMap.width(),tiledMap.height());
    layerZoneSubtropicalDesert->setColor(QColor("#e9ddc7"));
    tiledMap.addLayer(layerZoneSubtropicalDesert);

    arrayTerrain.resize(4);
    //high 1
    arrayTerrain[0].resize(6);
    arrayTerrain[0][0]=layerZoneSubtropicalDesert;//Moisture 1
    arrayTerrain[0][1]=layerZoneGrassland;
    arrayTerrain[0][2]=layerZoneTropicalSeasonalForest;
    arrayTerrain[0][3]=layerZoneTropicalSeasonalForest;
    arrayTerrain[0][4]=layerZoneTropicalRainForest;
    arrayTerrain[0][5]=layerZoneTropicalRainForest;
    //high 2
    arrayTerrain[1].resize(6);
    arrayTerrain[1][0]=layerZoneTemperateDesert;//Moisture 1
    arrayTerrain[1][1]=layerZoneGrassland;
    arrayTerrain[1][2]=layerZoneGrassland;
    arrayTerrain[1][3]=layerZoneTemperateDeciduousForest;
    arrayTerrain[1][4]=layerZoneTemperateDeciduousForest;
    arrayTerrain[1][5]=layerZoneTemperateRainForest;
    //high 3
    arrayTerrain[2].resize(6);
    arrayTerrain[2][0]=layerZoneTemperateDesert;//Moisture 1
    arrayTerrain[2][1]=layerZoneTemperateDesert;
    arrayTerrain[2][2]=layerZoneShrubland;
    arrayTerrain[2][3]=layerZoneShrubland;
    arrayTerrain[2][4]=layerZoneTaiga;
    arrayTerrain[2][5]=layerZoneTaiga;
    //high 4
    arrayTerrain[3].resize(6);
    arrayTerrain[3][0]=layerZoneScorched;//Moisture 1
    arrayTerrain[3][1]=layerZoneBare;
    arrayTerrain[3][2]=layerZoneTundra;
    arrayTerrain[3][3]=layerZoneSnow;
    arrayTerrain[3][4]=layerZoneSnow;
    arrayTerrain[3][5]=layerZoneSnow;

    return layerZoneWater;
}

Tiled::TileLayer *LoadMap::addTerrainLayer(Tiled::Map &tiledMap,std::vector<std::vector<Tiled::TileLayer *> > &arrayTerrain)
{
    Tiled::TileLayer *layerZoneWater=new Tiled::TileLayer("Water",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneWater);
    Tiled::TileLayer *layerZoneWalkable=new Tiled::TileLayer("Walkable",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneWalkable);
    Tiled::TileLayer *layerZoneTransition=new Tiled::TileLayer("Transition",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneTransition);
    Tiled::TileLayer *layerZoneCollisions=new Tiled::TileLayer("Collisions",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneCollisions);

    arrayTerrain.resize(4);
    //high 1
    arrayTerrain[0].resize(6);
    arrayTerrain[0][0]=layerZoneWalkable;//Moisture 1
    arrayTerrain[0][1]=layerZoneWalkable;
    arrayTerrain[0][2]=layerZoneWalkable;
    arrayTerrain[0][3]=layerZoneWalkable;
    arrayTerrain[0][4]=layerZoneWalkable;
    arrayTerrain[0][5]=layerZoneWalkable;
    //high 2
    arrayTerrain[1].resize(6);
    arrayTerrain[1][0]=layerZoneWalkable;//Moisture 1
    arrayTerrain[1][1]=layerZoneWalkable;
    arrayTerrain[1][2]=layerZoneWalkable;
    arrayTerrain[1][3]=layerZoneWalkable;
    arrayTerrain[1][4]=layerZoneWalkable;
    arrayTerrain[1][5]=layerZoneWalkable;
    //high 3
    arrayTerrain[2].resize(6);
    arrayTerrain[2][0]=layerZoneWalkable;//Moisture 1
    arrayTerrain[2][1]=layerZoneWalkable;
    arrayTerrain[2][2]=layerZoneWalkable;
    arrayTerrain[2][3]=layerZoneWalkable;
    arrayTerrain[2][4]=layerZoneWalkable;
    arrayTerrain[2][5]=layerZoneWalkable;
    //high 4
    arrayTerrain[3].resize(6);
    arrayTerrain[3][0]=layerZoneWalkable;//Moisture 1
    arrayTerrain[3][1]=layerZoneWalkable;
    arrayTerrain[3][2]=layerZoneWalkable;
    arrayTerrain[3][3]=layerZoneWalkable;
    arrayTerrain[3][4]=layerZoneWalkable;
    arrayTerrain[3][5]=layerZoneWalkable;

    return layerZoneWater;
}

void LoadMap::addTerrainTile(std::vector<std::vector<Tiled::Tile *> > &arrayTerrainTile,const Terrain &grass,const Terrain &montain)
{
    Tiled::Tile * const grassTile=grass.tileset->tileAt(grass.tileId);
    Tiled::Tile * const montainTile=montain.tileset->tileAt(montain.tileId);

    arrayTerrainTile.resize(4);
    //high 1
    arrayTerrainTile[0].resize(6);
    arrayTerrainTile[0][0]=grassTile;//Moisture 1
    arrayTerrainTile[0][1]=grassTile;
    arrayTerrainTile[0][2]=grassTile;
    arrayTerrainTile[0][3]=grassTile;
    arrayTerrainTile[0][4]=grassTile;
    arrayTerrainTile[0][5]=grassTile;
    //high 2
    arrayTerrainTile[1].resize(6);
    arrayTerrainTile[1][0]=grassTile;//Moisture 1
    arrayTerrainTile[1][1]=grassTile;
    arrayTerrainTile[1][2]=grassTile;
    arrayTerrainTile[1][3]=grassTile;
    arrayTerrainTile[1][4]=grassTile;
    arrayTerrainTile[1][5]=grassTile;
    //high 3
    arrayTerrainTile[2].resize(6);
    arrayTerrainTile[2][0]=grassTile;//Moisture 1
    arrayTerrainTile[2][1]=grassTile;
    arrayTerrainTile[2][2]=grassTile;
    arrayTerrainTile[2][3]=grassTile;
    arrayTerrainTile[2][4]=grassTile;
    arrayTerrainTile[2][5]=grassTile;
    //high 4
    arrayTerrainTile[3].resize(6);
    arrayTerrainTile[3][0]=montainTile;//Moisture 1
    arrayTerrainTile[3][1]=montainTile;
    arrayTerrainTile[3][2]=montainTile;
    arrayTerrainTile[3][3]=montainTile;
    arrayTerrainTile[3][4]=montainTile;
    arrayTerrainTile[3][5]=montainTile;
}

void LoadMap::addPolygoneTerrain(std::vector<std::vector<Tiled::ObjectGroup *> > &arrayTerrainPolygon,Tiled::ObjectGroup *layerZoneWaterPolygon,
                        std::vector<std::vector<Tiled::ObjectGroup *> > &arrayTerrainTile,Tiled::ObjectGroup *layerZoneWaterTile,
                        const Grid &grid,
                        const VoronioForTiledMapTmx::PolygonZoneMap &vd,const Simplex &heighmap,const Simplex &moisuremap,const float &noiseMapScale,
                        const int widthMap,const int heightMap,
                        const int offsetX,const int offsetY)
{
    const QPolygonF polyMap(QRectF(-offsetX,-offsetY,widthMap,heightMap));
    unsigned int index=0;
    while(index<grid.size())
    {
        const Point &centroid=grid.at(index);
        const VoronioForTiledMapTmx::PolygonZone &zone=vd.zones.at(index);

        QPolygonF poly;
        poly=zone.polygon;
        poly=poly.intersected(polyMap);
        Tiled::MapObject *objectPolygon = new Tiled::MapObject("Zone "+QString::number(index),"",QPointF(offsetX,offsetY), QSizeF(0.0,0.0));
        objectPolygon->setPolygon(poly);
        objectPolygon->setShape(Tiled::MapObject::Polygon);

        QPolygonF polyTile;
        polyTile=zone.pixelizedPolygon;
        polyTile=polyTile.intersected(polyMap);
        Tiled::MapObject *objectTile = new Tiled::MapObject("Zone "+QString::number(index),"",QPointF(offsetX,offsetY), QSizeF(0.0,0.0));
        objectTile->setPolygon(polyTile);
        objectTile->setShape(Tiled::MapObject::Polygon);

        const QList<QPointF> &edges=poly.toList();
        if(!edges.isEmpty())
        {
            //const QPointF &edge=edges.first();
            const unsigned int &heigh=floatToHigh(heighmap.Get({(float)centroid.x()/100,(float)centroid.y()/100},noiseMapScale));
            const unsigned int &moisure=floatToMoisure(moisuremap.Get({(float)centroid.x()/100,(float)centroid.y()/100},noiseMapScale*10));
            if(heigh==0)
            {
                layerZoneWaterPolygon->addObject(objectPolygon);
                if(!polyTile.isEmpty())
                    layerZoneWaterTile->addObject(objectTile);
            }
            else
            {
                arrayTerrainPolygon[heigh-1][moisure-1]->addObject(objectPolygon);
                if(!polyTile.isEmpty())
                    arrayTerrainTile[heigh-1][moisure-1]->addObject(objectTile);
            }
        }
        index++;
    }
}

void LoadMap::addTerrain(std::vector<std::vector<Tiled::TileLayer *> > &arrayTerrain,Tiled::TileLayer *layerZoneWater,
                        const Grid &grid,
                        const VoronioForTiledMapTmx::PolygonZoneMap &vd,const Simplex &heighmap,const Simplex &moisuremap,const float &noiseMapScale,
                        const int widthMap,const int heightMap,
                        const Terrain &water,const std::vector<std::vector<Tiled::Tile *> > &arrayTerrainTile,
                        const int offsetX,const int offsetY)
{
    const QPolygonF polyMap(QRectF(-offsetX,-offsetY,widthMap,heightMap));
    unsigned int index=0;
    while(index<grid.size())
    {
        const Point &centroid=grid.at(index);
        const VoronioForTiledMapTmx::PolygonZone &zone=vd.zones.at(index);

        QPolygonF polyTile;
        polyTile=zone.pixelizedPolygon;
        polyTile=polyTile.intersected(polyMap);

        const QList<QPointF> &edges=polyTile.toList();
        if(!edges.isEmpty())
        {
            //const QPointF &edge=edges.first();
            const unsigned int &heigh=floatToHigh(heighmap.Get({(float)centroid.x()/100,(float)centroid.y()/100},noiseMapScale));
            const unsigned int &moisure=floatToMoisure(moisuremap.Get({(float)centroid.x()/100,(float)centroid.y()/100},noiseMapScale*10));
            if(heigh==0)
            {
                unsigned int pointIndex=0;
                while(pointIndex<zone.points.size())
                {
                    const Point &point=zone.points.at(pointIndex);
                    Tiled::Cell cell;
                    cell.flippedHorizontally=false;
                    cell.flippedVertically=false;
                    cell.flippedAntiDiagonally=false;
                    cell.tile=water.tileset->tileAt(water.tileId);
                    layerZoneWater->setCell(point.x(),point.y(),cell);
                    pointIndex++;
                }
            }
            else
            {
                unsigned int pointIndex=0;
                while(pointIndex<zone.points.size())
                {
                    const Point &point=zone.points.at(pointIndex);
                    Tiled::Cell cell;
                    cell.flippedHorizontally=false;
                    cell.flippedVertically=false;
                    cell.flippedAntiDiagonally=false;
                    cell.tile=arrayTerrainTile.at(heigh-1).at(moisure-1);
                    arrayTerrain[heigh-1][moisure-1]->setCell(point.x(),point.y(),cell);
                    pointIndex++;
                }
            }
        }
        index++;
    }
}

unsigned int LoadMap::stringToTerrainInt(const QString &string)
{
    if(string=="water")
        return 0;
    else if(string=="grass")
        return 1;
    else
        return 2;
}

Tiled::Tile * LoadMap::intToTile(const Terrain &grass,const Terrain &water,const Terrain &montain,const unsigned int &terrainInt)
{
    switch(terrainInt)
    {
        case 0:
            return water.tileset->tileAt(water.tileId);
        break;
        case 1:
            return grass.tileset->tileAt(grass.tileId);
        break;
        case 2:
            return montain.tileset->tileAt(montain.tileId);
        break;
    }
    abort();
    return NULL;
}

void LoadMap::load_terrainTransitionList(const Terrain &grass,const Terrain &water,const Terrain &montain,
                                QHash<QString,Tiled::Tileset *> &cachedTileset,
                                std::vector<TerrainTransition> &terrainTransitionList,Tiled::Map &tiledMap)
{
    unsigned int terrainTransitionIndex=0;
    while(terrainTransitionIndex<terrainTransitionList.size())
    {
        //Tiled::Tile * from_type;
        TerrainTransition &terrainTransition=terrainTransitionList.at(terrainTransitionIndex);
        terrainTransition.from_type=intToTile(grass,water,montain,terrainTransition.tmp_from_type);
        unsigned int to_type_Index=0;
        while(to_type_Index<terrainTransition.tmp_to_type.size())
        {
            const unsigned int &to_type=terrainTransition.tmp_to_type.at(to_type_Index);
            terrainTransition.to_type.push_back(intToTile(grass,water,montain,to_type));
            to_type_Index++;
        }
        Tiled::Tileset *tileset;
        if(cachedTileset.contains(terrainTransition.tmp_transition_tsx))
            tileset=cachedTileset.value(terrainTransition.tmp_transition_tsx);
        else
        {
            tileset=readTileset(terrainTransition.tmp_transition_tsx,&tiledMap);
            cachedTileset[terrainTransition.tmp_transition_tsx]=tileset;
        }
        unsigned int transition_tile_index=0;
        while(transition_tile_index<terrainTransition.tmp_transition_tile.size())
        {
            const unsigned int &tileId=terrainTransition.tmp_transition_tile.at(transition_tile_index);
            if(tileId>(unsigned int)tileset->tileCount())
            {
                std::cerr << "tileId greater than tile count, abort(): " << std::to_string(tileId) << std::endl;
                abort();
            }
            terrainTransition.transition_tile.push_back(tileset->tileAt(tileId));
            transition_tile_index++;
        }

        terrainTransitionIndex++;
    }
}

Tiled::TileLayer * LoadMap::searchTileLayerByName(const Tiled::Map &tiledMap,const QString &name)
{
    unsigned int tileLayerIndex=0;
    while(tileLayerIndex<(unsigned int)tiledMap.layerCount())
    {
        Tiled::Layer * const layer=tiledMap.layerAt(tileLayerIndex);
        if(layer->isTileLayer() && layer->name()==name)
            return static_cast<Tiled::TileLayer *>(layer);
        tileLayerIndex++;
    }
    std::cerr << "Unable to found layer with name: " << name.toStdString() << std::endl;
    abort();
}

std::vector<Tiled::Tile *> LoadMap::getTileAt(const Tiled::Map &tiledMap,const unsigned int x,const unsigned int y)
{
    std::vector<Tiled::Tile *> tiles;
    unsigned int tileLayerIndex=0;
    while(tileLayerIndex<(unsigned int)tiledMap.layerCount())
    {
        Tiled::Layer * const layer=tiledMap.layerAt(tileLayerIndex);
        if(layer->isTileLayer())
        {
            if(layer->x()!=0 || layer->y()!=0)
                abort();
            if(x>=(unsigned int)layer->width() || y>=(unsigned int)layer->height())
                abort();
            tiles.push_back(static_cast<Tiled::TileLayer *>(layer)->cellAt(x,y).tile);
        }
        tileLayerIndex++;
    }
    return tiles;
}

Tiled::TileLayer *LoadMap::haveTileAt(const Tiled::Map &tiledMap,const unsigned int x,const unsigned int y,const Tiled::Tile * const tile)
{
    unsigned int tileLayerIndex=0;
    while(tileLayerIndex<(unsigned int)tiledMap.layerCount())
    {
        Tiled::Layer * const layer=tiledMap.layerAt(tileLayerIndex);
        if(layer->isTileLayer())
        {
            if(layer->x()!=0 || layer->y()!=0)
                abort();
            if(x>=(unsigned int)layer->width() || y>=(unsigned int)layer->height())
                abort();
            if(static_cast<Tiled::TileLayer *>(layer)->cellAt(x,y).tile==tile)
                return static_cast<Tiled::TileLayer *>(layer);
        }
        tileLayerIndex++;
    }
    return NULL;
}

Tiled::TileLayer *LoadMap::haveTileAt(const Tiled::Map &tiledMap,const unsigned int x,const unsigned int y,const std::vector<Tiled::Tile *> &tiles)
{
    unsigned int tileLayerIndex=0;
    while(tileLayerIndex<(unsigned int)tiledMap.layerCount())
    {
        Tiled::Layer * const layer=tiledMap.layerAt(tileLayerIndex);
        if(layer->isTileLayer())
        {
            if(layer->x()!=0 || layer->y()!=0)
                abort();
            if(x>=(unsigned int)layer->width() || y>=(unsigned int)layer->height())
                abort();
            if(vectorcontainsAtLeastOne(tiles,static_cast<Tiled::TileLayer *>(layer)->cellAt(x,y).tile))
                return static_cast<Tiled::TileLayer *>(layer);
        }
        tileLayerIndex++;
    }
    return NULL;
}
