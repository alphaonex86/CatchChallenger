#ifndef PROTOCOLPARSING_H
#define PROTOCOLPARSING_H

#include <QSet>
#include <QHash>
#include <QByteArray>
#include <QObject>

#include "GeneralStructures.h"
#include "GeneralVariable.h"

class ProtocolParsing : public QObject
{
	Q_OBJECT
public:
	ProtocolParsing(QIODevice * device);
	static PacketSizeMode packetSizeMode;
	static void initialiseTheVariable(PacketModeTransmission packetModeTransmission);
protected:
	QIODevice * device;
signals:
	void error(const QString &error);
	void message(const QString &message);
};

class ProtocolParsingInput : public ProtocolParsing
{
	Q_OBJECT
public:
	ProtocolParsingInput(QIODevice * device);
	friend class ProtocolParsing;
protected:
	virtual void parseIncommingData();
	virtual void parseMessage(const quint8 &mainCodeType,const QByteArray &data) = 0;
	virtual void parseMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data) = 0;
	virtual void parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data) = 0;
	virtual void parseQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data) = 0;
	// for data
	bool haveData;
	bool haveData_dataSize;
	QByteArray data_size;
	quint32 dataSize;
	QByteArray data;
	//to parse the netwrok stream
	quint8 mainCodeType;
	quint16 subCodeType;
	quint8 queryNumber;
	bool have_subCodeType,need_subCodeType,need_query_number;
	// function
	void dataClear();
	//temp data
	static quint8 temp_size_8Bits;
	static quint16 temp_size_16Bits;
	static quint32 temp_size_32Bits;
	/********************** static *********************/
	//connexion parameters
	static QSet<quint8> mainCodeWithoutSubCodeType;//if need sub code or not
	//if is a query
	static QSet<quint8> mainCode_IsQuery;
	//predefined size
	static QHash<quint8,quint16> sizeOnlyMainCodePacket;
	static QHash<quint8,QHash<quint16,quint16> > sizeMultipleCodePacket;

};

class ProtocolParsingOutput : public ProtocolParsing
{
	Q_OBJECT
public:
	ProtocolParsingOutput(QIODevice * device);
	friend class ProtocolParsing;
protected:
	virtual bool packOutcommingData(const quint8 &mainCodeType,const QByteArray &data);
	virtual bool packOutcommingData(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data);
	virtual bool packOutcommingData(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data);
	virtual bool packOutcommingData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data);
private:
	bool internalPackOutcommingData(const quint8 &mainCodeType,const QByteArray &data);
	//temp data
	static qint64 byteWriten;
	/********************** static *********************/
	//connexion parameters
	static QSet<quint8> mainCodeWithoutSubCodeType;//if need sub code or not
	//if is a query
	static QSet<quint8> mainCode_IsQuery;
	//predefined size
	static QHash<quint8,quint16> sizeOnlyMainCodePacket;
	static QHash<quint8,QHash<quint16,quint16> > sizeMultipleCodePacket;
signals:
	void fake_send_data(const QByteArray &data);
};

#endif // PROTOCOLPARSING_H
