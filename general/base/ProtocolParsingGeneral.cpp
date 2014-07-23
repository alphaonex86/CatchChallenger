#include "ProtocolParsing.h"
#include "DebugClass.h"
#include "GeneralVariable.h"

#include <lzma.h>

using namespace CatchChallenger;

#ifdef EPOLLCATCHCHALLENGERSERVER
char ProtocolParsingInputOutput::commonBuffer[CATCHCHALLENGER_COMMONBUFFERSIZE];
#endif
const quint16 ProtocolParsingInputOutput::sizeHeaderNullquint16=0;
#ifdef CATCHCHALLENGER_BIGBUFFERSIZE
char ProtocolParsingInputOutput::tempBigBufferForOutput[CATCHCHALLENGER_BIGBUFFERSIZE];
#endif

QSet<quint8>                        ProtocolParsing::mainCodeWithoutSubCodeTypeServerToClient;//if need sub code or not
//if is a query
QSet<quint8>                        ProtocolParsing::mainCode_IsQueryClientToServer;
quint8                              ProtocolParsing::replyCodeClientToServer;
ProtocolParsing::CompressionType    ProtocolParsing::compressionType=CompressionType_None;
//predefined size
QHash<quint8,quint16>                   ProtocolParsing::sizeOnlyMainCodePacketClientToServer;
QHash<quint8,QHash<quint16,quint16> >	ProtocolParsing::sizeMultipleCodePacketClientToServer;
QHash<quint8,quint16>                   ProtocolParsing::replySizeOnlyMainCodePacketClientToServer;
QHash<quint8,QHash<quint16,quint16> >	ProtocolParsing::replySizeMultipleCodePacketClientToServer;

QSet<quint8>                            ProtocolParsing::mainCodeWithoutSubCodeTypeClientToServer;//if need sub code or not
//if is a query
QSet<quint8>                            ProtocolParsing::mainCode_IsQueryServerToClient;
quint8                                  ProtocolParsing::replyCodeServerToClient;
//predefined size
QHash<quint8,quint16>                   ProtocolParsing::sizeOnlyMainCodePacketServerToClient;
QHash<quint8,QHash<quint16,quint16> >	ProtocolParsing::sizeMultipleCodePacketServerToClient;
QHash<quint8,quint16>                   ProtocolParsing::replySizeOnlyMainCodePacketServerToClient;
QHash<quint8,QHash<quint16,quint16> >	ProtocolParsing::replySizeMultipleCodePacketServerToClient;

QHash<quint8,QSet<quint16> >    ProtocolParsing::compressionMultipleCodePacketClientToServer;
QHash<quint8,QSet<quint16> >    ProtocolParsing::compressionMultipleCodePacketServerToClient;
QHash<quint8,QSet<quint16> >    ProtocolParsing::replyComressionMultipleCodePacketClientToServer;
QHash<quint8,QSet<quint16> >    ProtocolParsing::replyComressionMultipleCodePacketServerToClient;
QSet<quint8>                    ProtocolParsing::replyComressionOnlyMainCodePacketClientToServer;
QSet<quint8>                    ProtocolParsing::replyComressionOnlyMainCodePacketServerToClient;

/* read/write buffer sizes */
#define IN_BUF_MAX  409600
#define OUT_BUF_MAX 409600
/* analogous to xz CLI options: -0 to -9 */
#define COMPRESSION_LEVEL 9

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


QByteArray ProtocolParsingInputOutput::lzmaCompress(QByteArray data)
{
    QByteArray arr;
    lzma_check check = LZMA_CHECK_CRC64;
    lzma_stream strm = LZMA_STREAM_INIT; /* alloc and init lzma_stream struct */
    lzma_allocator al;
    al.alloc = lz_alloc;
    al.free = lz_free;
    strm.allocator = &al;
    quint8 *in_buf;
    quint8 out_buf [OUT_BUF_MAX];
    size_t in_len;  /* length of useful data in in_buf */
    size_t out_len; /* length of useful data in out_buf */
    lzma_ret ret_xz;

    /* initialize xz encoder */
    ret_xz = lzma_easy_encoder (&strm, 9, check);
    if (ret_xz != LZMA_OK) {
        return QByteArray();
    }

    in_len = data.size();
    in_buf = (quint8*)data.data();
    strm.next_in = in_buf;
    strm.avail_in = in_len;

    do {
        strm.next_out = out_buf;
        strm.avail_out = OUT_BUF_MAX;
        ret_xz = lzma_code (&strm, LZMA_FINISH);

        out_len = OUT_BUF_MAX - strm.avail_out;
        arr.append((char*)out_buf, out_len);
        out_buf[0] = 0;
    } while (strm.avail_out == 0);
    lzma_end (&strm);
    return arr;
}

