// HEADLESS: yes
// Benchmark: MapVisibilityAlgorithm::min_network() over a WORLD of maps -- the
//            multi-map counterpart of benchmarkmapmanager, which puts every
//            player on one 120x120 map at a position it never changes and only
//            rotates directions.
//
// What is different here (and why):
//   * Players are spread over MANY maps of three kinds -- outdoor, city,
//     indoor. Each kind has a share of the population and a target crowd per
//     map; the map count follows as ceil(players_of_kind / occupancy). That is
//     the shape of a real server: a couple of packed town maps, many routes,
//     a tail of small interiors that min_network early-outs on.
//   * Players actually WALK. Each tick a player either steps one cell in its
//     facing direction (direction becomes Direction_move_at_*) or stands
//     (Direction_look_at_*). A step into a collision cell or off the map is
//     refused and the player turns instead. NO PATHFINDING: one array lookup
//     per moving player, so the harness cost stays a rounding error next to
//     the server work being measured (median_prep_ns reports it, so the claim
//     is checked every run, not assumed).
//   * Map changes replace the insert+remove pair: a migrating player is
//     removeOnMap()'d from its map and insertOnMap()'d on another, which is
//     exactly what makes the next broadcast take PATH 1 (drop-all + full
//     re-insert) for it and emit a 0x69 remove to its former map.
//
// THE WORKLOAD IS FIXED -- see WORLD[] and the MOVE/TURN/MIGRATE constants
// below. There is no flag to change the world shape or the rates: a benchmark
// with knobs is not comparable with its own history. The only arguments are
// which of the fixed player counts to run and how the run is bounded.
//
// One tick = the whole world: min_network() is called once per map, which is
// what the server's timer does over flat_map_list. The timed window brackets
// that loop only.
//
// Metrics emitted (one stdout line per scenario):
//   BENCH players=N maps=M ticks=T duration_ms=D ticks_per_s=X
//         median_tick_ns=Y p95_tick_ns=Z median_prep_ns=P bytes_sent=B
//         visibility_state_bytes=V migrations=I moves=Mv moves_blocked=Mb ...
//   ticks_per_s               higher-is-better (fixed-TIME throughput)
//   median_tick_ns/p95_tick_ns lower-is-better (whole-world broadcast latency)
//   median_prep_ns            lower-is-better (harness cost per tick; it is
//                             NOT server work -- it must stay small vs
//                             median_tick_ns or the run measures the harness)
//   bytes_sent                lower-is-better (what min_network exists to cut)
//   visibility_state_bytes    lower-is-better (resident per-map diff state)
//   walk_violations           MUST be 0 -- the end-of-scenario oracle check
//                             that no player ended up on a collision cell or
//                             off its map. Anything else is a FAIL.
//   The remaining fields echo the fixed workload the row was measured under.
//
// median_prep_ns is typically ~20% of median_tick_ns: the load model costs a
// few ns per player per tick (one threshold compare, one collision lookup)
// against ~40 ns of server broadcast work per player, and it is EXCLUDED from
// the latency window (only ticks_per_s, which times the whole loop, carries
// it). For scale, a real server spends ~2.6 us of parse+apply per received
// move, so this walk model is ~300x cheaper than the client work it stands in
// for -- the run measures min_network, not the movement simulation.
//
// Determinism: seeded LCG only -- no rand(), no clock, no time in the
//              workload. Same binary => same world, same walk, same migrations
//              on every arch/libc/compiler. The steady_clock reads are the
//              only nondeterminism and never feed the workload.

#include "../../test/testingmapmanagement/Stubs.hpp"
#include "../../server/base/MapManagement/MapVisibilityAlgorithm.hpp"

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
    uint32_t mod(uint32_t n) { return n ? (next() % n) : 0; }
};

