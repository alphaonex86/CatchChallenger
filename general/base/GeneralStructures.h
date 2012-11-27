#ifndef POKECRAFT_GENERAL_STRUCTURES_H
#define POKECRAFT_GENERAL_STRUCTURES_H

#include <QObject>
#include <QList>
#include <QByteArray>
#include <QStringList>
#include <QString>
#include <QHash>
#include <QVariant>

#define COORD_TYPE quint8
#define SIMPLIFIED_PLAYER_ID_TYPE quint16
#define CLAN_ID_TYPE quint8
#define SPEED_TYPE quint8

namespace Pokecraft {
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

struct Player_private_and_public_informations
{
    Player_public_informations public_informations;
    QByteArray randomNumber;//for the battle
    quint64 cash;
    QHash<quint32,quint32> items;
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

struct Map_to_send
{
    Map_semi_border border;
    //QStringList other_map;//border and not

    //quint32 because the format allow it, checked into tryLoadMap()
    quint32 width;
    quint32 height;

    QHash<QString,QVariant> property;

    struct MapToSend_ParsedLayer
    {
        bool *walkable;
        bool *water;
        bool *grass;
        bool *dirt;
    };
    MapToSend_ParsedLayer parsed_layer;

    QList<Map_semi_teleport> teleport;

    struct Map_Point
    {
        COORD_TYPE x,y;
    };
    QList<Map_Point> rescue_points;
    QList<Map_Point> bot_spawn_points;
};
}

#endif // STRUCTURES_GENERAL_H
