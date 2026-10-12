// ESP-IDF entry for benchmarkmapmanager2 on the ESP32: runs the SAME stage 2
// replay as the fleet, once per player count, and prints the identical
// `BENCH players=N ...` lines to UART0. The workload is compiled into flash, so
// the board needs no filesystem.
//
// Built WITHOUT exceptions (CatchChallenger uses none): a failed allocation
// aborts. So each count is checked against the heap first and printed as
// `BENCH players=N status=skip_oom` when it cannot fit; `BENCH_TRY players=N`
// precedes every count actually run, and the panic handler HALTS instead of
// rebooting, so an estimate that was too optimistic shows as the last
// BENCH_TRY without its BENCH line instead of a reboot loop.

#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <fcntl.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"

int cc_replay_main(int argc, char **argv);
size_t cc_replay_world_bytes();
uint32_t cc_replay_max_players();

// Written by benchmarkmapmanager2.py beside the workload: the player counts of
// benchmarkmapmanager (so the two compare directly) and the per-cell budget
// (MAP_BENCH_MS), as CC_BENCH_PLAYERS_LIST and CC_BENCH_MS.
#include "esp32_bench_config.h"
static const unsigned int BENCH_PLAYERS[] = {CC_BENCH_PLAYERS_LIST};

// Per player: a ClientWithMap (~240 B on 32 bits) plus its captured-block,
// sendedStatus and visibleSlots vectors and its map slot. 1 KiB, doubled for
// the vectors' growth, is the estimate benchmarkmapmanager's firmware uses;
// the headroom covers the latency samples and the per-map lists.
static const size_t PER_PLAYER_BYTES = 1024;
static const size_t HEADROOM_BYTES = 48 * 1024;

static size_t freeHeap()
{
    return heap_caps_get_free_size(MALLOC_CAP_8BIT);
}

static size_t largestBlock()
{
    return heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
}

extern "C" void app_main(void)
{
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    // A native-USB board re-enumerates on every reset: give the host time to
    // reopen the port, or the first BENCH lines go nowhere.
    vTaskDelay(pdMS_TO_TICKS(5000));
#endif
    printf("BENCH_BEGIN free_heap=%u largest_block=%u world_bytes=%u\n",
           (unsigned)freeHeap(), (unsigned)largestBlock(),
           (unsigned)cc_replay_world_bytes());
    const unsigned int n = sizeof(BENCH_PLAYERS) / sizeof(BENCH_PLAYERS[0]);
    unsigned int i = 0;
    while(i < n)
    {
        const unsigned int players = BENCH_PLAYERS[i];
        const size_t world = cc_replay_world_bytes();
        const size_t need = world + HEADROOM_BYTES + (size_t)players * PER_PLAYER_BYTES * 2;
        if(players > cc_replay_max_players())
            printf("BENCH players=%u status=skip_error workload_players=%u\n",
                   players, (unsigned)cc_replay_max_players());
        else if(largestBlock() < world + 1024 || freeHeap() < need)
            printf("BENCH players=%u status=skip_oom need=%u free_heap=%u largest_block=%u\n",
                   players, (unsigned)need, (unsigned)freeHeap(), (unsigned)largestBlock());
        else
        {
            char count[16];
            snprintf(count, sizeof(count), "%u", players);
            char arg0[] = "benchmarkmapmanager2";
            char arg1[] = "--ms";
            char arg2[] = CC_BENCH_MS;
            char arg3[] = "--players";
            char *argv[] = {arg0, arg1, arg2, arg3, count, NULL};
            printf("BENCH_TRY players=%u free_heap=%u\n", players, (unsigned)freeHeap());
            fflush(stdout);
            const int rc = cc_replay_main(5, argv);
            if(rc != 0)
                printf("BENCH players=%u status=skip_error rc=%d\n", players, rc);
        }
        // Lets the idle task run between cells so the watchdog stays fed.
        vTaskDelay(pdMS_TO_TICKS(50));
        i++;
    }
    printf("BENCH_END free_heap=%u\n", (unsigned)freeHeap());
    fcntl(fileno(stdin), F_SETFL, O_NONBLOCK);
    // Idle without rebooting: a reboot would truncate the serial capture.
    while(true)
        vTaskDelay(pdMS_TO_TICKS(1000));
}
