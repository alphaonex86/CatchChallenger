#ifndef CATCHCHALLENGER_GENERAL_STRUCTURES_H
#define CATCHCHALLENGER_GENERAL_STRUCTURES_H

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "cpp11addition.h"

#include "GeneralType.h"
#if defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER)
#include "../../server/VariableServer.h"
#endif

#if defined(CATCHCHALLENGER_CLIENT)
//client
#define MAXIMIZEPERFORMANCEOVERDATABASESIZE
#else
//server
//#define MAXIMIZEPERFORMANCEOVERDATABASESIZE
#endif

#ifndef MAXIMIZEPERFORMANCEOVERDATABASESIZE
#include <map>
#include <set>
#endif

#define COORD_TYPE uint8_t
#define SIMPLIFIED_PLAYER_ID_TYPE uint16_t
#define CLAN_ID_TYPE uint32_t
#define SPEED_TYPE uint8_t

namespace CatchChallenger {

enum Chat_type : uint8_t
{
    Chat_type_local = 0x01,
    Chat_type_all = 0x02,
    Chat_type_clan = 0x03,
    Chat_type_aliance = 0x04,
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

enum CompressionType : uint8_t
{
    CompressionType_None = 0x00,
    CompressionType_Zstandard = 0x04
};

enum ActionAllow : uint8_t
{
    ActionAllow_Nothing=0x00,
    ActionAllow_Clan=0x01
};

/*According 12 pictures in 1, png: 48x96, tiled: 16x24*/
enum DrawTiledPosition
{
    LeftFoot_Top     = 0,
    Stop_At_Top      = 1,
    RightFoot_Top    = 2,
    LeftFoot_Right   = 3,
    Stop_At_Right    = 4,
    RightFoot_Right  = 5,
    LeftFoot_Bottom  = 6,
    Stop_At_Bottom   = 7,
    RightFoot_Bottom = 8,
    LeftFoot_Left    = 9,
    Stop_At_Left     = 10,
    RightFoot_Left   = 11,
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
    Plant_collect_empty_dirt=0x02,
    Plant_collect_owned_by_another_player=0x03,
    Plant_collect_impossible=0x04
};

struct Reputation
{
    std::vector<int32_t> reputation_positive/*start with 0*/,reputation_negative;
    /*server only*/
    int reverse_database_id;
    std::string name;
};

struct ReputationRewards
{
    uint8_t reputationId;
    int32_t point;
};

struct ReputationRequirements
{
    uint8_t reputationId;
    uint8_t level;
    bool positif;
};

struct QuestRequirements
{
    uint16_t quest;
    bool inverse;
};

struct Plant
{
    uint16_t itemUsed;
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
};

struct Item
{
    uint32_t price;//if 0 you can't sell or buy it
    bool consumeAtUse;
};

struct Trap
{
    float bonus_rate;
};

struct MonsterItemEffect
{
    MonsterItemEffectType type;
    union Data {
       uint32_t hp;
       uint8_t buff;
    } data;
};

struct MonsterItemEffectOutOfFight
{
    MonsterItemEffectTypeOutOfFight type;
    union Data {
       uint8_t level;
    } data;
};

struct ItemFull
{
    std::unordered_map<uint16_t, std::vector<MonsterItemEffect> > monsterItemEffect;
    std::unordered_map<uint16_t, std::vector<MonsterItemEffectOutOfFight> > monsterItemEffectOutOfFight;
    std::unordered_map<CATCHCHALLENGER_TYPE_ITEM/*item*/, std::unordered_map<uint16_t/*monster*/,uint16_t/*evolveTo*/> > evolutionItem;
    std::unordered_map<CATCHCHALLENGER_TYPE_ITEM/*item*/, std::unordered_set<uint16_t/*monster*/> > itemToLearn;
    std::unordered_map<uint16_t, uint32_t> repel;
    std::unordered_map<CATCHCHALLENGER_TYPE_ITEM, Item> item;
    CATCHCHALLENGER_TYPE_ITEM itemMaxId;
    std::unordered_map<uint16_t, Trap> trap;
};

struct LayersOptions
{
    uint8_t zoom;
};

struct IndustryLink
{
    uint16_t industry;
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
};

struct Industry
{
    uint64_t time;//should not be too short
    uint32_t cycletobefull;
    struct Resource
    {
        CATCHCHALLENGER_TYPE_ITEM item;
        uint32_t quantity;
    };
    struct Product
    {
        CATCHCHALLENGER_TYPE_ITEM item;
        uint32_t quantity;
    };
    std::vector<Resource> resources;
    std::vector<Product> products;
};

struct Event
{
    std::string name;
    std::vector<std::string > values;
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
    uint16_t monsterId;
    SPEED_TYPE speed;
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
};

class PublicPlayerMonster
{
    public:
    uint16_t monster;
    uint8_t level;
    uint32_t hp;
    uint16_t catched_with;
    Gender gender;
    std::vector<PlayerBuff> buffs;
};

class PlayerMonster : public PublicPlayerMonster
{
    public:
    struct PlayerSkill
    {
        uint16_t skill;
        uint8_t level;//start at 1
        uint8_t endurance;
    };
    uint32_t remaining_xp;
    uint32_t sp;
    uint32_t egg_step;
    //in form of list to get random into the list
    std::vector<PlayerSkill> skills;
    #ifndef CATCHCHALLENGER_VERSION_SINGLESERVER
    uint32_t id;//id into the db, only on server part, but no way to leave from here with risk of structure problem
    #endif
    uint32_t character_origin;
};

bool operator!=(const PlayerMonster &monster1,const PlayerMonster &monster2);

struct PlayerQuest
{
    uint8_t step;
    bool finish_one_time;
};

struct PlayerReputation
{
    int8_t level;
    int32_t point;
};

struct PlayerPlant
{
    uint8_t plant;//plant id
    uint64_t mature_at;//timestamp when is mature
};

struct Player_private_and_public_informations
{
    Player_public_informations public_informations;
    uint64_t cash,warehouse_cash;
    //crafting
    char * recipes;
    /// \todo put out of here to have mutalised engine
    std::vector<PlayerMonster> playerMonster,warehouse_playerMonster;
    CLAN_ID_TYPE clan;
    char * encyclopedia_monster;
    char * encyclopedia_item;//should be: CommonDatapack::commonDatapack.items.item.size()/8+1
    uint32_t repel_step;
    bool clan_leader;
    char * bot_already_beaten;//should be: CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId/8+1