QByteArray ProtocolParsingInputOutput::lzmaUncompress(QByteArray data)
{
    lzma_stream strm = LZMA_STREAM_INIT; /* alloc and init lzma_stream struct */
    const uint32_t flags = LZMA_TELL_UNSUPPORTED_CHECK | LZMA_CONCATENATED;
    const uint64_t memory_limit = UINT64_MAX; /* no memory limit */
    quint8 *in_buf;
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
    in_buf = (quint8*)data.data();

    strm.next_in = in_buf;
    strm.avail_in = in_len;
    do {
        strm.next_out = out_buf;
        strm.avail_out = OUT_BUF_MAX;
        ret_xz = lzma_code (&strm, LZMA_FINISH);

        out_len = OUT_BUF_MAX - strm.avail_out;
        /// \todo static buffer to prevent memory allocation and desallocation
        arr.append((char*)out_buf, out_len);
        out_buf[0] = 0;
    } while (strm.avail_out == 0);
    lzma_end (&strm);
    return arr;
}

ProtocolParsing::ProtocolParsing(
        #ifdef EPOLLCATCHCHALLENGERSERVER
            #ifndef SERVERNOSSL
                const int &infd, SSL_CTX *ctx
            #else
                const int &infd
            #endif
        #else
        ConnectedSocket *socket
        #endif
        ) :
    #ifdef EPOLLCATCHCHALLENGERSERVER
        #ifndef SERVERNOSSL
            epollSslClient(infd,*ctx)
        #else
            epollSocket(infd)
        #endif
    #else
    socket(socket)
    #endif
{
}

