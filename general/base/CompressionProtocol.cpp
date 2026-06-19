#ifndef CATCHCHALLENGER_SERVER_NO_COMPRESSION
#include "CompressionProtocol.hpp"
#include <zstd.h>
#include <iostream>

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

std::vector<uint8_t> CompressionProtocol::decompressScratch;

int64_t CompressionProtocol::decompressToScratch(const char * const source, const uint32_t &sourceSize, const char ** const out, const CompressionType &compressionType)
{
    //Probe the frame header: -1 = not a zstd frame at all → reject (a garbage
    //block can't force a large allocation). -2 = valid frame with no pledged
    //size (the streaming-compressed datapack file stream) → fall back to the
    //worst-case buffer rather than rejecting a good frame. >=0 = exact size →
    //right-size (the common one-shot character-block path, keeps RSS small).
    const int64_t expectedSize=CompressionProtocol::decompressedSizeZstandard(source,sourceSize);
    if(expectedSize==-1 || expectedSize>(int64_t)CATCHCHALLENGER_COMPRESSBUFFERSIZE)
        return -1;
    //computeDecompression()'s HARDENED guard abort()s when the destination is
    //smaller than the compressed input; a (rare) highly-compressible frame can
    //decompress to fewer bytes than it occupies compressed, so floor at sourceSize.
    size_t needed=(expectedSize==-2)
        ? (size_t)CATCHCHALLENGER_COMPRESSBUFFERSIZE
        : (size_t)expectedSize;
    if(needed<sourceSize)
        needed=sourceSize;
    //Grow-only: resize() up to the high-water mark and keep that capacity for the
    //next call (no per-call alloc/free). Never shrink back, so the buffer settles
    //at the largest block seen and stops reallocating.
    if(decompressScratch.size()<needed)
        decompressScratch.resize(needed);
    const int32_t decompressResult=CompressionProtocol::computeDecompression(source,reinterpret_cast<char *>(decompressScratch.data()),
        sourceSize,(unsigned int)decompressScratch.size(),compressionType);
    if(decompressResult<0)
        return -1;
    *out=reinterpret_cast<const char *>(decompressScratch.data());
    return decompressResult;
}

int64_t CompressionProtocol::decompressedSizeZstandard(const char * const input, const uint32_t &intputSize)
{
    unsigned long long const dSize = ZSTD_getFrameContentSize(input, intputSize);
    //-1  = not a zstd frame at all (reject).
    //-2  = valid frame but no pledged content size in its header. The character
    //      block is one-shot ZSTD_compress()'d (size present), but the datapack
    //      file-download stream (packet 0x77) is STREAMING-compressed with no
    //      pledged size, so the caller must fall back to the worst-case buffer
    //      instead of rejecting the (perfectly valid) frame.
    if(dSize==ZSTD_CONTENTSIZE_ERROR)
        return -1;
    if(dSize==ZSTD_CONTENTSIZE_UNKNOWN)
        return -2;
    return (int64_t)dSize;
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
    #ifdef CATCHCHALLENGER_HARDENED
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
    #ifdef CATCHCHALLENGER_HARDENED
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
