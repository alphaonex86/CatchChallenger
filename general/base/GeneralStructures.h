#ifndef CATCHCHALLENGER_GENERAL_STRUCTURES_H
#define CATCHCHALLENGER_GENERAL_STRUCTURES_H

#include <QObject>
#include <QList>
#include <QByteArray>
#include <QStringList>
#include <QString>
#include <QHash>
#include <QSet>
#include <QVariant>
#include <QDomElement>
#include <QMultiHash>

#define COORD_TYPE quint8
#define SIMPLIFIED_PLAYER_ID_TYPE quint16
#define CLAN_ID_TYPE quint32
#define SPEED_TYPE quint8

namespace CatchChallenger {

enum Chat_type
{
    Chat_type_local = 0x01,
    Chat_type_all = 0x02,
    Chat_type_clan = 0x03,
    Chat_type_aliance = 0x04,
    Chat_type_pm = 0x06,
    Chat_type_system = 0x07,
    Chat_type_system_important = 0x08
};

enum Player_type
{
    Player_type_normal = 0x10,
    Player_type_premium = 0x20,
    Player_type_gm = 0x30,
    Player_type_dev = 0x40
};

enum Orientation
{
    Orientation_top = 1,
    Orientation_right = 2,
    Orientation_bottom = 3,
    Orientation_left = 4
};

enum CompressionType
{
    CompressionType_None = 0x00,
    CompressionType_Zlib = 0x01,
    CompressionType_Xz = 0x02
};

enum ActionAllow
{
    ActionAllow_Clan
};

enum Direction
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

enum MonsterItemEffectType
{
    MonsterItemEffectType_AddHp = 0x01,
    MonsterItemEffectType_RemoveBuff = 0x02
};

enum RecipeUsage
{
    RecipeUsage_ok=0x01,
    RecipeUsage_impossible=0x02,//crafting materials not used
    RecipeUsage_failed=0x03//crafting materials used
};

enum Plant_collect
{
    Plant_collect_correctly_collected=0x01,
    Plant_collect_empty_dirt=0x02,
    Plant_collect_owned_by_another_player=0x03,
    Plant_collect_impossible=0x04
};

struct Plant
{
    quint32 itemUsed;
    quint32 fruits_seconds;
    //float quantity;//splited into 2 integer
    quint16 fix_quantity;
    quint16 random_quantity;
    //minimal memory impact at exchange of drop dual xml parse
    quint16 sprouted_seconds;
    quint16 taller_seconds;
    quint16 flowering_seconds;
};

struct Item
{
    quint32 price;
    bool consumeAtUse;
};

struct Trap
{
    float bonus_rate;
};

struct MonsterItemEffect
{
    MonsterItemEffectType type;
    qint32 value;
};

struct ItemFull
{
    QMultiHash<quint32, MonsterItemEffect> monsterItemEffect;
    QHash<quint32, QHash<quint32,quint32> > evolutionItem;
    QHash<quint32, quint32> repel;
    QHash<quint32, Item> item;
    QHash<quint32, Trap> trap;
};

struct Industry
{
    quint32 time;//should not be too short
    quint32 cycletobefull;
    struct Resource
    {
        quint32 item;
        quint32 quantity;
    };
    struct Product
    {
        quint32 item;
        quint32 quantity;
    };
    QList<Resource> resources;
    QList<Product> products;
};

enum ObjectUsage
{
    ObjectUsage_correctlyUsed=0x01,//is correctly used
    ObjectUsage_failed=0x02,//failed to use with consumption of the object
    ObjectUsage_impossible=0x03//failed to use without consumption of the object
};

enum BuyStat
{
    BuyStat_Done=0x01,
    BuyStat_BetterPrice=0x02,
    BuyStat_HaveNotQuantity=0x03,
    BuyStat_PriceHaveChanged=0x04
};
enum SoldStat
{
    SoldStat_Done=0x01,
    SoldStat_BetterPrice=0x02,
    SoldStat_WrongQuantity=0x03,
    SoldStat_PriceHaveChanged=0x04
};

struct map_management_insert
{
    QString fileName;
    COORD_TYPE x;
    COORD_TYPE y;
    Direction direction;//can be insert as direction when changing of map
    quint16 speed;
};

struct map_management_movement
{
    quint8 movedUnit;
    Direction direction;
};

struct map_management_move
{
    quint32 id;
    QList<map_management_movement> movement_list;
};

struct Player_public_informations
{
    SIMPLIFIED_PLAYER_ID_TYPE simplifiedId;
    QString pseudo;
    Player_type type;
    quint8 skinId;
    SPEED_TYPE speed;
};

enum Gender
{
    Gender_Male=0x01,
    Gender_Female=0x02,
    Gender_Unknown=0x03
};
struct PlayerBuff
{
    quint32 buff;
    quint8 level;
    quint8 remainingNumberOfTurn;
};

class PublicPlayerMonster
{
    public:
    quint32 monster;
    quint8 level;
    quint32 hp;
    quint32 captured_with;
    Gender gender;
    QList<PlayerBuff> buffs;
};

class PlayerMonster : public PublicPlayerMonster
{
    public:
    struct PlayerSkill
    {
        quint32 skill;
        quint8 level;
        quint8 endurance;
    };
    quint32 remaining_xp;
    quint32 sp;
    quint32 egg_step;
    //in form of list to get random into the list
    QList<PlayerSkill> skills;
    quint32 id;//id into the db
};

struct PlayerQuest
{
    quint8 step;
    bool finish_one_time;
};

struct PlayerReputation
{
    qint8 level;
    qint32 point;
};

struct Reputation
{
    QList<qint32> reputation_positive,reputation_negative;
    QStringList text_positive,text_negative;
};

struct Player_private_and_public_informations
{
    Player_public_informations public_informations;
    quint64 cash,warehouse_cash;
    double bitcoin;
    QString bitcoinAddress;
    QHash<quint32,quint32> items,warehouse_items;
    //crafting
    QSet<quint32> recipes;
    QHash<QString,PlayerReputation> reputation;
    //fight
    QSet<quint32> bot_already_beaten;
    /// \todo put out of here to have mutalised engine
    QList<PlayerMonster> playerMonster,warehouse_playerMonster;
    QHash<quint32, PlayerQuest> quests;
    CLAN_ID_TYPE clan;
    bool clan_leader;
    QSet<ActionAllow> allow;
    quint32 repel_step;
};

/// \brief Define the mode of transmission: client or server
enum PacketModeTransmission
{
    PacketModeTransmission_Server=0x00,
    PacketModeTransmission_Client=0x01
};

struct CharacterEntry
{
    quint32 character_id;
    QString pseudo;
    QString skin;
    quint32 delete_time_left;
    quint32 played_time;
    quint32 last_connect;
    QString map;
};

/* mpa related */
struct Map_semi_border_content_top_bottom
{
    QString fileName;
    qint16 x_offset;//can be negative, it's an offset!
};

struct Map_semi_border_content_left_right
{
    QString fileName;
    qint16 y_offset;//can be negative, it's an offset!
};

struct Map_semi_border
{
    Map_semi_border_content_top_bottom top;
    Map_semi_border_content_top_bottom bottom;
    Map_semi_border_content_left_right left;
    Map_semi_border_content_left_right right;
};

struct Map_semi_teleport
{
    COORD_TYPE source_x,source_y;
    COORD_TYPE destination_x,destination_y;
    QString map;
};

struct MapMonster
{
    quint32 id;
    quint8 minLevel;
    quint8 maxLevel;
    quint8 luck;
};

enum ParsedLayerLedges
{
    ParsedLayerLedges_NoLedges=0x00,
    ParsedLayerLedges_LedgesLeft=0x01,
    ParsedLayerLedges_LedgesRight=0x02,
    ParsedLayerLedges_LedgesTop=0x03,
    ParsedLayerLedges_LedgesBottom=0x04
};

struct ParsedLayer
{
    bool *walkable;
    bool *water;
    bool *grass;
    bool *dirt;
    //not stored as ParsedLayerLedges to prevent memory space unused
    quint8 *ledges;
};

struct Map_to_send
{
    Map_semi_border border;
    //QStringList other_map;//border and not

