#include "ClientBroadCast.h"

ClientBroadCast::ClientBroadCast()
{
	connect(this,SIGNAL(try_internal_disconnect()),this,SLOT(internal_disconnect()),Qt::QueuedConnection);
}

ClientBroadCast::~ClientBroadCast()
{
}

void ClientBroadCast::setVariable(GeneralData *generalData,Player_private_and_public_informations *player_informations)
{
	this->generalData	=generalData;
	this->player_informations=player_informations;
	connect(&generalData->player_updater,SIGNAL(newConnectedPlayer(qint32)),this,SLOT(receive_instant_player_number(qint32)),Qt::QueuedConnection);
}

void ClientBroadCast::disconnect()
{
	emit try_internal_disconnect();
	disconnection.acquire();
}

void ClientBroadCast::internal_disconnect()
{
	int index=0;
	int list_size=generalData->clientBroadCastList.size();
	while(index<list_size)
	{
		if(generalData->clientBroadCastList.at(index)->player_id==player_id)
		{
			generalData->clientBroadCastList.removeAt(index);
			break;
		}
		index++;
	}
	disconnection.release();
}

void ClientBroadCast::sendSystemMessage(const QString &text,const bool &important)
{
	if(important)
		emit sendChatText(Chat_type_system_important,text);
	else
		emit sendChatText(Chat_type_system,text);
}

void ClientBroadCast::receivePM(const QString &text,const quint32 &player_id)
{
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint8)0xC2;
	out << (quint16)0x0005;
	out << player_id;//id player here
	out << (quint8)0x06;
	out << text;
	emit sendPacket(outputData);
}

void ClientBroadCast::sendPM(const QString &text,const QString &pseudo)
{
	if(this->pseudo==pseudo)
	{
		emit error("Can't send them self the PM");
		return;
	}
	int index=0;
	int list_size=generalData->clientBroadCastList.size();
	while(index<list_size)
	{
		if(generalData->clientBroadCastList.at(index)->pseudo==pseudo)
		{
			generalData->clientBroadCastList.at(index)->receivePM(text,player_id);
			return;
		}
		index++;
	}
}

void ClientBroadCast::askIfIsReadyToStop()
{
	emit isReadyToStop();
}

void ClientBroadCast::receiveChatText(const Chat_type &chatType,const QString &text,const quint32 &sender_player_id)
{
	/* Multiple message when multiple player connected
	emit message(QString("receiveChatText(), text: %1, sender_player_id: %2, to player: %3").arg(text).arg(sender_player_id).arg(player_informations.id)); */
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint8)0xC2;
	out << (quint16)0x0005;
	out << sender_player_id;
	out << (quint8)chatType;
	out << text;
	emit sendPacket(outputData);
}

void ClientBroadCast::sendChatText(const Chat_type &chatType,const QString &text)
{
	emit message(QString("sendChatText(), text: %1").arg(text));
	int list_size=generalData->clientBroadCastList.size();
	if(chatType==Chat_type_clan)
	{
		if(player_informations->public_informations.clan==0)
			emit error("Unable to chat with clan, you have not clan");
		else
		{
			int index=0;
			while(index<list_size)
			{
				if(player_informations->public_informations.clan==generalData->clientBroadCastList.at(index)->player_informations->public_informations.clan && this!=generalData->clientBroadCastList.at(index))
					generalData->clientBroadCastList.at(index)->receiveChatText(chatType,text,player_id);
				index++;
			}
		}
	}
	else if(chatType==Chat_type_system || chatType==Chat_type_system_important)
	{
		if(player_informations->public_informations.type==Player_type_gm || player_informations->public_informations.type==Player_type_dev)
		{
			int index=0;
			while(index<list_size)
			{
				if(this!=generalData->clientBroadCastList.at(index))
					generalData->clientBroadCastList.at(index)->receiveChatText(chatType,text,0);
				index++;
			}
		}
		else
			emit error("Have not the right to send system message");
	}
	else
	{
		int index=0;
		while(index<list_size)
		{
			if(this!=generalData->clientBroadCastList.at(index))
				generalData->clientBroadCastList.at(index)->receiveChatText(chatType,text,player_id);
			index++;
		}
	}
}

void ClientBroadCast::send_player_informations()
{
	generalData->clientBroadCastList << this;
	this->pseudo=this->player_informations->public_informations.pseudo;
	this->player_id=this->player_informations->public_informations.id;
	QList<Player_private_and_public_informations> players_informations;
	players_informations << *(this->player_informations);
	send_players_informations(players_informations);
}

