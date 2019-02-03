#include "LoadMap.h"

#include <QDir>
#include <QCoreApplication>
#include <iostream>

#include "../../client/tiled/tiled_mapreader.h"
#include "../../client/tiled/tiled_tileset.h"
#include "../../client/tiled/tiled_objectgroup.h"
#include "../../client/tiled/tiled_mapobject.h"

#include "../../general/base/cpp11addition.h"

LoadMap::Terrain LoadMap::terrainList[5][6];
QStringList LoadMap::terrainFlatList;
QHash<QString,LoadMap::Terrain *> LoadMap::terrainNameToObject;
std::vector<LoadMap::GroupedTerrain> LoadMap::groupedTerrainList;

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
    QDir mapDir(QCoreApplication::applicationDirPath()+"/dest/map/main/official/");

    Tiled::MapReader reader;
    Tiled::Tileset *tilesetBase=reader.readTileset(QCoreApplication::applicationDirPath()+"/dest/map/"+tsx);
    if(tilesetBase==NULL)
    {
        std::cerr << "File not found: " << QCoreApplication::applicationDirPath().toStdString()
                  << "/dest/map/" << tsx.toStdString() << " " << reader.errorString().toStdString() << std::endl;
        abort();
    }
    /*if(tilesetBase->tileWidth()!=tiledMap->tileWidth())
    {
        std::cerr << "tile Width not match with map tile Width" << std::endl;
        abort();
    }
    if(tilesetBase->tileHeight()!=tiledMap->tileHeight())
    {
        std::cerr << "tile height not match with map tile height" << std::endl;
        abort();
    }*/
    tiledMap->addTileset(tilesetBase);
    tilesetBase->setFileName(mapDir.relativeFilePath(tilesetBase->fileName()));
    return tilesetBase;
}

Tiled::Map *LoadMap::readMap(const QString &tmx)
{
    Tiled::MapReader reader;
    Tiled::Map *map=reader.readMap(QCoreApplication::applicationDirPath()+"/"+tmx);
    if(map==NULL)
    {
        std::cerr << "File not found: " << QCoreApplication::applicationDirPath().toStdString() << "/" << tmx.toStdString() <<
                  ": " << reader.errorString().toStdString() << std::endl;
        abort();
    }
    return map;
}

Tiled::Tileset *LoadMap::readTilesetWithTileId(const uint32_t &tile,const QString &tsx,Tiled::Map *tiledMap)
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

