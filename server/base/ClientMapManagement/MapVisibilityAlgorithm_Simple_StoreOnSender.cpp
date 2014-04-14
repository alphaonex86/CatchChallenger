#include "MapVisibilityAlgorithm_Simple_StoreOnSender.h"
#include "MapVisibilityAlgorithm_WithoutSender.h"
#include "../GlobalServerData.h"
#include "../../VariableServer.h"

using namespace CatchChallenger;

//temp variable for purge buffer
CommonMap* MapVisibilityAlgorithm_Simple_StoreOnSender::old_map;
CommonMap* MapVisibilityAlgorithm_Simple_StoreOnSender::new_map;
bool MapVisibilityAlgorithm_Simple_StoreOnSender::mapHaveChanged;

//temp variable to move on the map
map_management_movement MapVisibilityAlgorithm_Simple_StoreOnSender::moveClient_tempMov;

MapVisibilityAlgorithm_Simple_StoreOnSender::MapVisibilityAlgorithm_Simple_StoreOnSender() :
    to_send_insert(false),
    to_send_reinsert(false)
{
    #ifdef CATCHCHALLENGER_SERVER_MAP_DROP_BLOCKED_MOVE
    previousMovedUnitBlocked=0;
    #endif
}

MapVisibilityAlgorithm_Simple_StoreOnSender::~MapVisibilityAlgorithm_Simple_StoreOnSender()
{
}

