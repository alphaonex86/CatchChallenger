#ifndef POKECRAFT_BROADCASTWITHOUTSENDER_H
#define POKECRAFT_BROADCASTWITHOUTSENDER_H

#include <QObject>
#include "../../general/base/GeneralStructures.h"
#include "ServerStructures.h"

namespace Pokecraft {
class BroadCastWithoutSender : public QObject
{
    Q_OBJECT
private:
    explicit BroadCastWithoutSender();
public:
    static BroadCastWithoutSender broadCastWithoutSender;
signals:
    void serverCommand(const QString &command,const QString &extraText);
    void new_player_is_connected(const Player_internal_informations &newPlayer);
    void player_is_disconnected(const QString &oldPlayer);
    void new_chat_message(const QString &pseudo,const Chat_type &type,const QString &text);
public slots:
    void emit_serverCommand(const QString &command,const QString &extraText);
    void emit_new_player_is_connected(const Player_internal_informations &newPlayer);
    void emit_player_is_disconnected(const QString &oldPlayer);
    void emit_new_chat_message(const QString &pseudo,const Chat_type &type,const QString &text);
};
}

#endif // BROADCASTWITHOUTSENDER_H
