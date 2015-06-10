#ifndef CATCHCHALLENGER_PROTOCOLPARSING_H
#define CATCHCHALLENGER_PROTOCOLPARSING_H

#include <QSet>
#include <QHash>
#include <QByteArray>
#include <QDebug>

#include "GeneralVariable.h"
#ifdef EPOLLCATCHCHALLENGERSERVER
    #ifdef SERVERSSL
        #include "../../server/epoll/EpollSslClient.h"
    #else
        #include "../../server/epoll/EpollClient.h"
    #endif
#else
#include "ConnectedSocket.h"
#endif

#define CATCHCHALLENGER_COMMONBUFFERSIZE 4096

#ifdef EPOLLCATCHCHALLENGERSERVER
    //#define CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION
    #define CATCHCHALLENGER_BIGBUFFERSIZE 10*1024*1024
    #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_MAX_PACKET_SIZE
    #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_MAX_PACKET_SIZE
    #endif
    #define CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER CATCHCHALLENGER_BIGBUFFERSIZE-16
    #ifndef CATCHCHALLENGER_EXTRA_CHECK
        #define CATCHCHALLENGERSERVERDROPIFCLENT
    #endif
#endif

namespace CatchChallenger {

class ProtocolParsingCheck;

class ProtocolParsing
{
public:
    /// \brief Define the mode of transmission: client or server
    enum PacketModeTransmission
    {
        PacketModeTransmission_Server=0x00,
        PacketModeTransmission_Client=0x01
    };
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    enum CompressionType
    {
        None,
        Zlib,
        Xz
    };
    #endif
    enum InitialiseTheVariableType
    {
        AllInOne,
        LoginServer,
        GameServer,
        MasterServer
    };
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    static CompressionType compressionTypeClient;
    #endif
    static CompressionType compressionTypeServer;
    #endif
    ProtocolParsing();
    static void initialiseTheVariable(const InitialiseTheVariableType &initialiseTheVariableType=InitialiseTheVariableType::AllInOne);
    static void setMaxPlayers(const quint16 &maxPlayers);
protected:
    /********************** static *********************/
    //connexion parameters
    static QSet<quint8> mainCodeWithoutSubCodeTypeServerToClient;//if need sub code or not
    static QSet<quint8> mainCodeWithoutSubCodeTypeClientToServer;//if need sub code or not
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    static QSet<quint8> toDebugValidMainCodeServerToClient;//if need sub code or not
    static QSet<quint8> toDebugValidMainCodeClientToServer;//if need sub code or not
    #endif
    //if is a query
    static QSet<quint8> mainCode_IsQueryClientToServer;
    static quint8 replyCodeClientToServer;
    static QSet<quint8> mainCode_IsQueryServerToClient;
    static quint8 replyCodeServerToClient;
    //predefined size
    static QHash<quint8,quint16> sizeOnlyMainCodePacketClientToServer;
    static QHash<quint8,QHash<quint16,quint16> > sizeMultipleCodePacketClientToServer;
    static QHash<quint8,quint16> replySizeOnlyMainCodePacketClientToServer;
    static QHash<quint8,QHash<quint16,quint16> > replySizeMultipleCodePacketClientToServer;
    static QHash<quint8,quint16> sizeOnlyMainCodePacketServerToClient;
    static QHash<quint8,QHash<quint16,quint16> > sizeMultipleCodePacketServerToClient;
    static QHash<quint8,quint16> replySizeOnlyMainCodePacketServerToClient;
    static QHash<quint8,QHash<quint16,quint16> > replySizeMultipleCodePacketServerToClient;

    //compression not found single main code because is reserved to fast/small message
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    static QHash<quint8,QSet<quint16> > compressionMultipleCodePacketClientToServer;
    static QHash<quint8,QSet<quint16> > compressionMultipleCodePacketServerToClient;
    static QHash<quint8,QSet<quint16> > replyComressionMultipleCodePacketClientToServer;
    static QHash<quint8,QSet<quint16> > replyComressionMultipleCodePacketServerToClient;
    static QSet<quint8> replyComressionOnlyMainCodePacketClientToServer;
    static QSet<quint8> replyComressionOnlyMainCodePacketServerToClient;
    #endif
protected:
    virtual void errorParsingLayer(const QString &error) = 0;
    virtual void messageParsingLayer(const QString &message) const = 0;
private:
    virtual void reset() = 0;
};

class ProtocolParsingBase : public ProtocolParsing
{
public:
    ProtocolParsingBase(
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    PacketModeTransmission packetModeTransmission
    #endif
    );
    virtual ~ProtocolParsingBase();
    friend class ProtocolParsing;
    friend class ProtocolParsingCheck;
    bool checkStringIntegrity(const char * const data, const unsigned int &size);
    bool checkStringIntegrity(const QByteArray &data);
    virtual ssize_t read(char * data, const size_t &size) = 0;
    virtual ssize_t write(const char * const data, const size_t &size) = 0;
public:
    bool parseIncommingDataRaw(const char * const commonBuffer, const quint32 &size,quint32 &cursor);
    #ifndef EPOLLCATCHCHALLENGERSERVER
    QStringList getQueryRunningList();
    #endif
protected:
    bool parseHeader(const char * const commonBuffer, const quint32 &size, quint32 &cursor);
    bool parseQueryNumber(const char * const commonBuffer, const quint32 &size,quint32 &cursor);
    bool parseDataSize(const char * const commonBuffer, const quint32 &size,quint32 &cursor);
    bool parseData(const char * const commonBuffer, const quint32 &size,quint32 &cursor);
    bool parseDispatch(const char * const data,const int &size);
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    bool isClient;
    #endif
    QByteArray header_cut;
protected:
    //have message without reply
    virtual void parseMessage(const quint8 &mainCodeType,const char * const data,const unsigned int &size) = 0;
    virtual void parseFullMessage(const quint8 &mainCodeType,const quint8 &subCodeType,const char * const data,const unsigned int &size) = 0;
    //have query with reply
    virtual void parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size) = 0;
    virtual void parseFullQuery(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size) = 0;
    //send reply
    virtual void parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size) = 0;
    virtual void parseFullReplyData(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size) = 0;

    virtual void reset();