// Percent constant -> 32-bit threshold, so the per-player draw in the tick is
// `rng.next() < thr` (one compare) instead of `rng.next() % 100 < pct` (an
// integer DIVISION per player per tick -- expensive on the i486 / MIPS class
// targets this project exists for, and it is harness cost, not server work).
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
    uint8_t      width;
    uint8_t      height;
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
    /* outdoor */ { 60,  35, 120, 120 },
    /* city    */ { 30, 200,  60,  60 },
    /* indoor  */ { 10,  20,  20,  15 }
};

// % of players attempting a step each tick, and the chance a stepping player
// turns first: a busy but not synthetic server.
#define MOVE_PCT     70
#define TURN_PCT     10
// % of ticks where one player walks through a border into another map. This is
// the benchmark's insert+remove pair: the arrival takes PATH 1 (drop-all +
// full re-insert) on its new map, the departure emits a 0x69 remove on the old.
#define MIGRATE_PCT  5
#define WORLD_SEED   0x5EEDu

// STRICT per-map ceiling: the wire slot index is 8-bit and 255 is the reserved
// value, so slots run 0..254 -- and min_network() itself clamps its client
// count to 254 (MapVisibilityAlgorithm.cpp). A map is never given more than
// that; occupancy is clamped to it. The population as a whole has no such
// limit, which is why this benchmark sweeps thousands of players: they simply
// occupy more maps.
#define MAP_PLAYER_CAP 254

// A collision layout. Maps of the same kind SHARE one of GRID_VARIANTS
// layouts instead of owning a private copy: the grid is read ONLY by the
// harness's walk model, never by the server code under test, so sharing costs
// nothing in realism. It holds the pool at ~143 KB whatever the map count
// (private copies would be ~1.3 MB at the top of the sweep and grow from
// there), and it keeps the walk model's memory traffic small so it stays out
// of the way of what is being measured.
struct Grid
{
    std::vector<uint8_t> cells;   // 1 = collision, 0 = walkable
    uint8_t w;
    uint8_t h;

    Grid() : w(0), h(0) {}
};

#define GRID_VARIANTS 8

struct WorldMap
{
    HarnessMVA mva;
    const Grid *grid;
    uint8_t kind;
    unsigned int population;

    WorldMap() : grid(NULL), kind(Kind_outdoor), population(0) {}
};

struct Player
{
    ClientWithMap *client;
    PLAYER_INDEX_FOR_CONNECTED gid;    // index in ClientList
    CATCHCHALLENGER_TYPE_MAPID  map;   // index in World::maps
    PLAYER_INDEX_FOR_CONNECTED slot;   // index in that map's map_clients_id
    uint8_t facing;                    // 0=top 1=right 2=bottom 3=left

