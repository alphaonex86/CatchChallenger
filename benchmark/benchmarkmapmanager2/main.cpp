// HEADLESS: yes
// Benchmark: MapVisibilityAlgorithm::min_network() over the REAL WORLD -- the
//            datapack's generated map set -- as opposed to benchmarkmapmanager,
//            which parks every player on ONE synthetic map at a position it
//            never changes and only rotates directions.
//
// What this one does differently, and why:
//   * The world is the DATAPACK. Every .tmx under <datapack>/map/main/generated
//     (647 real maps in the current set: towns, routes, shop/gym/house
//     interiors) is loaded with the PRODUCTION loader, general/base/
//     Map_loader.cpp, and every one of them is broadcast every tick -- which is
//     exactly what the server's timer does over flat_map_list (see
//     MapVisibilityAlgorithm_WithoutSender.cpp). Ticking hundreds of quiet maps
//     is a real part of the per-tick budget, and it is measured here.
//   * Only what the server keeps hot is kept: width, height and
//     flat_simplified_map per map (673 KB for the whole world). The tile
//     layers, bots, teleporters and parsed XML documents the loader builds on
//     the way are released as each map is read.
//   * Players are spread by kind -- 60% of them out on the routes, 30% in town,
//     10% indoors -- and crowded to the owner's target of 35 per route, 200 per
//     town map, 20 per interior. That decides how many maps are POPULATED; the
//     rest of the world is still there, still ticked, just quiet.
//   * Players WALK, in runs. An idle player picks a direction and a length, the
//     movement VECTOR is truncated at the first obstacle, and it then walks
//     that many cells, one per tick, carrying Direction_move_at_*; when the run
//     ends it stands with Direction_look_at_*. A vector truncated to zero means
//     it faced a wall: it turns in place and does not move. The obstacle test
//     is the PRODUCTION predicate itself (MoveOnTheMap::isWalkableWithDirection
//     -- collisions, one-way ledges, map border), NOT a copy of it, so the walk
//     is bounded by exactly what the game allows.
//   * NO PATHFINDING, and the collision scan happens ONCE PER RUN (at most
//     RUN_LEN_MAX cells), never per tick: a walking player costs a coordinate
//     update and nothing else. median_prep_ns reports that cost every run, so
//     "the harness is not what is being measured" stays checked, not assumed.
//   * A map change replaces the insert+remove pair: a migrating player is
//     removeOnMap()'d from its map and insertOnMap()'d on another, which is
//     what makes the next broadcast take PATH 1 (drop-all + full re-insert) for
//     it and emit a 0x69 remove to its former map.
//
// THE WORKLOAD IS FIXED -- see WORLD[] and the WALK/MIGRATE constants below,
// and the map set itself. There is no flag to change the world shape or the
// rates: a benchmark with knobs is not comparable with its own history. The
// only arguments are WHERE the datapack is, which of the fixed player counts
// to run, and how the run is bounded.
//
// Emitted on stdout -- one WORLD line per process, one BENCH line per scenario:
//   WORLD maps=N failed=N load_ms=N cells=N walkable_cells=N maps_<kind>=N
//     The world that was loaded. Recorded with the run so a regenerated or
//     swapped datapack shows up as a changed world instead of silently
//     shifting every timing below.
//   BENCH players=N maps=N maps_populated=N ticks=T ticks_per_s=X
//         median_tick_ns=Y p95_tick_ns=Z median_prep_ns=P bytes_sent=B
//         visibility_state_bytes=V tick_<kind>_ns=K ... walk_violations=0
//   ticks_per_s               higher-is-better (fixed-TIME throughput)
//   median_tick_ns/p95_tick_ns lower-is-better: one tick = the WHOLE world
//   tick_<kind>_ns            lower-is-better: the same tick split per map
//                             kind, so the crowded-town diff and the per-map
//                             constant of the quiet interiors can regress
//                             separately instead of hiding in one total
//   median_prep_ns            lower-is-better (harness cost per tick; NOT
//                             server work and NOT inside the latency window)
//   bytes_sent                lower-is-better (what min_network exists to cut)
//   visibility_state_bytes    lower-is-better (resident per-map diff state)
//   sampled_changed/sampled_slots  the share of slots that differ from the
//                             previous broadcast, measured on the sampled
//                             ticks. This is the workload's most important
//                             property: what min_network's stateful diff gets
//                             to SKIP. Descriptive, not better-lower.
//   walk_violations           MUST be 0 -- the end-of-scenario oracle check
//                             that every player stands where the production
//                             predicate says it may. Anything else is a FAIL.
//   The remaining fields echo the fixed workload the row was measured under.
//
// median_prep_ns is ~17% of median_tick_ns: the load model costs a few ns per
// player per tick and is EXCLUDED from the latency window (only ticks_per_s,
// which times the whole loop, carries it). For scale, a real server spends
// ~2.6 us of parse+apply per received move, so this walk model is orders of
// magnitude cheaper than the client work it stands in for -- the run measures
// min_network, not the movement simulation.
//
// Determinism: seeded LCG only -- no rand(), no clock, no time in the workload,
//              and the map list is sorted before use (readdir order is not
//              stable across machines and the map index decides where players
//              land). Same binary + same datapack => same world, same walk,
//              same migrations on every arch/libc/compiler. The steady_clock
//              reads are the only nondeterminism and never feed the workload.

#include "../../test/testingmapmanagement/Stubs.hpp"
#include "../../server/base/MapManagement/MapVisibilityAlgorithm.hpp"
// The world is the REAL datapack, loaded with the PRODUCTION loader: same
// parser, same flat_simplified_map, same walkability predicate as the game.
#include "../../general/base/Map_loader.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/MoveOnTheMap.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

using namespace CatchChallenger;

// Deterministic LCG (Numerical Recipes constants). Identical sequence for the
// same seed across every arch / libc / compiler.
struct Lcg
{
    uint32_t state;
    explicit Lcg(uint32_t s) : state(s ? s : 1) {}
    uint32_t next() { state = state * 1664525u + 1013904223u; return state; }

    // Range draw by MULTIPLY-SHIFT, never `% n`. Two reasons, both of which
    // bit this benchmark:
    //  - a power-of-two modulus keeps only the LOW bits, and in an LCG the low
    //    bits are the bad ones: with these constants `next() & 3` walks
    //    0,3,2,1,0,3,2,1... with period 4, so a direction drawn that way is a
    //    rotation, not a choice. Scaling by the whole word uses the HIGH bits.
    //  - `%` is an integer division: the tick loop runs it once per player per
    //    tick on i486/MIPS-class targets, and that is harness cost sitting on
    //    top of the server work being measured.
    uint32_t below(uint32_t n) { return n ? (uint32_t)(((uint64_t)next() * n) >> 32) : 0; }
    // 0..3, from the top two bits.
    uint8_t  pick4() { return (uint8_t)(next() >> 30); }
};

