#if ! defined(EPOLLCATCHCHALLENGERSERVER) && ! defined (ONLYMAPRENDER)

#include "ConnectedSocket.h"
#include <unistd.h>
#include <iostream>
#include <fcntl.h>

#ifdef __linux__
#include <netinet/tcp.h>
#include <netdb.h>
#endif

using namespace CatchChallenger;
#ifndef NOTCPSOCKET
ConnectedSocket::ConnectedSocket(QFakeSocket *socket) :
    fakeSocket(socket),
    sslSocket(NULL),
    tcpSocket(NULL)
{
    if(!connect(socket,&QFakeSocket::destroyed,     this,&ConnectedSocket::destroyedSocket,Qt::DirectConnection))
        abort();
    if(!connect(socket,&QFakeSocket::connected,     this,&ConnectedSocket::connected,Qt::QueuedConnection))
        abort();
    if(!connect(socket,&QFakeSocket::disconnected,  this,&ConnectedSocket::disconnected,Qt::QueuedConnection))
        abort();
    if(!connect(socket,static_cast<void(QFakeSocket::*)(QAbstractSocket::SocketError)>(&QFakeSocket::error),this,static_cast<void(ConnectedSocket::*)(QAbstractSocket::SocketError)>(&ConnectedSocket::error)))
        abort();
    if(!connect(socket,&QFakeSocket::stateChanged,   this,&ConnectedSocket::stateChanged,Qt::QueuedConnection))
        abort();
    if(!connect(socket,&QFakeSocket::readyRead,     this,&ConnectedSocket::readyRead,Qt::DirectConnection))
        abort();
    open(QIODevice::ReadWrite|QIODevice::Unbuffered);
    #ifndef NOWEBSOCKET
    webSocket=nullptr;
    #endif
}

ConnectedSocket::ConnectedSocket(QSslSocket *socket) :
    fakeSocket(NULL),
    sslSocket(socket),
    tcpSocket(NULL)
{
    socket->setSocketOption(QAbstractSocket::KeepAliveOption,1);
    if(!connect(socket,&QSslSocket::readyRead,      this,&ConnectedSocket::readyRead,Qt::DirectConnection))
        abort();
    if(!connect(socket,&QSslSocket::encrypted,      this,&ConnectedSocket::purgeBuffer,Qt::QueuedConnection))
        abort();
    if(!connect(socket,&QSslSocket::connected,      this,&ConnectedSocket::connected,Qt::QueuedConnection))
        abort();
    if(!connect(socket,&QSslSocket::destroyed,      this,&ConnectedSocket::destroyedSocket,Qt::DirectConnection))
        abort();
    if(!connect(socket,&QSslSocket::disconnected,   this,&ConnectedSocket::disconnected,Qt::QueuedConnection))
        abort();
    if(!connect(socket,static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),this,static_cast<void(ConnectedSocket::*)(QAbstractSocket::SocketError)>(&ConnectedSocket::error)))
        abort();
    if(!connect(socket,&QSslSocket::stateChanged,   this,&ConnectedSocket::stateChanged,Qt::QueuedConnection))
        abort();
    if(!connect(socket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),this,static_cast<void(ConnectedSocket::*)(const QList<QSslError> &errors)>(&ConnectedSocket::sslErrors),Qt::QueuedConnection))
        abort();
    purgeBuffer();
    open(QIODevice::ReadWrite|QIODevice::Unbuffered);
    #ifndef NOWEBSOCKET
    webSocket=nullptr;
    #endif
}

