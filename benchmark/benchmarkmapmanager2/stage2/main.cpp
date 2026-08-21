// HEADLESS: yes
// Benchmark: MapVisibilityAlgorithm::min_network() over a whole world, driven
//            by a PRE-GENERATED REPLAY. Stage 2 of benchmarkmapmanager2.
//
// Stage 1 (benchmarkmapmanager2/stage1) read the datapack's real map set with
// the production loader, decided how many players this node's RAM can hold,
// placed them, and wrote every movement vector they will walk -- each one
// already validated against the real collisions. Stage 2 compiles that in and
// replays it. So the binary carries its whole workload: no datapack, no map
// files, no collision test, nothing to read at runtime. That is what lets the
// same benchmark run on a board with a few hundred KB and no filesystem, and
// it is why the load model costs almost nothing here: the measurement is
// min_network and the tick loop around it, not the client being simulated.
//
// The vectors are NOT re-decided every tick. A vector says "walk 3 cells":
// the player then walks one cell per tick for 3 ticks and only THEN is its
// next vector fetched; if that one says 5, the one after it comes 5 ticks
// later. Per player per tick that is a countdown, a coordinate update and a
// direction store.
//
// When the replay reaches its end (CYCLE_TICKS) everything RESETS -- every
// player back to its spawn, every list back to its first vector -- and the
// same window replays. The list is sized so that happens rarely (stage 1
// trades list length against the bytes the binary has to carry); `resets` is
// reported so the cost is never invisible.
//
// Metrics (one BENCH line per player count):
//   ticks_per_s               higher-is-better (fixed-TIME throughput)
//   median_tick_ns/p95_tick_ns lower-is-better: one tick = the WHOLE world
//   median_prep_ns            lower-is-better: the replay's own cost per tick.
//                             NOT server work, and outside the latency window
//   bytes_sent                lower-is-better (what min_network exists to cut)
//   visibility_state_bytes    lower-is-better (resident per-map diff state)
//   sampled_changed/sampled_slots  share of slots that differ from the previous
//                             broadcast: what the stateful diff gets to SKIP
//   replay_mismatch           MUST be 0 -- after a full cycle the replayed
//                             state is hashed and compared with what stage 1
//                             computed. A mismatch means stage 2 decoded the
//                             vectors differently from the generator, and
//                             since the generator validated every vector
//                             against the real collisions, a match also proves
//                             no player walked into a wall.
//
// Determinism: no rand(), no clock in the workload -- it is a replay. The
//              steady_clock reads are the only nondeterminism and never feed
//              the workload.

#include "../../../test/testingmapmanagement/Stubs.hpp"
#include "../../../server/base/MapManagement/MapVisibilityAlgorithm.hpp"
#include "Workload.hpp"

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
namespace W = CCBenchWorkload;

class HarnessMVA : public MapVisibilityAlgorithm
{
public:
    using MapVisibilityAlgorithm::map_clients_id;
    using MapVisibilityAlgorithm::map_removed_index;
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

struct WorldMap
{
    HarnessMVA mva;
};

// Per player, everything the replay needs and nothing else.
struct Player
{
    ClientWithMap *client;
    PLAYER_INDEX_FOR_CONNECTED gid;
    CATCHCHALLENGER_TYPE_MAPID map;
    PLAYER_INDEX_FOR_CONNECTED slot;
    uint32_t entry;        // index of the vector being walked
    uint8_t remaining;     // ticks left in it
    uint8_t dir;           // 0 = standing, 1..4 = walking that way
    uint8_t facing;        // 0..3, what it looks at when it stands

    Player() : client(NULL), gid(0), map(0), slot(0), entry(0),
               remaining(0), dir(0), facing(0) {}
};

struct World
{
    ClientList cl;
    std::vector<WorldMap*> maps;      // every map of the world, in load order
    std::vector<Player> players;

