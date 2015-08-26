#include "MapVisibilityAlgorithm_WithBorder_StoreOnSender.h"
#include "../GlobalServerData.h"
#include "../../VariableServer.h"

using namespace CatchChallenger;

int MapVisibilityAlgorithm_WithBorder_StoreOnSender::index;
int MapVisibilityAlgorithm_WithBorder_StoreOnSender::loop_size;
MapVisibilityAlgorithm_WithBorder_StoreOnSender *MapVisibilityAlgorithm_WithBorder_StoreOnSender::current_client;

//temp variable for purge buffer
QByteArray MapVisibilityAlgorithm_WithBorder_StoreOnSender::purgeBuffer_outputData;
int MapVisibilityAlgorithm_WithBorder_StoreOnSender::purgeBuffer_index;
int MapVisibilityAlgorithm_WithBorder_StoreOnSender::purgeBuffer_list_size;
int MapVisibilityAlgorithm_WithBorder_StoreOnSender::purgeBuffer_list_size_internal;
int MapVisibilityAlgorithm_WithBorder_StoreOnSender::purgeBuffer_indexMovement;
map_management_move MapVisibilityAlgorithm_WithBorder_StoreOnSender::purgeBuffer_move;
std::unordered_map<SIMPLIFIED_PLAYER_ID_TYPE, std::vector<map_management_movement> >::const_iterator MapVisibilityAlgorithm_WithBorder_StoreOnSender::i_move;
std::unordered_map<SIMPLIFIED_PLAYER_ID_TYPE, std::vector<map_management_movement> >::const_iterator MapVisibilityAlgorithm_WithBorder_StoreOnSender::i_move_end;
std::unordered_map<SIMPLIFIED_PLAYER_ID_TYPE, MapVisibilityAlgorithm_WithBorder_StoreOnSender *>::const_iterator MapVisibilityAlgorithm_WithBorder_StoreOnSender::i_insert;
std::unordered_map<SIMPLIFIED_PLAYER_ID_TYPE, MapVisibilityAlgorithm_WithBorder_StoreOnSender *>::const_iterator MapVisibilityAlgorithm_WithBorder_StoreOnSender::i_insert_end;
Std::unordered_Set<SIMPLIFIED_PLAYER_ID_TYPE>::const_iterator MapVisibilityAlgorithm_WithBorder_StoreOnSender::i_remove;
Std::unordered_Set<SIMPLIFIED_PLAYER_ID_TYPE>::const_iterator MapVisibilityAlgorithm_WithBorder_StoreOnSender::i_remove_end;
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
        loop_size=temp_map->clients.size();
        if(Q_LIKELY(temp_map->showWithBorder))
        {
            if(Q_UNLIKELY((loop_size+temp_map->clientsOnBorder)>=GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder))
            {
                #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
                normalOutput(std::stringLiteral("insertClient() too many client, hide now, into: %1").arg(map->map_file));
                #endif
                temp_map->showWithBorder=false;
                //drop all show client because it have excess the limit
                //drop on all client
                if(Q_UNLIKELY(loop_size>=GlobalServerData::serverSettings.mapVisibility.withBorder.max))
                {}
                else
                {
                    index=0;
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
            normalOutput(std::stringLiteral("insertClient() too many client, hide now, into: %1").arg(map->map_file));
            #endif
            temp_map->show=false;
            //drop all show client because it have excess the limit
            //drop on all client
            index=0;
            while(index<loop_size)
            {
                temp_map->clients.at(index)->dropAllClients();
                index++;
            }
        }
        else//why else dropped?
        {
            #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
            normalOutput(std::stringLiteral("insertClient() insert the client, into: %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
            #endif
            //insert the new client
            const SIMPLIFIED_PLAYER_ID_TYPE &thisSimplifiedId=public_and_private_informations.public_informations.simplifiedId;
            index=0;
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
        normalOutput(std::stringLiteral("insertClient() already too many client, into: %1").arg(map->map_file));
        #endif
    }
    //auto insert to know where it have spawn, now in charge of ClientLocalCalcule
    //insertAnotherClient(player_id,current_map,x,y,last_direction,speed);

    //border map
    {
        int border_map_index=0;
        const int &border_map_loop_size=map->border_map.size();
        while(border_map_index<border_map_loop_size)
        {
            Map_server_MapVisibility_WithBorder_StoreOnSender *temp_border_map=static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map->border_map.at(border_map_index));
            loop_size=temp_border_map->clients.size();
            //insert border player on current player
            if(temp_map->showWithBorder)
            {
                index=0;
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
                    normalOutput(std::stringLiteral("insertClient() too many client, hide now, into: %1").arg(map->map_file));
                    #endif
                    temp_border_map->showWithBorder=false;
                    //drop all show client because it have excess the limit
                    //drop on all client
                    index=0;
                    while(index<loop_size)
                    {
                        temp_border_map->clients.at(index)->dropAllBorderClients();
                        index++;
                    }
                }
                else
                {
                    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
                    normalOutput(std::stringLiteral("insertClient() insert the client, into: %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    #endif
                    const SIMPLIFIED_PLAYER_ID_TYPE &thisSimplifiedId=public_and_private_informations.public_informations.simplifiedId;
                    index=0;
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
    loop_size=temp_map->clients.size();
    if(Q_UNLIKELY(mapHaveChanged))
    {
        #ifdef DEBUG_MESSAGE_CLIENT_MOVE
        normalOutput(std::stringLiteral("map have change, direction: %4: (%1,%2): %3, send at %5 player(s)").arg(x).arg(y).arg(public_and_private_informations.public_informations.simplifiedId).arg(MoveOnTheMap::directionToString(direction)).arg(loop_size-1));
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
        normalOutput(std::stringLiteral("after %4: (%1,%2): %3, send at %5 player(s)").arg(x).arg(y).arg(public_and_private_informations.public_informations.simplifiedId).arg(MoveOnTheMap::directionToString(direction)).arg(loop_size-1));
        #endif

        //normal operation
        if(Q_LIKELY(temp_map->show))
        {
            index=0;
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
    int border_map_index=0;
    int border_map_loop_size=map->border_map.size();
    while(border_map_index<border_map_loop_size)
    {
        Map_server_MapVisibility_WithBorder_StoreOnSender *temp_border_map=static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map->border_map.at(border_map_index));
        loop_size=temp_border_map->clients.size();
        if(temp_border_map->showWithBorder)
        {
            index=0;
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
        normalOutput(std::stringLiteral("reinsertClientForOthers() skip because not show").arg(map->map_file));
        #endif
        return;
    }
    int index;
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(std::stringLiteral("reinsertClientForOthers() normal work, just remove from client on: %1").arg(map->map_file));
    #endif
    /* useless because the insert will overwrite the position */
    const SIMPLIFIED_PLAYER_ID_TYPE &thisSimplifiedId=public_and_private_informations.public_informations.simplifiedId;
    index=0;
    while(index<loop_size)
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
    loop_size=temp_map->clients.size();
    if(Q_UNLIKELY(temp_map->show==false))
    {
        if(Q_UNLIKELY(loop_size<=(GlobalServerData::serverSettings.mapVisibility.withBorder.reshow)))
        {
            #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
            normalOutput(std::stringLiteral("removeClient() client of the map is now under the limit, reinsert all into: %1").arg(map->map_file));
            #endif
            temp_map->show=true;
            //insert all the client because it start to be visible
            if(Q_UNLIKELY((loop_size+temp_map->clientsOnBorder)<=(GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder)))
            {
                temp_map->showWithBorder=true;
                index=0;
                while(index<loop_size)
                {
                    temp_map->clients.at(index)->reinsertAllClientIncludingBorderClients();
                    index++;
                }
            }
            else
            {
                index=0;
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
            normalOutput(std::stringLiteral("removeClient() do nothing because client hiden, into: %1").arg(map->map_file));
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
                normalOutput(std::stringLiteral("removeClient() client of the map is now under the limit, reinsert all into: %1").arg(map->map_file));
                #endif
                temp_map->showWithBorder=true;
                //insert only the border client because it start to be visible
                index=0;
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
                normalOutput(std::stringLiteral("removeClient() do nothing because client hiden, into: %1").arg(map->map_file));
                #endif
            }
        }
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        normalOutput(std::stringLiteral("removeClient() normal work, just remove from client on: %1").arg(map->map_file));
        #endif
        index=0;
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
        int border_map_index=0;
        const int &border_map_loop_size=map->border_map.size();
        while(border_map_index<border_map_loop_size)
        {
            Map_server_MapVisibility_WithBorder_StoreOnSender *temp_border_map=static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map->border_map.at(border_map_index));
            loop_size=temp_border_map->clients.size();
            //remove border client on this
            if(Q_LIKELY(temp_map->showWithBorder==true))
            {
                index=0;
                while(index<loop_size)
                {
                    this->removeAnotherClient(temp_border_map->clients.at(index)->public_and_private_informations.public_informations.simplifiedId);
                    index++;
                }
            }
            //remove this ont border client
            if(Q_LIKELY(temp_border_map->showWithBorder==true))
            {
                index=0;
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
                    index=0;
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
    removeClient();
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::reinsertAllClient()
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(std::stringLiteral("reinsertAllClient() %1)").arg(public_and_private_informations.public_informations.simplifiedId));
    #endif
    const SIMPLIFIED_PLAYER_ID_TYPE &thisSimplifiedId=public_and_private_informations.public_informations.simplifiedId;
    index=0;
    while(index<loop_size)
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
    normalOutput(std::stringLiteral("reinsertOnlyTheBorderClients() %1)").arg(public_and_private_informations.public_informations.simplifiedId));
    #endif
    int border_map_index=0;
    int border_map_loop_size=map->border_map.size();
    while(border_map_index<border_map_loop_size)
    {
        Map_server_MapVisibility_WithBorder_StoreOnSender *temp_border_map=static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map->border_map.at(border_map_index));
        loop_size=temp_border_map->clients.size();
        index=0;
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
        errorOutput(std::stringLiteral("insertAnotherClient(%1,%2,%3,%4) passed id is wrong!").arg(player_id).arg(the_another_player->map->map_file).arg(the_another_player->x).arg(the_another_player->y));
        return;
    }
    #endif
    to_send_remove.remove(player_id);
    to_send_move.remove(player_id);
    to_send_reinsert.remove(player_id);

    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
    normalOutput(std::stringLiteral("insertAnotherClient(%1,%2,%3,%4)").arg(player_id).arg(the_another_player->map->map_file).arg(the_another_player->x).arg(the_another_player->y));
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
        errorOutput(std::stringLiteral("insertAnotherClient(%1,%2,%3,%4) passed id is wrong!").arg(player_id).arg(the_another_player->map->map_file).arg(the_another_player->x).arg(the_another_player->y));
        return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
    normalOutput(std::stringLiteral("reinsertAnotherClient(%1,%2,%3,%4)").arg(player_id).arg(the_another_player->map->map_file).arg(the_another_player->x).arg(the_another_player->y));
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
        errorOutput(std::stringLiteral("insertAnotherClient(%1,%2,%3,%4) passed id is wrong!").arg(player_id).arg(the_another_player->map->map_file).arg(the_another_player->x).arg(the_another_player->y));
        return;
    }
    #endif
    #ifdef CATCHCHALLENGER_SERVER_MAP_DROP_OVER_MOVE
    //already into over move
    if(to_send_insert.contains(player_id) || to_send_reinsert.contains(player_id))
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
        normalOutput(std::stringLiteral("moveAnotherClientWithMap(%1,%2,%3) to the player: %4, already into over move").arg(player_id).arg(movedUnit).arg(MoveOnTheMap::directionToString(direction)).arg(public_and_private_informations.public_informations.simplifiedId));
        #endif
        //to_send_map_management_remove.remove(player_id); -> what?
        return;//quit now
    }
    //go into over move
    else if(Q_UNLIKELY(
                ((uint32_t)to_send_move.value(player_id).size()*(sizeof(uint8_t)+sizeof(uint8_t))+sizeof(uint8_t))//the size of one move
                >=
                    //size of on insert
                    (uint32_t)GlobalServerData::serverPrivateVariables.sizeofInsertRequest+rawPseudo.size()
                ))
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
        normalOutput(std::stringLiteral("moveAnotherClientWithMap(%1,%2,%3) to the player: %4, go into over move").arg(player_id).arg(movedUnit).arg(MoveOnTheMap::directionToString(direction)).arg(public_and_private_informations.public_informations.simplifiedId));
        #endif
        to_send_move.remove(player_id);
        to_send_reinsert[player_id]=the_another_player;
        return;
    }
    #endif
    #ifdef CATCHCHALLENGER_SERVER_MAP_DROP_STOP_MOVE
    if(to_send_move.contains(player_id) && !to_send_move.value(player_id).isEmpty())
    {
        switch(to_send_move.value(player_id).last().direction)
        {
            case Direction_look_at_top:
            case Direction_look_at_right:
            case Direction_look_at_bottom:
            case Direction_look_at_left:
                #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
                normalOutput(std::stringLiteral("moveAnotherClientWithMap(%1,%2,%3) to the player: %4, compressed move").arg(player_id).arg(to_send_move.value(player_id).last().movedUnit).arg(MoveOnTheMap::directionToString(direction)).arg(public_and_private_informations.public_informations.simplifiedId));
                #endif
                to_send_move[player_id].last().direction=direction;
            return;
            default:
            break;
        }
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
    normalOutput(std::stringLiteral("moveAnotherClientWithMap(%1,%2,%3) to the player: %4, normal move").arg(player_id).arg(movedUnit).arg(MoveOnTheMap::directionToString(direction)).arg(public_and_private_informations.public_informations.simplifiedId));
    #endif
    moveClient_tempMov.movedUnit=movedUnit;
    moveClient_tempMov.direction=direction;
    to_send_move[player_id] << moveClient_tempMov;
    haveBufferToPurge=true;
}

#ifdef CATCHCHALLENGER_SERVER_VISIBILITY_CLEAR
//remove the move/insert
void MapVisibilityAlgorithm_WithBorder_StoreOnSender::removeAnotherClient(const SIMPLIFIED_PLAYER_ID_TYPE &player_id)
{
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(Q_UNLIKELY(to_send_remove.contains(player_id)))
    {
        normalOutput("removeAnotherClient() try dual remove");
        return;
    }
    #endif

    to_send_insert.remove(player_id);
    to_send_move.remove(player_id);
    to_send_reinsert.remove(player_id);

    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
    normalOutput(std::stringLiteral("removeAnotherClient(%1)").arg(player_id));
    #endif

    /* Do into the upper class, like MapVisibilityAlgorithm_WithBorder_StoreOnSender
     * #ifdef CATCHCHALLENGER_SERVER_VISIBILITY_CLEAR
    //remove the move/insert
    to_send_map_management_insert.remove(player_id);
    to_send_map_management_move.remove(player_id);
    #endif */
    to_send_remove << player_id;
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
    if(to_send_insert.size()==0)
        return;
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(std::stringLiteral("send_insert() of player: %4").arg(public_and_private_informations.public_informations.simplifiedId));
    #endif

    purgeBuffer_outputData.clear();
    QDataStream out(&purgeBuffer_outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);

    ///////////////////////// group by map ///////////////////////
    std::unordered_map<CommonMap*, std::vector<MapVisibilityAlgorithm_WithBorder_StoreOnSender *> >	send_insert_by_map;
    i_insert = to_send_insert.constBegin();
    i_insert_end = to_send_insert.constEnd();
    while (i_insert != i_insert_end)
    {
        send_insert_by_map[i_insert.value()->map] << i_insert.value();
        ++i_insert;
    }
    to_send_insert.clear();
    //////////////////////////// insert //////////////////////////
    {
        out << (uint8_t)send_insert_by_map.size();
        static std::unordered_map<CommonMap*, std::vector<MapVisibilityAlgorithm_WithBorder_StoreOnSender *> >::const_iterator i_insert;
        static std::unordered_map<CommonMap*, std::vector<MapVisibilityAlgorithm_WithBorder_StoreOnSender *> >::const_iterator i_insert_end;
        i_insert = send_insert_by_map.constBegin();
        i_insert_end = send_insert_by_map.constEnd();
        while (i_insert != i_insert_end)
        {
            Map_server_MapVisibility_WithBorder_StoreOnSender* map=static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(i_insert.key());
            const std::vector<MapVisibilityAlgorithm_WithBorder_StoreOnSender *> &clients_list=i_insert.value();
            const int &list_size=clients_list.size();

            if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
                out << (uint8_t)map->id;
            else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
                out << (uint16_t)map->id;
            else
                out << (uint32_t)map->id;
            if(GlobalServerData::serverSettings.max_players<=255)
                out << (uint8_t)list_size;
            else
                out << (uint16_t)list_size;

            uint16_t index=0;
            while(index<list_size)
            {
                const MapVisibilityAlgorithm_WithBorder_StoreOnSender * client=clients_list.at(index);
                #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
                normalOutput(
                    std::stringLiteral("insert player_id: %1, mapName %2, x: %3, y: %4,direction: %5, for player: %6, direction | type: %7, rawPseudo: %8, skinId: %9")
                    .arg(client->public_and_private_informations.public_informations.simplifiedId)
                    .arg(client->map->map_file)
                    .arg(client->x)
                    .arg(client->y)
                    .arg(MoveOnTheMap::directionToString(client->getLastDirection()))
                    .arg(public_and_private_informations.public_informations.simplifiedId)
                    .arg(((uint8_t)client->getLastDirection() | (uint8_t)client->public_and_private_informations.public_informations.type))
                    .arg(std::string(client->rawPseudo.toHex()))
                    .arg(client->public_and_private_informations.public_informations.skinId)
                     );
                #endif
                if(GlobalServerData::serverSettings.max_players<=255)
                    out << (uint8_t)client->public_and_private_informations.public_informations.simplifiedId;
                else
                    out << (uint16_t)client->public_and_private_informations.public_informations.simplifiedId;
                out << (COORD_TYPE)client->x;
                out << (COORD_TYPE)client->y;
                if(GlobalServerData::serverSettings.dontSendPlayerType)
                    out << (uint8_t)((uint8_t)client->getLastDirection() | (uint8_t)Player_type_normal);
                else
                    out << (uint8_t)((uint8_t)client->getLastDirection() | (uint8_t)client->public_and_private_informations.public_informations.type);
                if(CommonSettingsServer::commonSettingsServer.forcedSpeed==0)
                    out << client->public_and_private_informations.public_informations.speed;
                //pseudo
                if(!CommonSettingsServer::commonSettingsServer.dontSendPseudo)
                {
                    purgeBuffer_outputData+=client->rawPseudo;
                    out.device()->seek(out.device()->pos()+client->rawPseudo.size());
                }
                //skin
                out << client->public_and_private_informations.public_informations.skinId;

                index++;
            }
            ++i_insert;
        }
    }

    sendPacket(0xC0,purgeBuffer_outputData.constData(),purgeBuffer_outputData.size());
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::send_move()
{
    if(to_send_move.size()==0)
        return;
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(std::stringLiteral("send_move() of player: %4").arg(public_and_private_informations.public_informations.simplifiedId));
    #endif

    purgeBuffer_outputData.clear();
    QDataStream out(&purgeBuffer_outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);

    //////////////////////////// move //////////////////////////
    purgeBuffer_list_size_internal=0;
    purgeBuffer_indexMovement=0;
    i_move = to_send_move.constBegin();
    i_move_end = to_send_move.constEnd();
    if(GlobalServerData::serverSettings.max_players<=255)
        out << (uint8_t)to_send_move.size();
    else
        out << (uint16_t)to_send_move.size();
    while (i_move != i_move_end)
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
        normalOutput(
            std::stringLiteral("move player_id: %1, for player: %2")
            .arg(i_move.key())
            .arg(public_and_private_informations.public_informations.simplifiedId)
             );
        #endif
        if(GlobalServerData::serverSettings.max_players<=255)
            out << (uint8_t)i_move.key();
        else
            out << (uint16_t)i_move.key();

        purgeBuffer_list_size_internal=i_move.value().size();
        out << (uint8_t)purgeBuffer_list_size_internal;
        purgeBuffer_indexMovement=0;
        while(purgeBuffer_indexMovement<purgeBuffer_list_size_internal)
        {
            out << i_move.value().at(purgeBuffer_indexMovement).movedUnit;
            out << (uint8_t)i_move.value().at(purgeBuffer_indexMovement).direction;
            purgeBuffer_indexMovement++;
        }
        ++i_move;
    }
    to_send_move.clear();

    sendPacket(0xC7,purgeBuffer_outputData.constData(),purgeBuffer_outputData.size());
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::send_remove()
{
    if(to_send_remove.size()==0)
        return;
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(std::stringLiteral("send_remove() of player: %4").arg(public_and_private_informations.public_informations.simplifiedId));
    #endif

    purgeBuffer_outputData.clear();
    QDataStream out(&purgeBuffer_outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);

    //////////////////////////// remove //////////////////////////
    i_remove = to_send_remove.constBegin();
    i_remove_end = to_send_remove.constEnd();
    if(GlobalServerData::serverSettings.max_players<=255)
        out << (uint8_t)to_send_remove.size();
    else
        out << (uint16_t)to_send_remove.size();
    while (i_remove != i_remove_end)
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
        normalOutput(
            std::stringLiteral("player_id to remove: %1, for player: %2")
            .arg((uint32_t)(*i_remove))
            .arg(public_and_private_informations.public_informations.simplifiedId)
             );
        #endif
        if(GlobalServerData::serverSettings.max_players<=255)
            out << (uint8_t)*i_remove;
        else
            out << (uint16_t)*i_remove;
        ++i_remove;
    }
    to_send_remove.clear();

    sendPacket(0xC8,purgeBuffer_outputData.constData(),purgeBuffer_outputData.size());
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::send_reinsert()
{
    if(to_send_reinsert.size()==0)
        return;
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(std::stringLiteral("send_reinsert() of player: %4").arg(public_and_private_informations.public_informations.simplifiedId));
    #endif

    purgeBuffer_outputData.clear();
    QDataStream out(&purgeBuffer_outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);

    //////////////////////////// re-insert //////////////////////////
    if(GlobalServerData::serverSettings.max_players<=255)
        out << (uint8_t)to_send_reinsert.size();
    else
        out << (uint16_t)to_send_reinsert.size();

    i_insert = to_send_reinsert.constBegin();
    i_insert_end = to_send_reinsert.constEnd();
    while (i_insert != i_insert_end)
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
        normalOutput(
            std::stringLiteral("reinsert player_id: %1, x: %2, y: %3,direction: %4, for player: %5")
            .arg(i_insert.key())
            .arg(i_insert.value()->x)
            .arg(i_insert.value()->y)
            .arg(MoveOnTheMap::directionToString(i_insert.value()->getLastDirection()))
            .arg(public_and_private_informations.public_informations.simplifiedId)
             );
        #endif
        if(GlobalServerData::serverSettings.max_players<=255)
            out << (uint8_t)i_insert.key();
        else
            out << (uint16_t)i_insert.key();
        out << i_insert.value()->x;
        out << i_insert.value()->y;
        out << (uint8_t)i_insert.value()->getLastDirection();

        ++i_insert;
    }
    to_send_reinsert.clear();

    sendPacket(0xC5,purgeBuffer_outputData.constData(),purgeBuffer_outputData.size());
}

bool MapVisibilityAlgorithm_WithBorder_StoreOnSender::singleMove(const Direction &direction)
{
    old_map=map;
    if(!Client::singleMove(direction))//check of colision disabled because do into LocalClientHandler
        return false;
    if(old_map!=map)
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        normalOutput(std::stringLiteral("singleMove() have change from old map: %1, to the new map: %2").arg(old_map->map_file).arg(map->map_file));
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
    if(static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map)->clients.contains(this))
    {
        normalOutput("loadOnTheMap() try dual insert into the player list");
        return;
    }
    #endif
    static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map)->clients << this;
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSender::unloadFromTheMap()
{
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(!static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map)->clients.contains(this))
    {
        normalOutput("unloadFromTheMap() try remove of the player list, but not found");
        return;
    }
    #endif
    static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map)->clients.removeOne(this);
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
    normalOutput(std::stringLiteral("MapVisibilityAlgorithm_WithBorder_StoreOnSender::teleportValidatedTo() with mapChange: %1").arg(mapChange));
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
        normalOutput(std::stringLiteral("have changed of map for teleportation, old map: %1, new map: %2").arg(this->map->map_file).arg(map->map_file));
        #endif
        this->map=static_cast<Map_server_MapVisibility_WithBorder_StoreOnSender*>(map);
        loadOnTheMap();
    }
    else
        reinsertClientForOthersOnSameMap();
}
