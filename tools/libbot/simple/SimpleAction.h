/** \file SimpleAction.h
\brief Qt driver of the minimal bot brain (timers + chat reply)
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef SIMPLE_ACTION_BOT_INTERFACE_H
#define SIMPLE_ACTION_BOT_INTERFACE_H

#include "SimpleBotInterface.h"
#include "../../../client/libqtcatchchallenger/Api_protocol_Qt.hpp"

#include <QObject>
#include <QTimer>

/// QObject is inherited HERE and not by SimpleBotInterface: the brain itself is
/// toolkit-free, only its timer driver needs Qt.
class SimpleAction : public QObject, public SimpleBotInterface
{
    Q_OBJECT
public:
    SimpleAction();
    ~SimpleAction();
    void insert_player(CatchChallenger::Api_protocol *api,const CatchChallenger::Player_public_informations &player,
                       const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const CatchChallenger::Direction &direction);
private:
    QTimer moveTimer;
    QTimer textTimer;
private:
    void purgeCpuCache();
    void doMove();
    void doText();
    void new_chat_text(const CatchChallenger::Chat_type &chat_type, const std::string &text, const std::string &pseudo, const CatchChallenger::Player_type &type);
};

#endif // SIMPLE_ACTION_BOT_INTERFACE_H
