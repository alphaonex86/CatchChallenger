#ifndef POKECRAFT_CLIENTBROADCAST_H
#define POKECRAFT_CLIENTBROADCAST_H

#include <QObject>
#include <QDataStream>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QSemaphore>

#include "ServerStructures.h"
#include "../general/base/DebugClass.h"
#include "../general/base/GeneralStructures.h"
#include "../VariableServer.h"

namespace Pokecraft {
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
	Player_internal_informations *player_informations;
	// set the variable
	void setVariable(Player_internal_informations *player_informations);
	void disconnect();
	//player indexed list
	static QHash<QString,ClientBroadCast *> playerByPseudo;
	static QList<ClientBroadCast *> clientBroadCastList;
public slots:
	//global slot
	void receivePM(const QString &text,const QString &pseudo);
	void sendPM(const QString &text,const QString &pseudo);
	void receiveChatText(const Chat_type &chatType,const QString &text,const QString &sender_pseudo,const Player_type &sender_player_type);
	void receiveSystemText(const QString &text,const bool &important=false);
	void sendChatText(const Chat_type &chatType,const QString &text);
	void receive_instant_player_number(qint32 connected_players);
	void kick();
	void sendBroadCastCommand(const QString &command,const QString &extraText);
	void setRights(const Player_type& type);
	//after login
	void send_player_informations();
	//normal slots
	void askIfIsReadyToStop();
	void stop();
	void sendSystemMessage(const QString &text,const bool &important=false);
signals:
	//normal signals
	void error(const QString &error);
	void kicked();
	void message(const QString &message);
	void isReadyToStop();
	void try_internal_disconnect();
	//send packet on network
	void sendPacket(const quint8 &mainIdent,const quint16 &subIdent,const QByteArray &data=QByteArray());
	void sendPacket(const quint8 &mainIdent,const QByteArray &data=QByteArray());
private:
	//local data
	QList<Player_private_and_public_informations> players_informations_to_push;
	QSemaphore disconnection;
	qint32 connected_players;
	static QHash<QString,ClientBroadCast *>::const_iterator i_playerByPseudo;
	static QHash<QString,ClientBroadCast *>::const_iterator i_playerByPseudo_end;
private slots:
	void internal_disconnect();
};
}

#endif // CLIENTBROADCAST_H