/// \note Nomination is: function, direction
void ProtocolParsing::initialiseTheVariable()
{
    if(!mainCodeWithoutSubCodeTypeClientToServer.isEmpty())
        return;

    compressionType=CompressionType_Zlib;

    //def query without the sub code
    mainCodeWithoutSubCodeTypeServerToClient << 0xC0 << 0xC1 << 0xC3 << 0xC4 << 0xC5 << 0xC6 << 0xC7 << 0xC8 << 0xCA << 0xD1 << 0xD2;
    mainCodeWithoutSubCodeTypeClientToServer << 0x40 << 0x43 << 0x41 << 0x61 << 0x03 << 0x04 << 0x05;

    //define the size of direct query
    {
        //default like is was more than 255 players
        sizeOnlyMainCodePacketServerToClient[0xC3]=2;
    }
    sizeOnlyMainCodePacketServerToClient[0xC4]=0;
    sizeOnlyMainCodePacketClientToServer[0x40]=2;
    sizeOnlyMainCodePacketClientToServer[0x03]=5;
    sizeOnlyMainCodePacketClientToServer[0x04]=CATCHCHALLENGER_FIRSTLOGINPASSHASHSIZE*2;
    sizeOnlyMainCodePacketClientToServer[0x05]=CATCHCHALLENGER_FIRSTLOGINPASSHASHSIZE*2;
    sizeOnlyMainCodePacketClientToServer[0x61]=4;
    sizeMultipleCodePacketClientToServer[0x02][0x0004]=4;
    sizeMultipleCodePacketClientToServer[0x02][0x0005]=4;
    sizeMultipleCodePacketClientToServer[0x10][0x0006]=1;
    sizeMultipleCodePacketClientToServer[0x10][0x0007]=0;
    sizeMultipleCodePacketClientToServer[0x10][0x0008]=4;
    sizeMultipleCodePacketClientToServer[0x10][0x0009]=4;
    sizeMultipleCodePacketClientToServer[0x10][0x000A]=4;
    sizeMultipleCodePacketClientToServer[0x10][0x000B]=4*4;
    sizeMultipleCodePacketClientToServer[0x10][0x000C]=4*4;
    sizeMultipleCodePacketClientToServer[0x10][0x000D]=4;
    sizeMultipleCodePacketClientToServer[0x10][0x000E]=4*4;
    sizeMultipleCodePacketClientToServer[0x10][0x000F]=4*4;
    sizeMultipleCodePacketClientToServer[0x10][0x0010]=0;
    sizeMultipleCodePacketClientToServer[0x10][0x0013]=0;
    sizeMultipleCodePacketClientToServer[0x50][0x0002]=8;
    sizeMultipleCodePacketClientToServer[0x50][0x0004]=0;
    sizeMultipleCodePacketClientToServer[0x50][0x0005]=0;
    sizeMultipleCodePacketClientToServer[0x60][0x0002]=0;
    sizeMultipleCodePacketClientToServer[0x60][0x0003]=1;
    sizeMultipleCodePacketClientToServer[0x60][0x0004]=8;
    sizeMultipleCodePacketClientToServer[0x60][0x0006]=0;
    sizeMultipleCodePacketClientToServer[0x60][0x0007]=4;
    sizeMultipleCodePacketClientToServer[0x60][0x0008]=2;
    sizeMultipleCodePacketClientToServer[0x60][0x0009]=4;
    sizeMultipleCodePacketClientToServer[0x6a][0x0001]=2;
    sizeMultipleCodePacketClientToServer[0x6a][0x0002]=2;
    sizeMultipleCodePacketClientToServer[0x6a][0x0003]=2;
    sizeMultipleCodePacketClientToServer[0x6a][0x0004]=2;
    sizeMultipleCodePacketClientToServer[0x6a][0x0005]=1;
    sizeMultipleCodePacketClientToServer[0x42][0x0004]=1;

    sizeMultipleCodePacketServerToClient[0xC2][0x0009]=0;
    sizeMultipleCodePacketServerToClient[0xD0][0x0006]=0;
    sizeMultipleCodePacketServerToClient[0xD0][0x0007]=0;
    sizeMultipleCodePacketServerToClient[0xD0][0x0008]=0;
    sizeMultipleCodePacketServerToClient[0xE0][0x0007]=0;
    sizeMultipleCodePacketServerToClient[0x79][0x0002]=2;
    //define the size of the reply
    /** \note previously send by: sizeMultipleCodePacketServerToClient */
    replySizeMultipleCodePacketClientToServer[0x79][0x0001]=0;
    replySizeMultipleCodePacketClientToServer[0x79][0x0002]=0;
    /** \note previously send by: sizeMultipleCodePacketClientToServer */
    replySizeMultipleCodePacketServerToClient[0x10][0x0006]=1;
    replySizeMultipleCodePacketServerToClient[0x10][0x0007]=1;
    replySizeMultipleCodePacketServerToClient[0x80][0x0001]=1;

    compressionMultipleCodePacketServerToClient[0x02] << 0x000C;
    compressionMultipleCodePacketClientToServer[0xC2] << 0x0004;
    compressionMultipleCodePacketClientToServer[0xC2] << 0x000D;
    //define the compression of the reply
    /** \note previously send by: sizeMultipleCodePacketClientToServer */
    replyComressionMultipleCodePacketServerToClient[0x02] << 0x000C;
    replyComressionMultipleCodePacketServerToClient[0x02] << 0x0002;
    replyComressionMultipleCodePacketServerToClient[0x02] << 0x0005;

    //main code for query with reply
    ProtocolParsing::mainCode_IsQueryClientToServer << 0x02 << 0x03 << 0x04 << 0x05 << 0x10 << 0x20 << 0x30;//replySizeMultipleCodePacketServerToClient
    ProtocolParsing::mainCode_IsQueryServerToClient << 0x79 << 0x80 << 0x90 << 0xA0;//replySizeMultipleCodePacketClientToServer

    //reply code
    ProtocolParsing::replyCodeServerToClient=0xC1;
    ProtocolParsing::replyCodeClientToServer=0x41;

    //register meta type
    #ifndef EPOLLCATCHCHALLENGERSERVER
    qRegisterMetaType<CatchChallenger::PlayerMonster >("CatchChallenger::PlayerMonster");//for Api_protocol::tradeAddTradeMonster()
    qRegisterMetaType<QList<quint8> >("QList<quint8>");//for battleAcceptedByOther(stat,publicPlayerMonster);
    qRegisterMetaType<PublicPlayerMonster >("PublicPlayerMonster");//for battleAcceptedByOther(stat,publicPlayerMonster);
    qRegisterMetaType<QList<Skill::AttackReturn> >("QList<Skill::AttackReturn>");//for battleAcceptedByOther(stat,publicPlayerMonster);
    qRegisterMetaType<QList<MarketMonster> >("QList<MarketMonster>");
    qRegisterMetaType<QList<CharacterEntry> >("QList<CharacterEntry>");
    qRegisterMetaType<QSslSocket::SslMode>("QSslSocket::SslMode");
    #endif
}

