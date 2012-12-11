#include "ConnectedSocket.h"

using namespace Pokecraft;

ConnectedSocket::ConnectedSocket(QFakeSocket *socket,QObject *parent) :
    QIODevice(parent)
{
    this->fakeSocket=socket;
    this->tcpSocket=NULL;
    connect(socket,SIGNAL(destroyed()),this,SLOT(destroyedSocket()));
    connect(socket,SIGNAL(connected()),this,SIGNAL(connected()));
    connect(socket,SIGNAL(disconnected()),this,SIGNAL(disconnected()));
    connect(socket,SIGNAL(error(QAbstractSocket::SocketError)),this,SIGNAL(error(QAbstractSocket::SocketError)));
    connect(socket,SIGNAL(stateChanged(QAbstractSocket::SocketState)),this,SIGNAL(stateChanged(QAbstractSocket::SocketState)));
    connect(socket,SIGNAL(readyRead()),this,SIGNAL(readyRead()));
    open(QIODevice::ReadWrite|QIODevice::Unbuffered);
}

ConnectedSocket::ConnectedSocket(QTcpSocket *socket,QObject *parent) :
    QIODevice(parent)
{
    this->fakeSocket=NULL;
    this->tcpSocket=socket;
    connect(socket,SIGNAL(destroyed()),this,SLOT(destroyedSocket()));
    connect(socket,SIGNAL(connected()),this,SIGNAL(connected()));
    connect(socket,SIGNAL(disconnected()),this,SIGNAL(disconnected()));
    connect(socket,SIGNAL(error(QAbstractSocket::SocketError)),this,SIGNAL(error(QAbstractSocket::SocketError)));
    connect(socket,SIGNAL(stateChanged(QAbstractSocket::SocketState)),this,SIGNAL(stateChanged(QAbstractSocket::SocketState)));
    connect(socket,SIGNAL(readyRead()),this,SIGNAL(readyRead()));
    open(QIODevice::ReadWrite|QIODevice::Unbuffered);
}

ConnectedSocket::~ConnectedSocket()
{
    if(tcpSocket!=NULL)
    {
        delete tcpSocket;
        tcpSocket=NULL;
    }
    if(fakeSocket!=NULL)
    {
        delete fakeSocket;
        fakeSocket=NULL;
    }
}

void ConnectedSocket::destroyedSocket()
{
    tcpSocket=NULL;
    fakeSocket=NULL;
}

void ConnectedSocket::abort()
{
    if(fakeSocket!=NULL)
        fakeSocket->abort();
    if(tcpSocket!=NULL)
        tcpSocket->abort();
}

void ConnectedSocket::connectToHost(const QString & hostName, quint16 port)
{
    if(fakeSocket!=NULL)
        fakeSocket->connectToHost();
    if(tcpSocket!=NULL)
        tcpSocket->connectToHost(hostName,port);
}

void ConnectedSocket::connectToHost(const QHostAddress & address, quint16 port)
{
    if(fakeSocket!=NULL)
        fakeSocket->connectToHost();
    if(tcpSocket!=NULL)
        tcpSocket->connectToHost(address,port);
}

void ConnectedSocket::disconnectFromHost()
{
    if(fakeSocket!=NULL)
        fakeSocket->disconnectFromHost();
    if(tcpSocket!=NULL)
        tcpSocket->disconnectFromHost();
}

QAbstractSocket::SocketError ConnectedSocket::error() const
{
    if(fakeSocket!=NULL)
        return fakeSocket->error();
    if(tcpSocket!=NULL)
        return tcpSocket->error();
    return QAbstractSocket::UnknownSocketError;
}

bool ConnectedSocket::flush()
{
    if(fakeSocket!=NULL)
        return true;
    if(tcpSocket!=NULL)
        return tcpSocket->flush();
    return false;
}

bool ConnectedSocket::isValid() const
{
    if(fakeSocket!=NULL)
        return fakeSocket->isValid();
    if(tcpSocket!=NULL)
        return tcpSocket->isValid();
    return false;
}

QHostAddress ConnectedSocket::localAddress() const
{
    if(fakeSocket!=NULL)
        return QHostAddress::LocalHost;
    if(tcpSocket!=NULL)
        return tcpSocket->localAddress();
    return QHostAddress::Null;
}

quint16	ConnectedSocket::localPort() const
{
    if(fakeSocket!=NULL)
        return 9999;
    if(tcpSocket!=NULL)
        return tcpSocket->localPort();
    return 0;
}

QHostAddress	ConnectedSocket::peerAddress() const
{
    if(fakeSocket!=NULL)
        return QHostAddress::LocalHost;
    if(tcpSocket!=NULL)
        return tcpSocket->peerAddress();
    return QHostAddress::Null;
}

QString	ConnectedSocket::peerName() const
{
    return peerAddress().toString();
}

quint16	ConnectedSocket::peerPort() const
{
    if(fakeSocket!=NULL)
        return 15000;
    if(tcpSocket!=NULL)
        return tcpSocket->peerPort();
    return 0;
}

QAbstractSocket::SocketState ConnectedSocket::state() const
{
    if(fakeSocket!=NULL)
        return fakeSocket->state();
    if(tcpSocket!=NULL)
        return tcpSocket->state();
    return QAbstractSocket::UnconnectedState;
}

bool ConnectedSocket::waitForConnected(int msecs)
{
    if(fakeSocket!=NULL)
        return true;
    if(tcpSocket!=NULL)
        return tcpSocket->waitForConnected(msecs);
    return false;
}

bool ConnectedSocket::waitForDisconnected(int msecs)
{
    if(fakeSocket!=NULL)
        return true;
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
    return -1;
}

void ConnectedSocket::close()
{
    abort();
}

qint64 ConnectedSocket::readData(char * data, qint64 maxSize)
{
    if(fakeSocket!=NULL)
        return fakeSocket->read(data,maxSize);
    if(tcpSocket!=NULL)
        return tcpSocket->read(data,maxSize);
    return -1;
}

qint64 ConnectedSocket::writeData(const char * data, qint64 maxSize)
{
    if(fakeSocket!=NULL)
        return fakeSocket->write(data,maxSize);
    if(tcpSocket!=NULL)
        return tcpSocket->write(data,maxSize);
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
