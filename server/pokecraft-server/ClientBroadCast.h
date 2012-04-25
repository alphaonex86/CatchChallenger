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

/// \warning here, 0 random should be call
/// \warning here only the call with global scope need be do
class ClientBroadCast : public QObject
{
	Q_OBJECT
public:
	explicit ClientBroadCast();
	~ClientBroadCast();
	// general info
	QList<quint32> player_ids_to_watch;
	Player_private_and_public_informations *player_informations;
	// set the variable
	void setVariable(GeneralData *generalData,Player_private_and_public_informations *player_informations);
	void disconnect();
	//cache
	QString pseudo;
	quint32 player_id;
public slots:
	//global slot
	void receivePM(const QString &text,const quint32 &player_id);
	void sendPM(const QString &text,const QString &pseudo);
	void receiveChatText(const Chat_type &chatType,const QString &text,const quint32 &sender_player_id);
	void sendChatText(const Chat_type &chatType,const QString &text);
	void receive_instant_player_number(qint32 connected_players);
	void kick();
	void sendBroadCastCommand(const QString &command,const QString &extraText);
	//after login
	void send_player_informations();
	//normal slots
	void askIfIsReadyToStop();
	void sendSystemMessage(const QString &text,const bool &important=false);
	//player watching
	void addPlayersInformationToWatch(const QList<quint32> &player_ids,const quint8 &type_player_query);
	void removePlayersInformationToWatch(const QList<quint32> &player_ids);
	//void receiveQueryPlayersInformation(QList<quint32> player_ids);
	void askPlayersInformation(const QList<quint32> &player_ids);
signals:
	//normal signals
	void error(const QString &error);
	void kicked();
	void message(const QString &message);
	void sendPacket(const QByteArray &data);
	void isReadyToStop();
	void try_internal_disconnect();
private:
	//player login
	void send_players_informations(const QList<Player_private_and_public_informations> &players_informations);
	void send_players_informations(const QList<Player_public_informations> &players_informations);
	//local data
	QList<Player_private_and_public_informations> players_informations_to_push;
	GeneralData *generalData;
	QSemaphore disconnection;
	qint32 connected_players;
private slots:
	void receivePlayersInformation(const Player_private_and_public_informations &player_informations);
	void internal_disconnect();
};

#endif // CLIENTBROADCAST_H
