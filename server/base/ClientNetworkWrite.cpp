#include "ClientNetworkWrite.h"

ClientNetworkWrite::ClientNetworkWrite(GeneralData *generalData,QTcpSocket * socket) :
	ProtocolParsingOutput(socket)
{
	this->generalData=generalData;
	this->socket=socket;
}

ClientNetworkWrite::~ClientNetworkWrite()
{
}

/* not use mainCodeWithoutSubCodeTypeServerToClient because the reply have unknow code */
void ClientNetworkWrite::sendPacket(const quint8 &mainCodeType,const quint16 &subCodeType,const bool &packetSize,const QByteArray &data)
{
	if(!ProtocolParsingOutput::packOutcommingData(mainCodeType,subCodeType,packetSize,data))
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