void LoadMap::loadAllTileset(QHash<QString,Tiled::Tileset *> &cachedTileset,Tiled::Map &tiledMap)
{
    for(int height=0;height<5;height++)
        for(int moisure=0;moisure<6;moisure++)
        {
            LoadMap::Terrain &terrain=LoadMap::terrainList[height][moisure];
            const QString &layerString=terrain.tmp_layerString;
            const QString &terrainName=terrain.terrainName;
            const QString &tsx=terrain.tmp_tsx;
            const QString &transition_tsx=terrain.tmp_transition_tsx;
            const unsigned int &tileId=terrain.tmp_tileId;
            if(!layerString.isEmpty())
            {
                Tiled::Tileset *tilesetBase;
                if(cachedTileset.contains(tsx))
                    tilesetBase=cachedTileset.value(tsx);
                else
                {
                    tilesetBase=readTilesetWithTileId(tileId,tsx,&tiledMap);
                    cachedTileset[tsx]=tilesetBase;
                }
                terrain.tile=tilesetBase->tileAt(tileId);
                if(terrain.outsideBorder)
                    terrain.tileLayer=searchTileLayerByName(tiledMap,"[T]"+terrainName);
                else
                    terrain.tileLayer=searchTileLayerByName(tiledMap,terrain.tmp_layerString);

                //load the transition tile
                Tiled::Tileset *tilesetTransition;
                if(cachedTileset.contains(transition_tsx))
                    tilesetTransition=cachedTileset.value(transition_tsx);
                else
                {
                    tilesetTransition=readTileset(transition_tsx,&tiledMap);
                    cachedTileset[transition_tsx]=tilesetTransition;
                }
                unsigned int index=0;
                while(index<terrain.tmp_transition_tile.size())
                {
                    terrain.transition_tile.push_back(tilesetTransition->tileAt(terrain.tmp_transition_tile.at(index)));
                    index++;
                }

                LoadMap::terrainNameToObject.insert(terrain.terrainName,&terrain);
                if(!terrainFlatList.contains(terrain.terrainName))
                    terrainFlatList << terrain.terrainName;
            }
        }
    unsigned int groupedTerrainIndex=0;
    while(groupedTerrainIndex<groupedTerrainList.size())
    {
        GroupedTerrain &groupedTerrain=groupedTerrainList[groupedTerrainIndex];

        groupedTerrain.tileLayer=searchTileLayerByName(tiledMap,groupedTerrain.tmp_layerString);

        //load the transition tile
        Tiled::Tileset *tilesetTransition;
        if(cachedTileset.contains(groupedTerrain.tmp_transition_tsx))
            tilesetTransition=cachedTileset.value(groupedTerrain.tmp_transition_tsx);
        else
        {
            tilesetTransition=readTileset(groupedTerrain.tmp_transition_tsx,&tiledMap);
            cachedTileset[groupedTerrain.tmp_transition_tsx]=tilesetTransition;
        }
        unsigned int index=0;
        while(index<groupedTerrain.tmp_transition_tile.size())
        {
            groupedTerrain.transition_tile.push_back(tilesetTransition->tileAt(groupedTerrain.tmp_transition_tile.at(index)));
            index++;
        }

        groupedTerrainIndex++;
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

Tiled::TileLayer *LoadMap::addTerrainLayer(Tiled::Map &tiledMap,const bool dotransition)
{
    (void)dotransition;
    Tiled::TileLayer *layerZoneWater=new Tiled::TileLayer("Water",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneWater);
    Tiled::TileLayer *layerZoneOnWater=new Tiled::TileLayer("OnWater",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneOnWater);
    Tiled::TileLayer *layerZoneWalkable=new Tiled::TileLayer("Walkable",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneWalkable);
    Tiled::TileLayer *layerZoneOnGrass=new Tiled::TileLayer("OnGrass",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneOnGrass);
    Tiled::TileLayer *layerZoneOnGrass2=new Tiled::TileLayer("OnGrass2",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneOnGrass2);
    Tiled::TileLayer *layerZoneOnGrass3=new Tiled::TileLayer("OnGrass3",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneOnGrass3);
    Tiled::TileLayer *layerZoneGrass=new Tiled::TileLayer("Grass",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneGrass);
    Tiled::TileLayer *layerZoneCollisions=new Tiled::TileLayer("Collisions",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneCollisions);
    Tiled::TileLayer *layerZoneWalkBehind=new Tiled::TileLayer("WalkBehind",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneWalkBehind);
    Tiled::TileLayer *layerZoneWalkBehind2=new Tiled::TileLayer("WalkBehind",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneWalkBehind2);
    Tiled::ObjectGroup *layerMoving=new Tiled::ObjectGroup("Moving",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerMoving);

    //add temporary layer
    QSet<QString> terrainLayerNameAlreadySet;
    for(int height=0;height<5;height++)
        for(int moisure=0;moisure<6;moisure++)
        {
            const LoadMap::Terrain &terrain=LoadMap::terrainList[height][moisure];
            const QString &terrainName=terrain.terrainName;
            if(terrain.outsideBorder)
                if(!terrainLayerNameAlreadySet.contains(terrainName))
                {
                    Tiled::TileLayer *layerZoneterrainName=new Tiled::TileLayer("[T]"+terrainName,0,0,tiledMap.width(),tiledMap.height());
                    tiledMap.addLayer(layerZoneterrainName);
                    terrainLayerNameAlreadySet << terrainName;
                }
        }

    //add invisible tileset
    QDir mapDir(QCoreApplication::applicationDirPath()+"/dest/map/main/official/");
    QString tilesetPath(QFileInfo(QCoreApplication::applicationDirPath()+"/dest/map/tileset/invisible.tsx").absoluteFilePath());
    Tiled::MapReader reader;
    Tiled::Tileset *tilesetBase=reader.readTileset(tilesetPath);
    if(tilesetBase==NULL)
    {
        std::cerr << "File not found: " << tilesetPath.toStdString() << std::endl;
        abort();
    }
    tiledMap.addTileset(tilesetBase);
    tilesetBase->setFileName(mapDir.relativeFilePath(tilesetPath));

    return layerZoneWater;
}

void LoadMap::addPolygoneTerrain(std::vector<std::vector<Tiled::ObjectGroup *> > &arrayTerrainPolygon,Tiled::ObjectGroup *layerZoneWaterPolygon,
                        std::vector<std::vector<Tiled::ObjectGroup *> > &arrayTerrainTile,Tiled::ObjectGroup *layerZoneWaterTile,
                        const Grid &grid,
                        const VoronioForTiledMapTmx::PolygonZoneMap &vd,const Simplex &heightmap,const Simplex &moisuremap,
                        const float &noiseMapScaleMoisure,const float &noiseMapScaleMap,
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
            const unsigned int &height=floatToHigh(heightmap.Get({(float)centroid.x()/100,(float)centroid.y()/100},noiseMapScaleMap));
            const unsigned int &moisure=floatToMoisure(moisuremap.Get({(float)centroid.x()/100,(float)centroid.y()/100},noiseMapScaleMoisure));
            if(height==0)
            {
                layerZoneWaterPolygon->addObject(objectPolygon);
                if(!polyTile.isEmpty())
                    layerZoneWaterTile->addObject(objectTile);
            }
            else
            {
                arrayTerrainPolygon[height-1][moisure-1]->addObject(objectPolygon);
                if(!polyTile.isEmpty())
                    arrayTerrainTile[height-1][moisure-1]->addObject(objectTile);
            }
        }
        index++;
    }
}

