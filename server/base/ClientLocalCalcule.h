#include "ClientMapManagement/MapBasicMove.h"

#ifndef CLIENTLOCALCALCULE_H
#define CLIENTLOCALCALCULE_H

#include <QObject>
#include <QList>
#include <QString>
#include <QSemaphore>
#include <QTimer>
#include <QHash>
#include <QHashIterator>
#include <QSqlQuery>

#include "../general/base/DebugClass.h"
#include "ServerStructures.h"
#include "EventThreader.h"
#include "../VariableServer.h"
#include "Map_server.h"

/** \brief Do here only the calcule local to the client
 * That's mean map collision, monster event into grass, fight, object usage, ...
 * no access to other client -> broadcast, no file/db access, no visibility calcule, ...
 * Only here you need use the random list */

class ClientLocalCalcule : public MapBasicMove
{
	Q_OBJECT
public:
	explicit ClientLocalCalcule();
	virtual ~ClientLocalCalcule();
private:
	bool checkCollision();
	void getRandomNumberIfNeeded();

	//info linked
	Orientation			at_start_orientation;
	Map *				at_start_map_name;
	COORD_TYPE			at_start_x,at_start_y;
	static QString			temp_direction;

	//map move
	bool singleMove(const Direction &direction);
public slots:
	void put_on_the_map(Map *map,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation);
	bool moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction);
	//random linked signals
	void newRandomNumber(const QByteArray &randomData);
private slots:
	virtual void extraStop();
signals:
	void dbQuery(const QSqlQuery &sqlQuery);
	void askRandomNumber();
};

#endif // CLIENTLOCALCALCULE_H
