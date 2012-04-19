#ifndef CLIENTMAPMANAGEMENT_H
#define CLIENTMAPMANAGEMENT_H

#include <QObject>
#include <QList>
#include <QString>
#include <QSemaphore>
#if defined(POKECRAFT_SERVER_MAP_MANAGEMENT_LIST_TYPE_QT_QHASH) || defined(POKECRAFT_SERVER_MAP_MANAGEMENT_LIST_TYPE_QT_QHASH2)
#include <QHash>
#include <QHashIterator>
#endif
#if defined(POKECRAFT_SERVER_MAP_MANAGEMENT_LIST_TYPE_STL_LIST) || defined(POKECRAFT_SERVER_MAP_MANAGEMENT_LIST_TYPE_QT_QHASH2)
#include <list>
using namespace std;
#endif

#include "Map_custom.h"
#include "ServerStructures.h"
#include "EventThreader.h"
#include "../VariableServer.h"

class Map_custom;

class ClientMapManagement : public QObject
{
	Q_OBJECT
public:
	explicit ClientMapManagement();
	~ClientMapManagement();
	void setVariable(QList<Map_custom *> *map_list,EventThreader* map_loader_thread);
	//add clients linked
	void insertClient(const quint32 &player_id,const QString &map,const quint16 &x,const quint16 &y,const Direction &direction,const quint16 &speed);
	void moveClient(const quint32 &player_id,const quint8 &movedUnit,const Direction &direction);
	void removeClient(const quint32 &player_id);
	void put_on_the_map_by_diff(const QString & map,const qint16 &x_offset_or_real,const qint16 &y_offset_or_real);
	//info linked
	qint16				x,y;//can be negative because offset to insert on map diff can be put into
	quint16				at_start_x,at_start_y;
	quint32				player_id;
	quint16				speed;
	qint16				x_offset,y_offset;
	Map_custom*			current_map;
	bool				have_diff;
	Direction			last_direction;
	Direction			last_direction_diff;
	void mapError(QString errorString);
	void setBasePath(const QString &basePath);
	void stop();
	Map_player_info getMapPlayerInfo();
	void propagate();
signals:
	//map signals
	void needLoadMap(const QString & mapName);
	//normal signals
	void error(const QString &error);
	void message(const QString &message);
	void isReadyToStop();
	void sendPacket(const QByteArray &data);
	void updatePlayerPosition(const QString & map,const quint16 &x,const quint16 &y,const Orientation &orientation);
public slots:
	//map slots
	void put_on_the_map(const quint32 &player_id,const QString & map,const quint16 &x,const quint16 &y,const Orientation &orientation,const quint16 &speed);
	void moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction,bool with_diff=false);
	void purgeBuffer();
	//normal slots
	void askIfIsReadyToStop();
private:
	//internal var
	QList<Map_custom *> *		map_list;
	EventThreader*			map_loader_thread;
	//stuff to send
	QList<map_management_insert>	to_send_map_management_insert;

	QHash<quint32, QList<map_management_movement> >		to_send_map_management_move;

	QList<quint32>			to_send_map_management_remove;
	//info linked
	Orientation			at_start_orientation;
	QString				at_start_map_name;
	//method
	Map_custom *			getMap(const QString & mapName);
	void unloadFromCurrentMap();
	void unloadMapIfNeeded(Map_custom * map);
	void getNearClient(QList<ClientMapManagement *> & returnList);
	void preMapMove(const quint8 &previousMovedUnit,const Direction &direction);
	void postMapMove();
	bool propagated;
	//temp variable
	Map_custom * old_map;
	QString basePath;
	volatile bool stopIt;
	QSemaphore wait_the_end;
	quint16 purgeBuffer_player_affected;
	QByteArray purgeBuffer_outputData;
	QByteArray purgeBuffer_outputDataLoop;
	int purgeBuffer_index;
	int purgeBuffer_list_size;
	int purgeBuffer_list_size_internal;
	int purgeBuffer_indexMovement;
	map_management_move purgeBuffer_move;
	map_management_movement moveClient_tempMov;
	map_management_move moveClient_temp;
	int moveClient_list_size,moveClient_index;
	map_management_insert insertClient_temp;
	int postMapMove_size;
	QList<ClientMapManagement *> MapMove_nearClient;
	int list_size,index;
	int getNearClient_list_size,getNearClient_index;
	QList<ClientMapManagement *> moveThePlayer_returnList;
	int moveThePlayer_list_size,moveThePlayer_index;
	quint8 moveThePlayer_index_move;
	bool firstInsert;
};

#endif // CLIENTMAPMANAGEMENT_H
