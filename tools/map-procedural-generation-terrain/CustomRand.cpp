#include "CustomRand.h"

#include <xxhash.h>

#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>

//What a reason keeps: the seed derived once from (world seed, salt, reason), and
//how many draws it already served.
struct CustomRandStream
{
    uint64_t seed;
    uint64_t counter;
};

static uint64_t customRandWorldSeed=0;
static uint64_t customRandCurrentSalt=0;
static std::unordered_map<std::string,CustomRandStream> customRandStreams;

void customRandSeed(const uint64_t &seed)
{
    customRandWorldSeed=seed;
    customRandCurrentSalt=0;
    customRandStreams.clear();
}

void customRandSalt(const uint64_t &salt)
{
    customRandCurrentSalt=salt;
    customRandStreams.clear();
}

int customRand(const char * const reason)
{
    if(reason==NULL || *reason=='\0')
    {
        //a draw with no reason has no stream of its own: it would silently share
        //one with every other unnamed draw, which is what this generator exists
        //to stop. Say so instead of hiding it, and hand back a fixed value.
        std::cerr << "customRand() called with no reason" << std::endl;
        return 0;
    }
    const std::string key(reason);
    std::unordered_map<std::string,CustomRandStream>::iterator stream=customRandStreams.find(key);
    if(stream==customRandStreams.end())
    {
        CustomRandStream newStream;
        //the reason seed: world seed and salt go in as the hash SEED (a value
        //argument), the reason goes in as the hashed CHARACTERS. No integer is
        //hashed through its bytes, so the result does not depend on the endianness
        //of the host.
        newStream.seed=XXH64(key.c_str(),key.size(),
                             customRandWorldSeed^(customRandCurrentSalt*0x9E3779B97F4A7C15ull));
        newStream.counter=0;
        stream=customRandStreams.insert(std::pair<std::string,CustomRandStream>(key,newStream)).first;
    }
    //the counter is laid out little endian by hand, for that same reason
    uint8_t counterBytes[8];
    uint64_t counter=stream->second.counter;
    unsigned int byteIndex=0;
    while(byteIndex<sizeof(counterBytes))
    {
        counterBytes[byteIndex]=(uint8_t)(counter&0xFFu);
        counter>>=8;
        byteIndex++;
    }
    stream->second.counter++;
    return (int)(XXH64(counterBytes,sizeof(counterBytes),stream->second.seed)&(uint64_t)customRandMax);
}
