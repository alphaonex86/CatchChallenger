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
#include "Map_server.h"

#ifndef STRUCTURES_SERVER_H
#define STRUCTURES_SERVER_H

class EventThreader;
class Map_custom;
class ClientBroadCast;
class ClientMapManagement;
class FakeBot;

/*use template here, like:
template<T>
struct Map
{
	T *map;
};

struct Map_server : Map<Map_server>
{
	bool 	metadonnes;
};
  */

/** conversion x,y to position: x+y*width */
/*struct Map_server
{
	//the index is position (x+y*width)
	struct Map_ParsedLayer
	{
		bool *walkable;
		bool *water;
	};
	Map_ParsedLayer parsed_layer;

	struct Map_Border
	{
		struct Map_BorderContent_TopBottom
		{
			Map_server *map;
			qint32 x_offset;
		};
		struct Map_BorderContent_LeftRight
		{
			Map_server *map;
			qint32 y_offset;
		};
		Map_BorderContent_TopBottom top;
		Map_BorderContent_TopBottom bottom;
		Map_BorderContent_LeftRight left;
		Map_BorderContent_LeftRight right;
	};
	Map_Border border;

	QList<Map_server *> near_map;//not only the border
	struct Teleporter
	{
		quint32 x,y;
		Map_server *map;
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
};*/

struct Map_player_info
{
	Map_server *map;
	int x,y;
	QString skin;
};

enum MapVisibilityAlgorithm
{
	MapVisibilityAlgorithm_simple,
	MapVisibilityAlgorithm_none
};

struct Player_internal_informations
{
	Player_private_and_public_informations public_and_private_informations;
	bool isFake;
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
		QHash<QString,Map_server *> map_list;
		QTimer timer_to_send_insert_move_remove;

		//connection
		quint16 connected_players;
		PlayerUpdater player_updater;
		QSet<quint32> connected_players_id_list;

		//bot
		QList<FakeBot *> fake_clients;
		struct BotSpawn
		{
			QString map;
			quint16 x;
			quint16 y;
		};
		QList<BotSpawn> botSpawn;
		int botSpawnIndex;
	};
	ServerPrivateVariables serverPrivateVariables;
};

#endif // STRUCTURES_SERVER_H
