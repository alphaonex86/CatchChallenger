#include "ClientMapManagement/MapBasicMove.h"

#ifndef CATCHCHALLENGER_CLIENTLOCALBROADCAST_H
#define CATCHCHALLENGER_CLIENTLOCALBROADCAST_H

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

/** \brief broadcast at the local map */

namespace CatchChallenger {
class ClientLocalBroadcast : public MapBasicMove
{
    Q_OBJECT
public:
    explicit ClientLocalBroadcast();
    ~ClientLocalBroadcast();
public slots:
    //chat
    void sendLocalChatText(const QString &text);
    //map move
    bool singleMove(const Direction &direction);
    void put_on_the_map(CommonMap *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
    void teleportValidatedTo(CommonMap *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
    //seed
    void seedValidated();
    void plantSeed(const quint8 &query_id,const quint8 &plant_id);
    void collectPlant(const quint8 &query_id);
protected:
    void extraStop();
    void insertClient(CommonMap *map);
    void removeClient(CommonMap *map,const bool &withDestroy=false);
    void sendNearPlant();
    void removeNearPlant();

    struct PlantInWaiting
    {
        quint8 query_id;
        quint8 plant_id;
        CommonMap *map;
        quint8 x,y;
    };
    QList<PlantInWaiting> plant_list_in_waiting;
signals:
    //send reply
    void postReply(const quint8 &queryNumber,const QByteArray &data) const;
    //seed
    void useSeed(const quint8 &plant_id) const;
    void addObjectAndSend(const quint32 &item,const quint32 &quantity=1) const;
    //db
    void dbQuery(const QString &sqlQuery) const;

    bool sendRawSmallPacket(const QByteArray &data) const;
};
}

#endif // CATCHCHALLENGER_CLIENTLOCALBROADCAST_H