    //quint32 because the format allow it, checked into tryLoadMap()
    quint32 width;
    quint32 height;

    QHash<QString,QVariant> property;

    ParsedLayer parsed_layer;

    QList<Map_semi_teleport> teleport;

    struct Map_Point
    {
        COORD_TYPE x,y;
    };
    QList<Map_Point> rescue_points;
    QList<Map_Point> bot_spawn_points;

    struct Bot_Semi
    {
        Map_Point point;
        QString file;
        quint32 id;
        QHash<QString,QVariant> property_text;
    };
    QList<Bot_Semi> bots;

    QList<MapMonster> grassMonster;
    QList<MapMonster> waterMonster;
    QList<MapMonster> caveMonster;

    QDomElement xmlRoot;
};

struct CrafingRecipe
{
    quint32 itemToLearn;
    quint32 doItemId;
    quint16 quantity;
    quint8 success;//0-100
    struct Material
    {
        quint32 item;
        quint32 quantity;
    };
    QList<Material> materials;
};

struct Shop
{
    QList<quint32> items;
};

enum QuantityType
{
    QuantityType_Quantity,
    QuantityType_Percent
};

struct Buff
{
    enum Duration
    {
        Duration_Always,
        Duration_ThisFight,
        Duration_NumberOfTurn
    };
    struct Effect
    {
        enum EffectOn
        {
            EffectOn_HP,
            EffectOn_Defense
        };
        EffectOn on;
        qint32 quantity;
        QuantityType type;
    };
    struct EffectInWalk
    {
        Effect effect;
        quint32 steps;
    };
    struct GeneralEffect
    {
        QList<EffectInWalk> walk;
        QList<Effect> fight;
        float capture_bonus;
        Duration duration;
        quint8 durationNumberOfTurn;
    };
    QList<GeneralEffect> level;//first entry is buff level 1
};

enum ApplyOn
{
    ApplyOn_AloneEnemy=0x01,
    ApplyOn_AllEnemy=0x02,
    ApplyOn_Themself=0x03,
    ApplyOn_AllAlly=0x04,
    ApplyOn_Nobody=0x05
};

struct Skill
{
    struct BuffEffect
    {
        quint32 buff;
        ApplyOn on;
        quint8 level;
    };
    struct LifeEffect
    {
        qint32 quantity;
        QuantityType type;
        ApplyOn on;
    };
    struct LifeEffectReturn
    {
        qint32 quantity;
        ApplyOn on;
        bool critical;
        float effective;
    };
    struct AttackReturn
    {
        bool doByTheCurrentMonster;
        bool success;
        quint32 attack;
        QList<BuffEffect> addBuffEffectMonster,removeBuffEffectMonster;
        QList<LifeEffectReturn> lifeEffectMonster;
        QList<LifeEffectReturn> buffLifeEffectMonster;
    };
    struct Buff
    {
        quint8 success;
        BuffEffect effect;
    };
    struct Life
    {
        quint8 success;
        LifeEffect effect;
    };
    struct SkillList
    {
        QList<Buff> buff;
        QList<Life> life;
        quint8 endurance;
        quint32 sp_to_learn;
    };
    QList<Skill::SkillList> level;//first is level 1
    quint8 type;
};

struct Monster
{
    enum EvolutionType
    {
        EvolutionType_Level,
        EvolutionType_Item,
        EvolutionType_Trade
    };
    struct Evolution
    {
        EvolutionType type;
        qint32 level;
        quint32 evolveTo;
    };

