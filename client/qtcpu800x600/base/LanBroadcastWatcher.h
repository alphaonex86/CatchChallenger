#ifndef LANBROADCASTWATCHER_H
#define LANBROADCASTWATCHER_H

#include <QUdpSocket>
#include <QTimer>

class LanBroadcastWatcher : public QObject
{
    Q_OBJECT
public:
    static LanBroadcastWatcher *lanBroadcastWatcher;
    explicit LanBroadcastWatcher(QObject *parent = nullptr);
    struct ServerEntry
    {
        QString name;
        QHostAddress server;
        uint16_t port;
        uint64_t lastseen;//only for internal usage
        QString uniqueKey;
    };
    QList<ServerEntry> getLastServerList();
    void processPendingDatagrams();
    void clean();
private:
    QList<ServerEntry> list;
    QUdpSocket *udpSocket = nullptr;
    QTimer cleanTimer;
signals:
    void newServer();
};

#endif // LANBROADCASTWATCHER_H
