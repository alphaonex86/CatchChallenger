#ifndef CLIENTNETWORKREAD_H
#define CLIENTNETWORKREAD_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QStringList>

#include "ServerStructures.h"
#include "../pokecraft-general/DebugClass.h"

class ClientNetworkRead : public QObject
{
    Q_OBJECT
public:
	explicit ClientNetworkRead();
	void setSocket(QTcpSocket * socket);
	void setPlayerNumber(quint16 * current_player_number,quint16 * max_player_number);
	void stopRead();
	void fake_send_protocol();
public slots:
	void readyRead();
	void send_player_informations(Player_private_and_public_informations player_informations);
	void fake_receive_data(QByteArray data);
	//normal slots
	void askIfIsReadyToStop();
private slots:
	void parseInput(const QByteArray & inputData);
	void parseInputAfterLogin(const QByteArray & inputData);
signals:
	//normal signals
	void error(QString error);
	void message(QString message);
	void sendPacket(QByteArray data);
	void isReadyToStop();
	//packet parsed (heavy)
	void askLogin(quint8 query_id,QString login,QByteArray hash);
	void askRandomSeedList(quint8 query_id);
	void datapackList(quint8 query_id,QStringList files,QList<quint32> timestamps);
	//packet parsed (map management)
	void moveThePlayer(quint8 previousMovedUnit,Direction direction);
	//packet parsed (broadcast)
	void sendPM(QString text,QString pseudo);
	void sendChatText(Chat_type chatType,QString text);
	void addPlayersInformationToWatch(QList<quint32> player_ids,quint8 type_player_query);
	void removePlayersInformationToWatch(QList<quint32> player_ids);
	void sendBroadCastCommand(QString command,QString extraText);
	//to manipulate the server for restart and stop
	void serverCommand(QString command,QString extraText);
private:
	// for data
	bool haveData;
	quint32 dataSize;
	QByteArray data;
	// for status
	bool is_logged;
	bool have_send_protocol;
	bool is_logging_in_progess;
	Player_private_and_public_informations player_informations;
	bool stopIt;
	// for max player
	quint16 * current_player_number;
	quint16 * max_player_number;
	// function
	void dataClear();
	bool checkStringIntegrity(const QByteArray & data);
	QTcpSocket * socket;
};

#endif // CLIENTNETWORKREAD_H
