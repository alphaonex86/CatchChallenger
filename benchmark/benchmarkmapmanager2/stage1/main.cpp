// HEADLESS: yes
// Benchmark: MapVisibilityAlgorithm::min_balanced() over the REAL WORLD -- the
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
//   bytes_sent                lower-is-better (what min_balanced exists to cut)
//   visibility_state_bytes    lower-is-better (resident per-map diff state)
//   sampled_changed/sampled_slots  the share of slots that differ from the
//                             previous broadcast, measured on the sampled
//                             ticks. This is the workload's most important
//                             property: what min_balanced's stateful diff gets
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
// min_balanced, not the movement simulation.
//
// Determinism: seeded LCG only -- no rand(), no clock, no time in the workload,
//              and the map list is sorted before use (readdir order is not
//              stable across machines and the map index decides where players
//              land). Same binary + same datapack => same world, same walk,
//              same migrations on every arch/libc/compiler. The steady_clock
//              reads are the only nondeterminism and never feed the workload.

// Stage 1 links only the production datapack/map layer: it reads the world and
// writes a workload, it never broadcasts anything, so no server stub world and
// no MapVisibilityAlgorithm here.
#include "../../../general/base/Map_loader.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include "../../../general/base/FacilityLibGeneral.hpp"
#include "../../../general/base/MoveOnTheMap.hpp"

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
// half of min_balanced() that matters most: the slots that did NOT change.
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

// STRICT per-map guard. The wire slot index is 8-bit with 255 reserved, and
// min_balanced() clamps its client count to 254; 253 leaves a slot of headroom
// so a map can never sit exactly on the clamp. The POPULATION has no such
// limit -- it is bounded by the node's RAM and by 65530 (the connected-player
// index is 16-bit) -- players simply occupy more maps.
#define MAP_PLAYER_CAP 253

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
    uint32_t players;          // how many were placed here

    LoadedMap() : kind(Kind_outdoor), walkable_cells(0), players(0) {}
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
    std::cerr.setstate(std::ios_base::badbit);
    CommonDatapack::commonDatapack.parseDatapack(datapack);
    const std::string map_path = datapack + map_subdir;
    std::vector<std::string> files = FacilityLibGeneral::listFolder(map_path);
    if(files.empty())
    {
        std::cout.clear();
        std::cerr.clear();
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
    std::cerr.clear();
    if(out.empty())
    {
        error = "no .tmx loaded under " + map_path;
        return false;
    }
    return true;
}


static Direction move_of(uint8_t facing)
{
    if(facing == 0) return Direction_move_at_top;
    if(facing == 1) return Direction_move_at_right;
    if(facing == 2) return Direction_move_at_bottom;
    return Direction_move_at_left;
}

// How far a player at (x,y) can walk in `facing`, capped at `want`: the
// movement VECTOR truncated at the first obstacle. The test is the PRODUCTION
// predicate (collisions, one-way ledges, map border), so every vector this
// generator emits is one the game itself would allow -- which is why stage 2
// can replay them with no collision data at all.
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

// ---------------------------------------------------------------------------
// Placement -- who stands where when the replay starts
// ---------------------------------------------------------------------------
// Population split by kind weight, then a crowd per map: the owner's target
// first (200 in a town, 35 on a route, 20 indoors) and, only when the node can
// hold more players than that shape has room for, up to the hard per-map guard.
struct SimPlayer
{
    CATCHCHALLENGER_TYPE_MAPID map;   // index into the loaded map list
    uint8_t x, y;
    uint8_t facing;                   // 0=top 1=right 2=bottom 3=left

    SimPlayer() : map(0), x(0), y(0), facing(0) {}
};

// Maps of one kind, in load order, plus how many players each holds.
struct KindMaps
{
    std::vector<unsigned int> index;      // into the loaded map list
    std::vector<unsigned int> population;
};