    /* item and plant is keep under database format to keep the dataintegrity and save quickly the data
     * More memory usage by 2x, but improve the code maintenance because the id in memory is id in database
     * Less dictionary resolution to improve the cache flush */

    #ifdef MAXIMIZEPERFORMANCEOVERDATABASESIZE
        std::unordered_set<ActionAllow,std::hash<uint8_t>/*what hash use*/ > allow;
        //here to send at character login
        std::unordered_set<uint16_t> itemOnMap;
        #if defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER)
            #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
            std::unordered_map<uint16_t/*pointOnMap:indexOfDirtOnMap*/,PlayerPlant> plantOnMap;
            #endif
        #else
            std::unordered_map<uint16_t/*pointOnMap:indexOfDirtOnMap*/,PlayerPlant> plantOnMap;
        #endif
        std::unordered_map<uint16_t, PlayerQuest> quests;
        std::unordered_map<uint8_t,PlayerReputation> reputation;
        std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint32_t/*quantity*/> items,warehouse_items;
    #else
        std::set<ActionAllow> allow;
        //here to send at character login
        std::set<uint16_t> itemOnMap;
        #if defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER)
            #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
            std::map<uint16_t/*pointOnMap:indexOfDirtOnMap*/,PlayerPlant> plantOnMap;
            #endif
        #else
            std::map<uint16_t/*pointOnMap:indexOfDirtOnMap*/,PlayerPlant> plantOnMap;
        #endif
        std::map<uint16_t, PlayerQuest> quests;
        std::map<uint8_t/*internal id*/,PlayerReputation> reputation;
        std::map<CATCHCHALLENGER_TYPE_ITEM,uint32_t/*quantity*/> items,warehouse_items;
    #endif
};

struct CharacterEntry
{
    uint32_t character_id;
    std::string pseudo;
    uint8_t charactersGroupIndex;
    uint8_t skinId;
    uint32_t delete_time_left;
    uint32_t played_time;
    uint64_t last_connect;
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

struct MapCondition
{
    MapConditionType type;
    union Data {
       uint16_t quest;
       uint16_t item;
       uint16_t fightBot;
    } data;
};

struct MapMonster
{
    uint16_t id;
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
    bool *walkable;//walkable if !=false mean !=0
    uint8_t *monstersCollisionMap;
    std::vector<MonstersCollisionValue> monstersCollisionList;
    //bool *grass;
    bool *dirt;
    //not stored as ParsedLayerLedges to prevent memory space unused
    uint8_t *ledges;
};



struct CrafingRecipe
{
    CATCHCHALLENGER_TYPE_ITEM itemToLearn;
    CATCHCHALLENGER_TYPE_ITEM doItemId;
    uint16_t quantity;
    uint8_t success;//0-100
    struct Material
    {
        CATCHCHALLENGER_TYPE_ITEM item;
        uint32_t quantity;
    };
    std::vector<Material> materials;
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
};

struct Shop
{
    std::vector<uint32_t> prices;
    std::vector<CATCHCHALLENGER_TYPE_ITEM> items;
};

enum QuantityType : uint8_t
{
    QuantityType_Quantity,
    QuantityType_Percent
};

struct Buff
{
    enum Duration : uint8_t
    {
        Duration_Always,
        Duration_ThisFight,
        Duration_NumberOfTurn
    };
    struct Effect
    {
        enum EffectOn : uint8_t
        {
            EffectOn_HP,
            EffectOn_Defense,
            EffectOn_Attack
        };
        EffectOn on;
        int32_t quantity;
        QuantityType type;
    };
    struct EffectInWalk
    {
        Effect effect;
        uint32_t steps;
    };
    struct GeneralEffect
    {
        std::vector<EffectInWalk> walk;
        std::vector<Effect> fight;
        float capture_bonus;
        Duration duration;
        uint8_t durationNumberOfTurn;
    };
    std::vector<GeneralEffect> level;//first entry is buff level 1
};

enum ApplyOn : uint8_t
{
    ApplyOn_AloneEnemy=0x01,
    ApplyOn_AllEnemy=0x02,
    ApplyOn_Themself=0x04,
    ApplyOn_AllAlly=0x08,
    ApplyOn_Nobody=0x00
};

struct MonsterDrops
{
    uint16_t item;
    uint32_t quantity_min;
    uint32_t quantity_max;
    uint8_t luck;//seam be 0 to 100
};

struct Skill
{
    enum AttackReturnCase : uint8_t
    {
        AttackReturnCase_NormalAttack=0x01,
        AttackReturnCase_MonsterChange=0x02,
        AttackReturnCase_ItemUsage=0x03
    };
    struct BuffEffect
    {
        uint8_t buff;
        ApplyOn on;
        uint8_t level;
    };
    struct LifeEffect
    {
        int32_t quantity;
        QuantityType type;
        ApplyOn on;
    };
    struct LifeEffectReturn
    {
        int32_t quantity;
        ApplyOn on;
        bool critical;
        float effective;
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
    struct Buff
    {
        uint8_t success;
        BuffEffect effect;
    };
    struct Life
    {
        uint8_t success;
        LifeEffect effect;
    };
    struct SkillList
    {
        std::vector<Buff> buff;
        std::vector<Life> life;
        uint8_t endurance;
        uint32_t sp_to_learn;
    };
    std::vector<Skill::SkillList> level;//first is level 1
    uint8_t type;
};

enum Place : uint8_t
{
    OnPlayer=0,
    WareHouse=1,
    Market=2
};

struct Monster
{
    int8_t ratio_gender;///< -1 for no gender, 0 only male, 100 only female
    struct Stat
    {
        uint32_t hp;
        #ifndef CATCHCHALLENGER_CLASS_MASTER
        uint32_t attack;
        uint32_t defense;
        uint32_t special_attack;
        uint32_t special_defense;
        uint32_t speed;
        #endif
    };
    Stat stat;
    struct AttackToLearn
    {
        uint8_t learnAtLevel;
        uint16_t learnSkill;
        uint8_t learnSkillLevel;
    };
    std::vector<AttackToLearn> learn;

