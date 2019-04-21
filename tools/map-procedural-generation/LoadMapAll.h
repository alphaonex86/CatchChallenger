#ifndef LOADMAPALL_H
#define LOADMAPALL_H

#include "../map-procedural-generation-terrain/VoronioForTiledMapTmx.h"
#include "../../client/tiled/tiled_map.h"
#include "../../client/tiled/tiled_mapobject.h"
#include "../../general/base/cpp11addition.h"
#include "../map-procedural-generation-terrain/MapBrush.h"
#include "../map-procedural-generation-terrain/znoise/headers/Simplex.hpp"

#include "SettingsAll.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <cstdint>

class LoadMapAll
{
public:
    enum Orientation : uint8_t
    {
        Orientation_none = 0,//where the target orientation don't matter
        Orientation_top = 1,
        Orientation_right = 2,
        Orientation_bottom = 4,
        Orientation_left = 8
    };
    struct SimplifiedMapForPathFinding
    {
        struct PathToGo
        {
            std::vector<std::pair<Orientation,uint8_t/*step number*/> > left;
            std::vector<std::pair<Orientation,uint8_t/*step number*/> > right;
            std::vector<std::pair<Orientation,uint8_t/*step number*/> > top;
            std::vector<std::pair<Orientation,uint8_t/*step number*/> > bottom;
        };
        std::unordered_map<std::pair<uint16_t,uint16_t>,PathToGo,pairhash> pathToGo;
        std::unordered_set<std::pair<uint16_t,uint16_t>,pairhash> pointQueued;
    };