// Percent constant -> 32-bit threshold, so a per-tick draw is `rng.next() <
// thr` (one compare) rather than a division.
static uint32_t pct_threshold(unsigned int pct)
{
    if(pct >= 100) return 0xffffffffu;
    return (uint32_t)(((uint64_t)pct << 32) / 100u);
}

class HarnessMVA : public MapVisibilityAlgorithm
{
public:
    using MapVisibilityAlgorithm::map_clients_id;
    using MapVisibilityAlgorithm::map_removed_index;
};

// ---------------------------------------------------------------------------
// Map kinds
// ---------------------------------------------------------------------------
// Three kinds is what a datapack actually looks like: routes/fields (big,
// mostly open, thinly populated), towns (medium, dense buildings, the crowd),
// shop/gym interiors (tiny, walled, a handful of people). A fourth kind would
// only interpolate between these three.
enum MapKind : uint8_t
{
    Kind_outdoor = 0,
    Kind_city    = 1,
    Kind_indoor  = 2,
    Kind_count   = 3
};

static const char *kind_name(unsigned int k)
{
    if(k == Kind_outdoor) return "outdoor";
    if(k == Kind_city)    return "city";
    return "indoor";
}

struct KindCfg
{
    unsigned int weight;      // share of the population (relative, not %)
    unsigned int occupancy;   // target players per map of this kind
};

// ---------------------------------------------------------------------------
// THE WORKLOAD IS FIXED. Not a knob, not a flag, not an environment variable:
// a benchmark whose load can be changed from the command line stops being
// comparable with its own history the moment anybody passes a flag. Every
// number below is a constant of this benchmark; changing one invalidates
// champion.json and the recorded timeline, so it is a deliberate re-baseline,
// never a per-run decision.
//
// The shape is one evening on a small server: 60% of the population out on
// the routes, 30% in town, 10% indoors -- crowded at the owner's target of
// 200 per town map, 35 per route, 20 per shop/gym.
// ---------------------------------------------------------------------------
static const KindCfg WORLD[Kind_count] = {
    /* outdoor */ { 60,  35 },
    /* city    */ { 30, 200 },
    /* indoor  */ { 10,  20 }
};

// WALK MODEL: runs, not a per-tick coin flip. An idle player starts walking
// with WALK_START_PCT chance per tick, picks a direction, and then walks
// RUN_LEN_MAX steps at most before stopping again -- which is how people
// actually move, and it is also the only way this benchmark exercises the
// half of min_network() that matters most: the slots that did NOT change.
//
// With a per-tick coin flip at 70%, ~91% of the slots differed EVERY tick
// (measured), so the "same as last broadcast -> send nothing" path was never
// really taken and any optimisation of it would have been unmeasurable -- the
// same trap the owner fixed in benchmarkmapmanager by dropping its move rate
// to 40%. A 7% start rate over runs averaging 4.5 steps leaves a player
// walking ~1 tick in 4 and standing the rest.
#define WALK_START_PCT 7
#define RUN_LEN_MAX    8
// % of ticks where one player walks through a border into another map. This is
// the benchmark's insert+remove pair: the arrival takes PATH 1 (drop-all +
// full re-insert) on its new map, the departure emits a 0x69 remove on the old.
#define MIGRATE_PCT  5
#define WORLD_SEED   0x5EEDu
// The world is the datapack's GENERATED map set -- hundreds of real maps
// (towns, routes, interiors) with real collisions, not a synthesised grid.
#define WORLD_MAP_SUBDIR "map/main/generated/"

// STRICT per-map ceiling: the wire slot index is 8-bit and 255 is the reserved
// value, so slots run 0..254 -- and min_network() itself clamps its client
// count to 254 (MapVisibilityAlgorithm.cpp). A map is never given more than
// that; occupancy is clamped to it. The population as a whole has no such
// limit, which is why this benchmark sweeps thousands of players: they simply
// occupy more maps.
#define MAP_PLAYER_CAP 254

// ---------------------------------------------------------------------------
// The world: the REAL generated datapack maps, loaded with the PRODUCTION
// loader (general/base/Map_loader.cpp), keeping ONLY what the server keeps
// hot per map -- width, height and flat_simplified_map. Everything the loader
// builds on the way (tile layers, bots, teleporters, parsed XML documents) is
// released as soon as the map is loaded.
//
// Why the production loader rather than a private .tmx reader: the collision
// data IS the workload. A hand-rolled parser would be one more thing that can
// silently disagree with the game -- here the benchmark walks over exactly the
// bytes the server would, produced by exactly the same code path, and the
// walkability test below is the production predicate itself.
// ---------------------------------------------------------------------------

// One map, reduced to what the walk model needs. CommonMap is the production
// container; only width/height/flat_simplified_map are populated.
struct LoadedMap
{
    CommonMap map;
    uint8_t kind;
    uint32_t walkable_cells;   // cells a player may stand on -> crowd cap

    LoadedMap() : kind(Kind_outdoor), walkable_cells(0) {}
};

// What the DATAPACK says a map is: its sibling .xml carries
// <map type="city|indoor|outdoor|cave">, which is the generator's own
// statement and the only authority worth using. "cave" joins outdoor: it is a
// wild area people cross, not a room they stand in.
static uint8_t kind_of_type(const char *type)
{
    if(type == NULL)             return Kind_indoor;
    if(strcmp(type, "city") == 0)    return Kind_city;
    if(strcmp(type, "outdoor") == 0) return Kind_outdoor;
    if(strcmp(type, "cave") == 0)    return Kind_outdoor;
    return Kind_indoor;
}

// Fallback when a map has no sibling .xml (or no type attribute): guess from
// the path, which is how the generator lays this world out.
//   road-<n>/...        a route             -> outdoor
//   <town>/<town>.tmx   the town itself     -> city
//   <town>/<other>.tmx  a shop/gym/house    -> indoor
// It is only a guess: on the current map set it disagrees with the declared
// type on 85 of 647 maps (routes and caves filed under a town's folder), which
// is exactly why the declared type wins when there is one.
static uint8_t classify_map(const std::string &relative)
{
    const size_t slash = relative.find('/');
    if(slash == std::string::npos)
        return Kind_outdoor;               // a map at the root: open world
    const std::string dir = relative.substr(0, slash);
    std::string base = relative.substr(slash + 1);
    const size_t last = base.rfind('/');
    if(last != std::string::npos)
        base = base.substr(last + 1);
    if(base.size() > 4 && base.compare(base.size() - 4, 4, ".tmx") == 0)
        base.resize(base.size() - 4);
    if(dir.size() > 5 && dir.compare(0, 5, "road-") == 0)
        return Kind_outdoor;
    if(base == dir)
        return Kind_city;
    return Kind_indoor;
}

// Production walkability, not a copy of it: MoveOnTheMap::isWalkableWithDirection
// is what the game asks before every step (flat_simplified_map < 200 walkable,
// 250-253 one-way ledges, 254 blocked, out of map refused).
static bool step_allowed(const CommonMap &m, int x, int y, Direction d)
{
    if(x < 0 || y < 0 || x > 255 || y > 255)
        return false;
    return MoveOnTheMap::isWalkableWithDirection(m, (uint8_t)x, (uint8_t)y, d);
}