    QList<quint8> type;
    qint8 ratio_gender;///< -1 for no gender, 0 only male, 100 only female
    quint8 catch_rate;///< 0 to 255 (255 = very easy)
    quint32 egg_step;///< step to hatch, 0 to no egg and never hatch
    quint32 xp_for_max_level;///< xp to be level 100
    quint32 give_sp;
    quint32 give_xp;
    struct Stat
    {
        quint32 hp;
        quint32 attack;
        quint32 defense;
        quint32 special_attack;
        quint32 special_defense;
        quint32 speed;
    };
    Stat stat;
    QList<quint32> level_to_xp;//first is xp to level 1

    struct AttackToLearn
    {
        quint8 learnAtLevel;
        quint32 learnSkill;
        quint8 learnSkillLevel;
    };
    QList<AttackToLearn> learn;
    QList<Evolution> evolutions;
};

struct ItemToSellOrBuy
{
    quint32 object;
    quint32 price;
    quint32 quantity;
};

struct Quest
{
    struct Item
    {
        quint32 item;
        quint32 quantity;
    };
    struct ItemMonster
    {
        quint32 item;
        QList<quint32> monsters;
        quint8 rate;
    };
    struct ReputationRewards
    {
        QString type;
        qint32 point;
    };
    struct ReputationRequirements
    {
        QString type;
        quint8 level;
        bool positif;
    };
    struct Requirements
    {
        QList<quint32> quests;
        QList<ReputationRequirements> reputation;
    };
    struct Rewards
    {
        QList<Item> items;
        QList<ReputationRewards> reputation;
        QList<ActionAllow> allow;
    };
    struct StepRequirements
    {
        QList<Item> items;
        QList<quint32> fightId;
    };
    struct Step
    {
        QList<ItemMonster> itemsMonster;
        StepRequirements requirements;
        QList<quint32> bots;
    };