ConnectedSocket::ConnectedSocket(QTcpSocket *socket) :
    fakeSocket(NULL),
    sslSocket(NULL),
    tcpSocket(socket)
{
    socket->setSocketOption(QAbstractSocket::KeepAliveOption,1);
    if(!connect(socket,&QTcpSocket::readyRead,      this,&ConnectedSocket::readyRead,Qt::DirectConnection))
        abort();
    if(!connect(socket,&QTcpSocket::connected,      this,&ConnectedSocket::connected,Qt::QueuedConnection))
        abort();
    if(!connect(socket,&QTcpSocket::destroyed,      this,&ConnectedSocket::destroyedSocket,Qt::DirectConnection))
        abort();
    if(!connect(socket,&QTcpSocket::disconnected,   this,&ConnectedSocket::disconnected,Qt::QueuedConnection))
        abort();
    if(!connect(socket,static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),this,static_cast<void(ConnectedSocket::*)(QAbstractSocket::SocketError)>(&ConnectedSocket::error)))
        abort();
    if(!connect(socket,&QTcpSocket::stateChanged,   this,&ConnectedSocket::stateChanged,Qt::QueuedConnection))
        abort();
    open(QIODevice::ReadWrite|QIODevice::Unbuffered);
    #ifndef NOWEBSOCKET
    webSocket=nullptr;
    #endif
}
#endif
#ifndef NOWEBSOCKET
ConnectedSocket::ConnectedSocket(QWebSocket *socket) :
    webSocket(socket)
{
    if(!connect(socket,&QWebSocket::binaryMessageReceived,      this,&ConnectedSocket::binaryMessageReceived,Qt::DirectConnection))
        abort();
    if(!connect(socket,&QWebSocket::connected,      this,&ConnectedSocket::connected,Qt::QueuedConnection))
        abort();
    if(!connect(socket,&QWebSocket::destroyed,      this,&ConnectedSocket::destroyedSocket,Qt::DirectConnection))
        abort();
    if(!connect(socket,&QWebSocket::disconnected,   this,&ConnectedSocket::disconnected,Qt::QueuedConnection))
        abort();
    if(!connect(socket,static_cast<void(QWebSocket::*)(QAbstractSocket::SocketError)>(&QWebSocket::error),this,static_cast<void(ConnectedSocket::*)(QAbstractSocket::SocketError)>(&ConnectedSocket::error)))
        abort();
    if(!connect(socket,&QWebSocket::stateChanged,   this,&ConnectedSocket::stateChanged,Qt::QueuedConnection))
        abort();
/*    if(!connect(socket,&QWebSocket::sslErrors,      this,&ConnectedSocket::saveSslErrors,Qt::QueuedConnection))
        abort();*/
    open(QIODevice::ReadWrite|QIODevice::Unbuffered);
    #ifndef NOTCPSOCKET
    fakeSocket=nullptr;
    sslSocket=nullptr;
    tcpSocket=nullptr;
    #endif
}
#endif

ConnectedSocket::~ConnectedSocket()
{
    #ifndef NOTCPSOCKET
    if(sslSocket!=nullptr)
        sslSocket->deleteLater();
    if(tcpSocket!=nullptr)
        tcpSocket->deleteLater();
    if(fakeSocket!=nullptr)
        fakeSocket->deleteLater();
    #endif
    #ifndef NOWEBSOCKET
    if(webSocket!=nullptr)
        webSocket->deleteLater();
    #endif
}

#ifndef NOWEBSOCKET
void ConnectedSocket::binaryMessageReceived(const QByteArray &message)
{
    buffer.push_back(message);
    emit readyRead();
}

/*void ConnectedSocket::saveSslErrors(const QList<QSslError> &errors)
{
    m_sslErrors=errors;
}*/
#endif

#ifndef __EMSCRIPTEN__
QList<QSslError> ConnectedSocket::sslErrors() const
{
    #ifndef NOTCPSOCKET
    if(sslSocket!=nullptr)
        return sslSocket->sslErrors();
    #endif
    #ifndef NOWEBSOCKET
    return m_sslErrors;
    #endif
    return QList<QSslError>();
}
#endif

void ConnectedSocket::purgeBuffer()
{
    #ifndef NOTCPSOCKET
    if(sslSocket!=nullptr)
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
    #ifndef NOTCPSOCKET
    sslSocket=nullptr;
    tcpSocket=nullptr;
    fakeSocket=nullptr;
    #endif
    hostName.clear();
    port=0;
}

