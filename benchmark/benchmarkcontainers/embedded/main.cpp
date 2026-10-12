// HEADLESS: yes
// benchmark_containers_embedded: the embedded suite of benchmarkcontainers
// (EmbeddedSuite.hpp) as a plain Linux binary, for nodes that cannot load the
// datapack themselves (the 8 MB i486 laptop behind a serial link). The data is
// compiled in (CC_CONTAINERS_DATA_CPP); the files dataset is cut to MemAvailable.
//
// usage: benchmark_containers_embedded [--ms <window per cell>]

#include "../ContainerBench.hpp"
#include "../EmbeddedSuite.hpp"

#include <cstring>
#include <fstream>
#include <string>

// /proc/meminfo MemAvailable (MemFree on a kernel too old to have it), in bytes
static size_t availableBytes()
{
    std::ifstream meminfo("/proc/meminfo");
    std::string key;
    size_t valueKb=0;
    size_t freeKb=0;
    while(meminfo >> key >> valueKb)
    {
        if(key=="MemAvailable:")
            return valueKb*1024;
        if(key=="MemFree:")
            freeKb=valueKb;
        std::string unit;
        meminfo >> unit;
    }
    return freeKb*1024;
}

int main(int argc,char *argv[])
{
    int index=1;
    while(index<argc)
    {
        if(std::strcmp(argv[index],"--ms")==0 && index+1<argc)
        {
            g_windowNs=static_cast<uint64_t>(std::strtoull(argv[index+1],nullptr,10))*1000000;
            index++;
        }
        else
        {
            printf("unknown argument: %s\n",argv[index]);
            return 2;
        }
        index++;
    }
    const size_t available=availableBytes();
    if(available==0 || g_windowNs==0)
    {
        printf("unable to read /proc/meminfo or zero window\n");
        return 1;
    }
    print("mem_available_kb",static_cast<double>(available/1024));
    runEmbeddedSuite(available);
    return 0;
}
