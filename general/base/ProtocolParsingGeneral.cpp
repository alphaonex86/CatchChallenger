#include "ProtocolParsing.h"
#include "GeneralVariable.h"
#include "GeneralStructures.h"
#include "ProtocolParsingCheck.h"
#include <zlib.h>
#include <iostream>
#include <cstring>
#include <openssl/sha.h>

#ifndef EPOLLCATCHCHALLENGERSERVER
#include <QMetaType>
#endif

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
#include <lzma.h>
#include "lz4/lz4.h"
#endif

using namespace CatchChallenger;

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION

uint8_t ProtocolParsing::compressionLevel=6;

extern "C" void *lz_alloc(void *, size_t , size_t size)
{
    void *p = NULL;
    try{
        p = new char [size];
    }
    catch(std::bad_alloc &)
    {
        p = NULL;
    }
    return p;
}

extern "C" void lz_free(void *, void *ptr)
{
    delete [] (char*)ptr;
}

static void logZlibError(int error)
{
    switch (error)
    {
    case Z_MEM_ERROR:
        std::cerr << "Out of memory while (de)compressing data!" << std::endl;
        break;
    case Z_VERSION_ERROR:
        std::cerr << "Incompatible zlib version!" << std::endl;
        break;
    case Z_NEED_DICT:
    case Z_DATA_ERROR:
        std::cerr << "Incorrect zlib compressed data!" << std::endl;
        break;
    default:
    {
        std::cerr << "Unknown error while (de)compressing data!" << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
    }
    }
}

int32_t ProtocolParsing::decompressZlib(const char * const input, const uint32_t &intputSize, char * const output, const uint32_t &maxOutputSize)
{
    z_stream strm;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = (Bytef *) input;
    strm.avail_in = intputSize;
    strm.next_out = (Bytef *) output;
    strm.avail_out = maxOutputSize;

    int ret = inflateInit2(&strm, 15 + 32);

    if (ret != Z_OK) {
    logZlibError(ret);
    return -1;
    }

    do {
        ret=inflate(&strm, Z_SYNC_FLUSH);

        switch(ret)
        {
            case Z_NEED_DICT:
            case Z_STREAM_ERROR:
            ret = Z_DATA_ERROR;
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
            inflateEnd(&strm);
            logZlibError(ret);
            return -1;
        }

        if (ret != Z_OK && ret != Z_STREAM_END)
        {
            unsigned char * const val=reinterpret_cast<unsigned char * const>(output);
            if(val>strm.next_out)
            {
                logZlibError(Z_STREAM_ERROR);
                return -1;
            }
            if(static_cast<uint32_t>(strm.next_out-val)>static_cast<uint32_t>(maxOutputSize))
            {
                logZlibError(Z_STREAM_ERROR);
                return -1;
            }
            logZlibError(ret);
            return -1;
        }
    }
    while(ret!=Z_STREAM_END);

    if(strm.avail_in!=0)
    {
        logZlibError(Z_DATA_ERROR);
        return -1;
    }
    inflateEnd(&strm);

    return maxOutputSize-strm.avail_out;
}

int32_t ProtocolParsing::compressZlib(const char * const input, const uint32_t &intputSize, char * const output, const uint32_t &maxOutputSize)
{
    z_stream strm;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = (Bytef *) input;
    strm.avail_in = intputSize;
    strm.total_in = intputSize;
    strm.next_out = (Bytef *) output;
    strm.avail_out = maxOutputSize;
    strm.total_out = maxOutputSize;

    /*int ret = deflateInit(&strm, ProtocolParsing::compressionLevel);
    if(ret != Z_OK)
    {
        logZlibError(ret);
        return -1;
    }

    do {
        ret = deflate(&strm, Z_SYNC_FLUSH);

        switch (ret) {
            case Z_NEED_DICT:
            case Z_STREAM_ERROR:
            ret = Z_DATA_ERROR;
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
            deflateEnd(&strm);
            logZlibError(ret);
            return -1;
        }

        if(ret != Z_OK && ret != Z_STREAM_END)
        {
            if((strm.next_out-reinterpret_cast<unsigned char * const>(output))>maxOutputSize)
            {
                logZlibError(Z_STREAM_ERROR);
                return -1;
            }
            logZlibError(Z_STREAM_ERROR);
            return -1;
        }
    }
    while (ret != Z_STREAM_END);
    deflateEnd(&strm);

    return maxOutputSize-strm.avail_out;*/

    int nErr,nRet=-1;
    nErr=deflateInit(&strm, ProtocolParsing::compressionLevel);
    if(nErr==Z_OK)
    {
        nErr=deflate(&strm,Z_FINISH);
        if(nErr==Z_STREAM_END)
            nRet=static_cast<uint32_t>(strm.total_out);
        else
        {
            logZlibError(nErr);
            return -1;
        }
    }
    else
    {
        logZlibError(nErr);
        return -1;
    }
    deflateEnd(&strm);    // zlib function
    return nRet;
}

