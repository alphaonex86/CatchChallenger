#include "MapVisibilityAlgorithm_WithBorder_StoreOnSender.h"
#include "../GlobalServerData.h"
#include "../../VariableServer.h"

using namespace CatchChallenger;

MapVisibilityAlgorithm_WithBorder_StoreOnSender *MapVisibilityAlgorithm_WithBorder_StoreOnSender::current_client;

//temp variable for purge buffer
CommonMap* MapVisibilityAlgorithm_WithBorder_StoreOnSender::old_map;
bool MapVisibilityAlgorithm_WithBorder_StoreOnSender::mapHaveChanged;

//temp variable to move on the map
map_management_movement MapVisibilityAlgorithm_WithBorder_StoreOnSender::moveClient_tempMov;

MapVisibilityAlgorithm_WithBorder_StoreOnSender::MapVisibilityAlgorithm_WithBorder_StoreOnSender(
        #ifdef EPOLLCATCHCHALLENGERSERVER
            #ifdef SERVERSSL
                const int &infd, SSL_CTX *ctx
            #else
                const int &infd
            #endif
        #else
        ConnectedSocket *socket
        #endif
        ) :
    Client(
        #ifdef EPOLLCATCHCHALLENGERSERVER
            #ifdef SERVERSSL
                infd,ctx
            #else
                infd
            #endif
        #else
        socket
        #endif
        ),
    haveBufferToPurge(false)
{
    #ifdef CATCHCHALLENGER_SERVER_MAP_DROP_BLOCKED_MOVE
    previousMovedUnitBlocked=0;
    #endif
}

