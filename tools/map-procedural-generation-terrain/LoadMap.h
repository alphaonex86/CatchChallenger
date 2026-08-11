#ifndef LOADMAP_H
#define LOADMAP_H

#include <vector>
#include <QString>
#include <QSettings>
#include <unordered_map>
#include <string>

//#include <libtiled/tilelayer.h>
//#include <libtiled/map.h>
//#include "/usr/include/libtiled/tilelayer.h"
#include <libtiled/tilelayer.h>
#include <libtiled/map.h>


#include "VoronioForTiledMapTmx.h"
#include "znoise/headers/Simplex.hpp"

class LoadMap
{
public:
    struct TerrainMonster
    {
        uint16_t monster;
        uint8_t mapweight;
        //uint8_t luckweight;
    };
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
        //monster
        std::map<unsigned int,std::vector<TerrainMonster> > terrainMonsters;
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
    //The map LABEL the world is written under: dest/map/main/<mainCode>/, copied as
    //map/main/<mainCode>/ of a datapack. Resolved once from the settings ("maincode"
    //key) by resolveMainCode(); never hard code it, and never "official".
    static const QString &mainCode();
    //Read "maincode" from settings. Absent, empty, or not a valid datapack label ->
    //fall back on DATAPACK_MAINCODE_GENERATED and WRITE it back, so the next run is
    //explicit. "test" is refused here: that label belongs to the hand made harness
    //maps and a generated world would overwrite them.
    static void resolveMainCode(QSettings &settings);
    //dest/map/main/<mainCode>/: the root of the generated map label
    static QString destMainDir();
    //dest/map/main/<mainCode>/tileset/: the tileset dir SHIPPED with the generated
    //maps, the only one a written reference ever points at
    static QString shippedTilesetDir();
    //the run staging pool holding <fileName> (dest/map/tileset/, then the
    //dest/map/main/tileset/ the settings paths use), empty when neither has it
    static QString pooledTileset(const QString &fileName);
    //Fill the run staging pool (dest/map/tileset/ + dest/map/main/tileset/) with
    //every file of sourceDir, WITHOUT overwriting: whoever staged first wins, so a
    //--datapack copy keeps priority over the tilesets the tool ships itself. This
    //is what makes "clone, cmake, run" work with no argument at all. Returns false
    //on an I/O error (the caller reports and stops), true when sourceDir is absent.
    static bool stageTilesetPool(const QString &sourceDir);
    //Delete from the run staging pool every tileset the generated maps do not
    //reference ([General] cleanTileset). shippedTilesetDir() is the OUTPUT and is
    //never touched here — only the pool dirs the next run would reuse.
    static void cleanTilesetPool();
    //copy a tileset (the tsx and the images it references) into destinationDir
    static bool copyTilesetWithImages(const QString &sourceTsx,const QString &destinationDir);
    //ship a tileset next to the generated maps and return the absolute path of
    //that copy, empty when the source does not exist
    static QString shipTileset(const QString &tsxPath);
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