int32_t ProtocolParsing::decompressXz(const char * const input, const uint32_t &intputSize, char * const output, const uint32_t &maxOutputSize)
{
    lzma_stream strm = LZMA_STREAM_INIT; /* alloc and init lzma_stream struct */
    const uint32_t flags = LZMA_TELL_UNSUPPORTED_CHECK;
    const uint64_t memory_limit = 64*1024*1024; /* no memory limit */
    lzma_ret ret_xz;

    ret_xz = lzma_stream_decoder (&strm, memory_limit, flags);
    if (ret_xz != LZMA_OK) {
        return -1;
    }

    strm.next_in = (Bytef *) input;
    strm.avail_in = intputSize;
    strm.next_out = (Bytef *) output;
    strm.avail_out = maxOutputSize;
    do {
        ret_xz = lzma_code (&strm, LZMA_FINISH);

        if (ret_xz != LZMA_OK && ret_xz != LZMA_STREAM_END) {
            const char *msg;
            switch (ret_xz) {
            case LZMA_MEM_ERROR:
                msg = "Memory allocation failed";
                break;

            case LZMA_FORMAT_ERROR:
                msg = "The input is not in the .xz format";
                break;

            case LZMA_OPTIONS_ERROR:
                msg = "Unsupported compression options";
                break;

            case LZMA_DATA_ERROR:
                msg = "Compressed file is corrupt";
                break;

            case LZMA_BUF_ERROR:
                msg = "Compressed file is truncated or "
                        "otherwise corrupt";
                break;

            default:
                msg = "Unknown error, possibly a bug";
                break;
            }

            fprintf(stderr, "Decoder error: "
                    "%s (error code %u)\n",
                    msg, ret_xz);
            return -1;
        }
    } while (strm.avail_out == 0);
    lzma_end (&strm);
    return maxOutputSize-static_cast<uint32_t>(strm.avail_out);
}

int32_t ProtocolParsing::compressXz(const char * const input, const uint32_t &intputSize, char * const output, const uint32_t &maxOutputSize)
{
    std::vector<char> arr;
    lzma_check check = LZMA_CHECK_CRC64;
    lzma_stream strm = LZMA_STREAM_INIT; /* alloc and init lzma_stream struct */
    lzma_allocator al;
    al.alloc = lz_alloc;
    al.free = lz_free;
    strm.allocator = &al;
    strm.next_in = (Bytef *) input;
    strm.avail_in = intputSize;
    strm.next_out = (Bytef *) output;
    strm.avail_out = maxOutputSize;
    lzma_ret ret_xz;

    /* initialize xz encoder */
    ret_xz = lzma_easy_encoder (&strm, ProtocolParsing::compressionLevel, check);
    if (ret_xz != LZMA_OK) {
        return -1;
    }

    do {
        ret_xz = lzma_code (&strm, LZMA_FINISH);

        if (ret_xz != LZMA_OK && ret_xz != LZMA_STREAM_END) {
            const char *msg;
            switch (ret_xz) {
            case LZMA_MEM_ERROR:
                msg = "Memory allocation failed";
                break;

            case LZMA_FORMAT_ERROR:
                msg = "The input is not in the .xz format";
                break;

            case LZMA_OPTIONS_ERROR:
                msg = "Unsupported compression options";
                break;

            case LZMA_DATA_ERROR:
                msg = "Compressed file is corrupt";
                break;

            case LZMA_BUF_ERROR:
                msg = "Compressed file is truncated or "
                        "otherwise corrupt";
                break;
            default:
                msg = "Unknown error, possibly a bug";
                break;
            }

            fprintf(stderr, "Decoder error: "
                    "%s (error code %u)\n",
                    msg, ret_xz);
            return -1;
        }
    } while (strm.avail_out == 0);
    lzma_end (&strm);
    return maxOutputSize-static_cast<uint32_t>(strm.avail_out);
}

#endif

