#ifndef CATCHCHALLENGER_CLIENT_STRUCTURES_H
#define CATCHCHALLENGER_CLIENT_STRUCTURES_H

#include <string>
#include <unordered_map>
#include <vector>
#include "../../general/base/lib.h"

namespace CatchChallenger {
enum MapEvent
{
    MapEvent_DoubleClick,
    MapEvent_SimpleClick
};
class DLL_PUBLIC ServerFromPoolForDisplay
{
public:
    //connect info
    std::string host;
    uint16_t port;
    uint32_t uniqueKey;

    //displayed info
    std::string name;
    std::string description;
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
    std::string host;
    uint16_t port;
    uint32_t uniqueKey;

    //displayed info
    uint8_t charactersGroupIndex;
    uint16_t maxPlayer;
    uint16_t currentPlayer;

    //temp
    uint8_t logicalGroupIndex;
    std::string xml;
};
struct LogicialGroup
{
    std::string name;
    std::unordered_map<std::string,LogicialGroup *> logicialGroupList;
    std::vector<ServerFromPoolForDisplay> servers;
};
struct ServerForSelection
{
    uint8_t serverIndex;
    uint32_t playedTime;
    uint32_t lastConnect;
};

struct MarketObject
{
    uint32_t marketObjectUniqueId;
    uint16_t item;
    uint32_t quantity;
    uint64_t price;
};
struct MarketMonster
{
    uint32_t index;
    uint16_t monster;
    uint8_t level;
    uint64_t price;
};

}

#endif // CATCHCHALLENGER_CLIENT_STRUCTURES_H
