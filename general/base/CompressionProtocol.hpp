#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
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

    static char tempBigBufferForCompressedOutput[CATCHCHALLENGER_COMPRESSBUFFERSIZE];
    static char tempBigBufferForUncompressedInput[CATCHCHALLENGER_COMPRESSBUFFERSIZE];
};

#endif // COMPRESSIONPROTOCOL_H
#endif // EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
