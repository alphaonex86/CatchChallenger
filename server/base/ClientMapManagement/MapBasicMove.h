#ifndef MapBasicMove_H
#define MapBasicMove_H

#include <QObject>
#include <QList>
#include <QString>
#include <QSemaphore>
#include <QTimer>
#include <QHash>
#include <QHashIterator>

#include "../general/base/DebugClass.h"
#include "../ServerStructures.h"
#include "../../VariableServer.h"

class MapBasicMove : public QObject
{
	Q_OBJECT
public:
	explicit MapBasicMove();
	virtual ~MapBasicMove();
	virtual void setVariable(Player_internal_informations *player_informations);
	//info linked
	qint16				x,y;//can be negative because offset to insert on map diff can be put into
	Map_server*			current_map;
	//cache
	SIMPLIFIED_PLAYER_ID_TYPE	player_id;//to save at the close, and have cache
	//map vector informations
	Direction			last_direction;

	//debug function
	static QString directionToString(const Direction &direction);

	//internal var
	Player_internal_informations *player_informations;
protected:
	//pass to the Map management visibility algorithm
	virtual void mapTeleporterUsed();
	virtual bool checkCollision();

	//related to stop
	volatile bool stopCurrentMethod;
	volatile bool stopIt;
	virtual void extraStop();

	//map load/unload and change
	virtual void loadOnTheMap();
	virtual void unloadFromTheMap();
signals:
	//normal signals
	void error(const QString &error);
	void message(const QString &message);
	void isReadyToStop();
	void sendPacket(const quint8 &mainIdent,const quint16 &subIdent,const QByteArray &data=QByteArray());
	void sendPacket(const quint8 &mainIdent,const QByteArray &data=QByteArray());
public slots:
	//map slots, transmited by the current ClientNetworkRead
	virtual void put_on_the_map(Map_server *map,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation);
	virtual void moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction);
	//normal slots
	virtual void askIfIsReadyToStop();
	virtual void stop();
private:
	//temp variable for put on map
	static int moveThePlayer_index_move;
};

#endif // MapBasicMove_H
