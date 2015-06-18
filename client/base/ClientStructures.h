#ifndef CATCHCHALLENGER_CLIENT_STRUCTURES_H
#define CATCHCHALLENGER_CLIENT_STRUCTURES_H

#include <QString>
#include <QHash>
#include <QList>

namespace CatchChallenger {
enum MapEvent
{
    MapEvent_DoubleClick,
    MapEvent_SimpleClick
};
class ServerFromPoolForDisplay
{
public:
    //connect info
    QString host;
    quint16 port;
    quint32 uniqueKey;

    //displayed info
    QString name;
    QString description;
    quint8 charactersGroupIndex;
    quint16 maxPlayer;
    quint16 currentPlayer;
    quint32 playedTime;
    quint32 lastConnect;

    //select info
    quint8 serverOrdenedListIndex;

    bool operator<(const ServerFromPoolForDisplay &serverFromPoolForDisplay) const;
};
struct ServerFromPoolForDisplayTemp
{
    //connect info
    QString host;
    quint16 port;
    quint32 uniqueKey;

    //displayed info
    quint8 charactersGroupIndex;
    quint16 maxPlayer;
    quint16 currentPlayer;

    //temp
    quint8 logicalGroupIndex;
    QString xml;
};
struct LogicialGroup
{
    QString name;
    QHash<QString,LogicialGroup> logicialGroupList;
    QList<ServerFromPoolForDisplay> servers;
};
struct ServerForSelection
{
    quint8 serverIndex;
    quint32 playedTime;
    quint32 lastConnect;
};
}

#endif // CATCHCHALLENGER_CLIENT_STRUCTURES_H
