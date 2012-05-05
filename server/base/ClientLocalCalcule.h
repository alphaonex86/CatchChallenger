#ifndef CLIENTLOCALCALCULE_H
#define CLIENTLOCALCALCULE_H

#include <QObject>
#include <QList>
#include <QString>
#include <QSemaphore>
#include <QTimer>
#include <QHash>
#include <QHashIterator>

#include "../general/base/DebugClass.h"
#include "ServerStructures.h"
#include "EventThreader.h"
#include "../VariableServer.h"
#include "ClientMapManagement/MapBasicMove.h"

/** \brief Do here only the calcule local to the client
 * That's mean map collision, monster event into grass, fight, object usage, ...*/
class ClientLocalCalcule : public MapBasicMove
{
	Q_OBJECT
public:
	explicit ClientLocalCalcule();
	virtual ~ClientLocalCalcule();
private:
	bool checkCollision();
public slots:
	void put_on_the_map(const quint32 &player_id,Map_final *map,const quint16 &x,const quint16 &y,const Orientation &orientation,const quint16 &speed);
};

#endif // CLIENTLOCALCALCULE_H
