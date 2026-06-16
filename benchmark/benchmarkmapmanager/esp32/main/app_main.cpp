// app_main.cpp — ESP-IDF entry for the MapVisibilityAlgorithm::min_network()
// PURE-CPU microbenchmark on ESP32.
//
// Unlike the server firmware (server/cli/esp32/) this image does NOTHING but
// run the benchmark: NO WiFi, NO lwIP, NO datapack, NO server. It runs the SAME
// harness the native fleet runs (benchmark/benchmarkmapmanager/main.cpp's
// run_scenario()) at the SAME workload constants as benchmarkmapmanager.py
// (seed 0x5EED, insrem 5%, move 40%, 2000 ms fixed-time per count) and prints
// the IDENTICAL `BENCH players=N ...` lines to UART0 (printf / std::cout go to
// serial), so the on-device numbers are directly comparable to the fleet.
//
// RAM REALITY: a plain ESP32 has ~290 KiB usable DRAM, ~111 KiB largest
// contiguous free block. The harness allocates per-player state (Client +
// captured-block vectors), so the big player counts (200/300, often 100) will
// not fit. Each scenario is therefore guarded: we pre-check TOTAL free heap
// against a per-player estimate, and ALSO wrap run_scenario() in try/catch so a
// mid-run std::bad_alloc prints `BENCH players=N status=skip_oom` and continues
// to the NEXT count without crashing or rebooting. A small-RAM board thus still
// reports the counts it CAN do (likely 5/10/20/50).
//
// After the sweep app_main idles forever (vTaskDelay) — it never reboots
// mid-output, so the serial reader on the host captures the whole BENCH block.

#include <cstdio>
#include <cstdint>
#include <new>
#include <vector>
#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

// Provided by benchmark/benchmarkmapmanager/main.cpp, given external linkage
// there under CC_TARGET_ESP32 (the argv main() is compiled out on ESP32). Use
// uint64_t for budget_ms exactly as main.cpp declares it so the C++ mangled
// name matches on every ABI (uint64_t != unsigned long long on some hosts).
int run_scenario(unsigned int players, unsigned int ticks, unsigned int seed,
                 unsigned int insrem_pct, unsigned int move_pct,
                 uint64_t budget_ms);

// SAME constants as benchmark/benchmarkmapmanager.py so the on-device BENCH
// lines line up with the fleet champion for this benchmark.
static const unsigned int  BENCH_SEED        = 0x5EEDu;
static const unsigned int  BENCH_INSREM_PCT  = 5u;
static const unsigned int  BENCH_MOVE_PCT    = 40u;
static const unsigned long long BENCH_MS     = 2000ull;  // fixed-time per count
static const unsigned int  BENCH_PLAYERS[]   = {5, 10, 20, 50, 100, 200, 300};

// Rough per-player heap cost: each player is a heap-allocated ClientWithMap plus
// its captured-block + sendedStatus vectors and the harness's owned[] / map_*
// slots. ~700 B measured-ish; round UP to 1 KiB and require 2x headroom so the
// transient peak (insert+remove churn, vector growth) doesn't OOM mid-run. This
// is only the cheap PRE-skip; the try/catch below is the real safety net.
static const size_t PER_PLAYER_BYTES = 1024;
static const size_t SCENARIO_HEAP_HEADROOM = 24 * 1024;  // base bench + slack

static const char *TAG = "bench-mapmgr";

extern "C" void app_main(void)
{
    // Silence the std::cerr debug volume the production MapVisibilityAlgorithm.cpp
    // emits unconditionally under CATCHCHALLENGER_TESTING. On native this is done
    // in main() (compiled out here), so do it here: over UART the spam would both
    // pollute the BENCH lines the host parses AND throttle the timed loop via
    // serial back-pressure, skewing the numbers. (run_scenario itself already
    // gates std::cout around the loop, re-enabling it only for the BENCH line.)
    std::cerr.setstate(std::ios_base::badbit);

    // BENCH lines must hit the serial verbatim; flush ESP-IDF's logger to the
    // same UART. The numbers themselves come from std::cout in run_scenario.
    ESP_LOGI(TAG, "MapVisibilityAlgorithm::min_network benchmark — "
                  "seed=0x%X insrem=%u%% move=%u%% budget=%llums",
             BENCH_SEED, BENCH_INSREM_PCT, BENCH_MOVE_PCT, BENCH_MS);
    printf("BENCH_BEGIN free_heap=%u largest_block=%u\n",
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
           (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

    const unsigned int n = sizeof(BENCH_PLAYERS) / sizeof(BENCH_PLAYERS[0]);
    unsigned int i = 0;
    while(i < n)
    {
        const unsigned int players = BENCH_PLAYERS[i];
        // The per-player state is many SMALL scattered allocations (a
        // ClientWithMap + its vectors per player), so the limiting resource is
        // TOTAL free heap, not the single largest contiguous block — compare
        // against free_size. The try/catch below is the backstop for the rarer
        // contiguous-block failure (e.g. a big sample vector growing).
        size_t need = SCENARIO_HEAP_HEADROOM + (size_t)players * PER_PLAYER_BYTES * 2;
        size_t freesz = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        if(freesz < need)
        {
            // Won't fit — skip THIS count, keep going to the next (still print a
            // line so the parser/operator sees the count was attempted).
            printf("BENCH players=%u status=skip_oom need=%u free_heap=%u\n",
                   players, (unsigned)need, (unsigned)freesz);
        }
        else
        {
            // Real safety net: a mid-run allocation failure (estimate too
            // optimistic) is caught here, reported, and the sweep continues.
            try
            {
                run_scenario(players, 0 /*ticks: fixed-time*/, BENCH_SEED,
                             BENCH_INSREM_PCT, BENCH_MOVE_PCT,
                             (uint64_t)BENCH_MS);
            }
            catch(const std::bad_alloc &)
            {
                printf("BENCH players=%u status=skip_oom (bad_alloc mid-run) "
                       "largest_block=%u\n", players,
                       (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
            }
            catch(...)
            {
                printf("BENCH players=%u status=skip_error\n", players);
            }
        }
        // Yield between scenarios so the IDLE task can reclaim and the watchdog
        // stays fed during the next (potentially multi-second) fixed-time run.
        vTaskDelay(pdMS_TO_TICKS(50));
        i++;
    }

    printf("BENCH_END free_heap=%u\n",
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT));
    ESP_LOGI(TAG, "benchmark complete — idling (no reboot)");

    // Idle forever; do NOT esp_restart() — a reboot mid-output would truncate
    // the serial capture on the host. The host reader has the full BENCH block.
    while(true)
        vTaskDelay(pdMS_TO_TICKS(10000));
}
