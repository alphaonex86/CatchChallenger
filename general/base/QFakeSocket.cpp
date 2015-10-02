#ifndef EPOLLCATCHCHALLENGERSERVER
#include "QFakeSocket.h"
#include "QFakeServer.h"
#include "DebugClass.h"
#include "GeneralVariable.h"

#include <QMutexLocker>
#include <QThread>
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
    DebugClass::debugConsole(QStringLiteral("QFakeSocket::abort()"));
    #endif
    disconnectFromHost();
}

void QFakeSocket::disconnectFromHost()
{
    if(theOtherSocket==NULL)
        return;
    #ifdef FAKESOCKETDEBUG
    DebugClass::debugConsole(QStringLiteral("QFakeSocket::disconnectFromHost()"));
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
    DebugClass::debugConsole(QStringLiteral("QFakeSocket::disconnectFromFakeServer()"));
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
    DebugClass::debugConsole(QStringLiteral("QFakeSocket::connectToHost()"));
    #endif
    if(theOtherSocket!=NULL)
        return;
    QFakeServer::server.addPendingConnection(this);
    emit stateChanged(QAbstractSocket::ConnectedState);
    emit connected();
}

int64_t	QFakeSocket::bytesAvailableWithMutex()
{
    int64_t size;
    {
        QMutexLocker lock(&mutex);
        #ifdef FAKESOCKETDEBUG
        DebugClass::debugConsole(QStringLiteral("bytesAvailable(): data.size(): %1").arg(data.size()));
        #endif
        size=data.size();
    }
    return size;
}

int64_t	QFakeSocket::bytesAvailable() const
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

uint64_t QFakeSocket::getRXSize()
{
    return RX_size;
}

uint64_t QFakeSocket::getTXSize()
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

int64_t	QFakeSocket::readData(char * rawData, int64_t maxSize)
{
    QMutexLocker lock(&mutex);
    std::vector<char> extractedData=this->data.mid(0,maxSize);
    #ifdef FAKESOCKETDEBUG
    DebugClass::debugConsole(QStringLiteral("readData(): extractedData.size(): %1, data.size(): %2, extractedData: %3").arg(extractedData.size()).arg(data.size()).arg(QString(extractedData.toHex())));
    #endif
    memcpy(rawData,extractedData.data(),extractedData.size());
    this->data.remove(0,extractedData.size());
    return extractedData.size();
}

int64_t	QFakeSocket::writeData(const char * rawData, int64_t size)
{
    if(theOtherSocket==NULL)
    {
        DebugClass::debugConsole("writeData(): theOtherSocket==NULL");
        emit error(QAbstractSocket::NetworkError);
        return size;
    }
    std::vector<char> dataToSend;
    {
        QMutexLocker lock(&mutex);
        #ifdef FAKESOCKETDEBUG
        DebugClass::debugConsole(QStringLiteral("writeData(): size: %1").arg(size));
        #endif
        dataToSend=std::vector<char>(rawData,size);
    }
    theOtherSocket->internal_writeData(dataToSend);
    return size;
}

void QFakeSocket::internal_writeData(std::vector<char> rawData)
{
    {
        QMutexLocker lock(&mutex);
        #ifdef FAKESOCKETDEBUG
        DebugClass::debugConsole(QStringLiteral("internal_writeData(): size: %1").arg(rawData.size()));
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
