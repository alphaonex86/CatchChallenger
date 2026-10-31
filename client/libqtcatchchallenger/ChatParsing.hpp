#ifndef CATCHCHALLENGER_CHAT_PARSING_H
#define CATCHCHALLENGER_CHAT_PARSING_H

#include <string>

#include "../../general/base/GeneralStructures.hpp"
#include "../../general/base/lib.h"

namespace CatchChallenger {
/// \brief rich text shown to the player: chat lines and bot dialogs
class DLL_PUBLIC ChatParsing
{
public:
    //dimSystemText: grey small italic system lines (client look choice)
    static std::string new_chat_message(const std::string &pseudo,const Player_type &player_type,const Chat_type &chat_type,const std::string &text,const bool withLink=false,const bool dimSystemText=false);
    static std::string toHtmlEntities(const std::string &text);
    static std::string toSmilies(const std::string &text);
    //bot step html: drops the quest spans when the bot has no quest for the player, marks external links
    static std::string botTextToDisplay(const std::string &htmlContent,const bool botHaveQuest);
};
}

#endif // CHAT_PARSING_H
