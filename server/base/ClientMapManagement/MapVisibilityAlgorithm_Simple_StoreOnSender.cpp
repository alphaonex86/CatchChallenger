#include "MapVisibilityAlgorithm_Simple_StoreOnSender.hpp"
#include "MapVisibilityAlgorithm_WithoutSender.hpp"
#include "../GlobalServerData.hpp"
#include "../../VariableServer.hpp"

using namespace CatchChallenger;

//temp variable for purge buffer
bool MapVisibilityAlgorithm_Simple_StoreOnSender::mapHaveChanged;

MapVisibilityAlgorithm_Simple_StoreOnSender::MapVisibilityAlgorithm_Simple_StoreOnSender() :
    Client(),
    to_send_insert(false),
    haveNewMove(false)
{
    #ifdef CATCHCHALLENGER_SERVER_MAP_DROP_BLOCKED_MOVE
    previousMovedUnitBlocked=0;
    #endif
}

MapVisibilityAlgorithm_Simple_StoreOnSender::~MapVisibilityAlgorithm_Simple_StoreOnSender()
{
    extraStop();
}

/// \todo at client insert, check buffer overflow
void MapVisibilityAlgorithm_Simple_StoreOnSender::insertClient()
{
    Map_server_MapVisibility_Simple_StoreOnSender * const temp_map=static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(map);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        unsigned int index=0;
        while(index<GlobalServerData::serverPrivateVariables.map_list.size())
        {
            if(GlobalServerData::serverPrivateVariables.flat_map_list[index]==temp_map)
                break;
            index++;
        }
        if(index>=GlobalServerData::serverPrivateVariables.map_list.size())
        {
            std::cerr << "MapVisibilityAlgorithm_Simple_StoreOnSender::insertClient(): !vectorcontainsAtLeastOne(flat_map_list,temp_map)" << std::endl;
            abort();
        }
    }
    #endif
    if(Q_LIKELY(temp_map->show))
    {
        const size_t loop_size=temp_map->clients.size();
        if(Q_UNLIKELY(loop_size>=GlobalServerData::serverSettings.mapVisibility.simple.max))
        {
            #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
            normalOutput("insertClient() too many client, hide now, into: "+map->map_file);
            #endif
            temp_map->show=false;
            //drop all show client because it have excess the limit
            //drop on all client
            if(temp_map->send_reinsert_all==false)
                temp_map->send_drop_all=true;
            else
                temp_map->send_reinsert_all=false;
        }
        else//why else dropped?
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(this->x>=this->map->width)
            {
                std::cerr << "x to out of map: " << std::to_string(this->x) << " > " << std::to_string(this->map->width) << " (" << this->map->map_file << ")" << std::endl;
                abort();
                return;
            }
            if(this->y>=this->map->height)
            {
                std::cerr << "y to out of map: " << std::to_string(this->y) << " > " << std::to_string(this->map->height) << " (" << this->map->map_file << ")" << std::endl;
                abort();
                return;
            }
            #endif
            #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
            //normalOutput(std::stringLiteral("insertClient() insert the client, into: %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
            #endif
            //insert the new client
            to_send_insert=true;
            haveNewMove=false;
            vectorremoveOne(temp_map->to_send_remove,public_and_private_informations.public_informations.simplifiedId);
            temp_map->to_send_insert=true;
        }
        saveChange(temp_map);
    }
    else
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        normalOutput("insertClient() already too many client, into: "+map->map_file);
        #endif
    }
    //auto insert to know where it have spawn, now in charge of ClientLocalCalcule
    //insertAnotherClient(player_id,current_map,x,y,last_direction,speed);
}