#if ! defined (ONLYMAPRENDER)
char ProtocolParsingBase::tempBigBufferForOutput[];
char ProtocolParsingBase::tempBigBufferForInput[];//to store the input buffer on linux READ() interface or with Qt
#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
char ProtocolParsingBase::tempBigBufferForUncompressedInput[];
char ProtocolParsingBase::tempBigBufferForCompressedOutput[];
#endif
uint8_t ProtocolParsing::packetFixedSize[];

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
#ifndef CATCHCHALLENGERSERVERDROPIFCLENT
ProtocolParsing::CompressionType    ProtocolParsing::compressionTypeClient=CompressionType::None;
#endif
ProtocolParsing::CompressionType    ProtocolParsing::compressionTypeServer=CompressionType::None;
#endif

ProtocolParsing::ProtocolParsing()
{
}

/// \note Nomination is: function, direction
void ProtocolParsing::initialiseTheVariable(const InitialiseTheVariableType &initialiseTheVariableType)
{
    //test the sha224 lib
    {
        static const unsigned char ibuf[]={0xBA,0xD9,0x39,0xE0,0x62,0x4C,0x48,0x1B,0x6B,0x60,0x49,0x63,0x18,0x77,0x01,0xBA,0x0A,0x37,0x2C,0x15,0x4D,0xA4,0x0C,0x1D,0x82,0x8A,0xE8,0xF2};
        static const unsigned char requiredResult[]={0x1c,0x82,0x16,0x18,0xa8,0xaa,0xd1,0x00,0xf7,0x41,0xba,0xfc,0x84,0x0f,0xcd,0x61,0x3a,0x9d,0xee,0x51,0x84,0xe0,0x5e,0xfd,0x45,0x8c,0x8f,0x9d};
        unsigned char obuf[sizeof(requiredResult)];
        SHA224(ibuf,sizeof(ibuf),obuf);
        if(memcmp(requiredResult,obuf,sizeof(requiredResult))!=0)
        {
            std::cerr << "Sha224 lib don't return the correct result" << std::endl;
            abort();
        }
    }

    switch(initialiseTheVariableType)
    {
        case InitialiseTheVariableType::MasterServer:
        case InitialiseTheVariableType::LoginServer:
        case InitialiseTheVariableType::AllInOne:
        default:
            if(ProtocolParsingBase::packetFixedSize[0x02]==2)
                return;

            memset(ProtocolParsingBase::tempBigBufferForOutput,0,sizeof(ProtocolParsingBase::tempBigBufferForOutput));
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            memset(ProtocolParsingBase::tempBigBufferForCompressedOutput,0,sizeof(ProtocolParsingBase::tempBigBufferForCompressedOutput));
            memset(ProtocolParsingBase::tempBigBufferForUncompressedInput,0,sizeof(ProtocolParsingBase::tempBigBufferForUncompressedInput));
            ProtocolParsing::compressionTypeServer=CompressionType::Zlib;
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            ProtocolParsing::compressionTypeClient=CompressionType::Zlib;
            #endif
            #endif

            memset(ProtocolParsingBase::packetFixedSize,0xFF,sizeof(ProtocolParsingBase::packetFixedSize));

            //Value: 0xFF not found (blocked), 0xFE not fixed size
            ProtocolParsingBase::packetFixedSize[0x02]=2;
            ProtocolParsingBase::packetFixedSize[0x03]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x04]=1;
            ProtocolParsingBase::packetFixedSize[0x05]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x06]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x07]=0;
            ProtocolParsingBase::packetFixedSize[0x08]=1;
            ProtocolParsingBase::packetFixedSize[0x09]=1+2;
            ProtocolParsingBase::packetFixedSize[0x0A]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x0B]=0;
            ProtocolParsingBase::packetFixedSize[0x0C]=2;
            ProtocolParsingBase::packetFixedSize[0x0D]=2;
            ProtocolParsingBase::packetFixedSize[0x0E]=1;//monster position
            ProtocolParsingBase::packetFixedSize[0x0F]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x10]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x11]=2;
            ProtocolParsingBase::packetFixedSize[0x12]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x13]=2+4;
            ProtocolParsingBase::packetFixedSize[0x14]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x15]=0;
            ProtocolParsingBase::packetFixedSize[0x16]=0;
            ProtocolParsingBase::packetFixedSize[0x17]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x18]=0;
            ProtocolParsingBase::packetFixedSize[0x19]=1;
            ProtocolParsingBase::packetFixedSize[0x1A]=0;
            ProtocolParsingBase::packetFixedSize[0x1B]=2;
            ProtocolParsingBase::packetFixedSize[0x1C]=2;
            ProtocolParsingBase::packetFixedSize[0x1D]=2;
            ProtocolParsingBase::packetFixedSize[0x1E]=2;
            ProtocolParsingBase::packetFixedSize[0x1F]=1;
            ProtocolParsingBase::packetFixedSize[0x3E]=4;
            ProtocolParsingBase::packetFixedSize[0x3F]=2;

            ProtocolParsingBase::packetFixedSize[0x40]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x44]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x45]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x46]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x47]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x48]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x4D]=4;
            ProtocolParsingBase::packetFixedSize[0x50]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x51]=0;
            ProtocolParsingBase::packetFixedSize[0x52]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x53]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x54]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x55]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x56]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x57]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x58]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x59]=0;
            ProtocolParsingBase::packetFixedSize[0x5A]=0;
            ProtocolParsingBase::packetFixedSize[0x5B]=0;

            ProtocolParsingBase::packetFixedSize[0x5C]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x5D]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x5E]=0;
            ProtocolParsingBase::packetFixedSize[0x5F]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x60]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x61]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x62]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x63]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x64]=2;//default like is was more than 255 players
            ProtocolParsingBase::packetFixedSize[0x65]=0;
            ProtocolParsingBase::packetFixedSize[0x66]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x67]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x68]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x69]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x6A]=0;
            ProtocolParsingBase::packetFixedSize[0x6B]=0xFE;

            ProtocolParsingBase::packetFixedSize[0x75]=4+4;
            ProtocolParsingBase::packetFixedSize[0x76]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x77]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x78]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x7F]=0xFE;

            ProtocolParsingBase::packetFixedSize[0x80]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x81]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x82]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x83]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x84]=0;
            ProtocolParsingBase::packetFixedSize[0x85]=2;
            ProtocolParsingBase::packetFixedSize[0x86]=2;
            ProtocolParsingBase::packetFixedSize[0x87]=2;
            ProtocolParsingBase::packetFixedSize[0x88]=2*2+2*4;
            ProtocolParsingBase::packetFixedSize[0x89]=2*2+2*4;
            ProtocolParsingBase::packetFixedSize[0x8A]=2;
            ProtocolParsingBase::packetFixedSize[0x8B]=2*2+2*4;
            ProtocolParsingBase::packetFixedSize[0x8C]=2*2+2*4;
            ProtocolParsingBase::packetFixedSize[0x8D]=0;
            ProtocolParsingBase::packetFixedSize[0x8E]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x8F]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x90]=0;
            ProtocolParsingBase::packetFixedSize[0x91]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x92]=0xFE;
            ProtocolParsingBase::packetFixedSize[0x93]=CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER;
            ProtocolParsingBase::packetFixedSize[0xA0]=5;
            ProtocolParsingBase::packetFixedSize[0xA1]=0xFE;
            ProtocolParsingBase::packetFixedSize[0xA8]=CATCHCHALLENGER_SHA224HASH_SIZE+CATCHCHALLENGER_SHA224HASH_SIZE;
            ProtocolParsingBase::packetFixedSize[0xA9]=CATCHCHALLENGER_SHA224HASH_SIZE+CATCHCHALLENGER_SHA224HASH_SIZE;
            ProtocolParsingBase::packetFixedSize[0xAA]=0xFE;
            ProtocolParsingBase::packetFixedSize[0xAB]=1+4;
            ProtocolParsingBase::packetFixedSize[0xAC]=1+4+4;
            ProtocolParsingBase::packetFixedSize[0xAD]=CATCHCHALLENGER_SHA224HASH_SIZE;
            ProtocolParsingBase::packetFixedSize[0xB0]=0;
            ProtocolParsingBase::packetFixedSize[0xB1]=0;
            ProtocolParsingBase::packetFixedSize[0xB2]=0xFE;
            ProtocolParsingBase::packetFixedSize[0xB8]=9;
            ProtocolParsingBase::packetFixedSize[0xBD]=CATCHCHALLENGER_SHA224HASH_SIZE;
            ProtocolParsingBase::packetFixedSize[0xBE]=1+4+4+4;
            ProtocolParsingBase::packetFixedSize[0xBF]=0;
            ProtocolParsingBase::packetFixedSize[0xC0]=1;
            ProtocolParsingBase::packetFixedSize[0xC1]=1;

            ProtocolParsingBase::packetFixedSize[0xDF]=0xFE;
            ProtocolParsingBase::packetFixedSize[0xE0]=0xFE;
            ProtocolParsingBase::packetFixedSize[0xE1]=0xFE;
            ProtocolParsingBase::packetFixedSize[0xE2]=2;
            ProtocolParsingBase::packetFixedSize[0xF8]=4+4;
            ProtocolParsingBase::packetFixedSize[0xF9]=0;

            //define the size of the reply
            ProtocolParsingBase::packetFixedSize[128+0x80]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0x81]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0x82]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0x83]=1;
            ProtocolParsingBase::packetFixedSize[128+0x84]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0x85]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0x86]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0x87]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0x88]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0x89]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0x8A]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0x8B]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0x8C]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0x8D]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0x8E]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0x8F]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0x90]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0x91]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0x92]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0x93]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0xA0]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0xA1]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0xA8]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0xA9]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0xAA]=1+4;/*drop dynamic size to improve the packet size overhead*/
            ProtocolParsingBase::packetFixedSize[128+0xAB]=1;
            ProtocolParsingBase::packetFixedSize[128+0xAC]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0xAD]=0x01;
            ProtocolParsingBase::packetFixedSize[128+0xB0]=CATCHCHALLENGER_SERVER_MAXIDBLOCK*4;
            ProtocolParsingBase::packetFixedSize[128+0xB1]=5*4;
            ProtocolParsingBase::packetFixedSize[128+0xB2]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0xB8]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0xBD]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0xBE]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0xBF]=CATCHCHALLENGER_SERVER_MAXIDBLOCK*4;
            ProtocolParsingBase::packetFixedSize[128+0xC0]=CATCHCHALLENGER_SERVER_MAXIDBLOCK*4;
            ProtocolParsingBase::packetFixedSize[128+0xC1]=CATCHCHALLENGER_SERVER_MAXIDBLOCK*4;

            ProtocolParsingBase::packetFixedSize[128+0xDF]=1;
            ProtocolParsingBase::packetFixedSize[128+0xE0]=1;
            ProtocolParsingBase::packetFixedSize[128+0xE1]=0;
            ProtocolParsingBase::packetFixedSize[128+0xE2]=0;
            ProtocolParsingBase::packetFixedSize[128+0xF8]=0xFE;
            ProtocolParsingBase::packetFixedSize[128+0xF9]=0;

            //register meta type
            #ifndef EPOLLCATCHCHALLENGERSERVER
            qRegisterMetaType<CatchChallenger::PlayerMonster >("CatchChallenger::PlayerMonster");//for Api_protocol::tradeAddTradeMonster()
            qRegisterMetaType<PublicPlayerMonster >("PublicPlayerMonster");//for battleAcceptedByOther(stat,publicPlayerMonster);
            qRegisterMetaType<std::vector<uint8_t> >("std::vector<uint8_t>");//for battleAcceptedByOther(stat,publicPlayerMonster);
            qRegisterMetaType<std::vector<Skill::AttackReturn> >("std::vector<Skill::AttackReturn>");//for battleAcceptedByOther(stat,publicPlayerMonster);
            qRegisterMetaType<std::vector<CharacterEntry> >("std::vector<CharacterEntry>");
            qRegisterMetaType<std::vector<uint8_t> >("std::vector<uint8_t>");//for battleAcceptedByOther(stat,publicPlayerMonster);
            qRegisterMetaType<std::vector<Skill::AttackReturn> >("std::vector<Skill::AttackReturn>");//for battleAcceptedByOther(stat,publicPlayerMonster);
            qRegisterMetaType<std::vector<CharacterEntry> >("std::vector<CharacterEntry>");
            #if ! defined (ONLYMAPRENDER)
            qRegisterMetaType<QSslSocket::SslMode>("QSslSocket::SslMode");
            #endif
            #endif
        break;
    }
}

