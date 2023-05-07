/** \file LocalListener.cpp
\brief The have local server, to have unique instance, and send arguments to the current running instance
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "LocalListener.hpp"
#include "ExtraSocket.hpp"

#include <QLocalSocket>
#include <QCoreApplication>

LocalListener::LocalListener(QObject *parent) :
    QObject(parent)
{
    count=16;
}

LocalListener::~LocalListener()
{
    if(localServer.isListening())
    {
        localServer.close();
        QLocalServer::removeServer(QString::fromStdString(ExtraSocket::pathSocket("CatchChallenger-Client-"+std::to_string(count))));
    }
}

bool LocalListener::tryListen()
{
    count=0;
    while(count<16)
    {
        QLocalSocket localSocket;
        localSocket.connectToServer(
                    QString::fromStdString(ExtraSocket::pathSocket("CatchChallenger-Client-"+std::to_string(count))),QIODevice::WriteOnly);
        if(localSocket.waitForConnected(1000) && localSocket.isValid())
        {}
        else
        {
            listenServer(count);
            return true;
        }
        count++;
    }
    qDebug() << "Too many instance open";
    QCoreApplication::quit();
    return false;
}

void LocalListener::listenServer(const uint8_t &count)
{
    QLocalServer::removeServer(QString::fromStdString(ExtraSocket::pathSocket("CatchChallenger-Client-"+std::to_string(count))));
    #ifndef Q_OS_MAC
    localServer.setSocketOptions(QLocalServer::UserAccessOption);
    #endif
    if(localServer.listen(QString::fromStdString(ExtraSocket::pathSocket("CatchChallenger-Client-"+std::to_string(count)))))
        if(!connect(&localServer, &QLocalServer::newConnection, this, &LocalListener::newConnexion))
            abort();
}

void LocalListener::dataIncomming()
{
    QCoreApplication::quit();
}

void LocalListener::quitAllRunningInstance()
{
    QByteArray data;
    data[0]=0x00;
    int count=0;
    while(count<16)
    {
        QLocalSocket localSocket;
        localSocket.connectToServer(QString::fromStdString(ExtraSocket::pathSocket(
                           "CatchChallenger-Client-"+std::to_string(count))),QIODevice::WriteOnly);
        if(localSocket.waitForConnected(1000) && localSocket.isValid())
        {
            localSocket.write(data);
            localSocket.waitForBytesWritten(1000);
        }
        count++;
    }
}

void LocalListener::deconnectClient()
{
    QLocalSocket *socket = qobject_cast<QLocalSocket *>(sender());
    if (socket == 0)
        return;
    socket->deleteLater();
}

void LocalListener::newConnexion()
{
    QLocalSocket *socket = localServer.nextPendingConnection();
    if(socket->bytesAvailable()>0)
        QCoreApplication::quit();
    else
    {
        if(!connect(socket, &QLocalSocket::readyRead, this, &LocalListener::dataIncomming))
            abort();
        if(!connect(socket, &QLocalSocket::disconnected, this, &LocalListener::deconnectClient))
            abort();
    }
}
