#ifndef CATCHCHALLENGER_CHAT_PARSING_H
#define CATCHCHALLENGER_CHAT_PARSING_H

#include <string>

#include "../../general/base/GeneralStructures.hpp"

namespace CatchChallenger {
class ChatParsing
{
public:
    static std::string new_chat_message(const std::string &pseudo,const Player_type &player_type,const Chat_type &chat_type,const std::string &text,const bool withLink=false);
    static std::string toHtmlEntities(const std::string &text);
    static std::string toSmilies(const std::string &text);
};
}

#endif // CHAT_PARSING_H
