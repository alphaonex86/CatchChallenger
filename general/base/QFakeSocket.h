#ifndef QFAKESOCKET_H
#define QFAKESOCKET_H

#include <QAbstractSocket>
#include <QMutex>
#include <QByteArray>

namespace Pokecraft {
class QFakeSocket : public QAbstractSocket
{
	Q_OBJECT
public:
	friend class QFakeServer;
//	friend class QFakeSocket;
	explicit QFakeSocket();
	void abort();
	void disconnectFromHostImplementation();
	void connectToHostImplementation();
	qint64	bytesAvailable () const;
	qint64	bytesToWrite () const;
	bool	canReadLine () const;
	void	close ();
	bool	waitForBytesWritten ( int msecs = 30000 );
	bool	waitForReadyRead ( int msecs = 30000 );
	bool isValid();
	QFakeSocket * getTheOtherSocket();
	quint64 getRXSize();
	quint64 getTXSize();
protected:
	qint64	readData ( char * rawData, qint64 maxSize );
	qint64	writeData(const char * rawData, qint64 size);
/*signals:
	void	connected();
	void	disconnected();
	void	error ( QAbstractSocket::SocketError socketError );
	void	stateChanged ( QAbstractSocket::SocketState socketState );
	void	readyRead();*/
protected:
	QFakeSocket *theOtherSocket;
private:
	QByteArray data;
	QMutex mutex;
	void internal_writeData(QByteArray rawData);
	quint64 RX_size;
	qint64	bytesAvailableWithMutex();
};
}

#endif // QFAKESOCKET_H
