#ifndef CATCHCHALLENGER_CONNECTEDSOCKET_H
#define CATCHCHALLENGER_CONNECTEDSOCKET_H

#include <QIODevice>
#include <QSslSocket>
#include <QAbstractSocket>
#include <QObject>
#include <QHostAddress>
#include <QByteArray>

#include "QFakeSocket.h"

#ifdef EPOLLCATCHCHALLENGERSERVER
    #ifndef SERVERNOSSL
        #include "../../server/epoll/EpollSslClient.h"
    #else
        #include "../../server/epoll/EpollClient.h"
    #endif
#endif

namespace CatchChallenger {

class ConnectedSocket : public QIODevice
{
    Q_OBJECT
public:
    explicit ConnectedSocket(QFakeSocket *socket);
    explicit ConnectedSocket(QSslSocket *socket);
    ~ConnectedSocket();
    void	abort();
    void	connectToHost(const QString & hostName,quint16 port);
    void	connectToHost(const QHostAddress & address,quint16 port);
    void	disconnectFromHost();
    QAbstractSocket::SocketError error() const;
    bool	flush();
    bool	isValid() const;
    QHostAddress localAddress() const;
    quint16	localPort() const;
    QHostAddress peerAddress() const;
    QString	peerName() const;
    quint16	peerPort() const;
    QAbstractSocket::SocketState state() const;
    bool	waitForConnected(int msecs = 30000);
    bool	waitForDisconnected(int msecs = 30000);
    qint64	bytesAvailable() const;
    OpenMode openMode() const;
    QString errorString() const;
    void	close();
    #ifdef EPOLLCATCHCHALLENGERSERVER
    #ifndef SERVERNOSSL
    EpollSslClient *epollSocket;
    explicit ConnectedSocket(EpollSslClient *socket);
    #else
    EpollClient *epollSocket;
    explicit ConnectedSocket(EpollClient *socket);
    #endif
    #endif
    QFakeSocket *fakeSocket;
    QSslSocket *sslSocket;
    QByteArray tempClearData;
protected:
    virtual bool	isSequential() const;
    virtual bool canReadLine() const;
    virtual qint64	readData(char * data, qint64 maxSize);
    virtual qint64	writeData(const char * data, qint64 maxSize);
    void sslErrors(const QList<QSslError> &errors);
signals:
    void	connected();
    void	disconnected();
    void	error(QAbstractSocket::SocketError socketError);
    void	stateChanged(QAbstractSocket::SocketState socketState);
private slots:
    void destroyedSocket();
    void encrypted();
    //void startHandshake();
};
}

#endif
