// What one benchmark cell costs the ESP32 (CC_TARGET_ESP32 builds only):
//   cpu_percent  = share of the cell's wall time the core running it was NOT
//                  idle: (wall - idle task run time on that core) / wall, from
//                  the FreeRTOS run-time stats (esp_timer microseconds). The
//                  busy/total ratio getrusage()/wall gives on Linux. (The
//                  benchmark task's own counter is useless here: FreeRTOS only
//                  updates it when the task is switched out, and nothing of
//                  equal priority ever preempts it.)
//   heap_used_kb = free heap at the start - lowest TOTAL free heap seen at the
//                  sample points (ccEsp32ProbeHeap): the memory the cell keeps,
//                  like RSS. heap_caps_get_minimum_free_size() is not used: it
//                  sums each region's own low-water mark, reached at different
//                  times, and overstates a churning workload erratically.
// Needs CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS (+ the U64 counter) in the
// firmware's sdkconfig.defaults.
#ifndef CC_BENCH_ESP32_PROBE_HPP
#define CC_BENCH_ESP32_PROBE_HPP
#ifdef CC_TARGET_ESP32

#include <cstddef>
#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"

struct CcEsp32Sample
{
    uint64_t busyUs;
    int64_t wallUs;
    size_t freeStart;
    size_t freeLowest;
};

// Microseconds since boot the calling task's core spent outside its idle task.
static inline uint64_t ccEsp32BusyUs()
{
    const uint64_t idle = (uint64_t)ulTaskGetIdleRunTimeCounterForCore(xPortGetCoreID());
    const uint64_t now = (uint64_t)esp_timer_get_time();
    return now > idle ? now - idle : 0;
}

static inline CcEsp32Sample ccEsp32ProbeStart()
{
    CcEsp32Sample sample;
    sample.busyUs = ccEsp32BusyUs();
    sample.wallUs = esp_timer_get_time();
    sample.freeStart = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    sample.freeLowest = sample.freeStart;
    return sample;
}

// Call where the cell holds its working set (after a tick), outside any timed
// window.
static inline void ccEsp32ProbeHeap(CcEsp32Sample &sample)
{
    const size_t freeNow = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    if(freeNow < sample.freeLowest)
        sample.freeLowest = freeNow;
}

static inline void ccEsp32ProbeStop(CcEsp32Sample &sample, double &cpuPercent,
                                    long &heapUsedKb)
{
    ccEsp32ProbeHeap(sample);
    const int64_t wall = esp_timer_get_time() - sample.wallUs;
    const uint64_t busy = ccEsp32BusyUs() - sample.busyUs;
    cpuPercent = wall > 0 ? (double)busy * 100.0 / (double)wall : 0.0;
    if(cpuPercent > 100.0)
        cpuPercent = 100.0;
    heapUsedKb = (long)((sample.freeStart - sample.freeLowest) / 1024);
}

#endif
#endif
