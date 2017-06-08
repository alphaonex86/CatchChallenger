#ifndef LOADMAP_H
#define LOADMAP_H

#include <vector>
#include <QString>
#include <QHash>

#include "../../client/tiled/tiled_tilelayer.h"
#include "../../client/tiled/tiled_map.h"
#include "VoronioForTiledMapTmx.h"
#include "znoise/headers/Simplex.hpp"

class LoadMap
{
public:
    struct Terrain
    {
        QString tsx;
        uint32_t tileId;
        Tiled::Tileset *tileset;
        uint32_t baseX,baseY;
    };

    struct TerrainTransition
    {
        //before map creation
        bool replace_tile;
        //tempory value
        unsigned int tmp_from_type;
        std::vector<unsigned int> tmp_to_type;
        QString tmp_transition_tsx;
        std::vector<int> tmp_transition_tile;
        QString tmp_collision_tsx;
        std::vector<int> tmp_collision_tile;
        //after map creation
        Tiled::Tile * from_type;
        std::vector<Tiled::Tile *> to_type;
        std::vector<Tiled::Tile *> transition_tile;
        std::vector<Tiled::Tile *> collision_tile;
    };
    static unsigned int floatToHigh(const float f);
    static unsigned int floatToMoisure(const float f);
    static Tiled::Tileset *readTileset(const QString &tsx,Tiled::Map *tiledMap);
    static Tiled::Tileset *readTileset(const uint32_t &tile,const QString &tsx,Tiled::Map *tiledMap);
    static Tiled::Map *readMap(const QString &tmx);
    static void loadTileset(Terrain &terrain,QHash<QString,Tiled::Tileset *> &cachedTileset,Tiled::Map &tiledMap);
    static Tiled::ObjectGroup *addDebugLayer(Tiled::Map &tiledMap,std::vector<std::vector<Tiled::ObjectGroup *> > &arrayTerrain,bool polygon);
    static Tiled::TileLayer *addTerrainLayer(Tiled::Map &tiledMap,std::vector<std::vector<Tiled::TileLayer *> > &arrayTerrain);
    static void addTerrainTile(std::vector<std::vector<Tiled::Tile *> > &arrayTerrainTile,const Terrain &grass,const Terrain &montain);
    static void addPolygoneTerrain(std::vector<std::vector<Tiled::ObjectGroup *> > &arrayTerrainPolygon,Tiled::ObjectGroup *layerZoneWaterPolygon,
                            std::vector<std::vector<Tiled::ObjectGroup *> > &arrayTerrainTile,Tiled::ObjectGroup *layerZoneWaterTile,
                            const Grid &grid,
                            const VoronioForTiledMapTmx::PolygonZoneMap &vd,const Simplex &heighmap,const Simplex &moisuremap,const float &noiseMapScale,
                            const int widthMap,const int heightMap,
                            const int offsetX=0,const int offsetY=0);
    static void addTerrain(std::vector<std::vector<Tiled::TileLayer *> > &arrayTerrain, Tiled::TileLayer *layerZoneWater,
                            const Grid &grid,
                            VoronioForTiledMapTmx::PolygonZoneMap &vd, const Simplex &heighmap, const Simplex &moisuremap, const float &noiseMapScale,
                            const int widthMap, const int heightMap,
                            const Terrain &water, const std::vector<std::vector<Tiled::Tile *> > &arrayTerrainTile,
                            const int offsetX=0, const int offsetY=0);
    static unsigned int stringToTerrainInt(const QString &string);
    static Tiled::Tile * intToTile(const Terrain &grass,const Terrain &water,const Terrain &montain,const unsigned int &terrainInt);
    static void load_terrainTransitionList(const Terrain &grass,const Terrain &water,const Terrain &montain,
                                    QHash<QString,Tiled::Tileset *> &cachedTileset,
                                    std::vector<TerrainTransition> &terrainTransitionList,Tiled::Map &tiledMap);
    static Tiled::TileLayer *searchTileLayerByName(const Tiled::Map &tiledMap,const QString &name);
    static std::vector<Tiled::Tile *> getTileAt(const Tiled::Map &tiledMap,const unsigned int x,const unsigned int y);
    static Tiled::TileLayer * haveTileAt(const Tiled::Map &tiledMap,const unsigned int x,const unsigned int y,const Tiled::Tile * const tile);
    static Tiled::Tile * haveTileAtReturnTile(const Tiled::Map &tiledMap,const unsigned int x,const unsigned int y,const std::vector<Tiled::Tile *> &tiles);
};

#endif // LOADMAP_H
