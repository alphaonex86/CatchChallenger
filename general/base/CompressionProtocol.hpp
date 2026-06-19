#ifndef CATCHCHALLENGER_SERVER_NO_COMPRESSION
#ifndef COMPRESSIONPROTOCOL_H
#define COMPRESSIONPROTOCOL_H

#include "lib.h"

#include <cstdint>
#include <vector>
#define CATCHCHALLENGER_COMPRESSBUFFERSIZE 16*1024*1024

class DLL_PUBLIC CompressionProtocol
{
public:
    CompressionProtocol();

    enum CompressionType : uint8_t
    {
        None=0x00,
        Zstandard = 0x04
    };

    static uint8_t compressionLevel;
    static int32_t decompressZstandard(const char * const input, const uint32_t &intputSize, char * const output, const uint32_t &maxOutputSize);
    //Decompressed length read from the zstd frame header WITHOUT decompressing.
    //Returns -1 when input is not a valid zstd frame or the size is unknown, so
    //the caller can reject garbage/oversized blocks before allocating the output
    //buffer (a memory-amplification guard on attacker-controlled input).
    static int64_t decompressedSizeZstandard(const char * const input, const uint32_t &intputSize);
    //Reusable one-shot decompression scratch, SHARED and grown to the actual
    //frame size. Reused across calls so a long-running server never alloc/free
    //(and fragments the heap) on every character/datapack block; it starts empty
    //so constrained targets (ESP32 / MS-DOS, or builds that never enable
    //compression) pay nothing up front, never the 16 MB worst case. Single-
    //threaded by contract (epoll server / single client parser): the pointer
    //returned by decompressToScratch() is valid only until the next call.
    static std::vector<uint8_t> decompressScratch;
    //Decompress sourceSize bytes at source into decompressScratch and point
    //*out at the result. Returns the decompressed length, or -1 on a frame that
    //is malformed / unknown-size / larger than CATCHCHALLENGER_COMPRESSBUFFERSIZE
    //(rejected from the header alone, before any large allocation).
    static int64_t decompressToScratch(const char * const source, const uint32_t &sourceSize, const char ** const out, const CompressionType &compressionType);
    static int32_t compressZstandard(const char * const input, const uint32_t &intputSize, char * const output, const uint32_t &maxOutputSize);
    static int32_t computeDecompression(const char* const source, char* const dest, const unsigned int &sourceSize, const unsigned int &maxDecompressedSize, const CompressionType &compressionType);
    static int32_t computeCompression(const char* const source, char* const dest, const unsigned int &sourceSize, const unsigned int &maxDecompressedSize, const CompressionType &compressionType);
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT

    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        #ifdef CATCHCHALLENGERSERVERDROPIFCLENT
            #error Can t be CATCHCHALLENGER_CLASS_ONLYGAMESERVER and CATCHCHALLENGERSERVERDROPIFCLENT
        #endif
    #endif

    static CompressionType compressionTypeClient;
    #endif
    static CompressionType compressionTypeServer;

    //Previously two static char[CATCHCHALLENGER_COMPRESSBUFFERSIZE] (16 MB each)
    //ate 32 MB of .bss for scratch space that was only used during a few
    //(de)compression call sites. Each call site now declares a local
    //std::vector<uint8_t> sized to actual need; RAII frees it on return.
};

#endif // COMPRESSIONPROTOCOL_H
#endif // CATCHCHALLENGER_SERVER_NO_COMPRESSION
