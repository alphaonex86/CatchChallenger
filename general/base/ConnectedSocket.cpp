#include "ConnectedSocket.h"
#include <QSslSocket>
#include <unistd.h>
#include <iostream>
#include <fcntl.h>

using namespace CatchChallenger;

#ifndef EPOLLCATCHCHALLENGERSERVER
ConnectedSocket::ConnectedSocket(QFakeSocket *socket) :
    #ifdef EPOLLCATCHCHALLENGERSERVER
    epollSocket(NULL),
    #endif
    fakeSocket(socket),
    sslSocket(NULL),
    tcpSocket(NULL)
{
    connect(socket,&QFakeSocket::destroyed,     this,&ConnectedSocket::destroyedSocket,Qt::DirectConnection);
    connect(socket,&QFakeSocket::connected,     this,&ConnectedSocket::connected,Qt::QueuedConnection);
    connect(socket,&QFakeSocket::disconnected,  this,&ConnectedSocket::disconnected,Qt::QueuedConnection);
    connect(socket,static_cast<void(QFakeSocket::*)(QAbstractSocket::SocketError)>(&QFakeSocket::error),this,static_cast<void(ConnectedSocket::*)(QAbstractSocket::SocketError)>(&ConnectedSocket::error));
    connect(socket,&QFakeSocket::stateChanged,   this,&ConnectedSocket::stateChanged,Qt::QueuedConnection);
    connect(socket,&QFakeSocket::readyRead,     this,&ConnectedSocket::readyRead,Qt::DirectConnection);
    open(QIODevice::ReadWrite|QIODevice::Unbuffered);
}

ConnectedSocket::ConnectedSocket(QSslSocket *socket) :
    #ifdef EPOLLCATCHCHALLENGERSERVER
    epollSocket(NULL),
    #endif
    fakeSocket(NULL),
    sslSocket(socket),
    tcpSocket(NULL)
{
    socket->setSocketOption(QAbstractSocket::KeepAliveOption,1);
    connect(socket,&QSslSocket::readyRead,      this,&ConnectedSocket::readyRead,Qt::DirectConnection);
    connect(socket,&QSslSocket::encrypted,      this,&ConnectedSocket::purgeBuffer,Qt::QueuedConnection);
    connect(socket,&QSslSocket::connected,      this,&ConnectedSocket::connected,Qt::QueuedConnection);
    connect(socket,&QSslSocket::destroyed,      this,&ConnectedSocket::destroyedSocket,Qt::DirectConnection);
    //connect(socket,&QSslSocket::connected,      this,&ConnectedSocket::startHandshake,Qt::QueuedConnection);
    connect(socket,&QSslSocket::disconnected,   this,&ConnectedSocket::disconnected,Qt::QueuedConnection);
    connect(socket,static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),this,static_cast<void(ConnectedSocket::*)(QAbstractSocket::SocketError)>(&ConnectedSocket::error));
    connect(socket,&QSslSocket::stateChanged,   this,&ConnectedSocket::stateChanged,Qt::QueuedConnection);
    connect(socket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),this,static_cast<void(ConnectedSocket::*)(const QList<QSslError> &errors)>(&ConnectedSocket::sslErrors),Qt::QueuedConnection);
    purgeBuffer();
    open(QIODevice::ReadWrite|QIODevice::Unbuffered);
}

ConnectedSocket::ConnectedSocket(QTcpSocket *socket) :
    #ifdef EPOLLCATCHCHALLENGERSERVER
    epollSocket(NULL),
    #endif
    fakeSocket(NULL),
    sslSocket(NULL),
    tcpSocket(socket)
{
    socket->setSocketOption(QAbstractSocket::KeepAliveOption,1);
    connect(socket,&QTcpSocket::readyRead,      this,&ConnectedSocket::readyRead,Qt::DirectConnection);
    connect(socket,&QTcpSocket::connected,      this,&ConnectedSocket::connected,Qt::QueuedConnection);
    connect(socket,&QTcpSocket::destroyed,      this,&ConnectedSocket::destroyedSocket,Qt::DirectConnection);
    //connect(socket,&QTcpSocket::connected,      this,&ConnectedSocket::startHandshake,Qt::QueuedConnection);
    connect(socket,&QTcpSocket::disconnected,   this,&ConnectedSocket::disconnected,Qt::QueuedConnection);
    connect(socket,static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),this,static_cast<void(ConnectedSocket::*)(QAbstractSocket::SocketError)>(&ConnectedSocket::error));
    connect(socket,&QTcpSocket::stateChanged,   this,&ConnectedSocket::stateChanged,Qt::QueuedConnection);
    open(QIODevice::ReadWrite|QIODevice::Unbuffered);
}
#endif

