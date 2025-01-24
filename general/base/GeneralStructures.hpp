#ifndef CATCHCHALLENGER_GENERAL_STRUCTURES_H
#define CATCHCHALLENGER_GENERAL_STRUCTURES_H

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "GeneralType.hpp"

#if defined(CATCHCHALLENGER_CLIENT)
//client
#ifndef MAXIMIZEPERFORMANCEOVERDATABASESIZE
#define MAXIMIZEPERFORMANCEOVERDATABASESIZE
#endif
#else
//server
//#define MAXIMIZEPERFORMANCEOVERDATABASESIZE
#endif

#ifndef MAXIMIZEPERFORMANCEOVERDATABASESIZE
#include <map>
#include <set>
#endif

namespace CatchChallenger {

enum Chat_type : uint8_t
{
    Chat_type_local = 0x01,
    Chat_type_all = 0x02,
    Chat_type_clan = 0x03,
    Chat_type_pm = 0x06,
    Chat_type_system = 0x07,
    Chat_type_system_important = 0x08
};

enum Player_type : uint8_t
{
    Player_type_normal = 0x10,
    Player_type_premium = 0x20,
    Player_type_gm = 0x30,
    Player_type_dev = 0x40
};

enum Orientation : uint8_t
{
    Orientation_none = 0,//where the target orientation don't matter
    Orientation_top = 1,
    Orientation_right = 2,
    Orientation_bottom = 3,
    Orientation_left = 4
};

enum ActionAllow : uint8_t
{
    ActionAllow_Nothing=0x00,
    ActionAllow_Clan=0x01
};

enum Direction : uint8_t
{
    Direction_look_at_top = 1,
    Direction_look_at_right = 2,
    Direction_look_at_bottom = 3,
    Direction_look_at_left = 4,
    Direction_move_at_top = 5,
    Direction_move_at_right = 6,
    Direction_move_at_bottom = 7,
    Direction_move_at_left = 8
};

enum MonsterItemEffectType : uint8_t
{
    MonsterItemEffectType_AddHp = 0x01,
    MonsterItemEffectType_RemoveBuff = 0x02
};

enum MonsterItemEffectTypeOutOfFight : uint8_t
{
    MonsterItemEffectTypeOutOfFight_AddLevel = 0x01
};

enum RecipeUsage : uint8_t
{
    RecipeUsage_ok=0x01,
    RecipeUsage_impossible=0x02,//crafting materials not used
    RecipeUsage_failed=0x03//crafting materials used
};

enum Plant_collect : uint8_t
{
    Plant_collect_correctly_collected=0x01,
    /*Plant_collect_empty_dirt=0x02,
    Plant_collect_owned_by_another_player=0x03,
    Plant_collect_impossible=0x04*/
};

enum ObjectUsage : uint8_t
{
    ObjectUsage_correctlyUsed=0x01,//is correctly used
    ObjectUsage_failedWithConsumption=0x02,//failed to use with consumption of the object
    ObjectUsage_failedWithoutConsumption=0x03//failed to use without consumption of the object
};

enum BuyStat : uint8_t
{
    BuyStat_Done=0x01,
    BuyStat_BetterPrice=0x02,
    BuyStat_HaveNotQuantity=0x03,
    BuyStat_PriceHaveChanged=0x04
};
enum SoldStat : uint8_t
{
    SoldStat_Done=0x01,
    SoldStat_BetterPrice=0x02,
    SoldStat_WrongQuantity=0x03,
    SoldStat_PriceHaveChanged=0x04
};

struct map_management_insert
{
    std::string fileName;
    COORD_TYPE x;
    COORD_TYPE y;
    Direction direction;//can be insert as direction when changing of map
    uint16_t speed;
};

struct map_management_movement
{
    uint8_t movedUnit;
    Direction direction;
};

struct map_management_move
{
    uint32_t id;
    std::vector<map_management_movement> movement_list;
};

struct Player_public_informations
{
    SIMPLIFIED_PLAYER_ID_TYPE simplifiedId;
    std::string pseudo;
    Player_type type;
    uint8_t skinId;
    uint16_t monsterId;//the monster follow the player on map, other player can see it
    SPEED_TYPE speed;

    #ifdef CATCHCHALLENGER_DB_FILE
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << simplifiedId << pseudo << (uint8_t)type << skinId << monsterId << speed;
    }
    template <class B>
    void parse(B& buf) {
        uint8_t t;
        buf >> simplifiedId >> pseudo >> t >> skinId >> monsterId >> speed;
        type=(Player_type)t;
    }
    #endif
    #endif
};

enum Gender : uint8_t
{
    Gender_Male=0x01,
    Gender_Female=0x02,
    Gender_Unknown=0x03
};
struct PlayerBuff
{
    uint8_t buff;
    uint8_t level;
    uint8_t remainingNumberOfTurn;
    #ifdef CATCHCHALLENGER_DB_FILE
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << buff << level << remainingNumberOfTurn;
    }
    template <class B>
    void parse(B& buf) {
        buf >> buff >> level >> remainingNumberOfTurn;
    }
    #endif
    #endif
};

class PublicPlayerMonster
{
    public:
    CATCHCHALLENGER_TYPE_MONSTER monster;
    uint8_t level;
    uint32_t hp;
    uint16_t catched_with;
    Gender gender;
    std::vector<PlayerBuff> buffs;
    #ifdef CATCHCHALLENGER_DB_FILE
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << monster << level << hp << catched_with << gender << buffs;
    }
    template <class B>
    void parse(B& buf) {
        buf >> monster >> level >> hp >> catched_with >> gender >> buffs;
    }
    #endif
    #endif
};

