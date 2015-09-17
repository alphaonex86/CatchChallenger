#ifndef CATCHCHALLENGER_PROTOCOLPARSING_H
#define CATCHCHALLENGER_PROTOCOLPARSING_H

#include <unordered_set>
#include <unordered_map>
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
    //#define CATCHCHALLENGER_BIGBUFFERSIZE 256*1024 without datapack send
    #define CATCHCHALLENGER_BIGBUFFERSIZE 8*1024*1024
    #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_MAX_PACKET_SIZE
    #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_MAX_PACKET_SIZE
    #endif
    #define CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER CATCHCHALLENGER_BIGBUFFERSIZE-16
    #ifndef CATCHCHALLENGER_EXTRA_CHECK
        #define CATCHCHALLENGERSERVERDROPIFCLENT
    #endif
#endif

#define CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT 0x7F
#define CATCHCHALLENGER_PROTOCOL_REPLY_CLIENT_TO_SERVER 0x01

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
        Xz,
        Lz4
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
    static uint8_t compressionLevel;
    #endif
    static const uint8_t packetFixedSize[256+128];
    ProtocolParsing();
    static void initialiseTheVariable(const InitialiseTheVariableType &initialiseTheVariableType=InitialiseTheVariableType::AllInOne);
    static void setMaxPlayers(const uint16_t &maxPlayers);
protected:
    virtual void errorParsingLayer(const std::string &error) = 0;
    virtual void messageParsingLayer(const std::string &message) const = 0;
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
    void setMaxPlayers(const uint16_t &maxPlayers);
    friend class ProtocolParsing;
    friend class ProtocolParsingCheck;
    virtual ssize_t read(char * data, const size_t &size) = 0;
    virtual ssize_t write(const char * const data, const size_t &size) = 0;
    virtual void registerOutputQuery(const uint8_t &packetCode,const uint8_t &queryNumber);
public:
    bool parseIncommingDataRaw(const char * const commonBuffer, const uint32_t &size,uint32_t &cursor);
    #ifndef EPOLLCATCHCHALLENGERSERVER
    std::vector<std::string> getQueryRunningList();
    #endif
protected:
    #ifdef EPOLLCATCHCHALLENGERSERVER
    //internal fast path to boost the move on map performance
    virtual void moveClientFastPath(const uint8_t &previousMovedUnit,const uint8_t &direction) = 0;
    #endif
    bool parseHeader(const char * const commonBuffer, const uint32_t &size, uint32_t &cursor);
    bool parseQueryNumber(const char * const commonBuffer, const uint32_t &size,uint32_t &cursor);
    bool parseDataSize(const char * const commonBuffer, const uint32_t &size,uint32_t &cursor);
    bool parseData(const char * const commonBuffer, const uint32_t &size,uint32_t &cursor);
    bool parseDispatch(const char * const data,const int &size);
    inline bool isReply() const;
    QByteArray header_cut;
    uint8_t flags;
    /* flags & 0x80 = haveData
     * flags & 0x40 = haveData_dataSize
     * flags & 0x20 = have_query_number
     * flags & 0x10 = isClient */
protected:
    //have message without reply
    virtual bool parseMessage(const uint8_t &packetCode,const char * const data,const unsigned int &size) = 0;
    //have query with reply
    virtual bool parseQuery(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const unsigned int &size) = 0;
    //send reply
    virtual bool parseReplyData(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const unsigned int &size) = 0;

    virtual void reset();
private:
    QByteArray dataToWithoutHeader;
    uint32_t dataSize;
    // function
    void dataClear();
public:
    //reply to the query
    char outputQueryNumberToPacketCode[256];//invalidation packet code: 0x00, store the packetCode
    char inputQueryNumberToPacketCode[256];//invalidation packet code: 0x00, store the packetCode, store size is useless because the resolution or is do at send or at receive, then no performance gain

    static char tempBigBufferForOutput[CATCHCHALLENGER_BIGBUFFERSIZE];
    static char tempBigBufferForCompressedOutput[CATCHCHALLENGER_BIGBUFFERSIZE];
    static char tempBigBufferForUncompressedInput[CATCHCHALLENGER_BIGBUFFERSIZE];
private:
    bool internalPackOutcommingData(const char * const data,const int &size);

    // for data
    uint8_t packetCode;
    uint8_t queryNumber;
    static QByteArray lzmaCompress(QByteArray data);
    static QByteArray lzmaUncompress(QByteArray data);
public:
    virtual void storeInputQuery(const uint8_t &packetCode,const uint8_t &queryNumber);
protected:
    //reply to the query
    bool removeFromQueryReceived(const uint8_t &queryNumber);
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
    void storeInputQuery(const uint8_t &packetCode,const uint8_t &queryNumber);

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
    #ifndef EPOLLCATCHCHALLENGERSERVER
    char commonBuffer[CATCHCHALLENGER_COMMONBUFFERSIZE];
    #endif
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    int parseIncommingDataCount;//by object
    #endif
};

}

#endif // PROTOCOLPARSING_H
