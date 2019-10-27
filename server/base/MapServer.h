#ifndef CATCHCHALLENGER_MAP_VISIBILITY_SIMPLE_SERVERMAP_H
#define CATCHCHALLENGER_MAP_VISIBILITY_SIMPLE_SERVERMAP_H

#include "../../general/base/CommonMap.h"
#include "../../general/base/GeneralStructures.h"
#include "../crafting/MapServerCrafting.h"
#include "../VariableServer.h"

#include <vector>

namespace CatchChallenger {
class MapVisibilityAlgorithm_Simple_StoreOnReceiver;
class MapVisibilityAlgorithm_Simple_StoreOnSender;
class MapVisibilityAlgorithm_WithBorder_StoreOnSender;
class Client;

class MapServer : public CommonMap, public MapServerCrafting
{
public:
    MapServer();
    void doDDOSLocalChat();
    static unsigned int playerToFullInsert(const Client * const player, char * const bufferForOutput);
    std::map<std::pair<uint8_t,uint8_t>,Orientation/*,pairhash*/> rescue;
    uint8_t localChatDrop[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE];
    uint8_t localChatDropTotalCache;
    uint8_t localChatDropNewValue;
    int reverse_db_id;

    //at last to improve the other variable cache
    std::vector<Client *> clientsForBroadcast;//frequent remove/insert due to map change
    struct ItemOnMap
    {
        uint16_t pointOnMapDbCode;

        uint16_t item;
        bool infinite;
    };
    std::map<std::pair<uint8_t,uint8_t>,ItemOnMap/*,pairhash*/> pointOnMap_Item;//first is x,y, second is db code, item
    //std::map<std::pair<uint8_t,uint8_t>,PlantOnMap,pairhash> plants;->see MapServerCrafting
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        //CommonMap
        buf << border << teleporter_list_size;
        for(unsigned int i=0; i<teleporter_list_size; i++)
            buf << teleporter[i];
        buf << width << height << group << id << parsed_layer.monstersCollisionList;
        buf << std::string(parsed_layer.simplifiedMap,width*height);
        buf << shops << learn << heal << market << zonecapture << botsFight << botsFightTrigger;
        //MapServerCrafting
        buf << plants;
        //server map
        buf << rescue << reverse_db_id << pointOnMap_Item;
    }
    template <class B>
    void parse(B& buf) {
        //CommonMap
        buf >> border >> teleporter_list_size;
        teleporter=(CommonMap::Teleporter *)malloc(sizeof(CommonMap::Teleporter)*teleporter_list_size);
        for(unsigned int i=0; i<teleporter_list_size; i++)
            buf >> teleporter[i];
        buf >> width >> height >> group >> id >> parsed_layer.monstersCollisionList;
        std::string rawData;
        buf >> rawData;
        parsed_layer.simplifiedMap=(uint8_t *)malloc(rawData.size());
        memcpy(parsed_layer.simplifiedMap,rawData.data(),rawData.size());
        buf >> shops >> learn >> heal >> market >> zonecapture >> botsFight >> botsFightTrigger;
        //MapServerCrafting
        buf >> plants;
        //server map
        buf >> rescue << reverse_db_id << pointOnMap_Item;
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