class PlayerMonster : public PublicPlayerMonster
{
    public:
    class PlayerSkill
    {
    public:
        CATCHCHALLENGER_TYPE_SKILL skill;
        uint8_t level;//start at 1
        uint8_t endurance;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << skill << level << endurance;
        }
        template <class B>
        void parse(B& buf) {
            buf >> skill >> level >> endurance;
        }
        #endif
    };
    uint32_t remaining_xp;
    uint32_t egg_step;
    //in form of list to get random into the list
    std::vector<PlayerSkill> skills;
    #ifndef CATCHCHALLENGER_VERSION_SINGLESERVER
    uint32_t id;//id into the db, only on server part, but no way to leave from here with risk of structure problem
    #endif
    uint32_t character_origin;
    #ifdef CATCHCHALLENGER_DB_FILE
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << monster << level << hp << catched_with << (uint8_t)gender << buffs << remaining_xp << egg_step << skills << character_origin;
    }
    template <class B>
    void parse(B& buf) {
        uint8_t value=0;
        buf >> monster >> level >> hp >> catched_with >> value >> buffs >> remaining_xp >> egg_step >> skills >> character_origin;
        gender=(Gender)value;
    }
    #endif
    #endif
};

bool operator!=(const PlayerMonster &monster1,const PlayerMonster &monster2);

class PlayerQuest
{
public:
    uint8_t step;
    bool finish_one_time;
    #ifdef CATCHCHALLENGER_DB_FILE
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << step << finish_one_time;
    }
    template <class B>
    void parse(B& buf) {
        buf >> step >> finish_one_time;
    }
    #endif
    #endif
};

class PlayerReputation
{
public:
    int8_t level;
    int32_t point;
    #ifdef CATCHCHALLENGER_DB_FILE
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << level << point;
    }
    template <class B>
    void parse(B& buf) {
        buf >> level >> point;
    }
    #endif
    #endif
};

class PlayerPlant
{
public:
    uint8_t plant;//plant id
    uint64_t mature_at;//timestamp when is mature in second
    #ifdef CATCHCHALLENGER_DB_FILE
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << plant << mature_at;
    }
    template <class B>
    void parse(B& buf) {
        buf >> plant >> mature_at;
    }
    #endif
    #endif
};

//this structure allow load only map and near map and not all the data (not implemented)
class Player_private_and_public_informations_Map
{
    //std::pair<COORD_TYPE,COORD_TYPE> allow directly work without DB resolution
public:
#ifdef MAXIMIZEPERFORMANCEOVERDATABASESIZE
    //here to send at character login
    std::unordered_set<std::pair<COORD_TYPE,COORD_TYPE>> itemOnMap;
    std::unordered_map<std::pair<COORD_TYPE,COORD_TYPE>,PlayerPlant> plantOnMap;
    std::unordered_set<CATCHCHALLENGER_TYPE_BOTID> bot_already_beaten;
#else
    std::set<std::pair<COORD_TYPE,COORD_TYPE>> itemOnMap;
    std::map<std::pair<COORD_TYPE,COORD_TYPE>,PlayerPlant> plantOnMap;
    std::set<CATCHCHALLENGER_TYPE_BOTID> bot_already_beaten;
#endif
};

class Player_private_and_public_informations
{
public:
    Player_public_informations public_informations;
    uint64_t cash,warehouse_cash;
    //crafting
    char * recipes;//CommonDatapack::commonDatapack.get_craftingRecipesMaxId()/8+1, if store with HFS then store CommonDatapack::commonDatapack.get_craftingRecipesMaxId() as header
    /// max monster 255 inventory, 255 storage
    std::vector<PlayerMonster> playerMonster,warehouse_playerMonster;
    CLAN_ID_TYPE clan;//0 == no clan
    char * encyclopedia_monster;//CommonDatapack::commonDatapack.get_monstersMaxId()/8+1, if store with HFS then store get_monstersMaxId() as header
    char * encyclopedia_item;//CommonDatapack::commonDatapack.items.item.size()/8+1, if store with HFS then store CommonDatapack::commonDatapack.items.item.size() as header
    uint32_t repel_step;
    bool clan_leader;

    /* item and plant is keep under database format to keep the dataintegrity and save quickly the data
     * More memory usage by 2x, but improve the code maintenance because the id in memory is id in database
     * Less dictionary resolution to improve the cache flush */

    #ifdef MAXIMIZEPERFORMANCEOVERDATABASESIZE
        std::unordered_set<ActionAllow,std::hash<uint8_t>/*what hash use*/ > allow;
        //here to send at character login
        std::unordered_map<CATCHCHALLENGER_TYPE_QUEST, PlayerQuest> quests;
        std::unordered_map<uint8_t,PlayerReputation> reputation;
        std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,CATCHCHALLENGER_TYPE_ITEM_QUANTITY> items,warehouse_items;
        std::unordered_map<CATCHCHALLENGER_TYPE_MAPID,Player_private_and_public_informations_Map> mapData;
    #else
        std::set<ActionAllow> allow;
        //here to send at character login
                std::map<CATCHCHALLENGER_TYPE_QUEST, PlayerQuest> quests;
        std::map<uint8_t/*internal id*/,PlayerReputation> reputation;
        std::map<CATCHCHALLENGER_TYPE_ITEM,CATCHCHALLENGER_TYPE_ITEM_QUANTITY> items,warehouse_items;
        std::map<CATCHCHALLENGER_TYPE_MAPID,Player_private_and_public_informations_Map> mapData;
    #endif
};

