#ifndef CATCHCHALLENGER_CONNECTEDSOCKET_H
#define CATCHCHALLENGER_CONNECTEDSOCKET_H

#ifndef EPOLLCATCHCHALLENGERSERVER

#include <QIODevice>
#include <QSslSocket>
#include <QAbstractSocket>
#include <QObject>
#include <QHostAddress>
#include <std::vector<char>>

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
    void	connectToHost(const std::string & hostName,uint16_t port);
    void	connectToHost(const QHostAddress & address,uint16_t port);
    void	disconnectFromHost();
    QAbstractSocket::SocketError error() const;
    bool	flush();
    bool	isValid() const;
    void    setTcpCork(const bool &cork);
    QHostAddress localAddress() const;
    uint16_t	localPort() const;
    QHostAddress peerAddress() const;
    std::string	peerName() const;
    uint16_t	peerPort() const;
    QAbstractSocket::SocketState state() const;
    bool	waitForConnected(int msecs = 30000);
    bool	waitForDisconnected(int msecs = 30000);
    int64_t	bytesAvailable() const;
    OpenMode openMode() const;
    std::string errorString() const;
    int64_t	readData(char * data, int64_t maxSize);
    int64_t	writeData(const char * data, int64_t maxSize);
    void	close();
    QFakeSocket *fakeSocket;
    QSslSocket *sslSocket;
    QTcpSocket *tcpSocket;
protected:
    bool	isSequential() const;
    bool canReadLine() const;
    std::vector<QSslError> sslErrors() const;
signals:
    void	connected();
    void	disconnected();
    void	error(QAbstractSocket::SocketError socketError);
    void	stateChanged(QAbstractSocket::SocketState socketState);
    void    sslErrors(const std::vector<QSslError> &errors);
private:
    void destroyedSocket();
    void purgeBuffer();
    //void startHandshake();
};
}
#endif

#endif
