#ifndef CATCHCHALLENGER_BENCHPROBE_HPP
#define CATCHCHALLENGER_BENCHPROBE_HPP

#ifdef CATCHCHALLENGER_BENCHMARK
//Event-loop self-profiling for platforms where no external profiler can be
//installed (ESP32, OpenWrt routers, minimal rootfs). A handful of uint64
//accumulators around the event-loop body -- no heap, no histogram, no
//per-sample storage (<100 bytes total, fits the smallest targets). When
//CATCHCHALLENGER_BENCHMARK is off the #else branch below turns every macro
//into nothing: production builds are byte-identical, zero cost.
//
//Time base: the numbers are compared CROSS-ARCH, so the dump reports TIME
//(us), never raw cycles. The fine-grained slice source is nanoseconds via
//clock_gettime(CLOCK_MONOTONIC) on POSIX, and the CCOUNT cycle register on
//ESP32 (one-instruction read; converted to us at DUMP time via the fixed
//CPU MHz -- never in the hot path).

#include <cstdint>
#if defined(CC_TARGET_ESP32)
#include "esp_cpu.h"        //esp_cpu_get_cycle_count(): 32-bit CCOUNT read
#include "esp_timer.h"      //esp_timer_get_time(): 64-bit wall us
#include "esp_rom_sys.h"    //esp_rom_get_cpu_ticks_per_us(): fixed CPU MHz
#else
#include <ctime>
#endif

namespace CatchChallenger {

//Accumulators around the event loop. volatile because on POSIX the dump
//runs in the SIGINT handler while the loop may be mid-update; the server is
//single-threaded, so volatile gives the same guarantee the existing
//bench_packets_in counters rely on (no atomics needed).
struct BenchLoopProbe
{
    volatile uint64_t busy_ticks;      //ns on POSIX, CPU cycles on ESP32
    volatile uint64_t iterations;      //event-loop wakeups
    uint64_t last_in_ticks;            //raw counter captured at loop-in
    volatile uint64_t window_start_us; //wall us of the FIRST loop-in
    uint64_t last_dump_us;             //ESP32 periodic-dump bookkeeping
};

//monotonic wall clock in us -- read only at first loop-in and at dump time
inline uint64_t benchProbeWallUs()
{
    #if defined(CC_TARGET_ESP32)
    return static_cast<uint64_t>(esp_timer_get_time());
    #else
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC,&t);
    return static_cast<uint64_t>(t.tv_sec)*1000000ULL
          +static_cast<uint64_t>(t.tv_nsec)/1000ULL;
    #endif
}

//fine-grained tick source for busy slices: ns (POSIX) / cycles (ESP32)
inline uint64_t benchProbeTicks()
{
    #if defined(CC_TARGET_ESP32)
    return esp_cpu_get_cycle_count();  //32-bit, wraps ~17.9s @ 240MHz
    #else
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC,&t);
    return static_cast<uint64_t>(t.tv_sec)*1000000000ULL
          +static_cast<uint64_t>(t.tv_nsec);
    #endif
}

//ticks per us: the cross-arch normaliser, emitted alongside the raw ticks
//line so a suspicious conversion can always be re-checked by hand
inline uint64_t benchProbeTicksPerUs()
{
    #if defined(CC_TARGET_ESP32)
    return esp_rom_get_cpu_ticks_per_us(); //fixed: bench firmware runs DFS off
    #else
    return 1000ULL;                        //POSIX ticks are ns
    #endif
}

inline void benchLoopIn(BenchLoopProbe &p)
{
    p.last_in_ticks=benchProbeTicks();
    if(p.window_start_us==0)
        p.window_start_us=benchProbeWallUs();
}

inline void benchLoopOut(BenchLoopProbe &p)
{
    #if defined(CC_TARGET_ESP32)
    //CCOUNT is 32-bit: uint32 delta arithmetic survives one wrap, valid for
    //any busy slice under ~17.9s -- an event-loop pass is far below that.
    const uint32_t now=static_cast<uint32_t>(esp_cpu_get_cycle_count());
    const uint64_t delta=static_cast<uint32_t>(now-static_cast<uint32_t>(p.last_in_ticks));
    #else
    const uint64_t delta=benchProbeTicks()-p.last_in_ticks;
    #endif
    //plain assignment (not +=) so C++20+ builds don't warn -Wvolatile on RMW
    p.busy_ticks=p.busy_ticks+delta;
    p.iterations=p.iterations+1;
}

}

#define CC_BENCH_LOOP_IN(probe)  CatchChallenger::benchLoopIn(probe)
#define CC_BENCH_LOOP_OUT(probe) CatchChallenger::benchLoopOut(probe)

#else //CATCHCHALLENGER_BENCHMARK

#define CC_BENCH_LOOP_IN(probe)  do {} while(0)
#define CC_BENCH_LOOP_OUT(probe) do {} while(0)

#endif //CATCHCHALLENGER_BENCHMARK

#endif // CATCHCHALLENGER_BENCHPROBE_HPP
