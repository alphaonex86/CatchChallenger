#ifndef STRUCTURES_SERVER_H
#define STRUCTURES_SERVER_H

#include <QObject>
#include <QList>
#include <QStringList>
#include <QString>
#include <QSqlDatabase>
#include <QTimer>
#include <QMutex>

#include "../pokecraft-general/GeneralStructures.h"

class EventThreader;
class Map_custom;
class ClientBroadCast;

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

struct GeneralData
{
	//connection
	quint16 max_players;
	quint16 connected_players;
	QList<quint32> connected_players_id_list;
	bool instant_player_number;
	// files
	QStringList cached_files_name;/// \todo see if not search facility like QHash
	QList<QByteArray> cached_files_data;
	QList<quint32> cached_files_mtime;
	//bd
	QSqlDatabase *db;//use pointer here to init in correct thread
	//instant variable
	qint64 cache_max_file_size;
	qint64 cache_max_size;
	qint64 cache_size;
	//general data
	QTimer *timer_update_number_connected;
	QList<EventThreader *> eventThreaderList;
	QList<Map_custom *> map_list;
	QTimer *timer_player_map;
	//interconnected thread
	QList<ClientBroadCast *> clientBroadCastList;
	QMutex clientBroadCastListMutex;
};

#endif // STRUCTURES_SERVER_H
