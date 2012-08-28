#include <QObject>

#include "GeneralStructures.h"

#ifndef POKECRAFT_CHAT_PARSING_H
#define POKECRAFT_CHAT_PARSING_H

namespace Pokecraft {
class ChatParsing
{
public:
	static QString new_chat_message(QString pseudo,Player_type player_type,Chat_type chat_type,QString text);
	static QString toHtmlEntities(QString text);
	static QString toSmilies(QString text);
};
}

#endif // CHAT_PARSING_H