class CharacterEntry
{
public:
    uint32_t character_id;
    std::string pseudo;
    uint8_t charactersGroupIndex;
    uint8_t skinId;
    uint32_t delete_time_left;
    uint32_t played_time;
    uint64_t last_connect;
    #ifdef CATCHCHALLENGER_DB_FILE
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << character_id << pseudo << charactersGroupIndex << skinId << delete_time_left << played_time << last_connect;
    }
    template <class B>
    void parse(B& buf) {
        buf >> character_id >> pseudo >> charactersGroupIndex >> skinId >> delete_time_left >> played_time >> last_connect;
    }
    #endif
    #endif
};

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

enum ParsedLayerLedges : uint8_t
{
    ParsedLayerLedges_NoLedges=0x00,
    ParsedLayerLedges_LedgesLeft=0x01,
    ParsedLayerLedges_LedgesRight=0x02,
    ParsedLayerLedges_LedgesTop=0x03,
    ParsedLayerLedges_LedgesBottom=0x04
};

enum QuantityType : uint8_t
{
    QuantityType_Quantity,
    QuantityType_Percent
};

enum ApplyOn : uint8_t
{
    ApplyOn_AloneEnemy=0x01,
    ApplyOn_AllEnemy=0x02,
    ApplyOn_Themself=0x04,
    ApplyOn_AllAlly=0x08,
    ApplyOn_Nobody=0x00
};

enum Place : uint8_t
{
    OnPlayer=0,
    WareHouse=1,
    Market=2
};

struct ItemToSellOrBuy
{
    CATCHCHALLENGER_TYPE_ITEM object;
    uint32_t price;
    uint32_t quantity;
};

class City
{
public:
    class Capture
    {
    public:
        enum Frequency : uint8_t
        {
            Frequency_week=0x01,
            Frequency_month=0x02
        };
        Frequency frenquency;
        enum Day : uint8_t
        {
            Monday=0x01,
            Tuesday=0x02,
            Wednesday=0x03,
            Thursday=0x04,
            Friday=0x05,
            Saturday=0x06,
            Sunday=0x07
        };
        Day day;
        uint8_t hour,minute;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << (uint8_t)frenquency << (uint8_t)day << hour << minute;
        }
        template <class B>
        void parse(B& buf) {
            uint8_t value=0;
            buf >> value;
            frenquency=(Frequency)value;
            buf >> value;
            day=(Day)value;
            buf >> hour >> minute;
        }
        #endif
    };
    Capture capture;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << capture;
    }
    template <class B>
    void parse(B& buf) {
        buf >> capture;
    }
    #endif
};

struct IndustryStatus
{
    uint64_t last_update;
    std::unordered_map<uint32_t,uint32_t> resources;
    std::unordered_map<uint32_t,uint32_t> products;
    #ifdef CATCHCHALLENGER_DB_FILE
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << last_update << resources << products;
    }
    template <class B>
    void parse(B& buf) {
        buf >> last_update >> resources >> products;
    }
    #endif
    #endif
};

enum MonstersCollisionType : uint8_t
{
    MonstersCollisionType_WalkOn=0x00,
    MonstersCollisionType_ActionOn=0x01
};

/** --------------- used to parse, but useless in server when datapack is loaded, used into client for progressive load ------------ **/
class MonstersCollisionTemp
{
public:
    //needed at runtime
    MonstersCollisionType type;
    CATCHCHALLENGER_TYPE_ITEM item;

    //temporary to parse
    std::string tile;
    std::string layer;
    std::vector<std::string> monsterTypeList;
    std::vector<std::string> defautMonsterTypeList;
    std::string background;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    #ifdef CATCHCHALLENGER_CLIENT
    template <class B>
    void serialize(B& buf) const {
        buf << (uint8_t)type << item << tile << layer << monsterTypeList << defautMonsterTypeList << background << events;
    }
    template <class B>
    void parse(B& buf) {
        uint8_t typetemp;
        buf >> typetemp >> item >> tile >> layer >> monsterTypeList >> defautMonsterTypeList >> background >> events;
        type=(MonstersCollisionType)typetemp;
    }
    #endif
    #endif
    class MonstersCollisionEvent
    {
    public:
        uint8_t event;
        uint8_t event_value;
        std::vector<std::string > monsterTypeList;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        #ifdef CATCHCHALLENGER_CLIENT
        template <class B>
        void serialize(B& buf) const {
            buf << event << event_value << monsterTypeList;
        }
        template <class B>
        void parse(B& buf) {
            buf >> event >> event_value >> monsterTypeList;
        }
        #endif
        #endif
    };
    std::vector<MonstersCollisionEvent> events;
};

class Type
{
public:
    std::string name;
    std::unordered_map<uint8_t,int8_t> multiplicator;//negative = divide, not multiply
#ifdef CATCHCHALLENGER_CACHE_HPS
template <class B>
void serialize(B& buf) const {
    #if ! defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_CLIENT)
    buf << name;
    #endif
    buf << multiplicator;
}
template <class B>
void parse(B& buf) {
    #if ! defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_CLIENT)
    buf >> name;
    #endif
    buf >> multiplicator;
}
#endif
};

