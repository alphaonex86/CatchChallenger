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
#include <set>
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
        //chunk of a WATER PATH: a sea channel walled by rock instead of a road.
        //It rides the same graph as a land road (border teleports, level, wild
        //monsters — which come out of the water terrain band by construction),
        //only the painted content differs.
        bool isWater;
        //that channel is CLOSED and crossed by boat: no swimmable corridor, a
        //push-teleport on the boat tile jumps straight to the other shore
        bool isBoat;
    };
    //the two closed chunks a boat crossing joins, and the cities behind them
    struct BoatCrossing
    {
        uint16_t fromX,fromY;
        uint16_t toX,toY;
    };
    static std::vector<BoatCrossing> boatCrossings;
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
    //template/<group>/how-use.ini — how often an OPTIONAL template is used. Absent
    //file = the group is not spawned on its own (it is picked by name, like
    //heal-small or desert-market).
    //  [use]
    //  mapPercent=10   ; chance this template is used at all on an eligible map
    //  min=1           ; how many copies when it IS used
    //  max=3
    //  terrains=grass,sand   ; optional, empty = any surrounding terrain
    //  cityTypes=big,medium  ; optional, empty = any city size
    struct TemplateUse
    {
        bool valid;//false = no how-use.ini, the group is never spawned on its own
        unsigned int mapPercent;
        unsigned int minCount,maxCount;
        std::vector<std::string> terrains;
        std::vector<std::string> cityTypes;
    };
    struct BuildingGroup
    {
        std::string name;
        std::vector<BuildingVariant> variants;
        TemplateUse use;
    };
    //read template/<folder>/how-use.ini; use.valid stays false when it is absent
    static TemplateUse readTemplateUse(const QString &folderPath);
    //Roll how-use.ini for one map: 0 when the template is not used here, else the
    //number of copies to try. seedText makes it deterministic per map+template.
    static unsigned int templateUseCount(const TemplateUse &use);

    //Which building group plays a ROLE for a town, style FIRST then size:
    //  market+heal: <stem>-market-heal (ONE building for both)
    //            -> <stem>-market      + <stem>-heal
    //            -> <stem>-market-<size> + <stem>-heal-<size>
    //            -> shop-<size>        + heal-<size>
    //  gym:         <stem>-gym         -> gym-building
    //where <stem> is the city style folder without its "-city" suffix
    //("desert-city" -> "desert"). Nothing to declare: the folders are looked up on
    //disk, so dropping template/desert-market/ in is all it takes.
    struct CityBuildingSet
    {
        BuildingGroup *market;
        BuildingGroup *heal;
        //market==heal: ONE building holds both the shop and the heal bot (its
        //floor-N.xml skeleton carries a shop step AND a heal step)
        bool marketHealCombined;
        BuildingGroup *gym;
        //<stem>-special-building, spawned per its how-use.ini; NULL when absent
        BuildingGroup *special;
    };
    static CityBuildingSet cityBuildingSet(const std::string &style, const std::string &sizeSuffix);
    //style stem of a city style folder: "desert-city" -> "desert"
    static std::string cityStyleStem(const std::string &style);
    //cities-types.ini next to the binary: "<city name>=<style folder>" forces the
    //house style of that town (the terrain match is only a default). Loaded once.
    static void loadCityStyleOverrides();
    //the forced style of a city, empty when the file has no line for it
    static std::string cityStyleOverride(const std::string &cityName);
    //template/on-<terrain>/<name>/ : a DECORATION brushed on the terrain it is
    //named after, as often as its OWN how-use.ini says (per variant, not per
    //group). <terrain> is a [terrain] name — on-grass, on-water, on-mountain,
    //on-sand, on-snow... — or "cave" for the floor of a cave interior. It is NOT
    //a building: no door is wired, no interior is written, it is only brushed.
    //Discovered on disk like everything else, nothing to declare.
    struct DecorationVariant
    {
        std::string folder;//"on-grass/flower1"
        MapBrush::MapTemplate mapTemplate;
        TemplateUse use;
    };
    struct DecorationGroup
    {
        std::string terrain;//"grass", "water", "cave"...
        std::vector<DecorationVariant> variants;
    };
    static std::vector<DecorationGroup> decorationGroups;
    static void scanDecorationTemplates(Tiled::Map &worldMap,const unsigned int mapWidth,const unsigned int mapHeight);
    //brush the decorations of every chunk, on the cells whose terrain matches.
    //Runs BEFORE the vegetation so a decoration can mask the trees off itself.
    static void addTerrainDecorations(Tiled::Map &worldMap,const SettingsAll::SettingsExtra &setting);
    //the tiles a [terrain] name paints with, for the terrain match above
    static std::set<Tiled::Tile*> terrainTiles(const std::string &terrainName);

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
    //one avenue ground per city SIZE (index = CityType)
    static CityGround cityGround[3];
    //The HOLE a town is laid out in: the centered part of its chunk that holds the
    //avenue and every building, in chunk-local tiles. Everything outside stays
    //natural terrain and keeps its vegetation, which is what makes a small town
    //read as a town instead of a building lost in an empty field.
    //Pure function of the city size and the settings, so every pass (placement,
    //vegetation mask, debug overlay) agrees without storing anything.
    struct CityHole
    {
        unsigned int x,y,width,height;
    };
    static CityHole cityHole(const CityType &type,const unsigned int &mapWidth,const unsigned int &mapHeight,
                             const SettingsAll::SettingsExtra &setting);
    //Mark every city hole in MapBrush::mapMask, right after the mask is created and
    //BEFORE the terrain transitions run. That one mask drives three passes: the
    //outer-border pass then draws no mountain COLLISION ridge across the town
    //square (which is what refused every building lot), the transition pass leaves
    //the flattened ground alone, and the vegetation pass plants no tree inside.
    //Outside the hole nothing is masked, so the town keeps its natural tree frame.
    static void maskCityHoles(Tiled::Map &worldMap, const SettingsAll::SettingsExtra &setting);
    //tiles of vegetation kept between the hole and the chunk border, always
    //(a town with trees right up against its border reads as a clearing)
    static const unsigned int cityHoleBorderRing=5;
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
        ChunkPass_chunkBots=5,//emitRoadBotsForChunk
        ChunkPass_decoration=6//addTerrainDecorations
    };
    static void seedChunk(const unsigned int &seed, const unsigned int &chunkX, const unsigned int &chunkY,
                          const ChunkPass &pass);

    //Label of a chunk in the debug "Chunk" object layer of all.tmx: the REAL name
    //the chunk is kept under (the city, the road and its step, the cave), not the
    //"x,y" grid coordinates — those are already readable from the polygon.
    //Empty string when no map is written for that chunk.
    static std::string chunkDebugName(const unsigned int &x, const unsigned int &y);

    //WALKABILITY GUARD, run on the finished world before a single map is written.
    //Two things a generated world must never ship:
    // 1) a chunk whose border openings are not all reachable from one another —
    //    the player walks in through one side and cannot leave through the other,
    //    which cuts the world graph in two;
    // 2) a building door the player cannot reach — the doorstep must be in the
    //    BIGGEST walkable component of its chunk, the one the borders open on.
    //Walkable follows the engine: a cell is blocked when ANY layer named
    //Collisions holds a tile (they are OR-merged, Map_loaderMain.cpp); water and
    //ledges are passable (walkOn / one-way jumps).
    //Every problem of the whole world is collected so one run reports them all.
    //Returns false when the world must not be written.
    static bool checkWalkability(Tiled::Map &worldMap, const SettingsAll::SettingsExtra &setting,
                                 std::vector<std::string> &errors);
    //NO ISOLATED MAP: flood the chunk graph — every pair of chunks joined by a
    //border teleport, plus every boat crossing — from the chunk of the first
    //start city. A written map the player can never reach is a broken world, so
    //the ones that are not reachable are reported and nothing is generated.
    //This is what makes "go to any map from any map, walking or from water" a
    //guarantee instead of an intention.
    static bool checkNoIsolatedMap(const SettingsAll::SettingsExtra &setting,
                                   std::vector<std::string> &errors);

    //WATER BODIES: connected components of the Water layer over the whole world.
    //A component holding at least [water] seaMinTiles cells is a SEA — big enough
    //to sail, and the only kind a water path is ever routed on. Anything smaller
    //is a LAKE: a pond in a field must not become a shipping lane.
    struct WaterBody
    {
        unsigned int size;
        bool isSea;
        //bounding box in world tiles, and one cell known to belong to it
        unsigned int minX,minY,maxX,maxY;
        unsigned int seedX,seedY;
    };
    static std::vector<WaterBody> waterBodies;
    //world-sized, waterNoBody on land: which body a tile belongs to
    static std::vector<uint16_t> waterBodyOfTile;
    enum : uint16_t { waterNoBody=0xFFFF };
    static void detectWaterBodies(Tiled::Map &worldMap, const SettingsAll::SettingsExtra &setting);
    //[General] terrainDebug: Object layer "Terrain" with the OUTLINE polygon of
    //every sea and lake, named with its kind and size
    static void addDebugWaterBodies(Tiled::Map &worldMap, const SettingsAll::SettingsExtra &setting);
    //WATER PATHS: sea routes joining coastal towns, registered in the SAME graph
    //as the land roads (mapPathDirection). The chunk grouping, the city links,
    //the levels, the wild monsters (which come out of the WATER terrain band by
    //construction), the border teleports and the minimap then all come for free;
    //only the painted content differs. Called from addCity, right before the road
    //grouping — that is the last moment the graph can still be changed.
    //A route may be the ONLY way to a town: the no-isolated-map check is what
    //guarantees the world stays whole, not the land network.
    static void addWaterPaths(const unsigned int mapXCount,const unsigned int mapYCount,
                              const unsigned int singleMapWidth,const unsigned int singleMapHeight,
                              const unsigned int worldWidth,
                              const SettingsAll::SettingsExtra &setting,
                              std::vector<std::pair<uint16_t,uint16_t> > &waterChunks,
                              std::vector<std::pair<uint16_t,uint16_t> > &boatChunks);
    static void linkChunkToNeighbour(const unsigned int &from,const unsigned int &to,
                                     const unsigned int &mapXCount);
    //Paint ONE sea chunk: a water channel from border to border along its travel
    //axis, walled on both sides by a CONTINUOUS chain of borderTile rock whose
    //position wanders, so the player can cross but never wander off into the open
    //sea. What lies beyond the wall keeps its natural terrain and is simply never
    //reachable. A closed BOAT chunk is walled all round instead, with the boat
    //tile and its push-teleport. Called from addRoadContent, after the terrain
    //transitions, so nothing repaints over it.
    static void paintWaterChunk(Tiled::Map &worldMap,const unsigned int &chunkX,const unsigned int &chunkY,
                                const unsigned int &mapWidth,const unsigned int &mapHeight,
                                const RoadIndex &roadIndex,const uint8_t &zoneOrientation,
                                const SettingsAll::SettingsExtra &setting);

    static void addDebugCity(Tiled::Map &worldMap, unsigned int mapWidth, unsigned int mapHeight);
    //[General] cityDebug: Object layer "City" with the hole polygon of every town
    //and a ONE LINE label (name, size, level, type, style, hole, density, buildings)
    static void addDebugCityLimits(Tiled::Map &worldMap, const SettingsAll::SettingsExtra &setting);
    //buildings really placed in each city, filled by the city pass, read by the
    //debug overlay (index = city index)
    static std::vector<unsigned int> cityBuildingCount;
    static std::vector<unsigned int> cityBuildingArea;
    static void addCity(Tiled::Map &worldMap, const Grid &grid, const std::vector<std::string> &citiesNames,
                        const unsigned int &mapXCount, const unsigned int &mapYCount,
                        const unsigned int &maxCityLinks, const unsigned int &cityRadius,
                        const Simplex &levelmap, const float &levelmapscale, const unsigned int &levelmapmin, const unsigned int &levelmapmax,
                        const SettingsAll::SettingsExtra &setting);
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
