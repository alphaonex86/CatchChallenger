#include "ClientBroadCast.h"
#include "EventDispatcher.h"

using namespace Pokecraft;

QHash<QString,ClientBroadCast *> ClientBroadCast::playerByPseudo;
QList<ClientBroadCast *> ClientBroadCast::clientBroadCastList;
QHash<QString,ClientBroadCast *>::const_iterator ClientBroadCast::i_playerByPseudo;
QHash<QString,ClientBroadCast *>::const_iterator ClientBroadCast::i_playerByPseudo_end;

ClientBroadCast::ClientBroadCast()
{
	connected_players=0;
	connect(this,SIGNAL(try_internal_disconnect()),this,SLOT(internal_disconnect()),Qt::QueuedConnection);
}

ClientBroadCast::~ClientBroadCast()
{
}

void ClientBroadCast::setVariable(Player_internal_informations *player_informations)
{
	this->player_informations=player_informations;
	emit message(QString("boardcast: EventDispatcher::generalData.serverSettings.commmonServerSettings.sendPlayerNumber: %1").arg(EventDispatcher::generalData.serverSettings.commmonServerSettings.sendPlayerNumber));
	if(EventDispatcher::generalData.serverSettings.commmonServerSettings.sendPlayerNumber)
		connect(&EventDispatcher::generalData.serverPrivateVariables.player_updater,SIGNAL(newConnectedPlayer(qint32)),this,SLOT(receive_instant_player_number(qint32)),Qt::QueuedConnection);
}

void ClientBroadCast::disconnect()
{
	emit try_internal_disconnect();
	disconnection.acquire();
}

void ClientBroadCast::internal_disconnect()
{
	playerByPseudo.remove(player_informations->public_and_private_informations.public_informations.pseudo);
	clientBroadCastList.removeOne(this);
	disconnection.release();
}

//without verification of rights
void ClientBroadCast::sendSystemMessage(const QString &text,const bool &important)
{
	int list_size=clientBroadCastList.size();
	if(important)
	{
		int index=0;
		while(index<list_size)
		{
			if(this!=clientBroadCastList.at(index))
				clientBroadCastList.at(index)->receiveSystemText(Chat_type_system_important,text);
			index++;
		}
	}
	else
	{
		int index=0;
		while(index<list_size)
		{
			if(this!=clientBroadCastList.at(index))
				clientBroadCastList.at(index)->receiveSystemText(Chat_type_system,text);
			index++;
		}
	}
}

void ClientBroadCast::receivePM(const QString &text,const QString &pseudo)
{
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << pseudo;//id player here
	out << (quint8)0x06;
	out << text;
	emit sendPacket(0xC2,0x0005,outputData);
}

void ClientBroadCast::sendPM(const QString &text,const QString &pseudo)
{
	if(this->player_informations->public_and_private_informations.public_informations.pseudo==pseudo)
	{
		emit error("Can't send them self the PM");
		return;
	}
	if(!playerByPseudo.contains(pseudo))
	{
		receiveSystemText(Chat_type_system,QString("unable to found the connected player: pseudo: \"%1\"").arg(pseudo));
		emit message(QString("%1 have try send message to not connected user: %2").arg(this->player_informations->public_and_private_informations.public_informations.pseudo).arg(pseudo));
		return;
	}
	playerByPseudo[pseudo]->receivePM(text,this->player_informations->public_and_private_informations.public_informations.pseudo);
}

void ClientBroadCast::askIfIsReadyToStop()
{
	playerByPseudo.remove(player_informations->public_and_private_informations.public_informations.pseudo);
	clientBroadCastList.removeOne(this);
	emit isReadyToStop();
}

void ClientBroadCast::stop()
{
	deleteLater();
}

void ClientBroadCast::receiveChatText(const Chat_type &chatType,const QString &text,const QString &sender_pseudo,const Player_type &sender_player_type)
{
	/* Multiple message when multiple player connected
	emit message(QString("receiveChatText(), text: %1, sender_player_id: %2, to player: %3").arg(text).arg(sender_player_id).arg(player_informations.id)); */
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint8)chatType;
	out << text;
	out << sender_pseudo;
	out << (quint8)sender_player_type;
	emit sendPacket(0xC2,0x0005,outputData);
}

void ClientBroadCast::receiveSystemText(const bool &important,const QString &text)
{
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	if(important)
		out << (quint8)Chat_type_system_important;
	else
		out << (quint8)Chat_type_system;
	out << text;
	emit sendPacket(0xC2,0x0005,outputData);
}

