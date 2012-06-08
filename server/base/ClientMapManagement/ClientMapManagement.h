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
#include "MapBasicMove.h"

class Map_custom;

class ClientMapManagement : public MapBasicMove
{
	Q_OBJECT
public:
	explicit ClientMapManagement();
	virtual ~ClientMapManagement();
	virtual void setVariable(Player_private_and_public_informations *player_informations);
	/// \bug is not thread safe, and called by another thread, error can occure
	Map_player_info getMapPlayerInfo();
	//to change the info on another client
	virtual void insertAnotherClient(const quint32 &player_id,const Map_final *map,const quint16 &x,const quint16 &y,const Direction &direction,const quint16 &speed);
	virtual void moveAnotherClient(const quint32 &player_id,const quint8 &movedUnit,const Direction &direction);
	virtual void removeAnotherClient(const quint32 &player_id);
	//drop all clients
	virtual void dropAllClients();
protected:
	//pass to the Map management visibility algorithm
	virtual void insertClient() = 0;
	virtual void moveClient(const quint8 &movedUnit,const Direction &direction,const bool &mapHaveChanged) = 0;
	virtual void removeClient() = 0;
	virtual void mapVisiblity_unloadFromTheMap() = 0;
	// stuff to send
	QHash<quint32, map_management_insert>			to_send_map_management_insert;
	QHash<quint32, QList<map_management_movement> >		to_send_map_management_move;
	QSet<quint32>						to_send_map_management_remove;
signals:
	//normal signals
	void error(const QString &error);
	void message(const QString &message);
	void isReadyToStop();
	void sendPacket(const quint8 &mainIdent,const quint16 &subIdent,const bool &packetSize,const QByteArray &data);
	//specific to map signals
	void updatePlayerPosition(const QString & map,const quint16 &x,const quint16 &y,const Orientation &orientation);
public slots:
	//map slots, transmited by the current ClientNetworkRead
	virtual void put_on_the_map(const quint32 &player_id,Map_final *map,const quint16 &x,const quint16 &y,const Orientation &orientation,const quint16 &speed);
	virtual void moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction);
private slots:
	virtual void purgeBuffer();
	virtual void extraStop();
private:
	//info linked
	Orientation			at_start_orientation;
	QString				at_start_map_name;
	quint16				at_start_x,at_start_y;

	//temp variable for purge buffer
	static quint16 purgeBuffer_player_affected;
	static QByteArray purgeBuffer_outputData;
	static QByteArray purgeBuffer_outputDataLoop;
	static int purgeBuffer_index;
	static int purgeBuffer_list_size;
	static int purgeBuffer_list_size_internal;
	static int purgeBuffer_indexMovement;
	static map_management_move purgeBuffer_move;
	static QHash<quint32, QList<map_management_movement> >::const_iterator i_move;
	static QHash<quint32, QList<map_management_movement> >::const_iterator i_move_end;
	static QHash<quint32, map_management_insert>::const_iterator i_insert;
	static QHash<quint32, map_management_insert>::const_iterator i_insert_end;
	static QSet<quint32>::const_iterator i_remove;
	static QSet<quint32>::const_iterator i_remove_end;

	//temp variable to move on the map
	static map_management_movement moveClient_tempMov;
	static map_management_insert insertClient_temp;//can have lot of due to over move

	//temp variable for put on map
	int moveThePlayer_index_move;

	//map load/unload and change
	virtual void			loadOnTheMap();
	virtual void			unloadFromTheMap();
	bool				mapHaveChanged;

};

#endif // CLIENTMAPMANAGEMENT_H
