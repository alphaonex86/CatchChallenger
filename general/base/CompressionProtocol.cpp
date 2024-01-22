#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
#include "CompressionProtocol.hpp"
#include <zstd.h>
#include <iostream>

char CompressionProtocol::tempBigBufferForUncompressedInput[];
char CompressionProtocol::tempBigBufferForCompressedOutput[];
uint8_t CompressionProtocol::compressionLevel=6;
#ifndef CATCHCHALLENGERSERVERDROPIFCLENT
CompressionProtocol::CompressionType    CompressionProtocol::compressionTypeClient=CompressionProtocol::CompressionType::None;
#endif
CompressionProtocol::CompressionType    CompressionProtocol::compressionTypeServer=CompressionProtocol::CompressionType::None;

CompressionProtocol::CompressionProtocol()
{
}

int32_t CompressionProtocol::decompressZstandard(const char * const input, const uint32_t &intputSize, char * const output, const uint32_t &maxOutputSize)
{
    size_t const dSize = ZSTD_decompress(output, maxOutputSize, input, intputSize);
    if (ZSTD_isError(dSize)) {
        std::cerr << "error compressing" << ZSTD_getErrorName(dSize) << std::endl;
        return -1;
    }
    return dSize;
}

int32_t CompressionProtocol::compressZstandard(const char * const input, const uint32_t &intputSize, char * const output, const uint32_t &maxOutputSize)
{
    size_t const cSize = ZSTD_compress(output, maxOutputSize, input, intputSize, CompressionProtocol::compressionLevel);
    if (ZSTD_isError(cSize)) {
        std::cerr << "error compressing" << ZSTD_getErrorName(cSize) << std::endl;
        return -1;
    }
    return cSize;
}

int32_t CompressionProtocol::computeDecompression(const char* const source, char* const dest, const unsigned int &sourceSize, const unsigned int &maxDecompressedSize, const CompressionType &compressionType)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(maxDecompressedSize<sourceSize)
    {
        std::cerr << "maxDecompressedSize<compressedSize in ProtocolParsingBase::computeDecompression" << std::endl;
        abort();
    }
    #endif
    switch(compressionType)
    {
        case CompressionType::None:
            std::cerr << "CompressionType::None in ProtocolParsingBase::computeDecompression, do direct mapping" << std::endl;
            abort();
            return -1;
        break;
        case CompressionType::Zstandard:
        default:
            return CompressionProtocol::decompressZstandard(source,sourceSize,dest,maxDecompressedSize);
        break;
    }
}

int32_t CompressionProtocol::computeCompression(const char* const source, char* const dest, const unsigned int &sourceSize, const unsigned int &maxDecompressedSize, const CompressionType &compressionType)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(maxDecompressedSize<sourceSize)
    {
        std::cerr << "maxDecompressedSize<compressedSize in ProtocolParsingBase::computeDecompression" << std::endl;
        abort();
    }
    #endif
    switch(compressionType)
    {
        case CompressionType::None:
            std::cerr << "CompressionType::None in ProtocolParsingBase::computeDecompression, do direct mapping" << std::endl;
            abort();
            return -1;
        break;
        case CompressionType::Zstandard:
        default:
            return CompressionProtocol::compressZstandard(source,sourceSize,dest,maxDecompressedSize);
        break;
    }
}
#endif
