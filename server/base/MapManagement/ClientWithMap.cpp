#include "ClientWithMap.hpp"
#include "../GlobalServerData.hpp"
#include "../VariableServer.hpp"
#include "Map_server_MapVisibility_Simple_StoreOnSender.hpp"

using namespace CatchChallenger;

std::vector<ClientWithMap> ClientWithMap::clients;
std::vector<SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED> ClientWithMap::clients_removed_index;

ClientWithMap::ClientWithMap() :
    Client()
{
    #ifdef CATCHCHALLENGER_SERVER_MAP_DROP_BLOCKED_MOVE
    previousMovedUnitBlocked=0;
    #endif
}

ClientWithMap::~ClientWithMap()
{
    extraStop();
}

/// \todo at client insert, check buffer overflow
void ClientWithMap::insertClient()
{
}

void ClientWithMap::moveClient(const uint8_t &,const Direction &)
{
}

//drop all clients
void ClientWithMap::dropAllClients()
{
    Client::dropAllClients();
}

void ClientWithMap::purgeBuffer()
{
}

void ClientWithMap::removeClient()
{
}

void ClientWithMap::mapVisiblity_unloadFromTheMap()
{
    removeClient();
}

void ClientWithMap::extraStop()
{
    if(stat!=ClientStat::CharacterSelected)
        return;
    unloadFromTheMap();//product remove on the map
}

bool ClientWithMap::singleMove(const Direction &direction)
{
    CATCHCHALLENGER_TYPE_MAPID old_map=mapIndex;
    if(!Client::singleMove(direction))//check of colision disabled because do into LocalClientHandler
        return false;
    if(old_map!=mapIndex)
    {
        if(old_map==65535)
            normalOutput("singleMove() old map is null");
        else
        {
            #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
            normalOutput("singleMove() have change from old map: "+old_map->map_file);
            normalOutput("singleMove() to the new map: "+map->map_file);
            #endif
            CATCHCHALLENGER_TYPE_MAPID new_map=mapIndex;
            mapIndex=old_map;
            unloadFromTheMap();
            mapIndex=new_map;
            if(new_map==65535)
                normalOutput("singleMove() new map is null");
            else
            {
                #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
                normalOutput("singleMove() to the new map: "+map->map_file);
                #endif
                loadOnTheMap();
            }
        }
    }
    return true;
}

void ClientWithMap::loadOnTheMap()
{
    insertClient();
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(vectorcontainsAtLeastOne(static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(map)->clients,this))
    {
        normalOutput("loadOnTheMap() try dual insert into the player list");
        return;
    }
    #endif
    Map_server_MapVisibility_Simple_StoreOnSender &map=Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list[mapIndex];
    if(map.removed_index.empty())
    {
        index_on_map=map.clients.size();
        map.clients.push_back(index_connected_player);
    }
    else
    {
        index_on_map=map.removed_index.back();
        map.removed_index.pop_back();
        map.clients[index_on_map]=index_connected_player;
    }
}

void ClientWithMap::unloadFromTheMap()
{
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(!vectorcontainsAtLeastOne(static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(map)->clients,this))
    {
        normalOutput("unloadFromTheMap() try remove of the player list, but not found");
        return;
    }
    #endif
    Map_server_MapVisibility_Simple_StoreOnSender &map=Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list[mapIndex];
    map.clients[index_on_map]=255;
    map.removed_index.push_back(index_on_map);
    mapVisiblity_unloadFromTheMap();
    index_on_map=255;
}

//mapIndex slots, transmited by the current ClientNetworkRead
void ClientWithMap::put_on_the_map(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y, const Orientation &orientation)
{
    Client::put_on_the_map(mapIndex,x,y,orientation);//first insert, send pos to client
    loadOnTheMap();
}

bool ClientWithMap::moveThePlayer(const uint8_t &previousMovedUnit,const Direction &direction)
{
    //move the player on the server map
    if(!Client::moveThePlayer(previousMovedUnit,direction))
        return false;
    //send the move to the other client
    moveClient(previousMovedUnit,direction);
    return true;
}

void ClientWithMap::teleportValidatedTo(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation)
{
    bool mapChange=(this->mapIndex!=mapIndex);
    normalOutput("MapVisibilityAlgorithm_Simple_StoreOnSender::teleportValidatedTo() with mapChange: "+std::to_string(mapChange));
    if(mapChange)
        unloadFromTheMap();
    Client::teleportValidatedTo(mapIndex,x,y,orientation);//apply the change into it
}