void MapVisibilityAlgorithm_Simple_StoreOnSender::insertClient()
{
    Map_server_MapVisibility_Simple_StoreOnSender *temp_map=static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(map);
    if(Q_LIKELY(temp_map->show))
    {
        const int loop_size=temp_map->clients.size();
        if(Q_UNLIKELY(loop_size>=GlobalServerData::serverSettings.mapVisibility.simple.max))
        {
            #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
            emit message(QStringLiteral("insertClient() too many client, hide now, into: %1").arg(map->map_file));
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
            #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
            emit message(QStringLiteral("insertClient() insert the client, into: %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
            #endif
            //insert the new client
            to_send_insert=true;
            to_send_reinsert=false;
            to_send_move.clear();
            temp_map->to_send_remove.removeOne(player_informations->public_and_private_informations.public_informations.simplifiedId);
            temp_map->to_send_insert=true;
        }
    }
    else
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        emit message(QStringLiteral("insertClient() already too many client, into: %1").arg(map->map_file));
        #endif
    }
    //auto insert to know where it have spawn, now in charge of ClientLocalCalcule
    //insertAnotherClient(player_id,current_map,x,y,last_direction,speed);
}

void MapVisibilityAlgorithm_Simple_StoreOnSender::moveClient(const quint8 &movedUnit,const Direction &direction)
{
    Map_server_MapVisibility_Simple_StoreOnSender *temp_map=static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(map);
    if(Q_UNLIKELY(mapHaveChanged))
    {
        #ifdef DEBUG_MESSAGE_CLIENT_MOVE
        emit message(QStringLiteral("map have change, direction: %4: (%1,%2): %3, send at %5 player(s)").arg(x).arg(y).arg(player_informations->public_and_private_informations.public_informations.simplifiedId).arg(MoveOnTheMap::directionToString(direction)).arg(loop_size-1));
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
        if(to_send_insert)
            return;
        #ifdef CATCHCHALLENGER_SERVER_MAP_DROP_OVER_MOVE
        //already into over move
        if(to_send_reinsert)
        {
            #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
            emit message(QStringLiteral("moveAnotherClientWithMap(%1,%2,%3) to the player: %4, already into over move").arg(player_id).arg(movedUnit).arg(MoveOnTheMap::directionToString(direction)).arg(player_informations->public_and_private_informations.public_informations.simplifiedId));
            #endif
            //to_send_map_management_remove.remove(player_id); -> what?
            return;//quit now
        }
        #endif
        //here to know how player is affected
        #ifdef DEBUG_MESSAGE_CLIENT_MOVE
        emit message(QStringLiteral("after %4: (%1,%2): %3, send at %5 player(s)").arg(x).arg(y).arg(player_informations->public_and_private_informations.public_informations.simplifiedId).arg(MoveOnTheMap::directionToString(direction)).arg(loop_size-1));
        #endif

        //normal operation
        if(Q_LIKELY(temp_map->show))
        {
            #ifdef CATCHCHALLENGER_SERVER_MAP_DROP_OVER_MOVE
            //go into over move
            if(Q_UNLIKELY(
                        ((quint32)to_send_move.size()*(sizeof(quint8)+sizeof(quint8))+sizeof(quint8))//the size of one move
                        >=
                            //size of on insert
                            (quint32)GlobalServerData::serverPrivateVariables.sizeofInsertRequest+player_informations->rawPseudo.size()
                        ))
            {
                #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
                emit message(QStringLiteral("moveAnotherClientWithMap(%1,%2,%3) to the player: %4, go into over move").arg(player_id).arg(movedUnit).arg(MoveOnTheMap::directionToString(direction)).arg(player_informations->public_and_private_informations.public_informations.simplifiedId));
                #endif
                to_send_move.clear();
                to_send_reinsert=true;
                return;
            }
            #endif
            #ifdef CATCHCHALLENGER_SERVER_MAP_DROP_STOP_MOVE
            if(!to_send_move.isEmpty())
            {
                switch(to_send_move.last().direction)
                {
                    case Direction_look_at_top:
                    case Direction_look_at_right:
                    case Direction_look_at_bottom:
                    case Direction_look_at_left:
                        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
                        emit message(QStringLiteral("moveAnotherClientWithMap(%1,%2,%3) to the player: %4, compressed move").arg(player_id).arg(to_send_move.value(player_id).last().movedUnit).arg(MoveOnTheMap::directionToString(direction)).arg(player_informations->public_and_private_informations.public_informations.simplifiedId));
                        #endif
                        to_send_move.last().direction=direction;
                    return;
                    default:
                    break;
                }
            }
            #endif
            #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
            emit message(QStringLiteral("moveAnotherClientWithMap(%1,%2,%3) to the player: %4, normal move").arg(player_id).arg(movedUnit).arg(MoveOnTheMap::directionToString(direction)).arg(player_informations->public_and_private_informations.public_informations.simplifiedId));
            #endif
            moveClient_tempMov.movedUnit=movedUnit;
            moveClient_tempMov.direction=direction;
            to_send_move << moveClient_tempMov;
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
    to_send_reinsert=false;
    to_send_move.clear();

    ClientMapManagement::dropAllClients();
}

void MapVisibilityAlgorithm_Simple_StoreOnSender::purgeBuffer()
{
}

void MapVisibilityAlgorithm_Simple_StoreOnSender::removeClient()
{
    Map_server_MapVisibility_Simple_StoreOnSender *temp_map=static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(map);
    int loop_size=temp_map->clients.size();
    if(Q_UNLIKELY(temp_map->show==false))
    {
        if(Q_UNLIKELY(loop_size<=(GlobalServerData::serverSettings.mapVisibility.simple.reshow)))
        {
            #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
            emit message(QStringLiteral("removeClient() client of the map is now under the limit, reinsert all into: %1").arg(map->map_file));
            #endif
            temp_map->show=true;
            //insert all the client because it start to be visible
            if(temp_map->send_drop_all==false)
                temp_map->send_reinsert_all=true;
            else
                temp_map->send_drop_all=false;
        }
        //nothing removed because all clients are already hide
        else
        {
            #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
            emit message(QStringLiteral("removeClient() do nothing because client hiden, into: %1").arg(map->map_file));
            #endif
        }
    }
    else //normal working
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        emit message(QStringLiteral("removeClient() normal work, just remove from client on: %1").arg(map->map_file));
        #endif
        //on client side
        ClientMapManagement::dropAllClients();
        //on reste side
        if(to_send_insert)
            to_send_insert=false;
        else
        {
            to_send_reinsert=false;
            to_send_move.clear();
            temp_map->to_send_remove << player_informations->public_and_private_informations.public_informations.simplifiedId;
        }
    }
}

void MapVisibilityAlgorithm_Simple_StoreOnSender::mapVisiblity_unloadFromTheMap()
{
    removeClient();
}

void MapVisibilityAlgorithm_Simple_StoreOnSender::extraStop()
{
    MapVisibilityAlgorithm_WithoutSender::mapVisibilityAlgorithm_WithoutSender.allClient.removeOne(this);
    unloadFromTheMap();//product remove on the map

    to_send_insert=false;
    to_send_reinsert=false;
    to_send_move.clear();
}

bool MapVisibilityAlgorithm_Simple_StoreOnSender::singleMove(const Direction &direction)
{
    if(!MoveOnTheMap::canGoTo(direction,*map,x,y,false))//check of colision disabled because do into LocalClientHandler
        return false;
    old_map=map;
    new_map=map;
    MoveOnTheMap::move(direction,&new_map,&x,&y);
    if(old_map!=new_map)
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        emit message(QStringLiteral("singleMove() have change from old map: %1, to the new map: %2").arg(old_map->map_file).arg(new_map->map_file));
        #endif
        mapHaveChanged=true;
        map=old_map;
        unloadFromTheMap();
        map=static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(new_map);
        loadOnTheMap();
    }
    return true;
}

void MapVisibilityAlgorithm_Simple_StoreOnSender::loadOnTheMap()
{
    insertClient();
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(map)->clients.contains(this))
    {
        emit message("loadOnTheMap() try dual insert into the player list");
        return;
    }
    #endif
    static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(map)->clients << this;
}

