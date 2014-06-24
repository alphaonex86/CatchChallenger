/** \file LocalListener.cpp
\brief The have local server, to have unique instance, and send arguments to the current running instance
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "LocalListener.h"

#include <QLocalSocket>
#include <QMessageBox>
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
        QLocalServer::removeServer(ExtraSocket::pathSocket(QStringLiteral("CatchChallenger-Client-%1").arg(count)));
    }
}

bool LocalListener::tryListen()
{
    count=0;
    while(count<16)
    {
        QLocalSocket localSocket;
        localSocket.connectToServer(ExtraSocket::pathSocket(QStringLiteral("CatchChallenger-Client-%1").arg(count)),QIODevice::WriteOnly);
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

void LocalListener::listenServer(const quint8 &count)
{
    QLocalServer::removeServer(ExtraSocket::pathSocket(QStringLiteral("CatchChallenger-Client-%1").arg(count)));
    #ifndef Q_OS_MAC
    localServer.setSocketOptions(QLocalServer::UserAccessOption);
    #endif
    if(localServer.listen(ExtraSocket::pathSocket(QStringLiteral("CatchChallenger-Client-%1").arg(count))))
        connect(&localServer, &QLocalServer::newConnection, this, &LocalListener::newConnexion);
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
        localSocket.connectToServer(ExtraSocket::pathSocket(QStringLiteral("CatchChallenger-Client-%1").arg(count)),QIODevice::WriteOnly);
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
        connect(socket, &QLocalSocket::readyRead, this, &LocalListener::dataIncomming);
        connect(socket, &QLocalSocket::disconnected, this, &LocalListener::deconnectClient);
    }
}
