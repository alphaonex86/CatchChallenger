#ifndef CATCHCHALLENGER_CLIENTMAPMANAGEMENT_H
#define CATCHCHALLENGER_CLIENTMAPMANAGEMENT_H

#include <QObject>
#include <QList>
#include <QString>

#include "../../../general/base/DebugClass.h"
#include "../ServerStructures.h"
#include "../EventThreader.h"
#include "../../VariableServer.h"
#include "MapBasicMove.h"

namespace CatchChallenger {
#ifdef EPOLLCATCHCHALLENGERSERVER
class Client;
#endif
class Map_custom;

class ClientMapManagement : public MapBasicMove
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    Q_OBJECT
    #endif
public:
    explicit ClientMapManagement();
    virtual ~ClientMapManagement();
    virtual void setVariable(Player_internal_informations *player_informations);
    /// \bug is not thread safe, and called by another thread, error can occure
    inline Map_player_info getMapPlayerInfo() const;
    //drop all clients
    virtual void dropAllClients();
    virtual void dropAllBorderClients();
#ifdef EPOLLCATCHCHALLENGERSERVER
    Client *client;
#endif
protected:
    //pass to the Map management visibility algorithm
    virtual void insertClient() = 0;
    virtual void moveClient(const quint8 &previousMovedUnit,const Direction &direction) = 0;
public:
    //map slots, transmited by the current ClientNetworkRead
    virtual bool moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction);
    virtual void purgeBuffer() = 0;
private:
    virtual void extraStop();
};
}

#endif // CLIENTMAPMANAGEMENT_H
