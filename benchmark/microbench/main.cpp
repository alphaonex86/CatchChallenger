// HEADLESS: yes
// Microbenchmark run on a node BEFORE the real benchmarks, to learn what the
// CPU is when /proc/cpuinfo cannot tell (a 486 reports no MHz and no cache).
// Prints, one per line:
//   MICRO mhz=<n>          dependent-add chain rate; an add has a 1-cycle
//                          latency on in-order and out-of-order cores alike,
//                          so adds/s ~ clock in MHz (loop overhead under-reads
//                          by a few %). Unit MHz, higher = faster.
//   MICRO_CACHE kb=<n> ns_x100=<n>   latency of one dependent load over a
//                          random cycle of <n> KiB. Unit ns*100, lower = faster.
//   MICRO cache_kb=<a,b,..>  last size before latency steps up >=1.5x (L1, L2,
//                          ...); a step spread over consecutive doublings
//                          counts once. Heuristic: sharp on old CPUs, rough
//                          where latency rises gradually -- the table above is
//                          the data.
// Pinned to CPU 0 so an asymmetric CPU (big.LITTLE, two-CCD X3D) always
// reports the same core.
// Sampled: each point runs ~TIME_MS; expect a few % noise. No float: integer
// math only, like the server.
#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>
#endif
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#define TIME_MS 150u
#define MIN_KB 1u
#define MAX_KB 65536u

// Largest working set to probe: half of the memory the kernel says is
// available, so the probe never pushes a small board into swap or the OOM
// killer. 1 MiB when /proc/meminfo cannot be read.
static unsigned int maxProbeKb()
{
    std::ifstream meminfo("/proc/meminfo");
    std::string key;
    unsigned long long value = 0;
    std::string unit;
    unsigned long long available = 0, freeKb = 0;
    while(meminfo >> key >> value >> unit) {
        if(key == "MemAvailable:") available = value;
        else if(key == "MemFree:") freeKb = value;
    }
    const unsigned long long kb = (available ? available : freeKb) / 2;
    if(kb == 0) {
        std::cerr << "microbench: /proc/meminfo unreadable, probing up to 1024 KiB" << std::endl;
        return 1024;
    }
    return kb < MAX_KB ? (unsigned int)kb : MAX_KB;
}

static uint64_t nowNs()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

#define ADD8 x += 1; __asm__ volatile("" : "+r"(x)); \
             x += 1; __asm__ volatile("" : "+r"(x)); \
             x += 1; __asm__ volatile("" : "+r"(x)); \
             x += 1; __asm__ volatile("" : "+r"(x)); \
             x += 1; __asm__ volatile("" : "+r"(x)); \
             x += 1; __asm__ volatile("" : "+r"(x)); \
             x += 1; __asm__ volatile("" : "+r"(x)); \
             x += 1; __asm__ volatile("" : "+r"(x));

// 64 chained adds per iteration so the loop branch is ~3% of the work.
static uint64_t measureMhz()
{
    const uint64_t budget = (uint64_t)TIME_MS * 1000000ull;
    uint32_t x = 0;
    uint64_t iterations = 0;
    const uint64_t start = nowNs();
    uint64_t elapsed = 0;
    while(elapsed < budget) {
        unsigned int k = 0;
        while(k < 1000) {
            ADD8 ADD8 ADD8 ADD8 ADD8 ADD8 ADD8 ADD8
            k++;
        }
        iterations += 1000;
        elapsed = nowNs() - start;
    }
    if(x == 0xFFFFFFFFu)
        std::cerr << "unreachable" << std::endl;
    return iterations * 64ull * 1000ull / elapsed;
}

static uint32_t rngState = 12345u;
static uint32_t nextRandom()
{
    rngState = rngState * 1664525u + 1013904223u;
    return rngState >> 8;
}

// One node per 16 bytes (the 486 cache line: a larger node would touch only
// part of the lines and read the L1 twice too big), linked in a random single
// cycle by an in-place Sattolo shuffle, so the arena is the ONLY allocation (a
// 486 with 8 MB has under 1 MB free). Every load depends on the previous one
// and the prefetcher cannot guess the next address.
#define NODE_WORDS 4u
static uint64_t measureLoadNsX100(unsigned int kb)
{
    const size_t count = (size_t)kb * 1024 / (NODE_WORDS * sizeof(uint32_t));
    std::vector<uint32_t> arena(count * NODE_WORDS);
    size_t i = 0;
    while(i < count) {
        arena[i * NODE_WORDS] = (uint32_t)i;
        i++;
    }
    i = count - 1;
    while(i > 0) {
        const size_t j = nextRandom() % i;
        const uint32_t t = arena[i * NODE_WORDS];
        arena[i * NODE_WORDS] = arena[j * NODE_WORDS];
        arena[j * NODE_WORDS] = t;
        i--;
    }
    const uint64_t budget = (uint64_t)TIME_MS * 1000000ull;
    uint32_t at = 0;
    uint64_t loads = 0;
    const uint64_t start = nowNs();
    uint64_t elapsed = 0;
    while(elapsed < budget) {
        unsigned int k = 0;
        while(k < 4096) {
            at = *(volatile uint32_t *)&arena[(size_t)at * NODE_WORDS];
            k++;
        }
        loads += 4096;
        elapsed = nowNs() - start;
    }
    if(at == 0xFFFFFFFFu)
        std::cerr << "unreachable" << std::endl;
    return elapsed * 100ull / loads;
}

static void pinToCpu0()
{
#ifdef __linux__
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    if(sched_setaffinity(0, sizeof(set), &set) != 0)
        std::cerr << "microbench: cannot pin to CPU 0, results may vary by core" << std::endl;
#endif
}

int main()
{
    pinToCpu0();
    std::cout << "MICRO mhz=" << measureMhz() << std::endl;
    std::vector<unsigned int> sizes;
    std::vector<uint64_t> latencies;
    unsigned int kb = MIN_KB;
    const unsigned int limitKb = maxProbeKb();
    while(kb <= limitKb) {
        const uint64_t ns = measureLoadNsX100(kb);
        sizes.push_back(kb);
        latencies.push_back(ns);
        std::cout << "MICRO_CACHE kb=" << kb << " ns_x100=" << ns << std::endl;
        kb *= 2;
    }
    std::cout << "MICRO cache_kb=";
    bool first = true;
    bool previousWasStep = false;
    size_t i = 1;
    while(i < sizes.size()) {
        const bool step = latencies[i] * 2 >= latencies[i - 1] * 3;
        if(step && !previousWasStep) {
            std::cout << (first ? "" : ",") << sizes[i - 1];
            first = false;
        }
        previousWasStep = step;
        i++;
    }
    std::cout << std::endl;
    return 0;
}
