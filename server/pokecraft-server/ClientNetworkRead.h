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
	void error(const QString &error);
	void message(const QString &message);
	void sendPacket(const QByteArray &data);
	void isReadyToStop();
	//packet parsed (heavy)
	void askLogin(const quint8 &query_id,const QString &login,const QByteArray &hash);
	void askRandomSeedList(const quint8 &query_id);
	void datapackList(const quint8 &query_id,const QStringList &files,const QList<quint32> &timestamps);
	//packet parsed (map management)
	void moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction);
	//packet parsed (broadcast)
	void sendPM(const QString &text,const QString &pseudo);
	void sendChatText(const Chat_type &chatType,const QString &text);
	void addPlayersInformationToWatch(const QList<quint32> &player_ids,const quint8 &type_player_query);
	void removePlayersInformationToWatch(const QList<quint32> &player_ids);
	void sendBroadCastCommand(const QString &command,const QString &extraText);
	//to manipulate the server for restart and stop
	void serverCommand(const QString &command,const QString &extraText);
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
