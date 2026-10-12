// benchmark/benchmarkcontainers/EmbeddedSuite.hpp -- the cells run where the
// datapack cannot be loaded (ESP32: no filesystem; T2100: 8 MB RAM behind a
// serial link): the files + items datasets the host emitted as C++ arrays
// (benchmark_containers --emit-esp32), plus the per-player FIFO pattern.
// Include in the one translation unit of the binary (ContainerBench.hpp replaces operator new).
#ifndef CC_BENCH_EMBEDDEDSUITE_HPP
#define CC_BENCH_EMBEDDEDSUITE_HPP

#include "ContainerBench.hpp"
#include <cstring>

extern const char * const cc_files_path[];
extern const uint32_t cc_files_size[];
extern const size_t cc_files_count;
extern const uint32_t cc_items_price[];
extern const uint8_t cc_items_consume[];
extern const size_t cc_items_count;

// `freeBytes`: the memory the run may use. The files dataset is cut to what it
// holds: per element its FileEntry + the path block + a list node, three copies
// alive at worst (source, container, vector growth), half of it kept free.
static void runEmbeddedSuite(const size_t &freeBytes)
{
    size_t bytes=0;
    size_t index=0;
    while(index<cc_files_count)
    {
        bytes+=sizeof(FileEntry)+strlen(cc_files_path[index])+1+32;
        index++;
    }
    const size_t perFile=cc_files_count==0?1:bytes/cc_files_count;
    size_t count=freeBytes/2/(3*perFile);
    if(count>cc_files_count)
        count=cc_files_count;
    print("files.available",static_cast<double>(cc_files_count));
    {
        std::vector<FileEntry> files;
        files.reserve(count);
        index=0;
        while(index<count)
        {
            FileEntry entry;
            entry.path=cc_files_path[index];
            entry.size=cc_files_size[index];
            files.push_back(entry);
            index++;
        }
        measureAll("files",files);
    }
    {
        std::vector<ItemEntry> items;
        items.reserve(cc_items_count);
        index=0;
        while(index<cc_items_count)
        {
            ItemEntry entry;
            entry.price=cc_items_price[index];
            entry.consumeAtUse=cc_items_consume[index]!=0;
            items.push_back(entry);
            index++;
        }
        measureAll("items",items);
    }
    measureFifos();
    print("done",1);
}

#endif
