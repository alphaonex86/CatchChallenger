#ifndef LOADMAPALL_H
#define LOADMAPALL_H

#include "../map-procedural-generation-terrain/VoronioForTiledMapTmx.h"
#include "../../client/tiled/tiled_map.h"

class LoadMapAll
{
public:
    static void addDebugCity(Tiled::Map &worldMap, unsigned int mapWidth, unsigned int mapHeight);
    static void addCity(const Grid &grid, const std::vector<std::string> &citiesNames);
    enum CityType
    {
        CityType_small,
        CityType_medium,
        CityType_big,
    };
    struct City
    {
        unsigned int x,y;
        std::string name;
        CityType type;
    };
    static std::vector<City> cities;
};

#endif // LOADMAPALL_H
