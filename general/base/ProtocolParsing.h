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
    ProtocolParsing();
    static void initialiseTheVariable(const InitialiseTheVariableType &initialiseTheVariableType=InitialiseTheVariableType::AllInOne);
    static void setMaxPlayers(const uint16_t &maxPlayers);
protected:
    /********************** static *********************/
    //connexion parameters
    static std::unordered_set<uint8_t> mainCodeWithoutSubCodeTypeServerToClient;//if need sub code or not
    static std::unordered_set<uint8_t> mainCodeWithoutSubCodeTypeClientToServer;//if need sub code or not
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    static std::unordered_set<uint8_t> toDebugValidMainCodeServerToClient;//if need sub code or not
    static std::unordered_set<uint8_t> toDebugValidMainCodeClientToServer;//if need sub code or not
    #endif
    //if is a query
    static std::unordered_set<uint8_t> mainCode_IsQueryClientToServer;
    static uint8_t replyCodeClientToServer;
    static std::unordered_set<uint8_t> mainCode_IsQueryServerToClient;
    static uint8_t replyCodeServerToClient;
    //predefined size
    static std::unordered_map<uint8_t,uint16_t> sizeOnlyMainCodePacketClientToServer;
    static std::unordered_map<uint8_t,std::unordered_map<uint16_t,uint16_t> > sizeMultipleCodePacketClientToServer;
    static std::unordered_map<uint8_t,uint16_t> replySizeOnlyMainCodePacketClientToServer;
    static std::unordered_map<uint8_t,std::unordered_map<uint16_t,uint16_t> > replySizeMultipleCodePacketClientToServer;
    static std::unordered_map<uint8_t,uint16_t> sizeOnlyMainCodePacketServerToClient;
    static std::unordered_map<uint8_t,std::unordered_map<uint16_t,uint16_t> > sizeMultipleCodePacketServerToClient;
    static std::unordered_map<uint8_t,uint16_t> replySizeOnlyMainCodePacketServerToClient;
    static std::unordered_map<uint8_t,std::unordered_map<uint16_t,uint16_t> > replySizeMultipleCodePacketServerToClient;

    //compression not found single main code because is reserved to fast/small message
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    static std::unordered_map<uint8_t,std::unordered_set<uint16_t> > compressionMultipleCodePacketClientToServer;
    static std::unordered_map<uint8_t,std::unordered_set<uint16_t> > compressionMultipleCodePacketServerToClient;
    static std::unordered_map<uint8_t,std::unordered_set<uint16_t> > replyComressionMultipleCodePacketClientToServer;
    static std::unordered_map<uint8_t,std::unordered_set<uint16_t> > replyComressionMultipleCodePacketServerToClient;
    static std::unordered_set<uint8_t> replyComressionOnlyMainCodePacketClientToServer;
    static std::unordered_set<uint8_t> replyComressionOnlyMainCodePacketServerToClient;
    #endif
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
    friend class ProtocolParsing;
    friend class ProtocolParsingCheck;
    bool checkStringIntegrity(const char * const data, const unsigned int &size);
    bool checkStringIntegrity(const QByteArray &data);
    virtual ssize_t read(char * data, const size_t &size) = 0;
    virtual ssize_t write(const char * const data, const size_t &size) = 0;
public:
    bool parseIncommingDataRaw(const char * const commonBuffer, const uint32_t &size,uint32_t &cursor);
    #ifndef EPOLLCATCHCHALLENGERSERVER
    std::vector<std::string> getQueryRunningList();
    #endif
protected:
    bool parseHeader(const char * const commonBuffer, const uint32_t &size, uint32_t &cursor);
    bool parseQueryNumber(const char * const commonBuffer, const uint32_t &size,uint32_t &cursor);
    bool parseDataSize(const char * const commonBuffer, const uint32_t &size,uint32_t &cursor);
    bool parseData(const char * const commonBuffer, const uint32_t &size,uint32_t &cursor);
    bool parseDispatch(const char * const data,const int &size);
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    bool isClient;
    #endif
    QByteArray header_cut;
protected:
    //have message without reply
    virtual void parseMessage(const uint8_t &mainCodeType,const char * const data,const unsigned int &size) = 0;
    virtual void parseFullMessage(const uint8_t &mainCodeType,const uint8_t &subCodeType,const char * const data,const unsigned int &size) = 0;
    //have query with reply
    virtual void parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size) = 0;
    virtual void parseFullQuery(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size) = 0;
    //send reply
    virtual void parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size) = 0;
    virtual void parseFullReplyData(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size) = 0;

    virtual void reset();
private:
    // for data
    bool haveData;
    bool haveData_dataSize;
    bool is_reply;
    QByteArray dataToWithoutHeader;
    uint8_t data_size_size;
    uint32_t dataSize;
    //to parse the netwrok stream
    bool have_subCodeType,need_subCodeType,need_query_number,have_query_number;
    // function
    void dataClear();
public:
    //reply to the query
    std::unordered_map<uint8_t,uint8_t> waitedReply_mainCodeType;
    std::unordered_map<uint8_t,uint8_t> waitedReply_subCodeType;
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    std::unordered_set<uint8_t> replyOutputCompression;
    #endif
    std::unordered_map<uint8_t,uint16_t> replyOutputSize;
public:
    void newOutputQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber);
    void newFullOutputQuery(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber);
    //send message without reply
    bool packOutcommingData(const uint8_t &mainCodeType,const char * const data,const int &size);
    bool packFullOutcommingData(const uint8_t &mainCodeType,const uint8_t &subCodeType,const char * const data,const int &size);
    //send query with reply
    bool packOutcommingQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const int &size);
    bool packFullOutcommingQuery(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber,const char * const data,const int &size);
    //send reply
    bool postReplyData(const uint8_t &queryNumber, const char * const data,const int &size);

    //compute some packet
    //send message without reply
    static int computeOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            const bool &isClient,
            #endif
            char *buffer,
            const uint8_t &mainCodeType,const char * const data,const int &size);
    static int computeFullOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            const bool &isClient,
            #endif
            char *buffer,
            const uint8_t &mainCodeType,const uint8_t &subCodeType,const char * const data,const int &size);
    //send query with reply
    static int computeOutcommingQuery(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            const bool &isClient,
            #endif
            char *buffer,
            const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const int &size);
    static int computeFullOutcommingQuery(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            const bool &isClient,
            #endif
            char *buffer,
            const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber,const char * const data,const int &size);
    //send reply
    static int computeReplyData(
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        const bool &isClient,
        #endif
        char *dataBuffer, const uint8_t &queryNumber, const char * const data, const int &size,
        const int32_t &replyOutputSizeInt
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
    static qint8 encodeSize(char *data,const uint32_t &size);

    // for data
    uint8_t mainCodeType;
    uint8_t subCodeType;
    uint8_t queryNumber;
    static QByteArray lzmaCompress(QByteArray data);
    static QByteArray lzmaUncompress(QByteArray data);
    static const uint16_t sizeHeaderNulluint16_t;
public:
    virtual void storeInputQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber);
    virtual void storeFullInputQuery(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber);
protected:
    //reply to the query
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    bool removeFromQueryReceived(const uint8_t &queryNumber);
    std::unordered_set<uint8_t> queryReceived;
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
    void storeInputQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber);
    void storeFullInputQuery(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber);

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
    int parseIncommingDataCount;//by object
    #endif
};

}

#endif // PROTOCOLPARSING_H