void ConnectedSocket::abort()
{
    hostName.clear();
    port=0;

    #ifndef NOTCPSOCKET
    if(fakeSocket!=nullptr)
        fakeSocket->abort();
    if(sslSocket!=nullptr)
        sslSocket->abort();
    if(tcpSocket!=nullptr)
        tcpSocket->abort();
    #endif
    #ifndef NOWEBSOCKET
    if(webSocket!=nullptr)
        webSocket->abort();
    #endif
}

void ConnectedSocket::connectToHost(const QString & hostName, quint16 port)
{
    if(state()!=QAbstractSocket::UnconnectedState)
        return;
    #ifndef NOTCPSOCKET
    if(fakeSocket!=nullptr)
        fakeSocket->connectToHost();
    else
    {
        //workaround because QSslSocket don't return correct value for i2p via proxy
        this->hostName=hostName;
        this->port=port;

        if(sslSocket!=nullptr)
        {
            sslSocket->connectToHost(hostName,port);
            return;
        }
        else if(tcpSocket!=nullptr)
        {
            tcpSocket->connectToHost(hostName,port);
            return;
        }
    }
    #endif
    #ifndef NOWEBSOCKET
    if(webSocket!=nullptr)
    {
        QString tempHost(hostName);
        if(tempHost.contains(":") && !tempHost.contains("["))
            tempHost="["+tempHost+"]";
        QUrl url{QString("ws://"+tempHost+":"+QString::number(port)+"/")};
        QNetworkRequest request{url};
        request.setRawHeader("Sec-WebSocket-Protocol", "binary");
        webSocket->open(request);
        return;
    }
    #endif
}

void ConnectedSocket::connectToHost(const QHostAddress & address, quint16 port)
{
    if(state()!=QAbstractSocket::UnconnectedState)
        return;
    #ifndef NOTCPSOCKET
    if(fakeSocket!=nullptr)
    {
        fakeSocket->connectToHost();
        return;
    }
    else if(sslSocket!=nullptr)
    {
        sslSocket->connectToHost(address.toString(),port);
        return;
    }
    else if(tcpSocket!=nullptr)
    {
        tcpSocket->connectToHost(address,port);
        return;
    }
    #endif
    #ifndef NOWEBSOCKET
    if(webSocket!=nullptr)
    {
        connectToHost(address.toString(),port);
        return;
    }
    #endif
}

void ConnectedSocket::disconnectFromHost()
{
    if(state()!=QAbstractSocket::ConnectedState)
        return;
    hostName.clear();
    port=0;
    #ifndef NOTCPSOCKET
    if(fakeSocket!=nullptr)
        fakeSocket->disconnectFromHost();
    if(sslSocket!=nullptr)
        sslSocket->disconnectFromHost();
    if(tcpSocket!=nullptr)
        tcpSocket->disconnectFromHost();
    #endif
    #ifndef NOWEBSOCKET
    if(webSocket!=nullptr)
        webSocket->close();
    #endif
}

QAbstractSocket::SocketError ConnectedSocket::error() const
{
    #ifndef NOTCPSOCKET
    if(fakeSocket!=nullptr)
        return fakeSocket->error();
    if(sslSocket!=nullptr)
        return sslSocket->error();
    if(tcpSocket!=nullptr)
        return tcpSocket->error();
    #endif
    #ifndef NOWEBSOCKET
    if(webSocket!=nullptr)
        return webSocket->error();
    #endif
    return QAbstractSocket::UnknownSocketError;
}

bool ConnectedSocket::flush()
{
    #ifndef NOTCPSOCKET
    if(fakeSocket!=nullptr)
        return true;
    if(sslSocket!=nullptr)
        return sslSocket->flush();
    if(tcpSocket!=nullptr)
        return tcpSocket->flush();
    #endif
    #ifndef NOWEBSOCKET
    if(webSocket!=nullptr)
        return webSocket->flush();
    #endif
    return false;
}

