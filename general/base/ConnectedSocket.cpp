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
#ifndef __EMSCRIPTEN__
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
}
#else
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
}
#endif

ConnectedSocket::~ConnectedSocket()
{
    #ifndef __EMSCRIPTEN__
    if(sslSocket!=NULL)
        sslSocket->deleteLater();
    if(tcpSocket!=NULL)
        tcpSocket->deleteLater();
    if(fakeSocket!=NULL)
        fakeSocket->deleteLater();
    #else
    if(webSocket!=NULL)
        webSocket->deleteLater();
    #endif
}

#ifdef __EMSCRIPTEN__
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
    #ifndef __EMSCRIPTEN__
    if(sslSocket!=NULL)
        return sslSocket->sslErrors();
    #else
    return m_sslErrors;
    #endif
    return QList<QSslError>();
}
#endif

void ConnectedSocket::purgeBuffer()
{
    #ifndef __EMSCRIPTEN__
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
    #ifndef __EMSCRIPTEN__
    sslSocket=NULL;
    tcpSocket=NULL;
    fakeSocket=NULL;
    #endif
    hostName.clear();
    port=0;
}

void ConnectedSocket::abort()
{
    hostName.clear();
    port=0;

    #ifndef __EMSCRIPTEN__
    if(fakeSocket!=NULL)
        fakeSocket->abort();
    if(sslSocket!=NULL)
        sslSocket->abort();
    if(tcpSocket!=NULL)
        tcpSocket->abort();
    #endif
}

void ConnectedSocket::connectToHost(const QString & hostName, quint16 port)
{
    if(state()!=QAbstractSocket::UnconnectedState)
        return;
    #ifndef __EMSCRIPTEN__
    if(fakeSocket!=NULL)
        fakeSocket->connectToHost();
    else
    {
        //workaround because QSslSocket don't return correct value for i2p via proxy
        this->hostName=hostName;
        this->port=port;

        if(sslSocket!=NULL)
            sslSocket->connectToHost(hostName,port);
        else if(tcpSocket!=NULL)
            tcpSocket->connectToHost(hostName,port);
    }
    #else
    QString tempHost(hostName);
    if(tempHost.contains(":") && !tempHost.contains("["))
        tempHost="["+tempHost+"]";
    QUrl url{QString("ws://"+tempHost+":"+QString::number(port)+"/")};
    QNetworkRequest request{url};
    request.setRawHeader("Sec-WebSocket-Protocol", "binary");
    webSocket->open(request);
    #endif
}

void ConnectedSocket::connectToHost(const QHostAddress & address, quint16 port)
{
    if(state()!=QAbstractSocket::UnconnectedState)
        return;
    #ifndef __EMSCRIPTEN__
    if(fakeSocket!=NULL)
        fakeSocket->connectToHost();
    else if(sslSocket!=NULL)
        sslSocket->connectToHost(address.toString(),port);
    else if(tcpSocket!=NULL)
        tcpSocket->connectToHost(address,port);
    #else
    connectToHost(address.toString(),port);
    #endif
}

void ConnectedSocket::disconnectFromHost()
{
    if(state()!=QAbstractSocket::ConnectedState)
        return;
    hostName.clear();
    port=0;
    #ifndef __EMSCRIPTEN__
    if(fakeSocket!=NULL)
        fakeSocket->disconnectFromHost();
    if(sslSocket!=NULL)
        sslSocket->disconnectFromHost();
    if(tcpSocket!=NULL)
        tcpSocket->disconnectFromHost();
    #else
    webSocket->close();
    #endif
}

QAbstractSocket::SocketError ConnectedSocket::error() const
{
    #ifndef __EMSCRIPTEN__
    if(fakeSocket!=NULL)
        return fakeSocket->error();
    if(sslSocket!=NULL)
        return sslSocket->error();
    if(tcpSocket!=NULL)
        return tcpSocket->error();
    #else
    webSocket->error();
    #endif
    return QAbstractSocket::UnknownSocketError;
}

bool ConnectedSocket::flush()
{
    #ifndef __EMSCRIPTEN__
    if(fakeSocket!=NULL)
        return true;
    if(sslSocket!=NULL)
        return sslSocket->flush();
    if(tcpSocket!=NULL)
        return tcpSocket->flush();
    #else
    webSocket->flush();
    #endif
    return false;
}