// A cell a player may STAND on (direction-free, so never a ledge) -- the same
// test the game uses outside a move.
static bool stand_allowed(const CommonMap &m, int x, int y)
{
    if(x < 0 || y < 0 || x >= (int)m.width || y >= (int)m.height)
        return false;
    return MoveOnTheMap::isWalkable(m, (uint8_t)x, (uint8_t)y);
}

// Deterministic spawn on a standable cell: a few random draws, then a linear
// scan from a random offset so a heavily built map still resolves.
static bool find_spawn(const CommonMap &m, Lcg &rng, uint8_t &x, uint8_t &y)
{
    unsigned int tries = 0;
    while(tries < 32)
    {
        const uint8_t cx = (uint8_t)rng.below((uint32_t)m.width);
        const uint8_t cy = (uint8_t)rng.below((uint32_t)m.height);
        if(stand_allowed(m, cx, cy)) { x = cx; y = cy; return true; }
        tries++;
    }
    // Trust the array, not the header: index only what the loader actually
    // filled (they agree on every map of the current set, but a truncated
    // layer must not walk off the end).
    size_t cells = (size_t)m.width * m.height;
    if(cells > m.flat_simplified_map.size())
        cells = m.flat_simplified_map.size();
    if(cells == 0 || m.width == 0)
        return false;
    const size_t start = rng.below((uint32_t)cells);
    size_t i = 0;
    while(i < cells)
    {
        const size_t pos = (start + i) % cells;
        if(m.flat_simplified_map[pos] < 200)
        {
            x = (uint8_t)(pos % m.width);
            y = (uint8_t)(pos / m.width);
            return true;
        }
        i++;
    }
    return false;                          // fully blocked map: never populated
}

// Load every .tmx under <datapack>/<map_subdir> with the production loader.
// Returns false + `error` when the datapack cannot be read at all; individual
// map failures are counted, not fatal.
static bool load_world(const std::string &datapack, const std::string &map_subdir,
                       std::vector<LoadedMap*> &out, unsigned int &failed,
                       unsigned int &typed, std::string &error)
{
    // Map_loaderMain.cpp aborts unless the item and monster name tables are
    // populated (they resolve map items and the monster-collision zones that
    // become flat_simplified_map values 1..199). parseDatapack() is the
    // production call that fills them -- 2 ms for this datapack.
    // It narrates what it loads on stdout ("80 items(s) loaded", ...), and so
    // does the map loader; muffle both so the run's stdout is the BENCH data
    // and nothing else.
    std::cout.setstate(std::ios_base::badbit);
    CommonDatapack::commonDatapack.parseDatapack(datapack);
    const std::string map_path = datapack + map_subdir;
    std::vector<std::string> files = FacilityLibGeneral::listFolder(map_path);
    if(files.empty())
    {
        std::cout.clear();
        error = "no file found under " + map_path;
        return false;
    }
    // The directory order the filesystem hands back is NOT stable across
    // machines, and the map INDEX decides which map each player lands on, so
    // sort: without this the fleet would measure a different world per node.
    std::sort(files.begin(), files.end());
    Map_loader loader;
    size_t i = 0;
    while(i < files.size())
    {
        const std::string &relative = files.at(i);
        if(relative.size() > 4 && relative.compare(relative.size() - 4, 4, ".tmx") == 0)
        {
            CommonMap scratch;             // bots/teleporters/zones land here
            scratch.width = 0;
            scratch.height = 0;
            if(loader.tryLoadMap(map_path + relative, scratch, true)
               && loader.map_to_send.width > 0 && loader.map_to_send.height > 0
               && loader.map_to_send.flat_simplified_map.size()
                  == (size_t)loader.map_to_send.width * loader.map_to_send.height)
            {
                LoadedMap *lm = new LoadedMap();
                // tryLoadMap() leaves the dimensions on map_to_send, not on
                // the destination map (same as map2png reads them).
                lm->map.width  = (uint8_t)loader.map_to_send.width;
                lm->map.height = (uint8_t)loader.map_to_send.height;
                lm->map.flat_simplified_map.swap(loader.map_to_send.flat_simplified_map);
                // The loader still holds the sibling .xml it just parsed, so
                // the declared type is one attribute away -- read it BEFORE
                // the document cache is dropped below.
                lm->kind = classify_map(relative);
                if(loader.map_to_send.xmlRoot != NULL
                   && loader.map_to_send.xmlRoot->Attribute("type") != NULL)
                {
                    lm->kind = kind_of_type(loader.map_to_send.xmlRoot->Attribute("type"));
                    typed++;
                }
                size_t c = 0;
                while(c < lm->map.flat_simplified_map.size())
                {
                    if(lm->map.flat_simplified_map[c] < 200)
                        lm->walkable_cells++;
                    c++;
                }
                out.push_back(lm);
            }
            else
                failed++;
            // Release everything the server would not keep hot. Outside a
            // CATCHCHALLENGER_SERVER build the loader parks every parsed
            // XMLDocument in CommonDatapack::xmlLoadedFile and never frees it
            // (Map_loaderMain.cpp), so 647 maps would otherwise pile up their
            // whole DOM in RSS.
            loader.map_to_send = Map_to_send();
            CommonDatapack::commonDatapack.clear_xmlLoadedFile();
            Map_loader::teleportConditionsUnparsed.clear();
        }
        i++;
    }
    // The maps are in; the datapack tables were only needed to load them
    // (item names, monster-collision zones). Give that memory back -- and note
    // the order: unload() clears the very name tables tryLoadMap() aborts
    // without, so nothing may be loaded after this point.
    CommonDatapack::commonDatapack.unload();
    std::cout.clear();
    if(out.empty())
    {
        error = "no .tmx loaded under " + map_path;
        return false;
    }
    return true;
}

struct WorldMap
{
    HarnessMVA mva;
    const CommonMap *map;      // the loaded map: collisions + dimensions
    uint8_t kind;
    unsigned int population;
    unsigned int cap;          // most players this map may hold

    WorldMap() : map(NULL), kind(Kind_outdoor), population(0), cap(0) {}
};

struct Player
{
    ClientWithMap *client;
    const CommonMap *map_data;         // its map's collisions, cached
    PLAYER_INDEX_FOR_CONNECTED gid;    // index in ClientList
    CATCHCHALLENGER_TYPE_MAPID  map;   // index in World::maps
    PLAYER_INDEX_FOR_CONNECTED slot;   // index in that map's map_clients_id
    uint8_t facing;                    // 0=top 1=right 2=bottom 3=left
    uint8_t steps_left;                // >0 while walking a run

    Player() : client(NULL), map_data(NULL), gid(0), map(0), slot(0),
               facing(0), steps_left(0) {}
};

static Direction look_of(uint8_t facing)
{
    if(facing == 0) return Direction_look_at_top;
    if(facing == 1) return Direction_look_at_right;
    if(facing == 2) return Direction_look_at_bottom;
    return Direction_look_at_left;
}

