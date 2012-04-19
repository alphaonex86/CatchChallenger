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

void ClientNetworkWrite::sendPacket(const QByteArray &data)
{
	//can't group this block to keep the right header size calculation
	QDataStream out(&block, QIODevice::WriteOnly);
	out << qint32(data.size()+sizeof(qint32));
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