/** ---------------- Static datapack for serialiser ------------------------ **/

class MonstersCollision
{
public:
    //needed at runtime
    MonstersCollisionType type;
    CATCHCHALLENGER_TYPE_ITEM item;

    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << (uint8_t)type << item;
    }
    template <class B>
    void parse(B& buf) {
        uint8_t value=0;
        buf >> value >> item;
        type=(MonstersCollisionType)value;
    }
    #endif
};

class Reputation
{
public:
    std::vector<int32_t> reputation_positive/*start with 0*/,reputation_negative;
    /*server only*/
    int reverse_database_id;
    std::string name;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << reputation_positive << reputation_negative/* << reverse_database_id loaded after db*/ << name;
    }
    template <class B>
    void parse(B& buf) {
        buf >> reputation_positive >> reputation_negative/* >> reverse_database_id loaded after db*/ >> name;
    }
    #endif
};

class ReputationRewards
{
public:
    uint8_t reputationId;
    int32_t point;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << reputationId << point;
    }
    template <class B>
    void parse(B& buf) {
        buf >> reputationId >> point;
    }
    #endif
};

class ReputationRequirements
{
public:
    uint8_t reputationId;
    uint8_t level;
    bool positif;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << reputationId << level << positif;
    }
    template <class B>
    void parse(B& buf) {
        buf >> reputationId >> level >> positif;
    }
    #endif
};

class QuestRequirements
{
public:
    uint16_t quest;
    bool inverse;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << quest << inverse;
    }
    template <class B>
    void parse(B& buf) {
        buf >> quest >> inverse;
    }
    #endif
};

class Plant
{
public:
    CATCHCHALLENGER_TYPE_ITEM itemUsed;
    uint32_t fruits_seconds;
    //float quantity;//splited into 2 integer
    uint16_t fix_quantity;
    uint16_t random_quantity;
    //minimal memory impact at exchange of drop dual xml parse
    uint16_t sprouted_seconds;
    uint16_t taller_seconds;
    uint16_t flowering_seconds;
    struct Requirements
    {
        std::vector<ReputationRequirements> reputation;
    };
    Requirements requirements;
    struct Rewards
    {
        std::vector<ReputationRewards> reputation;
    };
    Rewards rewards;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << itemUsed << fruits_seconds << fix_quantity << random_quantity << sprouted_seconds << taller_seconds << flowering_seconds;
        buf << (uint8_t)requirements.reputation.size();
        for(auto const& v: requirements.reputation)
            buf << v;
        buf << (uint8_t)rewards.reputation.size();
        for(auto const& v: rewards.reputation)
            buf << v;
    }
    template <class B>
    void parse(B& buf) {
      buf >> itemUsed >> fruits_seconds >> fix_quantity >> random_quantity >> sprouted_seconds >> taller_seconds >> flowering_seconds;
      uint8_t vectorsize=0;
      buf >> vectorsize;
      for(unsigned int i=0; i<vectorsize; i++) {
          ReputationRequirements reputationRequirement;
          buf >> reputationRequirement;
          requirements.reputation.push_back(reputationRequirement);
      }
      buf >> vectorsize;
      for(unsigned int i=0; i<vectorsize; i++) {
          ReputationRewards reputationRewards;
          buf >> reputationRewards;
          rewards.reputation.push_back(reputationRewards);
      }
    }
    #endif
};

class Item
{
public:
    uint32_t price;//if 0 you can't sell or buy it
    bool consumeAtUse;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << price << consumeAtUse;
    }
    template <class B>
    void parse(B& buf) {
        buf >> price >> consumeAtUse;
    }
    #endif
};

class Trap
{
public:
    float bonus_rate;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << bonus_rate;
    }
    template <class B>
    void parse(B& buf) {
        buf >> bonus_rate;
    }
    #endif
};

class MonsterItemEffect
{
public:
    MonsterItemEffectType type;
    union Data {
       uint32_t hp;
       uint8_t buff;
    } data;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << (uint8_t)type << data.hp;
    }
    template <class B>
    void parse(B& buf) {
        uint8_t value=0;
        buf >> value >> data.hp;
        type=(MonsterItemEffectType)value;
    }
    #endif
};

class MonsterItemEffectOutOfFight
{
public:
    MonsterItemEffectTypeOutOfFight type;
    union Data {
       uint8_t level;
    } data;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << (uint8_t)type << data.level;
    }
    template <class B>
    void parse(B& buf) {
        uint8_t value=0;
        buf >> value >> data.level;
        type=(MonsterItemEffectTypeOutOfFight)value;
    }
    #endif
};

class ItemFull
{
public:
    std::unordered_map<uint16_t, std::vector<MonsterItemEffect> > monsterItemEffect;
    std::unordered_map<uint16_t, std::vector<MonsterItemEffectOutOfFight> > monsterItemEffectOutOfFight;
    std::unordered_map<CATCHCHALLENGER_TYPE_ITEM/*item*/, std::unordered_map<uint16_t/*monster*/,uint16_t/*evolveTo*/> > evolutionItem;
    std::unordered_map<CATCHCHALLENGER_TYPE_ITEM/*item*/, std::unordered_set<uint16_t/*monster*/> > itemToLearn;
    std::unordered_map<uint16_t, uint32_t> repel;
    std::unordered_map<CATCHCHALLENGER_TYPE_ITEM, Item> item;
    CATCHCHALLENGER_TYPE_ITEM itemMaxId;
    std::unordered_map<uint16_t, Trap> trap;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << monsterItemEffect << monsterItemEffectOutOfFight << evolutionItem << itemToLearn << repel << item << itemMaxId << trap;
    }
    template <class B>
    void parse(B& buf) {
        buf >> monsterItemEffect >> monsterItemEffectOutOfFight >> evolutionItem >> itemToLearn >> repel >> item >> itemMaxId >> trap;
    }
    #endif
};

