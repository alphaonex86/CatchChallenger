#ifndef CATCHCHALLENGER_BROADCASTWITHOUTSENDER_H
#define CATCHCHALLENGER_BROADCASTWITHOUTSENDER_H

#include <QObject>
#include "../../general/base/GeneralStructures.h"
#include "ServerStructures.h"

namespace CatchChallenger {
class BroadCastWithoutSender
        #ifndef EPOLLCATCHCHALLENGERSERVER
        : public QObject
        #endif
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    Q_OBJECT
    #endif
private:
    explicit BroadCastWithoutSender();
    static unsigned char bufferSendPlayer[3];
public:
    static BroadCastWithoutSender broadCastWithoutSender;
#ifndef EPOLLCATCHCHALLENGERSERVER
signals:
    void serverCommand(const std::basic_string<char> &command,const std::basic_string<char> &extraText) const;
    void new_player_is_connected(const Player_private_and_public_informations &newPlayer) const;
    void player_is_disconnected(const std::basic_string<char> &oldPlayer) const;
    void new_chat_message(const std::basic_string<char> &pseudo,const Chat_type &type,const std::basic_string<char> &text) const;
#endif
public:
#ifndef EPOLLCATCHCHALLENGERSERVER
    void emit_serverCommand(const std::basic_string<char> &command,const std::basic_string<char> &extraText);
    void emit_new_player_is_connected(const Player_private_and_public_informations &newPlayer);
    void emit_player_is_disconnected(const std::basic_string<char> &oldPlayer);
    void emit_new_chat_message(const std::basic_string<char> &pseudo,const Chat_type &type,const std::basic_string<char> &text);
#endif
    void receive_instant_player_number(const qint16 &connected_players);
    void doDDOSChat();
};
}

#endif // BROADCASTWITHOUTSENDER_H
