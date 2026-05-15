// HEADLESS: yes
// Benchmark: MapVisibilityAlgorithm::min_network() under a typical
//            mostly-move workload (>90% direction changes, <10%
//            insert+remove), parameterised by player count.
// Metrics emitted (one stdout line per scenario):
//   BENCH players=N ticks=T total_ns=X median_tick_ns=Y p95_tick_ns=Z
//                bytes_sent=B inserts=I removes=R
//   total_ns / median_tick_ns / p95_tick_ns are lower-is-better.
//   bytes_sent is the cumulative output volume produced by the
//   algorithm (sanity-check; should be deterministic).
// Determinism: seeded LCG; no rand(), no clock, no time. Per-scenario
//              warmup tick is excluded from timing.
// All wall-clock readings come from std::chrono::steady_clock and are
// the ONLY source of nondeterminism in the binary -- they affect the
// reported numbers but never the workload.

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

// Deterministic LCG (Numerical Recipes constants). Identical sequence
// for the same seed across every arch / libc / compiler.
struct Lcg
{
    uint32_t state;
    explicit Lcg(uint32_t s) : state(s ? s : 1) {}
    uint32_t next() { state = state * 1664525u + 1013904223u; return state; }
    uint32_t mod(uint32_t n) { return next() % n; }
};

class HarnessMVA : public MapVisibilityAlgorithm
{
public:
    using MapVisibilityAlgorithm::map_clients_id;
    using MapVisibilityAlgorithm::map_removed_index;
};

struct Bench
{
    ClientList cl;
    HarnessMVA mva;
    std::vector<ClientWithMap*> owned;

    Bench()
    {
        ClientList::list = &cl;
        // Match production-typical tunables. simple.max governs the
        // upper bound that triggers the early-out branch in
        // min_network(); set well above the largest scenario to keep
        // it out of the timing path.
        GlobalServerData::serverSettings.mapVisibility.simple.max = 1024;
        GlobalServerData::serverSettings.dontSendPlayerType = false;
        CommonSettingsServer::commonSettingsServer.dontSendPseudo = false;
    }
    ~Bench()
    {
        for(ClientWithMap *c : owned) delete c;
        owned.clear();
        cl.clear();
        ClientList::list = nullptr;
    }

    void addPlayer(uint32_t id, COORD_TYPE x, COORD_TYPE y, Direction d)
    {
        ClientWithMap *c = new ClientWithMap();
        c->setX(x); c->setY(y); c->setDirection(d);
        c->setPlayerId(id);
        c->setMapIndex(1);
        // characterId_db is `player_id` server-side -- already set above.
        // Pseudo and skin populated to match a real player.
        char pseudo[16];
        std::snprintf(pseudo, sizeof(pseudo), "p%u", id);
        c->public_and_private_informations.public_informations.pseudo = pseudo;
        c->public_and_private_informations.public_informations.type    = Player_type_normal;
        c->public_and_private_informations.public_informations.skinId  = static_cast<uint8_t>(id & 0xff);
        owned.push_back(c);
        PLAYER_INDEX_FOR_CONNECTED gid = cl.add(c);
        mva.insertOnMap(gid);
    }

    // Drop captured-block buffers between ticks so the benchmark
    // doesn't grow unbounded heap and skew RSS/timing.
    void clearCaptured()
    {
        for(ClientWithMap *c : owned)
            if(c) c->sentBlocks.clear();
    }

    uint64_t totalBytesAndClear()
    {
        uint64_t total = 0;
        for(ClientWithMap *c : owned)
        {
            if(!c) continue;
            for(const Client::CapturedBlock &b : c->sentBlocks)
                total += b.bytes.size();
            c->sentBlocks.clear();
        }
        return total;
    }
};

static Direction dir_of(uint32_t r)
{
    switch(r & 0x3)
    {
        case 0: return Direction_look_at_top;
        case 1: return Direction_look_at_right;
        case 2: return Direction_look_at_bottom;
        default: return Direction_look_at_left;
    }
}

