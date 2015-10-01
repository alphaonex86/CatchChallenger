#ifndef EPOLLCATCHCHALLENGERSERVER
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
    void disconnectFromFakeServer();
    void connectToHost();
    int64_t	bytesAvailable() const;
    void	close();
    bool isValid();
    QFakeSocket * getTheOtherSocket();
    uint64_t getRXSize();
    uint64_t getTXSize();
    QAbstractSocket::SocketError error() const;
    QAbstractSocket::SocketState state() const;
    bool isValid() const;
signals:
    void connected();
    void disconnected();
    void error(QAbstractSocket::SocketError socketError);
    void stateChanged(QAbstractSocket::SocketState socketState);
    void aboutToDelete();
protected:
    QFakeSocket *theOtherSocket;
    virtual bool isSequential() const;
    virtual bool canReadLine() const;
    virtual int64_t	readData(char * data, int64_t maxSize);
    virtual int64_t	writeData(const char * data, int64_t maxSize);
private:
    QByteArray data;
    QMutex mutex;
    void internal_writeData(QByteArray rawData);
    uint64_t RX_size;
    int64_t	bytesAvailableWithMutex();
};
}

#endif // QFAKESOCKET_H
#endif
