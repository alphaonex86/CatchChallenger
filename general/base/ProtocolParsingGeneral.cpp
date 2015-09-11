#include "ProtocolParsing.h"
#include "GeneralVariable.h"
#include "GeneralStructures.h"
#include "ProtocolParsingCheck.h"

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
#include <lzma.h>
#include "lz4/lz4.h"
#endif

using namespace CatchChallenger;

#ifdef EPOLLCATCHCHALLENGERSERVER
char ProtocolParsingInputOutput::commonBuffer[CATCHCHALLENGER_COMMONBUFFERSIZE];
#endif
const uint16_t ProtocolParsingBase::sizeHeaderNulluint16_t=0xFFFF;
#ifdef CATCHCHALLENGER_BIGBUFFERSIZE
char ProtocolParsingBase::tempBigBufferForOutput[CATCHCHALLENGER_BIGBUFFERSIZE];
#endif
const char ProtocolParsingBase::packetFixedSize[256+128];

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
#ifndef CATCHCHALLENGERSERVERDROPIFCLENT
ProtocolParsing::CompressionType    ProtocolParsing::compressionTypeClient=CompressionType::None;
#endif
ProtocolParsing::CompressionType    ProtocolParsing::compressionTypeServer=CompressionType::None;
uint8_t ProtocolParsing::compressionLevel=6;
#endif

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
/* read/write buffer sizes */
#define IN_BUF_MAX  409600
#define OUT_BUF_MAX 409600

extern "C" void *lz_alloc(void *opaque, size_t nmemb, size_t size)
{
    Q_UNUSED(opaque);
    Q_UNUSED(nmemb);
    void *p = NULL;
    try{
        p = new char [size];
    }
    catch(std::bad_alloc &ba)
    {
        p = NULL;
    }
    return p;
}

extern "C" void lz_free(void *opaque, void *ptr)
{
    Q_UNUSED(opaque);
    delete [] (char*)ptr;
}


QByteArray ProtocolParsingBase::lzmaCompress(QByteArray data)
{
    QByteArray arr;
    lzma_check check = LZMA_CHECK_CRC64;
    lzma_stream strm = LZMA_STREAM_INIT; /* alloc and init lzma_stream struct */
    lzma_allocator al;
    al.alloc = lz_alloc;
    al.free = lz_free;
    strm.allocator = &al;
    uint8_t *in_buf;
    uint8_t out_buf [OUT_BUF_MAX];
    size_t in_len;  /* length of useful data in in_buf */
    size_t out_len; /* length of useful data in out_buf */
    lzma_ret ret_xz;

    /* initialize xz encoder */
    ret_xz = lzma_easy_encoder (&strm, ProtocolParsing::compressionLevel, check);
    if (ret_xz != LZMA_OK) {
        return QByteArray();
    }

    in_len = data.size();
    in_buf = (uint8_t*)data.data();
    strm.next_in = in_buf;
    strm.avail_in = in_len;

    do {
        strm.next_out = out_buf;
        strm.avail_out = OUT_BUF_MAX;
        ret_xz = lzma_code (&strm, LZMA_FINISH);

        if (ret_xz != LZMA_OK && ret_xz != LZMA_STREAM_END) {
            // Once everything has been decoded successfully, the
            // return value of lzma_code() will be LZMA_STREAM_END.

            // It's not LZMA_OK nor LZMA_STREAM_END,
            // so it must be an error code. See lzma/base.h
            // (src/liblzma/api/lzma/base.h in the source package
            // or e.g. /usr/include/lzma/base.h depending on the
            // install prefix) for the list and documentation of
            // possible values. Many values listen in lzma_ret
            // enumeration aren't possible in this example, but
            // can be made possible by enabling memory usage limit
            // or adding flags to the decoder initialization.
            const char *msg;
            switch (ret_xz) {
            case LZMA_MEM_ERROR:
                msg = "Memory allocation failed";
                break;

            case LZMA_FORMAT_ERROR:
                // .xz magic bytes weren't found.
                msg = "The input is not in the .xz format";
                break;

            case LZMA_OPTIONS_ERROR:
                // For example, the headers specify a filter
                // that isn't supported by this liblzma
                // version (or it hasn't been enabled when
                // building liblzma, but no-one sane does
                // that unless building liblzma for an
                // embedded system). Upgrading to a newer
                // liblzma might help.
                //
                // Note that it is unlikely that the file has
                // accidentally became corrupt if you get this
                // error. The integrity of the .xz headers is
                // always verified with a CRC32, so
                // unintentionally corrupt files can be
                // distinguished from unsupported files.
                msg = "Unsupported compression options";
                break;

            case LZMA_DATA_ERROR:
                msg = "Compressed file is corrupt";
                break;

            case LZMA_BUF_ERROR:
                // Typically this error means that a valid
                // file has got truncated, but it might also
                // be a damaged part in the file that makes
                // the decoder think the file is truncated.
                // If you prefer, you can use the same error
                // message for this as for LZMA_DATA_ERROR.
                msg = "Compressed file is truncated or "
                        "otherwise corrupt";
                break;

            default:
                // This is most likely LZMA_PROG_ERROR.
                msg = "Unknown error, possibly a bug";
                break;
            }

            fprintf(stderr, "Decoder error: "
                    "%s (error code %u)\n",
                    msg, ret_xz);
            return QByteArray();
        }

        out_len = OUT_BUF_MAX - strm.avail_out;
        arr.append((char*)out_buf, out_len);
        out_buf[0] = 0;
    } while (strm.avail_out == 0);
    lzma_end (&strm);
    return arr;
}

