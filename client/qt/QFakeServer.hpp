#if ! defined(EPOLLCATCHCHALLENGERSERVER) && ! defined (ONLYMAPRENDER)
#ifndef CATCHCHALLENGER_QFAKESERVER_H
#define CATCHCHALLENGER_QFAKESERVER_H
#ifdef CATCHCHALLENGER_SOLO

#include <QObject>
#include <QPair>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <vector>

namespace CatchChallenger {
class QFakeSocket;

class QFakeServer : public QObject
{
    Q_OBJECT
private:
    explicit QFakeServer();
public:
    friend class QFakeSocket;
    static QFakeServer server;

    virtual bool hasPendingConnections();
    bool isListening() const;
    bool listen();
    virtual QFakeSocket * nextPendingConnection();
    void close();
signals:
    void newConnection();
private:
    QMutex mutex;
    bool m_isListening;
    QList<QPair<QFakeSocket *,QFakeSocket *> > m_listOfConnexion;
    QList<QPair<QFakeSocket *,QFakeSocket *> > m_pendingConnection;
protected:
    //from the server
    void addPendingConnection(QFakeSocket * socket);
public slots:
    void disconnectedSocket();
};
}

#endif
#endif // QFAKESERVER_H
#endif