MapVisibilityAlgorithm_WithBorder_StoreOnSender::~MapVisibilityAlgorithm_WithBorder_StoreOnSender()
{
    extraStop();
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::insertClient()
{
    Map_server_MapVisibility_WithBorder_StoreOnSender *temp_map=static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map);
    //local map
    if(Q_LIKELY(temp_map->show))
    {
        const uint16_t &loop_size=temp_map->clients.size();
        if(Q_LIKELY(temp_map->showWithBorder))
        {
            if(Q_UNLIKELY((loop_size+temp_map->clientsOnBorder)>=GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder))
            {
                #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
                normalOutput("insertClient() too many client, hide now, into: "+map->map_file);
                #endif
                temp_map->showWithBorder=false;
                //drop all show client because it have excess the limit
                //drop on all client
                if(Q_UNLIKELY(loop_size>=GlobalServerData::serverSettings.mapVisibility.withBorder.max))
                {}
                else
                {
                    uint16_t index=0;
                    while(index<loop_size)
                    {
                        temp_map->clients.at(index)->dropAllBorderClients();
                        index++;
                    }
                }
            }
        }
        if(Q_UNLIKELY(loop_size>=GlobalServerData::serverSettings.mapVisibility.withBorder.max))
        {
            #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
            normalOutput("insertClient() too many client, hide now, into: "+map->map_file);
            #endif
            temp_map->show=false;
            //drop all show client because it have excess the limit
            //drop on all client
            uint16_t index=0;
            while(index<loop_size)
            {
                temp_map->clients.at(index)->dropAllClients();
                index++;
            }
        }
        else//why else dropped?
        {
            #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
            normalOutput("insertClient() insert the client, into: "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
            #endif
            //insert the new client
            const SIMPLIFIED_PLAYER_ID_TYPE &thisSimplifiedId=public_and_private_informations.public_informations.simplifiedId;
            uint16_t index=0;
            while(index<loop_size)
            {
                current_client=temp_map->clients.at(index);
                current_client->insertAnotherClient(thisSimplifiedId,this);
                this->insertAnotherClient(current_client);
                index++;
            }
        }
    }
    else
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        normalOutput("insertClient() already too many client, into: %1"+map->map_file);
        #endif
    }
    //auto insert to know where it have spawn, now in charge of ClientLocalCalcule
    //insertAnotherClient(player_id,current_map,x,y,last_direction,speed);

    //border map
    {
        uint8_t border_map_index=0;
        const uint8_t &border_map_loop_size=map->border_map.size();
        while(border_map_index<border_map_loop_size)
        {
            Map_server_MapVisibility_WithBorder_StoreOnSender *temp_border_map=static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map->border_map.at(border_map_index));
            const uint16_t &loop_size=temp_border_map->clients.size();
            //insert border player on current player
            if(temp_map->showWithBorder)
            {
                uint16_t index=0;
                while(index<loop_size)
                {
                    this->insertAnotherClient(temp_border_map->clients.at(index));
                    index++;
                }
            }
            //insert the new client on border map
            if(Q_LIKELY(temp_border_map->showWithBorder))
            {
                temp_border_map->clientsOnBorder++;
                if(Q_UNLIKELY((loop_size+temp_border_map->clientsOnBorder)>=GlobalServerData::serverSettings.mapVisibility.withBorder.max))
                {
                    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
                    normalOutput("insertClient() too many client, hide now, into: "+map->map_file);
                    #endif
                    temp_border_map->showWithBorder=false;
                    //drop all show client because it have excess the limit
                    //drop on all client
                    uint16_t index=0;
                    while(index<loop_size)
                    {
                        temp_border_map->clients.at(index)->dropAllBorderClients();
                        index++;
                    }
                }
                else
                {
                    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
                    normalOutput("insertClient() insert the client, into: "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                    #endif
                    const SIMPLIFIED_PLAYER_ID_TYPE &thisSimplifiedId=public_and_private_informations.public_informations.simplifiedId;
                    uint16_t index=0;
                    while(index<loop_size)
                    {
                        temp_border_map->clients.at(index)->insertAnotherClient(thisSimplifiedId,this);
                        index++;
                    }
                }
            }
            border_map_index++;
        }
    }
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::moveClient(const uint8_t &movedUnit,const Direction &direction)
{
    Map_server_MapVisibility_WithBorder_StoreOnSender *temp_map=static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map);
    const uint16_t &loop_size=temp_map->clients.size();
    if(Q_UNLIKELY(mapHaveChanged))
    {
        #ifdef DEBUG_MESSAGE_CLIENT_MOVE
        normalOutput("map have change, direction: "+MoveOnTheMap::directionToString(direction)+
                     ": ("+std::to_string(x)+
                     ","+std::to_string(y)+
                     "): "+std::to_string(public_and_private_informations.public_informations.simplifiedId)+
                     ", send at "+std::to_string(loop_size-1)+
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
        //here to know how player is affected
        #ifdef DEBUG_MESSAGE_CLIENT_MOVE
        normalOutput("after "+MoveOnTheMap::directionToString(direction)+
                     ": ("+std::to_string(x)+
                     ","+std::to_string(y)+
                     "): "+std::to_string(public_and_private_informations.public_informations.simplifiedId)+
                     ", send at "+std::to_string(loop_size-1)+
                     " player(s)");
        #endif

        //normal operation
        if(Q_LIKELY(temp_map->show))
        {
            uint16_t index=0;
            while(index<loop_size)
            {
                current_client=temp_map->clients.at(index);
                if(Q_LIKELY(current_client!=this))
                     current_client->moveAnotherClientWithMap(public_and_private_informations.public_informations.simplifiedId,this,movedUnit,direction);
                index++;
            }
        }
        else //all client is dropped due to over load on the map
        {
        }
    }
    const SIMPLIFIED_PLAYER_ID_TYPE &thisSimplifiedId=public_and_private_informations.public_informations.simplifiedId;
    //border map
    uint8_t border_map_index=0;
    const uint8_t &border_map_loop_size=map->border_map.size();
    while(border_map_index<border_map_loop_size)
    {
        Map_server_MapVisibility_WithBorder_StoreOnSender *temp_border_map=static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map->border_map.at(border_map_index));
        const uint16_t &loop_size=temp_border_map->clients.size();
        if(temp_border_map->showWithBorder)
        {
            uint16_t index=0;
            while(index<loop_size)
            {
                temp_border_map->clients.at(index)->moveAnotherClientWithMap(thisSimplifiedId,this,movedUnit,direction);
                index++;
            }
        }
        border_map_index++;
    }
}

//drop all clients
void MapVisibilityAlgorithm_WithBorder_StoreOnSender::dropAllClients()
{
    to_send_insert.clear();
    to_send_move.clear();
    to_send_remove.clear();
    to_send_reinsert.clear();

    Client::dropAllClients();
}

