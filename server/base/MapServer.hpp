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
    std::map<std::pair<uint8_t,uint8_t>,Orientation/*,pairhash*/> rescue;
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
    std::vector<Client *> clientsForBroadcast;//frequent remove/insert due to map change
    class ItemOnMap
    {
    public:
        uint16_t item;
        bool infinite;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << item << infinite;
        }
        template <class B>
        void parse(B& buf) {
            buf >> item >> infinite;
        }
        #endif
    };
    std::map<std::pair<uint8_t,uint8_t>,ItemOnMap/*,pairhash*/> pointOnMap_Item;//first is x,y, second is db code, item
    //std::map<std::pair<uint8_t,uint8_t>,PlantOnMap,pairhash> plants;->see MapServerCrafting
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        //CommonMap
        buf << border.bottom.x_offset << border.bottom.mapIndex;
        buf << border.left.y_offset << border.left.mapIndex;
        buf << border.right.y_offset << border.right.mapIndex;
        buf << border.top.x_offset << border.top.mapIndex;
        buf << teleporter_list_size << teleporter_first_index;
        buf << width;
        buf << height;
        const uint16_t &mapSize=(uint16_t)width*(uint16_t)height;
        if(mapSize>hps::STREAM_INPUT_BUFFER_SIZE/2)
        {
            std::cerr << "mapSize: " << mapSize << ">STREAM_INPUT_BUFFER_SIZE: " << hps::STREAM_INPUT_BUFFER_SIZE << "/2" << std::endl;
            abort();
        }
        //buf << group;
        buf << id;
        buf << parsed_layer.monstersCollisionList;
        buf << parsed_layer.simplifiedMapIndex;
        buf << (uint8_t)shops.size();
        for (const auto &x : shops)
              buf << x.first << x.second;
        buf << (uint8_t)heal.size();
        for (const auto &x : heal)
              buf << x.first << x.second;
        buf << (uint8_t)zoneCapture.size();
        for (const auto &x : zoneCapture)
              buf << x.first << x.second;
        buf << (uint8_t)botsFight.size();
        for (const auto &x : botsFight)
              buf << x.first << x.second;
        buf << (uint8_t)botsFightTrigger.size();
        for (const auto &x : botsFightTrigger)
              buf << x.first << x.second;
        //MapServerCrafting
        buf << plants;
        //server map
        buf << (uint8_t)rescue.size();
        for (const auto &x : rescue)
              buf << x.first << (uint8_t)x.second;
        buf << id_db << zone << pointOnMap_Item;
        buf << botFights;
        buf << monsterDrops;
    }
    static CommonMap * posToPointer(const CATCHCHALLENGER_TYPE_MAPID &mappos);
    template <class B>
    void parse(B& buf) {
        //CommonMap
        buf >> border.bottom.x_offset >> border.bottom.mapIndex;
        buf >> border.left.y_offset >> border.left.mapIndex;
        buf >> border.right.y_offset >> border.right.mapIndex;
        buf >> border.top.x_offset >> border.top.mapIndex;
        buf >> teleporter_list_size >> teleporter_first_index;
        buf >> width;
        buf >> height;
        const uint16_t &mapSize=(uint16_t)width*(uint16_t)height;
        /*if(mapSize>hps::STREAM_INPUT_BUFFER_SIZE)
        {
            std::cerr << "mapSize: " << mapSize << ">STREAM_INPUT_BUFFER_SIZE: " << hps::STREAM_INPUT_BUFFER_SIZE << "/2" << std::endl;
            abort();
        }*/
        //buf >> group;
        buf >> id;
        buf >> parsed_layer.monstersCollisionList;
        buf >> parsed_layer.simplifiedMapIndex;
        uint8_t smallsize=0;
        std::pair<uint8_t,uint8_t> posXY;posXY.first=0;posXY.second=0;
        buf >> smallsize;
        for(uint8_t i=0; i<smallsize; i++)
        {
            Shop s;
            buf >> posXY >> s;
            shops[posXY]=s;
        }
        buf >> smallsize;
        for(uint8_t i=0; i<smallsize; i++)
        {
            buf >> posXY.first >> posXY.second;
            heal.insert(posXY);
        }
        buf >> smallsize;
        for(uint8_t i=0; i<smallsize; i++)
        {
            uint16_t value;
            buf >> posXY >> value;
            zoneCapture[posXY]=value;
        }
        buf >> smallsize;
        for(uint8_t i=0; i<smallsize; i++)
        {
            uint8_t value;
            buf >> posXY >> value;
            botsFight[posXY]=value;
        }
        buf >> smallsize;
        for(uint8_t i=0; i<smallsize; i++)
        {
            std::vector<uint8_t> value;
            buf >> posXY >> value;
            botsFightTrigger[posXY]=value;
        }
       //MapServerCrafting
        buf >> plants;
        //server map
        buf >> smallsize;
        for(uint8_t i=0; i<smallsize; i++)
        {
            uint8_t value;
            buf >> posXY >> value;
            rescue[posXY]=(Orientation)value;
        }
        buf >> id_db >> zone >> pointOnMap_Item;
        buf >> botFights;
        buf >> monsterDrops;
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