void MapVisibilityAlgorithm_Simple_StoreOnSender::moveClient(const uint8_t &,const Direction &)
{
    Map_server_MapVisibility_Simple_StoreOnSender *temp_map=static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(map);
    if(Q_UNLIKELY(mapHaveChanged))
    {
        #ifdef DEBUG_MESSAGE_CLIENT_MOVE
        normalOutput("map have change, direction: "+MoveOnTheMap::directionToString(direction)+
                     ": ("+std::to_string(x)+
                     ","+std::to_string(y)+
                     "): "+std::to_string(public_and_private_informations.public_informations.simplifiedId)+
                     ", send at "+std::to_string(temp_map->clients.size()-1)+
                     " player(s)");
        #endif
        if(Q_LIKELY(temp_map->show))
        {
            //insert the new client, do into insertClient(), call by singleMove()
        }
        else
        {
            //drop all show client because it have excess the limit, do into removeClient(), call by singleMove()
        }
    }
    else
    {
        saveChange(temp_map);
        if(to_send_insert)
            return;
        #ifdef CATCHCHALLENGER_SERVER_MAP_DROP_OVER_MOVE
        //already into over move
        if(haveNewMove)
        {
            #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
            normalOutput("moveAnotherClientWithMap("+std::to_string(getPlayerId())+
                         ","+std::to_string(movedUnit)+
                         ","+MoveOnTheMap::directionToString(direction)+
                         ") to the player: "+std::to_string(public_and_private_informations.public_informations.simplifiedId)+
                         ", already into over move");
            #endif
            //to_send_map_management_remove.remove(player_id); -> what?
            return;//quit now
        }
        #endif
        //here to know how player is affected
        #ifdef DEBUG_MESSAGE_CLIENT_MOVE
        normalOutput("after "+MoveOnTheMap::directionToString(direction)+
                     ": ("+std::to_string(x)+
                     ","+std::to_string(y)+
                     "): "+std::to_string(public_and_private_informations.public_informations.simplifiedId)+
                     ", send at "+std::to_string(temp_map->clients.size()-1)+
                     " player(s)");
        #endif

        //normal operation
        if(Q_LIKELY(temp_map->show))
        {
            haveNewMove=true;
            #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
            normalOutput("moveAnotherClientWithMap("+std::to_string(getPlayerId())+
                         ","+std::to_string(movedUnit)+
                         ","+MoveOnTheMap::directionToString(direction)+
                         ") to the player: "+std::to_string(public_and_private_informations.public_informations.simplifiedId)+
                         ", normal move");
            #endif
        }
        else //all client is dropped due to over load on the map
        {
        }
    }
}

//drop all clients
void MapVisibilityAlgorithm_Simple_StoreOnSender::dropAllClients()
{
    to_send_insert=false;
    haveNewMove=false;
    saveChange(static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(map));

    Client::dropAllClients();
}

void MapVisibilityAlgorithm_Simple_StoreOnSender::purgeBuffer()
{
}

void MapVisibilityAlgorithm_Simple_StoreOnSender::removeClient()
{
    Map_server_MapVisibility_Simple_StoreOnSender *temp_map=static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(map);
    const size_t loop_size=temp_map->clients.size();
    if(Q_UNLIKELY(temp_map->show==false))
    {
        if(Q_UNLIKELY(loop_size<=(GlobalServerData::serverSettings.mapVisibility.simple.reshow)))
        {
            #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
            normalOutput("removeClient() client of the map is now under the limit, reinsert all into: "+map->map_file);
            #endif
            temp_map->show=true;
            //insert all the client because it start to be visible
            if(loop_size<=0)
            {
                temp_map->to_send_insert=false;
                temp_map->send_drop_all=false;
                temp_map->send_reinsert_all=false;
                temp_map->have_change=false;
            }
            if(temp_map->send_drop_all==false)
                temp_map->send_reinsert_all=true;
            else
                temp_map->send_drop_all=false;
            saveChange(static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(map));
        }
        //nothing removed because all clients are already hide
        else
        {
            #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
            normalOutput("removeClient() do nothing because client hiden, into: "+map->map_file);
            #endif
        }
    }
    else //normal working
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        //normalOutput(std::stringLiteral("removeClient() normal work, just remove from client on: %1").arg(map->map_file));
        #endif
        //on client side to remove the other visible client for later reinsert
        Client::dropAllClients();
        //on rest side
        if(to_send_insert)
            to_send_insert=false;
        else
        {
            haveNewMove=false;
            temp_map->to_send_remove.push_back(public_and_private_informations.public_informations.simplifiedId);
        }
        saveChange(static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(map));
    }
}

void MapVisibilityAlgorithm_Simple_StoreOnSender::mapVisiblity_unloadFromTheMap()
{
    removeClient();
}

