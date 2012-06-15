#include "QFakeSocket.h"
#include "QFakeServer.h"

#include <QTcpSocket>

QFakeSocket::QFakeSocket(QObject *parent) :
	QAbstractSocket(QAbstractSocket::UnknownSocketType,parent)
{
	theOtherSocket=NULL;
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
	{
		QMutexLocker locker(&mutex);
		data.clear();
	}
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

bool	QFakeSocket::atEnd ()
{
	QMutexLocker locker(&mutex);
	return data.size()==0;
}

qint64	QFakeSocket::bytesAvailable ()
{
	QMutexLocker locker(&mutex);
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

qint64	QFakeSocket::readData(char * data, qint64 maxSize)
{
	QMutexLocker locker(&mutex);
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
	if(theOtherSocket==NULL)
		return 0;
	QByteArray dataToSend(data,size);
	theOtherSocket->internal_writeData(dataToSend);
	return size;
}

void	QFakeSocket::internal_writeData(QByteArray data)
{
	{
		QMutexLocker locker(&mutex);
		this->data+=data;
	}
	emit readyRead();
}
