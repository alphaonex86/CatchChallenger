#ifndef CATCHCHALLENGER_MAP_VISIBILITY_SIMPLE_SERVERMAP_H
#define CATCHCHALLENGER_MAP_VISIBILITY_SIMPLE_SERVERMAP_H

#ifdef CATCHCHALLENGER_CACHE_HPS
#include "../../general/hps/hps.h"
#endif
#include "../../general/base/CommonMap.hpp"
#include "../../general/base/GeneralStructures.hpp"
#include "../crafting/MapServerCrafting.hpp"
#include "VariableServer.hpp"
#include "GlobalServerData.hpp"

#include <vector>
#include <iostream>
#include <cstdint>

namespace CatchChallenger {
class MapVisibilityAlgorithm_Simple_StoreOnReceiver;
class MapVisibilityAlgorithm_Simple_StoreOnSender;
class Client;

class MapServer : public CommonMap, public MapServerCrafting
{
public:
    //on server you can use GlobalServerData::serverPrivateVariables.flat_map_list to store id and find the right pointer
    MapServer();
    void doDDOSLocalChat();
    static unsigned int playerToFullInsert(const Client * const player, char * const bufferForOutput);

    uint8_t localChatDrop[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE];
    uint8_t localChatDropTotalCache;
    uint8_t localChatDropNewValue;
    CATCHCHALLENGER_TYPE_MAPID id_db;
    //the next variable is related to GlobalServerData::serverPrivateVariables.idToZone
    ZONE_TYPE zone;//255 if no zone
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    void check6B(const char * const data,const unsigned int size);
    #endif

    //at last to improve the other variable cache
    std::vector<Client *> clientsForLocalBroadcast;//frequent remove/insert due to map change
    std::unordered_map<std::pair<uint8_t,uint8_t>,Orientation/*,pairhash*/> rescue;//less than 5% map have rescue point, maybe other structure like static std::unordered_map<std::unordered_map<std::pair<uint8_t,uint8_t> is better
    std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> heal;//less than 5% map have rescue point, maybe other structure like static std::unordered_map<std::unordered_set<std::pair<uint8_t,uint8_t> is better
    //std::map<std::pair<uint8_t,uint8_t>,PlantOnMap,pairhash> plants;->see MapServerCrafting
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << border.bottom.x_offset << border.bottom.mapIndex;
        buf << border.left.y_offset << border.left.mapIndex;
        buf << border.right.y_offset << border.right.mapIndex;
        buf << border.top.x_offset << border.top.mapIndex;
        buf << teleporter_first_index;
        buf << teleporter_list_size;
        buf << id;
        buf << flat_simplified_map_first_index;

        buf << width;
        buf << height;
        buf << monstersCollisionList;
        buf << industries;
        buf << botFights;
        buf << shops;
        buf << botsFightTrigger;
        buf << zoneCapture;
        buf << pointOnMap_Item;
        buf << monsterDrops;
    }
    template <class B>
    void parse(B& buf) {
        buf >> teleporter_first_index;
        buf >> teleporter_list_size;
        buf >> id;
        buf >> flat_simplified_map_first_index;

        buf >> width;
        buf >> height;
        buf >> monstersCollisionList;
        buf >> industries;
        buf >> botFights;
        buf >> shops;
        buf >> botsFightTrigger;
        buf >> zoneCapture;
        buf >> pointOnMap_Item;
        buf >> monsterDrops;

        for(int i=0;i<industries.size();i++)
        {
            IndustryStatus s;
            s.last_update=0;
            industriesStatus.push_back(s);
        }
    }
    #endif
};

class Map_server_MapVisibility_Simple_StoreOnReceiver : public MapServer
{
public:
    std::vector<MapVisibilityAlgorithm_Simple_StoreOnReceiver *> clients;//manipulated by thread of ClientMapManagement()

    bool show;
};
}

#endif