void LoadMap::addTerrain(const Grid &grid,
                        VoronioForTiledMapTmx::PolygonZoneMap &vd, const Simplex &heightmap, const Simplex &moisuremap, const float &noiseMapScaleMoisure,
                        const float &noiseMapScaleMap,
                        const int widthMap, const int heightMap,
                        const int offsetX, const int offsetY, bool draw)
{
    const QPolygonF polyMap(QRectF(-offsetX,-offsetY,widthMap,heightMap));
    unsigned int index=0;
    while(index<grid.size())
    {
        const Point &centroid=grid.at(index);
        VoronioForTiledMapTmx::PolygonZone &zone=vd.zones[index];

        QPolygonF polyTile;
        polyTile=zone.pixelizedPolygon;
        polyTile=polyTile.intersected(polyMap);

        const QList<QPointF> &edges=polyTile.toList();
        if(!edges.isEmpty())
        {
            //const QPointF &edge=edges.first();
            float xMap=(float)centroid.x();
            float yMap=(float)centroid.y();
            zone.heightFloat=heightmap.Get({xMap/100,yMap/100},noiseMapScaleMap);
            zone.moisureFloat=moisuremap.Get({xMap/100,yMap/100},noiseMapScaleMoisure);
            zone.height=floatToHigh(zone.heightFloat);
            zone.moisure=floatToMoisure(zone.moisureFloat);
            Terrain &terrain=LoadMap::terrainList[zone.height][zone.moisure-1];
            if(draw)
            {
                unsigned int pointIndex=0;
                while(pointIndex<zone.points.size())
                {
                    const Point &point=zone.points.at(pointIndex);
                    Tiled::Cell cell;
                    cell.flippedHorizontally=false;
                    cell.flippedVertically=false;
                    cell.flippedAntiDiagonally=false;
                    cell.tile=terrain.tile;
                    terrain.tileLayer->setCell(point.x(),point.y(),cell);
                    pointIndex++;
                }
            }
        }
        index++;
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

Tiled::ObjectGroup * LoadMap::searchObjectGroupByName(const Tiled::Map &tiledMap,const QString &name)
{
    unsigned int tileLayerIndex=0;
    while(tileLayerIndex<(unsigned int)tiledMap.layerCount())
    {
        Tiled::Layer * const layer=tiledMap.layerAt(tileLayerIndex);
        if(layer->isObjectGroup() && layer->name()==name)
            return static_cast<Tiled::ObjectGroup *>(layer);
        tileLayerIndex++;
    }
    /*std::cerr << "Unable to found layer with name: " << name.toStdString() << std::endl;
    abort();*/
    return NULL;
}

Tiled::Tileset *LoadMap::searchTilesetByName(const Tiled::Map &tiledMap,const QString &name)
{
    unsigned int tilesetIndex=0;
    while(tilesetIndex<(unsigned int)tiledMap.tilesetCount())
    {
        Tiled::Tileset * const layer=tiledMap.tilesetAt(tilesetIndex);
        if(layer->name()==name)
            return layer;
        tilesetIndex++;
    }
    std::cerr << "Unable to found layer with name: " << name.toStdString() << std::endl;
    abort();
}

unsigned int LoadMap::searchTileIndexByName(const Tiled::Map &tiledMap,const QString &name)
{
    unsigned int tileLayerIndex=0;
    while(tileLayerIndex<(unsigned int)tiledMap.layerCount())
    {
        Tiled::Layer * const layer=tiledMap.layerAt(tileLayerIndex);
        if(layer->isTileLayer() && layer->name()==name)
            return tileLayerIndex;
        tileLayerIndex++;
    }
    std::cerr << "Unable to found layer with name: " << name.toStdString() << std::endl;
    abort();
}

bool LoadMap::haveTileLayer(const Tiled::Map &tiledMap,const QString &name)
{
    unsigned int tileLayerIndex=0;
    while(tileLayerIndex<(unsigned int)tiledMap.layerCount())
    {
        Tiled::Layer * const layer=tiledMap.layerAt(tileLayerIndex);
        if(layer->isTileLayer() && layer->name()==name)
            return true;
        tileLayerIndex++;
    }
    return false;
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
    if(tile==NULL)
        abort();
    unsigned int tileLayerIndex=0;
    while(tileLayerIndex<(unsigned int)tiledMap.layerCount())
    {
        Tiled::Layer * const layer=tiledMap.layerAt(tileLayerIndex);
        if(layer->isTileLayer())
        {
            Tiled::TileLayer * const castedLayer=static_cast<Tiled::TileLayer *>(layer);
            if(layer->x()!=0 || layer->y()!=0)
                abort();
            if(x>=(unsigned int)layer->width() || y>=(unsigned int)layer->height())
                abort();
            if(castedLayer->cellAt(x,y).tile==tile)
                return castedLayer;
        }
        tileLayerIndex++;
    }
    return NULL;
}

Tiled::Tile * LoadMap::haveTileAtReturnTile(const Tiled::Map &tiledMap,const unsigned int x,const unsigned int y,const std::vector<Tiled::Tile *> &tiles)
{
    if(vectorcontainsAtLeastOne(tiles,static_cast<Tiled::Tile *>(NULL)))
        abort();
    unsigned int tileLayerIndex=0;
    while(tileLayerIndex<(unsigned int)tiledMap.layerCount())
    {
        Tiled::Layer * const layer=tiledMap.layerAt(tileLayerIndex);
        if(layer->isTileLayer())
        {
            Tiled::TileLayer * const castedLayer=static_cast<Tiled::TileLayer *>(layer);
            if(layer->x()!=0 || layer->y()!=0)
                abort();
            if(x>=(unsigned int)layer->width() || y>=(unsigned int)layer->height())
                abort();
            Tiled::Tile * const tile=castedLayer->cellAt(x,y).tile;
            if(vectorcontainsAtLeastOne(tiles,tile))
                return tile;
        }
        tileLayerIndex++;
    }
    return NULL;
}

Tiled::Tile * LoadMap::haveTileAtReturnTileUniqueLayer(const unsigned int x,const unsigned int y,const std::vector<Tiled::TileLayer *> &tilesLayers,const std::vector<Tiled::Tile *> &tiles)
{
    if(vectorcontainsAtLeastOne(tiles,static_cast<Tiled::Tile *>(NULL)))
        abort();
    unsigned int tileLayerIndex=0;
    while(tileLayerIndex<(unsigned int)tilesLayers.size())
    {
        const Tiled::TileLayer * const layer=tilesLayers.at(tileLayerIndex);
        const Tiled::Tile * const tileToSearch=tiles.at(tileLayerIndex);
        Tiled::Tile * const tile=layer->cellAt(x,y).tile;
        if(tile==tileToSearch)
            return tile;
        tileLayerIndex++;
    }
    return NULL;
}