#ifdef EPOLLCATCHCHALLENGERSERVER
#ifndef SERVERNOSSL
ConnectedSocket::ConnectedSocket(EpollSslClient *socket) :
#else
ConnectedSocket::ConnectedSocket(EpollClient *socket) :
#endif
    epollSocket(socket)
{
    open(QIODevice::ReadWrite|QIODevice::Unbuffered);
}
#endif

ConnectedSocket::~ConnectedSocket()
{
    #ifdef EPOLLCATCHCHALLENGERSERVER
    if(epollSocket!=NULL)
        delete epollSocket;
    #else
    if(sslSocket!=NULL)
        delete sslSocket;
    if(tcpSocket!=NULL)
        delete tcpSocket;
    if(fakeSocket!=NULL)
        delete fakeSocket;
    #endif
}

QList<QSslError> ConnectedSocket::sslErrors() const
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(sslSocket!=NULL)
        return sslSocket->sslErrors();
    else
    #endif
        return QList<QSslError>();
}

void ConnectedSocket::purgeBuffer()
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(sslSocket!=NULL)
    {
        if(sslSocket->bytesAvailable())
            emit readyRead();
        if(sslSocket->encryptedBytesAvailable())
            emit readyRead();
    }
    #endif
}

void ConnectedSocket::destroyedSocket()
{
    #ifdef EPOLLCATCHCHALLENGERSERVER
    epollSocket=NULL;
    #else
    sslSocket=NULL;
    tcpSocket=NULL;
    fakeSocket=NULL;
    #endif
}

void ConnectedSocket::abort()
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(fakeSocket!=NULL)
        fakeSocket->abort();
    if(sslSocket!=NULL)
        sslSocket->abort();
    if(tcpSocket!=NULL)
        tcpSocket->abort();
    #endif
}

#ifndef EPOLLCATCHCHALLENGERSERVER
void ConnectedSocket::connectToHost(const QString & hostName, quint16 port)
{
    if(state()!=QAbstractSocket::UnconnectedState)
        return;
    if(fakeSocket!=NULL)
        fakeSocket->connectToHost();
    else if(sslSocket!=NULL)
        sslSocket->connectToHost(hostName,port);
    else if(tcpSocket!=NULL)
        tcpSocket->connectToHost(hostName,port);
}

void ConnectedSocket::connectToHost(const QHostAddress & address, quint16 port)
{
    if(state()!=QAbstractSocket::UnconnectedState)
        return;
    if(fakeSocket!=NULL)
        fakeSocket->connectToHost();
    else if(sslSocket!=NULL)
        sslSocket->connectToHost(address.toString(),port);
    else if(tcpSocket!=NULL)
        tcpSocket->connectToHost(address,port);
}
#endif

void ConnectedSocket::disconnectFromHost()
{
    if(state()!=QAbstractSocket::ConnectedState)
        return;
    #ifdef EPOLLCATCHCHALLENGERSERVER
    if(epollSocket!=NULL)
        epollSocket->close();
    #else
    if(fakeSocket!=NULL)
        fakeSocket->disconnectFromHost();
    if(sslSocket!=NULL)
        sslSocket->disconnectFromHost();
    if(tcpSocket!=NULL)
        tcpSocket->disconnectFromHost();
    #endif
}

QAbstractSocket::SocketError ConnectedSocket::error() const
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(fakeSocket!=NULL)
        return fakeSocket->error();
    if(sslSocket!=NULL)
        return sslSocket->error();
    if(tcpSocket!=NULL)
        return tcpSocket->error();
    #endif
    return QAbstractSocket::UnknownSocketError;
}

bool ConnectedSocket::flush()
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(fakeSocket!=NULL)
        return true;
    if(sslSocket!=NULL)
        return sslSocket->flush();
    if(tcpSocket!=NULL)
        return tcpSocket->flush();
    #endif
    return false;
}

bool ConnectedSocket::isValid() const
{
    #ifdef EPOLLCATCHCHALLENGERSERVER
    if(epollSocket!=NULL)
        return epollSocket->isValid();
    #else
    if(fakeSocket!=NULL)
        return fakeSocket->isValid();
    else if(sslSocket!=NULL)
        return sslSocket->isValid();
    else if(tcpSocket!=NULL)
        return tcpSocket->isValid();
    #endif
    return false;
}

QHostAddress ConnectedSocket::localAddress() const
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(fakeSocket!=NULL)
        return QHostAddress::LocalHost;
    if(sslSocket!=NULL)
        return sslSocket->localAddress();
    if(tcpSocket!=NULL)
        return tcpSocket->localAddress();
    #endif
    return QHostAddress::Null;
}

quint16	ConnectedSocket::localPort() const
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(fakeSocket!=NULL)
        return 9999;
    if(sslSocket!=NULL)
        return sslSocket->localPort();
    if(tcpSocket!=NULL)
        return tcpSocket->localPort();
    #endif
    return 0;
}

