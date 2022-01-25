#ifndef CATCHCHALLENGER_CONNECTEDSOCKET_H
#define CATCHCHALLENGER_CONNECTEDSOCKET_H

#if ! defined(EPOLLCATCHCHALLENGERSERVER) && ! defined (ONLYMAPRENDER)

#include <QIODevice>
#include <QHostAddress>
#ifndef NOTCPSOCKET
    #include <QSslSocket>
    #include <QAbstractSocket>
    #include "../../server/qt/QFakeSocket.hpp"
#endif
#ifndef NOWEBSOCKET
    #include <QtWebSockets/QWebSocket>
#endif
#include <QObject>
#include <QByteArray>

namespace CatchChallenger {

class ConnectedSocket : public QIODevice
{
    Q_OBJECT
public:
    #ifndef NOTCPSOCKET
    explicit ConnectedSocket(QFakeSocket *socket);
    explicit ConnectedSocket(QSslSocket *socket);
    explicit ConnectedSocket(QTcpSocket *socket);
    #endif
    #ifndef NOWEBSOCKET
    explicit ConnectedSocket(QWebSocket *socket);
    #endif
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
    #ifndef NOTCPSOCKET
    QFakeSocket *fakeSocket;
    QSslSocket *sslSocket;
    QTcpSocket *tcpSocket;
    #endif
    #ifndef NOWEBSOCKET
    QWebSocket *webSocket;
    #endif
protected:
    bool	isSequential() const;
    bool canReadLine() const;
    #ifndef __EMSCRIPTEN__
        QList<QSslError> sslErrors() const;
    #endif
    #ifndef NOWEBSOCKET
        QByteArray buffer;
        #ifndef __EMSCRIPTEN__
            QList<QSslError> m_sslErrors;
            void saveSslErrors(const QList<QSslError> &errors);
        #endif
    #endif
    //workaround because QSslSocket don't return correct value for i2p via proxy
    QString hostName;
    uint16_t port;
    #ifndef NOWEBSOCKET
    void binaryMessageReceived(const QByteArray &message);
    #endif
signals:
    void	connected();
    void	disconnected();
    void	error(QAbstractSocket::SocketError socketError);
    void	stateChanged(QAbstractSocket::SocketState socketState);
    #ifndef __EMSCRIPTEN__
    void    sslErrors(const QList<QSslError> &errors);
    #endif
private:
    void destroyedSocket();
    void purgeBuffer();
    //void startHandshake();
};
}
#endif

#endif
