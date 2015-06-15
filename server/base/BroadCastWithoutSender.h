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
    void serverCommand(const QString &command,const QString &extraText) const;
    void new_player_is_connected(const Player_private_and_public_informations &newPlayer) const;
    void player_is_disconnected(const QString &oldPlayer) const;
    void new_chat_message(const QString &pseudo,const Chat_type &type,const QString &text) const;
#endif
public:
#ifndef EPOLLCATCHCHALLENGERSERVER
    void emit_serverCommand(const QString &command,const QString &extraText);
    void emit_new_player_is_connected(const Player_private_and_public_informations &newPlayer);
    void emit_player_is_disconnected(const QString &oldPlayer);
    void emit_new_chat_message(const QString &pseudo,const Chat_type &type,const QString &text);
#endif
    void receive_instant_player_number(const qint16 &connected_players);
    void doDDOSAction();
};
}

#endif // BROADCASTWITHOUTSENDER_H