    World()
    {
        ClientList::list = &cl;
        GlobalServerData::serverSettings.mapVisibility.simple.max = 1024;
        GlobalServerData::serverSettings.dontSendPlayerType = false;
        CommonSettingsServer::commonSettingsServer.dontSendPseudo = false;
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

    void build(uint32_t player_count)
    {
        uint16_t m = 0;
        while(m < W::MAPS)
        {
            WorldMap *wm = new WorldMap();
            // All the world stage 2 needs: the x/y range guard reads these.
            wm->mva.width  = W::MAP_W[m];
            wm->mva.height = W::MAP_H[m];
            maps.push_back(wm);
            m++;
        }
        players.reserve(player_count);
        uint32_t i = 0;
        while(i < player_count)
        {
            ClientWithMap *c = new ClientWithMap();
            Player p;
            p.map    = W::SPAWN_MAP[i];
            p.facing = W::SPAWN_FACING[i];
            c->setX(W::SPAWN_X[i]);
            c->setY(W::SPAWN_Y[i]);
            c->setDirection(look_of(p.facing));
            c->setPlayerId(1000u + i);
            c->setMapIndex(p.map);
            char pseudo[16];
            std::snprintf(pseudo, sizeof(pseudo), "p%u", (unsigned int)(1000u + i));
            c->public_and_private_informations.public_informations.pseudo = pseudo;
            c->public_and_private_informations.public_informations.type   = Player_type_normal;
            c->public_and_private_informations.public_informations.skinId = (uint8_t)(i & 0xff);
            p.client = c;
            p.gid    = cl.add(c);
            p.slot   = maps[p.map]->mva.insertOnMap(p.gid);
            players.push_back(p);
            i++;
        }
    }

    // Back to tick 0 of the replay: everyone home, every list rewound. Players
    // that had migrated rejoin their spawn map, which is the only part of a
    // reset that costs more than a store.
    void reset()
    {
        size_t i = 0;
        while(i < players.size())
        {
            Player &p = players[i];
            const CATCHCHALLENGER_TYPE_MAPID home = W::SPAWN_MAP[i];
            if(p.map != home)
            {
                maps[p.map]->mva.removeOnMap(p.slot);
                p.map  = home;
                p.slot = maps[home]->mva.insertOnMap(p.gid);
                p.client->setMapIndex(home);
            }
            p.client->setX(W::SPAWN_X[i]);
            p.client->setY(W::SPAWN_Y[i]);
            p.facing    = W::SPAWN_FACING[i];
            p.client->setDirection(look_of(p.facing));
            p.entry     = 0;
            p.remaining = 0;
            p.dir       = 0;
            i++;
        }
    }

    // The state stage 1 predicted for the end of a cycle.
    uint64_t stateHash() const
    {
        uint64_t h = 1469598103934665603ull;
        size_t i = 0;
        while(i < players.size())
        {
            const Player &p = players[i];
            h ^= p.map;                  h *= 1099511628211ull;
            h ^= p.client->getX();       h *= 1099511628211ull;
            h ^= p.client->getY();       h *= 1099511628211ull;
            h ^= p.facing;               h *= 1099511628211ull;
            i++;
        }
        return h;
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

    // One tick of the server timer: broadcast EVERY map, like the server's
    // timer does over flat_map_list.
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

// One tick of the replay. Per player: a countdown, and either a coordinate
// update or a direction store. The next vector is fetched ONLY when the
// current one runs out -- a 3-cell vector is 3 ticks of walking before the
// list is touched again.
static void replay_tick(World &w, uint32_t tick_in_cycle, uint32_t &next_mig)
{
    const uint32_t entries = W::ENTRIES_PER_PLAYER;
    size_t i = 0;
    while(i < w.players.size())
    {
        Player &p = w.players[i];
        ClientWithMap *c = p.client;
        // Deliver the 0xE3 reply exactly as production does in
        // ClientNetworkRead.cpp: without it every client looks permanently
        // lagging after the first tick and the server sends nothing.
        c->ackPing();
        if(p.remaining == 0)
        {
            const uint8_t e = W::REPLAY[(size_t)i * entries + p.entry];
            p.dir       = (uint8_t)(e >> 5);
            p.remaining = (uint8_t)((e & 31) + 1);
            p.entry++;
            if(p.entry >= entries)
                p.entry = 0;              // never read past the list
            // The direction is written ONCE, when the vector starts: a walking
            // player keeps Direction_move_at_* for the whole vector and a
            // standing one keeps Direction_look_at_*, so the remaining ticks
            // of an entry touch nothing but the coordinate (or nothing at all
            // -- and most players are standing at any given tick).
            if(p.dir != 0)
            {
                p.facing = (uint8_t)(p.dir - 1);
                c->setDirection(move_of(p.facing));
            }
            else
                c->setDirection(look_of(p.facing));
        }
        if(p.dir != 0)
        {
            if(p.facing == 0)      c->setY((COORD_TYPE)(c->getY() - 1));
            else if(p.facing == 1) c->setX((COORD_TYPE)(c->getX() + 1));
            else if(p.facing == 2) c->setY((COORD_TYPE)(c->getY() + 1));
            else                   c->setX((COORD_TYPE)(c->getX() - 1));
        }
        p.remaining--;
        i++;
    }
    // Map changes scheduled for this tick: the arrival takes PATH 1 (drop-all
    // + full re-insert) on its new map, the departure emits a 0x69 remove on
    // the old one. Nothing else in a replay exercises that path.
    while(next_mig < W::MIGRATIONS && W::MIG_TICK[next_mig] == tick_in_cycle)
    {
        const uint32_t pi = W::MIG_PLAYER[next_mig];
        const CATCHCHALLENGER_TYPE_MAPID dest = W::MIG_MAP[next_mig];
        // pi is out of range on a prefix run (that migration belongs to a
        // player this cell does not have); dest cannot be, unless the workload
        // and this binary were built from different generations -- refuse to
        // index the map list on it rather than corrupt the run.
        if(pi < w.players.size() && dest < w.maps.size())
        {
            Player &p = w.players[pi];
            w.maps[p.map]->mva.removeOnMap(p.slot);
            p.map  = dest;
            p.slot = w.maps[dest]->mva.insertOnMap(p.gid);
            p.client->setMapIndex(dest);
            p.client->setX(W::MIG_X[next_mig]);
            p.client->setY(W::MIG_Y[next_mig]);
            p.remaining = 0;              // its run belonged to the old map
            p.dir = 0;
        }
        next_mig++;
    }
}

// budget_ms > 0  -> FIXED-TIME: run until the budget elapses, report the ticks
//                   completed (benchmark/CLAUDE.md).
// budget_ms == 0 -> FIXED-ITERATION: exactly `ticks` ticks, for callgrind.
static int run_scenario(uint32_t players, unsigned int ticks, uint64_t budget_ms)
{
    World w;
    w.build(players);

    // Silence the CATCHCHALLENGER_TESTING slot-by-slot debug prints of
    // MapVisibilityAlgorithm.cpp: at this scale their volume would dominate
    // the timing. badbit makes every operator<< a no-op, algorithm untouched.
    std::cout.setstate(std::ios_base::badbit);

    // Warmup tick: every client takes PATH 1 (sendedMap != mapIndex) and does
    // the full drop+reinsert handshake. Cache priming, excluded from timing.
    const std::chrono::steady_clock::time_point warm0 = std::chrono::steady_clock::now();
    w.broadcast();
    const std::chrono::steady_clock::time_point warm1 = std::chrono::steady_clock::now();
    const uint64_t warm_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(warm1 - warm0).count();
    const uint64_t bytes_warm = w.totalBytesAndClear();

    std::vector<uint64_t> samples;      samples.reserve(ticks ? ticks : 4096);
    std::vector<uint64_t> prep_samples; prep_samples.reserve(ticks ? ticks : 4096);
    std::vector<uint32_t> snap;
    uint64_t bytes_total = 0, resets = 0, mismatches = 0;
    uint64_t sampled_slots = 0, sampled_changed = 0;
    uint32_t tick_in_cycle = 0, next_mig = 0;
    bool cycle_checked = false;

    const std::chrono::steady_clock::time_point loop_start = std::chrono::steady_clock::now();
    const std::chrono::milliseconds budget(budget_ms);
    unsigned int t = 0;

    // One tick of bookkeeping after the broadcast: the replay wraps at
    // CYCLE_TICKS, and the first wrap is where the oracle fires.
    #define CC_ADVANCE_CYCLE()                                                 \
        do {                                                                   \
            tick_in_cycle++;                                                   \
            if(tick_in_cycle >= W::CYCLE_TICKS)                                \
            {                                                                  \
                if(!cycle_checked && w.players.size() == W::PLAYERS)           \
                {                                                              \
                    if(w.stateHash() != W::CYCLE_END_HASH) mismatches++;       \
                    cycle_checked = true;                                      \
                }                                                              \
                w.reset();                                                     \
                tick_in_cycle = 0;                                             \
                next_mig = 0;                                                  \
                resets++;                                                      \
            }                                                                  \
        } while(0)

    if(budget_ms == 0)
    {
        while(t < ticks)
        {
            w.snapshotState(snap);
            const std::chrono::steady_clock::time_point p0 = std::chrono::steady_clock::now();
            replay_tick(w, tick_in_cycle, next_mig);
            const std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
            w.broadcast();
            const std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
            samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
            prep_samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t0 - p0).count());
            sampled_changed += w.countChanged(snap);
            sampled_slots   += w.players.size();
            bytes_total += w.totalBytesAndClear();
            CC_ADVANCE_CYCLE();
            t++;
        }
    }
    else
    {
        // FIXED-TIME with ADAPTIVE BATCHING: on a slow clock (Geode / old MIPS,
        // clock_gettime is a ~1-2us syscall with no vDSO) timing every tick
        // would skew the throughput count, so calibrate, batch, and sample
        // latency on one tick per batch.
        const uint64_t CHECK_INTERVAL_NS = 5000000ull;
        const unsigned int CALIB = 4;
        unsigned int k = 0;
        while(k < CALIB)
        {
            replay_tick(w, tick_in_cycle, next_mig);
            w.broadcast();
            bytes_total += w.totalBytesAndClear();
            CC_ADVANCE_CYCLE();
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
            w.snapshotState(snap);
            const std::chrono::steady_clock::time_point p0 = std::chrono::steady_clock::now();
            replay_tick(w, tick_in_cycle, next_mig);
            const std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
            w.broadcast();
            const std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
            samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
            prep_samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t0 - p0).count());
            sampled_changed += w.countChanged(snap);
            sampled_slots   += w.players.size();
            bytes_total += w.totalBytesAndClear();
            CC_ADVANCE_CYCLE();
            t++;
            k = 1;
            while(k < check_every)
            {
                replay_tick(w, tick_in_cycle, next_mig);
                w.broadcast();
                bytes_total += w.totalBytesAndClear();
                CC_ADVANCE_CYCLE();
                t++;
                k++;
            }
        }
    }
    #undef CC_ADVANCE_CYCLE

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

