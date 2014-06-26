#ifndef CATCHCHALLENGER_PROTOCOLPARSING_H
#define CATCHCHALLENGER_PROTOCOLPARSING_H

#include <QSet>
#include <QHash>
#include <QByteArray>

#include "GeneralStructures.h"
#include "GeneralVariable.h"
#include "ConnectedSocket.h"

#define CATCHCHALLENGER_COMMONBUFFERSIZE 4096

#ifdef EPOLLCATCHCHALLENGERSERVER
#define CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION
#define CATCHCHALLENGER_BIGBUFFERSIZE 16*1024*1024
#define CATCHCHALLENGERSERVERDROPIFCLENT
#endif

namespace CatchChallenger {

class ProtocolParsing
{
public:
    enum CompressionType
    {
        CompressionType_None,
        CompressionType_Zlib,
        CompressionType_Xz
    };
    static CompressionType compressionType;
    ProtocolParsing(ConnectedSocket * socket);
    static void initialiseTheVariable();
    static void setMaxPlayers(const quint16 &maxPlayers);
protected:
    ConnectedSocket * socket;
    /********************** static *********************/
    //connexion parameters
    static QSet<quint8> mainCodeWithoutSubCodeTypeServerToClient;//if need sub code or not
    static QSet<quint8> mainCodeWithoutSubCodeTypeClientToServer;//if need sub code or not
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
    static QHash<quint8,QSet<quint16> > compressionMultipleCodePacketClientToServer;
    static QHash<quint8,QSet<quint16> > compressionMultipleCodePacketServerToClient;
    static QHash<quint8,QSet<quint16> > replyComressionMultipleCodePacketClientToServer;
    static QHash<quint8,QSet<quint16> > replyComressionMultipleCodePacketServerToClient;
    static QSet<quint8> replyComressionOnlyMainCodePacketClientToServer;
    static QSet<quint8> replyComressionOnlyMainCodePacketServerToClient;
protected:
    virtual void errorParsingLayer(const QString &error) = 0;
    virtual void messageParsingLayer(const QString &message) const = 0;
private:
    virtual void reset() = 0;
};

class ProtocolParsingInputOutput : public ProtocolParsing
{
public:
    ProtocolParsingInputOutput(ConnectedSocket * socket
                               #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                               ,PacketModeTransmission packetModeTransmission
                               #endif
                               );
    friend class ProtocolParsing;
    bool checkStringIntegrity(const char *data, const unsigned int &size);
    bool checkStringIntegrity(const QByteArray &data);
    quint64 getRXSize() const;
protected:
    void parseIncommingData();
    bool parseHeader(const quint32 &size,quint32 &cursor);
    bool parseQueryNumber(const quint32 &size,quint32 &cursor);
    bool parseDataSize(const quint32 &size,quint32 &cursor);
    bool parseData(const quint32 &size,quint32 &cursor);
    void parseDispatch(const char * data,const int &size);
    static char commonBuffer[CATCHCHALLENGER_COMMONBUFFERSIZE];
protected:
    //have message without reply
    virtual void parseMessage(const quint8 &mainCodeType,const char *data,const int &size) = 0;
    virtual void parseFullMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const char *data,const int &size) = 0;
    //have query with reply
    virtual void parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size) = 0;
    virtual void parseFullQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *data,const int &size) = 0;
    //send reply
    virtual void parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size) = 0;
    virtual void parseFullReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *data,const int &size) = 0;

    virtual void reset();
private:
    // for data
    bool haveData;
    bool haveData_dataSize;
    bool is_reply;
    QByteArray dataToWithoutHeader;
    QByteArray data_size;
    QByteArray header_cut;
    quint32 dataSize;
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    bool isClient;
    #endif
    //to parse the netwrok stream
    quint64 RXSize;
    bool have_subCodeType,need_subCodeType,need_query_number,have_query_number;
    // function
    void dataClear();
    //reply to the query
    QHash<quint8,quint8> waitedReply_mainCodeType;
    QHash<quint8,quint16> waitedReply_subCodeType;
    QSet<quint8> replyOutputCompression;
    QHash<quint8,quint16> replyOutputSize;
public:
    void newOutputQuery(const quint8 &mainCodeType,const quint8 &queryNumber);
    void newFullOutputQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber);
    //send message without reply
    bool packOutcommingData(const quint8 &mainCodeType,const QByteArray &data);
    bool packFullOutcommingData(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data);
    //send query with reply
    bool packOutcommingQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data);
    bool packFullOutcommingQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data);
    //send reply
    bool postReplyData(const quint8 &queryNumber, const QByteArray &data);
    quint64 getTXSize() const;

    //compute some packet
    //send message without reply
    static QByteArray computeOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            const bool &isClient,
            #endif
            const quint8 &mainCodeType,const QByteArray &data);
    static QByteArray computeFullOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            const bool &isClient,
            #endif
            const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data);
    //send query with reply
    static QByteArray computeOutcommingQuery(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            const bool &isClient,
            #endif
            const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data);
    static QByteArray computeFullOutcommingQuery(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            const bool &isClient,
            #endif
            const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data);
    //send reply
    QByteArray computeReplyData(const quint8 &queryNumber, const QByteArray &data);
    int computeReplyData(char *dataBuffer, const quint8 &queryNumber, const QByteArray &data);
    //compression
    static QByteArray computeCompression(const QByteArray &data);
private:
    bool internalPackOutcommingData(QByteArray data);
    bool internalPackOutcommingData(const char *data,const int &size);
    static QByteArray encodeSize(const quint32 &size);
    static qint8 encodeSize(char *data,const quint32 &size);

    quint64 TXSize;
    //reply to the query
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    QSet<quint8> queryReceived;
    #endif
    // for data
    quint8 mainCodeType;
    quint32 subCodeType;
    quint8 queryNumber;
    static QByteArray lzmaCompress(QByteArray data);
    static QByteArray lzmaUncompress(QByteArray data);
    static quint16 sizeHeaderNullquint16;
    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    static char tempBigBuffer[CATCHCHALLENGER_BIGBUFFERSIZE];
    #endif
public:
    void storeInputQuery(const quint8 &mainCodeType,const quint8 &queryNumber);
    void storeFullInputQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber);
protected:
    //no control to be more fast
    bool internalSendRawSmallPacket(const QByteArray &data);
    bool internalSendRawSmallPacket(const char *data,const int &size);
};

}

#endif // PROTOCOLPARSING_H
