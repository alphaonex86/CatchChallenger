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
        //temporary values
        QString tsx;
        uint32_t tileId;
        QString layerString;
        QString terrainName;
    };
    static Terrain terrainList[5][6];
    static QHash<QString,Terrain *> terrainNameToObject;

    struct TerrainTransition
    {
        //before map creation
        bool replace_tile;
        //temporary values
        QString tmp_from_type;
        std::vector<QString> tmp_to_type;
        QString tmp_transition_tsx;
        std::vector<int> tmp_transition_tile;
        QString tmp_collision_tsx;
        std::vector<int> tmp_collision_tile;
        //after map creation
        Tiled::Tile * from_type_tile;
        Tiled::TileLayer * from_type_layer;
        std::vector<Tiled::Tile *> to_type_tile;
        std::vector<Tiled::TileLayer *> to_type_layer;
        std::vector<Tiled::Tile *> transition_tile;
        std::vector<Tiled::Tile *> collision_tile;
    };
    enum ZoneType
    {
        Water,
        SubtropicalDesert,
        Grassland,
        TropicalSeasonalForest,
        TropicalRainForest,
        TemperateDesert,
        TemperateDeciduousForest,
        TemperateRainForest,
        Shrubland,
        Taiga,
        Scorched,
        Bare,
        Tundra,
        Snow
    };

    static unsigned int floatToHigh(const float f);
    static unsigned int floatToMoisure(const float f);
    static Tiled::Tileset *readTileset(const QString &tsx,Tiled::Map *tiledMap);
    static Tiled::Tileset *readTilesetWithTileId(const uint32_t &tile,const QString &tsx,Tiled::Map *tiledMap);
    static Tiled::Map *readMap(const QString &tmx);
    static void loadAllTileset(QHash<QString,Tiled::Tileset *> &cachedTileset,Tiled::Map &tiledMap);
    static Tiled::ObjectGroup *addDebugLayer(Tiled::Map &tiledMap,std::vector<std::vector<Tiled::ObjectGroup *> > &arrayTerrain,bool polygon);
    static ZoneType heightAndMoisureToZoneType(const uint8_t &height,const uint8_t &moisure);
    static Tiled::TileLayer *addTerrainLayer(Tiled::Map &tiledMap);
    static void addPolygoneTerrain(std::vector<std::vector<Tiled::ObjectGroup *> > &arrayTerrainPolygon,Tiled::ObjectGroup *layerZoneWaterPolygon,
                            std::vector<std::vector<Tiled::ObjectGroup *> > &arrayTerrainTile,Tiled::ObjectGroup *layerZoneWaterTile,
                            const Grid &grid,
                            const VoronioForTiledMapTmx::PolygonZoneMap &vd,const Simplex &heighmap,const Simplex &moisuremap,const float &noiseMapScale,
                            const int widthMap,const int heightMap,
                            const int offsetX=0,const int offsetY=0);
    static void addTerrain(const Grid &grid,
                            VoronioForTiledMapTmx::PolygonZoneMap &vd, const Simplex &heighmap, const Simplex &moisuremap, const float &noiseMapScale,
                            const int widthMap, const int heightMap,
                            const int offsetX=0, const int offsetY=0);
    static void load_terrainTransitionList(QHash<QString,Tiled::Tileset *> &cachedTileset,
                                    std::vector<TerrainTransition> &terrainTransitionList,Tiled::Map &tiledMap);
    static Tiled::TileLayer *searchTileLayerByName(const Tiled::Map &tiledMap,const QString &name);
    static std::vector<Tiled::Tile *> getTileAt(const Tiled::Map &tiledMap,const unsigned int x,const unsigned int y);
    static Tiled::TileLayer * haveTileAt(const Tiled::Map &tiledMap,const unsigned int x,const unsigned int y,const Tiled::Tile * const tile);
    static Tiled::Tile * haveTileAtReturnTile(const Tiled::Map &tiledMap,const unsigned int x,const unsigned int y,const std::vector<Tiled::Tile *> &tiles);
    static Tiled::Tile * haveTileAtReturnTileUniqueLayer(const unsigned int x,const unsigned int y,const std::vector<Tiled::TileLayer *> &tilesLayers,const std::vector<Tiled::Tile *> &tiles);
};

#endif // LOADMAP_H