#ifndef EPOLLCATCHCHALLENGERSERVER
#ifndef CATCHCHALLENGER_QFAKESERVER_H
#define CATCHCHALLENGER_QFAKESERVER_H

#include <QObject>
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
    std::vector<std::pair<QFakeSocket *,QFakeSocket *> > m_listOfConnexion;
    std::vector<std::pair<QFakeSocket *,QFakeSocket *> > m_pendingConnection;
protected:
    //from the server
    void addPendingConnection(QFakeSocket * socket);
public slots:
    void disconnectedSocket();
};
}

#endif // QFAKESERVER_H
#endif
