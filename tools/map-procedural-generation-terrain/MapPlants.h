#ifndef MAPPLANTS_H
#define MAPPLANTS_H

#include <vector>
#include <unordered_map>

#include "../../client/tiled/tiled_map.h"
#include "../../client/tiled/tiled_tileset.h"
#include "VoronioForTiledMapTmx.h"

class MapPlants
{
public:
    struct MapTemplate
    {
        const Tiled::Map * tiledMap;
        std::vector<uint8_t> templateLayerNumberToMapLayerNumber;
        std::unordered_map<Tiled::Tileset *,Tiled::Tileset *> templateTilesetToMapTileset;
        uint8_t width,height,x,y;
        uint8_t baseLayerIndex;
    };

    struct MapPlantsOptions
    {
        QString tmx;
        Tiled::Map *map;
        MapTemplate mapTemplate;
    };

    static MapPlantsOptions mapPlantsOptions[5][6];
    static void loadTypeToMap(std::vector</*heigh*/std::vector</*moisure*/MapTemplate> > &templateResolver,
                       const unsigned int heigh/*heigh, starting at 0*/,
                       const unsigned int moisure/*moisure, starting at 0*/,
                       const MapTemplate &templateMap
                       );
    static bool detectBorder(Tiled::TileLayer *layer,MapTemplate *templateMap);
    static MapTemplate tiledMapToMapTemplate(const Tiled::Map *templateMap,Tiled::Map &worldMap);
    static void brushTheMap(Tiled::Map &worldMap,const MapTemplate &selectedTemplate,const int x,const int y,uint8_t * const mapMask);
    static bool brushHaveCollision(Tiled::Map &worldMap,const MapTemplate &selectedTemplate,const int x,const int y,const uint8_t * const mapMask);
    static void addVegetation(Tiled::Map &worldMap,const VoronioForTiledMapTmx::PolygonZoneMap &vd);
};

#endif // MAPPLANTS_H