//drop all clients
void MapVisibilityAlgorithm_WithBorder_StoreOnSender::dropAllBorderClients()
{
    to_send_insert.clear();
    to_send_move.clear();
    to_send_remove.clear();
    to_send_reinsert.clear();

    Client::dropAllBorderClients();
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::reinsertClientForOthersOnSameMap()
{
    Map_server_MapVisibility_WithBorder_StoreOnSender* map_temp=static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map);
    if(Q_UNLIKELY(map_temp->show==false))
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        normalOutput("reinsertClientForOthers() skip because not show"+map->map_file);
        #endif
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("reinsertClientForOthers() normal work, just remove from client on: "+map->map_file);
    #endif
    /* useless because the insert will overwrite the position */
    const SIMPLIFIED_PLAYER_ID_TYPE &thisSimplifiedId=public_and_private_informations.public_informations.simplifiedId;
    uint16_t index=0;
    while(index<map_temp->clients.size())
    {
        current_client=map_temp->clients.at(index);
        if(Q_UNLIKELY(current_client!=this))
            current_client->reinsertAnotherClient(thisSimplifiedId,this);
        index++;
    }
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::removeClient()
{
    Map_server_MapVisibility_WithBorder_StoreOnSender *temp_map=static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map);
    const uint16_t &loop_size=temp_map->clients.size();
    if(Q_UNLIKELY(temp_map->show==false))
    {
        if(Q_UNLIKELY(loop_size<=(GlobalServerData::serverSettings.mapVisibility.withBorder.reshow)))
        {
            #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
            normalOutput("removeClient() client of the map is now under the limit, reinsert all into: "+map->map_file);
            #endif
            temp_map->show=true;
            //insert all the client because it start to be visible
            if(Q_UNLIKELY((loop_size+temp_map->clientsOnBorder)<=(GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder)))
            {
                temp_map->showWithBorder=true;
                uint16_t index=0;
                while(index<loop_size)
                {
                    temp_map->clients.at(index)->reinsertAllClientIncludingBorderClients();
                    index++;
                }
            }
            else
            {
                uint16_t index=0;
                while(index<loop_size)
                {
                    temp_map->clients.at(index)->reinsertAllClient();
                    index++;
                }
            }
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
        if(Q_UNLIKELY(temp_map->showWithBorder==false))
        {
            if(Q_UNLIKELY((loop_size+temp_map->clientsOnBorder)<=(GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder)))
            {
                #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
                normalOutput("removeClient() client of the map is now under the limit, reinsert all into: "+map->map_file);
                #endif
                temp_map->showWithBorder=true;
                //insert only the border client because it start to be visible
                uint16_t index=0;
                while(index<loop_size)
                {
                    temp_map->clients.at(index)->reinsertCurrentPlayerOnlyTheBorderClients();
                    index++;
                }
            }
            //nothing removed because all clients are already hide
            else
            {
                #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
                normalOutput("removeClient() do nothing because client hiden, into: "+map->map_file);
                #endif
            }
        }
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        normalOutput("removeClient() normal work, just remove from client on: "+map->map_file);
        #endif
        uint16_t index=0;
        while(index<loop_size)
        {
            current_client=temp_map->clients.at(index);
            current_client->removeAnotherClient(public_and_private_informations.public_informations.simplifiedId);
            this->removeAnotherClient(current_client->public_and_private_informations.public_informations.simplifiedId);
            index++;
        }
    }

    //border map
    {
        uint8_t border_map_index=0;
        const uint8_t &border_map_loop_size=map->border_map.size();
        while(border_map_index<border_map_loop_size)
        {
            Map_server_MapVisibility_WithBorder_StoreOnSender *temp_border_map=static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map->border_map.at(border_map_index));
            const uint16_t &loop_size=temp_border_map->clients.size();
            //remove border client on this
            if(Q_LIKELY(temp_map->showWithBorder==true))
            {
                uint16_t index=0;
                while(index<loop_size)
                {
                    this->removeAnotherClient(temp_border_map->clients.at(index)->public_and_private_informations.public_informations.simplifiedId);
                    index++;
                }
            }
            //remove this ont border client
            if(Q_LIKELY(temp_border_map->showWithBorder==true))
            {
                uint16_t index=0;
                while(index<loop_size)
                {
                    temp_border_map->clients.at(index)->removeAnotherClient(public_and_private_informations.public_informations.simplifiedId);
                    index++;
                }
            }
            else
            {
                if(Q_UNLIKELY((loop_size+temp_border_map->clientsOnBorder)<=(GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder)))
                {
                    temp_border_map->showWithBorder=true;
                    //insert only the border client because it start to be visible
                    uint16_t index=0;
                    while(index<loop_size)
                    {
                        temp_border_map->clients.at(index)->reinsertCurrentPlayerOnlyTheBorderClients();
                        index++;
                    }
                }
            }
            temp_border_map->clientsOnBorder--;
            border_map_index++;
        }
    }
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::mapVisiblity_unloadFromTheMap()
{
    if(map==NULL)
        return;
    removeClient();
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::reinsertAllClient()
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("reinsertAllClient() "+std::to_string(public_and_private_informations.public_informations.simplifiedId));
    #endif
    const SIMPLIFIED_PLAYER_ID_TYPE &thisSimplifiedId=public_and_private_informations.public_informations.simplifiedId;
    uint16_t index=0;
    while(index<static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map)->clients.size())
    {
        current_client=static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map)->clients.at(index);
        if(Q_LIKELY(current_client!=this))
        {
            current_client->insertAnotherClient(thisSimplifiedId,this);
            this->insertAnotherClient(current_client);
        }
        index++;
    }
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::reinsertAllClientIncludingBorderClients()
{
    reinsertAllClient();
    reinsertCurrentPlayerOnlyTheBorderClients();
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::reinsertCurrentPlayerOnlyTheBorderClients()
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("reinsertOnlyTheBorderClients() "+std::to_string(public_and_private_informations.public_informations.simplifiedId));
    #endif
    uint8_t border_map_index=0;
    const uint8_t &border_map_loop_size=map->border_map.size();
    while(border_map_index<border_map_loop_size)
    {
        Map_server_MapVisibility_WithBorder_StoreOnSender *temp_border_map=static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map->border_map.at(border_map_index));
        const uint16_t &loop_size=temp_border_map->clients.size();
        uint16_t index=0;
        while(index<loop_size)
        {
            this->insertAnotherClient(current_client->public_and_private_informations.public_informations.simplifiedId,temp_border_map->clients.at(index));
            index++;
        }
        border_map_index++;
    }
}

