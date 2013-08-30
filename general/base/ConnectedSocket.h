#ifndef CATCHCHALLENGER_CONNECTEDSOCKET_H
#define CATCHCHALLENGER_CONNECTEDSOCKET_H

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
    explicit ConnectedSocket(QFakeSocket *socket,QObject *parent = 0);
    explicit ConnectedSocket(QSslSocket *socket,QObject *parent = 0);
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
    void	close();
    QFakeSocket *fakeSocket;
    QSslSocket *sslSocket;
protected:
    virtual bool	isSequential() const;
    virtual bool canReadLine() const;
    virtual qint64	readData(char * data, qint64 maxSize);
    virtual qint64	writeData(const char * data, qint64 maxSize);
signals:
    void	connected();
    void	disconnected();
    void	error(QAbstractSocket::SocketError socketError);
    void	stateChanged(QAbstractSocket::SocketState socketState);
private slots:
    void destroyedSocket();
};
}

#endif
