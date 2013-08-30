#include "ConnectedSocket.h"
#include <QSslSocket>

using namespace CatchChallenger;

ConnectedSocket::ConnectedSocket(QFakeSocket *socket,QObject *parent) :
    QIODevice(parent)
{
    this->fakeSocket=socket;
    this->tcpSocket=NULL;
    connect(socket,&QFakeSocket::destroyed,     this,&ConnectedSocket::destroyedSocket);
    connect(socket,&QFakeSocket::connected,     this,&ConnectedSocket::connected);
    connect(socket,&QFakeSocket::disconnected,  this,&ConnectedSocket::disconnected);
    connect(socket,static_cast<void(QFakeSocket::*)(QAbstractSocket::SocketError)>(&QFakeSocket::error),this,static_cast<void(ConnectedSocket::*)(QAbstractSocket::SocketError)>(&ConnectedSocket::error));
    connect(socket,&QFakeSocket::stateChanged,   this,&ConnectedSocket::stateChanged);
    connect(socket,&QFakeSocket::readyRead,     this,&ConnectedSocket::readyRead,Qt::DirectConnection);
    open(QIODevice::ReadWrite|QIODevice::Unbuffered);
}

ConnectedSocket::ConnectedSocket(QTcpSocket *socket,QObject *parent) :
    QIODevice(parent)
{
    this->fakeSocket=NULL;
    this->tcpSocket=socket;
    connect(socket,&QTcpSocket::destroyed,      this,&ConnectedSocket::destroyedSocket);
    connect(socket,&QTcpSocket::connected,      this,&ConnectedSocket::connected);
    connect(socket,&QTcpSocket::disconnected,   this,&ConnectedSocket::disconnected);
    connect(socket,static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),this,static_cast<void(ConnectedSocket::*)(QAbstractSocket::SocketError)>(&ConnectedSocket::error));
    connect(socket,&QTcpSocket::stateChanged,   this,&ConnectedSocket::stateChanged);
    connect(socket,&QTcpSocket::readyRead,      this,&ConnectedSocket::readyRead,Qt::DirectConnection);
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
        static_cast<QSslSocket *>(tcpSocket)->connectToHostEncrypted(hostName,port);
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