private:
    // for data
    bool haveData;
    bool haveData_dataSize;
    bool is_reply;
    QByteArray dataToWithoutHeader;
    quint8 data_size_size;
    quint32 dataSize;
    //to parse the netwrok stream
    bool have_subCodeType,need_subCodeType,need_query_number,have_query_number;
    // function
    void dataClear();
public:
    //reply to the query
    QHash<quint8,quint8> waitedReply_mainCodeType;
    QHash<quint8,quint8> waitedReply_subCodeType;
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    QSet<quint8> replyOutputCompression;
    #endif
    QHash<quint8,quint16> replyOutputSize;
public:
    void newOutputQuery(const quint8 &mainCodeType,const quint8 &queryNumber);
    void newFullOutputQuery(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber);
    //send message without reply
    bool packOutcommingData(const quint8 &mainCodeType,const char * const data,const int &size);
    bool packFullOutcommingData(const quint8 &mainCodeType,const quint8 &subCodeType,const char * const data,const int &size);
    //send query with reply
    bool packOutcommingQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const int &size);
    bool packFullOutcommingQuery(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char * const data,const int &size);
    //send reply
    bool postReplyData(const quint8 &queryNumber, const char * const data,const int &size);

    //compute some packet
    //send message without reply
    static int computeOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            const bool &isClient,
            #endif
            char *buffer,
            const quint8 &mainCodeType,const char * const data,const int &size);
    static int computeFullOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            const bool &isClient,
            #endif
            char *buffer,
            const quint8 &mainCodeType,const quint8 &subCodeType,const char * const data,const int &size);
    //send query with reply
    static int computeOutcommingQuery(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            const bool &isClient,
            #endif
            char *buffer,
            const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const int &size);
    static int computeFullOutcommingQuery(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            const bool &isClient,
            #endif
            char *buffer,
            const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char * const data,const int &size);
    //send reply
    static int computeReplyData(
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        const bool &isClient,
        #endif
        char *dataBuffer, const quint8 &queryNumber, const char * const data, const int &size,
        const qint32 &replyOutputSizeInt
        #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
        , const CompressionType &compressionType
        #endif
        );
    //compression
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    static QByteArray computeCompression(const QByteArray &data, const CompressionType &compressionType);
    #endif
private:
    bool internalPackOutcommingData(const char * const data,const int &size);
    static qint8 encodeSize(char *data,const quint32 &size);

    // for data
    quint8 mainCodeType;
    quint8 subCodeType;
    quint8 queryNumber;
    static QByteArray lzmaCompress(QByteArray data);
    static QByteArray lzmaUncompress(QByteArray data);
    static const quint16 sizeHeaderNullquint16;
public:
    virtual void storeInputQuery(const quint8 &mainCodeType,const quint8 &queryNumber);
    virtual void storeFullInputQuery(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber);
protected:
    //reply to the query
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    bool removeFromQueryReceived(const quint8 &queryNumber);
    QSet<quint8> queryReceived;
    #endif
    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    static char tempBigBufferForOutput[CATCHCHALLENGER_BIGBUFFERSIZE];
    #endif
    bool internalSendRawSmallPacket(const char * const data,const int &size);
    virtual void disconnectClient() = 0;
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    virtual ProtocolParsing::CompressionType getCompressType() const = 0;
    #endif
};

class ProtocolParsingInputOutput : public ProtocolParsingBase
{
public:
    ProtocolParsingInputOutput(
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
       );
    virtual ~ProtocolParsingInputOutput();
    friend class Client;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    quint64 getTXSize() const;
    quint64 getRXSize() const;
    #endif
    void storeInputQuery(const quint8 &mainCodeType,const quint8 &queryNumber);
    void storeFullInputQuery(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber);

    void closeSocket();
protected:
    void parseIncommingData();
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    ProtocolParsing::CompressionType getCompressType() const;
    #endif
    #ifdef EPOLLCATCHCHALLENGERSERVER
        #ifdef SERVERSSL
            EpollSslClient epollSocket;
        #else
            EpollClient epollSocket;
        #endif
    #else
        ConnectedSocket *socket;
    #endif
    ssize_t read(char * data, const size_t &size);
    ssize_t write(const char * const data, const size_t &size);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    ProtocolParsingCheck *protocolParsingCheck;
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    quint64 TXSize;
    quint64 RXSize;
    #endif
    #ifdef EPOLLCATCHCHALLENGERSERVER
    static char commonBuffer[CATCHCHALLENGER_COMMONBUFFERSIZE];
    #else
    char commonBuffer[CATCHCHALLENGER_COMMONBUFFERSIZE];
    #endif
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    static int parseIncommingDataCount;
    #endif
};

}

#endif // PROTOCOLPARSING_H