void ProtocolParsing::setMaxPlayers(const quint16 &maxPlayers)
{
    if(maxPlayers<=255)
    {
        ProtocolParsing::sizeOnlyMainCodePacketServerToClient[0xC3]=1;
    }
    else
    {
        //NO: this case do into initialiseTheVariable()
        //YES: reinitialise because the initialise already done, but this object can be reused
        ProtocolParsing::sizeOnlyMainCodePacketServerToClient[0xC3]=2;
    }
}

ProtocolParsingInputOutput::ProtocolParsingInputOutput(
        #ifdef EPOLLCATCHCHALLENGERSERVER
            #ifndef SERVERNOSSL
                const int &infd, SSL_CTX *ctx
            #else
                const int &infd
            #endif
        #else
        ConnectedSocket *socket
        #endif
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        ,PacketModeTransmission packetModeTransmission
        #endif
        ) :
    #ifdef EPOLLCATCHCHALLENGERSERVER
        #ifndef SERVERNOSSL
            ProtocolParsing(infd,ctx),
        #else
            ProtocolParsing(infd),
        #endif
    #else
    ProtocolParsing(socket),
    #endif
    // for data
    haveData(false),
    haveData_dataSize(0),
    is_reply(false),
    dataSize(0),
    //to parse the netwrok stream
    RXSize(0),
    have_subCodeType(false),
    need_subCodeType(false),
    need_query_number(false),
    have_query_number(false),
    TXSize(0),
    mainCodeType(0),
    subCodeType(0),
    queryNumber(0)
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    //if(!connect(socket,&ConnectedSocket::readyRead,this,&ProtocolParsingInputOutput::parseIncommingData,Qt::QueuedConnection/*to virtual socket*/))
    //    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::ProtocolParsingInputOutput(): can't connect the object"));
    #endif
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    isClient=(packetModeTransmission==PacketModeTransmission_Client);
    #endif
}

bool ProtocolParsingInputOutput::checkStringIntegrity(const QByteArray &data)
{
    return checkStringIntegrity(data.constData(),data.size());
}

bool ProtocolParsingInputOutput::checkStringIntegrity(const char *data, const unsigned int &size)
{
    if(size<(int)sizeof(unsigned int))
    {
        errorParsingLayer("header size not suffisient");
        return false;
    }
    const quint32 &tempInt=*reinterpret_cast<const quint32 *>(data);
    if(tempInt==4294967295)
    {
        //null string
        return true;
    }
    const quint32 &stringSize=be32toh(tempInt);
    if(stringSize>65535)
    {
        errorParsingLayer(QStringLiteral("String size is wrong: %1").arg(stringSize));
        return false;
    }
    if(size<stringSize)
    {
        errorParsingLayer(QStringLiteral("String size is greater than the data: %1>%2").arg(size).arg(stringSize));
        return false;
    }
    return true;
}

quint64 ProtocolParsingInputOutput::getRXSize() const
{
    return RXSize;
}

quint64 ProtocolParsingInputOutput::getTXSize() const
{
    return TXSize;
}

void ProtocolParsingInputOutput::reset()
{
    TXSize=0;
    waitedReply_mainCodeType.clear();
    waitedReply_subCodeType.clear();
    replyOutputCompression.clear();
    replyOutputSize.clear();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    queryReceived.clear();
    #endif

    RXSize=0;

    dataClear();
}