bool ConnectedSocket::isValid() const
{
    #ifndef __EMSCRIPTEN__
    if(fakeSocket!=NULL)
        return fakeSocket->isValid();
    else if(sslSocket!=NULL)
        return sslSocket->isValid();
    else if(tcpSocket!=NULL)
        return tcpSocket->isValid();
    #else
    webSocket->isValid();
    #endif
    return false;
}

void ConnectedSocket::setTcpCork(const bool &cork)
{
    #ifndef __EMSCRIPTEN__
    #ifdef __linux__
    if(sslSocket!=NULL)
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
    if(tcpSocket!=NULL)
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

    #ifndef __EMSCRIPTEN__
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
    #ifndef __EMSCRIPTEN__
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
    //deprecated form incorrect value for i2p
    std::cerr << "ConnectedSocket::peerAddress(): deprecated form incorrect value for i2p" << std::endl;

    #ifndef __EMSCRIPTEN__
    if(fakeSocket!=NULL)
        return QHostAddress::LocalHost;
    if(sslSocket!=NULL)
        return sslSocket->peerAddress();
    if(tcpSocket!=NULL)
        return tcpSocket->peerAddress();
    #endif
    return QHostAddress::Null;
}

QString ConnectedSocket::peerName() const
{
    #ifndef __EMSCRIPTEN__
    /// \warning via direct value for i2p. Never pass by peerAddress()
    QString pearName;
    if(fakeSocket!=NULL)
        return QString();
    if(sslSocket!=NULL)
        pearName=sslSocket->peerName();
    if(tcpSocket!=NULL)
        pearName=tcpSocket->peerName();
    if(!pearName.isEmpty())
        return pearName;
    else
        return hostName;
    #else
    return QString();
    #endif
}

quint16	ConnectedSocket::peerPort() const
{
    #ifndef __EMSCRIPTEN__
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
    #ifndef __EMSCRIPTEN__
    if(fakeSocket!=NULL)
        return fakeSocket->state();
    if(sslSocket!=NULL)
        return sslSocket->state();
    if(tcpSocket!=NULL)
        return tcpSocket->state();
    #else
    webSocket->state();
    #endif
    return QAbstractSocket::UnconnectedState;
}

bool ConnectedSocket::waitForConnected(int msecs)
{
    #ifndef __EMSCRIPTEN__
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
    #ifndef __EMSCRIPTEN__
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
    #ifndef __EMSCRIPTEN__
    if(fakeSocket!=NULL)
        return fakeSocket->bytesAvailable();
    if(tcpSocket!=NULL)
        return tcpSocket->bytesAvailable();
    if(sslSocket!=NULL)
        return sslSocket->bytesAvailable();
        else
    #else
    return buffer.size();
    #endif
    return -1;
}

QIODevice::OpenMode ConnectedSocket::openMode() const
{
    #ifndef __EMSCRIPTEN__
    if(fakeSocket!=NULL)
        return fakeSocket->openMode();
    if(sslSocket!=NULL)
        return sslSocket->openMode();
    if(tcpSocket!=NULL)
        return tcpSocket->openMode();
    #endif
    return QIODevice::ReadWrite;
}

QString ConnectedSocket::errorString() const
{
    #ifndef __EMSCRIPTEN__
    if(fakeSocket!=NULL)
        return fakeSocket->errorString();
    if(sslSocket!=NULL)
        return sslSocket->errorString();
    if(tcpSocket!=NULL)
        return tcpSocket->errorString();
    #else
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
    #ifndef __EMSCRIPTEN__
    if(fakeSocket!=NULL)
        return fakeSocket->read(data,maxSize);
    if(tcpSocket!=NULL)
        return tcpSocket->read(data,maxSize);
    if(sslSocket!=NULL)
        return sslSocket->read(data,maxSize);
    #else
    QByteArray temp(buffer.mid(0,maxSize));
    buffer.remove(0,temp.size());
    memcpy(data,temp.constData(),temp.size());
    return temp.size();
    #endif
    return -1;
}

qint64 ConnectedSocket::writeData(const char * data, qint64 maxSize)
{
    #ifndef __EMSCRIPTEN__
    if(fakeSocket!=NULL)
        return fakeSocket->write(data,maxSize);
    if(tcpSocket!=NULL)
        return tcpSocket->write(data,maxSize);
    if(sslSocket!=NULL)
        return sslSocket->write(data,maxSize);
    #else
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
