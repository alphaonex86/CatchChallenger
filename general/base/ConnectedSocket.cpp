#if ! defined(EPOLLCATCHCHALLENGERSERVER) && ! defined (ONLYMAPRENDER)

#include "ConnectedSocket.h"
#include <QSslSocket>
#include <unistd.h>
#include <iostream>
#include <fcntl.h>

#ifdef __linux__
#include <netinet/tcp.h>
#include <netdb.h>
#endif

using namespace CatchChallenger;

ConnectedSocket::ConnectedSocket(QFakeSocket *socket) :
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

ConnectedSocket::~ConnectedSocket()
{
    if(sslSocket!=NULL)
        sslSocket->deleteLater();
    if(tcpSocket!=NULL)
        tcpSocket->deleteLater();
    if(fakeSocket!=NULL)
        fakeSocket->deleteLater();
}

QList<QSslError> ConnectedSocket::sslErrors() const
{
    if(sslSocket!=NULL)
        return sslSocket->sslErrors();
    return QList<QSslError>();
}

void ConnectedSocket::purgeBuffer()
{
    if(sslSocket!=NULL)
    {
        if(sslSocket->bytesAvailable())
            emit readyRead();
        if(sslSocket->encryptedBytesAvailable())
            emit readyRead();
    }
}

void ConnectedSocket::destroyedSocket()
{
    sslSocket=NULL;
    tcpSocket=NULL;
    fakeSocket=NULL;
}

void ConnectedSocket::abort()
{
    if(fakeSocket!=NULL)
        fakeSocket->abort();
    if(sslSocket!=NULL)
        sslSocket->abort();
    if(tcpSocket!=NULL)
        tcpSocket->abort();
}

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

void ConnectedSocket::disconnectFromHost()
{
    if(state()!=QAbstractSocket::ConnectedState)
        return;
    if(fakeSocket!=NULL)
        fakeSocket->disconnectFromHost();
    if(sslSocket!=NULL)
        sslSocket->disconnectFromHost();
    if(tcpSocket!=NULL)
        tcpSocket->disconnectFromHost();
}

QAbstractSocket::SocketError ConnectedSocket::error() const
{
    if(fakeSocket!=NULL)
        return fakeSocket->error();
    if(sslSocket!=NULL)
        return sslSocket->error();
    if(tcpSocket!=NULL)
        return tcpSocket->error();
    return QAbstractSocket::UnknownSocketError;
}

bool ConnectedSocket::flush()
{
    if(fakeSocket!=NULL)
        return true;
    if(sslSocket!=NULL)
        return sslSocket->flush();
    if(tcpSocket!=NULL)
        return tcpSocket->flush();
    return false;
}

bool ConnectedSocket::isValid() const
{
    if(fakeSocket!=NULL)
        return fakeSocket->isValid();
    else if(sslSocket!=NULL)
        return sslSocket->isValid();
    else if(tcpSocket!=NULL)
        return tcpSocket->isValid();
    return false;
}

void ConnectedSocket::setTcpCork(const bool &cork)
{
    #ifdef __linux__
    if(sslSocket!=NULL)
    {
        const int &infd=sslSocket->socketDescriptor();
        if(infd!=-1)
        {
            int state = cork;
            if(setsockopt(infd, IPPROTO_TCP, TCP_CORK, &state, sizeof(state))!=0)
                std::cerr << "Unable to apply tcp cork" << std::endl;
        }
    }
    if(tcpSocket!=NULL)
    {
        const int &infd=tcpSocket->socketDescriptor();
        if(infd!=-1)
        {
            int state = cork;
            if(setsockopt(infd, IPPROTO_TCP, TCP_CORK, &state, sizeof(state))!=0)
                std::cerr << "Unable to apply tcp cork" << std::endl;
        }
    }
    #endif
}

