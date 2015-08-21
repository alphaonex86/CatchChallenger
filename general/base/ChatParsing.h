#ifndef CATCHCHALLENGER_CHAT_PARSING_H
#define CATCHCHALLENGER_CHAT_PARSING_H

#include <QObject>

#include "GeneralStructures.h"

namespace CatchChallenger {
class ChatParsing
{
public:
    static std::string new_chat_message(std::string pseudo,Player_type player_type,Chat_type chat_type,std::string text);
    static std::string toHtmlEntities(std::string text);
    static std::string toSmilies(std::string text);
};
}

#endif // CHAT_PARSING_H
