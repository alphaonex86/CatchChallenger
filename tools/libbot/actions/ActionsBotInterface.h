/** \file listener.h
\brief Define the server compatible with Ultracopier interface
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef ACTIONS_BOT_INTERFACE_H
#define ACTIONS_BOT_INTERFACE_H

#include "../BotInterface.h"
#include "MapServerMini.h"
#include <unordered_map>
#include <QObject>
#include <QRegularExpression>
#include <QElapsedTimer>
#include <map>
#include <set>
#include <string>

/// QObject is inherited HERE and not by BotInterface: the bot-brain interface
/// itself is toolkit-free, only this Qt implementation of it needs signals,
/// slots and timers.
class ActionsBotInterface : public QObject, public BotInterface
{
    Q_OBJECT
public:
    struct GlobalTarget
    {
        GlobalTarget();
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
        QElapsedTimer sinceTheLastAction;
        //std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > wildForwardStep,wildBackwardStep;
        uint8_t wildCycle;
        unsigned int points;
        /// Opaque handle to the front-end row(s) showing this target, 0 when
        /// none. The GUI owns the widget pointers and resolves the handle; the
        /// library only carries the integer, so no widget type leaks in here.
        uint32_t uiItemHandle;
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
        SIMPLIFIED_PLAYER_ID_FOR_MAP removeId;
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
        QElapsedTimer lastFightAction;
        CatchChallenger::Api_protocol_Qt  *api;
        std::map<uint16_t,CatchChallenger::Player_public_informations> visiblePlayers;
        std::set<std::string> viewedPlayers;

        //plant/seed is now local to player, no server async confirmation needed
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
    bool getValue(const std::string &variable,bool &value);
    bool setValue(const std::string &variable,const bool value);
    std::vector<std::string> variablesList();
    virtual void removeClient(CatchChallenger::Api_protocol *api);
    std::string name();
    std::string version();
    virtual void insert_player(CatchChallenger::Api_protocol *api,const CatchChallenger::Player_public_informations &player,const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const CatchChallenger::Direction &direction);
    /// The bot core hands out the toolkit-free protocol pointer; every index in
    /// this tree is keyed by the Qt subclass (it needs its signals). The Qt
    /// front-end created the object, so this is a plain static_cast, no RTTI.
    static CatchChallenger::Api_protocol_Qt *toQt(CatchChallenger::Api_protocol *api);
    static std::map<CatchChallenger::Api_protocol_Qt  *,Player> clientList;
    //not into clientList because clientList is not initialised when receive the signals (due to delay of map loading)
    static std::map<CatchChallenger::Api_protocol_Qt  *,std::vector<DelayedMapPlayerChange> > delayedMessage;
protected:
    bool randomText;
    bool globalChatRandomReply;
};

#endif // ACTIONS_BOT_INTERFACE_H
