#include <QObject>

#include "GeneralStructures.h"

#ifndef CATCHCHALLENGER_CHAT_PARSING_H
#define CATCHCHALLENGER_CHAT_PARSING_H

namespace CatchChallenger {
class ChatParsing
{
public:
	static QString new_chat_message(QString pseudo,Player_type player_type,Chat_type chat_type,QString text);
	static QString toHtmlEntities(QString text);
	static QString toSmilies(QString text);
};
}

#endif // CHAT_PARSING_H
