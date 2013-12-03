#include "ConnectedSocket.h"
#include <QSslSocket>

using namespace CatchChallenger;

ConnectedSocket::ConnectedSocket(QFakeSocket *socket,QObject *parent) :
    QIODevice(parent)
{
    this->fakeSocket=socket;
    this->sslSocket=NULL;
    connect(socket,&QFakeSocket::destroyed,     this,&ConnectedSocket::destroyedSocket,Qt::DirectConnection);
    connect(socket,&QFakeSocket::connected,     this,&ConnectedSocket::connected,Qt::QueuedConnection);
    connect(socket,&QFakeSocket::disconnected,  this,&ConnectedSocket::disconnected,Qt::QueuedConnection);
    connect(socket,static_cast<void(QFakeSocket::*)(QAbstractSocket::SocketError)>(&QFakeSocket::error),this,static_cast<void(ConnectedSocket::*)(QAbstractSocket::SocketError)>(&ConnectedSocket::error));
    connect(socket,&QFakeSocket::stateChanged,   this,&ConnectedSocket::stateChanged,Qt::QueuedConnection);
    connect(socket,&QFakeSocket::readyRead,     this,&ConnectedSocket::readyRead,Qt::DirectConnection);
    open(QIODevice::ReadWrite|QIODevice::Unbuffered);
}

ConnectedSocket::ConnectedSocket(QSslSocket *socket,QObject *parent) :
    QIODevice(parent)
{
    this->fakeSocket=NULL;
    this->sslSocket=socket;
    socket->setSocketOption(QAbstractSocket::KeepAliveOption,1);
    connect(socket,&QSslSocket::readyRead,      this,&ConnectedSocket::readyRead,Qt::DirectConnection);
    connect(socket,&QSslSocket::encrypted,      this,&ConnectedSocket::encrypted,Qt::QueuedConnection);
    connect(socket,&QSslSocket::connected,      this,&ConnectedSocket::connected,Qt::QueuedConnection);
    connect(socket,&QSslSocket::destroyed,      this,&ConnectedSocket::destroyedSocket,Qt::DirectConnection);
    connect(socket,&QSslSocket::connected,      this,&ConnectedSocket::startHandshake,Qt::QueuedConnection);
    connect(socket,&QSslSocket::disconnected,   this,&ConnectedSocket::disconnected,Qt::QueuedConnection);
    connect(socket,static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),this,static_cast<void(ConnectedSocket::*)(QAbstractSocket::SocketError)>(&ConnectedSocket::error));
    connect(socket,&QSslSocket::stateChanged,   this,&ConnectedSocket::stateChanged,Qt::QueuedConnection);
    encrypted();
    open(QIODevice::ReadWrite|QIODevice::Unbuffered);
}

ConnectedSocket::~ConnectedSocket()
{
    if(sslSocket!=NULL)
    {
        QSslSocket *sslSocket=this->sslSocket;
        this->sslSocket=NULL;
        delete sslSocket;
    }
    if(fakeSocket!=NULL)
    {
        QFakeSocket *fakeSocket=this->fakeSocket;
        this->fakeSocket=NULL;
        delete fakeSocket;
    }
}

void ConnectedSocket::encrypted()
{
    if(sslSocket!=NULL)
    {
        if(!tempClearData.isEmpty())
        {
            sslSocket->write(tempClearData.data(),tempClearData.size());
            tempClearData.clear();
        }
        if(sslSocket->bytesAvailable())
            emit readyRead();
        if(sslSocket->encryptedBytesAvailable())
            emit readyRead();
    }
}

void ConnectedSocket::destroyedSocket()
{
    sslSocket=NULL;
    fakeSocket=NULL;
}

void ConnectedSocket::abort()
{
    if(fakeSocket!=NULL)
        fakeSocket->abort();
    if(sslSocket!=NULL)
        sslSocket->abort();
}