static Direction move_of(uint8_t facing)
{
    if(facing == 0) return Direction_move_at_top;
    if(facing == 1) return Direction_move_at_right;
    if(facing == 2) return Direction_move_at_bottom;
    return Direction_move_at_left;
}

// ---------------------------------------------------------------------------
// World -- every loaded map, plus the players walking on them
// ---------------------------------------------------------------------------
struct World
{
    ClientList cl;
    std::vector<WorldMap*> maps;          // EVERY loaded map, grouped by kind
    std::vector<Player> players;
    unsigned int players_of_kind[Kind_count];
    unsigned int maps_of_kind[Kind_count];      // maps that EXIST, per kind
    unsigned int first_map_of_kind[Kind_count]; // where that kind starts in maps[]
    // The maps a kind's population actually sits on. Both the initial placement
    // and every later migration draw from this set, so the world SHAPE (a few
    // busy towns, a tail of quiet interiors) stays put instead of slowly
    // diffusing the population over all 600+ maps as the run goes on.
    std::vector<unsigned int> populated[Kind_count];
    unsigned int weight_total;
    const KindCfg *cfg;

    World() : weight_total(0), cfg(NULL)
    {
        ClientList::list = &cl;
        // Match production-typical tunables. simple.max governs the early-out
        // branch in min_network(); keep it above the per-map cap so it stays
        // out of the timing path.
        GlobalServerData::serverSettings.mapVisibility.simple.max = 1024;
        GlobalServerData::serverSettings.dontSendPlayerType = false;
        CommonSettingsServer::commonSettingsServer.dontSendPseudo = false;
        unsigned int k = 0;
        while(k < Kind_count)
        {
            players_of_kind[k] = 0;
            maps_of_kind[k] = 0;
            first_map_of_kind[k] = 0;
            k++;
        }
    }
    ~World()
    {
        size_t i = 0;
        while(i < players.size()) { delete players[i].client; i++; }
        players.clear();
        i = 0;
        while(i < maps.size()) { delete maps[i]; i++; }
        maps.clear();
        cl.clear();
        ClientList::list = NULL;
    }

    // Population split: weight_k / sum(weights).
    void split_population(unsigned int players_total, const KindCfg *c)
    {
        cfg = c;
        weight_total = 0;
        unsigned int k = 0;
        while(k < Kind_count) { weight_total += c[k].weight; k++; }
        if(weight_total == 0) weight_total = 1;
        unsigned int assigned = 0;
        k = 0;
        while(k < Kind_count)
        {
            players_of_kind[k] = players_total * c[k].weight / weight_total;
            assigned += players_of_kind[k];
            k++;
        }
        // Integer division loses at most Kind_count-1 players; give them to the
        // kind that carries the most weight so the totals always add back up.
        unsigned int biggest = 0;
        k = 1;
        while(k < Kind_count)
        {
            if(c[k].weight > c[biggest].weight) biggest = k;
            k++;
        }
        players_of_kind[biggest] += players_total - assigned;
    }

    void build(unsigned int players_total, const KindCfg *c,
               const std::vector<LoadedMap*> &loaded, Lcg &rng)
    {
        split_population(players_total, c);
        // EVERY loaded map joins the world and is broadcast every tick -- that
        // is what the server's timer does (it walks the whole flat_map_list:
        // MapVisibilityAlgorithm_WithoutSender.cpp), and the cost of ticking
        // hundreds of quiet maps is a real part of its per-tick budget.
        // maps[] is grouped by kind so each kind is one contiguous range: the
        // per-kind timing split and the migration draw both rely on that.
        unsigned int k = 0;
        while(k < Kind_count)
        {
            first_map_of_kind[k] = (unsigned int)maps.size();
            size_t i = 0;
            while(i < loaded.size())
            {
                if(loaded[i]->kind == k)
                {
                    WorldMap *m = new WorldMap();
                    m->kind = (uint8_t)k;
                    m->map  = &loaded[i]->map;
                    // No more players than the wire allows, and never more than
                    // there are cells to stand on.
                    m->cap = loaded[i]->walkable_cells < MAP_PLAYER_CAP
                             ? loaded[i]->walkable_cells : MAP_PLAYER_CAP;
                    // The CATCHCHALLENGER_TESTING x/y range guard reads these.
                    m->mva.width  = loaded[i]->map.width;
                    m->mva.height = loaded[i]->map.height;
                    maps.push_back(m);
                }
                i++;
            }
            maps_of_kind[k] = (unsigned int)maps.size() - first_map_of_kind[k];
            k++;
        }
        // Choose WHICH maps of each kind carry that kind's population: enough
        // of them for the owner's target crowd (200 per town, 35 per route, 20
        // per interior), drawn at random from the kind so it is not always the
        // same corner of the world.
        k = 0;
        while(k < Kind_count)
        {
            if(players_of_kind[k] > 0 && maps_of_kind[k] > 0)
            {
                unsigned int occ = c[k].occupancy;
                if(occ == 0) occ = 1;
                if(occ > MAP_PLAYER_CAP) occ = MAP_PLAYER_CAP;
                unsigned int need = (players_of_kind[k] + occ - 1) / occ;
                if(need > maps_of_kind[k]) need = maps_of_kind[k];
                // Partial Fisher-Yates over the kind's index range: `need`
                // distinct maps, deterministic for a given seed.
                std::vector<unsigned int> pool;
                pool.reserve(maps_of_kind[k]);
                unsigned int i = 0;
                while(i < maps_of_kind[k]) { pool.push_back(first_map_of_kind[k] + i); i++; }
                i = 0;
                while(i < need)
                {
                    const unsigned int j = i + rng.below((uint32_t)(pool.size() - i));
                    const unsigned int tmp = pool[i]; pool[i] = pool[j]; pool[j] = tmp;
                    populated[k].push_back(pool[i]);
                    i++;
                }
            }
            k++;
        }
        // Players land on a random map of their kind's populated set, so the
        // crowd varies map to map the way a real server's does.
        uint32_t next_id = 1000;
        k = 0;
        while(k < Kind_count)
        {
            unsigned int i = 0;
            while(i < players_of_kind[k])
            {
                const int mi = pick_map_in_kind(k, rng);
                if(mi < 0)
                    break;               // kind out of room: cannot happen with
                                         // the real map set, guarded anyway
                addPlayer(next_id++, (CATCHCHALLENGER_TYPE_MAPID)mi, rng);
                i++;
            }
            k++;
        }
    }

    // A map of kind `k` with room left, or -1. Random first (natural
    // clumping), then a scan so a nearly-full kind still resolves.
    int pick_map_in_kind(unsigned int k, Lcg &rng)
    {
        const std::vector<unsigned int> &set = populated[k];
        if(set.empty())
            return -1;
        unsigned int tries = 0;
        while(tries < 8)
        {
            const unsigned int mi = set[rng.below((uint32_t)set.size())];
            if(maps[mi]->population < maps[mi]->cap) return (int)mi;
            tries++;
        }
        size_t i = 0;
        while(i < set.size())
        {
            if(maps[set[i]]->population < maps[set[i]]->cap) return (int)set[i];
            i++;
        }
        return -1;
    }

