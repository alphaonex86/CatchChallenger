#include "LanBroadcastWatcher.hpp"
#include <QDateTime>
#include <QNetworkDatagram>
#include <QByteArray>
#include <QDataStream>
#include <QCryptographicHash>

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
        /* can't contain incorrect char for path!
         * example: toto's server
         * on windows ' is not supported */
        e.uniqueKey=QString(QCryptographicHash::hash(e.name.toUtf8(), QCryptographicHash::Sha224).toHex());

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
        if(list.at(index).lastseen<((uint64_t)QDateTime::currentSecsSinceEpoch()-3))
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
        if(list.at(index).lastseen<((uint64_t)QDateTime::currentSecsSinceEpoch()-3))
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
