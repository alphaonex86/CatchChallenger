#include "ClientNetworkWrite.h"

ClientNetworkWrite::ClientNetworkWrite()
{
}

ClientNetworkWrite::~ClientNetworkWrite()
{
}

void ClientNetworkWrite::setSocket(QTcpSocket * socket)
{
	this->socket=socket;
}

/* not use mainCodeWithoutSubCodeTypeServerToClient because the reply have unknow code */
void ClientNetworkWrite::sendPacket(const quint8 &mainIdent,const quint16 &subIdent,const bool &packetSize,const QByteArray &data)
{
	//can't group this block to keep the right header size calculation
	QDataStream out(&block, QIODevice::WriteOnly);
	out << qint32(mainIdent);
	if(!generalData->mainCodeWithoutSubCodeTypeServerToClient.contains(mainIdent))
		out << qint32(subIdent);
	if(packetSize)
		out << qint32(data.size()+sizeof(qint32));

	#ifdef POKECRAFT_SERVER_EXTRA_CHECK
	if(!generalData->mainCodeWithoutSubCodeTypeServerToClient.contains(mainIdent))
	{
		if(generalData->sizeMultipleCodePacketServerToClient.contains(mainIdent))
		{
			if(generalData->sizeMultipleCodePacketServerToClient[mainIdent].contains(subIdent))
			{
				if(generalData->sizeMultipleCodePacketServerToClient[mainIdent][subIdent]!=data.size())
				{
					emit error("Data size is not valid");
					return;
				}
			}
			else
			{
				if(!packetSize)
				{
					emit error("Packet size is not fix");
					return;
				}
			}
		}
		else
		{
			if(!packetSize)
			{
				emit error("Packet size is not fix");
				return;
			}
		}
	}
	else
	{
		if(generalData->sizeOnlyMainCodePacketClientToServer.contains(mainIdent))
		{
			if(generalData->sizeOnlyMainCodePacketClientToServer[mainIdent]!=data.size())
			{
				emit error("Data size is not valid");
				return;
			}
		}
		else
		{
			if(!packetSize && mainIdent!=0xC1)
			{
				emit error("Packet size is not fix");
				return;
			}
		}
	}
	#endif

	block+=data;

	if(socket!=NULL)
	{
		#ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
		emit message(QString("Sended packet size: %1").arg(data.size()+sizeof(qint32)));
		#endif // DEBUG_MESSAGE_CLIENT_RAW_NETWORK
		//emit message(QString("data: %1").arg(QString(block.toHex())));
		byteWriten = socket->write(block);
		if(!socket->isValid())
		{
			emit error("Socket is not valid");
			return;
		}
		if(block.size()!=byteWriten)
		{
			emit error(QString("All the bytes have not be written: %1").arg(socket->errorString()));
			return;
		}
	}
	else
		emit fake_send_data(block);
	block.clear();
}

void ClientNetworkWrite::askIfIsReadyToStop()
{
	emit isReadyToStop();
}

void ClientNetworkWrite::stop()
{
	deleteLater();
}

void ClientNetworkWrite::setVariable(GeneralData *generalData)
{
	this->generalData=generalData;
}