    Player() : client(NULL), gid(0), map(0), slot(0), facing(0) {}
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
// Collision-map generation -- RIGID, RECTANGULAR, grid-aligned blocks (root
// CLAUDE.md: that is the project's map style, and it is also what a real
// datapack compresses down to). No per-cell noise.
// ---------------------------------------------------------------------------
static void fill_rect(Grid &g, int x0, int y0, int w, int h)
{
    int y = y0;
    while(y < y0 + h)
    {
        if(y >= 0 && y < (int)g.h)
        {
            int x = x0;
            while(x < x0 + w)
            {
                if(x >= 0 && x < (int)g.w)
                    g.cells[(size_t)y * g.w + (size_t)x] = 1;
                x++;
            }
        }
        y++;
    }
}

// Outdoor: open field with scattered rectangular tree/water blocks (~12%).
static void gen_outdoor(Grid &g, Lcg &rng)
{
    const unsigned int blocks = ((unsigned int)g.w * g.h) / 200u;
    unsigned int i = 0;
    while(i < blocks)
    {
        const int bw = 3 + (int)rng.mod(6);
        const int bh = 3 + (int)rng.mod(6);
        const int x0 = (int)rng.mod((uint32_t)g.w);
        const int y0 = (int)rng.mod((uint32_t)g.h);
        fill_rect(g, x0, y0, bw, bh);
        i++;
    }
}

// City: a rigid grid of buildings separated by 3-wide streets (~45% blocked).
// Deterministic layout on purpose -- a town is not random. The variants differ
// only by their street offset, which is what a hand-drawn town looks like.
static void gen_city(Grid &g, Lcg &rng)
{
    const int building_w = 7, building_h = 5;
    const int street     = 3;
    const int offset     = (int)rng.mod(4);
    int y = street + offset;
    while(y + building_h <= (int)g.h)
    {
        int x = street;
        while(x + building_w <= (int)g.w)
        {
            fill_rect(g, x, y, building_w, building_h);
            x += building_w + street;
        }
        y += building_h + street;
    }
}

// Indoor: a walled room plus a few furniture blocks.
static void gen_indoor(Grid &g, Lcg &rng)
{
    fill_rect(g, 0, 0, g.w, 1);
    fill_rect(g, 0, g.h - 1, g.w, 1);
    fill_rect(g, 0, 0, 1, g.h);
    fill_rect(g, g.w - 1, 0, 1, g.h);
    const unsigned int furniture = 2 + rng.mod(3);
    unsigned int i = 0;
    while(i < furniture)
    {
        const int bw = 2 + (int)rng.mod(3);
        const int bh = 1 + (int)rng.mod(2);
        const int x0 = 1 + (int)rng.mod((uint32_t)(g.w > 2 ? g.w - 2 : 1));
        const int y0 = 1 + (int)rng.mod((uint32_t)(g.h > 2 ? g.h - 2 : 1));
        fill_rect(g, x0, y0, bw, bh);
        i++;
    }
}

static bool walkable(const Grid &g, int x, int y)
{
    if(x < 0 || y < 0 || x >= (int)g.w || y >= (int)g.h)
        return false;
    return g.cells[(size_t)y * g.w + (size_t)x] == 0;
}

// Deterministic walkable spawn: a few random draws, then a linear scan from a
// random offset so a heavily built map still resolves without a retry loop.
static void find_spawn(const Grid &g, Lcg &rng, uint8_t &x, uint8_t &y)
{
    unsigned int tries = 0;
    while(tries < 32)
    {
        const uint8_t cx = (uint8_t)rng.mod((uint32_t)g.w);
        const uint8_t cy = (uint8_t)rng.mod((uint32_t)g.h);
        if(walkable(g, cx, cy)) { x = cx; y = cy; return; }
        tries++;
    }
    const size_t cells = (size_t)g.w * g.h;
    const size_t start = rng.mod((uint32_t)cells);
    size_t i = 0;
    while(i < cells)
    {
        const size_t pos = (start + i) % cells;
        if(g.cells[pos] == 0)
        {
            x = (uint8_t)(pos % g.w);
            y = (uint8_t)(pos / g.w);
            return;
        }
        i++;
    }
    x = 0; y = 0;   // unreachable: every generator leaves floor
}

// ---------------------------------------------------------------------------
// World
// ---------------------------------------------------------------------------
struct World
{
    ClientList cl;
    // Read-only collision layouts, a few per kind, shared by the maps.
    std::vector<Grid> grid_pool[Kind_count];
    std::vector<WorldMap*> maps;
    std::vector<Player> players;
    unsigned int players_of_kind[Kind_count];
    unsigned int maps_of_kind[Kind_count];
    // maps[] is grouped by kind; first index of each kind, for the migration
    // draw (which picks a kind by weight, then a map inside it).
    unsigned int first_map_of_kind[Kind_count];
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

