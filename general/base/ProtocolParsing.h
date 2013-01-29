#ifndef POKECRAFT_PROTOCOLPARSING_H
#define POKECRAFT_PROTOCOLPARSING_H

#include <QSet>
#include <QHash>
#include <QByteArray>
#include <QObject>

#include "GeneralStructures.h"
#include "GeneralVariable.h"
#include "ConnectedSocket.h"

namespace Pokecraft {
class ProtocolParsing : public QObject
{
    Q_OBJECT
public:
    ProtocolParsing(ConnectedSocket * socket);
    static void initialiseTheVariable();
    static void setMaxPlayers(const quint16 &maxPlayers);
protected:
    ConnectedSocket * socket;
    /********************** static *********************/
    //connexion parameters
    static QSet<quint8> mainCodeWithoutSubCodeTypeClientToServer;//if need sub code or not
    static QSet<quint8> mainCodeWithoutSubCodeTypeServerToClient;//if need sub code or not
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
signals:
    void error(const QString &error);
    void message(const QString &message);
private slots:
    virtual void reset() = 0;
};

class ProtocolParsingInput : public ProtocolParsing
{
    Q_OBJECT
public:
    ProtocolParsingInput(ConnectedSocket * socket,PacketModeTransmission packetModeTransmission);
    friend class ProtocolParsing;
    bool checkStringIntegrity(const QByteArray & data);
    quint64 getRXSize();
protected:
    //have message without reply
    virtual void parseMessage(const quint8 &mainCodeType,const QByteArray &data) = 0;
    virtual void parseMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data) = 0;
    //have query with reply
    virtual void parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data) = 0;
    virtual void parseQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data) = 0;
    //send reply
    virtual void parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data) = 0;
    virtual void parseReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data) = 0;
    // for data
    bool haveData;
    bool haveData_dataSize;
    bool is_reply;
    QByteArray data_size;
    quint32 dataSize;
    QByteArray data;
    bool isClient;
    //to parse the netwrok stream
    quint64 RXSize;
    quint8 mainCodeType;
    quint16 subCodeType;
    quint8 queryNumber;
    bool have_subCodeType,need_subCodeType,need_query_number,have_query_number;
    // function
    void dataClear();
    //temp data
    static quint8 temp_size_8Bits;
    static quint16 temp_size_16Bits;
    static quint32 temp_size_32Bits;
    //reply to the query
    QHash<quint8,quint16> replySize;
    QHash<quint8,quint8> reply_mainCodeType;
    QHash<quint8,quint16> reply_subCodeType;
private slots:
    void parseIncommingData();
    void reset();
signals:
    void newInputQuery(const quint8 &mainCodeType,const quint8 &queryNumber);
    void newInputQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber);
public slots:
    void newOutputQuery(const quint8 &mainCodeType,const quint8 &queryNumber);
    void newOutputQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber);
};

class ProtocolParsingOutput : public ProtocolParsing
{
    Q_OBJECT
public:
    ProtocolParsingOutput(ConnectedSocket * socket,PacketModeTransmission packetModeTransmission);
    friend class ProtocolParsing;

    //send message without reply
    bool packOutcommingData(const quint8 &mainCodeType,const QByteArray &data);
    bool packOutcommingData(const quint8 &mainCodeType,const quint16 &subCodeType,QByteArray data);
    //send query with reply
    bool packOutcommingQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data);
    bool packOutcommingQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,QByteArray data);
    //send reply
    bool postReplyData(const quint8 &queryNumber, QByteArray data);
    quint64 getTXSize();
private:
    bool internalPackOutcommingData(const QByteArray &data);
    QByteArray encodeSize(quint32 size);

    quint64 TXSize;
    bool isClient;
    //temp data
    static qint64 byteWriten;
    //reply to the query
    QHash<quint8,quint16> replySize;
    QSet<quint8> replyCompression;
    #ifdef POKECRAFT_EXTRA_CHECK
    QSet<quint8> queryReceived;
    #endif
signals:
    void newOutputQuery(const quint8 &mainCodeType,const quint8 &queryNumber);
    void newOutputQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber);
public slots:
    void newInputQuery(const quint8 &mainCodeType,const quint8 &queryNumber);
    void newInputQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber);
private slots:
    void reset();
};
}

#endif // PROTOCOLPARSING_H
