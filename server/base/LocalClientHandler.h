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
private slots:
    virtual void extraStop();
signals:
    void dbQuery(const QString &sqlQuery);
    void askRandomNumber();

    void seedValidated();
};
}

#endif // POKECRAFT_LOCALPLAYERHANDLER_H
