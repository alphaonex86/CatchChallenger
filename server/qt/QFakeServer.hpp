#if ! defined(EPOLLCATCHCHALLENGERSERVER) && ! defined (ONLYMAPRENDER) && defined(CATCHCHALLENGER_SOLO)
#ifndef CATCHCHALLENGER_QFAKESERVER_H
#define CATCHCHALLENGER_QFAKESERVER_H

#include <QObject>
#include <QPair>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <vector>

#include "../../general/base/lib.h"

class QFakeSocket;

class DLL_PUBLIC QFakeServer : public QObject
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

#endif // QFAKESERVER_H
#endif