QByteArray ProtocolParsingBase::lzmaUncompress(QByteArray data)
{
    lzma_stream strm = LZMA_STREAM_INIT; /* alloc and init lzma_stream struct */
    const uint32_t flags = LZMA_TELL_UNSUPPORTED_CHECK;
    const uint64_t memory_limit = UINT64_MAX; /* no memory limit */
    uint8_t *in_buf;
    uint8_t out_buf [OUT_BUF_MAX];
    size_t in_len;  /* length of useful data in in_buf */
    size_t out_len; /* length of useful data in out_buf */
    lzma_ret ret_xz;
    QByteArray arr;

    ret_xz = lzma_stream_decoder (&strm, memory_limit, flags);
    if (ret_xz != LZMA_OK) {
        return QByteArray();
    }

    in_len = data.size();
    in_buf = (uint8_t*)data.data();

    strm.next_in = in_buf;
    strm.avail_in = in_len;
    do {
        strm.next_out = out_buf;
        strm.avail_out = OUT_BUF_MAX;
        ret_xz = lzma_code (&strm, LZMA_FINISH);

        if (ret_xz != LZMA_OK && ret_xz != LZMA_STREAM_END) {
            // Once everything has been decoded successfully, the
            // return value of lzma_code() will be LZMA_STREAM_END.

            // It's not LZMA_OK nor LZMA_STREAM_END,
            // so it must be an error code. See lzma/base.h
            // (src/liblzma/api/lzma/base.h in the source package
            // or e.g. /usr/include/lzma/base.h depending on the
            // install prefix) for the list and documentation of
            // possible values. Many values listen in lzma_ret
            // enumeration aren't possible in this example, but
            // can be made possible by enabling memory usage limit
            // or adding flags to the decoder initialization.
            const char *msg;
            switch (ret_xz) {
            case LZMA_MEM_ERROR:
                msg = "Memory allocation failed";
                break;

            case LZMA_FORMAT_ERROR:
                // .xz magic bytes weren't found.
                msg = "The input is not in the .xz format";
                break;

            case LZMA_OPTIONS_ERROR:
                // For example, the headers specify a filter
                // that isn't supported by this liblzma
                // version (or it hasn't been enabled when
                // building liblzma, but no-one sane does
                // that unless building liblzma for an
                // embedded system). Upgrading to a newer
                // liblzma might help.
                //
                // Note that it is unlikely that the file has
                // accidentally became corrupt if you get this
                // error. The integrity of the .xz headers is
                // always verified with a CRC32, so
                // unintentionally corrupt files can be
                // distinguished from unsupported files.
                msg = "Unsupported compression options";
                break;

            case LZMA_DATA_ERROR:
                msg = "Compressed file is corrupt";
                break;

            case LZMA_BUF_ERROR:
                // Typically this error means that a valid
                // file has got truncated, but it might also
                // be a damaged part in the file that makes
                // the decoder think the file is truncated.
                // If you prefer, you can use the same error
                // message for this as for LZMA_DATA_ERROR.
                msg = "Compressed file is truncated or "
                        "otherwise corrupt";
                break;

            default:
                // This is most likely LZMA_PROG_ERROR.
                msg = "Unknown error, possibly a bug";
                break;
            }

            fprintf(stderr, "Decoder error: "
                    "%s (error code %u)\n",
                    msg, ret_xz);
            return QByteArray();
        }

        out_len = OUT_BUF_MAX - strm.avail_out;
        /// \todo static buffer to prevent memory allocation and desallocation
        arr.append((char*)out_buf, out_len);
        out_buf[0] = 0;
    } while (strm.avail_out == 0);
    lzma_end (&strm);
    return arr;
}
#endif

