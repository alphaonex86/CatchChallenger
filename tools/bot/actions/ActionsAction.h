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
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

class ActionsAction : public ActionsBotInterface
{
    Q_OBJECT
public:
    ActionsAction();
    ~ActionsAction();
    void insert_player(CatchChallenger::Api_protocol *api,const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction);
    bool preload_the_map();
    bool preload_the_map_step1();
    bool preload_the_map_step2();
private:
    QTimer moveTimer;
    QTimer textTimer;

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

private:
    void doMove();
    void doText();
    void new_chat_text(const CatchChallenger::Chat_type &chat_type,const QString &text,const QString &pseudo,const CatchChallenger::Player_type &type);
    void preload_the_bots(const std::vector<Map_semi> &semi_loaded_map);
    void loadBotFile(const std::string &mapfile,const std::string &file);
};

#endif // ACTIONS_ACTION_BOT_INTERFACE_H