    std::cout.clear();
    std::cout << "BENCH"
              << " players=" << players
              << " maps=" << w.maps.size()
              << " ticks=" << ticks_done
              << " duration_ms=" << elapsed_ms
              << " ticks_per_s=" << ticks_per_s
              << " cycle_ticks=" << W::CYCLE_TICKS
              << " entries_per_player=" << W::ENTRIES_PER_PLAYER
              << " resets=" << resets
              << " migrations=" << W::MIGRATIONS
              << " visibility_state_bytes=" << w.visibilityStateBytes()
              << " warm_ns=" << warm_ns
              << " bytes_warm=" << bytes_warm
              << " total_ns=" << total
              << " median_tick_ns=" << median
              << " p95_tick_ns=" << p95
              << " median_prep_ns=" << median_prep
              << " bytes_sent=" << bytes_total
              << " sampled_slots=" << sampled_slots
              << " sampled_changed=" << sampled_changed;
    // MUST be 0: the replayed state after a full cycle equals what stage 1
    // predicted, which also proves no vector was decoded into a wall.
    std::cout << " replay_mismatch=" << mismatches
              << std::endl;
    return 0;
}

static void usage()
{
    std::cerr << "usage: benchmark_min_network_replay [--players N]... "
                 "[--ms BUDGET_MS | --ticks T]\n"
                 "\n"
                 "Stage 2 of benchmarkmapmanager2: replays the workload stage 1\n"
                 "generated for THIS node and THIS datapack, which is compiled in --\n"
                 "there is nothing to read at runtime. The workload is FIXED; the only\n"
                 "arguments are how much of it to run:\n"
                 "  --players N     run the first N players of the generated set.\n"
                 "                  Default: the generated count, a quarter of it and\n"
                 "                  a sixteenth (the large/medium/small of one node).\n"
                 "  --ms BUDGET_MS  fixed-time: run each count for BUDGET_MS and report\n"
                 "                  the ticks completed (default, 2000 ms)\n"
                 "  --ticks T       fixed-iteration: exactly T ticks. Only for\n"
                 "                  callgrind, whose metric is a deterministic\n"
                 "                  instruction count a wall budget would blur."
              << std::endl;
}

