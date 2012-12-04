#include "ClientMapManagement/MapBasicMove.h"

#ifndef POKECRAFT_LOCALPLAYERHANDLER_H
#define POKECRAFT_LOCALPLAYERHANDLER_H

#include <QObject>
#include <QList>
#include <QString>
#include <QTimer>
#include <QHash>
#include <QHashIterator>
#include <QSqlQuery>
#include <QSqlError>

#include "../../general/base/DebugClass.h"
#include "ServerStructures.h"
#include "EventThreader.h"
#include "../VariableServer.h"
#include "MapServer.h"
#include "SqlFunction.h"

/** \brief Do here only the calcule local to the client
 * That's mean map collision, monster event into grass, fight, object usage, ...
 * no access to other client -> broadcast, no file/db access, no visibility calcule, ...
 * Only here you need use the random list */

namespace Pokecraft {
class LocalClientHandler : public MapBasicMove
{
    Q_OBJECT
public:
    explicit LocalClientHandler();
    virtual ~LocalClientHandler();
private:
    bool checkCollision();
    void getRandomNumberIfNeeded();

    //info linked
    static Direction	temp_direction;
    static QHash<QString,LocalClientHandler *> playerByPseudo;

    //map move
    bool singleMove(const Direction &direction);
public slots:
    void put_on_the_map(Map *map,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation);
    bool moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction);
    //random linked signals
    void newRandomNumber(const QByteArray &randomData);
    //seed
    void useSeed(const quint8 &plant_id);
    void addObject(const quint32 &item,const quint32 &quantity=1);
    quint32 removeObject(const quint32 &item,const quint32 &quantity=1);
    void sendRemoveObject(const quint32 &item,const quint32 &quantity=1);
    quint32 objectQuantity(const quint32 &item);

    void sendHandlerCommand(const QString &command,const QString &extraText);
    //inventory
    void destroyObject(const quint32 &itemId,const quint32 &quantity);
    //teleportation
    void receiveTeleportTo(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
    virtual void teleportValidatedTo(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
private slots:
    virtual void extraStop();
signals:
    void dbQuery(const QString &sqlQuery);
    void askRandomNumber();
    void receiveSystemText(const QString &text,const bool &important=false);

    void seedValidated();
    void teleportTo(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
};
}

#endif // POKECRAFT_LOCALPLAYERHANDLER_H