    quint32 id;
    bool repeatable;
    Requirements requirements;
    Rewards rewards;
    QList<Step> steps;
};

struct City
{
    struct Capture
    {
        enum Frequency
        {
            Frequency_week=0x01,
            Frequency_month=0x02
        };
        Frequency frenquency;
        enum Day
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
        quint8 hour,minute;
    };
    Capture capture;
};

//permanent bot on client, temp to parse on the server
struct Bot
{
    QHash<quint8,QDomElement> step;
    QHash<QString,QString> properties;
    quint32 botId;//id need be unique for the quests, then 32Bits
    QString skin;
};

struct BotFight
{
    struct BotFightMonster
    {
        quint32 id;
        quint8 level;
        QList<PlayerMonster::PlayerSkill> attacks;
    };
    QList<BotFightMonster> monsters;
    quint32 cash;
};

struct MarketObject
{
    quint32 marketObjectId;
    quint32 objectId;
    quint32 quantity;
    quint32 price;
    double bitcoin;
};
struct MarketMonster
{
    quint32 monsterId;
    quint32 monster;
    quint8 level;
    quint32 price;
    double bitcoin;
};

struct IndustryStatus
{
    quint32 last_update;
    QHash<quint32,quint32> resources;
    QHash<quint32,quint32> products;
};

struct Profile
{
    struct Reputation
    {
        QString type;
        qint8 level;
        qint32 point;
    };
    struct Monster
    {
        quint32 id;
        qint8 level;
        qint32 captured_with;
    };
    struct Item
    {
        quint32 id;
        quint32 quantity;
    };
    QString map;
    quint8 x,y;
    QStringList forcedskin;
    quint64 cash;
    QList<Monster> monsters;
    QList<Reputation> reputation;
    QList<Item> items;
};

struct Type
{
    QString name;
    QHash<quint8,qint8> multiplicator;//negative = divide, not multiply
};

}
#endif // STRUCTURES_GENERAL_H
