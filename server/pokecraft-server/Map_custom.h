#ifndef MAP_CUSTOM_H
#define MAP_CUSTOM_H

#include <QObject>
#include <QList>
#include <QStringList>
#include <QStringList>

#include "../pokecraft-general/DebugClass.h"
#include "ServerStructures.h"
#include "Map_loader.h"
#include "ClientMapManagement.h"
#include "EventThreader.h"

class ClientMapManagement;

/// \todo group all into map_final

class Map_custom : public QObject
{
	Q_OBJECT
public:
	explicit Map_custom(EventThreader* map_loader_thread,QList<Map_custom *> *map_list);
	~Map_custom();
	//other method
	void loadMap(const QString &fileName,const QString &basePath);
	QString mapName();
	//map border
	struct Map_border_left_or_right
	{
		quint16 y;
		QString fileName;
		Map_custom * linked_map_object;
	};
	struct Map_border_top_or_bottom
	{
		quint16 x;
		QString fileName;
		Map_custom * linked_map_object;
	};
	Map_border_left_or_right map_border_left;
	Map_border_left_or_right map_border_right;
	Map_border_top_or_bottom map_border_top;
	Map_border_top_or_bottom map_border_bottom;
	//map info
	quint16 width;
	quint16 height;
	//status
	bool map_loaded;
	//linked map and client
	QList<Map_custom *> linked_map_object;
	QStringList map_not_loaded;
	QList<ClientMapManagement *> clients;
	void check_client_position(const int &index);
	bool is_walkalble(const quint16 &x,const quint16 &y);
	/* put into macro
	static bool is_walkalble(const quint16 &width,const QByteArray &null_data,const QByteArray &Walkable,const QByteArray &Collisions,const QByteArray &Water,quint16 x,quint16 y); */
	static QString directionToString(const Direction &direction);
	Map_final_temp map_final;
private:
	QString fileName;
	quint16 x_spawn;
	quint16 y_spawn;
	QStringList property_name;
	QList<QVariant> property_value;
	Map_loader * map_loader;
	EventThreader* map_loader_thread;
	QList<Map_custom *> *map_list;
	//internal map manipulation
	Map_custom * searchOtherMap(const QString &fileName);
	bool *bool_Walkable,*bool_Water;
	quint64 temp_map_point;
private slots:
	void map_content_loaded();
	void error(const QString &errorString);
signals:
	void tryLoadMap(const QString &fileName);
};

#endif // MAP_CUSTOM_H
