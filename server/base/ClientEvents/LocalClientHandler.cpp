#include "../Client.hpp"
#include "../PreparedDBQuery.hpp"
#include "../GlobalServerData.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../MapManagement/Map_server_MapVisibility_Simple_StoreOnSender.hpp"
#include "../ClientList.hpp"
#include "../GlobalServerData.hpp"
#include <cstring>

using namespace CatchChallenger;

bool Client::checkCollision()
{
    if(!MoveOnTheMap::isWalkable(Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.at(mapIndex),x,y))
    {
        errorOutput("can't wall at: "+std::to_string(x)+","+std::to_string(y)+" on map: "+std::to_string(mapIndex));
        return false;
    }
    else
        return true;
}

void Client::removeFromClan()
{
    clanChangeWithoutDb(0);
}

Player_private_and_public_informations &Client::get_public_and_private_informations()
{
    return public_and_private_informations;
}

const Player_private_and_public_informations &Client::get_public_and_private_informations_ro() const
{
    return public_and_private_informations;
}

std::string Client::directionToStringToSave(const Direction &direction)
{
    switch(direction)
    {
        case Direction_look_at_top:
        case Direction_move_at_top:
            return "1";
        break;
        case Direction_look_at_right:
        case Direction_move_at_right:
            return "2";
        break;
        case Direction_look_at_bottom:
        case Direction_move_at_bottom:
            return "3";
        break;
        case Direction_look_at_left:
        case Direction_move_at_left:
            return "4";
        break;
        default:
        break;
    }
    return "3";
}

std::string Client::orientationToStringToSave(const Orientation &orientation)
{
    switch(orientation)
    {
        case Orientation_top:
            return "top";
        break;
        case Orientation_bottom:
            return "bottom";
        break;
        case Orientation_right:
            return "right";
        break;
        case Orientation_left:
            return "left";
        break;
        default:
        break;
    }
    return "bottom";
}

/* why do that's here?
 * Because the ClientMapManagement can be totaly satured by the square complexity
 * that's allow to continue the player to connect and play
 * the overhead for the network it just at the connexion */
