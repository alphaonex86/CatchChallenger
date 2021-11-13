#ifndef CATCHCHALLENGER_MAP_VISIBILITY_SIMPLE_SERVERMAP_H
#define CATCHCHALLENGER_MAP_VISIBILITY_SIMPLE_SERVERMAP_H

#include "../../general/base/CommonMap.hpp"
#include "../../general/base/GeneralStructures.hpp"
#include "../../general/hps/hps.h"
#include "../crafting/MapServerCrafting.hpp"
#include "../VariableServer.hpp"
#include "GlobalServerData.hpp"

#include <vector>
#include <iostream>

namespace CatchChallenger {
class MapVisibilityAlgorithm_Simple_StoreOnReceiver;
class MapVisibilityAlgorithm_Simple_StoreOnSender;
class MapVisibilityAlgorithm_WithBorder_StoreOnSender;
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
    int reverse_db_id;

    //at last to improve the other variable cache
    std::vector<Client *> clientsForBroadcast;//frequent remove/insert due to map change
    class ItemOnMap
    {
    public:
        uint16_t pointOnMapDbCode;

        uint16_t item;
        bool infinite;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << pointOnMapDbCode << item << infinite;
        }
        template <class B>
        void parse(B& buf) {
            buf >> pointOnMapDbCode >> item >> infinite;
        }
        #endif
    };
    std::map<std::pair<uint8_t,uint8_t>,ItemOnMap/*,pairhash*/> pointOnMap_Item;//first is x,y, second is db code, item
    //std::map<std::pair<uint8_t,uint8_t>,PlantOnMap,pairhash> plants;->see MapServerCrafting
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        //CommonMap
        std::unordered_map<void *,int32_t> pointer_to_pos;
        pointer_to_pos[nullptr]=-1;
        for(int32_t i=0; i<(int32_t)GlobalServerData::serverPrivateVariables.map_list.size(); i++)
            pointer_to_pos[GlobalServerData::serverPrivateVariables.flat_map_list[i]]=i;
        buf << border.bottom.x_offset << pointer_to_pos.at(border.bottom.map);
        buf << border.left.y_offset << pointer_to_pos.at(border.left.map);
        buf << border.right.y_offset << pointer_to_pos.at(border.right.map);
        buf << border.top.x_offset << pointer_to_pos.at(border.top.map);
        buf << teleporter_list_size;
        for(unsigned int i=0; i<teleporter_list_size; i++)
        {
            const Teleporter &tp=teleporter[i];
            buf << tp.condition << tp.destination_x << tp.destination_y
                << pointer_to_pos.at(tp.map)
                << tp.source_x << tp.source_y;
        }
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
        buf.write((char *)parsed_layer.simplifiedMap,mapSize);
        buf << (uint8_t)shops.size();
        for (const auto x : shops)
              buf << x.first << x.second;
        buf << (uint8_t)learn.size();
        for (const auto x : learn)
              buf << x.first << x.second;
        buf << (uint8_t)heal.size();
        for (const auto x : heal)
              buf << x.first << x.second;
        buf << (uint8_t)market.size();
        for (const auto x : market)
              buf << x.first << x.second;
        buf << (uint8_t)zonecapture.size();
        for (const auto x : zonecapture)
              buf << x.first << x.second;
        buf << (uint8_t)botsFight.size();
        for (const auto x : botsFight)
              buf << x.first << x.second;
        buf << (uint8_t)botsFightTrigger.size();
        for (const auto x : botsFightTrigger)
              buf << x.first << x.second;
        //MapServerCrafting
        buf << plants;
        //server map
        buf << (uint8_t)rescue.size();
        for (const auto x : rescue)
              buf << x.first << (uint8_t)x.second;
        buf << reverse_db_id << pointOnMap_Item;
    }
    static uint32_t mapListSize;
    static CommonMap * posToPointer(const int32_t &pos);
    template <class B>
    void parse(B& buf) {
        //CommonMap
        int32_t pos=0;
        buf >> border.bottom.x_offset >> pos;
        border.bottom.map=posToPointer(pos);
        buf >> border.left.y_offset >> pos;
        border.left.map=posToPointer(pos);
        buf >> border.right.y_offset >> pos;
        border.right.map=posToPointer(pos);
        buf >> border.top.x_offset >> pos;
        border.top.map=posToPointer(pos);
        buf >> teleporter_list_size;
        teleporter=(CommonMap::Teleporter *)malloc(sizeof(CommonMap::Teleporter)*teleporter_list_size);
        for(unsigned int i=0; i<teleporter_list_size; i++)
        {
            Teleporter &tp=teleporter[i];
            buf >> tp.condition >> tp.destination_x >> tp.destination_y
                >> pos
                >> tp.source_x >> tp.source_y;
            tp.map=posToPointer(pos);
        }
        buf >> width;
        buf >> height;
        const uint16_t &mapSize=(uint16_t)width*(uint16_t)height;
        if(mapSize>hps::STREAM_INPUT_BUFFER_SIZE)
        {
            std::cerr << "mapSize: " << mapSize << ">STREAM_INPUT_BUFFER_SIZE: " << hps::STREAM_INPUT_BUFFER_SIZE << "/2" << std::endl;
            abort();
        }
        //buf >> group;
        buf >> id;
        buf >> parsed_layer.monstersCollisionList;
        parsed_layer.simplifiedMap=(uint8_t *)malloc(mapSize);
        buf.read((char *)parsed_layer.simplifiedMap,mapSize);
        uint8_t smallsize=0;
        std::pair<uint8_t,uint8_t> posXY;posXY.first=0;posXY.second=0;
        buf >> smallsize;
        for(uint8_t i=0; i<smallsize; i++)
        {
            std::vector<uint16_t> value;
            buf >> posXY >> value;
            shops[posXY]=value;
        }
        buf >> smallsize;
        for(uint8_t i=0; i<smallsize; i++)
        {
            buf >> posXY.first >> posXY.second;
            learn.insert(posXY);
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
            buf >> posXY.first >> posXY.second;
            market.insert(posXY);
        }
        buf >> smallsize;
        for(uint8_t i=0; i<smallsize; i++)
        {
            std::string value;
            buf >> posXY >> value;
            zonecapture[posXY]=value;
        }
        buf >> smallsize;
        for(uint8_t i=0; i<smallsize; i++)
        {
            std::vector<uint16_t> value;
            buf >> posXY >> value;
            botsFight[posXY]=value;
        }
        buf >> smallsize;
        for(uint8_t i=0; i<smallsize; i++)
        {
            std::vector<uint16_t> value;
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
        buf >> reverse_db_id >> pointOnMap_Item;
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
