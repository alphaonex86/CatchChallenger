#ifndef CATCHCHALLENGER_BROADCASTWITHOUTSENDER_H
#define CATCHCHALLENGER_BROADCASTWITHOUTSENDER_H

#include <QObject>
#include "../../general/base/GeneralStructures.h"
#include "ServerStructures.h"

namespace CatchChallenger {
class BroadCastWithoutSender : public QObject
{
    Q_OBJECT
private:
    explicit BroadCastWithoutSender();
public:
    static BroadCastWithoutSender broadCastWithoutSender;
signals:
    void serverCommand(const QString &command,const QString &extraText) const;
    void new_player_is_connected(const Player_internal_informations &newPlayer) const;
    void player_is_disconnected(const QString &oldPlayer) const;
    void new_chat_message(const QString &pseudo,const Chat_type &type,const QString &text) const;
public slots:
    void emit_serverCommand(const QString &command,const QString &extraText);
    void emit_new_player_is_connected(const Player_internal_informations &newPlayer);
    void emit_player_is_disconnected(const QString &oldPlayer);
    void emit_new_chat_message(const QString &pseudo,const Chat_type &type,const QString &text);
};
}

#endif // BROADCASTWITHOUTSENDER_H