void Client::put_on_the_map(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation)
{
    Map_server_MapVisibility_Simple_StoreOnSender &map=Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.at(mapIndex);
    MapBasicMove::put_on_the_map(mapIndex,x,y,orientation);
    insertClientOnMap(Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list[mapIndex]);

/* now send via Client::characterIsRightSendData() and now player on map is independant algo   uint32_t posOutput=0;

    //packet code
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x6B;
    posOutput=1+4;

    //map list size, only one because packet only for this player
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
    posOutput+=1;

    //send the current map of the player
    if(Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.size()<=255)
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(map.id);
        posOutput+=1;
    }
    else
    {
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(map.id);
        posOutput+=2;
    }
    //send only for this player
    if(GlobalServerData::serverSettings.max_players<=255)
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(public_and_private_informations.public_informations.simplifiedId_forMap);
        posOutput+=1;
    }
    else
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x00;
        posOutput+=1;
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(public_and_private_informations.public_informations.simplifiedId_forMap);
        posOutput+=2;
    }
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=x;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=y;
    posOutput+=1;
    if(GlobalServerData::serverSettings.dontSendPlayerType && public_and_private_informations.public_informations.type==Player_type_premium)
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=uint8_t((uint8_t)orientation | (uint8_t)Player_type_normal);
    else
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=uint8_t((uint8_t)orientation | (uint8_t)public_and_private_informations.public_informations.type);
    posOutput+=1;

    if(!CommonSettingsServer::commonSettingsServer.dontSendPseudo)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(public_and_private_informations.public_informations.pseudo.size()>255)
        {
            std::cerr << "preload_other() public_and_private_informations.public_informations.pseudo size > 255 abort" << std::endl;
            abort();
        }
        #endif
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(public_and_private_informations.public_informations.pseudo.size());
        posOutput+=1;
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,public_and_private_informations.public_informations.pseudo.data(),public_and_private_informations.public_informations.pseudo.size());
        posOutput+=public_and_private_informations.public_informations.pseudo.size();
    }

    //skin id
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=public_and_private_informations.public_informations.skinId;
    posOutput+=1;

    //the following monster id to show
    if(public_and_private_informations.playerMonster.empty())
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=0;
    else
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(public_and_private_informations.playerMonster.front().monster);
    posOutput+=2;

    //set the dynamic size
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);

    if(!sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput))
        return;*/

    //load the first time the random number list
    generateRandomNumber();

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(index_connected_player==SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
    {
        std::cerr << "index_connected_player is not set (abort)" << std::endl;
        abort();
    }
    #endif
    if(public_and_private_informations.clan>0)
        sendClanInfo();

    updateCanDoFight();
    if(getAbleToFight())
        botFightCollision(map,x,y);
    else if(haveMonsters())
        checkLoose();

    unsigned int index=0;
    while(index<GlobalServerData::serverPrivateVariables.server_message.size())
    {
        receiveSystemText(GlobalServerData::serverPrivateVariables.server_message.at(index));
        index++;
    }
}

void Client::createMemoryClan()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(index_connected_player==SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
    {
        std::cout << "not connected player, abort" << std::endl;
        abort();
    }
    #endif

    if(GlobalServerData::serverPrivateVariables.clanList.find(public_and_private_informations.clan)==GlobalServerData::serverPrivateVariables.clanList.end())
    {
        Clan clan;
        clan.cash=0;
        clan.connected_players.push_back(index_connected_player);
        GlobalServerData::serverPrivateVariables.clanList[public_and_private_informations.clan]=clan;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(GlobalServerData::serverPrivateVariables.cityStatusListReverse.find(clan->clanId)!=GlobalServerData::serverPrivateVariables.cityStatusListReverse.end())
            clan->capturedCity=GlobalServerData::serverPrivateVariables.cityStatusListReverse.at(clan->clanId);
        #endif
    }
    else
        GlobalServerData::serverPrivateVariables.clanList[public_and_private_informations.clan].connected_players.push_back(index_connected_player);
}

void Client::addCash(const uint64_t &cash, const bool &forceSave)
{
    if(cash==0 && !forceSave)
        return;
    public_and_private_informations.cash+=cash;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_cash.asyncWrite({
                std::to_string(public_and_private_informations.cash),
                std::to_string(character_id)
                });
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}

void Client::removeCash(const uint64_t &cash)
{
    if(cash==0)
        return;
    public_and_private_informations.cash-=cash;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_cash.asyncWrite({
                std::to_string(public_and_private_informations.cash),
                std::to_string(character_id)
                });
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}

void Client::setEvent(const uint8_t &event, const uint8_t &new_value)
{
    const uint8_t &event_value=GlobalServerData::serverPrivateVariables.events.at(event);
    const auto &now = sFrom1970();

    //send the network reply
    ProtocolParsingBase::tempBigBufferForOutput[0x00]=0xE2;
    ProtocolParsingBase::tempBigBufferForOutput[0x02]=event;
    ProtocolParsingBase::tempBigBufferForOutput[0x03]=new_value;

    unsigned int index=0;
    while(index<ClientList::list->size())
    {
        if(ClientList::list->empty(index))
        {
            Client &c=ClientList::list->rw(index);
            c.sendNewEvent(ProtocolParsingBase::tempBigBufferForOutput,4);
            c.addEventInQueue(event,event_value,now);
        }
        index++;
    }
    GlobalServerData::serverPrivateVariables.events[event]=new_value;
}

void Client::addEventInQueue(const uint8_t &event, const uint8_t &event_value, const uint64_t &currentDateTime)
{
    if(oldEvents.oldEventList.size()==0)
        oldEvents.time=currentDateTime;
    OldEvents::OldEventEntry entry;
    entry.event=event;
    entry.eventValue=event_value;
    oldEvents.oldEventList.push_back(entry);
}

void Client::removeFirstEventInQueue()
{
    if(oldEvents.oldEventList.size()==0)
    {
        errorOutput("Not event in queue to remove");
        return;
    }
    oldEvents.oldEventList.erase(oldEvents.oldEventList.begin());
    if(oldEvents.oldEventList.size()>0)
    {
        const auto &now = sFrom1970();
        oldEvents.time=now;
    }
}

void Client::receiveTeleportTo(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const /*COORD_TYPE*/uint8_t &x,const /*COORD_TYPE*/uint8_t &y,const Orientation &orientation)
{
    teleportTo(mapIndex,x,y,orientation);
}

void Client::teleportValidatedTo(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const /*COORD_TYPE*/uint8_t &x,const /*COORD_TYPE*/uint8_t &y,const Orientation &orientation)
{
    normalOutput("teleportValidatedTo("+std::to_string(mapIndex)+
                                    ","+std::to_string(x)+
                                    ","+std::to_string(y)+
                                    ","+std::to_string((uint8_t)orientation)+")");
    MapBasicMove::teleportValidatedTo(mapIndex,x,y,orientation);
    if(GlobalServerData::serverSettings.positionTeleportSync)
        savePosition();
    bool mapChange=(this->mapIndex!=mapIndex);
    normalOutput("MapVisibilityAlgorithm_Simple_StoreOnSender::teleportValidatedTo() with mapChange: "+std::to_string(mapChange));
    if(mapChange)
        removeClientOnMap(Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.at(mapIndex));
}

Direction Client::lookToMove(const Direction &direction)
{
    switch(direction)
    {
        case Direction_look_at_top:
            return Direction_move_at_top;
        break;
        case Direction_look_at_right:
            return Direction_move_at_right;
        break;
        case Direction_look_at_bottom:
            return Direction_move_at_bottom;
        break;
        case Direction_look_at_left:
            return Direction_move_at_left;
        break;
        default:
        return direction;
    }
}

//return nullptr if can't move in this direction
const Map_server_MapVisibility_Simple_StoreOnSender * Client::mapAndPosIfMoveInLookingDirectionJumpColision(COORD_TYPE &x,COORD_TYPE &y)
{
    // to not apply the changes
    CATCHCHALLENGER_TYPE_MAPID mapIndex=this->mapIndex;
    const Map_server_MapVisibility_Simple_StoreOnSender &map=Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.at(mapIndex);
    x=this->x;
    y=this->y;
    //resolv the object
    Direction direction=getLastDirection();
    switch(direction)
    {
        case Direction_look_at_top:
        case Direction_look_at_right:
        case Direction_look_at_bottom:
        case Direction_look_at_left:
            direction=lookToMove(direction);
            if(MoveOnTheMap::canGoTo(Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list,direction,map,x,y,false))
            {
                if(!MoveOnTheMap::move<Map_server_MapVisibility_Simple_StoreOnSender>(Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list,direction,mapIndex,x,y,false))
                {
                    const Map_server_MapVisibility_Simple_StoreOnSender &map=Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.at(mapIndex);
                    if(!MoveOnTheMap::isWalkable(map,x,y))
                    {
                        if(MoveOnTheMap::canGoTo(Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list,direction,map,x,y,true))
                        {
                            if(!MoveOnTheMap::move<Map_server_MapVisibility_Simple_StoreOnSender>(Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list,direction,mapIndex,x,y,true))
                            {}
                        }
                        else
                            return nullptr;
                    }
                    return nullptr;
                }
            }
            else
                return nullptr;
        break;
        default:
            return nullptr;
        return nullptr;
    }
    return &Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.at(mapIndex);
}

bool CatchChallenger::operator==(const CatchChallenger::MonsterDrops &monsterDrops1,const CatchChallenger::MonsterDrops &monsterDrops2)
{
    if(monsterDrops1.item!=monsterDrops2.item)
        return false;
    if(monsterDrops1.luck!=monsterDrops2.luck)
        return false;
    if(monsterDrops1.quantity_min!=monsterDrops2.quantity_min)
        return false;
    if(monsterDrops1.quantity_max!=monsterDrops2.quantity_max)
        return false;
    return true;
}

void Client::appendAllow(const ActionAllow &allow)
{
    if(public_and_private_informations.allow.find(allow)!=public_and_private_informations.allow.cend())
        return;
    public_and_private_informations.allow.insert(allow);
    syncDatabaseAllow();
}

void Client::removeAllow(const ActionAllow &allow)
{
    if(public_and_private_informations.allow.find(allow)==public_and_private_informations.allow.cend())
        return;
    public_and_private_informations.allow.erase(allow);
    syncDatabaseAllow();
}

//return true if validated and gift sended
bool Client::triggerDaillyGift(const uint64_t &timeRangeEventTimestamps)
{
    if(lastdaillygift==timeRangeEventTimestamps || stat!=ClientStat::CharacterSelected)
        return false;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.gift_list.empty())
    {
        std::cerr << "triggerDaillyGift can't have GlobalServerData::serverPrivateVariables.gift_list.empty()" << std::endl;
        abort();
    }
    #endif
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_gift.asyncWrite({
                std::to_string(timeRangeEventTimestamps),
                std::to_string(character_id)
                });
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif

    const uint32_t &randomNumber=rand();
    unsigned int index=0;
    while(index<GlobalServerData::serverPrivateVariables.gift_list.size())
    {
        const ServerPrivateVariables::GiftEntry &giftEntry=GlobalServerData::serverPrivateVariables.gift_list.at(index);
        if(giftEntry.min_random_number>=randomNumber && giftEntry.max_random_number>=randomNumber)
        {
            addObjectAndSend(giftEntry.item,1);
            break;
        }
        index++;
    }
    if(index>=GlobalServerData::serverPrivateVariables.gift_list.size())
    {
        const ServerPrivateVariables::GiftEntry &giftEntry=GlobalServerData::serverPrivateVariables.gift_list.back();
        addObjectAndSend(giftEntry.item,1);
    }
    lastdaillygift=timeRangeEventTimestamps;
    return true;
}
