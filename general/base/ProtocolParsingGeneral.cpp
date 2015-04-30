#include "ProtocolParsing.h"
#include "DebugClass.h"
#include "GeneralVariable.h"
#include "ProtocolParsingCheck.h"

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
#include <lzma.h>
#endif

using namespace CatchChallenger;

#ifdef EPOLLCATCHCHALLENGERSERVER
char ProtocolParsingInputOutput::commonBuffer[CATCHCHALLENGER_COMMONBUFFERSIZE];
#endif
const quint16 ProtocolParsingBase::sizeHeaderNullquint16=0;
#ifdef CATCHCHALLENGER_BIGBUFFERSIZE
char ProtocolParsingBase::tempBigBufferForOutput[CATCHCHALLENGER_BIGBUFFERSIZE];
#endif
#ifdef CATCHCHALLENGER_EXTRA_CHECK
int ProtocolParsingInputOutput::parseIncommingDataCount=0;
#endif

QSet<quint8>                        ProtocolParsing::mainCodeWithoutSubCodeTypeServerToClient;//if need sub code or not
//if is a query
QSet<quint8>                        ProtocolParsing::mainCode_IsQueryClientToServer;

#ifdef CATCHCHALLENGER_EXTRA_CHECK
QSet<quint8> ProtocolParsing::toDebugValidMainCodeServerToClient;//if need sub code or not
QSet<quint8> ProtocolParsing::toDebugValidMainCodeClientToServer;//if need sub code or not
#endif

quint8                              ProtocolParsing::replyCodeClientToServer;
#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
#ifndef CATCHCHALLENGERSERVERDROPIFCLENT
ProtocolParsing::CompressionType    ProtocolParsing::compressionTypeClient=CompressionType::None;
#endif
ProtocolParsing::CompressionType    ProtocolParsing::compressionTypeServer=CompressionType::None;
#endif
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

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
QHash<quint8,QSet<quint16> >    ProtocolParsing::compressionMultipleCodePacketClientToServer;
QHash<quint8,QSet<quint16> >    ProtocolParsing::compressionMultipleCodePacketServerToClient;
QHash<quint8,QSet<quint16> >    ProtocolParsing::replyComressionMultipleCodePacketClientToServer;
QHash<quint8,QSet<quint16> >    ProtocolParsing::replyComressionMultipleCodePacketServerToClient;
QSet<quint8>                    ProtocolParsing::replyComressionOnlyMainCodePacketClientToServer;
QSet<quint8>                    ProtocolParsing::replyComressionOnlyMainCodePacketServerToClient;
#endif

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
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


QByteArray ProtocolParsingBase::lzmaCompress(QByteArray data)
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

