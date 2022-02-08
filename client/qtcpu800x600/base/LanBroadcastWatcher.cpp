#include "LanBroadcastWatcher.h"
#include <QDateTime>
#include <QNetworkDatagram>
#include <QByteArray>
#include <QDataStream>

LanBroadcastWatcher *LanBroadcastWatcher::lanBroadcastWatcher=nullptr;

LanBroadcastWatcher::LanBroadcastWatcher(QObject *parent) :
    QObject(parent)
{
    udpSocket = new QUdpSocket(this);
    if(!udpSocket->bind(42490, QUdpSocket::ShareAddress))
        abort();
    //bind(QHostAddress::Any, 42490);
    if(!connect(udpSocket, &QUdpSocket::readyRead,
            this, &LanBroadcastWatcher::processPendingDatagrams))
        abort();
    if(!connect(&cleanTimer,&QTimer::timeout,this,&LanBroadcastWatcher::clean))
        abort();
    cleanTimer.start(500);
}

void LanBroadcastWatcher::processPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = udpSocket->receiveDatagram();
        ServerEntry e;
        e.server=datagram.senderAddress();
        QByteArray replyData = datagram.data();
        QDataStream stream(&replyData,QIODevice::ReadOnly);
        stream.setVersion(QDataStream::Qt_4_4);
        stream >> e.name;
        e.port=0;
        stream >> e.port;
        e.lastseen=QDateTime::currentSecsSinceEpoch();
        e.uniqueKey=e.name;

        int index=0;
        while(index<list.size())
        {
            const ServerEntry &l=list.at(index);
            if(e.server==l.server && e.port==l.port)
            {
                list[index]=e;
                break;
            }
            index++;
        }
        if(index>=list.size())
        {
            if(list.size()<16)
            {
                list.push_back(e);
                emit newServer();
            }
        }
    }
}

QList<LanBroadcastWatcher::ServerEntry> LanBroadcastWatcher::getLastServerList()
{
    int index=0;
    while(index<list.size())
    {
        if(list.at(index).lastseen<(QDateTime::currentSecsSinceEpoch()-3))
            list.removeAt(index);
        else
            index++;
    }
    return list;
}

void LanBroadcastWatcher::clean()
{
    bool change=false;
    int index=0;
    while(index<list.size())
    {
        if(list.at(index).lastseen<(QDateTime::currentSecsSinceEpoch()-3))
        {
            change=true;
            list.removeAt(index);
        }
        else
            index++;
    }
    if(change)
        emit newServer();
}
