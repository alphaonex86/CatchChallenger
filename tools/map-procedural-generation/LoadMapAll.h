#ifndef LOADMAPALL_H
#define LOADMAPALL_H

#include "../map-procedural-generation-terrain/VoronioForTiledMapTmx.h"
#include <libtiled/map.h>
#include <libtiled/mapobject.h>
#include "../../general/base/cpp11addition.hpp"
#include "../map-procedural-generation-terrain/MapBrush.h"
#include "../map-procedural-generation-terrain/znoise/headers/Simplex.hpp"

#include "SettingsAll.h"

#include <unordered_map>
#include <unordered_set>
#include <map>
#include <vector>
#include <string>
#include <cstdint>

class TerrainFlattener;

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
        //element type matched from the surrounding terrain (gym type follows it)
        std::string elementType;
        //visual style group of template/ the filler houses are drawn from
        //("sea-city", "desert-city"...), matched on the surrounding terrain
        std::string style;
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
        //chunk converted to a cave (walled corridor, cave encounters)
        bool isCave;
    };
    //plan of one cave chunk, decided at selection time: the chunk qualifies only
    //when its road connections are SEPARATED by the natural terrain (cliffs) so
    //the cave cannot be bypassed; each side enters through a mouth placed ON the
    //cliff collision line and lands on its own floor (crossing may require going
    //deeper through the stairs)
    struct CavePlanSide
    {
        uint8_t used;//0 = side absent or no mouth found
        uint8_t mouthKind;//1 = cliff faces bottom (entranceTile), 2 = cliff faces top (entranceTopTile)
        //1 = the mouth is a small ROCK OUTCROP standing in the pocket (flat
        //terrain, no cliff near); 0 = the mouth is embedded in the pocket wall
        uint8_t outcrop;
        uint8_t mouthX,mouthY;//overworld mouth, chunk-local tiles (on the cliff collision)
        uint8_t landX,landY;//overworld cell in front of the mouth (exit landing)
        uint8_t floor;//interior floor this side connects to
        uint8_t exitX,exitY;//interior exit cell on the ring line (floor frame)
        uint8_t exitLandX,exitLandY;//interior cell in front of the exit (entry landing)
    };
    struct CavePlan
    {
        uint8_t depth;
        std::vector<std::pair<uint8_t,uint8_t> > stairCells;
        CavePlanSide sides[4];//side order: left,right,top,bottom
    };
    static std::map<std::pair<uint16_t,uint16_t>,CavePlan> cavePlans;
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

    //A building template is a FOLDER of template/: one exterior tmx (the facade
    //brushed on the city ground) plus its floor-N.tmx interiors and their
    //floor-N.xml content skeleton. A group holds one folder per variant
    //("sea-city/1".."sea-city/7"), or the files directly when it has a single
    //variant ("shop-small"). Groups are discovered on disk, never hard-coded.
    struct BuildingVariant
    {
        std::string folder;//"sea-city/1/", under template/
        std::string exterior;//"building-house": the exterior tmx base name
        MapBrush::MapTemplate mapTemplate;
        //door wiring, template-local tiles. doorX/doorY is the doorstep CELL the
        //player stands on (the engine door tile is the collision one above it),
        //spawnX/spawnY where they land inside floor-0.
        unsigned int doorX,doorY;
        unsigned int spawnX,spawnY;
    };
    struct BuildingGroup
    {
        std::string name;
        std::vector<BuildingVariant> variants;
    };
    static std::map<std::string,BuildingGroup> buildingGroups;
    //the discovered "*-city" style groups, in scan order
    static std::vector<std::string> cityStyles;
    static int botId;

    //one inline <bot> definition is emitted per bot, as a direct child of the
    //map's own sibling .xml — that is the ONLY place the engine reads bots from
    //(Map_loader.cpp), matched to the .tmx bot object by its integer id.
    enum BotKind : uint8_t
    {
        BotKind_text=0,
        BotKind_heal=1,
        BotKind_shop=2,
        BotKind_fight=3,
        BotKind_leader=4
    };
    static QString botStepXml(const unsigned int &id, const BotKind &kind, const std::string &name,
                              const QString &lookAt, const SettingsAll::SettingsExtra &setting,
                              const std::vector<RoadMonster> &monsterPool, const uint8_t &level,
                              const std::string &gymTypeName, const std::vector<std::string> &gymTypeMonsters);
    //lowercase monster name when configured ([wildMonsters] <id>\name), else the id
    static QString monsterRef(const uint16_t &monsterId, const SettingsAll::SettingsExtra &setting);
    static bool isCaveChunk(const unsigned int &x, const unsigned int &y);
    //interior map base name of a cave chunk, e.g. "2-cave" (same folder as the chunk)
    static std::string caveInteriorBaseName(const unsigned int &x, const unsigned int &y);
    //paint the cave corridor over the chunk region of the WORLD map and write it as
    //the separate <chunk>-cave.tmx interior (type=cave, encounters + trainers);
    //call AFTER the natural overworld chunk has been saved — the region is consumed
    static bool writeCaveInterior(Tiled::Map &worldMap,
                                  const unsigned int &chunkX, const unsigned int &chunkY,
                                  const unsigned int &singleMapWidth, const unsigned int &singleMapHeight,
                                  const RoadIndex &roadIndex, const SettingsAll::SettingsExtra &setting,
                                  const std::string &overworldFile, const std::string &zoneName);
    //recolor the gym tileset blue parts with each gym type color and write
    //gym-<type>.png/.tsx into dest/map/tileset/ (the lower sprite parts sit exactly
    //128px below their position in the building and act as the recolor mask)
    static void generateGymTilesets(const SettingsAll::SettingsExtra &setting);
    //brush only the building exterior, skipping its door objects: a facade
    //building without content (big city filler)
    static void brushFacade(const MapBrush::MapTemplate &mapTemplate, Tiled::Map &worldMap,
                            const int &tileX, const int &tileY);
    //avenue/plaza ground derived from a city template tmx: Walkable fill tile +
    //OnGrass 3x3 border ring, translated to WORLD tileset cells
    struct CityGround
    {
        Tiled::Cell fill;
        Tiled::Cell border[3][3];//[y][x], center unused
        bool valid;
    };
    static CityGround cityGroundBig;
    static CityGround cityGroundMedium;
    //city sign cells: re-asserted AFTER vegetation so a tree canopy (WalkBehind)
    //or any later decoration can never hide a sign
    struct CitySign
    {
        unsigned int x,y;
        Tiled::Cell cell;
    };
    static std::vector<CitySign> citySigns;
    static void reassertCitySigns(Tiled::Map &worldMap);
    //keep the later vegetation brush off a cell that must stay visible
    static void maskVegetationAround(Tiled::Map &worldMap, const unsigned int &tileX, const unsigned int &tileY,
                                     const int radius);
    //world-tile footprints of the placed city buildings: the avenue/path paint
    //must NEVER overwrite a building tile
    struct BuildingRect
    {
        unsigned int x,y,w,h;
    };
    static std::vector<BuildingRect> cityBuildingRects;

    //Re-seed rand() for a CHUNK-LOCAL pass, from the world seed and the chunk
    //coordinates. Every chunk then draws from its OWN stream: changing how many
    //random numbers one chunk consumes (a new feature, a reordered block) can no
    //longer shift the content of every other chunk of the world, so a diff stays
    //readable and a regression stays local.
    //ONLY for passes whose result depends on that chunk alone. The height /
    //moisure / voronoi noise is global by construction and keeps the single
    //global sequence — reseeding it per chunk would tile the world.
    //pass distinguishes the successive passes over the same chunk, so pass N+1
    //does not replay the numbers pass N already drew.
    enum ChunkPass : uint8_t
    {
        ChunkPass_roadContent=1,//generateRoadContent: zones, buildings, caves plan
        ChunkPass_roadPaint=2,//addRoadContent: ground paint, ledges, bots
        ChunkPass_townsfolk=3,//addCityTownsfolk
        ChunkPass_caveInterior=4,//writeCaveInterior
        ChunkPass_chunkBots=5//emitRoadBotsForChunk
    };
    static void seedChunk(const unsigned int &seed, const unsigned int &chunkX, const unsigned int &chunkY,
                          const ChunkPass &pass);

    static void addDebugCity(Tiled::Map &worldMap, unsigned int mapWidth, unsigned int mapHeight);
    static void addCity(Tiled::Map &worldMap, const Grid &grid, const std::vector<std::string> &citiesNames,
                        const unsigned int &mapXCount, const unsigned int &mapYCount,
                        const unsigned int &maxCityLinks, const unsigned int &cityRadius,
                        const Simplex &levelmap, const float &levelmapscale, const unsigned int &levelmapmin, const unsigned int &levelmapmax);
    //Declare one FLAT zone per city to the terrain shaper: a town cut in half by
    //two terrains looks wrong, so the height/moisure noise of its chunk is forced
    //to the DOMINANT terrain already found there (water excluded, a town is on
    //land) and ramps back to the natural noise outside over [city] flattenFalloff
    //tiles. Every Voronoi zone painting inside the shape is bound to it, so no
    //seam survives. Must run AFTER addCity (it needs the city list) and BEFORE the
    //terrain is drawn.
    static void addCityFlatZones(TerrainFlattener &flattener,const unsigned int worldWidth,const unsigned int worldHeight,
                                 const SettingsAll::SettingsExtra &config);
    static bool haveCityEntryInternal(const std::unordered_map<uint32_t,std::unordered_map<uint32_t,CityInternal *> > &positionsAndIndex,
                              const unsigned int &x, const unsigned int &y);
    static bool haveCityEntry(const std::unordered_map<uint16_t, std::unordered_map<uint16_t, unsigned int> > &positionsAndIndex,
                              const unsigned int &x, const unsigned int &y);
    static bool haveCityPath(const std::unordered_map<uint32_t,std::unordered_map<uint32_t,std::unordered_map<uint32_t,std::unordered_set<uint32_t> > > > &resolvedPath,
                                  const unsigned int &x1, const unsigned int &y1,
                                  const unsigned int &x2, const unsigned int &y2);
    static Orientation reverseOrientation(const Orientation &orientation);
    static std::string orientationToString(const Orientation &orientation);
    static void loadMapTemplate(const char * folderName,MapBrush::MapTemplate &mapTemplate,const QString& fileName,const unsigned int mapWidth,const unsigned int mapHeight,Tiled::Map &worldMap);
    //discover every building template group of template/ and load its variants
    //(exterior + interiors), staging their tilesets into dest/map/tileset/ and
    //wiring the door/exit objects. Called once before the city pass.
    static void scanBuildingTemplates(Tiled::Map &worldMap,const unsigned int mapWidth,const unsigned int mapHeight);
    //Validate the INPUT templates before a single file is written. Only the defects
    //nobody can repair without knowing the author's intent land here; every error is
    //collected so one run reports them all. Returns false when generation must stop.
    static bool precheckTemplates(std::vector<std::string> &errors);
    //NULL when the group does not exist on disk
    static BuildingGroup *buildingGroup(const std::string &name);
    //copy a tileset (tsx + its image) used by a template into dest/map/tileset/
    //so the generated datapack is self contained, and return the world instance
    static Tiled::SharedTileset stageTemplateTileset(Tiled::Map &worldMap,const QString &tsxPath);
    //compute the doorstep/spawn cells and create the missing door (exterior) and
    //exit (floor-0) objects: a template only has to draw the building
    static void wireBuildingDoors(BuildingVariant &variant);
    //<bot> definitions of one interior floor, rebuilt from the template's
    //floor-N.xml SKELETON (bot ids, step ids and step types are kept, every
    //content is regenerated for this city: texts, shop products, fight teams)
    //injectedBots receives the bot objects the generator ADDED to the shared
    //template (the extra gym trainers): the caller removes them once the map is
    //written, so the template stays as the author drew it
    static QString interiorBotXml(const BuildingVariant &variant,const std::string &floorName,
                                  const std::string &destinationFile,
                                  Tiled::Map *floorMap,const BotKind &kind,const City &city,
                                  const SettingsAll::SettingsExtra &setting,
                                  const std::vector<RoadMonster> &monsterPool,const uint8_t &level,
                                  const std::string &gymTypeName,const std::vector<std::string> &gymTypeMonsters,
                                  std::vector<Tiled::MapObject*> &injectedBots);
    //fight step content shared by the gym trainers and the in-house trainers
    static QString fightStepXml(const unsigned int &stepId,const bool &leader,
                                const SettingsAll::SettingsExtra &setting,
                                const std::vector<RoadMonster> &monsterPool,const uint8_t &level,
                                const std::string &gymTypeName,const std::vector<std::string> &gymTypeMonsters,
                                const QString &startText,const QString &winText);
    //write the npc text slots collected during the generation, so npcfill.py can
    //regenerate every line with the local LLM
    static void writeNpcSlots(const SettingsAll::SettingsExtra &setting);
    static void addMapChange(Tiled::Map &worldMap, const unsigned int &mapXCount, const unsigned int &mapYCount);
    static std::string getMapFile(const unsigned int &x, const unsigned int &y);
    static std::string lowerCase(std::string str);
    static void deleteMapList(MapBrush::MapTemplate &mapTemplatebuilding);
    static std::vector<Tiled::MapObject*> getDoorsListAndTp(Tiled::Map * map);
    static void addBuildingChain(const std::string &baseName, const std::string &description, const MapBrush::MapTemplate &mapTemplatebuilding, Tiled::Map &worldMap, const uint32_t &x, const uint32_t &y, const unsigned int mapWidth, const unsigned int mapHeight,
                                 const std::pair<uint8_t,uint8_t> pos, const City &city, const std::string &zone,
                                 const BotKind &botKind, const SettingsAll::SettingsExtra &setting,
                                 const std::vector<RoadMonster> &monsterPool, const uint8_t &level,
                                 const std::string &gymTypeName, const std::vector<std::string> &gymTypeMonsters,
                                 const BuildingVariant &variant);

    /**
     * @brief addRoadContent Populate road between the city
     * @param worldMap The world map
     */
    static void generateRoadContent(Tiled::Map &worldMap, const SettingsAll::SettingsExtra &setting);
    static void addRoadContent(Tiled::Map &worldMap, const SettingsAll::SettingsExtra &setting);
    static void cleanRoadPath(unsigned int *map, unsigned int width, unsigned int height);
    static bool checkPathing(unsigned int * map, unsigned int width, unsigned int height, unsigned int sx, unsigned int sy, unsigned int dx, unsigned int dy);
    static QString emitRoadBotsForChunk(Tiled::Map &worldMap,
                                        const unsigned int &chunkTileX, const unsigned int &chunkTileY,
                                        const unsigned int &singleMapWidth, const unsigned int &singleMapHeight,
                                        const RoadIndex &roadIndex, const SettingsAll::SettingsExtra &setting);
    //scatter a few flavour townsfolk (text NPCs) on the open ground of every city,
    //so a town is not an empty field (owner feedback).
    static void addCityTownsfolk(Tiled::Map &worldMap, const SettingsAll::SettingsExtra &setting,
                                 const unsigned int mapWidth, const unsigned int mapHeight);
    //inline <bot> defs (text) for the city chunk: renumbers the chunk's world bot
    //objects to local ids — the engine reads bots only from the map's own .xml.
    static QString emitCityBotsForChunk(Tiled::Map &worldMap,
                                        const unsigned int &chunkTileX, const unsigned int &chunkTileY,
                                        const unsigned int &singleMapWidth, const unsigned int &singleMapHeight,
                                        const SettingsAll::SettingsExtra &setting);
    static Tiled::Tile* fetchTile(Tiled::Map &worldMap, QString data);
};

#endif // LOADMAPALL_H
