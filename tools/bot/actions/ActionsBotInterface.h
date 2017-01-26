/** \file listener.h
\brief Define the server compatible with Ultracopier interface
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef ACTIONS_BOT_INTERFACE_H
#define ACTIONS_BOT_INTERFACE_H

#include "../BotInterface.h"
#include "../../../client/fight/interface/ClientFightEngine.h"
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
            Dirt,
            Plant,
            None
        };
        GlobalTargetType type;
        uint32_t extra;
        const MapServerMini::BlockObject * blockObject;//NULL if no target
        std::vector<const MapServerMini::BlockObject *> bestPath;
        std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > localStep;
        MapServerMini::BlockObject::LinkPoint linkPoint;
    };
    struct Player
    {
        uint32_t mapId;
        uint8_t x;
        uint8_t y;
        CatchChallenger::Direction direction;
        GlobalTarget target;
        uint8_t previousStepWalked;
        CatchChallenger::ClientFightEngine *fightEngine;
        CatchChallenger::Api_protocol *api;

        //plant seed in waiting
        struct SeedInWaiting
        {
            uint32_t seedItemId;
            uint16_t indexOnMap;
        };
        QList<SeedInWaiting> seed_in_waiting;
        struct ClientPlantInCollecting
        {
            uint16_t indexOnMap;
            uint8_t plant_id;
            uint16_t seconds_to_mature;
        };
        QList<ClientPlantInCollecting> plant_collect_in_waiting;
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
    static QHash<CatchChallenger::Api_protocol *,Player> clientList;
protected:
    bool randomText;
    bool globalChatRandomReply;
};

#endif // ACTIONS_BOT_INTERFACE_H
