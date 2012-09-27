#ifndef POKECRAFT_CLIENTNETWORKWRITE_H
#define POKECRAFT_CLIENTNETWORKWRITE_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>

#include "ServerStructures.h"
#include "../../general/base/DebugClass.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/GeneralVariable.h"
#include "../VariableServer.h"

namespace Pokecraft {
class ClientNetworkWrite : public ProtocolParsingOutput
{
    Q_OBJECT
public:
	explicit ClientNetworkWrite(QAbstractSocket * socket);
	~ClientNetworkWrite();
public slots:
	void sendPacket(const quint8 &mainIdent,const quint16 &subIdent,const QByteArray &data);
	void sendPacket(const quint8 &mainIdent,const QByteArray &data);
	//send reply
	void postReply(const quint8 &queryNumber,const QByteArray &data);
	//normal slots
	void askIfIsReadyToStop();
	void stop();
private:
	QAbstractSocket * socket;
signals:
	void isReadyToStop();
};
}

#endif // CLIENTNETWORKWRITE_H
