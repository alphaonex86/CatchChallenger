// benchmark/benchmarkcontainers/ContainerBench.hpp -- the measuring core shared
// by the host binary (main.cpp) and the ESP32 firmware (esp32/main/app_main.cpp).
// Include it in exactly ONE translation unit: it replaces operator new/delete.
//
// Every cell runs for a fixed window and prints `BENCH <key>=<value>` lines.
// Heap: the real footprint of each block (what the allocator made usable + its
// own chunk header), counted by the replaced operator new; no header of ours
// is added, so the allocator sees the program's real requests.
// Each pass ends with a compiler barrier on the container: no pass can be
// hoisted, merged or have its allocations elided.
#ifndef CC_BENCH_CONTAINERBENCH_HPP
#define CC_BENCH_CONTAINERBENCH_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <list>
#include <new>
#include <string>
#include <vector>

#ifdef CC_TARGET_ESP32
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "../esp32_probe.hpp"
#else
#include <malloc.h>
#include <sys/resource.h>
#include <chrono>
#endif

// ---- heap accounting -------------------------------------------------------
static uint64_t g_liveBytes=0;
static uint64_t g_liveBlocks=0;
static uint64_t g_totalBlocks=0;

static uint64_t blockBytes(void *pointer)
{
    #ifdef CC_TARGET_ESP32
    return heap_caps_get_allocated_size(pointer)+sizeof(size_t);
    #else
    return malloc_usable_size(pointer)+sizeof(size_t);
    #endif
}

static void *countedAlloc(size_t size)
{
    void *pointer=malloc(size==0?1:size);
    if(pointer==nullptr)
    {
        printf("BENCH_OOM bytes=%u\n",static_cast<unsigned int>(size));
        fflush(stdout);
        abort();
    }
    g_liveBytes+=blockBytes(pointer);
    g_liveBlocks++;
    g_totalBlocks++;
    return pointer;
}

static void countedFree(void *pointer)
{
    if(pointer==nullptr)
        return;
    g_liveBytes-=blockBytes(pointer);
    g_liveBlocks--;
    free(pointer);
}

void *operator new(size_t size) { return countedAlloc(size); }
void *operator new[](size_t size) { return countedAlloc(size); }
void *operator new(size_t size,const std::nothrow_t &) noexcept { return countedAlloc(size); }
void *operator new[](size_t size,const std::nothrow_t &) noexcept { return countedAlloc(size); }
void operator delete(void *pointer) noexcept { countedFree(pointer); }
void operator delete[](void *pointer) noexcept { countedFree(pointer); }
void operator delete(void *pointer,size_t) noexcept { countedFree(pointer); }
void operator delete[](void *pointer,size_t) noexcept { countedFree(pointer); }

// the container escapes: the compiler must assume any memory changed
static void barrier(const void *pointer)
{
    asm volatile("" : : "g"(pointer) : "memory");
}

// ---- clocks ----------------------------------------------------------------
static uint64_t nowNs()
{
    #ifdef CC_TARGET_ESP32
    return static_cast<uint64_t>(esp_timer_get_time())*1000;
    #else
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
    #endif
}

// CPU time of this program: rusage on the host, the core's busy time on the ESP32
static uint64_t cpuUs()
{
    #ifdef CC_TARGET_ESP32
    return ccEsp32BusyUs();
    #else
    struct rusage usage;
    if(getrusage(RUSAGE_SELF,&usage)!=0)
        return 0;
    return static_cast<uint64_t>(usage.ru_utime.tv_sec+usage.ru_stime.tv_sec)*1000000
           +static_cast<uint64_t>(usage.ru_utime.tv_usec+usage.ru_stime.tv_usec);
    #endif
}

static uint64_t g_windowNs=200000000;
// Fixed TIME, not fixed iterations: the clock is read every CHECK_* elements
// inside a pass, so a slow node stops after its window plus one chunk, never
// after a whole dataset pass (a pass of a big dataset on a 486 is seconds).
static const size_t CHECK_BUILD=64;
static const size_t CHECK_SCAN=1024;
static volatile uint64_t g_sink=0;

static void print(const std::string &key,const double &value)
{
    printf("BENCH %s=%.6g\n",key.c_str(),value);
    fflush(stdout);
}

// ---- datasets shared by both builds ----------------------------------------
struct FileEntry
{
    std::string path;
    uint32_t size;
};

// an item as the datapack stores it (CatchChallenger::Item has the same layout)
struct ItemEntry
{
    uint32_t price;
    bool consumeAtUse;
};

static uint64_t weight(const FileEntry &entry) { return entry.size+entry.path.size(); }
static uint64_t weight(const ItemEntry &item) { return item.price; }

// vector_reserved: what a deserialiser does, it knows the count and reserves it
template <class T>
static void reserveIfAsked(std::vector<T> &container,const size_t &count,const bool &reserve) { if(reserve) container.reserve(count); }
template <class T>
static void reserveIfAsked(std::list<T> &,const size_t &,const bool &) {}
template <class T>
static void reserveIfAsked(std::deque<T> &,const size_t &,const bool &) {}

template <class Container,class T>
static void fillContainer(Container &container,const std::vector<T> &source,const bool &reserve)
{
    reserveIfAsked(container,source.size(),reserve);
    size_t index=0;
    while(index<source.size())
    {
        container.push_back(source[index]);
        index++;
    }
    barrier(&container);
}

static void printCpu(const std::string &prefix,const uint64_t &cpuStart,const uint64_t &wallStart)
{
    const uint64_t wallUs=(nowNs()-wallStart)/1000;
    print(prefix+"cpu_percent",wallUs==0?0.0:100.0*static_cast<double>(cpuUs()-cpuStart)/static_cast<double>(wallUs));
}

