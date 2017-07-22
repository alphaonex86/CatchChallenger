#ifndef LOADMAP_H
#define LOADMAP_H

#include <vector>
#include <QString>
#include <unordered_map>
#include <string>

#include "../../client/tiled/tiled_tilelayer.h"
#include "../../client/tiled/tiled_map.h"
#include "VoronioForTiledMapTmx.h"
#include "znoise/headers/Simplex.hpp"

class LoadMap
{
public:
    struct Terrain
    {
        //final values
        Tiled::Tile *tile;
        Tiled::TileLayer *tileLayer;
        bool outsideBorder;//if true, the border is out of the zone else use OnGrass layer
        std::vector<Tiled::Tile *> transition_tile;
        QString terrainName;
        //temporary values
        QString tmp_tsx;
        uint32_t tmp_tileId;
        QString tmp_layerString;
        std::vector<uint32_t> tmp_transition_tile;
        QString tmp_transition_tsx;
    };
    static Terrain terrainList[5][6];
    static QStringList terrainFlatList;
    static QHash<QString,Terrain *> terrainNameToObject;
    struct GroupedTerrain
    {
        uint8_t height;
        Tiled::TileLayer *tileLayer;
        std::vector<Tiled::Tile *> transition_tile;
        //temporary values
        QString tmp_layerString;
        std::vector<uint32_t> tmp_transition_tile;
        QString tmp_transition_tsx;
    };
    static std::vector<GroupedTerrain> groupedTerrainList;

    static unsigned int floatToHigh(const float f);
    static unsigned int floatToMoisure(const float f);
    static Tiled::Tileset *readTileset(const QString &tsx,Tiled::Map *tiledMap);
    static Tiled::Tileset *readTilesetWithTileId(const uint32_t &tile,const QString &tsx,Tiled::Map *tiledMap);
    static Tiled::Map *readMap(const QString &tmx);
    static void loadAllTileset(QHash<QString,Tiled::Tileset *> &cachedTileset,Tiled::Map &tiledMap);
    static Tiled::ObjectGroup *addDebugLayer(Tiled::Map &tiledMap,std::vector<std::vector<Tiled::ObjectGroup *> > &arrayTerrain,bool polygon);
    //static ZoneType heightAndMoisureToZoneType(const uint8_t &height,const uint8_t &moisure);
    static Tiled::TileLayer *addTerrainLayer(Tiled::Map &tiledMap, const bool dotransition);
    static void addPolygoneTerrain(std::vector<std::vector<Tiled::ObjectGroup *> > &arrayTerrainPolygon, Tiled::ObjectGroup *layerZoneWaterPolygon,
                            std::vector<std::vector<Tiled::ObjectGroup *> > &arrayTerrainTile, Tiled::ObjectGroup *layerZoneWaterTile,
                            const Grid &grid,
                            const VoronioForTiledMapTmx::PolygonZoneMap &vd, const Simplex &heighmap, const Simplex &moisuremap,
                            const float &noiseMapScaleMoisure, const float &noiseMapScaleMap,
                            const int widthMap, const int heightMap,
                            const int offsetX=0, const int offsetY=0);
    static void addTerrain(const Grid &grid,
                            VoronioForTiledMapTmx::PolygonZoneMap &vd, const Simplex &heighmap, const Simplex &moisuremap,
                            const float &noiseMapScaleHeat,const float &noiseMapScaleMap,
                            const int widthMap, const int heightMap,
                            const int offsetX=0, const int offsetY=0, bool draw=true);
    static Tiled::TileLayer *searchTileLayerByName(const Tiled::Map &tiledMap,const QString &name);
    static Tiled::ObjectGroup *searchObjectGroupByName(const Tiled::Map &tiledMap,const QString &name);
    static Tiled::Tileset *searchTilesetByName(const Tiled::Map &tiledMap,const QString &name);
    static unsigned int searchTileIndexByName(const Tiled::Map &tiledMap,const QString &name);
    static bool haveTileLayer(const Tiled::Map &tiledMap,const QString &name);
    static std::vector<Tiled::Tile *> getTileAt(const Tiled::Map &tiledMap,const unsigned int x,const unsigned int y);
    static Tiled::TileLayer * haveTileAt(const Tiled::Map &tiledMap,const unsigned int x,const unsigned int y,const Tiled::Tile * const tile);
    static Tiled::Tile * haveTileAtReturnTile(const Tiled::Map &tiledMap,const unsigned int x,const unsigned int y,const std::vector<Tiled::Tile *> &tiles);
    static Tiled::Tile * haveTileAtReturnTileUniqueLayer(const unsigned int x,const unsigned int y,const std::vector<Tiled::TileLayer *> &tilesLayers,const std::vector<Tiled::Tile *> &tiles);
};

#endif // LOADMAP_H
