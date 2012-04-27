#ifndef CLIENTMAPMANAGEMENT_H
#define CLIENTMAPMANAGEMENT_H

#include <QObject>
#include <QList>
#include <QString>
#include <QSemaphore>
#include <QTimer>
#include <QHash>
#include <QHashIterator>

#include "../general/base/DebugClass.h"
#include "../ServerStructures.h"
#include "../EventThreader.h"
#include "../../VariableServer.h"

class Map_custom;

class ClientMapManagement : public QObject
{
	Q_OBJECT
public:
	explicit ClientMapManagement();
	~ClientMapManagement();
	void setVariable(GeneralData *generalData);
	/// \bug is not thread safe, and called by another thread, error can occure
	Map_player_info getMapPlayerInfo();
	//to change the info on another client
	virtual void insertAnotherClient(const quint32 &player_id,const Map_final *map,const quint16 &x,const quint16 &y,const Direction &direction,const quint16 &speed);
	virtual void moveAnotherClient(const quint32 &player_id,const quint8 &movedUnit,const Direction &direction);
	virtual void removeAnotherClient(const quint32 &player_id);
	//drop all clients
	void dropAllClients();

	//public to access to info from other object to register
	//info linked
	qint16				x,y;//can be negative because offset to insert on map diff can be put into
	Map_final*			current_map;
	//cache
	quint32	player_id;//to save at the close, and have cache
	//map vector informations
	Direction			last_direction;
	quint16				speed;
protected:
	//pass to the Map management visibility algorithm
	virtual void insertClient() = 0;
	virtual void moveClient(const quint8 &movedUnit,const Direction &direction,const bool &mapHaveChanged) = 0;
	virtual void removeClient() = 0;
	virtual void mapVisiblity_unloadFromTheMap() = 0;
	//internal var
	GeneralData *generalData;
	//debug function
	QString directionToString(const Direction &direction);
	// stuff to send
	QHash<quint32, map_management_insert>			to_send_map_management_insert;
	QHash<quint32, QList<map_management_movement> >		to_send_map_management_move;
	QSet<quint32>						to_send_map_management_remove;
signals:
	//normal signals
	void error(const QString &error);
	void message(const QString &message);
	void isReadyToStop();
	void sendPacket(const QByteArray &data);
	//specific to map signals
	void updatePlayerPosition(const QString & map,const quint16 &x,const quint16 &y,const Orientation &orientation);
public slots:
	//map slots, transmited by the current ClientNetworkRead
	void put_on_the_map(const quint32 &player_id,Map_final *map,const quint16 &x,const quint16 &y,const Orientation &orientation,const quint16 &speed);
	virtual void moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction);
	//normal slots
	void askIfIsReadyToStop();
	void stop();
private slots:
	void purgeBuffer();
private:
	//info linked
	Orientation			at_start_orientation;
	QString				at_start_map_name;

	//related to stop
	volatile bool stopIt;

	//temp variable for purge buffer
	quint16 purgeBuffer_player_affected;
	QByteArray purgeBuffer_outputData;
	QByteArray purgeBuffer_outputDataLoop;
	int purgeBuffer_index;
	int purgeBuffer_list_size;
	int purgeBuffer_list_size_internal;
	int purgeBuffer_indexMovement;
	map_management_move purgeBuffer_move;
	QHash<quint32, QList<map_management_movement> >::const_iterator i_move;
	QHash<quint32, QList<map_management_movement> >::const_iterator i_move_end;
	QHash<quint32, map_management_insert>::const_iterator i_insert;
	QHash<quint32, map_management_insert>::const_iterator i_insert_end;
	QSet<quint32>::const_iterator i_remove;
	QSet<quint32>::const_iterator i_remove_end;

	//temp variable to move on the map
	map_management_movement moveClient_tempMov;

	//temp variable for put on map
	int moveThePlayer_index_move;

	//cache
	bool *walkable;
	quint16 width;

	//map start related
	quint16				at_start_x,at_start_y;
	//map load/unload and change
	void				loadOnTheMap();
	void				unloadFromTheMap();
	bool				mapHaveChanged;

};

#endif // CLIENTMAPMANAGEMENT_H
