#ifndef QFAKESOCKET_H
#define QFAKESOCKET_H

#include <QAbstractSocket>
#include <QMutex>
#include <QByteArray>

class QFakeSocket : public QAbstractSocket
{
	Q_OBJECT
public:
	friend class QFakeServer;
//	friend class QFakeSocket;
	explicit QFakeSocket(QObject *parent = 0);
	void abort();
	void disconnectFromHost();
	void connectToHost();
	bool	atEnd ();
	qint64	bytesAvailable ();
	qint64	bytesToWrite () const;
	bool	canReadLine () const;
	void	close ();
	bool	isSequential () const;
	bool	waitForBytesWritten ( int msecs = 30000 );
	bool	waitForReadyRead ( int msecs = 30000 );
	bool isValid();
	QFakeSocket * getTheOtherSocket();
protected:
	qint64	readData ( char * data, qint64 maxSize );
	qint64	readLineData(char * data, qint64 maxlen );
	qint64	writeData(const char * data, qint64 size);
signals:
	void	connected();
	void	disconnected();
	void	error ( QAbstractSocket::SocketError socketError );
	void	stateChanged ( QAbstractSocket::SocketState socketState );
	void	readyRead();
protected:
	QFakeSocket *theOtherSocket;
private:
	QMutex mutex;
	QByteArray data;
	void internal_writeData(QByteArray data);
};

#endif // QFAKESOCKET_H