void MapVisibilityAlgorithm_Simple_StoreOnSender::extraStop()
{
    if(stat!=ClientStat::CharacterSelected)
        return;
    unloadFromTheMap();//product remove on the map

    to_send_insert=false;
    haveNewMove=false;
}

bool MapVisibilityAlgorithm_Simple_StoreOnSender::singleMove(const Direction &direction)
{
    CommonMap *old_map=map;
    if(!Client::singleMove(direction))//check of colision disabled because do into LocalClientHandler
        return false;
    if(old_map!=map)
    {
        if(old_map==NULL)
            normalOutput("singleMove() old map is null");
        else
        {
            #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
            normalOutput("singleMove() have change from old map: "+old_map->map_file);
            normalOutput("singleMove() to the new map: "+map->map_file);
            #endif
            mapHaveChanged=true;
            CommonMap *new_map=map;
            map=old_map;
            unloadFromTheMap();
            map=static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(new_map);
            if(map==NULL)
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

void MapVisibilityAlgorithm_Simple_StoreOnSender::loadOnTheMap()
{
    insertClient();
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(vectorcontainsAtLeastOne(static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(map)->clients,this))
    {
        normalOutput("loadOnTheMap() try dual insert into the player list");
        return;
    }
    #endif
    static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(map)->clients.push_back(this);
}

void MapVisibilityAlgorithm_Simple_StoreOnSender::unloadFromTheMap()
{
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(!vectorcontainsAtLeastOne(static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(map)->clients,this))
    {
        normalOutput("unloadFromTheMap() try remove of the player list, but not found");
        return;
    }
    #endif
    if(map==NULL)
        return;
    vectorremoveOne(static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(map)->clients,this);
    mapVisiblity_unloadFromTheMap();
}

//map slots, transmited by the current ClientNetworkRead
void MapVisibilityAlgorithm_Simple_StoreOnSender::put_on_the_map(CommonMap * const map,const /*COORD_TYPE*/uint8_t &x,const /*COORD_TYPE*/uint8_t &y,const Orientation &orientation)
{
    Client::put_on_the_map(map,x,y,orientation);
    loadOnTheMap();
}

bool MapVisibilityAlgorithm_Simple_StoreOnSender::moveThePlayer(const uint8_t &previousMovedUnit,const Direction &direction)
{
    mapHaveChanged=false;
    //move the player on the server map
    if(!Client::moveThePlayer(previousMovedUnit,direction))
        return false;
    //send the move to the other client
    moveClient(previousMovedUnit,direction);
    return true;
}

void MapVisibilityAlgorithm_Simple_StoreOnSender::teleportValidatedTo(CommonMap * const map,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation)
{
    bool mapChange=(this->map!=map);
    normalOutput("MapVisibilityAlgorithm_Simple_StoreOnSender::teleportValidatedTo() with mapChange: "+std::to_string(mapChange));
    if(mapChange)
        unloadFromTheMap();
    Client::teleportValidatedTo(map,x,y,orientation);//apply the change into it
    if(mapChange)
    {
        if(this->map->map_file!=map->map_file)
        {
            errorOutput("Warning: Map pointer != but map_file is same: "+this->map->map_file+
                        " && "+map->map_file+", need be done into Client::teleportValidatedTo()");
            /*#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
            normalOutput(std::stringLiteral("have changed of map for teleportation, old map: %1, new map: %2").arg(this->map->map_file).arg(map->map_file));
            #endif
            this->map=static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(map);
            loadOnTheMap();*/
        }
    }
    else
        haveNewMove=true;
}

void MapVisibilityAlgorithm_Simple_StoreOnSender::saveChange(Map_server_MapVisibility_Simple_StoreOnSender * const map)
{
    if(!map->have_change)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(Map_server_MapVisibility_Simple_StoreOnSender::map_to_update==NULL)
        {
            std::cerr << "Map_server_MapVisibility_Simple_StoreOnSender::map_to_update is null!" << std::endl;
            abort();
        }
        #endif
        map->have_change=true;
        Map_server_MapVisibility_Simple_StoreOnSender::map_to_update[Map_server_MapVisibility_Simple_StoreOnSender::map_to_update_size]=map;
        Map_server_MapVisibility_Simple_StoreOnSender::map_to_update_size++;
    }
}