#ifdef CATCHCHALLENGER_SERVER_VISIBILITY_CLEAR
void MapVisibilityAlgorithm_WithBorder_StoreOnSender::insertAnotherClient(MapVisibilityAlgorithm_WithBorder_StoreOnSender *the_another_player)
{
    insertAnotherClient(the_another_player->public_and_private_informations.public_informations.simplifiedId,the_another_player);
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::insertAnotherClient(const SIMPLIFIED_PLAYER_ID_TYPE &player_id,MapVisibilityAlgorithm_WithBorder_StoreOnSender *the_another_player)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(the_another_player->public_and_private_informations.public_informations.simplifiedId!=player_id)
    {
        errorOutput("insertAnotherClient("+std::to_string(player_id)+
                    ","+the_another_player->map->map_file+
                    ","+std::to_string(the_another_player->x)+
                    ","+std::to_string(the_another_player->y)+
                    ") passed id is wrong!");
        return;
    }
    #endif
    to_send_remove.erase(player_id);
    to_send_move.erase(player_id);
    to_send_reinsert.erase(player_id);

    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
    normalOutput("insertAnotherClient("+std::to_string(player_id)+
                 ","+the_another_player->map->map_file+
                 ","+std::to_string(the_another_player->x)+
                 ","+std::to_string(the_another_player->y)+
                 ")");
    #endif
    to_send_insert[player_id]=the_another_player;
    haveBufferToPurge=true;
}
#endif

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::reinsertAnotherClient(MapVisibilityAlgorithm_WithBorder_StoreOnSender *the_another_player)
{
    reinsertAnotherClient(the_another_player->public_and_private_informations.public_informations.simplifiedId,the_another_player);
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::reinsertAnotherClient(const SIMPLIFIED_PLAYER_ID_TYPE &player_id,MapVisibilityAlgorithm_WithBorder_StoreOnSender *the_another_player)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(the_another_player->public_and_private_informations.public_informations.simplifiedId!=player_id)
    {
        errorOutput("insertAnotherClient("+std::to_string(player_id)+
                    ","+the_another_player->map->map_file+
                    ","+std::to_string(the_another_player->x)+
                    ","+std::to_string(the_another_player->y)+
                    ") passed id is wrong!");
        return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
    normalOutput("reinsertAnotherClient("+std::to_string(player_id)+
                 ","+the_another_player->map->map_file+
                 ","+std::to_string(the_another_player->x)+
                 ","+std::to_string(the_another_player->y)+
                 ")");
    #endif
    to_send_reinsert[player_id]=the_another_player;
    haveBufferToPurge=true;
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::moveAnotherClientWithMap(MapVisibilityAlgorithm_WithBorder_StoreOnSender *the_another_player, const uint8_t &movedUnit, const Direction &direction)
{
    moveAnotherClientWithMap(the_another_player->public_and_private_informations.public_informations.simplifiedId,the_another_player,movedUnit,direction);
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::moveAnotherClientWithMap(const SIMPLIFIED_PLAYER_ID_TYPE &player_id,MapVisibilityAlgorithm_WithBorder_StoreOnSender *the_another_player, const uint8_t &movedUnit, const Direction &direction)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(the_another_player->public_and_private_informations.public_informations.simplifiedId!=player_id)
    {
        errorOutput("insertAnotherClient("+std::to_string(player_id)+
                    ","+the_another_player->map->map_file+
                    ","+std::to_string(the_another_player->x)+
                    ","+std::to_string(the_another_player->y)+
                    ") passed id is wrong!");
        return;
    }
    #endif
    #ifdef CATCHCHALLENGER_SERVER_MAP_DROP_OVER_MOVE
    //already into over move
    if(to_send_insert.find(player_id)!=to_send_insert.cend() || to_send_reinsert.find(player_id)!=to_send_reinsert.cend())
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
        normalOutput("moveAnotherClientWithMap("+std::to_string(player_id)+
                     ","+std::to_string(movedUnit)+
                     ","+MoveOnTheMap::directionToString(direction)+
                     ") to the player: "+std::to_string(public_and_private_informations.public_informations.simplifiedId)+
                     ", already into over move");
        #endif
        //to_send_map_management_remove.remove(player_id); -> what?
        return;//quit now
    }
    //go into over move
    else if(Q_UNLIKELY(
                ((uint32_t)to_send_move.at(player_id).size()*(sizeof(uint8_t)+sizeof(uint8_t))+sizeof(uint8_t))//the size of one move
                >=
                    //size of on insert
                    (uint32_t)GlobalServerData::serverPrivateVariables.sizeofInsertRequest+public_and_private_informations.public_informations.pseudo.size()+1
                ))
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
        normalOutput("moveAnotherClientWithMap("+std::to_string(player_id)+
                     ","+std::to_string(movedUnit)+
                     ","+MoveOnTheMap::directionToString(direction)+
                     ") to the player: "+std::to_string(public_and_private_informations.public_informations.simplifiedId)+
                     ", go into over move");
        #endif
        to_send_move.erase(player_id);
        to_send_reinsert[player_id]=the_another_player;
        return;
    }
    #endif
    #ifdef CATCHCHALLENGER_SERVER_MAP_DROP_STOP_MOVE
    if(to_send_move.find(player_id)!=to_send_move.cend() && !to_send_move.at(player_id).empty())
    {
        switch(to_send_move.at(player_id).back().direction)
        {
            case Direction_look_at_top:
            case Direction_look_at_right:
            case Direction_look_at_bottom:
            case Direction_look_at_left:
                #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
                normalOutput("moveAnotherClientWithMap("+std::to_string(player_id)+
                             ","+std::to_string(to_send_move.at(player_id).back().movedUnit)+
                             ","+MoveOnTheMap::directionToString(direction)+
                             ") to the player: "+std::to_string(public_and_private_informations.public_informations.simplifiedId)+
                             ", compressed move");
                #endif
                to_send_move[player_id].back().direction=direction;
            return;
            default:
            break;
        }
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
    normalOutput("moveAnotherClientWithMap("+std::to_string(player_id)+
                 ","+std::to_string(movedUnit)+
                 ","+MoveOnTheMap::directionToString(direction)+
                 ") to the player: "+std::to_string(public_and_private_informations.public_informations.simplifiedId)+
                 ", normal move");
    #endif
    moveClient_tempMov.movedUnit=movedUnit;
    moveClient_tempMov.direction=direction;
    to_send_move[player_id].push_back(moveClient_tempMov);
    haveBufferToPurge=true;
}

#ifdef CATCHCHALLENGER_SERVER_VISIBILITY_CLEAR
//remove the move/insert
void MapVisibilityAlgorithm_WithBorder_StoreOnSender::removeAnotherClient(const SIMPLIFIED_PLAYER_ID_TYPE &player_id)
{
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(Q_UNLIKELY(to_send_remove.find(player_id)!=to_send_remove.cend()))
    {
        normalOutput("removeAnotherClient() try dual remove");
        return;
    }
    #endif

    to_send_insert.erase(player_id);
    to_send_move.erase(player_id);
    to_send_reinsert.erase(player_id);

    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
    normalOutput("removeAnotherClient("+std::to_string(player_id)+")");
    #endif

    /* Do into the upper class, like MapVisibilityAlgorithm_WithBorder_StoreOnSender
     * #ifdef CATCHCHALLENGER_SERVER_VISIBILITY_CLEAR
    //remove the move/insert
    to_send_map_management_insert.remove(player_id);
    to_send_map_management_move.remove(player_id);
    #endif */
    to_send_remove.insert(player_id);
    haveBufferToPurge=true;
}
#endif

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::extraStop()
{
    haveBufferToPurge=false;
    unloadFromTheMap();//product remove on the map

    to_send_insert.clear();
    to_send_move.clear();
    to_send_remove.clear();
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::purgeBuffer()
{
    if(haveBufferToPurge)
    {
        send_insert();
        send_move();
        send_remove();
        send_reinsert();
        haveBufferToPurge=false;
    }
}

//for the purge buffer
void MapVisibilityAlgorithm_WithBorder_StoreOnSender::send_insert()
{
    if(to_send_insert.empty())
        return;
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("send_insert() of player: "+std::to_string(public_and_private_informations.public_informations.simplifiedId));
    #endif

    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x68;
    posOutput+=1+4;

    ///////////////////////// group by map ///////////////////////
    std::unordered_map<CommonMap*, std::vector<MapVisibilityAlgorithm_WithBorder_StoreOnSender *> >	send_insert_by_map;
    auto i_insert = to_send_insert.begin();
    while(i_insert != to_send_insert.cend())
    {
        send_insert_by_map[i_insert->second->map].push_back(i_insert->second);
        ++i_insert;
    }
    to_send_insert.clear();
    //////////////////////////// insert //////////////////////////
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=send_insert_by_map.size();
        posOutput+=1;
        auto i_insert = send_insert_by_map.begin();
        while (i_insert != send_insert_by_map.cend())
        {
            Map_server_MapVisibility_WithBorder_StoreOnSender* map=static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(i_insert->first);
            const std::vector<MapVisibilityAlgorithm_WithBorder_StoreOnSender *> &clients_list=i_insert->second;
            const uint16_t &list_size=clients_list.size();

            if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
            {
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=map->id;
                posOutput+=1;
            }
            else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
            {
                *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(map->id);
                posOutput+=2;
            }
            else
            {
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(map->id);
                posOutput+=4;
            }
            if(GlobalServerData::serverSettings.max_players<=255)
            {
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=list_size;
                posOutput+=1;
            }
            else
            {
                *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(list_size);
                posOutput+=2;
            }

            uint16_t index=0;
            while(index<list_size)
            {
                const MapVisibilityAlgorithm_WithBorder_StoreOnSender * client=clients_list.at(index);
                #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
                normalOutput("insert player_id: "+std::to_string(client->public_and_private_informations.public_informations.simplifiedId)+
                            ", mapName "+client->map->map_file+
                            ", x: "+std::to_string(client->x)+
                             ", y: "+std::to_string(client->y)+
                             ",direction: "+MoveOnTheMap::directionToString(client->getLastDirection())+
                             ", for player: "+std::to_string(public_and_private_informations.public_informations.simplifiedId)+
                             ", direction | type: "+std::to_string(((uint8_t)client->getLastDirection() | (uint8_t)client->public_and_private_informations.public_informations.type))+
                             ", rawPseudo: "+public_and_private_informations.public_informations.pseudo+
                             ", skinId: "+std::to_string(client->public_and_private_informations.public_informations.skinId)
                             );
                #endif
                if(GlobalServerData::serverSettings.max_players<=255)
                {
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=client->public_and_private_informations.public_informations.simplifiedId;
                    posOutput+=1;
                }
                else
                {
                    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(client->public_and_private_informations.public_informations.simplifiedId);
                    posOutput+=2;
                }
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=client->x;
                posOutput+=1;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=client->y;
                posOutput+=1;
                if(GlobalServerData::serverSettings.dontSendPlayerType)
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=((uint8_t)client->getLastDirection() | (uint8_t)Player_type_normal);
                else
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=((uint8_t)client->getLastDirection() | (uint8_t)client->public_and_private_informations.public_informations.type);
                posOutput+=1;
                if(CommonSettingsServer::commonSettingsServer.forcedSpeed==0)
                {
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=client->public_and_private_informations.public_informations.speed;
                    posOutput+=1;
                }
                //pseudo
                if(!CommonSettingsServer::commonSettingsServer.dontSendPseudo)
                {
                    const std::string &text=client->public_and_private_informations.public_informations.pseudo;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
                    posOutput+=1;
                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
                    posOutput+=text.size();
                }
                //skin
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=client->public_and_private_informations.public_informations.skinId;
                posOutput+=1;

                index++;
            }
            ++i_insert;
        }
    }

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::send_move()
{
    if(to_send_move.empty())
        return;
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("send_move() of player: "+std::to_string(public_and_private_informations.public_informations.simplifiedId));
    #endif

    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x65;
    posOutput+=1+4;

    //////////////////////////// move //////////////////////////
    uint32_t purgeBuffer_list_size_internal=0;
    uint32_t purgeBuffer_indexMovement=0;
    auto i_move = to_send_move.begin();
    if(GlobalServerData::serverSettings.max_players<=255)
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=to_send_move.size();
        posOutput+=1;
    }
    else
    {
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(to_send_move.size());
        posOutput+=2;
    }
    while (i_move != to_send_move.cend())
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
        normalOutput("move player_id: "+std::to_string(i_move->first)+", for player: "+std::to_string(public_and_private_informations.public_informations.simplifiedId));
        #endif
        if(GlobalServerData::serverSettings.max_players<=255)
        {
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=i_move->first;
            posOutput+=1;
        }
        else
        {
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(i_move->first);
            posOutput+=2;
        }

        purgeBuffer_list_size_internal=i_move->second.size();
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=purgeBuffer_list_size_internal;
        posOutput+=1;
        purgeBuffer_indexMovement=0;
        while(purgeBuffer_indexMovement<purgeBuffer_list_size_internal)
        {
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=i_move->second.at(purgeBuffer_indexMovement).movedUnit;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)i_move->second.at(purgeBuffer_indexMovement).direction;
            posOutput+=1;
            purgeBuffer_indexMovement++;
        }
        ++i_move;
    }
    to_send_move.clear();

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::send_remove()
{
    if(to_send_remove.empty())
        return;
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("send_remove() of player: "+std::to_string(public_and_private_informations.public_informations.simplifiedId));
    #endif

    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x66;
    posOutput+=1+4;

    //////////////////////////// remove //////////////////////////
    auto i_remove = to_send_remove.begin();
    if(GlobalServerData::serverSettings.max_players<=255)
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=to_send_remove.size();
        posOutput+=1;
    }
    else
    {
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(to_send_remove.size());
        posOutput+=2;
    }
    while (i_remove != to_send_remove.cend())
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
        normalOutput("player_id to remove: "+std::to_string((uint32_t)(*i_remove))+", for player: "+std::to_string(public_and_private_informations.public_informations.simplifiedId));
        #endif
        if(GlobalServerData::serverSettings.max_players<=255)
        {
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=*i_remove;
            posOutput+=1;
        }
        else
        {
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(*i_remove);
            posOutput+=2;
        }
        ++i_remove;
    }
    to_send_remove.clear();

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::send_reinsert()
{
    if(to_send_reinsert.empty())
        return;
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("send_reinsert() of player: "+std::to_string(public_and_private_informations.public_informations.simplifiedId));
    #endif

    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x63;
    posOutput+=1+4;

    //////////////////////////// re-insert //////////////////////////
    if(GlobalServerData::serverSettings.max_players<=255)
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=to_send_reinsert.size();
        posOutput+=1;
    }
    else
    {
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(to_send_reinsert.size());
        posOutput+=2;
    }

    auto i_insert = to_send_reinsert.begin();
    while (i_insert != to_send_reinsert.cend())
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
        normalOutput("reinsert player_id: "+std::to_string(i_insert->first)+
                     ", x: "+std::to_string(i_insert->second->x)+
                     ", y: "+std::to_string(i_insert->second->y)+
                     ",direction: "+MoveOnTheMap::directionToString(i_insert->second->getLastDirection())+
                     ", for player: "+std::to_string(public_and_private_informations.public_informations.simplifiedId)
                     );
        #endif
        if(GlobalServerData::serverSettings.max_players<=255)
        {
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=i_insert->first;
            posOutput+=1;
        }
        else
        {
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(i_insert->first);
            posOutput+=2;
        }
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=i_insert->second->x;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=i_insert->second->y;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)i_insert->second->getLastDirection();
        posOutput+=1;

        ++i_insert;
    }
    to_send_reinsert.clear();

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