int main(int argc, char **argv)
{
    // The production MapVisibilityAlgorithm.cpp writes to std::cerr
    // unconditionally; redirecting is a no-op on the algorithm but stops pipe
    // back-pressure from skewing the timing.
    std::cerr.setstate(std::ios_base::badbit);

    std::vector<uint32_t> players_list;
    unsigned int ticks     = 0;
    uint64_t     budget_ms = 0;

    int i = 1;
    while(i < argc)
    {
        const std::string a = argv[i];
        if(a == "--players" && i + 1 < argc)
        {
            const uint32_t n = (uint32_t)std::strtoul(argv[++i], NULL, 10);
            if(n == 0 || n > W::PLAYERS)
            {
                std::cerr.clear();
                std::cerr << "--players " << n << " is outside the generated set (1.."
                          << W::PLAYERS << ")" << std::endl;
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
        // small / medium / large of THIS node, all out of the same generated
        // set: a prefix of the players is a valid workload on its own.
        const uint32_t big = W::PLAYERS;
        const uint32_t mid = big / 4 ? big / 4 : 1;
        const uint32_t small = big / 16 ? big / 16 : 1;
        players_list.push_back(small);
        if(mid > small) players_list.push_back(mid);
        if(big > mid)   players_list.push_back(big);
    }
    if(budget_ms == 0 && ticks == 0) budget_ms = 2000;

    std::cout << "WORKLOAD node=" << W::NODE
              << " players=" << W::PLAYERS
              << " maps=" << W::MAPS
              << " cycle_ticks=" << W::CYCLE_TICKS
              << " entries_per_player=" << W::ENTRIES_PER_PLAYER
              << " migrations=" << W::MIGRATIONS
              << " replay_bytes=" << (uint64_t)W::PLAYERS * W::ENTRIES_PER_PLAYER
              << " datapack=" << W::DATAPACK
              << std::endl;

    int rc = 0;
    size_t pi = 0;
    while(pi < players_list.size())
    {
        rc |= run_scenario(players_list[pi], ticks, budget_ms);
        pi++;
    }
    return rc;
}
