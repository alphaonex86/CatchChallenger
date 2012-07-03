#include "QFakeSocket.h"
#include "QFakeServer.h"
#include "DebugClass.h"

QFakeSocket::QFakeSocket() :
	QAbstractSocket(QAbstractSocket::UnknownSocketType,0)
{
	theOtherSocket=NULL;
	RX_size=0;
}

void QFakeSocket::abort()
{
}

void QFakeSocket::disconnectFromHost()
{
	if(theOtherSocket==NULL)
		return;
	theOtherSocket->disconnect();
	theOtherSocket=NULL;
	data.clear();
	emit stateChanged(QAbstractSocket::UnconnectedState);
	emit disconnected();
}

void QFakeSocket::connectToHost()
{
	if(theOtherSocket!=NULL)
		return;
	QFakeServer::server.addPendingConnection(this);
	emit stateChanged(QAbstractSocket::ConnectedState);
	emit connected();
}

qint64	QFakeSocket::bytesAvailable () const
{
	return data.size();
}

qint64	QFakeSocket::bytesToWrite () const
{
	return 0;
}

bool	QFakeSocket::canReadLine () const
{
	return false;
}

void	QFakeSocket::close ()
{
	disconnectFromHost();
}

bool	QFakeSocket::isSequential () const
{
	return true;
}

bool	QFakeSocket::waitForBytesWritten ( int msecs )
{
	Q_UNUSED(msecs);
	return true;
}

bool	QFakeSocket::waitForReadyRead ( int msecs )
{
	Q_UNUSED(msecs);
	return true;
}

bool QFakeSocket::isValid()
{
	return theOtherSocket!=NULL;
}

QFakeSocket * QFakeSocket::getTheOtherSocket()
{
	return theOtherSocket;
}

quint64 QFakeSocket::getRXSize()
{
	return RX_size;
}

quint64 QFakeSocket::getTXSize()
{
	return theOtherSocket->getRXSize();
}

qint64	QFakeSocket::readData(char * data, qint64 maxSize)
{
	QByteArray extractedData=this->data.mid(0,maxSize);
	memcpy(data,extractedData.data(),extractedData.size());
	return extractedData.size();
}

qint64	QFakeSocket::readLineData ( char * data, qint64 maxlen )
{
	Q_UNUSED(data);
	Q_UNUSED(maxlen);
	return 0;
}

qint64	QFakeSocket::writeData ( const char * data, qint64 size )
{
	DebugClass::debugConsole(QString("writeData(): size: %1").arg(size));
	if(theOtherSocket==NULL)
	{
		DebugClass::debugConsole("theOtherSocket==NULL");
		return 0;
	}
	QByteArray dataToSend(data,size);
	theOtherSocket->internal_writeData(dataToSend);
	return size;
}

void QFakeSocket::internal_writeData(QByteArray data)
{
	RX_size+=data.size();
	this->data+=data;
	emit readyRead();
}
