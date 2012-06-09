#include "ClientNetworkWrite.h"

ClientNetworkWrite::ClientNetworkWrite(QTcpSocket * socket) :
	ProtocolParsingOutput(socket)
{
	this->socket=socket;
}

ClientNetworkWrite::~ClientNetworkWrite()
{
}

/* not use mainCodeWithoutSubCodeTypeServerToClient because the reply have unknow code */
void ClientNetworkWrite::sendPacket(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data)
{
	if(!ProtocolParsingOutput::packOutcommingData(mainCodeType,subCodeType,data))
		return;
	if(!socket->isValid())
	{
		emit error("device is not valid");
		return;
	}
}

void ClientNetworkWrite::sendPacket(const quint8 &mainCodeType,const QByteArray &data)
{
	if(!ProtocolParsingOutput::packOutcommingData(mainCodeType,data))
		return;
	if(!socket->isValid())
	{
		emit error("device is not valid");
		return;
	}
}

//send reply
void ClientNetworkWrite::postReply(const quint8 &queryNumber,const QByteArray &data)
{
	if(!ProtocolParsingOutput::postReplyData(queryNumber,data))
		return;
	if(!socket->isValid())
	{
		emit error("device is not valid");
		return;
	}
}

void ClientNetworkWrite::askIfIsReadyToStop()
{
	emit isReadyToStop();
}

void ClientNetworkWrite::stop()
{
	deleteLater();
}

