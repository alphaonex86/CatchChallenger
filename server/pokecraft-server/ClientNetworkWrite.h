#ifndef CLIENTNETWORKWRITE_H
#define CLIENTNETWORKWRITE_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>

class ClientNetworkWrite : public QObject
{
    Q_OBJECT
public:
	explicit ClientNetworkWrite();
	~ClientNetworkWrite();
	void setSocket(QTcpSocket * socket);
public slots:
	void sendPacket(QByteArray data);
	//normal slots
	void askIfIsReadyToStop();
private:
	QTcpSocket * socket;
	QByteArray block;
	qint64 byteWriten;
signals:
	void isReadyToStop();
	void fake_send_data(QByteArray data);
	void error(QString error);
	void message(QString message);
};

#endif // CLIENTNETWORKWRITE_H
