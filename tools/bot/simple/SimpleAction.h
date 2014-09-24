/** \file listener.h
\brief Define the server compatible with Ultracopier interface
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef SIMPLE_ACTION_BOT_INTERFACE_H
#define SIMPLE_ACTION_BOT_INTERFACE_H

#include "SimpleBotInterface.h"

#include <QTimer>
#include <QString>

class SimpleAction : public SimpleBotInterface
{
    Q_OBJECT
public:
    SimpleAction();
    ~SimpleAction();
    void insert_player(CatchChallenger::Api_protocol *api,const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction);
private:
    QTimer moveTimer;
    QTimer textTimer;
private:
    void doMove();
    void doText();
    void new_chat_text(const CatchChallenger::Chat_type &chat_type,const QString &text,const QString &pseudo,const CatchChallenger::Player_type &type);
};

#endif // SIMPLE_ACTION_BOT_INTERFACE_H
