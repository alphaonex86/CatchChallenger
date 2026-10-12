// HEADLESS: yes
// Measures std::vector vs std::list vs std::deque (the default container of
// std::queue) on REAL datapack data, read with the production loader: the
// datapack file list, the items and the monsters, plus the small FIFO the
// server Client keeps per player (DB callbacks, pending teleports).
// Containers: vector (push_back growth), vector_reserved (reserve(n) first, as a
// deserialiser does), list, deque. Metrics, per <dataset>.<container>:
//   build_ns_per_elem  ns   lower   construct by push_back, fixed time
//   scan_elems_per_s   1/s  higher  read-only full traversal, fixed time
//   heap_bytes         B    lower   bytes requested from the heap by one built container
//   heap_blocks        -    lower   heap blocks of one built container (each has an allocator header)
//   cpu_percent        %    lower   process CPU over the cell (single thread: 100 = one core)
// FIFO, per fifo<depth>.<container>: ops_per_s (higher), allocs_per_op (lower),
// empty_heap_bytes (lower, what an EMPTY container already holds).
// Deterministic data; the timings are sampled (fixed-time window per cell).
//
// usage: benchmark_containers --datapack <dir> [--ms <window per cell>]

#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"

#include <sys/resource.h>
#include <sys/stat.h>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <new>
#include <string>
#include <vector>

// ---- heap accounting: every operator new goes through here ----------------
static uint64_t g_liveBytes=0;
static uint64_t g_liveBlocks=0;
static uint64_t g_totalBlocks=0;
static const size_t HEADER=alignof(std::max_align_t)>sizeof(size_t)?alignof(std::max_align_t):sizeof(size_t);

static void *countedAlloc(size_t size)
{
    unsigned char *raw=static_cast<unsigned char *>(malloc(size+HEADER));
    if(raw==nullptr)
    {
        std::cerr << "out of memory allocating " << size << " bytes" << std::endl;
        abort();
    }
    *reinterpret_cast<size_t *>(raw)=size;
    g_liveBytes+=size;
    g_liveBlocks++;
    g_totalBlocks++;
    return raw+HEADER;
}

static void countedFree(void *pointer)
{
    if(pointer==nullptr)
        return;
    unsigned char *raw=static_cast<unsigned char *>(pointer)-HEADER;
    g_liveBytes-=*reinterpret_cast<size_t *>(raw);
    g_liveBlocks--;
    free(raw);
}

void *operator new(size_t size) { return countedAlloc(size); }
void *operator new[](size_t size) { return countedAlloc(size); }
void *operator new(size_t size,const std::nothrow_t &) noexcept { return countedAlloc(size); }
void *operator new[](size_t size,const std::nothrow_t &) noexcept { return countedAlloc(size); }
void operator delete(void *pointer) noexcept { countedFree(pointer); }
void operator delete[](void *pointer) noexcept { countedFree(pointer); }
void operator delete(void *pointer,size_t) noexcept { countedFree(pointer); }
void operator delete[](void *pointer,size_t) noexcept { countedFree(pointer); }

// ---- clocks --------------------------------------------------------------
static uint64_t nowNs()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

static uint64_t cpuUs()
{
    struct rusage usage;
    if(getrusage(RUSAGE_SELF,&usage)!=0)
        return 0;
    return static_cast<uint64_t>(usage.ru_utime.tv_sec+usage.ru_stime.tv_sec)*1000000
           +static_cast<uint64_t>(usage.ru_utime.tv_usec+usage.ru_stime.tv_usec);
}

static uint64_t g_windowNs=200000000;
static volatile uint64_t g_sink=0;//keeps the scans from being optimised away

static void print(const std::string &key,const double &value)
{
    std::cout << "BENCH " << key << "=" << value << std::endl;
}

// ---- the datasets ----------------------------------------------------------
struct FileEntry
{
    std::string path;
    uint32_t size;
};

static uint64_t weight(const FileEntry &entry) { return entry.size+entry.path.size(); }
static uint64_t weight(const CatchChallenger::Item &item) { return item.price; }
static uint64_t weight(const CatchChallenger::Monster &monster) { return monster.give_xp+monster.learn.size(); }

// vector_reserved: what a deserialiser does, it knows the count and reserves it
template <class T>
static void reserveIfAsked(std::vector<T> &container,const size_t &count,const bool &reserve) { if(reserve) container.reserve(count); }
template <class T>
static void reserveIfAsked(std::list<T> &,const size_t &,const bool &) {}
template <class T>
static void reserveIfAsked(std::deque<T> &,const size_t &,const bool &) {}

// ---- the generic part: one function per access pattern ---------------------
template <class Container,class T>
static void measureDataset(const std::string &name,const std::string &containerName,const std::vector<T> &source,const bool &reserve)
{
    const std::string prefix=name+"."+containerName+".";
    const uint64_t cpuStart=cpuUs();
    const uint64_t wallStart=nowNs();

    //build: construct from scratch by push_back for the whole window
    uint64_t builds=0;
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
        }
        g_sink+=container.size();
        builds++;
    }
    const uint64_t buildNs=nowNs()-start;
    print(prefix+"build_ns_per_elem",static_cast<double>(buildNs)/static_cast<double>(builds*source.size()));

    //memory of one built container
    const uint64_t bytesBefore=g_liveBytes;
    const uint64_t blocksBefore=g_liveBlocks;
    Container container;
    reserveIfAsked(container,source.size(),reserve);
    size_t index=0;
    while(index<source.size())
    {
        container.push_back(source[index]);
        index++;
    }
    print(prefix+"heap_bytes",static_cast<double>(g_liveBytes-bytesBefore));
    print(prefix+"heap_blocks",static_cast<double>(g_liveBlocks-blocksBefore));

    //read-only full traversal for the whole window
    uint64_t scanned=0;
    start=nowNs();
    while(nowNs()-start<g_windowNs)
    {
        uint64_t sum=0;
        typename Container::const_iterator it=container.cbegin();
        while(it!=container.cend())
        {
            sum+=weight(*it);
            ++it;
        }
        g_sink+=sum;
        scanned+=container.size();
    }
    const uint64_t scanNs=nowNs()-start;
    print(prefix+"scan_elems_per_s",static_cast<double>(scanned)*1e9/static_cast<double>(scanNs));

    const uint64_t wallUs=(nowNs()-wallStart)/1000;
    print(prefix+"cpu_percent",wallUs==0?0.0:100.0*static_cast<double>(cpuUs()-cpuStart)/static_cast<double>(wallUs));
}