QHostAddress	ConnectedSocket::peerAddress() const
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(fakeSocket!=NULL)
        return QHostAddress::LocalHost;
    if(sslSocket!=NULL)
        return sslSocket->peerAddress();
    if(tcpSocket!=NULL)
        return tcpSocket->peerAddress();
    #endif
    return QHostAddress::Null;
}

QString	ConnectedSocket::peerName() const
{
    return peerAddress().toString();
}

quint16	ConnectedSocket::peerPort() const
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(fakeSocket!=NULL)
        return 15000;
    if(sslSocket!=NULL)
        return sslSocket->peerPort();
    if(tcpSocket!=NULL)
        return tcpSocket->peerPort();
    #endif
    return 0;
}

QAbstractSocket::SocketState ConnectedSocket::state() const
{
    #ifdef EPOLLCATCHCHALLENGERSERVER
    if(epollSocket!=NULL)
    {
        if(epollSocket->isValid())
            return QAbstractSocket::ConnectedState;
        else
            return QAbstractSocket::UnconnectedState;
    }
    #else
    if(fakeSocket!=NULL)
        return fakeSocket->state();
    if(sslSocket!=NULL)
        return sslSocket->state();
    if(tcpSocket!=NULL)
        return tcpSocket->state();
    #endif
    return QAbstractSocket::UnconnectedState;
}

bool ConnectedSocket::waitForConnected(int msecs)
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(fakeSocket!=NULL)
        return true;
    if(sslSocket!=NULL)
        return sslSocket->waitForConnected(msecs);
    if(tcpSocket!=NULL)
        return tcpSocket->waitForConnected(msecs);
    #else
    Q_UNUSED(msecs);
    #endif
    return false;
}

bool ConnectedSocket::waitForDisconnected(int msecs)
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(fakeSocket!=NULL)
        return true;
    if(sslSocket!=NULL)
        return sslSocket->waitForDisconnected(msecs);
    if(tcpSocket!=NULL)
        return tcpSocket->waitForDisconnected(msecs);
    #else
    Q_UNUSED(msecs);
    #endif
    return false;
}

qint64 ConnectedSocket::bytesAvailable() const
{
    #ifdef EPOLLCATCHCHALLENGERSERVER
    if(epollSocket!=NULL)
        return epollSocket->bytesAvailable();
    #else
    if(fakeSocket!=NULL)
        return fakeSocket->bytesAvailable();
    if(tcpSocket!=NULL)
        return tcpSocket->bytesAvailable();
    if(sslSocket!=NULL)
        return sslSocket->bytesAvailable();
        else
    #endif
    return -1;
}

QIODevice::OpenMode ConnectedSocket::openMode() const
{
    #ifdef EPOLLCATCHCHALLENGERSERVER
    if(epollSocket!=NULL)
        return QIODevice::ReadWrite;
    #else
    if(fakeSocket!=NULL)
        return fakeSocket->openMode();
    if(sslSocket!=NULL)
        return sslSocket->openMode();
    if(tcpSocket!=NULL)
        return tcpSocket->openMode();
    #endif
    return QIODevice::NotOpen;
}

QString ConnectedSocket::errorString() const
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(fakeSocket!=NULL)
        return fakeSocket->errorString();
    if(sslSocket!=NULL)
        return sslSocket->errorString();
    if(tcpSocket!=NULL)
        return tcpSocket->errorString();
    #endif
    return QString();
}

void ConnectedSocket::close()
{
    #ifdef EPOLLCATCHCHALLENGERSERVER
    disconnectFromHost();
    #else
    abort();
    #endif
}

qint64 ConnectedSocket::readData(char * data, qint64 maxSize)
{
    #ifdef EPOLLCATCHCHALLENGERSERVER
    if(epollSocket!=NULL)
        return epollSocket->read(data,maxSize);
    #else
    if(fakeSocket!=NULL)
        return fakeSocket->read(data,maxSize);
    if(tcpSocket!=NULL)
        return tcpSocket->read(data,maxSize);
    if(sslSocket!=NULL)
        return sslSocket->read(data,maxSize);
    #endif
    return -1;
}

qint64 ConnectedSocket::writeData(const char * data, qint64 maxSize)
{
    #ifdef EPOLLCATCHCHALLENGERSERVER
    if(epollSocket!=NULL)
        return epollSocket->write(const_cast<char *>(data),maxSize);
    #else
    if(fakeSocket!=NULL)
        return fakeSocket->write(data,maxSize);
    if(tcpSocket!=NULL)
        return tcpSocket->write(data,maxSize);
    if(sslSocket!=NULL)
        return sslSocket->write(data,maxSize);
    #endif
    return -1;
}

bool ConnectedSocket::isSequential() const
{
    return true;
}

bool ConnectedSocket::canReadLine () const
{
    return false;
}