QHostAddress ConnectedSocket::localAddress() const
{
    if(fakeSocket!=NULL)
        return QHostAddress::LocalHost;
    if(sslSocket!=NULL)
        return sslSocket->localAddress();
    if(tcpSocket!=NULL)
        return tcpSocket->localAddress();
    return QHostAddress::Null;
}

quint16	ConnectedSocket::localPort() const
{
    if(fakeSocket!=NULL)
        return 9999;
    if(sslSocket!=NULL)
        return sslSocket->localPort();
    if(tcpSocket!=NULL)
        return tcpSocket->localPort();
    return 0;
}

QHostAddress	ConnectedSocket::peerAddress() const
{
    if(fakeSocket!=NULL)
        return QHostAddress::LocalHost;
    if(sslSocket!=NULL)
        return sslSocket->peerAddress();
    if(tcpSocket!=NULL)
        return tcpSocket->peerAddress();
    return QHostAddress::Null;
}

QString ConnectedSocket::peerName() const
{
    return peerAddress().toString();
}

quint16	ConnectedSocket::peerPort() const
{
    if(fakeSocket!=NULL)
        return 15000;
    if(sslSocket!=NULL)
        return sslSocket->peerPort();
    if(tcpSocket!=NULL)
        return tcpSocket->peerPort();
    return 0;
}

QAbstractSocket::SocketState ConnectedSocket::state() const
{
    if(fakeSocket!=NULL)
        return fakeSocket->state();
    if(sslSocket!=NULL)
        return sslSocket->state();
    if(tcpSocket!=NULL)
        return tcpSocket->state();
    return QAbstractSocket::UnconnectedState;
}

bool ConnectedSocket::waitForConnected(int msecs)
{
    if(fakeSocket!=NULL)
        return true;
    if(sslSocket!=NULL)
        return sslSocket->waitForConnected(msecs);
    if(tcpSocket!=NULL)
        return tcpSocket->waitForConnected(msecs);
    return false;
}

bool ConnectedSocket::waitForDisconnected(int msecs)
{
    if(fakeSocket!=NULL)
        return true;
    if(sslSocket!=NULL)
        return sslSocket->waitForDisconnected(msecs);
    if(tcpSocket!=NULL)
        return tcpSocket->waitForDisconnected(msecs);
    return false;
}

qint64 ConnectedSocket::bytesAvailable() const
{
    if(fakeSocket!=NULL)
        return fakeSocket->bytesAvailable();
    if(tcpSocket!=NULL)
        return tcpSocket->bytesAvailable();
    if(sslSocket!=NULL)
        return sslSocket->bytesAvailable();
        else
    return -1;
}

QIODevice::OpenMode ConnectedSocket::openMode() const
{
    if(fakeSocket!=NULL)
        return fakeSocket->openMode();
    if(sslSocket!=NULL)
        return sslSocket->openMode();
    if(tcpSocket!=NULL)
        return tcpSocket->openMode();
    return QIODevice::NotOpen;
}

QString ConnectedSocket::errorString() const
{
    if(fakeSocket!=NULL)
        return fakeSocket->errorString();
    if(sslSocket!=NULL)
        return sslSocket->errorString();
    if(tcpSocket!=NULL)
        return tcpSocket->errorString();
    return QString();
}

void ConnectedSocket::close()
{
    disconnectFromHost();
}

qint64 ConnectedSocket::readData(char * data, qint64 maxSize)
{
    if(fakeSocket!=NULL)
        return fakeSocket->read(data,maxSize);
    if(tcpSocket!=NULL)
        return tcpSocket->read(data,maxSize);
    if(sslSocket!=NULL)
        return sslSocket->read(data,maxSize);
    return -1;
}

qint64 ConnectedSocket::writeData(const char * data, qint64 maxSize)
{
    if(fakeSocket!=NULL)
        return fakeSocket->write(data,maxSize);
    if(tcpSocket!=NULL)
        return tcpSocket->write(data,maxSize);
    if(sslSocket!=NULL)
        return sslSocket->write(data,maxSize);
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

#endif