bool MapVisibilityAlgorithm_WithBorder_StoreOnSender::singleMove(const Direction &direction)
{
    old_map=map;
    if(!Client::singleMove(direction))//check of colision disabled because do into LocalClientHandler
        return false;
    if(old_map!=map)
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        normalOutput("singleMove() have change from old map: "+old_map->map_file+", to the new map: "+map->map_file);
        #endif
        mapHaveChanged=true;
        map=old_map;
        unloadFromTheMap();
        map=static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map);
        loadOnTheMap();
    }
    return true;
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::loadOnTheMap()
{
    insertClient();
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(vectorcontainsAtLeastOne(static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map)->clients,this))
    {
        normalOutput("loadOnTheMap() try dual insert into the player list");
        return;
    }
    #endif
    static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map)->clients.push_back(this);
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::unloadFromTheMap()
{
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(!vectorcontainsAtLeastOne(static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map)->clients,this))
    {
        normalOutput("unloadFromTheMap() try remove of the player list, but not found");
        return;
    }
    #endif
    vectorremoveOne(static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map)->clients,this);
    mapVisiblity_unloadFromTheMap();
}

//map slots, transmited by the current ClientNetworkRead
void MapVisibilityAlgorithm_WithBorder_StoreOnSender::put_on_the_map(CommonMap *map,const /*COORD_TYPE*/uint8_t &x,const /*COORD_TYPE*/uint8_t &y,const Orientation &orientation)
{
    Client::put_on_the_map(map,x,y,orientation);
    loadOnTheMap();
}

