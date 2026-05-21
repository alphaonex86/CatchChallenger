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
// One tick's workload PREP: rotate every player's direction (mostly-move) +
// the <insrem_pct>% insert/remove perturbation + clearCaptured. Factored out
// so the timed (latency-sampled) and untimed (throughput) paths share it
// without a lambda (CLAUDE.md: no lambdas). min_network() is timed by the
// caller, NOT here, so the latency window stays exactly around min_network.
static void prepare_tick(Bench &b, Lcg &rng, unsigned int insrem_pct,
                         uint64_t &inserts_total, uint64_t &removes_total)
{
    for(unsigned int i = 0; i < b.owned.size(); i++)
    {
        ClientWithMap *c = b.owned[i];
        if(!c) continue;
        c->setDirection(dir_of(rng.next()));
    }
    if(insrem_pct > 0 && rng.mod(100) < insrem_pct)
    {
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
            static uint32_t fresh_id = 5000;
            COORD_TYPE x = static_cast<COORD_TYPE>(rng.mod(120) + 1);
            COORD_TYPE y = static_cast<COORD_TYPE>(rng.mod(120) + 1);
            b.addPlayer(fresh_id++, x, y, dir_of(rng.next()));
            inserts_total++;
        }
    }
    b.clearCaptured();
}

// budget_ms > 0  -> FIXED-TIME: loop until the wall budget elapses and
//                   report how many ticks completed (higher-is-better
//                   throughput; benchmark/CLAUDE.md "Fixed-time, not
//                   fixed-iteration"). `ticks` is then only a reserve hint.
// budget_ms == 0 -> FIXED-ITERATION: run exactly `ticks` ticks (used by
//                   callgrind, whose metric is a deterministic instruction
//                   count -- a wall budget would make the count vary).
static int run_scenario(unsigned int players, unsigned int ticks,
                        unsigned int seed, unsigned int insrem_pct,
                        uint64_t budget_ms)
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

    std::vector<uint64_t> samples; samples.reserve(ticks ? ticks : 4096);
    uint64_t bytes_total = 0;
    uint64_t inserts_total = 0, removes_total = 0;

    const auto loop_start = std::chrono::steady_clock::now();
    const auto budget = std::chrono::milliseconds(budget_ms);
    unsigned int t = 0;

    if(budget_ms == 0)
    {
        // FIXED-ITERATION: time every tick. The metric is a deterministic
        // instruction count (callgrind), where the wall clock is emulated
        // and irrelevant -- per-tick timing is free of distortion concerns.
        while(t < ticks)
        {
            prepare_tick(b, rng, insrem_pct, inserts_total, removes_total);
            auto t0 = std::chrono::steady_clock::now();
            b.mva.min_network(1);
            auto t1 = std::chrono::steady_clock::now();
            samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
            bytes_total += b.totalBytesAndClear();
            t++;
        }
    }
    else
    {
        // FIXED-TIME with ADAPTIVE BATCHING.
        //
        // The per-tick clock reads needed for LATENCY would dominate cheap
        // ticks on a slow clock (Geode/old MIPS: clock_gettime is a ~1-2us
        // syscall, no vDSO; a 5-player min_network is only ~3.7us), badly
        // skewing the THROUGHPUT count. So we never time every tick:
        //   1. calibrate per-tick cost from a short UNTIMED burst;
        //   2. size a batch so the budget is polled only ~every
        //      CHECK_INTERVAL of work -> the clock cost amortises to ~0;
        //   3. sample latency on ONE tick per batch (plenty of samples for
        //      median/p95, negligible overhead).
        // check_every SELF-SCALES: cheap ticks batch heavily (low relative
        // clock cost); expensive ticks (300 players, ms each) collapse to
        // check_every==1, where a us-scale clock read is already negligible.
        // The throughput count is therefore ~unaffected by clock cost on
        // every arch.
        const uint64_t CHECK_INTERVAL_NS = 5000000ull;   // poll budget ~every 5 ms
        const unsigned int CALIB = 4;                    // tiny calib burst
        for(unsigned int k = 0; k < CALIB; k++)
        {
            prepare_tick(b, rng, insrem_pct, inserts_total, removes_total);
            b.mva.min_network(1);
            bytes_total += b.totalBytesAndClear();
            t++;
        }
        uint64_t calib_ns = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - loop_start).count());
        uint64_t per_tick_ns = calib_ns / (CALIB ? CALIB : 1);
        if(per_tick_ns == 0) per_tick_ns = 1;
        uint64_t ce = CHECK_INTERVAL_NS / per_tick_ns;
        if(ce < 1)            ce = 1;
        if(ce > 1000000ull)   ce = 1000000ull;
        const unsigned int check_every = static_cast<unsigned int>(ce);

        while(std::chrono::steady_clock::now() - loop_start < budget)
        {
            // ONE latency-sampled tick (t0..t1 brackets only min_network).
            prepare_tick(b, rng, insrem_pct, inserts_total, removes_total);
            auto t0 = std::chrono::steady_clock::now();
            b.mva.min_network(1);
            auto t1 = std::chrono::steady_clock::now();
            samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
            bytes_total += b.totalBytesAndClear();
            t++;
            // (check_every - 1) UNTIMED ticks -- zero clock reads.
            for(unsigned int k = 1; k < check_every; k++)
            {
                prepare_tick(b, rng, insrem_pct, inserts_total, removes_total);
                b.mva.min_network(1);
                bytes_total += b.totalBytesAndClear();
                t++;
            }
        }
    }
    const uint64_t ticks_done = t;
    const uint64_t elapsed_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - loop_start).count());

    // Median + p95
    std::vector<uint64_t> sorted = samples;
    std::sort(sorted.begin(), sorted.end());
    uint64_t median = sorted.empty() ? 0 : sorted[sorted.size()/2];
    uint64_t p95    = sorted.empty() ? 0 : sorted[(sorted.size() * 95) / 100];
    uint64_t total  = 0; for(uint64_t s : samples) total += s;
    // Throughput: ticks per second over the measured window. The headline
    // higher-is-better number under the fixed-time model.
    double ticks_per_s = elapsed_ms > 0
        ? (double)ticks_done * 1000.0 / (double)elapsed_ms : 0.0;

    // Re-enable stdout for the result line.
    std::cout.clear();

    std::cout << "BENCH"
              << " players=" << players
              << " ticks="   << ticks_done
              << " duration_ms=" << elapsed_ms
              << " ticks_per_s=" << ticks_per_s
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
    std::cerr << "usage: benchmark_min_network [--players N]... "
                 "[--ms BUDGET_MS | --ticks T] [--seed S] [--insrem-pct P]\n"
                 "  --ms BUDGET_MS  fixed-time: run each player-count for "
                 "BUDGET_MS wall-ms, report ticks completed (default)\n"
                 "  --ticks T       fixed-iteration: run exactly T ticks "
                 "(deterministic instruction count -- for callgrind)"
              << std::endl;
}