    void addPlayer(uint32_t id, CATCHCHALLENGER_TYPE_MAPID map_index, Lcg &rng)
    {
        WorldMap &m = *maps[map_index];
        uint8_t x = 0, y = 0;
        if(!find_spawn(*m.map, rng, x, y))
            return;                        // fully blocked map: leave it empty
        ClientWithMap *c = new ClientWithMap();
        Player p;
        p.facing = rng.pick4();
        c->setX(x); c->setY(y); c->setDirection(look_of(p.facing));
        c->setPlayerId(id);
        c->setMapIndex(map_index);
        char pseudo[16];
        std::snprintf(pseudo, sizeof(pseudo), "p%u", id);
        c->public_and_private_informations.public_informations.pseudo = pseudo;
        c->public_and_private_informations.public_informations.type    = Player_type_normal;
        c->public_and_private_informations.public_informations.skinId  = (uint8_t)(id & 0xff);
        p.client   = c;
        p.map_data = m.map;
        p.gid      = cl.add(c);
        p.map      = map_index;
        p.slot     = m.mva.insertOnMap(p.gid);
        m.population++;
        players.push_back(p);
    }

    // A player walks through a border/teleporter into another map. This is the
    // benchmark's insert+remove pair: the arrival takes PATH 1 (drop-all + full
    // re-insert) on its new map, the departure emits a 0x69 remove on the old.
    bool migrate(unsigned int player_index, Lcg &rng)
    {
        Player &p = players[player_index];
        WorldMap &from = *maps[p.map];
        // Destination kind drawn by weight, so migrations keep the world's
        // shape instead of flattening it over time.
        unsigned int roll = rng.below(weight_total);
        unsigned int k = 0;
        while(k + 1 < Kind_count && roll >= cfg[k].weight)
        {
            roll -= cfg[k].weight;
            k++;
        }
        if(populated[k].empty())
        {
            k = 0;
            while(k < Kind_count && populated[k].empty()) k++;
            if(k >= Kind_count) return false;
        }
        const int dest = pick_map_in_kind(k, rng);
        if(dest < 0 || (unsigned int)dest == p.map) return false;
        WorldMap &to = *maps[dest];
        uint8_t x = 0, y = 0;
        if(!find_spawn(*to.map, rng, x, y)) return false;
        from.mva.removeOnMap(p.slot);
        from.population--;
        p.client->setX(x); p.client->setY(y);
        p.client->setMapIndex((CATCHCHALLENGER_TYPE_MAPID)dest);
        p.map      = (CATCHCHALLENGER_TYPE_MAPID)dest;
        p.map_data = to.map;
        p.slot     = to.mva.insertOnMap(p.gid);
        to.population++;
        // Whatever was left of its run belongs to the map it just left.
        p.steps_left = 0;
        return true;
    }

    unsigned int populated_maps() const
    {
        unsigned int n = 0;
        size_t i = 0;
        while(i < maps.size()) { if(maps[i]->population > 0) n++; i++; }
        return n;
    }

    void clearCaptured()
    {
        size_t i = 0;
        while(i < players.size()) { players[i].client->sentBlocks.clear(); i++; }
    }

    uint64_t totalBytesAndClear()
    {
        uint64_t total = 0;
        size_t i = 0;
        while(i < players.size())
        {
            ClientWithMap *c = players[i].client;
            size_t b = 0;
            while(b < c->sentBlocks.size()) { total += c->sentBlocks[b].bytes.size(); b++; }
            c->sentBlocks.clear();
            i++;
        }
        return total;
    }

    // Resident visibility state: one shared snapshot per map plus the private
    // baseline of each recipient currently held back by flow control.
    uint64_t visibilityStateBytes()
    {
        uint64_t total = 0;
        size_t mi = 0;
        while(mi < maps.size())
        {
            HarnessMVA &mva = maps[mi]->mva;
            total += (uint64_t)mva.previousDenseBuffer.capacity() * sizeof(DensePlayerState);
            size_t slot = 0;
            while(slot < mva.map_clients_id.size())
            {
                const PLAYER_INDEX_FOR_CONNECTED gid = mva.map_clients_id[slot];
                if(gid != PLAYER_INDEX_FOR_CONNECTED_MAX)
                    total += (uint64_t)cl.rwWithMap(gid).sendedStatus.capacity()
                             * sizeof(DensePlayerState);
                slot++;
            }
            mi++;
        }
        return total;
    }

    // Snapshot / compare of what the wire actually carries per player
    // (x, y, direction). Used on the latency-sampled tick ONLY -- one pass each
    // side of the tick, once per batch -- to report how many slots min_network
    // finds unchanged. That fraction is the whole point of the stateful diff,
    // and without measuring it a load model can silently drift into "everything
    // changes every tick", where the diff has nothing to skip and any
    // optimisation of it is unmeasurable.
    void snapshotState(std::vector<uint32_t> &out) const
    {
        out.resize(players.size());
        size_t i = 0;
        while(i < players.size())
        {
            const Client *c = players[i].client;
            out[i] = (uint32_t)c->getX() | ((uint32_t)c->getY() << 8)
                     | ((uint32_t)c->getLastDirection() << 16);
            i++;
        }
    }

    unsigned int countChanged(const std::vector<uint32_t> &prev) const
    {
        unsigned int changed = 0;
        size_t i = 0;
        while(i < players.size() && i < prev.size())
        {
            const Client *c = players[i].client;
            const uint32_t now = (uint32_t)c->getX() | ((uint32_t)c->getY() << 8)
                                 | ((uint32_t)c->getLastDirection() << 16);
            if(now != prev[i]) changed++;
            i++;
        }
        return changed;
    }

    // One tick of the server timer: broadcast EVERY map, in order.
    void broadcast()
    {
        size_t i = 0;
        while(i < maps.size())
        {
            maps[i]->mva.min_network((CATCHCHALLENGER_TYPE_MAPID)i);
            i++;
        }
    }

    // The same tick, split per kind. Used on the latency-sampled tick only, so
    // its two extra clock reads amortise over a whole batch: it says where the
    // per-tick budget actually goes -- the crowded towns, the routes, or the
    // constant of ticking hundreds of quiet interiors.
    void broadcast_by_kind(uint64_t out_ns[Kind_count])
    {
        unsigned int k = 0;
        while(k < Kind_count)
        {
            const std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
            unsigned int i = first_map_of_kind[k];
            const unsigned int end = first_map_of_kind[k] + maps_of_kind[k];
            while(i < end)
            {
                maps[i]->mva.min_network((CATCHCHALLENGER_TYPE_MAPID)i);
                i++;
            }
            const std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
            out_ns[k] = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
            k++;
        }
    }
};

