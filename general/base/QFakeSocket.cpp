#include "QFakeSocket.h"
#include "QFakeServer.h"
#include "DebugClass.h"
#include "GeneralVariable.h"

#include <QMutexLocker>

QFakeSocket::QFakeSocket() :
	QAbstractSocket(QAbstractSocket::UnknownSocketType,0)
{
	setSocketError(QAbstractSocket::UnknownSocketError);
	setSocketState(QAbstractSocket::UnconnectedState);
	theOtherSocket=NULL;
	RX_size=0;
}

void QFakeSocket::abort()
{
	#ifdef FAKESOCKETDEBUG
	DebugClass::debugConsole(QString("disconnectFromHostImplementation()"));
	#endif
}

void QFakeSocket::disconnectFromHostImplementation()
{
	#ifdef FAKESOCKETDEBUG
	DebugClass::debugConsole(QString("disconnectFromHostImplementation()"));
	#endif
	if(theOtherSocket==NULL)
		return;
	QFakeSocket *tempOtherSocket=theOtherSocket;
	theOtherSocket=NULL;
	{
		QMutexLocker lock(&mutex);
		data.clear();
	}
	setSocketState(QAbstractSocket::UnconnectedState);
	tempOtherSocket->disconnectFromHost();
	emit stateChanged(QAbstractSocket::UnconnectedState);
	emit disconnected();
}

void QFakeSocket::connectToHostImplementation()
{
	#ifdef FAKESOCKETDEBUG
	DebugClass::debugConsole(QString("connectToHostImplementation()"));
	#endif
	if(theOtherSocket!=NULL)
		return;
	QFakeServer::server.addPendingConnection(this);
	setSocketState(QAbstractSocket::ConnectedState);
	emit stateChanged(QAbstractSocket::ConnectedState);
	emit connected();
}

qint64	QFakeSocket::bytesAvailableWithMutex()
{
	QMutexLocker lock(&mutex);
	#ifdef FAKESOCKETDEBUG
	DebugClass::debugConsole(QString("bytesAvailable(): data.size(): %1").arg(data.size()));
	#endif
	return data.size();
}

qint64	QFakeSocket::bytesAvailable () const
{
	return const_cast<QFakeSocket *>(this)->bytesAvailableWithMutex();
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
	if(theOtherSocket==NULL)
		return 0;
	return theOtherSocket->getRXSize();
}

qint64	QFakeSocket::readData(char * rawData, qint64 maxSize)
{
	QMutexLocker lock(&mutex);
	QByteArray extractedData=this->data.mid(0,maxSize);
	#ifdef FAKESOCKETDEBUG
	DebugClass::debugConsole(QString("readData(): extractedData.size(): %1, this->data: %2, extractedData: %3").arg(extractedData.size()).arg(this->data.size()).arg(QString(extractedData.toHex())));
	#endif
	memcpy(rawData,extractedData.data(),extractedData.size());
	this->data.remove(0,extractedData.size());
	return extractedData.size();
}

qint64	QFakeSocket::writeData ( const char * rawData, qint64 size )
{
	QMutexLocker lock(&mutex);
	#ifdef FAKESOCKETDEBUG
	DebugClass::debugConsole(QString("writeData(): size: %1").arg(size));
	#endif
	if(theOtherSocket==NULL)
	{
		DebugClass::debugConsole("writeData(): theOtherSocket==NULL");
		return 0;
	}
	QByteArray dataToSend(rawData,size);
	theOtherSocket->internal_writeData(dataToSend);
	return size;
}

void QFakeSocket::internal_writeData(QByteArray rawData)
{
	{
		QMutexLocker lock(&mutex);
		#ifdef FAKESOCKETDEBUG
		DebugClass::debugConsole(QString("internal_writeData(): size: %1").arg(rawData.size()));
		#endif
		RX_size+=rawData.size();
		this->data+=rawData;
	}
	emit readyRead();
}