// build, heap of one built container, read-only scan
template <class Container,class T>
static void measureDataset(const std::string &name,const std::string &containerName,const std::vector<T> &source,const bool &reserve)
{
    const std::string prefix=name+"."+containerName+".";
    const uint64_t cpuStart=cpuUs();
    const uint64_t wallStart=nowNs();

    //build: construct by push_back (the destruction of each container included)
    uint64_t pushed=0;
    uint64_t start=nowNs();
    while(nowNs()-start<g_windowNs)
    {
        Container container;
        reserveIfAsked(container,source.size(),reserve);
        size_t index=0;
        while(index<source.size())
        {
            container.push_back(source[index]);
            index++;
            if(index%CHECK_BUILD==0 && nowNs()-start>=g_windowNs)
                break;
        }
        barrier(&container);
        g_sink+=container.size();
        pushed+=index;
    }
    const uint64_t buildNs=nowNs()-start;
    print(prefix+"build_ns_per_elem",static_cast<double>(buildNs)/static_cast<double>(pushed));

    const uint64_t bytesBefore=g_liveBytes;
    const uint64_t blocksBefore=g_liveBlocks;
    Container container;
    fillContainer(container,source,reserve);
    //read the counters BEFORE building the key string: the key may be a heap block too
    const double heapBytes=static_cast<double>(g_liveBytes-bytesBefore);
    const double heapBlocks=static_cast<double>(g_liveBlocks-blocksBefore);
    print(prefix+"heap_bytes",heapBytes);
    print(prefix+"heap_blocks",heapBlocks);

    uint64_t scanned=0;
    start=nowNs();
    while(nowNs()-start<g_windowNs)
    {
        //counted chunks: no per-element test in the inner loop (it stays
        //vectorisable), one clock read per CHECK_SCAN elements
        uint64_t sum=0;
        size_t inPass=0;
        const size_t size=container.size();
        typename Container::const_iterator it=container.cbegin();
        while(inPass<size)
        {
            size_t chunk=size-inPass;
            if(chunk>CHECK_SCAN)
                chunk=CHECK_SCAN;
            inPass+=chunk;
            while(chunk>0)
            {
                sum+=weight(*it);
                ++it;
                chunk--;
            }
            //the end of a pass is checked by the outer loop: one clock read per pass at most
            if(inPass<size && nowNs()-start>=g_windowNs)
                break;
        }
        g_sink+=sum;
        barrier(&container);
        scanned+=inPass;
    }
    const uint64_t scanNs=nowNs()-start;
    print(prefix+"scan_elems_per_s",static_cast<double>(scanned)*1e9/static_cast<double>(scanNs));
    printCpu(prefix,cpuStart,wallStart);
}

template <class T>
static void measureAll(const std::string &name,const std::vector<T> &source)
{
    print(name+".elements",static_cast<double>(source.size()));
    print(name+".sizeof_elem",static_cast<double>(sizeof(T)));
    if(source.empty())
        return;
    measureDataset<std::vector<T> >(name,"vector",source,false);
    measureDataset<std::vector<T> >(name,"vector_reserved",source,true);
    measureDataset<std::list<T> >(name,"list",source,false);
    measureDataset<std::deque<T> >(name,"deque",source,false);
}

// ---- the server Client's per-player FIFO (PlayerOnMap is 6 bytes) ----------
struct QueueEntry
{
    uint16_t mapIndex;
    uint8_t x,y,orientation,pad;
};

static void popFront(std::vector<QueueEntry> &container) { container.erase(container.begin()); }
static void popFront(std::list<QueueEntry> &container) { container.pop_front(); }
static void popFront(std::deque<QueueEntry> &container) { container.pop_front(); }

template <class Container>
static void measureFifo(const unsigned int &depth,const std::string &containerName)
{
    const std::string prefix="fifo"+std::to_string(depth)+"."+containerName+".";
    const uint64_t cpuStart=cpuUs();
    const uint64_t wallStart=nowNs();

    const uint64_t emptyBefore=g_liveBytes;
    Container container;
    barrier(&container);
    const double emptyBytes=static_cast<double>(g_liveBytes-emptyBefore);
    print(prefix+"empty_heap_bytes",emptyBytes);

    QueueEntry entry;
    entry.mapIndex=1;
    entry.x=2;
    entry.y=3;
    entry.orientation=0;
    entry.pad=0;
    unsigned int index=0;
    while(index<depth)
    {
        container.push_back(entry);
        index++;
    }
    //steady state: one in, one out, the depth never changes
    uint64_t ops=0;
    const uint64_t blocksStart=g_totalBlocks;
    const uint64_t start=nowNs();
    while(nowNs()-start<g_windowNs)
    {
        int burst=0;
        while(burst<256)
        {
            entry.x++;
            container.push_back(entry);
            barrier(&container);
            g_sink+=container.front().x;
            popFront(container);
            barrier(&container);
            burst++;
        }
        ops+=256;
    }
    const uint64_t elapsed=nowNs()-start;
    const double allocsPerOp=static_cast<double>(g_totalBlocks-blocksStart)/static_cast<double>(ops);
    print(prefix+"ops_per_s",static_cast<double>(ops)*1e9/static_cast<double>(elapsed));
    print(prefix+"allocs_per_op",allocsPerOp);
    printCpu(prefix,cpuStart,wallStart);
}

static void measureFifos()
{
    unsigned int depth=1;
    while(depth<=16)
    {
        measureFifo<std::vector<QueueEntry> >(depth,"vector");
        measureFifo<std::list<QueueEntry> >(depth,"list");
        measureFifo<std::deque<QueueEntry> >(depth,"deque");
        depth*=16;
    }
}

#endif