    #ifndef CATCHCHALLENGER_CLASS_MASTER
    enum EvolutionType : uint8_t
    {
        EvolutionType_Level,
        EvolutionType_Item,
        EvolutionType_Trade
    };
    struct Evolution
    {
        EvolutionType type;
        union Data {
           int8_t level;
           uint16_t item;
        } data;
        uint16_t evolveTo;
    };

    std::vector<uint8_t> type;
    uint8_t catch_rate;///< 0 to 255 (255 = very easy)
    uint32_t egg_step;///< step to hatch, 0 to no egg and never hatch
    uint32_t xp_for_max_level;///< xp to be level 100
    uint32_t give_sp;
    uint32_t give_xp;
    std::vector<uint32_t> level_to_xp;//first is xp to level 1
    #ifdef CATCHCHALLENGER_CLIENT
    double powerVar;
    #endif

    struct AttackToLearnByItem
    {
        uint16_t learnSkill;
        uint8_t learnSkillLevel;
    };
    std::unordered_map<CATCHCHALLENGER_TYPE_ITEM/*item*/,AttackToLearnByItem/*skill*/> learnByItem;
    std::vector<Evolution> evolutions;
    #endif
};

struct ItemToSellOrBuy
{
    CATCHCHALLENGER_TYPE_ITEM object;
    uint32_t price;
    uint32_t quantity;
};

struct Quest
{
    struct Item
    {
        CATCHCHALLENGER_TYPE_ITEM item;
        uint32_t quantity;
    };
    struct ItemMonster
    {
        CATCHCHALLENGER_TYPE_ITEM item;
        std::vector<uint16_t> monsters;
        uint8_t rate;
    };
    struct Requirements
    {
        std::vector<QuestRequirements> quests;
        std::vector<ReputationRequirements> reputation;
    };
    struct Rewards
    {
        std::vector<Item> items;
        std::vector<ReputationRewards> reputation;
        std::vector<ActionAllow> allow;
    };
    struct StepRequirements
    {
        std::vector<Item> items;
        std::vector<uint16_t> fightId;
    };
    struct Step
    {
        std::vector<ItemMonster> itemsMonster;
        StepRequirements requirements;
        std::vector<uint16_t> bots;
    };

