#if ! defined(EPOLLCATCHCHALLENGERSERVER) && ! defined (ONLYMAPRENDER)

#include "ConnectedSocket.h"
#include <unistd.h>
#include <iostream>
#include <fcntl.h>

/*#include <netinet/tcp.h>
#include <netdb.h>*/

using namespace CatchChallenger;
#ifndef NOTCPSOCKET
#ifdef CATCHCHALLENGER_SOLO
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
#endif

ConnectedSocket::ConnectedSocket(QSslSocket *socket) :
    #ifdef CATCHCHALLENGER_SOLO
    fakeSocket(NULL),
    #endif
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
    #ifdef CATCHCHALLENGER_SOLO
    fakeSocket(NULL),
    #endif
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
    #ifdef CATCHCHALLENGER_SOLO
    fakeSocket=nullptr;
    #endif
    sslSocket=nullptr;
    tcpSocket=nullptr;
    #endif
}
#endif

ConnectedSocket::~ConnectedSocket()
{
    #ifndef NOTCPSOCKET
    if(sslSocket!=nullptr)
    {
        sslSocket->deleteLater();
        sslSocket=nullptr;
    }
    if(tcpSocket!=nullptr)
    {
        tcpSocket->deleteLater();
        tcpSocket=nullptr;
    }
    #ifdef CATCHCHALLENGER_SOLO
    if(fakeSocket!=nullptr)
    {
        fakeSocket->deleteLater();
        fakeSocket=nullptr;
    }
    #endif
    #endif
    #ifndef NOWEBSOCKET
    /*generate crash because delete in internal by Qt:
    Address 0x1fb0bac8 is 8 bytes inside a block of size 16 free'd
    at 0x483808B: operator delete(void*, unsigned long) (vg_replace_malloc.c:595)
    by 0x666F287: QObject::event(QEvent*) (qobject.cpp:1251)
    by 0x4D455A0: QApplicationPrivate::notify_helper(QObject*, QEvent*) (qapplication.cpp:3736)
    by 0x4D4CB3F: QApplication::notify(QObject*, QEvent*) (qapplication.cpp:3483)
    by 0x6646EA1: QCoreApplication::notifyInternal2(QObject*, QEvent*) (qcoreapplication.cpp:1060)
    by 0x664A006: QCoreApplicationPrivate::sendPostedEvents(QObject*, int, QThreadData*) (qcoreapplication.cpp:1799)
    by 0x6698672: postEventSourceDispatch(_GSource*, int (*)(void*), void*) (qeventdispatcher_glib.cpp:276)
    by 0x763CA0D: g_main_dispatch (gmain.c:3182)
    by 0x763CA0D: g_main_context_dispatch (gmain.c:3847)
    by 0x763CCA7: g_main_context_iterate.isra.26 (gmain.c:3920)
    by 0x763CD3B: g_main_context_iteration (gmain.c:3981)
    by 0x6698412: QEventDispatcherGlib::processEvents(QFlags<QEventLoop::ProcessEventsFlag>) (qeventdispatcher_glib.cpp:422)
    by 0x6645E7A: QEventLoop::exec(QFlags<QEventLoop::ProcessEventsFlag>) (qeventloop.cpp:225)
    if(webSocket!=nullptr)
    {
        webSocket->deleteLater();
        webSocket=nullptr;
    }*/
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
    #ifdef CATCHCHALLENGER_SOLO
    fakeSocket=nullptr;
    #endif
    #endif
    hostName.clear();
    port=0;
}

void ConnectedSocket::abort()
{
    hostName.clear();
    port=0;

    #ifndef NOTCPSOCKET
    #ifdef CATCHCHALLENGER_SOLO
    if(fakeSocket!=nullptr)
        fakeSocket->abort();
    #endif
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
    #ifdef CATCHCHALLENGER_SOLO
    if(fakeSocket!=nullptr)
        fakeSocket->connectToHost();
    else
    #endif
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
    #ifdef CATCHCHALLENGER_SOLO
    if(fakeSocket!=nullptr)
    {
        fakeSocket->connectToHost();
        return;
    }
    else
    #endif
    if(sslSocket!=nullptr)
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
    #ifdef CATCHCHALLENGER_SOLO
    if(fakeSocket!=nullptr)
        fakeSocket->disconnectFromHost();
    #endif
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
    #ifdef CATCHCHALLENGER_SOLO
    if(fakeSocket!=nullptr)
        return fakeSocket->error();
    #endif
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
    #ifdef CATCHCHALLENGER_SOLO
    if(fakeSocket!=nullptr)
        return true;
    #endif
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
    #ifdef CATCHCHALLENGER_SOLO
    if(fakeSocket!=nullptr)
        return fakeSocket->isValid();
    else
    #endif
    if(sslSocket!=nullptr)
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

QHostAddress ConnectedSocket::localAddress() const
{
    //deprecated form incorrect value for i2p
    std::cerr << "ConnectedSocket::localAddress(): deprecated form incorrect value for i2p" << std::endl;

    #ifndef NOTCPSOCKET
    #ifdef CATCHCHALLENGER_SOLO
    if(fakeSocket!=nullptr)
        return QHostAddress::LocalHost;
    #endif
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
    #ifdef CATCHCHALLENGER_SOLO
    if(fakeSocket!=nullptr)
        return 9999;
    #endif
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
    #ifdef CATCHCHALLENGER_SOLO
    if(fakeSocket!=nullptr)
        return QHostAddress::LocalHost;
    #endif
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
    #ifdef CATCHCHALLENGER_SOLO
    if(fakeSocket!=nullptr)
        return QString();
    #endif
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
    #ifdef CATCHCHALLENGER_SOLO
    if(fakeSocket!=nullptr)
        return 15000;
    #endif
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
    #ifdef CATCHCHALLENGER_SOLO
    if(fakeSocket!=nullptr)
        return fakeSocket->state();
    #endif
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
    #ifdef CATCHCHALLENGER_SOLO
    if(fakeSocket!=nullptr)
        return true;
    #endif
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
    #ifdef CATCHCHALLENGER_SOLO
    if(fakeSocket!=nullptr)
        return true;
    #endif
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
    #ifdef CATCHCHALLENGER_SOLO
    if(fakeSocket!=nullptr)
        return fakeSocket->bytesAvailable();
    #endif
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
    #ifdef CATCHCHALLENGER_SOLO
    if(fakeSocket!=nullptr)
        return fakeSocket->openMode();
    #endif
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
    #ifdef CATCHCHALLENGER_SOLO
    if(fakeSocket!=nullptr)
        return fakeSocket->errorString();
    #endif
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
    #ifdef CATCHCHALLENGER_SOLO
    if(fakeSocket!=nullptr)
        return fakeSocket->read(data,maxSize);
    #endif
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
    #ifdef CATCHCHALLENGER_SOLO
    if(fakeSocket!=nullptr)
        return fakeSocket->write(data,maxSize);
    #endif
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
