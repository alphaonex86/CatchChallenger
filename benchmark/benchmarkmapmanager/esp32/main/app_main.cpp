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
#include <fcntl.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

// ESP-IDF exposes the app descriptor under different names across major
// versions; both give the same struct (version / project_name / date / idf_ver).
#if __has_include("esp_app_desc.h")
#  include "esp_app_desc.h"                 // IDF >= 5.0
#  define CC_APP_DESC() esp_app_get_description()
#else
#  include "esp_ota_ops.h"                  // IDF 4.x
#  define CC_APP_DESC() esp_ota_get_app_description()
#endif

// What this board answers with when something pokes its console (see the idle
// loop at the end of app_main() for why it answers at all).
//
// It reports the FIRMWARE VERSION, which ESP-IDF fills from `git describe` of
// the CatchChallenger tree whenever PROJECT_VER is not set explicitly — as here.
// That matters for benchmarking: the BENCH numbers a board produces are only
// interpretable if you know which commit produced them, and reading it off the
// running device beats trusting that the flashed image is what you think it is.
// 224, not 192: the ESP-IDF app descriptor fields are fixed-size (project_name
// 32, version 32, date 16, time 16, idf_ver 32) and GCC computes this snprintf
// at up to 196 bytes, so 192 could silently truncate the identity of the very
// firmware you are trying to identify (-Wformat-truncation caught it).
static char ident_line[224];

static void build_ident_line(void)
{
    const esp_app_desc_t *d = CC_APP_DESC();
    snprintf(ident_line, sizeof(ident_line),
             "ESP32 CatchChallenger %s — idle, NOT an ONU console — "
             "fw=%s built=%s %s idf=%s",
             (d && d->project_name[0]) ? d->project_name : "benchmarkmapmanager",
             (d && d->version[0]) ? d->version : "unknown",
             (d && d->date[0]) ? d->date : "?",
             (d && d->time[0]) ? d->time : "",
             (d && d->idf_ver[0]) ? d->idf_ver : "?");
}

// Provided by benchmark/benchmarkmapmanager/main.cpp, given external linkage
// there under CC_TARGET_ESP32 (the argv main() is compiled out on ESP32). Use
// uint64_t for budget_ms exactly as main.cpp declares it so the C++ mangled
// name matches on every ABI (uint64_t != unsigned long long on some hosts).
int run_scenario(unsigned int players, unsigned int ticks, unsigned int seed,
                 unsigned int insrem_pct, unsigned int move_pct,
                 uint64_t budget_ms,
                 unsigned int lag_pct, unsigned int lag_rounds);

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
                             (uint64_t)BENCH_MS,
                             0 /*lag_pct: healthy link*/, 1 /*lag_rounds*/);
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
    //
    // ...but ANSWER IF POKED. Why: this board sits on a shared lab bench next to
    // a GPON ONU's serial console, and both use a CP2102 (10c4:ea60), so the USB
    // VID:PID CANNOT tell them apart — and their /dev/ttyUSBn indices shuffle on
    // every re-enumeration. Tooling that must find the ONU console therefore
    // identifies it by REPLY SIGNATURE. While this firmware stayed mute, it was
    // only excluded from that search by accident (it happened to be silent);
    // anything that made it emit output could have got it mistaken for the ONU,
    // and the neighbouring ports on that bench are an irreplaceable OLT and a
    // relay that cuts board power. Answering with a self-describing line turns
    // "the one that says nothing" into a positively identified device.
    //
    // Cost is nil: stdin is put in non-blocking mode, so the read returns
    // immediately when nothing is pending and the task still spends all its time
    // in vTaskDelay(). This runs only AFTER BENCH_END, so it can never interfere
    // with the timed measurements or pollute the BENCH lines the host parses.
    fcntl(fileno(stdin), F_SETFL, O_NONBLOCK);
    build_ident_line();
    // Print it once unprompted too: a host that attaches to the port AFTER the
    // sweep finished then sees what this board is without having to poke it.
    printf("%s\n", ident_line);
    fflush(stdout);

    while(true)
    {
        // Drain whatever arrived and answer once per burst, so holding a key
        // down cannot turn into a flood of identity lines.
        bool poked = false;
        int c;
        while((c = getchar()) != EOF)
            poked = true;
        // ★ REQUIRED: a non-blocking read with nothing pending returns EOF and
        // LATCHES the stream's eof/error flag, after which every later getchar()
        // keeps returning EOF even once bytes do arrive. Without this the board
        // would answer the first poke and then appear mute forever — which is
        // exactly the ambiguity this code exists to remove.
        clearerr(stdin);
        if(poked)
        {
            printf("%s\n", ident_line);
            fflush(stdout);
        }
        // 200 ms keeps a poke feeling instant to a prober with a ~1 s deadline,
        // while leaving the CPU essentially idle.
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