class LayersOptions
{
public:
    uint8_t zoom;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << zoom;
    }
    template <class B>
    void parse(B& buf) {
        buf >> zoom;
    }
    #endif
};

class IndustryLink
{
public:
    uint16_t industry;
    class Requirements
    {
    public:
        std::vector<ReputationRequirements> reputation;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << reputation;
        }
        template <class B>
        void parse(B& buf) {
            buf >> reputation;
        }
        #endif
    };
    Requirements requirements;
    class Rewards
    {
    public:
        std::vector<ReputationRewards> reputation;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << reputation;
        }
        template <class B>
        void parse(B& buf) {
            buf >> reputation;
        }
        #endif
    };
    Rewards rewards;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << industry << requirements << rewards;
    }
    template <class B>
    void parse(B& buf) {
        buf >> industry >> requirements >> rewards;
    }
    #endif
};

class Industry
{
public:
    uint64_t time;//should not be too short
    uint32_t cycletobefull;
    class Resource
    {
    public:
        CATCHCHALLENGER_TYPE_ITEM item;
        uint32_t quantity;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << item << quantity;
        }
        template <class B>
        void parse(B& buf) {
            buf >> item >> quantity;
        }
        #endif
    };
    class Product
    {
    public:
        CATCHCHALLENGER_TYPE_ITEM item;
        uint32_t quantity;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << item << quantity;
        }
        template <class B>
        void parse(B& buf) {
            buf >> item >> quantity;
        }
        #endif
    };
    std::vector<Resource> resources;
    std::vector<Product> products;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << time << cycletobefull << resources << products;
    }
    template <class B>
    void parse(B& buf) {
        buf >> time >> cycletobefull >> resources >> products;
    }
    #endif
};

class Event
{
public:
    std::string name;
    std::vector<std::string> values;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << name << values;
    }
    template <class B>
    void parse(B& buf) {
        buf >> name >> values;
    }
    #endif
};

class MapFight
{
public:
    CATCHCHALLENGER_TYPE_BOTID fightBot;
    CATCHCHALLENGER_TYPE_MAPID mapId;
};

class MapCondition
{
public:
    MapConditionType type;
    union Data {
       CATCHCHALLENGER_TYPE_QUEST quest;
       CATCHCHALLENGER_TYPE_ITEM item;
       MapFight fight;
    } data;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << (uint8_t)type << data.quest;
    }
    template <class B>
    void parse(B& buf) {
        uint8_t temp=0;
        buf >> temp >> data.quest;
        type=(MapConditionType)temp;
    }
    #endif
};

class MapMonster
{
public:
    CATCHCHALLENGER_TYPE_MONSTER id;
    uint8_t minLevel;
    uint8_t maxLevel;
    uint8_t luck;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << id << minLevel << maxLevel << luck;
    }
    template <class B>
    void parse(B& buf) {
        buf >> id >> minLevel >> maxLevel >> luck;
    }
    #endif
};

class MonstersCollisionValue
{
public:
    std::vector<uint8_t/*index into CatchChallenger::CommonDatapack::commonDatapack.monstersCollision*/> walkOn;
    std::vector<uint8_t/*index into CatchChallenger::CommonDatapack::commonDatapack.monstersCollision*/> actionOn;
    //it's the dynamic part
    class MonstersCollisionValueOnCondition
    {
    public:
        uint8_t event;
        uint8_t event_value;
        std::vector<MapMonster> monsters;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << event << event_value << monsters;
        }
        template <class B>
        void parse(B& buf) {
            buf >> event >> event_value >> monsters;
        }
        #endif
    };
    //static part pre-computed
    class MonstersCollisionContent
    {
    public:
        std::vector<MapMonster> defaultMonsters;
        std::vector<MonstersCollisionValueOnCondition> conditions;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << defaultMonsters << conditions;
        }
        template <class B>
        void parse(B& buf) {
            buf >> defaultMonsters >> conditions;
        }
        #endif
    };
    std::vector<MonstersCollisionContent> walkOnMonsters;
    std::vector<std::vector<MapMonster> > actionOnMonsters;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << walkOn << actionOn << walkOnMonsters << actionOnMonsters;
    }
    template <class B>
    void parse(B& buf) {
        buf >> walkOn >> actionOn >> walkOnMonsters >> actionOnMonsters;
    }
    #endif
};

struct ParsedLayer
{
    /// \warning not serialisable directly, lack uint8_t * size, serialise out of there
public:
    /* 0 walkable: index = 0 for monster is used into cave
     * 254 not walkable
     * 253 ParsedLayerLedges_LedgesBottom
     * 252 ParsedLayerLedges_LedgesTop
     * 251 ParsedLayerLedges_LedgesRight
     * 250 ParsedLayerLedges_LedgesLeft
     * 249 dirt
     * 200 - 248 reserved */
    uint8_t *simplifiedMap;
    /* 0 cave def
     * 1-199 monster def and condition */
    std::vector<MonstersCollisionValue> monstersCollisionList;
};

