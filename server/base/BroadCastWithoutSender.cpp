#include "BroadCastWithoutSender.h"

using namespace Pokecraft;

/// \todo call directly from here: receive_instant_player_number(qint32)
BroadCastWithoutSender BroadCastWithoutSender::broadCastWithoutSender;

BroadCastWithoutSender::BroadCastWithoutSender()
{
}

void BroadCastWithoutSender::emit_serverCommand(const QString &command,const QString &extraText)
{
    emit serverCommand(command,extraText);
}

void BroadCastWithoutSender::emit_new_player_is_connected(const Player_internal_informations &newPlayer)
{
    emit new_player_is_connected(newPlayer);
}

void BroadCastWithoutSender::emit_player_is_disconnected(const QString &oldPlayer)
{
    emit player_is_disconnected(oldPlayer);
}

void BroadCastWithoutSender::emit_new_chat_message(const QString &pseudo,const Chat_type &type,const QString &text)
{
    emit new_chat_message(pseudo,type,text);
}