// One scenario = one player count + one tick count + one ratio.
// Per the brief: most ticks change ONLY direction (x/y stay fixed at
// startup values), <10% of ticks include one insert+remove pair to
// exercise the diff path's "slot replaced" / "remove" / "new slot"
// branches without changing the number of map clients.
static int run_scenario(unsigned int players, unsigned int ticks,
                        unsigned int seed, unsigned int insrem_pct)
{
    Bench b;
    Lcg rng(seed);

    // Startup population: x,y deterministic via seeded LCG, distinct
    // characterId_db, dir spread across all 4 values. x,y stay fixed
    // for the rest of the run.
    for(unsigned int i = 0; i < players; i++)
    {
        COORD_TYPE x = static_cast<COORD_TYPE>(rng.mod(120) + 1);
        COORD_TYPE y = static_cast<COORD_TYPE>(rng.mod(120) + 1);
        b.addPlayer(1000u + i, x, y, dir_of(rng.next()));
    }

    // Silence stdout during the timed loop. Even though the [BRANCH]
    // traces are gone, MapVisibilityAlgorithm.cpp still has plenty of
    // std::cerr/std::cout debug prints (slot-by-slot state) under
    // CATCHCHALLENGER_TESTING — at ~300 slots * 2000 ticks the volume
    // would dominate the timing. setstate(badbit) makes every
    // operator<< a no-op without changing the algorithm.
    std::cout.setstate(std::ios_base::badbit);

    // Warmup tick: PATH 1 in min_network() (sendedMap != mapIndex)
    // every player does the full "drop + reinsert all" handshake. This
    // is hot-cache priming; not in timed median.
    b.clearCaptured();
    auto warm0 = std::chrono::steady_clock::now();
    b.mva.min_network(1);
    auto warm1 = std::chrono::steady_clock::now();
    uint64_t warm_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(warm1 - warm0).count();
    uint64_t bytes_warm = b.totalBytesAndClear();

    std::vector<uint64_t> samples; samples.reserve(ticks);
    uint64_t bytes_total = 0;
    uint64_t inserts_total = 0, removes_total = 0;

    for(unsigned int t = 0; t < ticks; t++)
    {
        // Mostly-move workload: rotate every player's direction by a
        // pseudo-random delta. x,y are not touched on purpose -- the
        // brief asks for x,y to stay at startup values.
        for(unsigned int i = 0; i < b.owned.size(); i++)
        {
            ClientWithMap *c = b.owned[i];
            if(!c) continue;
            c->setDirection(dir_of(rng.next()));
        }

        // Insert/remove perturbation: roll once per tick. When fired,
        // remove one random slot and add a fresh player. Net player
        // count unchanged so the workload size is stable across ticks
        // (deterministic comparison across seeds/runs is preserved).
        if(insrem_pct > 0 && rng.mod(100) < insrem_pct)
        {
            // Pick a non-empty slot to remove.
            unsigned int n = static_cast<unsigned int>(b.mva.map_clients_id.size());
            if(n > 1)
            {
                unsigned int slot = rng.mod(n);
                PLAYER_INDEX_FOR_CONNECTED gid = b.mva.map_clients_id[slot];
                if(gid != PLAYER_INDEX_FOR_CONNECTED_MAX)
                {
                    b.mva.removeOnMap(slot);
                    removes_total++;
                }
                // Add a fresh player at deterministic x,y.
                static uint32_t fresh_id = 5000;
                COORD_TYPE x = static_cast<COORD_TYPE>(rng.mod(120) + 1);
                COORD_TYPE y = static_cast<COORD_TYPE>(rng.mod(120) + 1);
                b.addPlayer(fresh_id++, x, y, dir_of(rng.next()));
                inserts_total++;
            }
        }

        b.clearCaptured();
        auto t0 = std::chrono::steady_clock::now();
        b.mva.min_network(1);
        auto t1 = std::chrono::steady_clock::now();
        samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        bytes_total += b.totalBytesAndClear();
    }

    // Median + p95
    std::vector<uint64_t> sorted = samples;
    std::sort(sorted.begin(), sorted.end());
    uint64_t median = sorted.empty() ? 0 : sorted[sorted.size()/2];
    uint64_t p95    = sorted.empty() ? 0 : sorted[(sorted.size() * 95) / 100];
    uint64_t total  = 0; for(uint64_t s : samples) total += s;

    // Re-enable stdout for the result line.
    std::cout.clear();

    std::cout << "BENCH"
              << " players=" << players
              << " ticks="   << ticks
              << " warm_ns=" << warm_ns
              << " bytes_warm=" << bytes_warm
              << " total_ns=" << total
              << " median_tick_ns=" << median
              << " p95_tick_ns=" << p95
              << " bytes_sent=" << bytes_total
              << " inserts=" << inserts_total
              << " removes=" << removes_total
              << std::endl;
    return 0;
}

static void usage()
{
    std::cerr << "usage: benchmark_min_network [--players N]... [--ticks T] [--seed S] [--insrem-pct P]" << std::endl;
}

int main(int argc, char **argv)
{
    // Suppress the std::cerr volume the production
    // MapVisibilityAlgorithm.cpp emits unconditionally -- redirecting
    // is a no-op on the algorithm itself but stops pipe back-pressure
    // from skewing the timing.
    std::cerr.setstate(std::ios_base::badbit);

    std::vector<unsigned int> players_list = {5, 10, 20, 50, 100, 200, 300};
    unsigned int ticks      = 2000;
    unsigned int seed       = 0x5EEDu;
    unsigned int insrem_pct = 5;          // 5% of ticks insert+remove
    std::vector<unsigned int> overridden;

    for(int i = 1; i < argc; i++)
    {
        std::string a = argv[i];
        if(a == "--players" && i+1 < argc) { overridden.push_back(static_cast<unsigned int>(std::atoi(argv[++i]))); }
        else if(a == "--ticks" && i+1 < argc)      { ticks = static_cast<unsigned int>(std::atoi(argv[++i])); }
        else if(a == "--seed"  && i+1 < argc)      { seed  = static_cast<unsigned int>(std::strtoul(argv[++i], nullptr, 0)); }
        else if(a == "--insrem-pct" && i+1 < argc) { insrem_pct = static_cast<unsigned int>(std::atoi(argv[++i])); }
        else if(a == "-h" || a == "--help")        { usage(); return 0; }
        else                                       { usage(); return 2; }
    }
    if(!overridden.empty()) players_list = overridden;

    int rc = 0;
    for(unsigned int p : players_list)
    {
        if(p == 0) continue;
        rc |= run_scenario(p, ticks, seed, insrem_pct);
    }
    return rc;
}