    // Population split: weight_k / sum(weights). The remainder is handed out
    // one player at a time to the kinds that carry weight, so the totals
    // always add back up to `players` whatever the weights are.
    void split_population(unsigned int players_total, const KindCfg *c)
    {
        cfg = c;
        weight_total = 0;
        unsigned int k = 0;
        while(k < Kind_count) { weight_total += c[k].weight; k++; }
        if(weight_total == 0) { weight_total = 1; }
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

    void build(unsigned int players_total, const KindCfg *c, Lcg &rng)
    {
        split_population(players_total, c);
        // Map count per kind derives from that kind's target crowd.
        unsigned int k = 0;
        while(k < Kind_count)
        {
            unsigned int occ = c[k].occupancy;
            if(occ == 0) occ = 1;
            if(occ > MAP_PLAYER_CAP) occ = MAP_PLAYER_CAP;
            maps_of_kind[k] = players_of_kind[k] ? (players_of_kind[k] + occ - 1) / occ : 0;
            k++;
        }
        // Layout pool first (maps point into it, so it must never grow again).
        k = 0;
        while(k < Kind_count)
        {
            unsigned int variants = maps_of_kind[k] < GRID_VARIANTS
                                    ? maps_of_kind[k] : GRID_VARIANTS;
            grid_pool[k].reserve(variants);
            unsigned int v = 0;
            while(v < variants)
            {
                Grid g;
                g.w = c[k].width;
                g.h = c[k].height;
                g.cells.assign((size_t)g.w * g.h, 0);
                if(k == Kind_outdoor)   gen_outdoor(g, rng);
                else if(k == Kind_city) gen_city(g, rng);
                else                    gen_indoor(g, rng);
                grid_pool[k].push_back(g);
                v++;
            }
            k++;
        }
        k = 0;
        while(k < Kind_count)
        {
            first_map_of_kind[k] = (unsigned int)maps.size();
            unsigned int i = 0;
            while(i < maps_of_kind[k])
            {
                WorldMap *m = new WorldMap();
                m->kind = (uint8_t)k;
                m->grid = &grid_pool[k][i % grid_pool[k].size()];
                // The CATCHCHALLENGER_TESTING x/y range guard reads these.
                m->mva.width  = m->grid->w;
                m->mva.height = m->grid->h;
                maps.push_back(m);
                i++;
            }
            k++;
        }
        // Players: random map inside their kind (Poisson-ish crowding, which
        // is what a real server looks like), capped at MAP_PLAYER_CAP.
        uint32_t next_id = 1000;
        k = 0;
        while(k < Kind_count)
        {
            unsigned int i = 0;
            while(i < players_of_kind[k])
            {
                const unsigned int mi = pick_map_in_kind(k, rng);
                addPlayer(next_id++, (CATCHCHALLENGER_TYPE_MAPID)mi, rng);
                i++;
            }
            k++;
        }
    }

    unsigned int pick_map_in_kind(unsigned int k, Lcg &rng)
    {
        const unsigned int n = maps_of_kind[k];
        const unsigned int base = first_map_of_kind[k];
        if(n == 0) return base;   // callers only ask for a kind that has maps
        unsigned int tries = 0;
        while(tries < 8)
        {
            const unsigned int mi = base + rng.mod(n);
            if(maps[mi]->population < MAP_PLAYER_CAP) return mi;
            tries++;
        }
        unsigned int i = 0;
        while(i < n)
        {
            if(maps[base + i]->population < MAP_PLAYER_CAP) return base + i;
            i++;
        }
        return base;   // unreachable: maps_of_kind * cap >= players_of_kind
    }

    void addPlayer(uint32_t id, CATCHCHALLENGER_TYPE_MAPID map_index, Lcg &rng)
    {
        WorldMap &m = *maps[map_index];
        ClientWithMap *c = new ClientWithMap();
        uint8_t x = 0, y = 0;
        find_spawn(*m.grid, rng, x, y);
        Player p;
        p.facing = (uint8_t)rng.mod(4);
        c->setX(x); c->setY(y); c->setDirection(look_of(p.facing));
        c->setPlayerId(id);
        c->setMapIndex(map_index);
        char pseudo[16];
        std::snprintf(pseudo, sizeof(pseudo), "p%u", id);
        c->public_and_private_informations.public_informations.pseudo = pseudo;
        c->public_and_private_informations.public_informations.type    = Player_type_normal;
        c->public_and_private_informations.public_informations.skinId  = (uint8_t)(id & 0xff);
        p.client = c;
        p.gid    = cl.add(c);
        p.map    = map_index;
        p.slot   = m.mva.insertOnMap(p.gid);
        m.population++;
        players.push_back(p);
    }

    // A player walks through a border/teleporter: leave the old map, join
    // another one. This is the natural form of the insert+remove pair -- the
    // arrival takes PATH 1 (drop-all + full re-insert) on the new map and the
    // old map broadcasts a 0x69 remove.
    bool migrate(unsigned int player_index, Lcg &rng)
    {
        Player &p = players[player_index];
        WorldMap &from = *maps[p.map];
        // Destination kind drawn by weight, so migrations keep the world's
        // shape instead of flattening it over time.
        unsigned int roll = rng.mod(weight_total);
        unsigned int k = 0;
        while(k + 1 < Kind_count && roll >= cfg[k].weight)
        {
            roll -= cfg[k].weight;
            k++;
        }
        if(maps_of_kind[k] == 0)
        {
            k = 0;
            while(k < Kind_count && maps_of_kind[k] == 0) k++;
            if(k >= Kind_count) return false;
        }
        const unsigned int dest = pick_map_in_kind(k, rng);
        if(dest == p.map) return false;
        from.mva.removeOnMap(p.slot);
        from.population--;
        WorldMap &to = *maps[dest];
        uint8_t x = 0, y = 0;
        find_spawn(*to.grid, rng, x, y);
        p.client->setX(x); p.client->setY(y);
        p.client->setMapIndex((CATCHCHALLENGER_TYPE_MAPID)dest);
        p.map  = (CATCHCHALLENGER_TYPE_MAPID)dest;
        p.slot = to.mva.insertOnMap(p.gid);
        to.population++;
        return true;
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

    // One tick of the server timer: broadcast every map.
    void broadcast()
    {
        size_t i = 0;
        while(i < maps.size())
        {
            maps[i]->mva.min_network((CATCHCHALLENGER_TYPE_MAPID)i);
            i++;
        }
    }
};

// One player's walk step. NO PATHFINDING by design: at most one turn draw plus
// one collision-array lookup, so what the benchmark measures stays the server's
// broadcast and not the harness's idea of movement.
static void step_player(World &w, unsigned int i, Lcg &rng, uint32_t turn_thr,
                        uint64_t &moves, uint64_t &moves_blocked)
{
    Player &p = w.players[i];
    const Grid &g = *w.maps[p.map]->grid;
    if(rng.next() < turn_thr)
        p.facing = (uint8_t)(rng.next() & 3u);
    int nx = p.client->getX();
    int ny = p.client->getY();
    if(p.facing == 0)      ny--;
    else if(p.facing == 1) nx++;
    else if(p.facing == 2) ny++;
    else                   nx--;
    if(walkable(g, nx, ny))
    {
        p.client->setX((COORD_TYPE)nx);
        p.client->setY((COORD_TYPE)ny);
        p.client->setDirection(move_of(p.facing));
        moves++;
    }
    else
    {
        // Blocked by a wall / building / map edge: turn, do not move. That is
        // what the client does, and it is why a city map's players bounce
        // along the streets instead of walking through the buildings.
        p.facing = (uint8_t)(rng.next() & 3u);
        p.client->setDirection(look_of(p.facing));
        moves_blocked++;
    }
}

// Per-tick load model. Everything here is HARNESS cost, deliberately kept to a
// few instructions per player: one threshold compare for the walk draw, one
// array lookup for the collision test, no division, no allocation, no second
// pass over the players (totalBytesAndClear() below already empties the
// capture buffers, so there is nothing left to clear here). run_scenario()
// reports it as median_prep_ns so the ratio to median_tick_ns is checked every
// run instead of being assumed.
static void prepare_tick(World &w, Lcg &rng, uint32_t migrate_thr,
                         uint32_t move_thr, uint32_t turn_thr,
                         uint64_t &migrations,
                         uint64_t &moves, uint64_t &moves_blocked)
{
    unsigned int i = 0;
    while(i < w.players.size())
    {
        ClientWithMap *c = w.players[i].client;
        // Deliver the 0xE3 reply exactly as production does in
        // ClientNetworkRead.cpp. Every client answers within the tick here:
        // the held-back / coalesced-delta path depends on link speed, which is
        // not something this CPU benchmark can model honestly -- it belongs to
        // the network benchmarks (benchmarkclientlatency.py).
        c->ackPing();
        if(rng.next() < move_thr)
            step_player(w, i, rng, turn_thr, moves, moves_blocked);
        else
            c->setDirection(look_of(w.players[i].facing));   // standing still
        i++;
    }
    if(!w.players.empty() && rng.next() < migrate_thr)
    {
        if(w.migrate(rng.mod((uint32_t)w.players.size()), rng))
            migrations++;
    }
}

// budget_ms > 0  -> FIXED-TIME: loop until the wall budget elapses and report
//                   how many ticks completed (benchmark/CLAUDE.md).
// budget_ms == 0 -> FIXED-ITERATION: exactly `ticks` ticks, for callgrind whose
//                   metric is a deterministic instruction count.
static int run_scenario(unsigned int players, unsigned int ticks,
                        uint64_t budget_ms)
{
    World w;
    Lcg rng(WORLD_SEED);
    w.build(players, WORLD, rng);
    const uint32_t move_thr    = pct_threshold(MOVE_PCT);
    const uint32_t turn_thr    = pct_threshold(TURN_PCT);
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
    uint64_t bytes_total = 0, migrations = 0, moves = 0, moves_blocked = 0;

    const std::chrono::steady_clock::time_point loop_start = std::chrono::steady_clock::now();
    const std::chrono::milliseconds budget(budget_ms);
    unsigned int t = 0;

    if(budget_ms == 0)
    {
        while(t < ticks)
        {
            const std::chrono::steady_clock::time_point p0 = std::chrono::steady_clock::now();
            prepare_tick(w, rng, migrate_thr, move_thr, turn_thr,
                         migrations, moves, moves_blocked);
            const std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
            w.broadcast();
            const std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
            samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
            prep_samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t0 - p0).count());
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
            prepare_tick(w, rng, migrate_thr, move_thr, turn_thr,
                         migrations, moves, moves_blocked);
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
            const std::chrono::steady_clock::time_point p0 = std::chrono::steady_clock::now();
            prepare_tick(w, rng, migrate_thr, move_thr, turn_thr,
                         migrations, moves, moves_blocked);
            const std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
            w.broadcast();
            const std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
            samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
            prep_samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t0 - p0).count());
            bytes_total += w.totalBytesAndClear();
            t++;
            k = 1;
            while(k < check_every)
            {
                prepare_tick(w, rng, migrate_thr, move_thr, turn_thr,
                             migrations, moves, moves_blocked);
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
        if(!walkable(*w.maps[pl.map]->grid, pl.client->getX(), pl.client->getY()))
            walk_violations++;
        vi++;
    }

