#ifndef CATCHCHALLENGER_SERVER_NO_COMPRESSION
#ifndef COMPRESSIONPROTOCOL_H
#define COMPRESSIONPROTOCOL_H

#include "lib.h"

#include <cstdint>
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