int main(int argc, char **argv)
{
    // Suppress the std::cerr volume the production
    // MapVisibilityAlgorithm.cpp emits unconditionally -- redirecting
    // is a no-op on the algorithm itself but stops pipe back-pressure
    // from skewing the timing.
    std::cerr.setstate(std::ios_base::badbit);

    std::vector<unsigned int> players_list = {5, 10, 20, 50, 100, 200, 300};
    unsigned int ticks      = 0;          // >0 => fixed-iteration mode
    uint64_t     budget_ms  = 0;          // >0 => fixed-time mode
    unsigned int seed       = 0x5EEDu;
    unsigned int insrem_pct = 5;          // 5% of ticks insert+remove
    std::vector<unsigned int> overridden;

    for(int i = 1; i < argc; i++)
    {
        std::string a = argv[i];
        if(a == "--players" && i+1 < argc) { overridden.push_back(static_cast<unsigned int>(std::atoi(argv[++i]))); }
        else if(a == "--ms" && i+1 < argc)         { budget_ms = std::strtoull(argv[++i], nullptr, 10); }
        else if(a == "--ticks" && i+1 < argc)      { ticks = static_cast<unsigned int>(std::atoi(argv[++i])); }
        else if(a == "--seed"  && i+1 < argc)      { seed  = static_cast<unsigned int>(std::strtoul(argv[++i], nullptr, 0)); }
        else if(a == "--insrem-pct" && i+1 < argc) { insrem_pct = static_cast<unsigned int>(std::atoi(argv[++i])); }
        else if(a == "-h" || a == "--help")        { usage(); return 0; }
        else                                       { usage(); return 2; }
    }
    if(!overridden.empty()) players_list = overridden;
    // --ms and --ticks are mutually exclusive in effect; --ms wins. When
    // neither is given, default to a fixed-time budget so a bare run still
    // honours the fixed-time model (benchmark/CLAUDE.md).
    if(budget_ms == 0 && ticks == 0) budget_ms = 2000;

    int rc = 0;
    for(unsigned int p : players_list)
    {
        if(p == 0) continue;
        rc |= run_scenario(p, ticks, seed, insrem_pct, budget_ms);
    }
    return rc;
}