// How far the player can actually walk from (x,y) in `facing`, capped at
// `want`: the movement VECTOR is truncated at the first obstacle instead of
// being walked into it. The test is the PRODUCTION one
// (MoveOnTheMap::isWalkableWithDirection -- collisions, one-way ledges and the
// map border, exactly as the game answers it). It scans at most RUN_LEN_MAX
// cells and runs ONCE per run, not once per tick, so the per-tick cost of a
// walking player is a coordinate update and nothing else.
static uint8_t free_run_length(const CommonMap &m, int x, int y, uint8_t facing,
                               uint8_t want)
{
    const Direction d = move_of(facing);
    int dx = 0, dy = 0;
    if(facing == 0)      dy = -1;
    else if(facing == 1) dx = 1;
    else if(facing == 2) dy = 1;
    else                 dx = -1;
    uint8_t n = 0;
    while(n < want)
    {
        x += dx;
        y += dy;
        if(!step_allowed(m, x, y, d))
            break;
        n++;
    }
    return n;
}

// One player's turn in the walk model. NO PATHFINDING by design -- a straight
// vector, truncated by the collision data, and the player simply stops early
// when something is in the way. Three states:
//   walking (steps_left>0) -- advance one cell along the already-validated
//                             vector. No test: the map never changes and
//                             nothing else moves the player.
//   idle, run starts       -- pick a direction and a length, truncate the
//                             vector at the first obstacle, walk its first
//                             cell. A vector truncated to 0 means the player
//                             faced a wall: it turns in place and stays put.
//   idle                   -- stand, facing where it last looked.
static void advance_player(World &w, unsigned int i, Lcg &rng,
                           uint32_t start_thr, uint64_t &moves,
                           uint64_t &runs, uint64_t &runs_blocked,
                           uint64_t &runs_truncated)
{
    Player &p = w.players[i];
    if(p.steps_left == 0)
    {
        if(rng.next() >= start_thr)
        {
            p.client->setDirection(look_of(p.facing));   // standing still
            return;
        }
        p.facing = rng.pick4();
        const uint8_t want = (uint8_t)(1u + (rng.next() >> 29));   // 1..8
        const uint8_t free_len = free_run_length(*p.map_data, p.client->getX(),
                                                 p.client->getY(), p.facing, want);
        runs++;
        if(free_len == 0)
        {
            // Facing a wall, a building or the map edge: turn, do not move.
            p.client->setDirection(look_of(p.facing));
            runs_blocked++;
            return;
        }
        if(free_len < want)
            runs_truncated++;
        p.steps_left = free_len;
    }
    int nx = p.client->getX();
    int ny = p.client->getY();
    if(p.facing == 0)      ny--;
    else if(p.facing == 1) nx++;
    else if(p.facing == 2) ny++;
    else                   nx--;
    p.client->setX((COORD_TYPE)nx);
    p.client->setY((COORD_TYPE)ny);
    p.client->setDirection(move_of(p.facing));
    p.steps_left--;
    moves++;
}

// Per-tick load model. Everything here is HARNESS cost, deliberately kept to a
// few instructions per player: one threshold compare for the walk draw, one
// array lookup for the collision test, no division, no allocation, no second
// pass over the players (totalBytesAndClear() below already empties the
// capture buffers, so there is nothing left to clear here). run_scenario()
// reports it as median_prep_ns so the ratio to median_tick_ns is checked every
// run instead of being assumed.
static void prepare_tick(World &w, Lcg &rng, uint32_t migrate_thr,
                         uint32_t start_thr, uint64_t &migrations,
                         uint64_t &moves, uint64_t &runs,
                         uint64_t &runs_blocked, uint64_t &runs_truncated)
{
    unsigned int i = 0;
    while(i < w.players.size())
    {
        // Deliver the 0xE3 reply exactly as production does in
        // ClientNetworkRead.cpp. Every client answers within the tick here:
        // the held-back / coalesced-delta path depends on link speed, which is
        // not something this CPU benchmark can model honestly -- it belongs to
        // the network benchmarks (benchmarkclientlatency.py).
        w.players[i].client->ackPing();
        advance_player(w, i, rng, start_thr, moves, runs, runs_blocked,
                       runs_truncated);
        i++;
    }
    if(!w.players.empty() && rng.next() < migrate_thr)
    {
        if(w.migrate(rng.below((uint32_t)w.players.size()), rng))
            migrations++;
    }
}