ProtocolParsingBase::ProtocolParsingBase(
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        PacketModeTransmission packetModeTransmission
        #endif
        ) :
    #ifdef EPOLLCATCHCHALLENGERSERVER
        #ifdef SERVERSSL
            ProtocolParsing(),
        #else
            ProtocolParsing(),
        #endif
    #else
    ProtocolParsing(),
    #endif
    flags(0),
    // for data
    dataSize(0),
    //to parse the netwrok stream
    packetCode(0),
    queryNumber(0)
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    //if(!connect(socket,&ConnectedSocket::readyRead,this,&ProtocolParsingInputOutput::parseIncommingData,Qt::QueuedConnection/*to virtual socket*/))
    //    messageParsingLayer(std::to_string(isClient)+std::stringLiteral(" ProtocolParsingInputOutput::ProtocolParsingInputOutput(): can't connect the object"));
    #endif
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(packetModeTransmission==PacketModeTransmission_Client)
        flags |= 0x10;
    #endif
    memset(outputQueryNumberToPacketCode,0x00,sizeof(outputQueryNumberToPacketCode));
}

void ProtocolParsing::setMaxPlayers(const uint16_t &maxPlayers)
{
    if(maxPlayers<=255)
        ProtocolParsingBase::packetFixedSize[0x64]=1;
    else
        //NO: this case do into initialiseTheVariable()
        //YES: reinitialise because the initialise already done, but this object can be reused
        ProtocolParsingBase::packetFixedSize[0x64]=2;
}

