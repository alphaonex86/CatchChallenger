#ifndef CATCHCHALLENGER_QTBROADCASTWITHOUTSENDER_H
#define CATCHCHALLENGER_QTBROADCASTWITHOUTSENDER_H

#include "../../general/base/GeneralStructures.hpp"
#include "ServerStructures.hpp"

namespace CatchChallenger {
class BroadCastWithoutSender
{
private:
    explicit BroadCastWithoutSender();
    static unsigned char bufferSendPlayer[3];
public:
    static BroadCastWithoutSender broadCastWithoutSender;
#ifndef EPOLLCATCHCHALLENGERSERVER
signals:
    void serverCommand(const std::string &command,const std::string &extraText) const;
    void new_player_is_connected(const Player_private_and_public_informations &newPlayer) const;
    void player_is_disconnected(const std::string &oldPlayer) const;
    void new_chat_message(const std::string &pseudo,const Chat_type &type,const std::string &text) const;
#endif
public:
#ifndef EPOLLCATCHCHALLENGERSERVER
    void emit_serverCommand(const std::string &command,const std::string &extraText);
    void emit_new_player_is_connected(const Player_private_and_public_informations &newPlayer);
    void emit_player_is_disconnected(const std::string &oldPlayer);
    void emit_new_chat_message(const std::string &pseudo,const Chat_type &type,const std::string &text);
#endif
};
}

#endif // BROADCASTWITHOUTSENDER_H
