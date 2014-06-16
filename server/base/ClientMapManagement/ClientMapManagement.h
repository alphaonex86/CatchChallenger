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
public:
    /// \bug is not thread safe, and called by another thread, error can occure
    //map slots, transmited by the current ClientNetworkRead
    bool moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction);
    virtual void purgeBuffer() = 0;
protected:
    //pass to the Map management visibility algorithm
    virtual void insertClient() = 0;
    virtual void moveClient(const quint8 &previousMovedUnit,const Direction &direction) = 0;
};
}

#endif // CLIENTMAPMANAGEMENT_H