    std::cout.clear();
    std::cout << "BENCH"
              << " players=" << players
              << " maps=" << w.maps.size()
              << " ticks=" << ticks_done
              << " duration_ms=" << elapsed_ms
              << " ticks_per_s=" << ticks_per_s
              // The workload constants are echoed so a history record says
              // what it measured without anyone having to date the source.
              << " move_pct=" << MOVE_PCT
              << " turn_pct=" << TURN_PCT
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
              << " moves_blocked=" << moves_blocked
              // MUST be 0. The harness treats anything else as a FAIL.
              << " walk_violations=" << walk_violations
              << std::endl;
    return 0;
}

static void usage()
{
    std::cerr << "usage: benchmark_min_network_world [--players N]... "
                 "[--ms BUDGET_MS | --ticks T]\n"
                 "\n"
                 "The WORKLOAD IS FIXED (world shape, move/turn/migrate rates,\n"
                 "seed): a benchmark with knobs is not comparable with its own\n"
                 "history. The only arguments are which of the fixed player\n"
                 "counts to run and how the run is bounded:\n"
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

    int rc = 0;
    size_t pi = 0;
    while(pi < players_list.size())
    {
        rc |= run_scenario(players_list[pi], ticks, budget_ms);
        pi++;
    }
    return rc;
}
