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
class Client;

class MapServer : public CommonMap, public MapServerCrafting
{
public:
    //on server you can use GlobalServerData::serverPrivateVariables.flat_map_list to store id and find the right pointer
    MapServer();
    void doDDOSLocalChat();
    bool parseUnknownMoving(std::string type,uint32_t object_x,uint32_t object_y,std::unordered_map<std::string,std::string> property_text) override;
    bool parseUnknownObject(std::string type,uint32_t object_x,uint32_t object_y,std::unordered_map<std::string,std::string> property_text) override;
    bool parseUnknownBotStep(uint32_t object_x,uint32_t object_y,const tinyxml2::XMLElement *step) override;
    static unsigned int playerToFullInsert(const Client &player, char * const bufferForOutput);

    uint8_t localChatDrop[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE];
    uint8_t localChatDropTotalCache;
    uint8_t localChatDropNewValue;
    CATCHCHALLENGER_TYPE_MAPID id_db;
    //the next variable is related to GlobalServerData::serverPrivateVariables.idToZone
    ZONE_TYPE zone;//255 if no zone

protected:
    /* clients list on map
    can be 1000 player but only 10 visible
    16Bits if max connected player is >=255 else 8Bits
    client know their map index, then O(1) remove */
    std::vector<SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED> map_clients_id;
    std::vector<SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED> map_removed_index;

public:
    //return index into map list
    SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED insertOnMap(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index_global);
    void removeOnMap(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index_map);

    SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED map_clients_list_size() const;
    bool map_clients_list_isValid(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index) const;
    const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &map_clients_list_at(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index) const;//abort if index is not valid
public:
    std::unordered_map<std::pair<uint8_t,uint8_t>,Orientation,pairhash> rescue;//less than 5% map have rescue point, maybe other structure like static std::unordered_map<std::unordered_map<std::pair<uint8_t,uint8_t> is better
    std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> heal;//less than 5% map have rescue point, maybe other structure like static std::unordered_map<std::unordered_set<std::pair<uint8_t,uint8_t> is better
    std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> zoneCapture;//ZONE_TYPE removed, will use the zone of current map (prevent error and resolv zone to id), 5% of the map
    //std::map<std::pair<uint8_t,uint8_t>,PlantOnMap,pairhash> plants;->see MapServerCrafting

    //see Map_server_MapVisibility_Simple_StoreOnSender for flat_map_list

    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << border.bottom.x_offset << border.bottom.mapIndex;
        buf << border.left.y_offset << border.left.mapIndex;
        buf << border.right.y_offset << border.right.mapIndex;
        buf << border.top.x_offset << border.top.mapIndex;
        buf << teleporters;
        buf << flat_simplified_map;

        buf << width;
        buf << height;
        buf << monstersCollisionList;
        buf << industries;
        buf << botFights;

        //std::unordered_map<std::pair<uint8_t,uint8_t> and std::unordered_set<std::pair<uint8_t,uint8_t> not supported by HPS
        buf << (uint8_t)shops.size();
        for(const auto& n : shops)
        {
            buf << n.first.first;
            buf << n.first.second;
            buf << n.second;
        }
        buf << (uint8_t)botsFightTrigger.size();
        for(const auto& n : botsFightTrigger)
        {
            buf << n.first.first;
            buf << n.first.second;
            buf << n.second;
        }
        buf << (uint8_t)items.size();
        for(const auto& n : items)
        {
            buf << n.first.first;
            buf << n.first.second;
            buf << n.second;
        }

        buf << (uint8_t)rescue.size();
        for(const auto& n : rescue)
        {
            buf << n.first.first;
            buf << n.first.second;
            buf << (uint8_t)n.second;
        }
        buf << (uint8_t)heal.size();
        for(const auto& n : heal)
        {
            buf << n.first;
            buf << n.second;
        }
        buf << (uint8_t)heal.size();
        for(const auto& n : heal)
        {
            buf << n.first;
            buf << n.second;
        }

        buf << monsterDrops;
    }
    template <class B>
    void parse(B& buf) {
        buf >> teleporters;
        buf >> flat_simplified_map;

        buf >> width;
        buf >> height;
        buf >> monstersCollisionList;
        buf >> industries;
        buf >> botFights;

        uint8_t tempSize=0;

        buf >> tempSize;
        for(int i=0;i<tempSize;i++)
        {
            uint8_t x=0,y=0;
            buf >> x;
            buf >> y;
            Shop s;
            buf >> s;
            shops[std::pair<uint8_t,uint8_t>(x,y)]=s;
        }
        for(int i=0;i<tempSize;i++)
        {
            uint8_t x=0,y=0;
            buf >> x;
            buf >> y;
            uint8_t s;
            buf >> s;
            botsFightTrigger[std::pair<uint8_t,uint8_t>(x,y)]=s;
        }
        for(int i=0;i<tempSize;i++)
        {
            uint8_t x=0,y=0;
            buf >> x;
            buf >> y;
            ItemOnMap s;
            buf >> s;
            items[std::pair<uint8_t,uint8_t>(x,y)]=s;
        }

        for(int i=0;i<tempSize;i++)
        {
            uint8_t x=0,y=0;
            buf >> x;
            buf >> y;
            uint8_t s;
            buf >> s;
            rescue[std::pair<uint8_t,uint8_t>(x,y)]=(Orientation)s;
        }
        for(int i=0;i<tempSize;i++)
        {
            uint8_t x=0,y=0;
            buf >> x;
            buf >> y;
            heal.insert(std::pair<uint8_t,uint8_t>(x,y));
        }
        for(int i=0;i<tempSize;i++)
        {
            uint8_t x=0,y=0;
            buf >> x;
            buf >> y;
            Shop s;
            buf >> s;
            zoneCapture.insert(std::pair<uint8_t,uint8_t>(x,y));
        }

        buf >> monsterDrops;

        for(unsigned int i=0;i<industries.size();i++)
        {
            IndustryStatus s;
            s.last_update=0;
            industriesStatus.push_back(s);
        }
    }
    #endif
};

/*class Map_server_MapVisibility_Simple_StoreOnReceiver : public MapServer
{
public:
    std::vector<MapVisibilityAlgorithm_Simple_StoreOnReceiver *> clients;//manipulated by thread of ClientMapManagement()

    bool show;
};*/
}

#endif
