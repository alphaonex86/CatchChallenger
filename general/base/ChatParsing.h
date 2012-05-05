#include <QObject>

#include "GeneralStructures.h"

#ifndef CHAT_PARSING_H
#define CHAT_PARSING_H

class ChatParsing
{
public:
	static QString new_chat_message(QString pseudo,Player_type player_type,Chat_type chat_type,QString text);
	static QString toHtmlEntities(QString text);
	static QString toSmilies(QString text);
};

#endif // CHAT_PARSING_H