bool ConnectedSocket::isValid() const
{
    #ifndef NOTCPSOCKET
    if(fakeSocket!=nullptr)
        return fakeSocket->isValid();
    else if(sslSocket!=nullptr)
        return sslSocket->isValid();
    else if(tcpSocket!=nullptr)
        return tcpSocket->isValid();
    #endif
    #ifndef NOWEBSOCKET
    if(webSocket!=nullptr)
        webSocket->isValid();
    #endif
    return false;
}

void ConnectedSocket::setTcpCork(const bool &cork)
{
    #ifndef __EMSCRIPTEN__
    #ifdef __linux__
    if(sslSocket!=nullptr)
    {
        #if ! defined(EPOLLCATCHCHALLENGERSERVER) && ! defined (ONLYMAPRENDER)
        const qintptr &infd=
        #else
        const int32_t &infd=
        #endif
                sslSocket->socketDescriptor();
        if(infd!=-1)
        {
            int state = cork;
            if(setsockopt(static_cast<int>(infd), IPPROTO_TCP, TCP_CORK, &state, sizeof(state))!=0)
                std::cerr << "Unable to apply tcp cork" << std::endl;
        }
    }
    if(tcpSocket!=nullptr)
    {
        #if ! defined(EPOLLCATCHCHALLENGERSERVER) && ! defined (ONLYMAPRENDER)
        const qintptr &infd=
        #else
        const int32_t &infd=
        #endif
                tcpSocket->socketDescriptor();
        if(infd!=-1)
        {
            int state = cork;
            if(setsockopt(static_cast<int>(infd), IPPROTO_TCP, TCP_CORK, &state, sizeof(state))!=0)
                std::cerr << "Unable to apply tcp cork" << std::endl;
        }
    }
    #endif
    #else
    Q_UNUSED(cork);
    #endif
}

QHostAddress ConnectedSocket::localAddress() const
{
    //deprecated form incorrect value for i2p
    std::cerr << "ConnectedSocket::localAddress(): deprecated form incorrect value for i2p" << std::endl;

    #ifndef NOTCPSOCKET
    if(fakeSocket!=nullptr)
        return QHostAddress::LocalHost;
    if(sslSocket!=nullptr)
        return sslSocket->localAddress();
    if(tcpSocket!=nullptr)
        return tcpSocket->localAddress();
    #endif
    return QHostAddress::Null;
}

quint16	ConnectedSocket::localPort() const
{
    #ifndef NOTCPSOCKET
    if(fakeSocket!=nullptr)
        return 9999;
    if(sslSocket!=nullptr)
        return sslSocket->localPort();
    if(tcpSocket!=nullptr)
        return tcpSocket->localPort();
    #endif
    return 0;
}

QHostAddress	ConnectedSocket::peerAddress() const
{
    //deprecated form incorrect value for i2p
    std::cerr << "ConnectedSocket::peerAddress(): deprecated form incorrect value for i2p" << std::endl;

    #ifndef NOTCPSOCKET
    if(fakeSocket!=nullptr)
        return QHostAddress::LocalHost;
    if(sslSocket!=nullptr)
        return sslSocket->peerAddress();
    if(tcpSocket!=nullptr)
        return tcpSocket->peerAddress();
    #endif
    return QHostAddress::Null;
}

QString ConnectedSocket::peerName() const
{
    #ifndef NOTCPSOCKET
    /// \warning via direct value for i2p. Never pass by peerAddress()
    QString pearName;
    if(fakeSocket!=nullptr)
        return QString();
    if(sslSocket!=nullptr)
        pearName=sslSocket->peerName();
    if(tcpSocket!=nullptr)
        pearName=tcpSocket->peerName();
    if(!pearName.isEmpty())
        return pearName;
    else
        return hostName;
    #endif
    #ifndef NOWEBSOCKET
    return QString();
    #endif
}

quint16	ConnectedSocket::peerPort() const
{
    #ifndef NOTCPSOCKET
    if(fakeSocket!=nullptr)
        return 15000;
    if(sslSocket!=nullptr)
        return sslSocket->peerPort();
    if(tcpSocket!=nullptr)
        return tcpSocket->peerPort();
    #endif
    return 0;
}