class CraftingRecipe
{
public:
    CATCHCHALLENGER_TYPE_ITEM itemToLearn;
    CATCHCHALLENGER_TYPE_ITEM doItemId;
    uint16_t quantity;
    uint8_t success;//0-100
    class Material
    {
    public:
        CATCHCHALLENGER_TYPE_ITEM item;
        uint32_t quantity;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << item << quantity;
        }
        template <class B>
        void parse(B& buf) {
            buf >> item >> quantity;
        }
        #endif
    };
    std::vector<Material> materials;
    class Requirements
    {
    public:
        std::vector<ReputationRequirements> reputation;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << reputation;
        }
        template <class B>
        void parse(B& buf) {
            buf >> reputation;
        }
        #endif
    };
    Requirements requirements;
    class Rewards
    {
    public:
        std::vector<ReputationRewards> reputation;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << reputation;
        }
        template <class B>
        void parse(B& buf) {
            buf >> reputation;
        }
        #endif
    };
    Rewards rewards;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << itemToLearn << doItemId << quantity << success << materials << requirements << rewards;
    }
    template <class B>
    void parse(B& buf) {
        buf >> itemToLearn >> doItemId >> quantity >> success >> materials >> requirements >> rewards;
    }
    #endif
};

class Shop
{
public:
    std::vector<uint32_t> prices;
    std::vector<CATCHCHALLENGER_TYPE_ITEM> items;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << prices << items;
    }
    template <class B>
    void parse(B& buf) {
        buf >> prices >> items;
    }
    #endif
};

class Buff
{
public:
    enum Duration : uint8_t
    {
        Duration_Always,
        Duration_ThisFight,
        Duration_NumberOfTurn
    };
    class Effect
    {
    public:
        enum EffectOn : uint8_t
        {
            EffectOn_HP,
            EffectOn_Defense,
            EffectOn_Attack
        };
        EffectOn on;
        int32_t quantity;
        QuantityType type;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << (uint8_t)on << quantity << (uint8_t)type;
        }
        template <class B>
        void parse(B& buf) {
            uint8_t value=0;
            uint8_t valueB=0;
            buf >> value >> quantity >> valueB;
            on=(EffectOn)value;
            type=(QuantityType)valueB;
        }
        #endif
    };
    class EffectInWalk
    {
    public:
        Effect effect;
        uint32_t steps;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << effect << steps;
        }
        template <class B>
        void parse(B& buf) {
            buf >> effect >> steps;
        }
        #endif
    };
    class GeneralEffect
    {
    public:
        std::vector<EffectInWalk> walk;
        std::vector<Effect> fight;
        float capture_bonus;
        Duration duration;
        uint8_t durationNumberOfTurn;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << walk << fight << capture_bonus << (uint8_t)duration << durationNumberOfTurn;
        }
        template <class B>
        void parse(B& buf) {
            uint8_t value=0;
            buf >> walk >> fight >> capture_bonus >> value >> durationNumberOfTurn;
            duration=(Duration)value;
        }
        #endif
    };
    std::vector<GeneralEffect> level;//first entry is buff level 1
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << level;
    }
    template <class B>
    void parse(B& buf) {
        buf >> level;
    }
    #endif
};

class MonsterDrops
{
public:
    CATCHCHALLENGER_TYPE_ITEM item;
    uint32_t quantity_min;
    uint32_t quantity_max;
    uint8_t luck;//seam be 0 to 100
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << item << quantity_min << quantity_max << luck;
    }
    template <class B>
    void parse(B& buf) {
        buf >> item >> quantity_min >> quantity_max >> luck;
    }
    #endif
};

