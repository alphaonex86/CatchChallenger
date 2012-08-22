#ifndef PROTOCOLPARSING_H
#define PROTOCOLPARSING_H

#include <QSet>
#include <QHash>
#include <QByteArray>
#include <QObject>
#include <QAbstractSocket>

#include "GeneralStructures.h"
#include "GeneralVariable.h"

namespace Pokecraft {
class ProtocolParsing : public QObject
{
	Q_OBJECT
public:
	ProtocolParsing(QAbstractSocket * socket);
	static void initialiseTheVariable();
	static QByteArray toUTF8(const QString &text);
protected:
	QAbstractSocket * socket;
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
signals:
	void error(const QString &error);
	void message(const QString &message);
};

class ProtocolParsingInput : public ProtocolParsing
{
	Q_OBJECT
public:
	ProtocolParsingInput(QAbstractSocket * socket,PacketModeTransmission packetModeTransmission);
	friend class ProtocolParsing;
	bool checkStringIntegrity(const QByteArray & data);
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
	ProtocolParsingOutput(QAbstractSocket * socket,PacketModeTransmission packetModeTransmission);
	friend class ProtocolParsing;

	//send message without reply
	bool packOutcommingData(const quint8 &mainCodeType,const QByteArray &data);
	bool packOutcommingData(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data);
	//send query with reply
	bool packOutcommingQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data);
	bool packOutcommingQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data);
	//send reply
	bool postReplyData(const quint8 &queryNumber,const QByteArray &data);
private:
	bool internalPackOutcommingData(const QByteArray &data);
	QByteArray encodeSize(quint32 size);

	bool isClient;
	//temp data
	static qint64 byteWriten;
	//reply to the query
	QHash<quint8,quint16> replySize;
signals:
	void newOutputQuery(const quint8 &mainCodeType,const quint8 &queryNumber);
	void newOutputQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber);
public slots:
	void newInputQuery(const quint8 &mainCodeType,const quint8 &queryNumber);
	void newInputQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber);
};
}

#endif // PROTOCOLPARSING_H
