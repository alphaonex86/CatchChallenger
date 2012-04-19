#ifndef STRUCTURES_SERVER_H
#define STRUCTURES_SERVER_H

#include <QObject>
#include <QList>
#include <QStringList>
#include <QString>

#include "../pokecraft-general/GeneralStructures.h"

struct Map_border_temp
{
	qint16 x_offset;//can be negative, it's an offset!
	qint16 y_offset;//can be negative, it's an offset!
	QString fileName;
};

struct Map_final_temp
{
	Map_border_temp border_top;
	Map_border_temp border_bottom;
	Map_border_temp border_left;
	Map_border_temp border_right;
	QStringList other_map;//border and not
	quint16 width;
	quint16 height;
	QStringList property_name;
	QList<QVariant> property_value;
	quint16 x_spawn;
	quint16 y_spawn;
	bool *bool_Walkable,*bool_Water;
};

struct Map_player_info
{
	Map_final_temp map;
	QString map_file_path;
	int x,y;
	bool loaded;
	QString skin;
};

#endif // STRUCTURES_SERVER_H
