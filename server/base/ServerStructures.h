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

#include "../general/base/GeneralStructures.h"
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

struct Map_to_send
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

	struct Temp_teleport
	{
		quint32 source_x,source_y;
		quint32 destination_x,destination_y;
		QString map;
	};
	QList<Temp_teleport> teleport;
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
	struct Teleporter
	{
		quint32 x,y;
		Map_final *map;
	};
	QHash<quint32,Teleporter> teleporter;//the int (x+y*width) is position

	QString map_file;
	quint16 width;
	quint16 height;
	quint32 group;

	QList<ClientMapManagement *> clients;//manipulated by thread of ClientMapManagement()

	struct MapVisibility_simple
	{
		bool show;
	};
	struct MapVisibility
	{
		MapVisibility_simple simple;
	};
	MapVisibility mapVisibility;
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

	//visibility algorithm
	MapVisibilityAlgorithm mapVisibilityAlgorithm;
	struct MapVisibility_simple
	{
		quint16 max;
		quint16 reshow;
	};
	struct MapVisibility
	{
		MapVisibility_simple simple;
	};
	MapVisibility mapVisibility;

	QSet<quint8> mainCodeWithoutSubCodeTypeClientToServer;
	QSet<quint8> mainCodeWithoutSubCodeTypeServerToClient;
	QHash<quint8,quint16> sizeOnlyMainCodePacketClientToServer;
	QHash<quint8,quint16> sizeOnlyMainCodePacketServerToClient;
	QHash<quint8,QHash<quint16,quint16> > sizeMultipleCodePacketClientToServer;
	QHash<quint8,QHash<quint16,quint16> > sizeMultipleCodePacketServerToClient;
};

#endif // STRUCTURES_SERVER_H