// How many players this world can hold at all: every map filled to the hard
// guard, but still respecting the kind weights (the scarcest kind binds).
static uint32_t world_capacity(const KindMaps *km, const KindCfg *cfg)
{
    uint32_t total_weight = 0;
    unsigned int k = 0;
    while(k < Kind_count) { total_weight += cfg[k].weight; k++; }
    if(total_weight == 0) return 0;
    uint32_t binding = 0xffffffffu;
    k = 0;
    while(k < Kind_count)
    {
        if(cfg[k].weight > 0)
        {
            const uint64_t cap = (uint64_t)km[k].index.size() * MAP_PLAYER_CAP;
            const uint32_t allows = (uint32_t)(cap * total_weight / cfg[k].weight);
            if(allows < binding) binding = allows;
        }
        k++;
    }
    return binding;
}

// Group the loaded maps by kind, in load order (deterministic).
static void group_by_kind(const std::vector<LoadedMap*> &loaded, KindMaps *km)
{
    size_t i = 0;
    while(i < loaded.size())
    {
        km[loaded[i]->kind].index.push_back((unsigned int)i);
        i++;
    }
    unsigned int k = 0;
    while(k < Kind_count)
    {
        km[k].population.assign(km[k].index.size(), 0);
        k++;
    }
}

// Spread `players` over the world and give each one a standable spawn cell.
// Returns the number actually placed (a map with no free floor is skipped).
static uint32_t place_players(uint32_t players, const std::vector<LoadedMap*> &loaded,
                              KindMaps *km, const KindCfg *cfg, Lcg &rng,
                              std::vector<SimPlayer> &out)
{
    uint32_t total_weight = 0;
    unsigned int k = 0;
    while(k < Kind_count) { total_weight += cfg[k].weight; k++; }
    if(total_weight == 0) return 0;

    uint32_t of_kind[Kind_count];
    uint32_t assigned = 0;
    k = 0;
    while(k < Kind_count)
    {
        of_kind[k] = (uint32_t)((uint64_t)players * cfg[k].weight / total_weight);
        assigned += of_kind[k];
        k++;
    }
    unsigned int biggest = 0;
    k = 1;
    while(k < Kind_count) { if(cfg[k].weight > cfg[biggest].weight) biggest = k; k++; }
    of_kind[biggest] += players - assigned;

    out.reserve(players);
    k = 0;
    while(k < Kind_count)
    {
        const unsigned int nmaps = (unsigned int)km[k].index.size();
        if(of_kind[k] > 0 && nmaps > 0)
        {
            // Crowd per map: the owner's target, raised only if this node holds
            // more players than the target shape has room for, and never past
            // the hard guard.
            unsigned int crowd = cfg[k].occupancy;
            if(crowd == 0) crowd = 1;
            if((uint64_t)crowd * nmaps < of_kind[k])
                crowd = (unsigned int)((of_kind[k] + nmaps - 1) / nmaps);
            if(crowd > MAP_PLAYER_CAP) crowd = MAP_PLAYER_CAP;
            unsigned int need = (unsigned int)((of_kind[k] + crowd - 1) / crowd);
            if(need > nmaps) need = nmaps;
            // Which maps carry the crowd: a deterministic random subset, so it
            // is not always the same corner of the world.
            std::vector<unsigned int> pool = km[k].index;
            unsigned int i = 0;
            while(i < need)
            {
                const unsigned int j = i + rng.below((uint32_t)(pool.size() - i));
                const unsigned int tmp = pool[i]; pool[i] = pool[j]; pool[j] = tmp;
                i++;
            }
            pool.resize(need);
            uint32_t placed_here = 0;
            while(placed_here < of_kind[k])
            {
                // A map with room, drawn at random (crowds vary map to map the
                // way a real server's do), then a scan when the draw keeps
                // landing on full ones.
                int chosen = -1;
                unsigned int tries = 0;
                while(tries < 8 && chosen < 0)
                {
                    const unsigned int c = pool[rng.below((uint32_t)pool.size())];
                    if(loaded[c]->players < crowd && loaded[c]->players < loaded[c]->walkable_cells)
                        chosen = (int)c;
                    tries++;
                }
                if(chosen < 0)
                {
                    i = 0;
                    while(i < pool.size() && chosen < 0)
                    {
                        const unsigned int c = pool[i];
                        if(loaded[c]->players < crowd && loaded[c]->players < loaded[c]->walkable_cells)
                            chosen = (int)c;
                        i++;
                    }
                }
                if(chosen < 0)
                    break;                     // this kind is full: stop here
                SimPlayer p;
                if(!find_spawn(loaded[chosen]->map, rng, p.x, p.y))
                    break;
                p.map    = (CATCHCHALLENGER_TYPE_MAPID)chosen;
                p.facing = rng.pick4();
                loaded[chosen]->players++;
                out.push_back(p);
                placed_here++;
            }
        }
        k++;
    }
    return (uint32_t)out.size();
}