ProtocolParsingBase::~ProtocolParsingBase()
{
}

#ifndef EPOLLCATCHCHALLENGERSERVER
uint64_t ProtocolParsingInputOutput::getRXSize() const
{
    return RXSize;
}

uint64_t ProtocolParsingInputOutput::getTXSize() const
{
    return TXSize;
}
#endif

void ProtocolParsingBase::reset()
{
    //outputQueryNumberToPacketCode.clear();

    dataClear();
}

void ProtocolParsingBase::resetForReconnect()
{
    flags &= 0x18;
    // ProtocolParsingBase
    // for data
    dataSize=0;
    //to parse the netwrok stream
    packetCode=0;
    queryNumber=0;
    memset(outputQueryNumberToPacketCode,0x00,sizeof(outputQueryNumberToPacketCode));
}

void ProtocolParsingInputOutput::resetForReconnect()
{
    ProtocolParsingBase::resetForReconnect();

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    parseIncommingDataCount=0;
    #endif
    //ProtocolParsingInputOutput
    #ifndef EPOLLCATCHCHALLENGERSERVER
    RXSize=0;
    TXSize=0;
    #endif
    memset(inputQueryNumberToPacketCode,0x00,sizeof(inputQueryNumberToPacketCode));
}

ProtocolParsingInputOutput::ProtocolParsingInputOutput(
        #if defined(EPOLLCATCHCHALLENGERSERVER) || defined (ONLYMAPRENDER)
            #ifdef SERVERSSL
                const int &infd, SSL_CTX *ctx
            #else
                const int &infd
            #endif
        #else
        ConnectedSocket *socket
        #endif
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        ,const PacketModeTransmission &packetModeTransmission
        #endif
        ) :
    ProtocolParsingBase(
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        packetModeTransmission
        #endif
        ),
    #ifdef EPOLLCATCHCHALLENGERSERVER
        #ifdef SERVERSSL
            epollSocket(infd,ctx)
        #else
            epollSocket(infd)
        #endif
    #else
        #if ! defined (ONLYMAPRENDER)
        socket(socket)
        #endif
    #endif
      #ifdef CATCHCHALLENGER_EXTRA_CHECK
      ,parseIncommingDataCount(0)
      #endif
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        if(packetModeTransmission==PacketModeTransmission_Client)
            flags |= 0x10;
        if(packetModeTransmission==PacketModeTransmission_Client)
            protocolParsingCheck=new ProtocolParsingCheck(PacketModeTransmission_Server);
        else
            protocolParsingCheck=new ProtocolParsingCheck(PacketModeTransmission_Client);
    #else
        #error "Can't have both CATCHCHALLENGERSERVERDROPIFCLENT and CATCHCHALLENGER_EXTRA_CHECK enabled because ProtocolParsingCheck work as client"
    #endif
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    RXSize=0;
    TXSize=0;
    #endif
    memset(inputQueryNumberToPacketCode,0x00,sizeof(inputQueryNumberToPacketCode));
}

ProtocolParsingInputOutput::~ProtocolParsingInputOutput()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(protocolParsingCheck!=NULL)
    {
        delete protocolParsingCheck;
        protocolParsingCheck=NULL;
    }
    #endif
}

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
ProtocolParsing::CompressionType ProtocolParsingInputOutput::getCompressType() const
{
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(flags & 0x10)
        return compressionTypeClient;
    else
    #endif
        return compressionTypeServer;
}
#endif
#endif