QByteArray ProtocolParsingBase::lzmaUncompress(QByteArray data)
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
            if(!mainCodeWithoutSubCodeTypeClientToServer.isEmpty())
                return;

            #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
            memset(ProtocolParsingBase::tempBigBufferForOutput,0,sizeof(ProtocolParsingBase::tempBigBufferForOutput));
            #endif
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            compressionTypeServer=CompressionType::Zlib;
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            compressionTypeClient=CompressionType::Zlib;
            #endif
            #endif

            //reply code
            ProtocolParsing::replyCodeServerToClient=0xC1;
            ProtocolParsing::replyCodeClientToServer=0x41;

            //def query without the sub code
            mainCodeWithoutSubCodeTypeServerToClient << 0xC0 << 0xC3 << 0xC4 << 0xC5 << 0xC6 << 0xC7 << 0xC8 << 0xCA << 0xD1 << 0xD2;
            mainCodeWithoutSubCodeTypeClientToServer << 0x03 << 0x04 << 0x05  << 0x07  << 0x08  << 0x09 << 0x40 << 0x43 << 0x61;
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            toDebugValidMainCodeServerToClient << 0x79 << 0xC2 << 0x90 << 0xE0 << 0xD0 << 0x80 << 0xF0;
            toDebugValidMainCodeClientToServer << 0x02 << 0x42 << 0x60 << 0x50 << 0x6a;
            #endif

            //define the size of direct query
            {
                //default like is was more than 255 players
                sizeOnlyMainCodePacketServerToClient[0xC3]=2;
            }
            sizeOnlyMainCodePacketServerToClient[0xC4]=0;
            sizeOnlyMainCodePacketClientToServer[0x01]=8+64;
            sizeOnlyMainCodePacketClientToServer[0x40]=2;
            sizeOnlyMainCodePacketClientToServer[0x03]=5;
            sizeOnlyMainCodePacketClientToServer[0x08]=0;
            sizeOnlyMainCodePacketClientToServer[0x04]=CATCHCHALLENGER_FIRSTLOGINPASSHASHSIZE*2;
            sizeOnlyMainCodePacketClientToServer[0x05]=CATCHCHALLENGER_FIRSTLOGINPASSHASHSIZE*2;
            sizeOnlyMainCodePacketClientToServer[0x61]=2;
            sizeMultipleCodePacketClientToServer[0x02][0x04]=1+4;
            sizeMultipleCodePacketClientToServer[0x02][0x05]=4+1+4;
            sizeMultipleCodePacketClientToServer[0x10][0x07]=0;
            sizeMultipleCodePacketClientToServer[0x10][0x08]=2;
            sizeMultipleCodePacketClientToServer[0x10][0x09]=2;
            sizeMultipleCodePacketClientToServer[0x10][0x0A]=2;
            sizeMultipleCodePacketClientToServer[0x10][0x0B]=2*2+2*4;
            sizeMultipleCodePacketClientToServer[0x10][0x0C]=2*2+2*4;
            sizeMultipleCodePacketClientToServer[0x10][0x0D]=2;
            sizeMultipleCodePacketClientToServer[0x10][0x0E]=2*2+2*4;
            sizeMultipleCodePacketClientToServer[0x10][0x0F]=2*2+2*4;
            sizeMultipleCodePacketClientToServer[0x10][0x10]=0;
            sizeMultipleCodePacketClientToServer[0x10][0x13]=0;
            sizeMultipleCodePacketClientToServer[0x11][0x01]=4;
            sizeMultipleCodePacketClientToServer[0x11][0x02]=4;
            sizeMultipleCodePacketClientToServer[0x11][0x03]=4;
            sizeMultipleCodePacketClientToServer[0x11][0x04]=4;
            sizeMultipleCodePacketClientToServer[0x50][0x02]=2+4;
            sizeMultipleCodePacketClientToServer[0x50][0x04]=0;
            sizeMultipleCodePacketClientToServer[0x50][0x05]=0;
            sizeMultipleCodePacketClientToServer[0x50][0x07]=0;
            sizeMultipleCodePacketClientToServer[0x60][0x02]=0;
            sizeMultipleCodePacketClientToServer[0x60][0x03]=1;
            sizeMultipleCodePacketClientToServer[0x60][0x04]=4+2;
            sizeMultipleCodePacketClientToServer[0x60][0x06]=0;
            sizeMultipleCodePacketClientToServer[0x60][0x07]=2;
            sizeMultipleCodePacketClientToServer[0x60][0x08]=2;
            sizeMultipleCodePacketClientToServer[0x60][0x09]=4;//monster id in db
            sizeMultipleCodePacketClientToServer[0x6a][0x01]=2;
            sizeMultipleCodePacketClientToServer[0x6a][0x02]=2;
            sizeMultipleCodePacketClientToServer[0x6a][0x03]=2;
            sizeMultipleCodePacketClientToServer[0x6a][0x04]=2;
            sizeMultipleCodePacketClientToServer[0x6a][0x05]=1;
            sizeMultipleCodePacketClientToServer[0x42][0x04]=1;

            sizeMultipleCodePacketServerToClient[0xC2][0x09]=0;
            sizeMultipleCodePacketServerToClient[0xD0][0x06]=0;
            sizeMultipleCodePacketServerToClient[0xD0][0x07]=0;
            sizeMultipleCodePacketServerToClient[0xD0][0x08]=0;
            sizeMultipleCodePacketServerToClient[0xE0][0x07]=0;
            sizeMultipleCodePacketServerToClient[0x79][0x02]=2;
            //define the size of the reply
            replySizeOnlyMainCodePacketServerToClient[0x01]=1;
            /** \note previously send by: sizeMultipleCodePacketServerToClient */
            replySizeMultipleCodePacketClientToServer[0x79][0x01]=0;
            replySizeMultipleCodePacketClientToServer[0x79][0x02]=0;
            /** \note previously send by: sizeMultipleCodePacketClientToServer */
            replySizeMultipleCodePacketServerToClient[0x10][0x06]=1;
            replySizeMultipleCodePacketServerToClient[0x10][0x07]=1;
            replySizeMultipleCodePacketServerToClient[0x80][0x01]=1;
            replySizeMultipleCodePacketServerToClient[0x11][0x01]=50*4;
            replySizeMultipleCodePacketServerToClient[0x11][0x02]=50*4;
            replySizeMultipleCodePacketServerToClient[0x11][0x03]=50*4;
            replySizeMultipleCodePacketServerToClient[0x11][0x04]=50*4;

            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            compressionMultipleCodePacketClientToServer[0x02] << 0x000C;
            compressionMultipleCodePacketServerToClient[0xC2] << 0x0004;
            compressionMultipleCodePacketServerToClient[0xC2] << 0x000D;
            compressionMultipleCodePacketServerToClient[0xC2] << 0x000E;
            compressionMultipleCodePacketServerToClient[0xC2] << 0x000F;
            compressionMultipleCodePacketServerToClient[0xC2] << 0x0010;
            //define the compression of the reply
            /** \note previously send by: sizeMultipleCodePacketClientToServer */
            replyComressionMultipleCodePacketServerToClient[0x02] << 0x000C;
            replyComressionMultipleCodePacketServerToClient[0x02] << 0x0002;
            replyComressionMultipleCodePacketServerToClient[0x02] << 0x0005;
            #endif

            //main code for query with reply
            ProtocolParsing::mainCode_IsQueryClientToServer << 0x01 << 0x02 << 0x03 << 0x04 << 0x05 << 0x07 << 0x08 << 0x09 << 0x10 << 0x20 << 0x30;//replySizeMultipleCodePacketServerToClient
            ProtocolParsing::mainCode_IsQueryServerToClient << 0x79 << 0x80 << 0x90 << 0xA0;//replySizeMultipleCodePacketClientToServer

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
        break;
    }
    mainCodeWithoutSubCodeTypeServerToClient << ProtocolParsing::replyCodeServerToClient;
    mainCodeWithoutSubCodeTypeClientToServer << ProtocolParsing::replyCodeClientToServer;
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
    mainCodeType(0),
    subCodeType(0),
    queryNumber(0)
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    //if(!connect(socket,&ConnectedSocket::readyRead,this,&ProtocolParsingInputOutput::parseIncommingData,Qt::QueuedConnection/*to virtual socket*/))
    //    messageParsingLayer(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::ProtocolParsingInputOutput(): can't connect the object"));
    #endif
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    isClient=(packetModeTransmission==PacketModeTransmission_Client);
    #endif
}

ProtocolParsingBase::~ProtocolParsingBase()
{
}

bool ProtocolParsingBase::checkStringIntegrity(const QByteArray &data)
{
    return checkStringIntegrity(data.constData(),data.size());
}

bool ProtocolParsingBase::checkStringIntegrity(const char * const data, const unsigned int &size)
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
    const quint32 &stringSize=le32toh(tempInt);
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
    waitedReply_mainCodeType.clear();
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
