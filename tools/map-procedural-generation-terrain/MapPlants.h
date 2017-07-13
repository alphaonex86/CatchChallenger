#ifndef MAPPLANTS_H
#define MAPPLANTS_H

#include <vector>
#include <unordered_map>

#include "../../client/tiled/tiled_map.h"
#include "../../client/tiled/tiled_tileset.h"
#include "VoronioForTiledMapTmx.h"
#include "MapBrush.h"

class MapPlants
{
public:
    struct MapPlantsOptions
    {
        QString tmx;
        Tiled::Map *map;
        MapBrush::MapTemplate mapTemplate;
    };

    static MapPlantsOptions mapPlantsOptions[5][6];
    static void loadTypeToMap(std::vector</*heigh*/std::vector</*moisure*/MapBrush::MapTemplate> > &templateResolver,
                       const unsigned int heigh/*heigh, starting at 0*/,
                       const unsigned int moisure/*moisure, starting at 0*/,
                       const MapBrush::MapTemplate &templateMap
                       );
    static void addVegetation(Tiled::Map &worldMap,const VoronioForTiledMapTmx::PolygonZoneMap &vd);
};

#endif // MAPPLANTS_H
