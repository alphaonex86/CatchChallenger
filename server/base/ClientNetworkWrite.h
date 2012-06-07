#ifndef CLIENTNETWORKWRITE_H
#define CLIENTNETWORKWRITE_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>

#include "ServerStructures.h"
#include "../general/base/DebugClass.h"
#include "../general/base/ProtocolParsing.h"
#include "../general/base/GeneralVariable.h"
#include "../VariableServer.h"

class ClientNetworkWrite : public ProtocolParsingOutput
{
    Q_OBJECT
public:
	explicit ClientNetworkWrite(GeneralData *generalData,QTcpSocket * socket);
	~ClientNetworkWrite();
public slots:
	void sendPacket(const quint8 &mainIdent,const quint16 &subIdent,const bool &packetSize,const QByteArray &data);
	//normal slots
	void askIfIsReadyToStop();
	void stop();
private:
	QTcpSocket * socket;
	QByteArray block;
	qint64 byteWriten;
	GeneralData *generalData;
signals:
	void isReadyToStop();
};

#endif // CLIENTNETWORKWRITE_H
