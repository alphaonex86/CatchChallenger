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
    uint16_t port;
    uint32_t uniqueKey;

    //displayed info
    QString name;
    QString description;
    uint8_t charactersGroupIndex;
    uint16_t maxPlayer;
    uint16_t currentPlayer;
    uint32_t playedTime;
    uint32_t lastConnect;

    //select info
    uint8_t serverOrdenedListIndex;

    bool operator<(const ServerFromPoolForDisplay &serverFromPoolForDisplay) const;
};
struct ServerFromPoolForDisplayTemp
{
    //connect info
    QString host;
    uint16_t port;
    uint32_t uniqueKey;

    //displayed info
    uint8_t charactersGroupIndex;
    uint16_t maxPlayer;
    uint16_t currentPlayer;

    //temp
    uint8_t logicalGroupIndex;
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
    uint8_t serverIndex;
    uint32_t playedTime;
    uint32_t lastConnect;
};
}

#endif // CATCHCHALLENGER_CLIENT_STRUCTURES_H
