/** \file listener.h
\brief Define the server compatible with Ultracopier interface
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef ACTIONS_BOT_INTERFACE_H
#define ACTIONS_BOT_INTERFACE_H

#include "../BotInterface.h"
#include "MapServerMini.h"
#include <unordered_map>

class ActionsBotInterface : public BotInterface
{
    Q_OBJECT
public:
    struct GlobalTarget
    {
        enum GlobalTargetType
        {
            ItemOnMap,//x,y
            Fight,//fight id
            Shop,//shop id
            Heal,
            WildMonster,
            None
        };
        GlobalTargetType type;
        uint32_t extra;
        const MapServerMini::BlockObject * blockObject;//NULL if no target
        std::vector<const MapServerMini::BlockObject *> bestPath;
    };
    struct Player
    {
        CatchChallenger::Player_public_informations player;
        quint32 mapId;
        quint16 x;
        quint16 y;
        CatchChallenger::Direction direction;
        std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint32_t/*quantity*/> items,warehouse_items;
        GlobalTarget target;
    };

    ActionsBotInterface();
    ~ActionsBotInterface();
    QVariant getValue(const QString &variable);
    bool setValue(const QString &variable,const QVariant &value);
    QStringList variablesList();
    virtual void removeClient(CatchChallenger::Api_protocol *api);
    QString name();
    QString version();
    virtual void insert_player(CatchChallenger::Api_protocol *api,const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction);
    QHash<CatchChallenger::Api_protocol *,Player> clientList;
protected:
    bool move;
    bool randomText;
    bool bugInDirection;
    bool globalChatRandomReply;
};

#endif // ACTIONS_BOT_INTERFACE_H