    struct MapPointToParse
    {
        uint16_t x,y;
    };

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
        std::unordered_map<uint16_t,std::vector<Orientation> > nearRoad;//road number, Orientation
        uint8_t level;
    };
    struct CityInternal
    {
        unsigned int x,y;
        std::string name;
        CityType type;
        std::vector<CityInternal *> citiesNeighbor;
    };
    static std::vector<City> cities;
    static std::unordered_map<uint16_t,std::unordered_map<uint16_t,unsigned int> > citiesCoordToIndex;
    static uint8_t *mapPathDirection;
    static unsigned int **roadData;
    struct Road
    {
        std::vector<std::pair<uint16_t,uint16_t> > coords;
        bool haveOnlySegmentNearCity;
    };
    static std::vector<Road> roads;
    struct RoadToCity
    {
        Orientation orientation;
        unsigned int cityIndex;
    };
    struct RoadMonster
    {
        uint16_t monsterId;
        uint8_t minLevel;
        uint8_t maxLevel;
        uint8_t luck;
    };
    struct RoadBot
    {
        uint8_t id;
        uint8_t look_at;
        uint8_t skin;
        unsigned int x,y;
    };
    struct RoadIndex
    {
        unsigned int roadIndex;
        std::vector<RoadToCity> cityIndex;
        //monster ref
        uint8_t level;//average zone level

        std::vector<RoadMonster> roadMonsters;
        std::vector<RoadBot> roadBot;
    };
    static std::unordered_map<uint16_t,std::unordered_map<uint16_t,RoadIndex> > roadCoordToIndex;
    struct Zone
    {
        std::string name;
    };
    static std::unordered_map<std::string,Zone> zones;

    struct RoadMountain
    {
        QString terrain;
        QString layer;
        QString tile;
        QString tsx;
    };
    static RoadMountain mountain;

    struct RoomSettings
    {
        SettingsAll::Furnitures table;
        SettingsAll::Furnitures exit;
        SettingsAll::Furnitures stairUp;
        SettingsAll::Furnitures stairDown;
        SettingsAll::RoomStructure wall;
        QString floor;
        int id;
        int hasFloorUp;
        int hasFloorDown;
    };
    static int botId;

    static void addDebugCity(Tiled::Map &worldMap, unsigned int mapWidth, unsigned int mapHeight);
    static void addCity(Tiled::Map &worldMap, const Grid &grid, const std::vector<std::string> &citiesNames,
                        const unsigned int &mapXCount, const unsigned int &mapYCount,
                        const unsigned int &maxCityLinks, const unsigned int &cityRadius,
                        const Simplex &levelmap, const float &levelmapscale, const unsigned int &levelmapmin, const unsigned int &levelmapmax,
                        const Simplex &heightmap, const Simplex &moisuremap, const float &noiseMapScaleMoisure,const float &noiseMapScaleMap);
    static bool haveCityEntryInternal(const std::unordered_map<uint32_t,std::unordered_map<uint32_t,CityInternal *> > &positionsAndIndex,
                              const unsigned int &x, const unsigned int &y);
    static bool haveCityEntry(const std::unordered_map<uint16_t, std::unordered_map<uint16_t, unsigned int> > &positionsAndIndex,
                              const unsigned int &x, const unsigned int &y);
    static bool haveCityPath(const std::unordered_map<uint32_t,std::unordered_map<uint32_t,std::unordered_map<uint32_t,std::unordered_set<uint32_t> > > > &resolvedPath,
                                  const unsigned int &x1, const unsigned int &y1,
                                  const unsigned int &x2, const unsigned int &y2);
    static Orientation reverseOrientation(const Orientation &orientation);
    static std::string orientationToString(const Orientation &orientation);
    static void addCityContent(Tiled::Map &worldMap, const unsigned int &mapXCount, const unsigned int &mapYCount, bool full);
    static void loadMapTemplate(const char * folderName,MapBrush::MapTemplate &mapTemplate,const QString& fileName,const unsigned int mapWidth,const unsigned int mapHeight,Tiled::Map &worldMap);
    static void addMapChange(Tiled::Map &worldMap, const unsigned int &mapXCount, const unsigned int &mapYCount);
    static std::string getMapFile(const unsigned int &x, const unsigned int &y);
    static std::string lowerCase(std::string str);
    static void deleteMapList(MapBrush::MapTemplate &mapTemplatebuilding);
    static std::vector<Tiled::MapObject*> getDoorsListAndTp(Tiled::Map * map);
    static void addBuildingChain(const std::string &baseName, const std::string &description, const MapBrush::MapTemplate &mapTemplatebuilding, Tiled::Map &worldMap, const uint32_t &x, const uint32_t &y, const unsigned int mapWidth, const unsigned int mapHeight,
                                 const std::pair<uint8_t,uint8_t> pos, const City &city, const std::string &zone);

    /**
     * @brief addRoadContent Populate road between the city
     * @param worldMap The world map
     */
    static void generateRoadContent(Tiled::Map &worldMap, const SettingsAll::SettingsExtra &setting);
    static void addRoadContent(Tiled::Map &worldMap, const SettingsAll::SettingsExtra &setting);
    static void cleanRoadPath(unsigned int *map, unsigned int width, unsigned int height);
    static bool checkPathing(unsigned int * map, unsigned int width, unsigned int height, unsigned int sx, unsigned int sy, unsigned int dx, unsigned int dy);
    static void writeRoadContent(Tiled::Map &worldMap, const unsigned int &mapXCount, const unsigned int &mapYCount);
    static Tiled::Tile* fetchTile(Tiled::Map &worldMap, QString data);
    static void generateRoom(Tiled::Map& worldMap, const MapBrush::MapTemplate& mapTemplate, const unsigned int id, const uint32_t &x, const uint32_t &y,
                             const std::pair<uint8_t,uint8_t> pos, const City &city, const std::string &zone, const SettingsAll::SettingsExtra &setting, RoomSettings &roomSettings);
    static void generateRoomContent(Tiled::Map& roomMap, const SettingsAll::SettingsExtra &setting, const RoomSettings& roomSettings);
    static void placeRoomFurniture(Tiled::Map& roomMap, const SettingsAll::Furnitures& furnitures, int x, int y);
};

#endif // LOADMAPALL_H