void ConnectedSocket::connectToHost(const QString & hostName, quint16 port)
{
    if(fakeSocket!=NULL)
        fakeSocket->connectToHost();
    if(sslSocket!=NULL)
        sslSocket->connectToHostEncrypted(hostName,port);
}

void ConnectedSocket::connectToHost(const QHostAddress & address, quint16 port)
{
    if(fakeSocket!=NULL)
        fakeSocket->connectToHost();
    if(sslSocket!=NULL)
        sslSocket->connectToHost(address,port);
}

void ConnectedSocket::startHandshake()
{
/*    if(sslSocket!=NULL)
        if(!sslSocket->isEncrypted())
            sslSocket->startClientEncryption();*/
}

void ConnectedSocket::disconnectFromHost()
{
    if(fakeSocket!=NULL)
        fakeSocket->disconnectFromHost();
    if(sslSocket!=NULL)
        sslSocket->disconnectFromHost();
}

QAbstractSocket::SocketError ConnectedSocket::error() const
{
    if(fakeSocket!=NULL)
        return fakeSocket->error();
    if(sslSocket!=NULL)
        return sslSocket->error();
    return QAbstractSocket::UnknownSocketError;
}

bool ConnectedSocket::flush()
{
    if(fakeSocket!=NULL)
        return true;
    if(sslSocket!=NULL)
        return sslSocket->flush();
    return false;
}

bool ConnectedSocket::isValid() const
{
    if(fakeSocket!=NULL)
        return fakeSocket->isValid();
    if(sslSocket!=NULL)
        return sslSocket->isValid();
    return false;
}

QHostAddress ConnectedSocket::localAddress() const
{
    if(fakeSocket!=NULL)
        return QHostAddress::LocalHost;
    if(sslSocket!=NULL)
        return sslSocket->localAddress();
    return QHostAddress::Null;
}

quint16	ConnectedSocket::localPort() const
{
    if(fakeSocket!=NULL)
        return 9999;
    if(sslSocket!=NULL)
        return sslSocket->localPort();
    return 0;
}

QHostAddress	ConnectedSocket::peerAddress() const
{
    if(fakeSocket!=NULL)
        return QHostAddress::LocalHost;
    if(sslSocket!=NULL)
        return sslSocket->peerAddress();
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
    if(sslSocket!=NULL)
        return sslSocket->peerPort();
    return 0;
}

QAbstractSocket::SocketState ConnectedSocket::state() const
{
    if(fakeSocket!=NULL)
        return fakeSocket->state();
    if(sslSocket!=NULL)
        return sslSocket->state();
    return QAbstractSocket::UnconnectedState;
}

bool ConnectedSocket::waitForConnected(int msecs)
{
    if(fakeSocket!=NULL)
        return true;
    if(sslSocket!=NULL)
        return sslSocket->waitForConnected(msecs);
    return false;
}

bool ConnectedSocket::waitForDisconnected(int msecs)
{
    if(fakeSocket!=NULL)
        return true;
    if(sslSocket!=NULL)
        return sslSocket->waitForDisconnected(msecs);
    return false;
}

qint64 ConnectedSocket::bytesAvailable() const
{
    if(fakeSocket!=NULL)
        return fakeSocket->bytesAvailable();
    if(sslSocket!=NULL)
    {
        if(sslSocket->isEncrypted())
            return sslSocket->bytesAvailable();
        else
            return 0;
    }
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
    if(sslSocket!=NULL)
    {
        if(sslSocket->isEncrypted())
            return sslSocket->read(data,maxSize);
        else
            return 0;
    }
    return -1;
}

qint64 ConnectedSocket::writeData(const char * data, qint64 maxSize)
{
    if(fakeSocket!=NULL)
        return fakeSocket->write(data,maxSize);
    if(sslSocket!=NULL)
    {
        if(sslSocket->isEncrypted())
            return sslSocket->write(data,maxSize);
        else
        {
            tempClearData+=QByteArray(data,maxSize);
            return maxSize;
        }
    }
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
