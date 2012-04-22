#ifndef STRUCTURES_SERVER_H
#define STRUCTURES_SERVER_H

#include <QObject>
#include <QList>
#include <QStringList>
#include <QString>
#include <QSqlDatabase>
#include <QTimer>
#include <QMutex>
#include <QHash>
#include <QVariant>

#include "../pokecraft-general/GeneralStructures.h"

class EventThreader;
class Map_custom;
class ClientBroadCast;
class ClientMapManagement;

struct Map_semi_border_content
{
	QString fileName;
	qint32 x_offset;//can be negative, it's an offset!
	qint32 y_offset;//can be negative, it's an offset!
};

struct Map_semi_border
{
	Map_semi_border_content top;
	Map_semi_border_content bottom;
	Map_semi_border_content left;
	Map_semi_border_content right;
};

struct Map_final_temp
{
	Map_semi_border border;
	//QStringList other_map;//border and not
	quint16 width;
	quint16 height;
	QHash<QString,QVariant> property;

	struct Map_final_parsed_layer
	{
		bool *walkable;
		bool *water;
	};
	Map_final_parsed_layer parsed_layer;
};

/** conversion x,y to position: x+y*width */
struct Map_final
{
	//the index is position (x+y*width)
	struct Map_final_parsed_layer
	{
		bool *walkable;
		bool *water;
	};
	Map_final_parsed_layer parsed_layer;

	struct Map_final_border
	{
		struct Map_final_border_content
		{
			Map_final *map;
			qint32 x_offset;
			qint32 y_offset;
		};
		Map_final_border_content top;
		Map_final_border_content bottom;
		Map_final_border_content left;
		Map_final_border_content right;
	};
	Map_final_border border;

	QList<Map_final *> near_map;//not only the border
	QHash<quint32,Map_final *> other_linked_map;//the int (x+y*width) is position

	QString map_file;
	quint16 width;
	quint16 height;
	QHash<QString,QVariant> property;
	quint32 group;

	QList<ClientMapManagement *> clients;//manipulated by thread of ClientMapManagement()
};

struct Map_player_info
{
	Map_final *map;
	int x,y;
	QString skin;
};

struct GeneralData
{
	//connection
	quint16 max_players;
	quint16 connected_players;
	QList<quint32> connected_players_id_list;
	bool instant_player_number;
	quint16 max_players_displayed,max_view_range;

	// files
	QStringList cached_files_name;/// \todo see if not search facility like QHash

	//bd
	QSqlDatabase *db;//use pointer here to init in correct thread

	//general data
	QTimer *timer_update_number_connected;
	QList<EventThreader *> eventThreaderList;
	QTimer *timer_player_map;

	//interconnected thread
	QList<ClientBroadCast *> clientBroadCastList;
	QMutex clientBroadCastListMutex;

	//map
	QString mapBasePath;
	QHash<QString,Map_final> map_list;
	QHash<quint32,Map_final *> map_list_by_visibility_group;
};

#endif // STRUCTURES_SERVER_H
