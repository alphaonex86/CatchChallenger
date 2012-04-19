#include <QObject>

#include "../pokecraft-general/GeneralStructures.h"

#ifndef POKECRAFT_CLIENT_CHAT_H
#define POKECRAFT_CLIENT_CHAT_H

class Pokecraft_client_chat
{
public:
	static QString new_chat_message(QString pseudo,Player_type player_type,Chat_type chat_type,QString text);
	static QString toHtmlEntities(QString text);
	static QString toSmilies(QString text);
};

#endif // POKECRAFT_CLIENT_CHAT_H
