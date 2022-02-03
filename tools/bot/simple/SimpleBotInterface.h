/** \file listener.h
\brief Define the server compatible with Ultracopier interface
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef SIMPLE_BOT_INTERFACE_H
#define SIMPLE_BOT_INTERFACE_H

#include "../BotInterface.h"

class SimpleBotInterface : public BotInterface
{
    Q_OBJECT
public:
    struct Player
    {
        CatchChallenger::Player_public_informations player;
        quint32 mapId;
        quint16 x;
        quint16 y;
        CatchChallenger::Direction direction;
    };

    SimpleBotInterface();
    ~SimpleBotInterface();
    QVariant getValue(const QString &variable);
    bool setValue(const QString &variable,const QVariant &value);
    QStringList variablesList();
    virtual void removeClient(CatchChallenger::Api_protocol_Qt *api);
    QString name();
    QString version();
    virtual void insert_player(CatchChallenger::Api_protocol_Qt *api,const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction);
protected:
    bool move;
    bool randomText;
    bool bugInDirection;
    bool globalChatRandomReply;
    QHash<CatchChallenger::Api_protocol_Qt *,Player> clientList;
};

#endif // SIMPLE_BOT_INTERFACE_H