ProtocolParsing::ProtocolParsing()
{
}

/// \note Nomination is: function, direction
void ProtocolParsing::initialiseTheVariable(const InitialiseTheVariableType &initialiseTheVariableType)
{
    switch(initialiseTheVariableType)
    {
        case InitialiseTheVariableType::MasterServer:
        case InitialiseTheVariableType::LoginServer:
        case InitialiseTheVariableType::AllInOne:
        default:
            if(!mainCodeWithoutSubCodeTypeClientToServer.empty())
                return;

            #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
            memset(ProtocolParsingBase::tempBigBufferForOutput,0,sizeof(ProtocolParsingBase::tempBigBufferForOutput));
            #endif
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            ProtocolParsing::compressionTypeServer=CompressionType::Zlib;
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            ProtocolParsing::compressionTypeClient=CompressionType::Zlib;
            #endif
            #endif

            char *writePacketFixedSize=const_cast<char *>(ProtocolParsingBase::packetFixedSize);
            memset(writePacketFixedSize,0,sizeof(writePacketFixedSize));

            //define the size of direct query
            {
                //default like is was more than 255 players
                writePacketFixedSize[0x61]=2;
            }
            writePacketFixedSize[0x62]=0;
            writePacketFixedSize[0xB8]=9;
            writePacketFixedSize[0x02]=2;
            writePacketFixedSize[0xA0]=5;
            writePacketFixedSize[0xBD]=CATCHCHALLENGER_FIRSTLOGINPASSHASHSIZE;
            writePacketFixedSize[0xA8]=CATCHCHALLENGER_FIRSTLOGINPASSHASHSIZE*2;
            writePacketFixedSize[0xA9]=CATCHCHALLENGER_FIRSTLOGINPASSHASHSIZE*2;
            writePacketFixedSize[0x11]=2;
            writePacketFixedSize[0xAB]=1+4;
            writePacketFixedSize[0xAC]=1+4+4;
            writePacketFixedSize[0x93]=CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER;
            writePacketFixedSize[0xBE]=1+4+4+4;
            writePacketFixedSize[0x84]=0;
            writePacketFixedSize[0x85]=2;
            writePacketFixedSize[0x86]=2;
            writePacketFixedSize[0x87]=2;
            writePacketFixedSize[0x88]=2*2+2*4;
            writePacketFixedSize[0x89]=2*2+2*4;
            writePacketFixedSize[0x8A]=2;
            writePacketFixedSize[0x8B]=2*2+2*4;
            writePacketFixedSize[0x8C]=2*2+2*4;
            writePacketFixedSize[0x8D]=0;
            writePacketFixedSize[0x90]=0;
            writePacketFixedSize[0xBF]=0;
            writePacketFixedSize[0xC0]=4;
            writePacketFixedSize[0xC1]=4;
            writePacketFixedSize[0xB0]=0;
            writePacketFixedSize[0xB1]=0;
            writePacketFixedSize[0x3E]=4;
            writePacketFixedSize[0x3F]=2;
            writePacketFixedSize[0x13]=2+4;
            writePacketFixedSize[0x15]=0;
            writePacketFixedSize[0x16]=0;
            writePacketFixedSize[0x18]=0;
            writePacketFixedSize[0x19]=1;
            writePacketFixedSize[0x1A]=0;
            writePacketFixedSize[0x07]=0;
            writePacketFixedSize[0x08]=1;
            writePacketFixedSize[0x09]=4+2;
            writePacketFixedSize[0x0B]=0;
            writePacketFixedSize[0x0C]=2;
            writePacketFixedSize[0x0D]=2;
            writePacketFixedSize[0x0E]=4;//monster id in db
            writePacketFixedSize[0x1B]=2;
            writePacketFixedSize[0x1C]=2;
            writePacketFixedSize[0x1D]=2;
            writePacketFixedSize[0x1E]=2;
            writePacketFixedSize[0x1F]=1;
            writePacketFixedSize[0x04]=1;
            writePacketFixedSize[0x5E]=0;
            writePacketFixedSize[0x59]=0;
            writePacketFixedSize[0x5A]=0;
            writePacketFixedSize[0x5B]=0;
            writePacketFixedSize[0x51]=0;
            writePacketFixedSize[0x4D]=4;
            writePacketFixedSize[0xF8]=4+4;
            writePacketFixedSize[0xE2]=2;

            //define the size of the reply
            /** \note previously send by: sizeMultipleCodePacketServerToClient */
            writePacketFixedSize[256+0xE1]=0;
            writePacketFixedSize[256+0xE2]=0;
            /** \note previously send by: sizeMultipleCodePacketClientToServer */
            writePacketFixedSize[256+0x83]=1;
            writePacketFixedSize[256+0x64]=1;
            writePacketFixedSize[256+0xE0]=1;
            writePacketFixedSize[256+0xBF]=50*4;
            writePacketFixedSize[256+0xC0]=50*4;
            writePacketFixedSize[256+0xC1]=50*4;
            writePacketFixedSize[256+0xB0]=50*4;
            writePacketFixedSize[256+0xB1]=5*4;

            //register meta type
            #ifndef EPOLLCATCHCHALLENGERSERVER
            qRegisterMetaType<CatchChallenger::PlayerMonster >("CatchChallenger::PlayerMonster");//for Api_protocol::tradeAddTradeMonster()
            qRegisterMetaType<PublicPlayerMonster >("PublicPlayerMonster");//for battleAcceptedByOther(stat,publicPlayerMonster);
            qRegisterMetaType<std::vector<uint8_t> >("std::vector<uint8_t>");//for battleAcceptedByOther(stat,publicPlayerMonster);
            qRegisterMetaType<std::vector<Skill::AttackReturn> >("std::vector<Skill::AttackReturn>");//for battleAcceptedByOther(stat,publicPlayerMonster);
            qRegisterMetaType<std::vector<MarketMonster> >("std::vector<MarketMonster>");
            qRegisterMetaType<std::vector<CharacterEntry> >("std::vector<CharacterEntry>");
            qRegisterMetaType<std::vector<uint8_t> >("std::vector<uint8_t>");//for battleAcceptedByOther(stat,publicPlayerMonster);
            qRegisterMetaType<std::vector<Skill::AttackReturn> >("std::vector<Skill::AttackReturn>");//for battleAcceptedByOther(stat,publicPlayerMonster);
            qRegisterMetaType<std::vector<MarketMonster> >("std::vector<MarketMonster>");
            qRegisterMetaType<std::vector<CharacterEntry> >("std::vector<CharacterEntry>");
            qRegisterMetaType<QSslSocket::SslMode>("QSslSocket::SslMode");
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
    // for data
    haveData(false),
    haveData_dataSize(0),
    is_reply(false),
    dataSize(0),
    //to parse the netwrok stream
    have_subCodeType(false),
    need_subCodeType(false),
    need_query_number(false),
    have_query_number(false),
    packetCode(0),
    subCodeType(0),
    queryNumber(0)
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    //if(!connect(socket,&ConnectedSocket::readyRead,this,&ProtocolParsingInputOutput::parseIncommingData,Qt::QueuedConnection/*to virtual socket*/))
    //    messageParsingLayer(std::string::number(isClient)+std::stringLiteral(" ProtocolParsingInputOutput::ProtocolParsingInputOutput(): can't connect the object"));
    #endif
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    isClient=(packetModeTransmission==PacketModeTransmission_Client);
    #endif
}

ProtocolParsingBase::~ProtocolParsingBase()
{
}

#ifndef EPOLLCATCHCHALLENGERSERVER
quint64 ProtocolParsingInputOutput::getRXSize() const
{
    return RXSize;
}

quint64 ProtocolParsingInputOutput::getTXSize() const
{
    return TXSize;
}
#endif

void ProtocolParsingBase::reset()
{
    waitedReply_packetCode.clear();
    waitedReply_subCodeType.clear();
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    replyOutputCompression.clear();
    #endif
    replyOutputSize.clear();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    queryReceived.clear();
    #endif

    dataClear();
}

ProtocolParsingInputOutput::ProtocolParsingInputOutput(
        #ifdef EPOLLCATCHCHALLENGERSERVER
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
    socket(socket)
    #endif
      #ifdef CATCHCHALLENGER_EXTRA_CHECK
      ,parseIncommingDataCount(0)
      #endif
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        isClient=(packetModeTransmission==PacketModeTransmission_Client);
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
}

ProtocolParsingInputOutput::~ProtocolParsingInputOutput()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    delete protocolParsingCheck;
    #endif
}

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
ProtocolParsing::CompressionType ProtocolParsingInputOutput::getCompressType() const
{
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
        return compressionTypeClient;
    else
    #endif
        return compressionTypeServer;
}
#endif
