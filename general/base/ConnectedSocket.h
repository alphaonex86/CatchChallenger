#ifndef CATCHCHALLENGER_CONNECTEDSOCKET_H
#define CATCHCHALLENGER_CONNECTEDSOCKET_H

#if ! defined(EPOLLCATCHCHALLENGERSERVER) && ! defined (ONLYMAPRENDER)

#include <QIODevice>
#include <QSslSocket>
#include <QAbstractSocket>
#include <QObject>
#include <QHostAddress>
#include <QByteArray>

#include "QFakeSocket.h"

namespace CatchChallenger {

class ConnectedSocket : public QIODevice
{
    Q_OBJECT
public:
    explicit ConnectedSocket(QFakeSocket *socket);
    explicit ConnectedSocket(QSslSocket *socket);
    explicit ConnectedSocket(QTcpSocket *socket);
    ~ConnectedSocket();
    void	abort();
    void	connectToHost(const QString & hostName,quint16 port);
    void	connectToHost(const QHostAddress & address,quint16 port);
    void	disconnectFromHost();
    QAbstractSocket::SocketError error() const;
    bool	flush();
    bool	isValid() const;
    void    setTcpCork(const bool &cork);
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
    QFakeSocket *fakeSocket;
    QSslSocket *sslSocket;
    QTcpSocket *tcpSocket;
protected:
    bool	isSequential() const;
    bool canReadLine() const;
    QList<QSslError> sslErrors() const;
signals:
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

#endif
