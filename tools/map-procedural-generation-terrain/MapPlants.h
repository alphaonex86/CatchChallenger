#ifndef MAPPLANTS_H
#define MAPPLANTS_H

#include <vector>
#include <unordered_map>

#include <libtiled/map.h>
#include <libtiled/tileset.h>

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

    //ONE plant as it was brushed: where, and with which template. A path opened
    //through the forest AFTER the vegetation is down has to take the WHOLE tree
    //or none of it — clearing the single cell that blocked it left the trunk gone
    //and the canopy hanging in the air. The road generator never has the problem:
    //it masks its way before anything grows there.
    struct PlantInstance
    {
        uint16_t x,y;
        uint8_t height,moisure;
    };
    static std::vector<PlantInstance> plantInstances;
    //which plant covers a world tile, -1 for none (index into plantInstances)
    static std::vector<int32_t> plantInstanceOfTile;
    static std::vector</*heigh*/std::vector</*moisure*/MapBrush::MapTemplate> > plantTemplateResolver;

    static MapPlantsOptions mapPlantsOptions[5][6];
    static void loadTypeToMap(std::vector</*heigh*/std::vector</*moisure*/MapBrush::MapTemplate> > &templateResolver,
                       const unsigned int heigh/*heigh, starting at 0*/,
                       const unsigned int moisure/*moisure, starting at 0*/,
                       const MapBrush::MapTemplate &templateMap
                       );
    static void addVegetation(Tiled::Map &worldMap,const VoronioForTiledMapTmx::PolygonZoneMap &vd);
    //erase the WHOLE plant covering (x,y), every cell of it on every layer it
    //wrote. false when no plant covers that tile.
    static bool removePlantAt(Tiled::Map &worldMap,const unsigned int &x,const unsigned int &y);
};

#endif // MAPPLANTS_H