void ClientBroadCast::send_players_informations(const QList<Player_private_and_public_informations> &players_informations)
{
	#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
	emit message(QString("send_players_informations, players_informations.size(): %1").arg(players_informations.size()));
	#endif
	if(players_informations.size()==0)
	{
		emit message("players_informations.size()==0 at send_players_informations()");
		return;
	}
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint8)0xC2;
	out << (quint16)0x0002;
	out << (quint16)players_informations.size();
	int index=0;
	int list_size=players_informations.size();
	while(index<list_size)
	{
		out << players_informations.at(index).public_informations.id;
		out << players_informations.at(index).public_informations.pseudo;
		out << players_informations.at(index).public_informations.description;
		out << players_informations.at(index).public_informations.clan;
		out << (quint8)players_informations.at(index).public_informations.type;
		out << players_informations.at(index).public_informations.skin;
		if(players_informations.at(index).public_informations.id==player_id)
			out << players_informations.at(index).cash;
		index++;
	}
	emit sendPacket(outputData);
}

void ClientBroadCast::send_players_informations(const QList<Player_public_informations> &players_informations)
{
	#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
	emit message(QString("send_players_informations, players_informations.size(): %1").arg(players_informations.size()));
	#endif
	if(players_informations.size()==0)
	{
		emit message("players_informations.size()==0 at send_players_informations()");
		return;
	}
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint8)0xC2;
	out << (quint16)0x0002;
	out << (quint16)players_informations.size();
	int index=0;
	int list_size=players_informations.size();
	while(index<list_size)
	{
		out << players_informations.at(index).id;
		out << players_informations.at(index).pseudo;
		out << players_informations.at(index).description;
		out << players_informations.at(index).clan;
		out << (quint8)players_informations.at(index).type;
		out << players_informations.at(index).skin;
		index++;
	}
	emit sendPacket(outputData);
}

void ClientBroadCast::receive_instant_player_number(qint32 connected_players)
{
	if(this->connected_players==connected_players)
		return;
	this->connected_players=connected_players;
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint8)0xC2;
	out << (quint16)0x0009;
	out << generalData->connected_players;
	out << generalData->max_players;
	emit sendPacket(outputData);
}

void ClientBroadCast::kick()
{
	emit kicked();
}

void ClientBroadCast::sendBroadCastCommand(const QString &command,const QString &extraText)
{
	emit message(QString("command: %1, text: %2").arg(command).arg(extraText));
	if(command=="kick")
	{
		//drop, and do the command here to separate the loop
		int index=0;
		int list_size=generalData->clientBroadCastList.size();
		while(index<list_size)
		{
			if(generalData->clientBroadCastList.at(index)->pseudo==extraText)
			{
				emit sendChatText(Chat_type_system,QString("%1 have been kicked by %2").arg(extraText).arg(pseudo));
				generalData->clientBroadCastList.at(index)->kick();
				return;
			}
			index++;
		}
	}
	else
		emit message(QString("unknow command: %1, text: %2").arg(command).arg(extraText));
}

void ClientBroadCast::addPlayersInformationToWatch(const QList<quint32> &player_ids,const quint8 &type_player_query)
{
	if(type_player_query==0x01)
		player_ids_to_watch<<player_ids;
	emit askPlayersInformation(player_ids);
}

void ClientBroadCast::removePlayersInformationToWatch(const QList<quint32> &player_ids)
{
	int index=0;
	int list_size=player_ids.size();
	while(index<list_size)
	{
		player_ids_to_watch.removeOne(player_ids.at(index));
		index++;
	}
}

void ClientBroadCast::askPlayersInformation(const QList<quint32> &player_ids)
{
	QStringList player_ids_string;
	int index=0;
	int list_size=player_ids.size();
	while(index<list_size)
	{
		player_ids_string << QString::number(player_ids.at(index));
		index++;
	}
	emit message(QString("askPlayersInformation: %1 for the player: %2").arg(player_ids_string.join(", ")).arg(player_id));
	QList<Player_public_informations> players_informations;
	index=0;
	list_size=generalData->clientBroadCastList.size();
	while(index<list_size)
	{
		//do by this way to prevent one memory copy
		if(player_ids.contains(generalData->clientBroadCastList.at(index)->player_id))
			players_informations << generalData->clientBroadCastList.at(index)->player_informations->public_informations;
		index++;
	}
	if(players_informations.size())
		send_players_informations(players_informations);
}

void ClientBroadCast::receivePlayersInformation(const Player_private_and_public_informations &player_informations)
{
	emit message(QString("receivePlayersInformation: %1, send to player: %2").arg(player_informations.public_informations.id).arg(player_id));
	players_informations_to_push << player_informations;
	QList<Player_private_and_public_informations> players_informations;
	players_informations << player_informations;
	send_players_informations(players_informations);
	//update the player info on all which have ask to look it by: addPlayersInformationToWatch()
}