QAbstractSocket::SocketState ConnectedSocket::state() const
{
    #ifndef NOTCPSOCKET
    if(fakeSocket!=nullptr)
        return fakeSocket->state();
    if(sslSocket!=nullptr)
        return sslSocket->state();
    if(tcpSocket!=nullptr)
        return tcpSocket->state();
    #endif
    #ifndef NOWEBSOCKET
    if(webSocket!=nullptr)
        webSocket->state();
    #endif
    return QAbstractSocket::UnconnectedState;
}

bool ConnectedSocket::waitForConnected(int msecs)
{
    #ifndef NOTCPSOCKET
    if(fakeSocket!=nullptr)
        return true;
    if(sslSocket!=nullptr)
        return sslSocket->waitForConnected(msecs);
    if(tcpSocket!=nullptr)
        return tcpSocket->waitForConnected(msecs);
    #endif
    Q_UNUSED(msecs);
    return false;
}

bool ConnectedSocket::waitForDisconnected(int msecs)
{
    #ifndef NOTCPSOCKET
    if(fakeSocket!=nullptr)
        return true;
    if(sslSocket!=nullptr)
        return sslSocket->waitForDisconnected(msecs);
    if(tcpSocket!=nullptr)
        return tcpSocket->waitForDisconnected(msecs);
    #endif
    Q_UNUSED(msecs);
    return false;
}

qint64 ConnectedSocket::bytesAvailable() const
{
    #ifndef NOTCPSOCKET
    if(fakeSocket!=nullptr)
        return fakeSocket->bytesAvailable();
    if(tcpSocket!=nullptr)
        return tcpSocket->bytesAvailable();
    if(sslSocket!=nullptr)
        return sslSocket->bytesAvailable();
        else
    #endif
    #ifndef NOWEBSOCKET
    if(webSocket!=nullptr)
        return buffer.size();
    #endif
    return -1;
}

QIODevice::OpenMode ConnectedSocket::openMode() const
{
    #ifndef NOTCPSOCKET
    if(fakeSocket!=nullptr)
        return fakeSocket->openMode();
    if(sslSocket!=nullptr)
        return sslSocket->openMode();
    if(tcpSocket!=nullptr)
        return tcpSocket->openMode();
    #endif
    return QIODevice::ReadWrite;
}

QString ConnectedSocket::errorString() const
{
    #ifndef NOTCPSOCKET
    if(fakeSocket!=nullptr)
        return fakeSocket->errorString();
    if(sslSocket!=nullptr)
        return sslSocket->errorString();
    if(tcpSocket!=nullptr)
        return tcpSocket->errorString();
    #endif
    #ifndef NOWEBSOCKET
    if(webSocket!=nullptr)
        webSocket->errorString();
    #endif
    return QString();
}

void ConnectedSocket::close()
{
    disconnectFromHost();
}

qint64 ConnectedSocket::readData(char * data, qint64 maxSize)
{
    #ifndef NOTCPSOCKET
    if(fakeSocket!=nullptr)
        return fakeSocket->read(data,maxSize);
    if(tcpSocket!=nullptr)
        return tcpSocket->read(data,maxSize);
    if(sslSocket!=nullptr)
        return sslSocket->read(data,maxSize);
    #endif
    #ifndef NOWEBSOCKET
    if(webSocket!=nullptr)
    {
        QByteArray temp(buffer.mid(0,maxSize));
        buffer.remove(0,temp.size());
        memcpy(data,temp.constData(),temp.size());
        return temp.size();
    }
    #endif
    return -1;
}

qint64 ConnectedSocket::writeData(const char * data, qint64 maxSize)
{
    #ifndef NOTCPSOCKET
    if(fakeSocket!=nullptr)
        return fakeSocket->write(data,maxSize);
    if(tcpSocket!=nullptr)
        return tcpSocket->write(data,maxSize);
    if(sslSocket!=nullptr)
        return sslSocket->write(data,maxSize);
    #endif
    #ifndef NOWEBSOCKET
    if(webSocket!=nullptr)
        return webSocket->sendBinaryMessage(QByteArray(data,maxSize));
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

#endif
