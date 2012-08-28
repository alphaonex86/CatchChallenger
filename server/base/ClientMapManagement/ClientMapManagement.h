#ifndef POKECRAFT_CLIENTMAPMANAGEMENT_H
#define POKECRAFT_CLIENTMAPMANAGEMENT_H

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

namespace Pokecraft {
class Map_custom;

class ClientMapManagement : public MapBasicMove
{
	Q_OBJECT
public:
	explicit ClientMapManagement();
	virtual ~ClientMapManagement();
	virtual void setVariable(Player_internal_informations *player_informations);
	/// \bug is not thread safe, and called by another thread, error can occure
	Map_player_info getMapPlayerInfo();
	//drop all clients
	virtual void dropAllClients();
protected:
	//pass to the Map management visibility algorithm
	virtual void insertClient() = 0;
	virtual void moveClient(const quint8 &previousMovedUnit,const Direction &direction) = 0;
public slots:
	//map slots, transmited by the current ClientNetworkRead
	virtual bool moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction);
	virtual void purgeBuffer() = 0;
private slots:
	virtual void extraStop();
};
}

#endif // CLIENTMAPMANAGEMENT_H
