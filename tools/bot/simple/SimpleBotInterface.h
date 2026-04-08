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
        uint8_t mapId;
        uint8_t x;
        uint8_t y;
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
    virtual void insert_player(CatchChallenger::Api_protocol_Qt *api,const CatchChallenger::Player_public_informations &player,const uint8_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction);
protected:
    bool move;
    bool randomText;
    bool bugInDirection;
    bool globalChatRandomReply;
    QHash<CatchChallenger::Api_protocol_Qt *,Player> clientList;
};

#endif // SIMPLE_BOT_INTERFACE_H
