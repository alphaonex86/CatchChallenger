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
#include <iostream>

LocalListener::LocalListener(QObject *parent) :
    QObject(parent)
{
    controlSocket=nullptr;
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
    std::cerr << "Too many instance open" << std::endl;
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
    QLocalSocket *socket = qobject_cast<QLocalSocket *>(sender());
    if(socket==nullptr)
        socket=controlSocket;
    if(socket==nullptr)
        return;
    //remember this socket so replies/events (STATE/INVENTORY/chat) go back to
    //the controller that is driving us
    controlSocket=socket;
    controlBuffer.append(socket->readAll());
    //legacy single-instance signal: a 0x00 byte means "another instance asked
    //me to quit" (see quitAllRunningInstance()).
    if(controlBuffer.contains('\0'))
    {
        QCoreApplication::quit();
        return;
    }
    //otherwise consume complete newline-terminated text commands
    int newlineIndex=controlBuffer.indexOf('\n');
    while(newlineIndex>=0)
    {
        const QString line=QString::fromUtf8(controlBuffer.left(newlineIndex)).trimmed();
        controlBuffer.remove(0,newlineIndex+1);
        if(!line.isEmpty())
        {
            if(line.compare(QStringLiteral("quit"),Qt::CaseInsensitive)==0)
            {
                QCoreApplication::quit();
                return;
            }
            emit actionReceived(line);
        }
        newlineIndex=controlBuffer.indexOf('\n');
    }
}

void LocalListener::sendReply(const QString &line)
{
    if(controlSocket!=nullptr && controlSocket->state()==QLocalSocket::ConnectedState)
    {
        controlSocket->write(line.toUtf8()+'\n');
        controlSocket->flush();
    }
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
    if(controlSocket==socket)
    {
        controlSocket=nullptr;
        controlBuffer.clear();
    }
    socket->deleteLater();
}

void LocalListener::newConnexion()
{
    QLocalSocket *socket = localServer.nextPendingConnection();
    if(socket==nullptr)
        return;
    //Route every connection through dataIncomming(): it handles the legacy
    //single-instance quit (0x00 / "quit") AND the new text command channel, so
    //a controller can both quit us and drive us with KEY/CLICK/GET commands.
    controlSocket=socket;
    if(!connect(socket, &QLocalSocket::readyRead, this, &LocalListener::dataIncomming))
        abort();
    if(!connect(socket, &QLocalSocket::disconnected, this, &LocalListener::deconnectClient))
        abort();
    if(socket->bytesAvailable()>0)
        dataIncomming();
}
