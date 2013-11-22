#ifndef CATCHCHALLENGER_QFAKESOCKET_H
#define CATCHCHALLENGER_QFAKESOCKET_H

#include <QAbstractSocket>
#include <QMutex>
#include <QByteArray>
#include <QIODevice>

namespace CatchChallenger {
class QFakeSocket : public QIODevice
{
    Q_OBJECT
public:
    friend class QFakeServer;
//	friend class QFakeSocket;
    explicit QFakeSocket();
    ~QFakeSocket();
    void abort();
    void disconnectFromHost();
    void connectToHost();
    qint64	bytesAvailable() const;
    void	close();
    bool isValid();
    QFakeSocket * getTheOtherSocket();
    quint64 getRXSize();
    quint64 getTXSize();
    QAbstractSocket::SocketError error() const;
    QAbstractSocket::SocketState state() const;
    bool isValid() const;
signals:
    void	connected();
    void	disconnected();
    void	error(QAbstractSocket::SocketError socketError);
    void	stateChanged(QAbstractSocket::SocketState socketState);
protected:
    QFakeSocket *theOtherSocket;
    virtual bool isSequential() const;
    virtual bool canReadLine() const;
    virtual qint64	readData(char * data, qint64 maxSize);
    virtual qint64	writeData(const char * data, qint64 maxSize);
private:
    QByteArray data;
    QMutex mutex;
    void internal_writeData(QByteArray rawData);
    quint64 RX_size;
    qint64	bytesAvailableWithMutex();
};
}

#endif // QFAKESOCKET_H
