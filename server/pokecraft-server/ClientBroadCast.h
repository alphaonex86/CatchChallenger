#ifndef CLIENTBROADCAST_H
#define CLIENTBROADCAST_H

#include <QObject>
#include <QDataStream>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QSemaphore>

#include "ServerStructures.h"
#include "../pokecraft-general/DebugClass.h"
#include "../pokecraft-general/GeneralStructures.h"
#include "../VariableServer.h"

class ClientBroadCast : public QObject
{
	Q_OBJECT
public:
	explicit ClientBroadCast();
	~ClientBroadCast();
	// general info
	QString pseudo;
	Player_private_and_public_informations player_informations;
	quint32 player_id;
	QList<quint32> player_ids_to_watch;
	// set the variable
	void setVariable(quint16 *connected_players,quint16 *max_players,QList<ClientBroadCast *> *clientBroadCastList);
	void disconnect();
public slots:
	//global slot
	void receivePM(QString text,quint32 player_id);
	void sendPM(QString text,QString pseudo);
	void receiveChatText(Chat_type chatType,QString text,quint32 sender_player_id);
	void sendChatText(Chat_type chatType,QString text);
	void receive_instant_player_number();
	void receiveBroadCastCommand(QString command,QString extraText,QString sender_pseudo);
	void sendBroadCastCommand(QString command,QString extraText);
	//after login
	void send_player_informations(Player_private_and_public_informations player_informations);
	//normal slots
	void askIfIsReadyToStop();
	void sendSystemMessage(QString text,bool important=false);
	//player watching
	void addPlayersInformationToWatch(QList<quint32> player_ids,quint8 type_player_query);
	void removePlayersInformationToWatch(QList<quint32> player_ids);
	//void receiveQueryPlayersInformation(QList<quint32> player_ids);
	void askPlayersInformation(QList<quint32> player_ids);
	//relayed signals
	void send_instant_player_number();
signals:
	//normal signals
	void error(QString error);
	void kicked();
	void message(QString message);
	void sendPacket(QByteArray data);
	void isReadyToStop();
	void try_internal_disconnect();
private:
	//player login
	void send_players_informations(QList<Player_private_and_public_informations> players_informations);
	//local data
	quint16 *connected_players;
	quint16 *max_players;
	QList<Player_private_and_public_informations> players_informations_to_push;
	QList<ClientBroadCast *> *clientBroadCastList;
	QSemaphore disconnection;
private slots:
	void receivePlayersInformation(Player_private_and_public_informations player_informations);
	void internal_disconnect();
};

#endif // CLIENTBROADCAST_H
