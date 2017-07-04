#ifndef LOADMAPALL_H
#define LOADMAPALL_H

#include "../map-procedural-generation-terrain/VoronioForTiledMapTmx.h"
#include "../../client/tiled/tiled_map.h"

#include <unordered_map>
#include <vector>
#include <string>

class LoadMapAll
{
public:
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
    struct CityInternal
    {
        unsigned int x,y;
        std::string name;
        CityType type;
        std::vector<CityInternal *> citiesNeighbor;
    };

    static void addDebugCity(Tiled::Map &worldMap, unsigned int mapWidth, unsigned int mapHeight);
    static void addCity(const Tiled::Map &worldMap, const Grid &grid, const std::vector<std::string> &citiesNames, const unsigned int &w, const unsigned int &h);
    static bool haveCityEntry(const std::unordered_map<uint32_t,std::unordered_map<uint32_t,CityInternal *> > &positionsAndIndex,
                              const unsigned int &x, const unsigned int &y);
    static std::vector<City> cities;
};

#endif // LOADMAPALL_H
