#if ! defined(EPOLLCATCHCHALLENGERSERVER) && ! defined (ONLYMAPRENDER) && defined(CATCHCHALLENGER_SOLO)
#include "QFakeSocket.hpp"
#include "QFakeServer.hpp"
#include "ClientVariable.hpp"

#include <QMutexLocker>
using namespace CatchChallenger;

QFakeSocket::QFakeSocket()
{
    theOtherSocket=NULL;
    RX_size=0;
    open(QIODevice::ReadWrite|QIODevice::Unbuffered);
}

QFakeSocket::~QFakeSocket()
{
    if(theOtherSocket!=NULL)
    {
        QFakeSocket *tempOtherSocket=theOtherSocket;
        theOtherSocket=NULL;
        tempOtherSocket->theOtherSocket=NULL;
        tempOtherSocket->stateChanged(QAbstractSocket::UnconnectedState);
        tempOtherSocket->disconnected();
    }
    emit aboutToDelete();
}

void QFakeSocket::abort()
{
    #ifdef FAKESOCKETDEBUG
    qDebug() << (std::stringLiteral("QFakeSocket::abort()"));
    #endif
    disconnectFromHost();
}

void QFakeSocket::disconnectFromHost()
{
    if(theOtherSocket==NULL)
        return;
    #ifdef FAKESOCKETDEBUG
    qDebug() << (std::stringLiteral("QFakeSocket::disconnectFromHost()"));
    #endif
    QFakeSocket *tempOtherSocket=theOtherSocket;
    theOtherSocket=NULL;
    tempOtherSocket->disconnectFromHost();
    {
        QMutexLocker lock(&mutex);
        data.clear();
    }
    emit stateChanged(QAbstractSocket::UnconnectedState);
    emit disconnected();
}

void QFakeSocket::disconnectFromFakeServer()
{
    if(theOtherSocket==NULL)
        return;
    #ifdef FAKESOCKETDEBUG
    qDebug() << (std::stringLiteral("QFakeSocket::disconnectFromFakeServer()"));
    #endif
    theOtherSocket=NULL;
    {
        QMutexLocker lock(&mutex);
        data.clear();
    }
    emit stateChanged(QAbstractSocket::UnconnectedState);
    emit disconnected();
}

void QFakeSocket::connectToHost()
{
    #ifdef FAKESOCKETDEBUG
    qDebug() << (std::stringLiteral("QFakeSocket::connectToHost()"));
    #endif
    if(theOtherSocket!=NULL)
        return;
    QFakeServer::server.addPendingConnection(this);
    emit stateChanged(QAbstractSocket::ConnectedState);
    emit connected();
}

qint64	QFakeSocket::bytesAvailableWithMutex()
{
    qint64 size;
    {
        QMutexLocker lock(&mutex);
        #ifdef FAKESOCKETDEBUG
        qDebug() << (std::stringLiteral("bytesAvailable(): data.size(): %1").arg(data.size()));
        #endif
        size=data.size();
    }
    return size;
}

qint64 QFakeSocket::bytesAvailable() const
{
    return const_cast<QFakeSocket *>(this)->bytesAvailableWithMutex();
}

void	QFakeSocket::close()
{
    disconnectFromHost();
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

QAbstractSocket::SocketError QFakeSocket::error() const
{
    return QAbstractSocket::UnknownSocketError;
}

QAbstractSocket::SocketState QFakeSocket::state() const
{
    if(theOtherSocket==NULL)
        return QAbstractSocket::UnconnectedState;
    else
        return QAbstractSocket::ConnectedState;
}

qint64 QFakeSocket::readData(char * rawData, qint64 maxSize)
{
    QMutexLocker lock(&mutex);
    QByteArray extractedData=this->data.mid(0,static_cast<int>(maxSize));
    #ifdef FAKESOCKETDEBUG
    qDebug() << (std::stringLiteral("readData(): extractedData.size(): %1, data.size(): %2, extractedData: %3").arg(extractedData.size()).arg(data.size()).arg(std::string(extractedData.toHex())));
    #endif
    memcpy(rawData,extractedData.data(),extractedData.size());
    this->data.remove(0,extractedData.size());
    return extractedData.size();
}

qint64	QFakeSocket::writeData(const char * rawData, qint64 size)
{
    if(theOtherSocket==NULL)
    {
        qDebug() << ("writeData(): theOtherSocket==NULL");
        emit error(QAbstractSocket::NetworkError);
        return size;
    }
    QByteArray dataToSend;
    {
        QMutexLocker lock(&mutex);
        #ifdef FAKESOCKETDEBUG
        qDebug() << (std::stringLiteral("writeData(): size: %1").arg(size));
        #endif
        dataToSend=QByteArray(rawData,static_cast<int>(size));
    }
    theOtherSocket->internal_writeData(dataToSend);
    return size;
}

void QFakeSocket::internal_writeData(QByteArray rawData)
{
    {
        QMutexLocker lock(&mutex);
        #ifdef FAKESOCKETDEBUG
        qDebug() << (std::stringLiteral("internal_writeData(): size: %1").arg(rawData.size()));
        #endif
        RX_size+=rawData.size();
        this->data+=rawData;
    }
    emit readyRead();
}

bool QFakeSocket::isSequential() const
{
    return true;
}

bool QFakeSocket::canReadLine () const
{
    return false;
}
#endif