// ---------------------------------------------------------------------------
// Replay generation
// ---------------------------------------------------------------------------
// One byte per entry: direction in the top 3 bits, length-1 in the low 5.
//   dir 0        stand still for `len` ticks, keeping the direction faced
//   dir 1..4     walk `len` cells (one per tick) top/right/bottom/left
// A move entry is only ever emitted for a vector the PRODUCTION predicate
// accepts cell by cell, so the replay can never walk into a wall and stage 2
// needs no collision data at all -- which is what makes it small enough for a
// board with a few hundred KB of flash.
#define ENTRY_MAX_LEN 32
#define ENTRY(dir, len) ((uint8_t)(((uint8_t)(dir) << 5) | (uint8_t)((len) - 1)))

struct Migration
{
    uint32_t tick;
    uint32_t player;
    uint16_t map;
    uint8_t x, y;
};

// Simulate the world for `ticks` ticks and record, per player, the vector list
// that reproduces exactly that walk. Migrations are simulated too, so the
// vectors that follow one are validated against the map the player moved TO.
// Returns the largest number of entries any single player needed.
static uint32_t simulate(uint32_t ticks, std::vector<SimPlayer> &players,
                         const std::vector<LoadedMap*> &loaded, KindMaps *km,
                         const KindCfg *cfg, uint32_t migrate_thr, Lcg &rng,
                         std::vector<std::vector<uint8_t> > &streams,
                         std::vector<Migration> &migrations)
{
    const uint32_t start_thr = pct_threshold(WALK_START_PCT);
    streams.assign(players.size(), std::vector<uint8_t>());
    migrations.clear();
    // Per-player countdown of the entry being walked, and its direction.
    std::vector<uint8_t> steps_left(players.size(), 0);
    std::vector<uint8_t> idle_run(players.size(), 0);   // ticks already idled

    uint32_t total_weight = 0;
    unsigned int wk = 0;
    while(wk < Kind_count) { total_weight += cfg[wk].weight; wk++; }

    uint32_t t = 0;
    while(t < ticks)
    {
        size_t i = 0;
        while(i < players.size())
        {
            SimPlayer &p = players[i];
            if(steps_left[i] > 0)
            {
                // Mid-vector: one cell along the direction already validated.
                if(p.facing == 0)      p.y--;
                else if(p.facing == 1) p.x++;
                else if(p.facing == 2) p.y++;
                else                   p.x--;
                steps_left[i]--;
            }
            else if(rng.next() >= start_thr)
            {
                // Standing still. Merge consecutive idle ticks into one entry
                // until it reaches the 5-bit length limit.
                std::vector<uint8_t> &st = streams[i];
                if(idle_run[i] > 0 && idle_run[i] < ENTRY_MAX_LEN && !st.empty()
                   && (st.back() >> 5) == 0)
                {
                    idle_run[i]++;
                    st.back() = ENTRY(0, idle_run[i]);
                }
                else
                {
                    idle_run[i] = 1;
                    st.push_back(ENTRY(0, 1));
                }
            }
            else
            {
                // A new vector: try directions until one has room, so a player
                // against a wall turns and walks instead of standing there.
                idle_run[i] = 0;
                uint8_t len = 0;
                unsigned int tries = 0;
                while(tries < 4 && len == 0)
                {
                    const uint8_t facing = rng.pick4();
                    const uint8_t want = (uint8_t)(1u + (rng.next() >> 29));   // 1..8
                    const uint8_t free_len = free_run_length(loaded[p.map]->map,
                                                             p.x, p.y, facing, want);
                    if(free_len > 0)
                    {
                        p.facing = facing;
                        len = free_len;
                    }
                    tries++;
                }
                std::vector<uint8_t> &st = streams[i];
                if(len == 0)
                {
                    idle_run[i] = 1;
                    st.push_back(ENTRY(0, 1));         // boxed in: stand a tick
                }
                else
                {
                    st.push_back(ENTRY(p.facing + 1, len));
                    // its first cell is walked on this same tick
                    if(p.facing == 0)      p.y--;
                    else if(p.facing == 1) p.x++;
                    else if(p.facing == 2) p.y++;
                    else                   p.x--;
                    steps_left[i] = (uint8_t)(len - 1);
                }
            }
            i++;
        }
        // A player walks through a border into another map. Scheduled here so
        // stage 2 only has to apply it, and so the vectors that follow are
        // generated against the map the player lands on.
        if(!players.empty() && rng.next() < migrate_thr)
        {
            const uint32_t pi = rng.below((uint32_t)players.size());
            SimPlayer &p = players[pi];
            uint32_t roll = rng.below(total_weight);
            unsigned int k = 0;
            while(k + 1 < Kind_count && roll >= cfg[k].weight) { roll -= cfg[k].weight; k++; }
            if(!km[k].index.empty())
            {
                const unsigned int dest = km[k].index[rng.below((uint32_t)km[k].index.size())];
                uint8_t nx = 0, ny = 0;
                if(dest != p.map && loaded[dest]->players < MAP_PLAYER_CAP
                   && find_spawn(loaded[dest]->map, rng, nx, ny))
                {
                    loaded[p.map]->players--;
                    loaded[dest]->players++;
                    Migration m;
                    m.tick = t; m.player = pi;
                    m.map = (CATCHCHALLENGER_TYPE_MAPID)dest; m.x = nx; m.y = ny;
                    migrations.push_back(m);
                    p.map = (CATCHCHALLENGER_TYPE_MAPID)dest;
                    p.x = nx; p.y = ny;
                    steps_left[pi] = 0;      // its run belongs to the old map
                    idle_run[pi] = 0;
                }
            }
        }
        t++;
    }
    uint32_t worst = 0;
    size_t i = 0;
    while(i < streams.size())
    {
        if(streams[i].size() > worst) worst = (uint32_t)streams[i].size();
        i++;
    }
    return worst;
}

