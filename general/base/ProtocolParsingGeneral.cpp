#include "ProtocolParsing.h"
#include "GeneralVariable.h"
#include "GeneralStructures.h"
#include "ProtocolParsingCheck.h"

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
#include <lzma.h>
#include "lz4/lz4.h"
#endif

using namespace CatchChallenger;

char ProtocolParsingBase::tempBigBufferForUncompressedInput[];
char ProtocolParsingBase::tempBigBufferForOutput[];
char ProtocolParsingBase::tempBigBufferForCompressedOutput[];
const uint8_t ProtocolParsing::packetFixedSize[]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

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
            if(ProtocolParsingBase::tempBigBufferForOutput[0x02]==2)
                return;

            memset(ProtocolParsingBase::tempBigBufferForOutput,0,sizeof(ProtocolParsingBase::tempBigBufferForOutput));
            memset(ProtocolParsingBase::tempBigBufferForCompressedOutput,0,sizeof(ProtocolParsingBase::tempBigBufferForCompressedOutput));
            memset(ProtocolParsingBase::tempBigBufferForUncompressedInput,0,sizeof(ProtocolParsingBase::tempBigBufferForUncompressedInput));
            memset(ProtocolParsingBase::tempBigBufferForOutput,0,sizeof(ProtocolParsingBase::tempBigBufferForOutput));
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            ProtocolParsing::compressionTypeServer=CompressionType::Zlib;
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            ProtocolParsing::compressionTypeClient=CompressionType::Zlib;
            #endif
            #endif

            uint8_t *writePacketFixedSize=const_cast<uint8_t *>(ProtocolParsingBase::packetFixedSize);
            memset(writePacketFixedSize,0xFF,sizeof(ProtocolParsingBase::packetFixedSize));

            //Value: 0xFF not found (blocked), 0xFE not fixed size
            writePacketFixedSize[0x02]=2;
            writePacketFixedSize[0x03]=0xFE;
            writePacketFixedSize[0x04]=1;
            writePacketFixedSize[0x05]=0xFE;
            writePacketFixedSize[0x06]=0xFE;
            writePacketFixedSize[0x07]=0;
            writePacketFixedSize[0x08]=1;
            writePacketFixedSize[0x09]=4+2;
            writePacketFixedSize[0x0A]=0xFE;
            writePacketFixedSize[0x0B]=0;
            writePacketFixedSize[0x0C]=2;
            writePacketFixedSize[0x0D]=2;
            writePacketFixedSize[0x0E]=4;//monster id in db
            writePacketFixedSize[0x0F]=0xFE;
            writePacketFixedSize[0x10]=0xFE;
            writePacketFixedSize[0x11]=2;
            writePacketFixedSize[0x12]=0xFE;
            writePacketFixedSize[0x13]=2+4;
            writePacketFixedSize[0x14]=0xFE;
            writePacketFixedSize[0x15]=0;
            writePacketFixedSize[0x16]=0;
            writePacketFixedSize[0x17]=0xFE;
            writePacketFixedSize[0x18]=0;
            writePacketFixedSize[0x19]=1;
            writePacketFixedSize[0x1A]=0;
            writePacketFixedSize[0x1B]=2;
            writePacketFixedSize[0x1C]=2;
            writePacketFixedSize[0x1D]=2;
            writePacketFixedSize[0x1E]=2;
            writePacketFixedSize[0x1F]=1;
            writePacketFixedSize[0x3E]=4;
            writePacketFixedSize[0x3F]=2;

            writePacketFixedSize[0x40]=0xFE;
            writePacketFixedSize[0x44]=0xFE;
            writePacketFixedSize[0x45]=0xFE;
            writePacketFixedSize[0x46]=0xFE;
            writePacketFixedSize[0x47]=0xFE;
            writePacketFixedSize[0x4D]=4;
            writePacketFixedSize[0x50]=0xFE;
            writePacketFixedSize[0x51]=0;
            writePacketFixedSize[0x52]=0xFE;
            writePacketFixedSize[0x53]=0xFE;
            writePacketFixedSize[0x54]=0xFE;
            writePacketFixedSize[0x55]=0xFE;
            writePacketFixedSize[0x56]=0xFE;
            writePacketFixedSize[0x57]=0xFE;
            writePacketFixedSize[0x58]=0xFE;
            writePacketFixedSize[0x59]=0;
            writePacketFixedSize[0x5A]=0;
            writePacketFixedSize[0x5B]=0;
            writePacketFixedSize[0x5C]=0xFE;
            writePacketFixedSize[0x5D]=0xFE;
            writePacketFixedSize[0x5E]=0;
            writePacketFixedSize[0x5F]=0xFE;
            writePacketFixedSize[0x60]=0xFE;
            writePacketFixedSize[0x61]=2;//default like is was more than 255 players
            writePacketFixedSize[0x62]=0;
            writePacketFixedSize[0x63]=0xFE;
            writePacketFixedSize[0x64]=0xFE;
            writePacketFixedSize[0x65]=0xFE;
            writePacketFixedSize[0x66]=0xFE;
            writePacketFixedSize[0x67]=0;
            writePacketFixedSize[0x68]=0xFE;
            writePacketFixedSize[0x75]=4+4;
            writePacketFixedSize[0x76]=0xFE;
            writePacketFixedSize[0x77]=0xFE;
            writePacketFixedSize[0x7F]=0xFE;

            writePacketFixedSize[0x80]=0xFE;
            writePacketFixedSize[0x81]=0xFE;
            writePacketFixedSize[0x82]=0xFE;
            writePacketFixedSize[0x83]=0xFE;
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
            writePacketFixedSize[0x8E]=0xFE;
            writePacketFixedSize[0x8F]=0xFE;
            writePacketFixedSize[0x90]=0;
            writePacketFixedSize[0x91]=0xFE;
            writePacketFixedSize[0x92]=0xFE;
            writePacketFixedSize[0x93]=CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER;
            writePacketFixedSize[0xA0]=5;
            writePacketFixedSize[0xA1]=0xFE;
            writePacketFixedSize[0xA8]=CATCHCHALLENGER_FIRSTLOGINPASSHASHSIZE*2;
            writePacketFixedSize[0xA9]=CATCHCHALLENGER_FIRSTLOGINPASSHASHSIZE*2;
            writePacketFixedSize[0xAA]=0xFE;
            writePacketFixedSize[0xAB]=1+4;
            writePacketFixedSize[0xAC]=1+4+4;
            writePacketFixedSize[0xB0]=0;
            writePacketFixedSize[0xB1]=0;
            writePacketFixedSize[0xB2]=0xFE;
            writePacketFixedSize[0xB8]=9;
            writePacketFixedSize[0xBD]=CATCHCHALLENGER_FIRSTLOGINPASSHASHSIZE;
            writePacketFixedSize[0xBE]=1+4+4+4;
            writePacketFixedSize[0xBF]=0;
            writePacketFixedSize[0xC0]=4;
            writePacketFixedSize[0xC1]=4;

            writePacketFixedSize[0xE0]=0xFE;
            writePacketFixedSize[0xE1]=0xFE;
            writePacketFixedSize[0xE2]=2;
            writePacketFixedSize[0xF8]=4+4;

            //define the size of the reply
            writePacketFixedSize[256+0x80-128]=0xFE;
            writePacketFixedSize[256+0x81-128]=0xFE;
            writePacketFixedSize[256+0x82-128]=0xFE;
            writePacketFixedSize[256+0x83-128]=1;
            writePacketFixedSize[256+0x84-128]=0xFE;
            writePacketFixedSize[256+0x85-128]=0xFE;
            writePacketFixedSize[256+0x86-128]=0xFE;
            writePacketFixedSize[256+0x87-128]=0xFE;
            writePacketFixedSize[256+0x88-128]=0xFE;
            writePacketFixedSize[256+0x89-128]=0xFE;
            writePacketFixedSize[256+0x8A-128]=0xFE;
            writePacketFixedSize[256+0x8B-128]=0xFE;
            writePacketFixedSize[256+0x8C-128]=0xFE;
            writePacketFixedSize[256+0x8D-128]=0xFE;
            writePacketFixedSize[256+0x8E-128]=0xFE;
            writePacketFixedSize[256+0x8F-128]=0xFE;
            writePacketFixedSize[256+0x90-128]=0xFE;
            writePacketFixedSize[256+0x91-128]=0xFE;
            writePacketFixedSize[256+0x92-128]=0xFE;
            writePacketFixedSize[256+0x93-128]=0xFE;
            writePacketFixedSize[256+0xA0-128]=0xFE;
            writePacketFixedSize[256+0xA1-128]=0xFE;
            writePacketFixedSize[256+0xA8-128]=0xFE;
            writePacketFixedSize[256+0xA9-128]=0xFE;
            writePacketFixedSize[256+0xAA-128]=0xFE;
            writePacketFixedSize[256+0xAB-128]=0xFE;
            writePacketFixedSize[256+0xAC-128]=0xFE;
            writePacketFixedSize[256+0xB0-128]=50*4;
            writePacketFixedSize[256+0xB1-128]=5*4;
            writePacketFixedSize[256+0xB2-128]=0xFE;
            writePacketFixedSize[256+0xB8-128]=0xFE;
            writePacketFixedSize[256+0xBD-128]=0xFE;
            writePacketFixedSize[256+0xBE-128]=0xFE;
            writePacketFixedSize[256+0xBF-128]=50*4;
            writePacketFixedSize[256+0xC0-128]=50*4;
            writePacketFixedSize[256+0xC1-128]=50*4;

            writePacketFixedSize[256+0xE0-128]=1;
            writePacketFixedSize[256+0xE1-128]=0;
            writePacketFixedSize[256+0xE2-128]=0;
            writePacketFixedSize[256+0xF8-128]=0xFE;

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
    flags(0),
    // for data
    dataSize(0),
    //to parse the netwrok stream
    packetCode(0),
    queryNumber(0)
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    //if(!connect(socket,&ConnectedSocket::readyRead,this,&ProtocolParsingInputOutput::parseIncommingData,Qt::QueuedConnection/*to virtual socket*/))
    //    messageParsingLayer(std::string::number(isClient)+std::stringLiteral(" ProtocolParsingInputOutput::ProtocolParsingInputOutput(): can't connect the object"));
    #endif
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    flags |= (packetModeTransmission==PacketModeTransmission_Client);
    #endif
    memset(outputQueryNumberToPacketCode,0x00,sizeof(outputQueryNumberToPacketCode));
}

void ProtocolParsing::setMaxPlayers(const uint16_t &maxPlayers)
{
    uint8_t *writePacketFixedSize=const_cast<uint8_t *>(ProtocolParsingBase::packetFixedSize);

    if(maxPlayers<=255)
        writePacketFixedSize[0x61]=1;
    else
        //NO: this case do into initialiseTheVariable()
        //YES: reinitialise because the initialise already done, but this object can be reused
        writePacketFixedSize[0x61]=2;
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
    //outputQueryNumberToPacketCode.clear();
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
        flags |= (packetModeTransmission==PacketModeTransmission_Client);
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
    delete protocolParsingCheck;
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