    uint16_t id;
    bool repeatable;
    Requirements requirements;
    Rewards rewards;
    std::vector<Step> steps;
};

struct City
{
    struct Capture
    {
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
    };
    Capture capture;
};

struct BotFight
{
    struct BotFightMonster
    {
        uint16_t id;
        uint8_t level;
        std::vector<PlayerMonster::PlayerSkill> attacks;
    };
    std::vector<BotFightMonster> monsters;
    uint32_t cash;
    struct Item
    {
        CATCHCHALLENGER_TYPE_ITEM id;
        uint32_t quantity;
    };
    std::vector<Item> items;
};

struct IndustryStatus
{
    uint64_t last_update;
    std::unordered_map<uint32_t,uint32_t> resources;
    std::unordered_map<uint32_t,uint32_t> products;
};

class Profile
{
public:
    struct Reputation
    {
        //struct Reputation have int reverse_database_id; uint8_t reputationDatabaseId;//datapack order, can can need the dicionary to db resolv
        uint8_t internalIndex;
        int8_t level;
        int32_t point;
    };
    struct Monster
    {
        uint16_t id;
        uint8_t level;
        uint16_t captured_with;
    };
    struct Item
    {
        CATCHCHALLENGER_TYPE_ITEM id;
        uint32_t quantity;
    };
    std::vector<uint8_t> forcedskin;
    uint64_t cash;
    std::vector<std::vector<Profile::Monster> > monstergroup;
    std::vector<Reputation> reputations;
    std::vector<Item> items;
    std::string databaseId;/*to resolve with the dictionary, in string (not the number), need port prepare profile*/
};

struct ServerSpecProfile
{
    void * mapPointer;
    /*COORD_TYPE*/ uint8_t x;
    /*COORD_TYPE*/ uint8_t y;
    Orientation orientation;

    /// \todo support multiple map start, change both:
    std::string mapString;
    std::string databaseId;/*to resolve with the dictionary, in string (not the number), need port prepare profile*/
};

enum MonstersCollisionType : uint8_t
{
    MonstersCollisionType_WalkOn=0x00,
    MonstersCollisionType_ActionOn=0x01
};

struct MonstersCollision
{
    MonstersCollisionType type;
    CATCHCHALLENGER_TYPE_ITEM item;
    std::string tile;
    std::string layer;
    std::vector<std::string> monsterTypeList;
    std::vector<std::string> defautMonsterTypeList;
    std::string background;
    struct MonstersCollisionEvent
    {
        uint8_t event;
        uint8_t event_value;
        std::vector<std::string > monsterTypeList;
    };
    std::vector<MonstersCollisionEvent> events;
};

struct Type
{
    std::string name;
    std::unordered_map<uint8_t,int8_t> multiplicator;//negative = divide, not multiply
};

}
#endif // STRUCTURES_GENERAL_H
