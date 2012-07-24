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
	virtual void setVariable(Player_internal_informations *player_informations);
	/// \bug is not thread safe, and called by another thread, error can occure
	Map_player_info getMapPlayerInfo();
	//drop all clients
	virtual void dropAllClients();
protected:
	//pass to the Map management visibility algorithm
	virtual void insertClient() = 0;
	virtual void moveClient(const quint8 &movedUnit,const Direction &direction,const bool &mapHaveChanged) = 0;
	//virtual void removeClient() = 0;
	virtual void mapVisiblity_unloadFromTheMap() = 0;
public slots:
	//map slots, transmited by the current ClientNetworkRead
	virtual void put_on_the_map(Map_server *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
	virtual bool moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction);
	virtual void purgeBuffer() = 0;
private slots:
	virtual void extraStop();
private:
	//map load/unload and change
	virtual void			loadOnTheMap();
	virtual void			unloadFromTheMap();
	bool				mapHaveChanged;

};

#endif // CLIENTMAPMANAGEMENT_H
