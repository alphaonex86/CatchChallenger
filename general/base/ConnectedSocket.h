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
    #ifndef EPOLLCATCHCHALLENGERSERVER
    Q_OBJECT
    #endif
public:
    #ifndef EPOLLCATCHCHALLENGERSERVER
    explicit ConnectedSocket(QFakeSocket *socket);
    explicit ConnectedSocket(QSslSocket *socket);
    explicit ConnectedSocket(QTcpSocket *socket);
    #endif
    ~ConnectedSocket();
    void	abort();
    #ifndef EPOLLCATCHCHALLENGERSERVER
    void	connectToHost(const QString & hostName,quint16 port);
    void	connectToHost(const QHostAddress & address,quint16 port);
    #endif
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
    qint64	readData(char * data, qint64 maxSize);
    qint64	writeData(const char * data, qint64 maxSize);
    void	close();
    #ifdef EPOLLCATCHCHALLENGERSERVER
            #ifndef SERVERNOSSL
            EpollSslClient *epollSocket;
            explicit ConnectedSocket(EpollSslClient *socket);
        #else
            EpollClient *epollSocket;
            explicit ConnectedSocket(EpollClient *socket);
        #endif
    #else
    QFakeSocket *fakeSocket;
    QSslSocket *sslSocket;
    QTcpSocket *tcpSocket;
    #endif
protected:
    bool	isSequential() const;
    bool canReadLine() const;
    QList<QSslError> sslErrors() const;
#ifndef EPOLLCATCHCHALLENGERSERVER
signals:
#else
public:
#endif
    void	connected();
    void	disconnected();
    void	error(QAbstractSocket::SocketError socketError);
    void	stateChanged(QAbstractSocket::SocketState socketState);
    void    sslErrors(const QList<QSslError> &errors);
private:
    void destroyedSocket();
    void purgeBuffer();
    //void startHandshake();
};
}

#endif
