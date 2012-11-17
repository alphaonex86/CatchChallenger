#include "ClientMapManagement/MapBasicMove.h"

#ifndef POKECRAFT_CLIENTLOCALBROADCAST_H
#define POKECRAFT_CLIENTLOCALBROADCAST_H

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
#include "Map_server.h"
#include "SqlFunction.h"

/** \brief broadcast at the local map */

namespace Pokecraft {
class ClientLocalBroadcast : public MapBasicMove
{
    Q_OBJECT
public:
    explicit ClientLocalBroadcast();
    virtual ~ClientLocalBroadcast();
public slots:
    void sendLocalChatText(const QString &text);
    void receiveChatText(const QString &text, const Player_internal_informations *sender_informations);

    bool singleMove(const Direction &direction);
    virtual void put_on_the_map(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
};
}

#endif // POKECRAFT_CLIENTLOCALBROADCAST_H