bool MapVisibilityAlgorithm_WithBorder_StoreOnSender::moveThePlayer(const uint8_t &previousMovedUnit,const Direction &direction)
{
    mapHaveChanged=false;
    //do on server part, because the client send when is blocked to sync the position
    #ifdef CATCHCHALLENGER_SERVER_MAP_DROP_BLOCKED_MOVE
    if(previousMovedUnitBlocked>0)
    {
        if(previousMovedUnit==0)
        {
            if(!Client::moveThePlayer(previousMovedUnit,direction))
            {
                previousMovedUnitBlocked=0;
                return false;
            }
            //send the move to the other client
            moveClient(previousMovedUnitBlocked,direction);
            previousMovedUnitBlocked=0;
            return true;
        }
        else
        {
            errorOutput(std::stringLiteral("previousMovedUnitBlocked>0 but previousMovedUnit!=0"));
            return false;
        }
    }
    Direction temp_last_direction=last_direction;
    switch(last_direction)
    {
        case Direction_move_at_top:
            //move the player on the server map
            if(!Client::moveThePlayer(previousMovedUnit,direction))
                return false;
            if(direction==Direction_look_at_top && !MoveOnTheMap::canGoTo(temp_last_direction,*map,x,y,true))
            {
                //blocked into the wall
                previousMovedUnitBlocked=previousMovedUnit;
                return true;
            }
        break;
        case Direction_move_at_right:
            //move the player on the server map
            if(!Client::moveThePlayer(previousMovedUnit,direction))
                return false;
            if(direction==Direction_look_at_right && !MoveOnTheMap::canGoTo(temp_last_direction,*map,x,y,true))
            {
                //blocked into the wall
                previousMovedUnitBlocked=previousMovedUnit;
                return true;
            }
        break;
        case Direction_move_at_bottom:
            //move the player on the server map
            if(!Client::moveThePlayer(previousMovedUnit,direction))
                return false;
            if(direction==Direction_look_at_bottom && !MoveOnTheMap::canGoTo(temp_last_direction,*map,x,y,true))
            {
                //blocked into the wall
                previousMovedUnitBlocked=previousMovedUnit;
                return true;
            }
        break;
        case Direction_move_at_left:
            //move the player on the server map
            if(!Client::moveThePlayer(previousMovedUnit,direction))
                return false;
            if(direction==Direction_look_at_left && !MoveOnTheMap::canGoTo(temp_last_direction,*map,x,y,true))
            {
                //blocked into the wall
                previousMovedUnitBlocked=previousMovedUnit;
                return true;
            }
        break;
        case Direction_move_at_top:
        case Direction_move_at_right:
        case Direction_move_at_bottom:
        case Direction_move_at_left:
            //move the player on the server map
            if(!Client::moveThePlayer(previousMovedUnit,direction))
                return false;
        break;
        default:
            errorOutput(std::stringLiteral("moveThePlayer(): direction not managed"));
            return false;
        break;
    }
    #else
    //move the player on the server map
    if(!Client::moveThePlayer(previousMovedUnit,direction))
        return false;
    #endif
    //send the move to the other client
    moveClient(previousMovedUnit,direction);
    return true;
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::teleportValidatedTo(CommonMap *map,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation)
{
    bool mapChange=(this->map!=map);
    normalOutput("MapVisibilityAlgorithm_WithBorder_StoreOnSender::teleportValidatedTo() with mapChange: "+std::to_string(mapChange));
    if(mapChange)
        unloadFromTheMap();
    Client::teleportValidatedTo(map,x,y,orientation);
    if(mapChange)
    {
        if(this->map->map_file==map->map_file)
        {
            errorOutput("Map pointer != but map_file is same");
            return;
        }
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        normalOutput("have changed of map for teleportation, old map:"+this->map->map_file+", new map: "+map->map_file);
        #endif
        this->map=static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map);
        loadOnTheMap();
    }
    else
        reinsertClientForOthersOnSameMap();
}
