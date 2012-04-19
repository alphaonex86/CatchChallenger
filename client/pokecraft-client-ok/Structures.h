#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <QObject>
#include <QList>
#include <QByteArray>
#include <QStringList>
#include <QString>
#include <QTimer>

struct Player_informations
{
	quint32 id;
	QString pseudo;
	QString description;
	quint16 clan;
	quint8 status;
	QString skin;
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
	Orientation orientation;
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

#endif // STRUCTURES_H
