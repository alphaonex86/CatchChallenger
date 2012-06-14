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
#include <QSqlQuery>

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
	MapVisibilityAlgorithm_simple,
	MapVisibilityAlgorithm_none
};

struct GeneralData
{
	struct ServerSettings
	{
		//the listen, implicit on the client
		quint16 server_port;
		QString server_ip;

		//settings of the server shared with the client
		CommmonServerSettings commmonServerSettings;

		//rates
		qreal rates_xp_premium;
		qreal rates_gold_premium;
		qreal rates_shiny_premium;

		struct Database
		{
			enum DatabaseType
			{
				DatabaseType_Mysql,
				DatabaseType_SQLite
			};
			DatabaseType type;

			struct Mysql
			{
				//mysql settings
				QString host;
				QString db;
				QString login;
				QString pass;
			};
			Mysql mysql;
		};
		Database database;

		//connection
		quint16 max_players;

		//visibility algorithm
		struct MapVisibility
		{
			MapVisibilityAlgorithm mapVisibilityAlgorithm;

			struct MapVisibility_simple
			{
				quint16 max;
				quint16 reshow;
			};
			MapVisibility_simple simple;
		};
		MapVisibility mapVisibility;
	};
	ServerSettings serverSettings;

	struct ServerPrivateVariables
	{
		//bd
		QSqlDatabase *db;//use pointer here to init in correct thread
		QString datapack_basePath;
		QString datapack_mapPath;
		QRegExp datapack_rightFileName;

		//sql query
		QSqlQuery loginQuery;
		QSqlQuery updateMapPositionQuery;

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

		//connection
		quint16 connected_players;
		PlayerUpdater player_updater;
		QSet<quint32> connected_players_id_list;
	};
	ServerPrivateVariables serverPrivateVariables;
};

#endif // STRUCTURES_SERVER_H