// ---------------------------------------------------------------------------
// Emission -- the generated stage-2 workload
// ---------------------------------------------------------------------------
static void emit_u8(FILE *f, const char *type, const char *name, size_t count,
                    const uint8_t *data)
{
    std::fprintf(f, "extern const %s %s[%u];\nconst %s %s[%u] = {",
                 type, name, (unsigned int)count, type, name, (unsigned int)count);
    size_t i = 0;
    while(i < count)
    {
        if((i % 24) == 0) std::fprintf(f, "\n");
        std::fprintf(f, "%u,", (unsigned int)data[i]);
        i++;
    }
    std::fprintf(f, "\n};\n");
}

static void emit_u16(FILE *f, const char *name, const std::vector<uint16_t> &v)
{
    std::fprintf(f, "extern const uint16_t %s[%u];\nconst uint16_t %s[%u] = {",
                 name, (unsigned int)v.size(), name, (unsigned int)v.size());
    size_t i = 0;
    while(i < v.size())
    {
        if((i % 16) == 0) std::fprintf(f, "\n");
        std::fprintf(f, "%u,", (unsigned int)v[i]);
        i++;
    }
    std::fprintf(f, "\n};\n");
}

static void emit_u32(FILE *f, const char *name, const std::vector<uint32_t> &v)
{
    std::fprintf(f, "extern const uint32_t %s[%u];\nconst uint32_t %s[%u] = {",
                 name, (unsigned int)v.size(), name, (unsigned int)v.size());
    size_t i = 0;
    while(i < v.size())
    {
        if((i % 12) == 0) std::fprintf(f, "\n");
        std::fprintf(f, "%uu,", (unsigned int)v[i]);
        i++;
    }
    std::fprintf(f, "\n};\n");
}

