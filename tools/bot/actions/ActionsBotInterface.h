/** \file listener.h
\brief Define the server compatible with Ultracopier interface
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef ACTIONS_BOT_INTERFACE_H
#define ACTIONS_BOT_INTERFACE_H

#include "../BotInterface.h"
#include "../../../client/qt/fight/interface/ClientFightEngine.h"
#include "MapServerMini.h"
#include <unordered_map>
#include <QRegularExpression>
#include <QTime>
#include <QList>
#include <QtWidgets/QListWidgetItem>
#include <map>
#include <set>

class ActionsBotInterface : public BotInterface
{
    Q_OBJECT
public:
    struct GlobalTarget
    {
        enum GlobalTargetType
        {
            ItemOnMap=0,//indexOfItemOnMap
            Fight=1,//fight id
            Shop=2,//shop id
            Heal=3,
            WildMonster=4,
            Dirt=5,
            Plant=6,
            None=7
        };
        GlobalTargetType type;
        uint32_t extra;
        const MapServerMini::BlockObject * blockObject;//NULL if no target
        std::vector<const MapServerMini::BlockObject *> bestPath;//without the current path
        std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > localStep;
        MapServerMini::BlockObject::LinkPoint linkPoint;
        QTime sinceTheLastAction;
        //std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > wildForwardStep,wildBackwardStep;
        uint8_t wildCycle;
        unsigned int points;
        QList<QListWidgetItem *> uiItems;
    };
    struct ChatEntry
    {
        std::string player_pseudo;
        CatchChallenger::Player_type player_type;
        CatchChallenger::Chat_type chat_type;
        std::string text;
    };
    enum DelayedMapPlayerChangeType
    {
        DelayedMapPlayerChangeType_Insert,
        DelayedMapPlayerChangeType_InsertAll,
        DelayedMapPlayerChangeType_Delete
    };
    struct DelayedMapPlayerChange
    {
        DelayedMapPlayerChangeType type;
        CatchChallenger::Player_public_informations player;
        uint32_t mapId;
        uint16_t x,y;
        CatchChallenger::Direction direction;
    };
    struct Player
    {
        unsigned int repel_step;
        bool canMoveOnMap;
        std::vector<uint8_t> events;
        uint32_t mapId;
        uint8_t x;
        uint8_t y;
        uint32_t internalId;
        //CatchChallenger::Direction direction;
        GlobalTarget target;
        //uint8_t previousStepWalked;do into the api, see MoveOnTheMap::newDirection()
        CatchChallenger::ClientFightEngine *fightEngine;
        QTime lastFightAction;
        CatchChallenger::Api_protocol *api;
        std::map<uint16_t,CatchChallenger::Player_public_informations> visiblePlayers;
        std::set<QString> viewedPlayers;

        //plant seed in waiting
        struct SeedInWaiting
        {
            uint32_t seedItemId;
            uint16_t indexOnMap;
        };
        std::vector<SeedInWaiting> seed_in_waiting;
        struct ClientPlantInCollecting
        {
            uint16_t indexOnMap;
            uint8_t plant_id;
            uint16_t seconds_to_mature;
        };
        std::vector<ClientPlantInCollecting> plant_collect_in_waiting;
        QRegularExpression regexMatchPseudo;

        std::vector<uint32_t> mapIdListLocalTarget;
        std::vector<GlobalTarget> targetListGlobalTarget;
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
    static std::map<CatchChallenger::Api_protocol *,Player> clientList;
    //not into clientList because clientList is not initialised when receive the signals (due to delay of map loading)
    static std::map<CatchChallenger::Api_protocol *,std::vector<DelayedMapPlayerChange> > delayedMessage;
protected:
    bool randomText;
    bool globalChatRandomReply;
};

#endif // ACTIONS_BOT_INTERFACE_H
