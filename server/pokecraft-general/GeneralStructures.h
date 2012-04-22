#ifndef STRUCTURES_GENERAL_H
#define STRUCTURES_GENERAL_H

#include <QObject>
#include <QList>
#include <QByteArray>
#include <QStringList>
#include <QString>

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
	Player_type_normal = 0x01,
	Player_type_premium = 0x02,
	Player_type_gm = 0x03,
	Player_type_dev = 0x04
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
	quint32 id;
	QString fileName;
	quint16 x;
	quint16 y;
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
	quint32 id;
	QString pseudo;
	QString description;
	quint16 clan;
	Player_type type;
	QString skin;
};

struct Player_private_and_public_informations
{
	Player_public_informations public_informations;
	quint64 cash;
	bool is_fake;
};

#endif // STRUCTURES_GENERAL_H
