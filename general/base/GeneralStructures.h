#ifndef CATCHCHALLENGER_GENERAL_STRUCTURES_H
#define CATCHCHALLENGER_GENERAL_STRUCTURES_H

#include <QObject>
#include <QList>
#include <QByteArray>
#include <QStringList>
#include <QString>
#include <QHash>
#include <QVariant>
#include <QDomElement>

#define COORD_TYPE quint8
#define SIMPLIFIED_PLAYER_ID_TYPE quint16
#define CLAN_ID_TYPE quint8
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
    CLAN_ID_TYPE clan;
    Player_type type;
    quint8 skinId;
    SPEED_TYPE speed;
};

struct PlayerMonster
{
    enum Gender
    {
        Male=0x01,
        Female=0x02,
        Unknown=0x03
    };
    struct PlayerBuff
    {
        quint32 buff;
        quint8 level;
    };
    struct PlayerSkill
    {
        quint32 skill;
        quint8 level;
    };
    quint32 monster;
    quint8 level;
    quint32 remaining_xp;
    quint32 hp;
    quint32 sp;
    quint32 captured_with;
    Gender gender;
    quint32 egg_step;
    //in form of list to get random into the list
    QList<PlayerBuff> buffs;
    QList<PlayerSkill> skills;
    quint32 id;
};

struct PublicPlayerMonster
{
    quint32 monster;
    quint8 level;
    quint32 hp;
    quint32 captured_with;
    PlayerMonster::Gender gender;
    quint32 egg_step;
    QList<PlayerMonster::PlayerBuff> buffs;
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
    quint64 cash;
    QHash<quint32,quint32> items;
    //crafting
    QList<quint32> recipes;
    QHash<QString,PlayerReputation> reputation;
    //fight
    /// \todo put out of here to have mutalised engine
    QList<PlayerMonster> playerMonster;
    QHash<quint32, PlayerQuest> quests;
};

/// \brief Define the mode of transmission: client or server
enum PacketModeTransmission
{
    PacketModeTransmission_Server=0x00,
    PacketModeTransmission_Client=0x01
};

/** \brief settings of the server shared with the client
 * This settings is send if remote client
 * If benchmark/local client -> single player, the settigns is send to keep the structure, but few useless, no performance impact */
struct CommmonServerSettings
{
    //fight
    bool pvp;
    bool sendPlayerNumber;

    //rates
    qreal rates_xp;
    qreal rates_gold;
    qreal rates_shiny;

    //chat allowed
    bool chat_allow_all;
    bool chat_allow_local;
    bool chat_allow_private;
    bool chat_allow_aliance;
    bool chat_allow_clan;
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
    };
    QList<Bot_Semi> bots;

    QList<MapMonster> grassMonster;
    QList<MapMonster> waterMonster;
    QList<MapMonster> caveMonster;
};

struct CrafingRecipe
{
    quint32 itemToLearn;
    quint32 doItemId;
    quint16 quantity;
    quint8 success;//0-100
    struct Material
    {
        quint32 itemId;
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
    };
    QList<GeneralEffect> level;//first entry is buff level 1
};

enum ApplyOn
{
    ApplyOn_AloneEnemy,
    ApplyOn_AllEnemy,
    ApplyOn_Themself,
    ApplyOn_AllAlly
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
    };
    struct AttackReturn
    {
        bool doByTheCurrentMonster;
        bool success;
        QList<BuffEffect> buffEffectMonster;
        QList<LifeEffectReturn> lifeEffectMonster;
        quint32 attack;
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
        quint32 sp;
    };
    QList<Skill::SkillList> level;//first is level 1
};

struct Monster
{
    QString type,type2;
    qint8 ratio_gender;///< -1 for no gender, 0 only male, 100 only female
    quint8 catch_rate;///< 0 to 100
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
    };
    struct StepRequirements
    {
        QList<Item> items;
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

//permanent bot on client, temp to parse on the server
struct Bot
{
    QHash<quint8,QDomElement> step;
    QHash<QString,QString> properties;
    quint32 botId;
};

}
#endif // STRUCTURES_GENERAL_H