void MapVisibilityAlgorithm_Simple_StoreOnSender::unloadFromTheMap()
{
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(!static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(map)->clients.contains(this))
    {
        emit message("unloadFromTheMap() try remove of the player list, but not found");
        return;
    }
    #endif
    static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(map)->clients.removeOne(this);
    mapVisiblity_unloadFromTheMap();
}

//map slots, transmited by the current ClientNetworkRead
void MapVisibilityAlgorithm_Simple_StoreOnSender::put_on_the_map(CommonMap *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation)
{
    MapVisibilityAlgorithm_WithoutSender::mapVisibilityAlgorithm_WithoutSender.allClient << static_cast<void*>(this);
    MapBasicMove::put_on_the_map(map,x,y,orientation);
    loadOnTheMap();
}

bool MapVisibilityAlgorithm_Simple_StoreOnSender::moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction)
{
    mapHaveChanged=false;
    //do on server part, because the client send when is blocked to sync the position
    #ifdef CATCHCHALLENGER_SERVER_MAP_DROP_BLOCKED_MOVE
    if(previousMovedUnitBlocked>0)
    {
        if(previousMovedUnit==0)
        {
            if(!MapBasicMove::moveThePlayer(previousMovedUnit,direction))
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
            emit error(QStringLiteral("previousMovedUnitBlocked>0 but previousMovedUnit!=0"));
            return false;
        }
    }
    Direction temp_last_direction=last_direction;
    switch(last_direction)
    {
        case Direction_move_at_top:
            //move the player on the server map
            if(!MapBasicMove::moveThePlayer(previousMovedUnit,direction))
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
            if(!MapBasicMove::moveThePlayer(previousMovedUnit,direction))
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
            if(!MapBasicMove::moveThePlayer(previousMovedUnit,direction))
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
            if(!MapBasicMove::moveThePlayer(previousMovedUnit,direction))
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
            if(!MapBasicMove::moveThePlayer(previousMovedUnit,direction))
                return false;
        break;
        default:
            emit error(QStringLiteral("moveThePlayer(): direction not managed"));
            return false;
        break;
    }
    #else
    //move the player on the server map
    if(!MapBasicMove::moveThePlayer(previousMovedUnit,direction))
        return false;
    #endif
    //send the move to the other client
    moveClient(previousMovedUnit,direction);
    return true;
}

void MapVisibilityAlgorithm_Simple_StoreOnSender::teleportValidatedTo(CommonMap *map,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation)
{
    bool mapChange=(this->map!=map);
    emit message(QStringLiteral("MapVisibilityAlgorithm_Simple_StoreOnSender::teleportValidatedTo() with mapChange: %1").arg(mapChange));
    if(mapChange)
        unloadFromTheMap();
    MapBasicMove::teleportValidatedTo(map,x,y,orientation);
    if(mapChange)
    {
        if(this->map->map_file==map->map_file)
        {
            emit error("Map pointer != but map_file is same");
            return;
        }
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        emit message(QStringLiteral("have changed of map for teleportation, old map: %1, new map: %2").arg(this->map->map_file).arg(map->map_file));
        #endif
        this->map=static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(map);
        loadOnTheMap();
    }
    else
        to_send_reinsert=true;
}