static void usage()
{
    std::cerr << "usage: benchmark_world_stage1 --datapack DIR --out FILE.cpp\n"
                 "                              --max-players N [--node LABEL]\n"
                 "                              [--replay-bytes N]\n"
                 "\n"
                 "Stage 1 of benchmarkmapmanager2: read the datapack's world, decide\n"
                 "how many players this node can hold, place them, and WRITE THE\n"
                 "WORKLOAD as a C++ source file for stage 2 to compile in. Nothing\n"
                 "here is measured -- it runs on the orchestrating host.\n"
                 "  --datapack DIR    the world (its " WORLD_MAP_SUBDIR " map set)\n"
                 "  --max-players N   ceiling for this node, from its RAM. Clamped to\n"
                 "                    65530 (16-bit connected-player index) and to what\n"
                 "                    the world can hold at " "253" " players per map.\n"
                 "  --out FILE.cpp    where to write the generated workload\n"
                 "  --node LABEL      stamped into the file for provenance\n"
                 "  --replay-bytes N  byte budget for the replay table (default 1 MiB).\n"
                 "                    It sets how long the replay runs before it loops:\n"
                 "                    too small resets constantly, too large bloats the\n"
                 "                    binary a small board has to hold."
              << std::endl;
}

int main(int argc, char **argv)
{
    std::string datapack, out_path, node_label = "unknown";
    uint32_t max_players = 0;
    uint32_t replay_bytes = 1u << 20;

    int i = 1;
    while(i < argc)
    {
        const std::string a = argv[i];
        if(a == "--datapack" && i + 1 < argc)          { datapack = argv[++i]; }
        else if(a == "--out" && i + 1 < argc)          { out_path = argv[++i]; }
        else if(a == "--node" && i + 1 < argc)         { node_label = argv[++i]; }
        else if(a == "--max-players" && i + 1 < argc)  { max_players = (uint32_t)std::strtoul(argv[++i], NULL, 10); }
        else if(a == "--replay-bytes" && i + 1 < argc) { replay_bytes = (uint32_t)std::strtoul(argv[++i], NULL, 10); }
        else if(a == "-h" || a == "--help")            { usage(); return 0; }
        else                                           { usage(); return 2; }
        i++;
    }
    if(datapack.empty() || out_path.empty() || max_players == 0)
    {
        usage();
        return 2;
    }
    if(datapack.at(datapack.size() - 1) != '/')
        datapack += '/';

    std::vector<LoadedMap*> loaded;
    unsigned int failed = 0, typed = 0;
    std::string error;
    if(!load_world(datapack, WORLD_MAP_SUBDIR, loaded, failed, typed, error))
    {
        std::cerr.clear();
        std::cerr << "stage1: cannot load the world: " << error << std::endl;
        return 2;
    }

    KindMaps km[Kind_count];
    group_by_kind(loaded, km);

    // What this node may hold: its RAM ceiling, the 16-bit connected-player
    // index, and what the world itself can take at 253 players per map.
    const uint32_t hard_cap = 65530u;
    uint32_t players = max_players;
    if(players > hard_cap) players = hard_cap;
    const uint32_t capacity = world_capacity(km, WORLD);
    if(players > capacity) players = capacity;

    Lcg rng(WORLD_SEED);
    std::vector<SimPlayer> sim;
    players = place_players(players, loaded, km, WORLD, rng, sim);
    uint32_t of_kind_placed[Kind_count] = {0, 0, 0};
    { size_t i = 0;
      while(i < sim.size()) { of_kind_placed[loaded[sim[i].map]->kind]++; i++; } }
    if(players == 0)
    {
        std::cerr.clear();
        std::cerr << "stage1: no player could be placed" << std::endl;
        return 2;
    }

    // How long the replay runs before it loops, and therefore how many
    // entries each player carries. Two failure modes to stay between: a short
    // list resets constantly (every reset is a tick where the whole world
    // jumps home, and enough of them poison the tail latency), a long one
    // bloats a table the binary has to carry -- and a small board has little
    // room for software. So: aim at a cycle, and shorten it only if the bytes
    // do not fit.
    //
    // The busiest player is what sizes the table (every player carries the
    // same number of entries), so the shape of the walk decides the ratio and
    // it is MEASURED here rather than assumed: probe once, then aim.
    std::vector<std::vector<uint8_t> > streams;
    std::vector<Migration> migrations;
    std::vector<SimPlayer> spawn = sim;
    const uint32_t migrate_thr = pct_threshold(MIGRATE_PCT);
    uint32_t affordable = replay_bytes / players;
    if(affordable < 16)   affordable = 16;      // a shorter list is all resets
    if(affordable > 1024) affordable = 1024;    // past this the table is waste

    uint32_t cycle_ticks = 200;                 // probe
    uint32_t worst = 0;
    // What the streams and the migration schedule below actually cover. The
    // loop may leave `cycle_ticks` holding an aim it never got to simulate
    // (it runs out of attempts), and emitting THAT would hand stage 2 a cycle
    // length its vectors do not match.
    uint32_t simulated = 0;
    unsigned int attempt = 0;
    while(attempt < 5)
    {
        sim = spawn;
        size_t r = 0;
        while(r < loaded.size()) { loaded[r]->players = 0; r++; }
        r = 0;
        while(r < sim.size()) { loaded[sim[r].map]->players++; r++; }
        Lcg sim_rng(WORLD_SEED + 1u);
        worst = simulate(cycle_ticks, sim, loaded, km, WORLD, migrate_thr,
                         sim_rng, streams, migrations);
        simulated = cycle_ticks;
        if(worst == 0)
            break;
        // Ticks per entry, measured on what was just simulated.
        const uint64_t aim = (uint64_t)cycle_ticks * affordable / worst;
        uint32_t next = (uint32_t)(aim > 8 ? aim : 8);
        if(next > 4000) next = 4000;            // long enough for any node
        // Converged: it fits and asking for more would not change the aim.
        if(worst <= affordable && (next <= cycle_ticks || attempt >= 3))
            break;
        if(next == cycle_ticks)
            break;
        cycle_ticks = next;
        attempt++;
    }
    // Emit the window that was simulated, never the one that was only aimed at.
    cycle_ticks = simulated;
    if(worst > affordable || cycle_ticks == 0)
    {
        std::cerr.clear();
        std::cerr << "stage1: cannot fit the replay in " << affordable
                  << " entries per player" << std::endl;
        return 2;
    }
    // The table only needs the entries the busiest player used.
    const uint32_t entries = worst > 0 ? worst : 1;
    // ---- write the workload ------------------------------------------------
    FILE *f = std::fopen(out_path.c_str(), "w");
    if(f == NULL)
    {
        std::cerr.clear();
        std::cerr << "stage1: cannot write " << out_path << std::endl;
        return 2;
    }
    std::fprintf(f,
        "// GENERATED by benchmark/benchmarkmapmanager2/stage1 -- do not edit.\n"
        "//\n"
        "// The workload of one benchmark run: which maps exist, how many players\n"
        "// there are, where they start, and the movement vectors they replay. Every\n"
        "// vector was validated against the datapack's real collisions by the\n"
        "// PRODUCTION predicate at generation time, so stage 2 replays it with no\n"
        "// map data and no collision test at all.\n"
        "//\n"
        "// It is specific to BOTH the execution node (its RAM decides the player\n"
        "// count) and the input datapack (its maps are the world).\n"
        "//   node:     %s\n"
        "//   datapack: %s\n"
        "//   maps:     %u loaded, %u failed, %u typed by the datapack\n"
        "#include <stdint.h>\n"
        "namespace CCBenchWorkload {\n",
        node_label.c_str(), datapack.c_str(),
        (unsigned int)loaded.size(), failed, typed);

    std::fprintf(f, "extern const char NODE[];\nconst char NODE[] = \"%s\";\n", node_label.c_str());
    std::fprintf(f, "extern const char DATAPACK[];\nconst char DATAPACK[] = \"%s\";\n", datapack.c_str());
    std::fprintf(f, "extern const uint32_t PLAYERS;\nconst uint32_t PLAYERS = %uu;\n", players);
    std::fprintf(f, "extern const uint32_t CYCLE_TICKS;\nconst uint32_t CYCLE_TICKS = %uu;\n", cycle_ticks);
    std::fprintf(f, "extern const uint32_t ENTRIES_PER_PLAYER;\nconst uint32_t ENTRIES_PER_PLAYER = %uu;\n", entries);
    std::fprintf(f, "extern const uint16_t MAPS;\nconst uint16_t MAPS = %uu;\n", (unsigned int)loaded.size());
    // ORACLE. Where every player stands after one full cycle, hashed. Stage 2
    // replays the same vectors and must land on the same state: a mismatch
    // means its decoding drifted from what was generated -- and since the
    // generator validated every vector against the real collisions, a matching
    // hash is also proof that no replayed player walked into a wall.
    {
        uint64_t h = 1469598103934665603ull;
        size_t p = 0;
        while(p < sim.size())
        {
            h ^= sim[p].map;    h *= 1099511628211ull;
            h ^= sim[p].x;      h *= 1099511628211ull;
            h ^= sim[p].y;      h *= 1099511628211ull;
            h ^= sim[p].facing; h *= 1099511628211ull;
            p++;
        }
        std::fprintf(f, "extern const uint64_t CYCLE_END_HASH;\nconst uint64_t CYCLE_END_HASH = %lluull;\n",
                     (unsigned long long)h);
    }

    // Map dimensions: all stage 2 needs of the world (the x/y range guard).
    {
        std::vector<uint8_t> w, h;
        w.reserve(loaded.size()); h.reserve(loaded.size());
        size_t m = 0;
        while(m < loaded.size())
        {
            w.push_back(loaded[m]->map.width);
            h.push_back(loaded[m]->map.height);
            m++;
        }
        // Dimensions only: the map KIND is a stage-1 notion -- it decides how
        // many players spawn on each map -- and stage 2 has no use for it.
        emit_u8(f, "uint8_t", "MAP_W", w.size(), w.data());
        emit_u8(f, "uint8_t", "MAP_H", h.size(), h.data());
    }
    // Where everyone starts -- and where a reset puts them back.
    {
        std::vector<uint16_t> smap;
        std::vector<uint8_t> sx, sy, sf;
        smap.reserve(players); sx.reserve(players); sy.reserve(players); sf.reserve(players);
        size_t p = 0;
        while(p < spawn.size())
        {
            smap.push_back(spawn[p].map);
            sx.push_back(spawn[p].x);
            sy.push_back(spawn[p].y);
            sf.push_back(spawn[p].facing);
            p++;
        }
        emit_u16(f, "SPAWN_MAP", smap);
        emit_u8(f, "uint8_t", "SPAWN_X", sx.size(), sx.data());
        emit_u8(f, "uint8_t", "SPAWN_Y", sy.size(), sy.data());
        emit_u8(f, "uint8_t", "SPAWN_FACING", sf.size(), sf.data());
    }
    // The replay itself: ENTRIES_PER_PLAYER bytes per player, player-major.
    // dir = byte>>5 (0 stand, 1..4 top/right/bottom/left), len = (byte&31)+1.
    {
        std::vector<uint8_t> flat((size_t)players * entries, ENTRY(0, 1));
        size_t p = 0;
        while(p < streams.size())
        {
            size_t e = 0;
            while(e < streams[p].size() && e < entries)
            {
                flat[p * entries + e] = streams[p][e];
                e++;
            }
            p++;
        }
        emit_u8(f, "uint8_t", "REPLAY", flat.size(), flat.data());
    }
    // Map changes, applied by tick: this is what makes a client take PATH 1
    // (drop-all + full re-insert) on its new map, which no amount of walking
    // does. Sorted by tick, as generated.
    {
        std::vector<uint32_t> mt, mp;
        std::vector<uint16_t> mm;
        std::vector<uint8_t> mx, my;
        size_t g = 0;
        while(g < migrations.size())
        {
            mt.push_back(migrations[g].tick);
            mp.push_back(migrations[g].player);
            mm.push_back(migrations[g].map);
            mx.push_back(migrations[g].x);
            my.push_back(migrations[g].y);
            g++;
        }
        std::fprintf(f, "extern const uint32_t MIGRATIONS;\nconst uint32_t MIGRATIONS = %uu;\n", (unsigned int)mt.size());
        if(mt.empty())
        {
            std::fprintf(f, "extern const uint32_t MIG_TICK[1];\n"
                            "const uint32_t MIG_TICK[1] = {0u};\n"
                            "extern const uint32_t MIG_PLAYER[1];\n"
                            "const uint32_t MIG_PLAYER[1] = {0u};\n"
                            "extern const uint16_t MIG_MAP[1];\n"
                            "const uint16_t MIG_MAP[1] = {0};\n"
                            "extern const uint8_t MIG_X[1];\n"
                            "const uint8_t MIG_X[1] = {0};\n"
                            "extern const uint8_t MIG_Y[1];\n"
                            "const uint8_t MIG_Y[1] = {0};\n");
        }
        else
        {
            emit_u32(f, "MIG_TICK", mt);
            emit_u32(f, "MIG_PLAYER", mp);
            emit_u16(f, "MIG_MAP", mm);
            emit_u8(f, "uint8_t", "MIG_X", mx.size(), mx.data());
            emit_u8(f, "uint8_t", "MIG_Y", my.size(), my.data());
        }
    }
    std::fprintf(f, "}\n");
    std::fclose(f);

    const uint64_t bytes = (uint64_t)players * entries
                           + (uint64_t)players * 5 + (uint64_t)loaded.size() * 2
                           + (uint64_t)migrations.size() * 11;
    std::cout << "STAGE1"
              << " node=" << node_label
              << " players=" << players
              << " maps=" << loaded.size();
    { unsigned int k = 0;
      while(k < Kind_count)
      {
          std::cout << " maps_" << kind_name(k) << "=" << km[k].index.size()
                    << " players_" << kind_name(k) << "=" << of_kind_placed[k];
          k++;
      } }
    std::cout
              << " cycle_ticks=" << cycle_ticks
              << " entries_per_player=" << entries
              << " migrations=" << migrations.size()
              << " workload_bytes=" << bytes
              << " out=" << out_path
              << std::endl;
    size_t d = 0;
    while(d < loaded.size()) { delete loaded[d]; d++; }
    return 0;
}
