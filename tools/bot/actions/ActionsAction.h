/** \file listener.h
\brief Define the server compatible with Ultracopier interface
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef ACTIONS_ACTION_BOT_INTERFACE_H
#define ACTIONS_ACTION_BOT_INTERFACE_H

#include "ActionsBotInterface.h"
#include "../../general/base/CommonMap.h"
#include "../../general/base/Map_loader.h"
#include "../bot/actions/MapServerMini.h"

#include <QTimer>
#include <QString>
#include <QThread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <QHash>
#include <QList>

class ActionsAction : public ActionsBotInterface
{
    Q_OBJECT
public:
    static ActionsAction *actionsAction;
    ActionsAction();
    ~ActionsAction();

    enum BlockedOn
    {
        BlockedOn_ZoneItem,
        BlockedOn_ZoneFight,
        BlockedOn_RandomNumber,
        BlockedOn_Fight
    };

    void insert_player(CatchChallenger::Api_protocol *api,const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction);
    void insert_player_all(CatchChallenger::Api_protocol *api,const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction);
    void remove_player(CatchChallenger::Api_protocol *api,const uint16_t &id);
    void dropAllPlayerOnTheMap(CatchChallenger::Api_protocol *api);
    bool preload_other_pre();
    bool preload_the_map_step1();
    bool preload_the_map_step2();
    bool preload_post_subdatapack();
    uint64_t elementToLoad() const;
    uint64_t elementLoaded() const;
    static bool haveBeatBot(CatchChallenger::Api_protocol *api,const uint16_t &botFightId);
    static bool botHaveQuest(CatchChallenger::Api_protocol *api,const uint32_t &botId);
    static bool tryValidateQuestStep(CatchChallenger::Api_protocol *api, const uint16_t &questId, const uint32_t &botId, bool silent);
    static void addBeatenBotFight(CatchChallenger::Api_protocol *api,const uint16_t &botFightId);
    static bool haveNextStepQuestRequirements(CatchChallenger::Api_protocol *api,const CatchChallenger::Quest &quest);
    static bool haveStartQuestRequirement(CatchChallenger::Api_protocol *api,const CatchChallenger::Quest &quest);
    static bool startQuest(CatchChallenger::Api_protocol *api,const CatchChallenger::Quest &quest);
    static std::vector<std::pair<uint32_t,std::string> > getQuestList(CatchChallenger::Api_protocol *api,const uint32_t &botId);
    static bool nextStepQuest(CatchChallenger::Api_protocol *api,const CatchChallenger::Quest &quest);
    static void appendReputationPoint(CatchChallenger::Api_protocol *api,const QString &type,const int32_t &point);
    static uint32_t itemQuantity(CatchChallenger::Api_protocol *api,const uint32_t &itemId);
    static bool haveReputationRequirements(CatchChallenger::Api_protocol *api,const QList<CatchChallenger::ReputationRequirements> &reputationList);
    static bool haveReputationRequirements(CatchChallenger::Api_protocol *api,const std::vector<CatchChallenger::ReputationRequirements> &reputationList);
    static void appendReputationRewards(CatchChallenger::Api_protocol *api,const QList<CatchChallenger::ReputationRewards> &reputationList);
    static void appendReputationRewards(CatchChallenger::Api_protocol *api,const std::vector<CatchChallenger::ReputationRewards> &reputationList);

    void have_inventory_slot(const std::unordered_map<uint16_t, uint32_t> &items, const std::unordered_map<uint16_t, uint32_t> &warehouse_items);
    static void have_inventory(CatchChallenger::Api_protocol *api,const std::unordered_map<uint16_t, uint32_t> &items, const std::unordered_map<uint16_t, uint32_t> &warehouse_items);
    static void add_to_inventory(CatchChallenger::Api_protocol *api,const uint32_t &item,const uint32_t &quantity=1);
    static void add_to_inventory(CatchChallenger::Api_protocol *api,const QList<QPair<uint16_t,uint32_t> > &items);
    static void add_to_inventory(CatchChallenger::Api_protocol *api, const QHash<uint16_t,uint32_t> &items);
    void add_to_inventory_slot(const QHash<uint16_t,uint32_t> &items);
    static void remove_to_inventory(CatchChallenger::Api_protocol *api,const QHash<uint16_t,uint32_t> &items);
    void remove_to_inventory_slot(const QHash<uint16_t,uint32_t> &items);
    static void remove_to_inventory(CatchChallenger::Api_protocol *api,const uint32_t &itemId,const uint32_t &quantity=1);

    static void showTip(const QString &text);

    static bool canGoTo(CatchChallenger::Api_protocol *api, const CatchChallenger::Direction &direction, const MapServerMini &map, COORD_TYPE x, COORD_TYPE y, const bool &checkCollision, const bool &allowTeleport);
    static bool move(CatchChallenger::Api_protocol *api, CatchChallenger::Direction direction, const MapServerMini ** map, COORD_TYPE *x, COORD_TYPE *y, const bool &checkCollision, const bool &allowTeleport);
    static bool moveWithoutTeleport(CatchChallenger::Api_protocol *api, CatchChallenger::Direction direction, const MapServerMini ** map, COORD_TYPE *x, COORD_TYPE *y, const bool &checkCollision, const bool &allowTeleport);
    static bool teleport(const MapServerMini **map, COORD_TYPE *x, COORD_TYPE *y);
    static bool isWalkable(const MapServerMini &map, const uint8_t &x, const uint8_t &y);
    static bool isDirt(const MapServerMini &map, const uint8_t &x, const uint8_t &y);
    static bool needBeTeleported(const MapServerMini &map, const COORD_TYPE &x, const COORD_TYPE &y);

    static void seed_planted(CatchChallenger::Api_protocol *api,const bool &ok);
    void seed_planted_slot(const bool &ok);
    static void plant_collected(CatchChallenger::Api_protocol *api,const CatchChallenger::Plant_collect &stat);
    void plant_collected_slot(const CatchChallenger::Plant_collect &stat);
public slots:
    bool preload_the_map();
signals:
    void preload_the_map_finished();
    void stepListFinish();
private:
    QTimer textTimer;
    uint64_t loaded;
public:
    //map
    /* map related */
    struct Map_semi_border_content_top_bottom
    {
        std::string fileName;
        int16_t x_offset;//can be negative, it's an offset!
    };

    struct Map_semi_border_content_left_right
    {
        std::string fileName;
        int16_t y_offset;//can be negative, it's an offset!
    };

    struct Map_semi_border
    {
        Map_semi_border_content_top_bottom top;
        Map_semi_border_content_top_bottom bottom;
        Map_semi_border_content_left_right left;
        Map_semi_border_content_left_right right;
    };

    enum MapConditionType : uint8_t
    {
        MapConditionType_None=0x00,
        MapConditionType_Quest=0x01,
        MapConditionType_Item=0x02,
        MapConditionType_Clan=0x03,
        MapConditionType_FightBot=0x04
    };

    struct MapCondition
    {
        MapConditionType type;
        uint32_t value;
    };

    struct MapMonster
    {
        uint32_t id;
        uint8_t minLevel;
        uint8_t maxLevel;
        uint8_t luck;
    };

    enum ParsedLayerLedges : uint8_t
    {
        ParsedLayerLedges_NoLedges=0x00,
        ParsedLayerLedges_LedgesLeft=0x01,
        ParsedLayerLedges_LedgesRight=0x02,
        ParsedLayerLedges_LedgesTop=0x03,
        ParsedLayerLedges_LedgesBottom=0x04
    };

    struct MonstersCollisionValue
    {
        std::vector<uint32_t/*index into CatchChallenger::CommonDatapack::commonDatapack.monstersCollision*/> walkOn;
        std::vector<uint32_t/*index into CatchChallenger::CommonDatapack::commonDatapack.monstersCollision*/> actionOn;
        //it's the dynamic part
        struct MonstersCollisionValueOnCondition
        {
            uint8_t event;
            uint8_t event_value;
            std::vector<MapMonster> monsters;
        };
        //static part pre-computed
        struct MonstersCollisionContent
        {
            std::vector<MapMonster> defaultMonsters;
            std::vector<MonstersCollisionValueOnCondition> conditions;
        };
        std::vector<MonstersCollisionContent> walkOnMonsters;
        std::vector<std::vector<MapMonster> > actionOnMonsters;
    };

    struct ParsedLayer
    {
        bool *walkable;
        uint8_t *monstersCollisionMap;
        std::vector<MonstersCollisionValue> monstersCollisionList;
        //bool *grass;
        bool *dirt;
        //not stored as ParsedLayerLedges to prevent memory space unused
        uint8_t *ledges;
    };
    struct Map_semi
    {
        //conversion x,y to position: x+y*width
        CatchChallenger::CommonMap* map;
        Map_semi_border border;
        CatchChallenger::Map_to_send old_map;
    };

    std::vector<Map_semi> semi_loaded_map;
    std::unordered_map<std::string,CatchChallenger::CommonMap *> map_list;
    CatchChallenger::CommonMap ** flat_map_list;
    std::unordered_map<uint32_t,std::string > id_map_to_map;
    //bot
    std::unordered_map<std::string/*file name*/,std::unordered_map<uint8_t/*bot id*/,CatchChallenger::Bot> > botFiles;
    std::unordered_set<uint32_t> botIdLoaded;

    QTimer moveTimer;
private:
    void doMove();
    void doText();
    void new_chat_text(const CatchChallenger::Chat_type &chat_type,const QString &text,const QString &pseudo,const CatchChallenger::Player_type &type);
    void preload_the_bots(const std::vector<Map_semi> &semi_loaded_map);
    void loadBotFile(const std::string &mapfile,const std::string &file);
    void checkOnTileEvent(Player &player);
};

#endif // ACTIONS_ACTION_BOT_INTERFACE_H