void ClientBroadCast::sendChatText(const Chat_type &chatType,const QString &text)
{
	emit message(QString("sendChatText(), text: %1").arg(text));
	int list_size=clientBroadCastList.size();
	if(chatType==Chat_type_clan)
	{
		if(player_informations->public_and_private_informations.public_informations.clan==0)
			emit error("Unable to chat with clan, you have not clan");
		else
		{
			int index=0;
			while(index<list_size)
			{
				if(player_informations->public_and_private_informations.public_informations.clan==clientBroadCastList.at(index)->player_informations->public_and_private_informations.public_informations.clan && this!=clientBroadCastList.at(index))
					clientBroadCastList.at(index)->receiveChatText(chatType,text,player_informations->public_and_private_informations.public_informations.pseudo,player_informations->public_and_private_informations.public_informations.type);
				index++;
			}
		}
	}
	else if(chatType==Chat_type_system || chatType==Chat_type_system_important)
	{
		if(player_informations->public_and_private_informations.public_informations.type==Player_type_gm || player_informations->public_and_private_informations.public_informations.type==Player_type_dev)
		{
			int index=0;
			while(index<list_size)
			{
				if(this!=clientBroadCastList.at(index))
					clientBroadCastList.at(index)->receiveSystemText(chatType,text);
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
			if(this!=clientBroadCastList.at(index))
				clientBroadCastList.at(index)->receiveChatText(chatType,text,player_informations->public_and_private_informations.public_informations.pseudo,player_informations->public_and_private_informations.public_informations.type);
			index++;
		}
	}
}

void ClientBroadCast::send_player_informations()
{
	playerByPseudo[player_informations->public_and_private_informations.public_informations.pseudo]=this;
	clientBroadCastList << this;
}

void ClientBroadCast::receive_instant_player_number(qint32 connected_players)
{
	emit message(QString("connected_players: %1").arg(connected_players));
	if(this->connected_players==connected_players)
		return;
	this->connected_players=connected_players;
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	if(EventDispatcher::generalData.serverSettings.max_players<=255)
		out << (qint8)connected_players;
	else
		out << (qint16)connected_players;
	emit message(QString("send connected_players: %1").arg(connected_players));
	emit sendPacket(0xC3,outputData);
}

void ClientBroadCast::kick()
{
	emit kicked();
}

void ClientBroadCast::sendBroadCastCommand(const QString &command,const QString &extraText)
{
	emit message(QString("command: %1, text: %2").arg(command).arg(extraText));
	if(command=="chat")
	{
		QStringList list=extraText.split(' ');
		if(list.size()<2)
		{
			receiveSystemText(Chat_type_system,QString("command not understand").arg(extraText));
			emit message(QString("command not understand").arg(extraText));
			return;
		}
		if(list.first()=="system")
		{
			list.removeFirst();
			sendChatText(Chat_type_system,list.join(" "));
			return;
		}
		if(list.first()=="system_important")
		{
			list.removeFirst();
			sendChatText(Chat_type_system_important,list.join(" "));
			return;
		}
		else
		{
			receiveSystemText(Chat_type_system,QString("command not understand").arg(extraText));
			emit message(QString("command not understand").arg(extraText));
			return;
		}
	}
	else if(command=="setrights")
	{
		QStringList list=extraText.split(' ');
		if(list.size()!=2)
		{
			receiveSystemText(Chat_type_system,QString("command not understand").arg(extraText));
			emit message(QString("command not understand").arg(extraText));
			return;
		}
		if(!playerByPseudo.contains(list.first()))
		{
			receiveSystemText(Chat_type_system,QString("unable to found the connected player to kick: pseudo: \"%1\"").arg(list.first()));
			emit message(QString("unable to found the connected player to kick: pseudo: \"%1\"").arg(list.first()));
			return;
		}
		if(list.last()=="normal")
			playerByPseudo[extraText]->setRights(Player_type_normal);
		else if(list.last()=="premium")
			playerByPseudo[extraText]->setRights(Player_type_premium);
		else if(list.last()=="gm")
			playerByPseudo[extraText]->setRights(Player_type_gm);
		else if(list.last()=="dev")
			playerByPseudo[extraText]->setRights(Player_type_dev);
		else
		{
			receiveSystemText(Chat_type_system,QString("unable to found this rights level: \"%1\"").arg(list.last()));
			emit message(QString("unable to found this rights level: \"%1\"").arg(list.last()));
			return;
		}
	}
	else if(command=="playerlist")
	{
		if(playerByPseudo.size()==1)
			receiveSystemText(Chat_type_system,QString("You are alone on the server!"));
		else
		{
			QStringList playerStringList;
			i_playerByPseudo = playerByPseudo.constBegin();
			i_playerByPseudo_end = playerByPseudo.constEnd();
			while (i_playerByPseudo != i_playerByPseudo_end)
			{
				playerStringList << "<b>"+i_playerByPseudo.value()->player_informations->public_and_private_informations.public_informations.pseudo+"</b>";
				++i_playerByPseudo;
			}
			receiveSystemText(Chat_type_system,QString("players connected: %1").arg(playerStringList.join(", ")));
		}
		return;
	}
	else if(command=="playernumber")
	{
		if(playerByPseudo.size()==1)
			receiveSystemText(Chat_type_system,QString("You are alone on the server!"));
		else
			receiveSystemText(Chat_type_system,QString("<b>%1</b> players connected").arg(playerByPseudo.size()));
		return;
	}
	else if(command=="kick")
	{
		//drop, and do the command here to separate the loop
		if(!playerByPseudo.contains(extraText))
		{
			receiveSystemText(Chat_type_system,QString("unable to found the connected player to kick: pseudo: \"%1\"").arg(extraText));
			emit message(QString("unable to found the connected player to kick: pseudo: \"%1\"").arg(extraText));
			return;
		}
		playerByPseudo[extraText]->kick();
		sendSystemMessage(QString("%1 have been kicked by %2").arg(extraText).arg(player_informations->public_and_private_informations.public_informations.pseudo));
		return;
	}
	else
		emit message(QString("unknow command: %1, text: %2").arg(command).arg(extraText));
}

void ClientBroadCast::setRights(const Player_type& type)
{
	player_informations->public_and_private_informations.public_informations.type=type;
}