class Skill
{
public:
    enum AttackReturnCase : uint8_t
    {
        AttackReturnCase_NormalAttack=0x01,
        AttackReturnCase_MonsterChange=0x02,
        AttackReturnCase_ItemUsage=0x03
    };
    class BuffEffect
    {
    public:
        uint8_t buff;
        ApplyOn on;
        uint8_t level;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << buff << (uint8_t)on << level;
        }
        template <class B>
        void parse(B& buf) {
            uint8_t value=0;
            buf >> buff >> value >> level;
            on=(ApplyOn)value;
        }
        #endif
    };
    class LifeEffect
    {
    public:
        int32_t quantity;
        QuantityType type;
        ApplyOn on;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << quantity << (uint8_t)type << (uint8_t)on;
        }
        template <class B>
        void parse(B& buf) {
            uint8_t value=0;
            uint8_t valueB=0;
            buf >> quantity >> value >> valueB;
            type=(QuantityType)value;
            on=(ApplyOn)valueB;
        }
        #endif
    };
    class LifeEffectReturn
    {
    public:
        int32_t quantity;
        ApplyOn on;
        bool critical;
        float effective;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << quantity << (uint8_t)on << critical << effective;
        }
        template <class B>
        void parse(B& buf) {
            uint8_t value=0;
            buf >> quantity >> value >> critical >> effective;
            on=(ApplyOn)value;
        }
        #endif
    };
    struct AttackReturn
    {
        bool doByTheCurrentMonster;
        AttackReturnCase attackReturnCase;
        //normal attack
        bool success;
        uint16_t attack;
        std::vector<BuffEffect> addBuffEffectMonster,removeBuffEffectMonster;
        std::vector<LifeEffectReturn> lifeEffectMonster;
        std::vector<LifeEffectReturn> buffLifeEffectMonster;
        //change monster if monsterPlace !=0
        uint8_t monsterPlace;
        PublicPlayerMonster publicPlayerMonster;
        //use objet on monster if item!=0
        bool on_current_monster;
        CATCHCHALLENGER_TYPE_ITEM item;
    };
    class Buff
    {
    public:
        uint8_t success;
        BuffEffect effect;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << success << effect;
        }
        template <class B>
        void parse(B& buf) {
            buf >> success >> effect;
        }
        #endif
    };
    class Life
    {
    public:
        uint8_t success;
        LifeEffect effect;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << success << effect;
        }
        template <class B>
        void parse(B& buf) {
            buf >> success >> effect;
        }
        #endif
    };
    class SkillList
    {
    public:
        std::vector<Buff> buff;
        std::vector<Life> life;
        uint8_t endurance;
        uint32_t sp_to_learn;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << buff << life << endurance << sp_to_learn;
        }
        template <class B>
        void parse(B& buf) {
            buf >> buff >> life >> endurance >> sp_to_learn;
        }
        #endif
    };
    std::vector<Skill::SkillList> level;//first is level 1
    uint8_t type;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << level << type;
    }
    template <class B>
    void parse(B& buf) {
        buf >> level >> type;
    }
    #endif
};

class Monster
{
public:
    class Stat
    {
    public:
        uint32_t hp;
        #ifndef CATCHCHALLENGER_CLASS_MASTER
        uint32_t attack;
        uint32_t defense;
        uint32_t special_attack;
        uint32_t special_defense;
        uint32_t speed;
        #endif
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << hp;
            #ifndef CATCHCHALLENGER_CLASS_MASTER
            buf << attack << defense << special_attack << special_defense << speed;
            #endif
        }
        template <class B>
        void parse(B& buf) {
            buf >> hp;
            #ifndef CATCHCHALLENGER_CLASS_MASTER
            buf >> attack >> defense >> special_attack >> special_defense >> speed;
            #endif
        }
        #endif
    };
    class AttackToLearn
    {
    public:
        uint8_t learnAtLevel;
        uint16_t learnSkill;
        uint8_t learnSkillLevel;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << learnAtLevel << learnSkill << learnSkillLevel;
        }
        template <class B>
        void parse(B& buf) {
            buf >> learnAtLevel >> learnSkill >> learnSkillLevel;
        }
        #endif
    };
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    enum EvolutionType : uint8_t
    {
        EvolutionType_Level,
        EvolutionType_Item,
        EvolutionType_Trade
    };
    class Evolution
    {
    public:
        EvolutionType type;
        union Data {
           int8_t level;
           CATCHCHALLENGER_TYPE_ITEM item;
        } data;
        CATCHCHALLENGER_TYPE_MONSTER evolveTo;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << (uint8_t)type << data.item << evolveTo;
        }
        template <class B>
        void parse(B& buf) {
            uint8_t value=0;
            buf >> value >> data.item >> evolveTo;
            type=(EvolutionType)value;
        }
        #endif
    };
    class AttackToLearnByItem
    {
    public:
        uint16_t learnSkill;
        uint8_t learnSkillLevel;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << learnSkill << learnSkillLevel;
        }
        template <class B>
        void parse(B& buf) {
            buf >> learnSkill >> learnSkillLevel;
        }
        #endif
    };

    std::vector<uint8_t> type;
    uint8_t catch_rate;///< 0 to 255 (255 = very easy)
    uint32_t egg_step;///< step to hatch, 0 to no egg and never hatch
    uint32_t xp_for_max_level;///< xp to be level 100
    uint32_t give_sp;
    uint32_t give_xp;
    std::vector<uint32_t> level_to_xp;//first is xp to level 1
    std::unordered_map<CATCHCHALLENGER_TYPE_ITEM/*item*/,AttackToLearnByItem/*skill*/> learnByItem;
    std::vector<Evolution> evolutions;
    #ifdef CATCHCHALLENGER_CLIENT
    double powerVar;
    #endif
    #endif
    std::vector<AttackToLearn> learn;
    Stat stat;
    int8_t ratio_gender;///< -1 for no gender, 0 only male, 100 only female
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf
            #ifndef CATCHCHALLENGER_CLASS_MASTER
            << type << catch_rate << egg_step << xp_for_max_level << give_sp << give_xp << level_to_xp << learnByItem << evolutions
            #ifdef CATCHCHALLENGER_CLIENT
            << powerVar
            #endif
            #endif
            << learn << stat << ratio_gender
               ;
    }
    template <class B>
    void parse(B& buf) {
        buf
            #ifndef CATCHCHALLENGER_CLASS_MASTER
            >> type >> catch_rate >> egg_step >> xp_for_max_level >> give_sp >> give_xp >> level_to_xp >> learnByItem >> evolutions
            #ifdef CATCHCHALLENGER_CLIENT
            >> powerVar
            #endif
            #endif
            >> learn >> stat >> ratio_gender
               ;
    }
    #endif
};