// budget_ms > 0  -> FIXED-TIME: loop until the wall budget elapses and report
//                   how many ticks completed (benchmark/CLAUDE.md).
// budget_ms == 0 -> FIXED-ITERATION: exactly `ticks` ticks, for callgrind whose
//                   metric is a deterministic instruction count.
static int run_scenario(unsigned int players, unsigned int ticks,
                        uint64_t budget_ms,
                        const std::vector<LoadedMap*> &loaded)
{
    World w;
    Lcg rng(WORLD_SEED);
    w.build(players, WORLD, loaded, rng);
    const uint32_t start_thr   = pct_threshold(WALK_START_PCT);
    const uint32_t migrate_thr = pct_threshold(MIGRATE_PCT);

    // Silence the CATCHCHALLENGER_TESTING slot-by-slot debug prints of
    // MapVisibilityAlgorithm.cpp: at this scale their volume would dominate
    // the timing. badbit makes every operator<< a no-op, algorithm untouched.
    std::cout.setstate(std::ios_base::badbit);

    // Warmup tick: every client takes PATH 1 (sendedMap != mapIndex) and does
    // the full drop+reinsert handshake. Cache priming, excluded from timing.
    w.clearCaptured();
    const std::chrono::steady_clock::time_point warm0 = std::chrono::steady_clock::now();
    w.broadcast();
    const std::chrono::steady_clock::time_point warm1 = std::chrono::steady_clock::now();
    const uint64_t warm_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(warm1 - warm0).count();
    const uint64_t bytes_warm = w.totalBytesAndClear();

    std::vector<uint64_t> samples;   samples.reserve(ticks ? ticks : 4096);
    std::vector<uint64_t> prep_samples; prep_samples.reserve(ticks ? ticks : 4096);
    uint64_t bytes_total = 0, migrations = 0, moves = 0;
    uint64_t runs = 0, runs_blocked = 0, runs_truncated = 0;
    // Changed-slot census, taken on the latency-sampled ticks only.
    std::vector<uint32_t> snap;
    uint64_t sampled_slots = 0, sampled_changed = 0;
    // Where the tick goes, per map kind, on those same sampled ticks: the
    // crowded towns, the routes, or the constant of ticking the quiet
    // interiors. Accumulated as sums and reported as a per-tick MEAN (the
    // three parts then add up to the whole tick, which a median would not).
    uint64_t kind_ns[Kind_count];
    uint64_t kind_ticks = 0;
    { unsigned int k = 0; while(k < Kind_count) { kind_ns[k] = 0; k++; } }

    const std::chrono::steady_clock::time_point loop_start = std::chrono::steady_clock::now();
    const std::chrono::milliseconds budget(budget_ms);
    unsigned int t = 0;

    if(budget_ms == 0)
    {
        while(t < ticks)
        {
            // Census brackets the tick from OUTSIDE both timed windows: it
            // is diagnostic, not part of the load model, and must not land in
            // median_prep_ns. Broadcasting does not touch player state, so
            // counting after it gives the same answer.
            w.snapshotState(snap);
            const std::chrono::steady_clock::time_point p0 = std::chrono::steady_clock::now();
            prepare_tick(w, rng, migrate_thr, start_thr, migrations, moves,
                         runs, runs_blocked, runs_truncated);
            uint64_t one_kind[Kind_count];
            const std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
            w.broadcast_by_kind(one_kind);
            const std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
            samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
            prep_samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t0 - p0).count());
            { unsigned int k = 0; while(k < Kind_count) { kind_ns[k] += one_kind[k]; k++; } }
            kind_ticks++;
            sampled_changed += w.countChanged(snap);
            sampled_slots   += w.players.size();
            bytes_total += w.totalBytesAndClear();
            t++;
        }
    }
    else
    {
        // FIXED-TIME with ADAPTIVE BATCHING -- same reasoning as
        // benchmarkmapmanager: on a slow clock (Geode / old MIPS,
        // clock_gettime is a ~1-2us syscall with no vDSO) timing every tick
        // would skew the throughput count, so calibrate, batch, and sample
        // latency on one tick per batch.
        const uint64_t CHECK_INTERVAL_NS = 5000000ull;
        const unsigned int CALIB = 4;
        unsigned int k = 0;
        while(k < CALIB)
        {
            prepare_tick(w, rng, migrate_thr, start_thr, migrations, moves,
                         runs, runs_blocked, runs_truncated);
            w.broadcast();
            bytes_total += w.totalBytesAndClear();
            t++;
            k++;
        }
        uint64_t calib_ns = (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - loop_start).count();
        uint64_t per_tick_ns = calib_ns / CALIB;
        if(per_tick_ns == 0) per_tick_ns = 1;
        uint64_t ce = CHECK_INTERVAL_NS / per_tick_ns;
        if(ce < 1)          ce = 1;
        if(ce > 1000000ull) ce = 1000000ull;
        const unsigned int check_every = (unsigned int)ce;

        while(std::chrono::steady_clock::now() - loop_start < budget)
        {
            // Census brackets the tick from OUTSIDE both timed windows: it
            // is diagnostic, not part of the load model, and must not land in
            // median_prep_ns. Broadcasting does not touch player state, so
            // counting after it gives the same answer.
            w.snapshotState(snap);
            const std::chrono::steady_clock::time_point p0 = std::chrono::steady_clock::now();
            prepare_tick(w, rng, migrate_thr, start_thr, migrations, moves,
                         runs, runs_blocked, runs_truncated);
            uint64_t one_kind[Kind_count];
            const std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
            w.broadcast_by_kind(one_kind);
            const std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
            samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
            prep_samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t0 - p0).count());
            { unsigned int k = 0; while(k < Kind_count) { kind_ns[k] += one_kind[k]; k++; } }
            kind_ticks++;
            sampled_changed += w.countChanged(snap);
            sampled_slots   += w.players.size();
            bytes_total += w.totalBytesAndClear();
            t++;
            k = 1;
            while(k < check_every)
            {
                prepare_tick(w, rng, migrate_thr, start_thr, migrations, moves,
                             runs, runs_blocked, runs_truncated);
                w.broadcast();
                bytes_total += w.totalBytesAndClear();
                t++;
                k++;
            }
        }
    }
    const uint64_t ticks_done = t;
    const uint64_t elapsed_ms = (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - loop_start).count();

    std::vector<uint64_t> sorted = samples;
    std::sort(sorted.begin(), sorted.end());
    const uint64_t median = sorted.empty() ? 0 : sorted[sorted.size() / 2];
    const uint64_t p95    = sorted.empty() ? 0 : sorted[(sorted.size() * 95) / 100];
    std::vector<uint64_t> sorted_prep = prep_samples;
    std::sort(sorted_prep.begin(), sorted_prep.end());
    const uint64_t median_prep = sorted_prep.empty() ? 0 : sorted_prep[sorted_prep.size() / 2];
    uint64_t total = 0;
    size_t si = 0;
    while(si < samples.size()) { total += samples[si]; si++; }
    const double ticks_per_s = elapsed_ms > 0
        ? (double)ticks_done * 1000.0 / (double)elapsed_ms : 0.0;

    // Oracle: the walk model must never park a player on a collision cell or
    // off the map. Checked once per scenario (O(players), outside every timed
    // window) so a broken collision test shows up as a FAILing benchmark
    // instead of silently changing what the workload means. The production
    // CATCHCHALLENGER_TESTING guard (assertXYInRange) already aborts on an
    // out-of-map coordinate every tick; this covers the blocked-cell half.
    uint64_t walk_violations = 0;
    size_t vi = 0;
    while(vi < w.players.size())
    {
        const Player &pl = w.players[vi];
        if(!stand_allowed(*w.maps[pl.map]->map, pl.client->getX(), pl.client->getY()))
            walk_violations++;
        vi++;
    }

    std::cout.clear();
    std::cout << "BENCH"
              << " players=" << players
              // maps = every map ticked (the whole loaded world, like the
              // server); maps_populated = those that actually hold someone.
              << " maps=" << w.maps.size()
              << " maps_populated=" << w.populated_maps()
              << " ticks=" << ticks_done
              << " duration_ms=" << elapsed_ms
              << " ticks_per_s=" << ticks_per_s
              // The workload constants are echoed so a history record says
              // what it measured without anyone having to date the source.
              << " walk_start_pct=" << WALK_START_PCT
              << " run_len_max=" << RUN_LEN_MAX
              << " migrate_pct=" << MIGRATE_PCT;
    unsigned int k = 0;
    while(k < Kind_count)
    {
        std::cout << " weight_" << kind_name(k) << "=" << WORLD[k].weight
                  << " occ_" << kind_name(k) << "=" << WORLD[k].occupancy
                  << " players_" << kind_name(k) << "=" << w.players_of_kind[k]
                  << " maps_" << kind_name(k) << "=" << w.maps_of_kind[k];
        k++;
    }
    std::cout << " visibility_state_bytes=" << w.visibilityStateBytes()
              << " warm_ns=" << warm_ns
              << " bytes_warm=" << bytes_warm
              << " total_ns=" << total
              << " median_tick_ns=" << median
              << " p95_tick_ns=" << p95
              // Harness cost per tick (walk + ACK + migration). It is NOT
              // server work: if it ever approaches median_tick_ns the run is
              // measuring the load model, not min_network.
              << " median_prep_ns=" << median_prep
              << " bytes_sent=" << bytes_total
              << " migrations=" << migrations
              << " moves=" << moves
              << " runs=" << runs
              // A run refused outright (the player faced a wall and turned in
              // place) and a run cut short by an obstacle: together they say
              // the collision grid is really shaping the walk, not decorating
              // it.
              << " runs_blocked=" << runs_blocked
              << " runs_truncated=" << runs_truncated
              // Changed-slot census over the sampled ticks. sampled_changed /
              // sampled_slots is the share of slots min_network has to send;
              // the rest is what its stateful diff skips. If this ever
              // approaches 100% the load model has drifted and the skip path
              // is no longer being measured.
              << " sampled_slots=" << sampled_slots
              << " sampled_changed=" << sampled_changed;
    { unsigned int k = 0;
      while(k < Kind_count)
      {
          std::cout << " tick_" << kind_name(k) << "_ns="
                    << (kind_ticks ? kind_ns[k] / kind_ticks : 0);
          k++;
      } }
    std::cout
              // MUST be 0. The harness treats anything else as a FAIL.
              << " walk_violations=" << walk_violations
              << std::endl;
    return 0;
}