// the Client's per-player queues hold a few small entries (PlayerOnMap is 6 bytes)
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
    print(prefix+"empty_heap_bytes",static_cast<double>(g_liveBytes-emptyBefore));

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
            g_sink+=container.front().x;
            popFront(container);
            burst++;
        }
        ops+=256;
    }
    const uint64_t elapsed=nowNs()-start;
    print(prefix+"ops_per_s",static_cast<double>(ops)*1e9/static_cast<double>(elapsed));
    print(prefix+"allocs_per_op",static_cast<double>(g_totalBlocks-blocksStart)/static_cast<double>(ops));

    const uint64_t wallUs=(nowNs()-wallStart)/1000;
    print(prefix+"cpu_percent",wallUs==0?0.0:100.0*static_cast<double>(cpuUs()-cpuStart)/static_cast<double>(wallUs));
}

template <class T>
static void measureAll(const std::string &name,const std::vector<T> &source)
{
    print(name+".elements",static_cast<double>(source.size()));
    print(name+".sizeof_elem",static_cast<double>(sizeof(T)));
    if(source.empty())
    {
        std::cerr << "dataset " << name << " is empty, skipped" << std::endl;
        return;
    }
    measureDataset<std::vector<T> >(name,"vector",source,false);
    measureDataset<std::vector<T> >(name,"vector_reserved",source,true);
    measureDataset<std::list<T> >(name,"list",source,false);
    measureDataset<std::deque<T> >(name,"deque",source,false);
}

int main(int argc,char *argv[])
{
    std::string datapack;
    int index=1;
    while(index<argc)
    {
        const std::string arg=argv[index];
        if(arg=="--datapack" && index+1<argc)
        {
            datapack=argv[index+1];
            index++;
        }
        else if(arg=="--ms" && index+1<argc)
        {
            g_windowNs=static_cast<uint64_t>(std::strtoull(argv[index+1],nullptr,10))*1000000;
            index++;
        }
        else
        {
            std::cerr << "unknown argument: " << arg << std::endl;
            return 2;
        }
        index++;
    }
    if(datapack.empty() || g_windowNs==0)
    {
        std::cerr << "usage: " << argv[0] << " --datapack <dir> [--ms <window per cell>]" << std::endl;
        return 2;
    }
    if(datapack.back()!='/')
        datapack+='/';

    //files: the datapack's file list, as the server keeps it to serve the datapack
    std::vector<FileEntry> files;
    {
        const std::vector<std::string> list=CatchChallenger::FacilityLibGeneral::listFolder(datapack);
        size_t fileIndex=0;
        while(fileIndex<list.size())
        {
            struct stat info;
            if(::stat((datapack+list[fileIndex]).c_str(),&info)==0)
            {
                FileEntry entry;
                entry.path=list[fileIndex];
                entry.size=static_cast<uint32_t>(info.st_size);
                files.push_back(entry);
            }
            else
                std::cerr << "stat failed: " << list[fileIndex] << std::endl;
            fileIndex++;
        }
    }
    if(files.empty())
    {
        std::cerr << "no file in the datapack: " << datapack << std::endl;
        return 1;
    }

    //items and monsters: parsed by the production loader
    CatchChallenger::CommonDatapack::commonDatapack.parseDatapack(datapack);
    std::vector<CatchChallenger::Item> items;
    {
        uint32_t id=0;
        while(id<=CatchChallenger::CommonDatapack::commonDatapack.get_itemMaxId())
        {
            if(CatchChallenger::CommonDatapack::commonDatapack.has_item(static_cast<CATCHCHALLENGER_TYPE_ITEM>(id)))
                items.push_back(CatchChallenger::CommonDatapack::commonDatapack.get_item(static_cast<CATCHCHALLENGER_TYPE_ITEM>(id)));
            id++;
        }
    }
    std::vector<CatchChallenger::Monster> monsters;
    {
        uint32_t id=0;
        while(id<=CatchChallenger::CommonDatapack::commonDatapack.get_monstersMaxId())
        {
            if(CatchChallenger::CommonDatapack::commonDatapack.has_monster(static_cast<CATCHCHALLENGER_TYPE_MONSTER>(id)))
                monsters.push_back(CatchChallenger::CommonDatapack::commonDatapack.get_monster(static_cast<CATCHCHALLENGER_TYPE_MONSTER>(id)));
            id++;
        }
    }

    measureAll("files",files);
    measureAll("items",items);
    measureAll("monsters",monsters);

    unsigned int depth=1;
    while(depth<=16)
    {
        measureFifo<std::vector<QueueEntry> >(depth,"vector");
        measureFifo<std::list<QueueEntry> >(depth,"list");
        measureFifo<std::deque<QueueEntry> >(depth,"deque");
        depth*=16;
    }
    std::cout << "BENCH done=1" << std::endl;
    return 0;
}