class Quest
{
public:
    class Item
    {
    public:
        CATCHCHALLENGER_TYPE_ITEM item;
        uint32_t quantity;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << item << quantity;
        }
        template <class B>
        void parse(B& buf) {
            buf >> item >> quantity;
        }
        #endif
    };
    class ItemMonster
    {
    public:
        CATCHCHALLENGER_TYPE_ITEM item;
        std::vector<uint16_t> monsters;
        uint8_t rate;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << item << monsters << rate;
        }
        template <class B>
        void parse(B& buf) {
            buf >> item >> monsters >> rate;
        }
        #endif
    };
    class Requirements
    {
    public:
        std::vector<QuestRequirements> quests;
        std::vector<ReputationRequirements> reputation;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << quests << reputation;
        }
        template <class B>
        void parse(B& buf) {
            buf >> quests >> reputation;
        }
        #endif
    };
    class Rewards
    {
    public:
        std::vector<Item> items;
        std::vector<ReputationRewards> reputation;
        std::vector<ActionAllow> allow;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << items << reputation;
            buf << (uint8_t)allow.size();
            for(auto const& v: allow)
                buf << (uint8_t)v;
        }
        template <class B>
        void parse(B& buf) {
            buf >> items >> reputation;
            uint8_t vectorsize=0;
            buf >> vectorsize;
            for(unsigned int i=0; i<vectorsize; i++) {
                uint8_t value=0;
                buf >> value;
                allow.push_back((ActionAllow)value);
            }
        }
        #endif
    };
    class StepRequirements
    {
    public:
        std::vector<Item> items;
        std::vector<MapFight> fights;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << items << fights;
        }
        template <class B>
        void parse(B& buf) {
            buf >> items >> fights;
        }
        #endif
    };
    class Step
    {
    public:
        std::vector<ItemMonster> itemsMonster;
        StepRequirements requirements;
        std::vector<uint16_t> bots;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << itemsMonster << requirements << bots;
        }
        template <class B>
        void parse(B& buf) {
            buf >> itemsMonster >> requirements >> bots;
        }
        #endif
    };

    uint16_t id;
    bool repeatable;
    Requirements requirements;
    Rewards rewards;
    std::vector<Step> steps;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << id << repeatable << requirements << rewards << steps;
    }
    template <class B>
    void parse(B& buf) {
        buf >> id >> repeatable >> requirements >> rewards >> steps;
    }
    #endif
};

class BotFight
{
public:
    class BotFightMonster
    {
    public:
        uint16_t id;
        uint8_t level;
        std::vector<PlayerMonster::PlayerSkill> attacks;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << id << level << attacks;
        }
        template <class B>
        void parse(B& buf) {
            buf >> id >> level >> attacks;
        }
        #endif
    };
    class Item
    {
    public:
        CATCHCHALLENGER_TYPE_ITEM id;
        uint32_t quantity;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << id << quantity;
        }
        template <class B>
        void parse(B& buf) {
            buf >> id >> quantity;
        }
        #endif
    };
    std::vector<BotFightMonster> monsters;
    uint32_t cash;
    std::vector<Item> items;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << monsters << cash << items;
    }
    template <class B>
    void parse(B& buf) {
        buf >> monsters >> cash >> items;
    }
    #endif
};

class Profile
{
public:
    class Reputation
    {
    public:
        //struct Reputation have int reverse_database_id; uint8_t reputationDatabaseId;//datapack order, can can need the dicionary to db resolv
        uint8_t internalIndex;
        int8_t level;
        int32_t point;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << internalIndex << level << point;
        }
        template <class B>
        void parse(B& buf) {
            buf >> internalIndex >> level >> point;
        }
        #endif
    };
    class Monster
    {
    public:
        uint16_t id;
        uint8_t level;
        uint16_t captured_with;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << id << level << captured_with;
        }
        template <class B>
        void parse(B& buf) {
            buf >> id >> level >> captured_with;
        }
        #endif
    };
    class Item
    {
    public:
        CATCHCHALLENGER_TYPE_ITEM id;
        uint32_t quantity;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << id << quantity;
        }
        template <class B>
        void parse(B& buf) {
            buf >> id >> quantity;
        }
        #endif
    };
    std::vector<uint8_t> forcedskin;
    uint64_t cash;
    std::vector<std::vector<Profile::Monster> > monstergroup;
    std::vector<Reputation> reputations;
    std::vector<Item> items;
    std::string databaseId;/*to resolve with the dictionary, in string (not the number), need port prepare profile*/
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << forcedskin << cash << monstergroup << reputations << items << databaseId;
    }
    template <class B>
    void parse(B& buf) {
        buf >> forcedskin >> cash >> monstergroup >> reputations >> items >> databaseId;
    }
    #endif
};

class ServerSpecProfile
{
public:
    void * mapPointer;
    /*COORD_TYPE*/ uint8_t x;
    /*COORD_TYPE*/ uint8_t y;
    Orientation orientation;

    /// \todo support multiple map start, change both:
    std::string mapString;
    std::string databaseId;/*to resolve with the dictionary, in string (not the number), need port prepare profile*/
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << x << y << (uint8_t)orientation << mapString << databaseId;
    }
    template <class B>
    void parse(B& buf) {
        uint8_t value=0;
        buf >> x >> y >> value >> mapString >> databaseId;
        orientation=(Orientation)value;
        mapPointer=nullptr;
    }
    #endif
};

}
#endif // STRUCTURES_GENERAL_H
