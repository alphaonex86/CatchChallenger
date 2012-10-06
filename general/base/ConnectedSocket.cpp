#include "ConnectedSocket.h"

using namespace Pokecraft;

ConnectedSocket::ConnectedSocket(QFakeSocket *socket,QObject *parent) :
    QIODevice(parent)
{
    this->fakeSocket=socket;
    this->tcpSocket=NULL;
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

void ConnectedSocket::abort()
{
    if(fakeSocket!=NULL)
        fakeSocket->abort();
    else
        tcpSocket->abort();
}

void ConnectedSocket::connectToHost(const QString & hostName, quint16 port)
{
    if(fakeSocket!=NULL)
        fakeSocket->connectToHost();
    else
        tcpSocket->connectToHost(hostName,port);
}

void ConnectedSocket::connectToHost(const QHostAddress & address, quint16 port)
{
    if(fakeSocket!=NULL)
        fakeSocket->connectToHost();
    else
        tcpSocket->connectToHost(address,port);
}

void ConnectedSocket::disconnectFromHost()
{
    if(fakeSocket!=NULL)
        fakeSocket->disconnectFromHost();
    else
        tcpSocket->disconnectFromHost();
}

QAbstractSocket::SocketError ConnectedSocket::error() const
{
    if(fakeSocket!=NULL)
        return fakeSocket->error();
    else
        return tcpSocket->error();
}

bool ConnectedSocket::flush()
{
    if(fakeSocket!=NULL)
        return true;
    else
        return tcpSocket->flush();
}

bool ConnectedSocket::isValid() const
{
    if(fakeSocket!=NULL)
        return fakeSocket->isValid();
    else
        return tcpSocket->isValid();
}

QHostAddress ConnectedSocket::localAddress() const
{
    if(fakeSocket!=NULL)
        return QHostAddress::LocalHost;
    else
        return tcpSocket->localAddress();
}

quint16	ConnectedSocket::localPort() const
{
    if(fakeSocket!=NULL)
        return 9999;
    else
        return tcpSocket->localPort();
}

QHostAddress	ConnectedSocket::peerAddress() const
{
    if(fakeSocket!=NULL)
        return QHostAddress::LocalHost;
    else
        return tcpSocket->peerAddress();
}

QString	ConnectedSocket::peerName() const
{
    return peerAddress().toString();
}

quint16	ConnectedSocket::peerPort() const
{
    if(fakeSocket!=NULL)
        return 15000;
    else
        return tcpSocket->peerPort();
}

QAbstractSocket::SocketState ConnectedSocket::state() const
{
    if(fakeSocket!=NULL)
        return fakeSocket->state();
    else
        return tcpSocket->state();
}

bool ConnectedSocket::waitForConnected(int msecs)
{
    if(fakeSocket!=NULL)
        return true;
    else
        return tcpSocket->waitForConnected(msecs);
}

bool ConnectedSocket::waitForDisconnected(int msecs)
{
    if(fakeSocket!=NULL)
        return true;
    else
        return tcpSocket->waitForDisconnected(msecs);
}

qint64 ConnectedSocket::bytesAvailable() const
{
    if(fakeSocket!=NULL)
        return fakeSocket->bytesAvailable();
    else
        return tcpSocket->bytesAvailable();
}

void ConnectedSocket::close()
{
    abort();
}

qint64 ConnectedSocket::readData(char * data, qint64 maxSize)
{
    if(fakeSocket!=NULL)
        return fakeSocket->read(data,maxSize);
    else
        return tcpSocket->read(data,maxSize);
}

qint64 ConnectedSocket::writeData(const char * data, qint64 maxSize)
{
    if(fakeSocket!=NULL)
        return fakeSocket->write(data,maxSize);
    else
        return tcpSocket->write(data,maxSize);
}

bool ConnectedSocket::isSequential() const
{
    return true;
}

bool ConnectedSocket::canReadLine () const
{
    return false;
}
