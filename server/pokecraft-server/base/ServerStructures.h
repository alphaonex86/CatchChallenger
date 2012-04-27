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
#include <QSet>

#include "../../pokecraft-general/base/GeneralStructures.h"
#include "PlayerUpdater.h"

class EventThreader;
class Map_custom;
class ClientBroadCast;
class ClientMapManagement;

struct Map_semi_border_content_top_bottom
{
	QString fileName;
	qint32 x_offset;//can be negative, it's an offset!
};

struct Map_semi_border_content_left_right
{
	QString fileName;
	qint32 y_offset;//can be negative, it's an offset!
};

struct Map_semi_border
{
	Map_semi_border_content_top_bottom top;
	Map_semi_border_content_top_bottom bottom;
	Map_semi_border_content_left_right left;
	Map_semi_border_content_left_right right;
};

struct Map_final_temp
{
	Map_semi_border border;
	//QStringList other_map;//border and not
	quint32 width;
	quint32 height;
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
		struct Map_final_border_content_top_bottom
		{
			Map_final *map;
			qint32 x_offset;
		};
		struct Map_final_border_content_left_right
		{
			Map_final *map;
			qint32 y_offset;
		};
		Map_final_border_content_top_bottom top;
		Map_final_border_content_top_bottom bottom;
		Map_final_border_content_left_right left;
		Map_final_border_content_left_right right;
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

enum MapVisibilityAlgorithm
{
	MapVisibilityAlgorithm_simple
};

struct GeneralData
{
	//connection
	quint16 max_players;
	quint16 connected_players;
	PlayerUpdater player_updater;
	QSet<quint32> connected_players_id_list;
	quint16 max_players_displayed,max_view_range;

	//bd
	QSqlDatabase *db;//use pointer here to init in correct thread

	//general data
	QList<EventThreader *> eventThreaderList;
	QTimer *timer_player_map;

	//interconnected thread
	QList<ClientBroadCast *> clientBroadCastList;
	QMutex clientBroadCastListMutex;

	//map
	QString mapBasePath;
	QHash<QString,Map_final *> map_list;
	QTimer timer_to_send_insert_move_remove;
	MapVisibilityAlgorithm mapVisibilityAlgorithm;

	struct MapVisibility_simple
	{
		quint16 max;
	};
	struct MapVisibility
	{
		MapVisibility_simple simple;
	};
	MapVisibility mapVisibility;
};

#endif // STRUCTURES_SERVER_H