static void usage()
{
    std::cerr << "usage: benchmark_min_network_world --datapack DIR "
                 "[--players N]... [--ms BUDGET_MS | --ticks T]\n"
                 "\n"
                 "The WORKLOAD IS FIXED (world shape, move/turn/migrate rates,\n"
                 "seed): a benchmark with knobs is not comparable with its own\n"
                 "history. The only arguments are which of the fixed player\n"
                 "counts to run and how the run is bounded:\n"
                 "  --datapack DIR  the datapack whose " WORLD_MAP_SUBDIR " map set\n"
                 "                  IS the world (required). WHERE the fixed\n"
                 "                  workload lives, not what it is.\n"
                 "  --players N     run only this count; repeatable. Must be one\n"
                 "                  of the fixed sweep: 50 250 1000 2500 5000.\n"
                 "                  Default: all of them, in that order.\n"
                 "  --ms BUDGET_MS  fixed-time: run each count for BUDGET_MS and\n"
                 "                  report ticks completed (default, 2000 ms)\n"
                 "  --ticks T       fixed-iteration: exactly T ticks. Only for\n"
                 "                  callgrind, whose metric is a deterministic\n"
                 "                  instruction count a wall budget would blur."
              << std::endl;
}

// The fixed sweep. 50 fits in L1; 1000 already fills town maps to the owner's
// 200-per-map target; 5000 is ~120 maps and ~14 MB resident, which is the
// large end that still fits the 52 MB of the smallest fleet node (the
// population is NOT limited by the 254 wire ceiling -- that is per map -- but
// by what every node in the fleet can hold).
static const unsigned int PLAYER_COUNTS[] = { 50, 250, 1000, 2500, 5000 };
#define PLAYER_COUNTS_SIZE (sizeof(PLAYER_COUNTS)/sizeof(PLAYER_COUNTS[0]))

static bool is_known_count(unsigned int n)
{
    size_t i = 0;
    while(i < PLAYER_COUNTS_SIZE)
    {
        if(PLAYER_COUNTS[i] == n) return true;
        i++;
    }
    return false;
}

int main(int argc, char **argv)
{
    // The production MapVisibilityAlgorithm.cpp writes to std::cerr
    // unconditionally; redirecting is a no-op on the algorithm but stops pipe
    // back-pressure from skewing the timing.
    std::cerr.setstate(std::ios_base::badbit);

    std::vector<unsigned int> players_list;
    unsigned int ticks     = 0;    // >0 => fixed-iteration mode
    uint64_t     budget_ms = 0;    // >0 => fixed-time mode
    std::string  datapack;         // WHERE the world is, not WHAT it is

    int i = 1;
    while(i < argc)
    {
        const std::string a = argv[i];
        if(a == "--players" && i + 1 < argc)
        {
            const unsigned int n = (unsigned int)std::atoi(argv[++i]);
            if(!is_known_count(n))
            {
                // Refusing an unknown count is the point: it is the only way a
                // caller could otherwise change the workload from outside.
                std::cerr.clear();
                std::cerr << "--players " << n << " is not part of the fixed "
                             "sweep (50 250 1000 2500 5000)" << std::endl;
                return 2;
            }
            players_list.push_back(n);
        }
        else if(a == "--ms" && i + 1 < argc)    { budget_ms = std::strtoull(argv[++i], NULL, 10); }
        else if(a == "--ticks" && i + 1 < argc) { ticks = (unsigned int)std::atoi(argv[++i]); }
        else if(a == "--datapack" && i + 1 < argc) { datapack = argv[++i]; }
        else if(a == "-h" || a == "--help")     { std::cerr.clear(); usage(); return 0; }
        else                                    { std::cerr.clear(); usage(); return 2; }
        i++;
    }
    if(players_list.empty())
    {
        size_t k = 0;
        while(k < PLAYER_COUNTS_SIZE) { players_list.push_back(PLAYER_COUNTS[k]); k++; }
    }
    if(budget_ms == 0 && ticks == 0) budget_ms = 2000;
    if(datapack.empty())
    {
        std::cerr.clear();
        std::cerr << "--datapack DIR is required (the world this benchmark "
                     "walks is the datapack's generated map set)" << std::endl;
        return 2;
    }
    if(datapack.at(datapack.size() - 1) != '/')
        datapack += '/';

    // Load the world ONCE for the whole process: every scenario re-populates
    // it with a different number of players, but the maps themselves -- the
    // fixed part of the workload -- are parsed a single time.
    std::vector<LoadedMap*> loaded;
    unsigned int failed = 0, typed = 0;
    std::string error;
    const std::chrono::steady_clock::time_point load0 = std::chrono::steady_clock::now();
    if(!load_world(datapack, WORLD_MAP_SUBDIR, loaded, failed, typed, error))
    {
        std::cerr.clear();
        std::cerr << "cannot load the world: " << error << std::endl;
        return 2;
    }
    const std::chrono::steady_clock::time_point load1 = std::chrono::steady_clock::now();
    uint64_t world_cells = 0, world_walkable = 0;
    unsigned int per_kind[Kind_count] = {0, 0, 0};
    size_t li = 0;
    while(li < loaded.size())
    {
        world_cells    += loaded[li]->map.flat_simplified_map.size();
        world_walkable += loaded[li]->walkable_cells;
        per_kind[loaded[li]->kind]++;
        li++;
    }
    // The world IS the workload, so its shape is printed once: a different
    // datapack (or a regenerated map set) shows up here instead of silently
    // shifting every number below.
    std::cout << "WORLD maps=" << loaded.size()
              << " failed=" << failed
              // How many maps stated their own kind, rather than being guessed
              // from the path: on this world it should be all of them.
              << " typed=" << typed
              << " load_ms=" << std::chrono::duration_cast<std::chrono::milliseconds>(load1 - load0).count()
              << " cells=" << world_cells
              << " walkable_cells=" << world_walkable;
    { unsigned int k = 0;
      while(k < Kind_count)
      {
          std::cout << " maps_" << kind_name(k) << "=" << per_kind[k];
          k++;
      } }
    std::cout << std::endl;

    int rc = 0;
    size_t pi = 0;
    while(pi < players_list.size())
    {
        rc |= run_scenario(players_list[pi], ticks, budget_ms, loaded);
        pi++;
    }
    li = 0;
    while(li < loaded.size()) { delete loaded[li]; li++; }
    return rc;
}
