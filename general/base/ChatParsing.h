#ifndef CATCHCHALLENGER_CHAT_PARSING_H
#define CATCHCHALLENGER_CHAT_PARSING_H

#include <QObject>

#include "GeneralStructures.h"

namespace CatchChallenger {
class ChatParsing
{
public:
    static std::basic_string<char> new_chat_message(std::basic_string<char> pseudo,Player_type player_type,Chat_type chat_type,std::basic_string<char> text);
    static std::basic_string<char> toHtmlEntities(std::basic_string<char> text);
    static std::basic_string<char> toSmilies(std::basic_string<char> text);
};
}

#endif // CHAT_PARSING_H
