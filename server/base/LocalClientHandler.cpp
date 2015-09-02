#include "Client.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "../../general/base/cpp11addition.h"
#include "../base/PreparedDBQuery.h"
#include "../base/BaseServerMasterLoadDictionary.h"
#include "SqlFunction.h"
#include "MapServer.h"
#include "DictionaryLogin.h"

#include "GlobalServerData.h"

#include <vector>
#include <string>
#include <iostream>

using namespace CatchChallenger;

bool Client::checkCollision()
{
    if(map->parsed_layer.walkable==NULL)
        return false;
    if(!map->parsed_layer.walkable[x+y*map->width])
    {
        errorOutput("move at "+std::to_string(temp_direction)+", can't wall at: "+std::to_string(x)+","+std::to_string(y)+" on map: "+map->map_file);
        return false;
    }
    else
        return true;
}

void Client::removeFromClan()
{
    clanChangeWithoutDb(0);
}

std::string Client::directionToStringToSave(const Direction &direction)
{
    switch(direction)
    {
        case Direction_look_at_top:
        case Direction_move_at_top:
            return Client::text_top;
        break;
        case Direction_look_at_right:
        case Direction_move_at_right:
            return Client::text_right;
        break;
        case Direction_look_at_bottom:
        case Direction_move_at_bottom:
            return Client::text_bottom;
        break;
        case Direction_look_at_left:
        case Direction_move_at_left:
            return Client::text_left;
        break;
        default:
        break;
    }
    return Client::text_bottom;
}

std::string Client::orientationToStringToSave(const Orientation &orientation)
{
    switch(orientation)
    {
        case Orientation_top:
            return Client::text_top;
        break;
        case Orientation_bottom:
            return Client::text_bottom;
        break;
        case Orientation_right:
            return Client::text_right;
        break;
        case Orientation_left:
            return Client::text_left;
        break;
        default:
        break;
    }
    return Client::text_bottom;
}

void Client::savePosition()
{
    //virtual stop the player
    //Orientation orientation;
    #ifdef DEBUG_MESSAGE_CLIENT_MOVE
    normalOutput(
                "map->map_file: "+
                map->map_file+
                ",x: "+std::to_string(x)+
                ",y: "+std::to_string(y)+
                ", orientation: "+std::to_string((int)last_direction)
                );
    #endif
    /* disable because use memory, but useful only into less than < 0.1% of case
     * if(map!=at_start_map_name || x!=at_start_x || y!=at_start_y || orientation!=at_start_orientation) */
    std::string updateMapPositionQuery,updateMapPositionQuerySub;
    const uint32_t &map_file_database_id=static_cast<MapServer *>(map)->reverse_db_id;
    const uint32_t &rescue_map_file_database_id=static_cast<MapServer *>(rescue.map)->reverse_db_id;
    const uint32_t &unvalidated_rescue_map_file_database_id=static_cast<MapServer *>(unvalidated_rescue.map)->reverse_db_id;
    updateMapPositionQuery=PreparedDBQueryServer::db_query_update_character_forserver_map_part1;
    stringreplace(updateMapPositionQuery,"%1",std::to_string(map_file_database_id));
    stringreplace(updateMapPositionQuery,"%2",std::to_string(x));
    stringreplace(updateMapPositionQuery,"%3",std::to_string(y));
    stringreplace(updateMapPositionQuery,"%4",std::to_string((uint8_t)getLastDirection()));
    updateMapPositionQuerySub=PreparedDBQueryServer::db_query_update_character_forserver_map_part2;
    stringreplace(updateMapPositionQuerySub,"%1",std::to_string(rescue_map_file_database_id));
    stringreplace(updateMapPositionQuerySub,"%2",std::to_string(rescue.x));
    stringreplace(updateMapPositionQuerySub,"%3",std::to_string(rescue.y));
    stringreplace(updateMapPositionQuerySub,"%4",std::to_string((uint8_t)rescue.orientation));
    stringreplace(updateMapPositionQuerySub,"%5",std::to_string(unvalidated_rescue_map_file_database_id));
    stringreplace(updateMapPositionQuerySub,"%6",std::to_string(unvalidated_rescue.x));
    stringreplace(updateMapPositionQuerySub,"%7",std::to_string(unvalidated_rescue.y));
    stringreplace(updateMapPositionQuerySub,"%8",std::to_string((uint8_t)unvalidated_rescue.orientation));
    stringreplace(updateMapPositionQuery,"%5",updateMapPositionQuerySub);
    stringreplace(updateMapPositionQuery,"%6",std::to_string(character_id));
    dbQueryWriteServer(updateMapPositionQuery);
/* do at moveDownMonster and moveUpMonster
 *     const std::vector<PlayerMonster> &playerMonsterList=getPlayerMonster();
    int index=0;
    while(index<playerMonsterList.size())
    {
        const PlayerMonster &playerMonster=playerMonsterList.at(index);
        saveMonsterPosition(playerMonster.id,index+1);
        index++;
    }*/
}

/* why do that's here?
 * Because the ClientMapManagement can be totaly satured by the square complexity
 * that's allow to continue the player to connect and play
 * the overhead for the network it just at the connexion */
void Client::put_on_the_map(CommonMap *map,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation)
{
    MapBasicMove::put_on_the_map(map,x,y,orientation);
    insertClientOnMap(map);

    //send to the client the position of the player
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);

    out << (uint8_t)0x01;
    if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
        out << (uint8_t)map->id;
    else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
        out << (uint16_t)map->id;
    else
        out << (uint32_t)map->id;
    if(GlobalServerData::serverSettings.max_players<=255)
    {
        out << (uint8_t)0x01;
        out << (uint8_t)public_and_private_informations.public_informations.simplifiedId;
    }
    else
    {
        out << (uint16_t)0x0001;
        out << (uint16_t)public_and_private_informations.public_informations.simplifiedId;
    }
    out << x;
    out << y;
    if(GlobalServerData::serverSettings.dontSendPlayerType)
        out << uint8_t((uint8_t)orientation | (uint8_t)Player_type_normal);
    else
        out << uint8_t((uint8_t)orientation | (uint8_t)public_and_private_informations.public_informations.type);
    if(CommonSettingsServer::commonSettingsServer.forcedSpeed==0)
        out << public_and_private_informations.public_informations.speed;

    if(!CommonSettingsServer::commonSettingsServer.dontSendPseudo)
    {
        outputData+=rawPseudo;
        out.device()->seek(out.device()->pos()+rawPseudo.size());
    }
    out << public_and_private_informations.public_informations.skinId;

    sendPacket(0xC0,outputData.constData(),outputData.size());

    //load the first time the random number list
    generateRandomNumber();

    playerByPseudo[public_and_private_informations.public_informations.pseudo]=this;
    playerById[character_id]=this;
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
    if(clanList.find(public_and_private_informations.clan)==clanList.end())
    {
        clan=new Clan;
        clan->cash=0;
        clan->clanId=public_and_private_informations.clan;
        clanList[public_and_private_informations.clan]=clan;
        if(GlobalServerData::serverPrivateVariables.cityStatusListReverse.find(clan->clanId)!=GlobalServerData::serverPrivateVariables.cityStatusListReverse.end())
            clan->capturedCity=GlobalServerData::serverPrivateVariables.cityStatusListReverse.at(clan->clanId);
    }
    else
        clan=clanList.at(public_and_private_informations.clan);
    clan->players.push_back(this);
}

bool Client::moveThePlayer(const uint8_t &previousMovedUnit,const Direction &direction)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(this->x>=this->map->width)
    {
        std::cerr << "x to out of map: " << this->x << " > " << this->map->width << " (" << this->map->map_file << ")" << std::endl;
        abort();
        return false;
    }
    if(this->y>=this->map->height)
    {
        std::cerr << "y to out of map: " << this->y << " > " << this->map->height << " (" << this->map->map_file << ")" << std::endl;
        abort();
        return false;
    }
    #endif

    #ifdef DEBUG_MESSAGE_CLIENT_MOVE
    std::cout << "moveThePlayer(): for player (" << x << "," << y << "): " << public_and_private_informations.public_informations.simplifiedId
              << ", previousMovedUnit: " << previousMovedUnit << " (" << MoveOnTheMap::directionToString(getLastDirection())
              << "), next direction: " << MoveOnTheMap::directionToString(direction) << std::endl;
    #endif
    const bool &returnValue=MapBasicMove::moveThePlayer(previousMovedUnit,direction);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(returnValue)
    {
        if(this->x>=this->map->width)
        {
            std::cerr << "x to out of map: " << this->x << " > " << this->map->width << " (" << this->map->map_file << ")" << std::endl;
            abort();
            return false;
        }
        if(this->y>=this->map->height)
        {
            std::cerr << "y to out of map: " << this->y << " > " << this->map->height << " (" << this->map->map_file << ")" << std::endl;
            abort();
            return false;
        }
    }
    #endif
    return returnValue;
}

bool Client::captureCityInProgress()
{
    if(clan==NULL)
        return false;
    if(clan->captureCityInProgress.size()==0)
        return false;
    //search in capture not validated
    if(captureCity.find(clan->captureCityInProgress)!=captureCity.end())
        if(vectorindexOf(captureCity.at(clan->captureCityInProgress),this)>=0)
            return true;
    //search in capture validated
    if(captureCityValidatedList.find(clan->captureCityInProgress)!=captureCityValidatedList.end())
    {
        const CaptureCityValidated &captureCityValidated=captureCityValidatedList.at(clan->captureCityInProgress);
        //check if this player is into the capture city with the other player of the team
        if(vectorindexOf(captureCityValidated.players,this)>=0)
            return true;
    }
    return false;
}

bool Client::singleMove(const Direction &direction)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(this->x>=this->map->width)
    {
        std::cerr << "x to out of map: " << this->x << " > " << this->map->width << " (" << this->map->map_file << ")" << std::endl;
        abort();
        return false;
    }
    if(this->y>=this->map->height)
    {
        std::cerr << "x to out of map: " << this->y << " > " << this->map->height << " (" << this->map->map_file << ")" << std::endl;
        abort();
        return false;
    }
    #endif

    if(isInFight())//check if is in fight
    {
        errorOutput("error: try move when is in fight");
        return false;
    }
    if(captureCityInProgress())
    {
        errorOutput("Try move when is in capture city");
        return false;
    }
    if(oldEvents.oldEventList.size()>0 && (QDateTime::currentDateTime().toTime_t()-oldEvents.time.toTime_t())>30/*30s*/)
    {
        errorOutput("Try move but lost of event sync");
        return false;
    }
    COORD_TYPE x=this->x,y=this->y;
    temp_direction=direction;
    CommonMap* map=this->map;
    #ifdef DEBUG_MESSAGE_CLIENT_MOVE
    normalOutput("Client::singleMove(), go in this direction: "+MoveOnTheMap::directionToString(direction)+" with map: "+map->map_file+"("+std::to_string(x)+","+std::to_string(y)+")");
    #endif
    if(!MoveOnTheMap::canGoTo(direction,*map,x,y,true))
    {
        errorOutput("Client::singleMove(), can't go into this direction: "+MoveOnTheMap::directionToString(direction)+" with map: "+map->map_file+"("+std::to_string(x)+","+std::to_string(y)+")");
        return false;
    }
    if(!MoveOnTheMap::moveWithoutTeleport(direction,&map,&x,&y,false,true))
    {
        errorOutput("Client::singleMove(), can go but move failed into this direction: "+MoveOnTheMap::directionToString(direction)+" with map: "+map->map_file+"("+std::to_string(x)+","+std::to_string(y)+")");
        return false;
    }

    if(map->teleporter.find(x+y*map->width)!=map->teleporter.end())
    {
        const CommonMap::Teleporter &teleporter=map->teleporter.at(x+y*map->width);
        switch(teleporter.condition.type)
        {
            case CatchChallenger::MapConditionType_None:
            case CatchChallenger::MapConditionType_Clan://not do for now
            break;
            case CatchChallenger::MapConditionType_FightBot:
                if(public_and_private_informations.bot_already_beaten.find(teleporter.condition.value)==public_and_private_informations.bot_already_beaten.end())
                {
                    errorOutput("Need have FightBot win to use this teleporter: "+std::to_string(teleporter.condition.value)+" with map: "+map->map_file+"("+std::to_string(x)+","+std::to_string(y)+")");
                    return false;
                }
            break;
            case CatchChallenger::MapConditionType_Item:
                if(public_and_private_informations.items.find(teleporter.condition.value)==public_and_private_informations.items.cend())
                {
                    errorOutput("Need have item to use this teleporter: "+std::to_string(teleporter.condition.value)+" with map: "+map->map_file+"("+std::to_string(x)+","+std::to_string(y)+")");
                    return false;
                }
            break;
            case CatchChallenger::MapConditionType_Quest:
                if(public_and_private_informations.quests.find(teleporter.condition.value)==public_and_private_informations.quests.cend())
                {
                    errorOutput("Need have quest to use this teleporter: "+std::to_string(teleporter.condition.value)+" with map: "+map->map_file+"("+std::to_string(x)+","+std::to_string(y)+")");
                    return false;
                }
                if(!public_and_private_informations.quests.at(teleporter.condition.value).finish_one_time)
                {
                    errorOutput("Need have finish the quest to use this teleporter: "+std::to_string(teleporter.condition.value)+" with map: "+map->map_file+"("+std::to_string(x)+","+std::to_string(y)+")");
                    return false;
                }
            break;
            default:
            break;
        }
        x=teleporter.x;
        y=teleporter.y;
        map=teleporter.map;
    }

    if(this->map!=map)
    {
        CommonMap *new_map=map;
        CommonMap *old_map=this->map;
        this->map=old_map;
        removeClientOnMap(old_map);
        this->map=new_map;
        insertClientOnMap(map);
    }

    this->map=map;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(this->map!=NULL)
        if(this->map->width==999)
            this->map->width=9999;
    #endif
    this->x=x;
    this->y=y;
    {
        MapServer* mapServer=static_cast<MapServer*>(map);
        if(mapServer->rescue.find(std::pair<uint8_t,uint8_t>(x,y))!=mapServer->rescue.cend())
        {
            unvalidated_rescue.map=map;
            unvalidated_rescue.x=x;
            unvalidated_rescue.y=y;
            unvalidated_rescue.orientation=mapServer->rescue.at(std::pair<uint8_t,uint8_t>(x,y));
        }
    }
    if(botFightCollision(map,x,y))
        return true;
    if(public_and_private_informations.repel_step<=0)
    {
        //merge the result event:
        std::vector<uint8_t> mergedEvents(GlobalServerData::serverPrivateVariables.events);
        if(oldEvents.oldEventList.size()>0)
        {
            unsigned int index=0;
            while(index<oldEvents.oldEventList.size())
            {
                mergedEvents[oldEvents.oldEventList.at(index).event]=oldEvents.oldEventList.at(index).eventValue;
                index++;
            }
        }
        if(generateWildFightIfCollision(map,x,y,public_and_private_informations.items,mergedEvents))
        {
            normalOutput("Client::singleMove(), now is in front of wild monster with map: "+map->map_file+"("+std::to_string(x)+","+std::to_string(y)+")");
            return true;
        }
    }
    else
        public_and_private_informations.repel_step--;

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(this->map!=NULL)
    {
        if(this->x>=this->map->width)
        {
            std::cerr << "x to out of map: "+std::to_string(this->x)+" > "+std::to_string(this->map->width)+" ("+this->map->map_file+")" << std::endl;
            abort();
            return false;
        }
        if(this->y>=this->map->height)
        {
            std::cerr << "y to out of map: "+std::to_string(this->y)+" > "+std::to_string(this->map->height)+" ("+this->map->map_file+")" << std::endl;
            abort();
            return false;
        }
    }
    #endif

    return true;
}

void Client::addObjectAndSend(const uint16_t &item,const uint32_t &quantity)
{
    addObject(item,quantity);
    //add into the inventory
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint16_t)1;
    out << (uint16_t)item;
    out << (uint32_t)quantity;
    sendFullPacket(0xD0,0x02,outputData.constData(),outputData.size());
}

void Client::addObject(const uint16_t &item,const uint32_t &quantity)
{
    if(CommonDatapack::commonDatapack.items.item.find(item)==CommonDatapack::commonDatapack.items.item.cend())
    {
        errorOutput("Object is not found into the item list");
        return;
    }
    if(public_and_private_informations.items.find(item)!=public_and_private_informations.items.cend())
    {
        public_and_private_informations.items[item]+=quantity;
        std::string queryText=PreparedDBQueryCommon::db_query_update_item;
        stringreplace(queryText,"%1",std::to_string(public_and_private_informations.items.at(item)));
        stringreplace(queryText,"%2",std::to_string(item));
        stringreplace(queryText,"%3",std::to_string(character_id));
        dbQueryWriteCommon(queryText);
    }
    else
    {
        std::string queryText=PreparedDBQueryCommon::db_query_insert_item;
        stringreplace(queryText,"%1",std::to_string(item));
        stringreplace(queryText,"%2",std::to_string(character_id));
        stringreplace(queryText,"%3",std::to_string(quantity));
        dbQueryWriteCommon(queryText);
        public_and_private_informations.items[item]=quantity;
    }
}

void Client::addWarehouseObject(const uint16_t &item,const uint32_t &quantity)
{
    if(public_and_private_informations.warehouse_items.find(item)!=public_and_private_informations.warehouse_items.cend())
    {
        public_and_private_informations.warehouse_items[item]+=quantity;
        std::string queryText=PreparedDBQueryCommon::db_query_update_item_warehouse;
        stringreplace(queryText,"%1",std::to_string(public_and_private_informations.items.at(item)));
        stringreplace(queryText,"%2",std::to_string(item));
        stringreplace(queryText,"%3",std::to_string(character_id));
        dbQueryWriteCommon(queryText);
    }
    else
    {
        std::string queryText=PreparedDBQueryCommon::db_query_insert_item_warehouse;
        stringreplace(queryText,"%1",std::to_string(item));
        stringreplace(queryText,"%2",std::to_string(character_id));
        stringreplace(queryText,"%3",std::to_string(quantity));
        dbQueryWriteCommon(queryText);
        public_and_private_informations.warehouse_items[item]=quantity;
    }
}

uint32_t Client::removeWarehouseObject(const uint16_t &item,const uint32_t &quantity)
{
    if(public_and_private_informations.warehouse_items.find(item)!=public_and_private_informations.warehouse_items.cend())
    {
        if(public_and_private_informations.warehouse_items.at(item)>quantity)
        {
            public_and_private_informations.warehouse_items[item]-=quantity;
            std::string queryText=PreparedDBQueryCommon::db_query_update_item_warehouse;
            stringreplace(queryText,"%1",std::to_string(public_and_private_informations.items.at(item)));
            stringreplace(queryText,"%2",std::to_string(item));
            stringreplace(queryText,"%3",std::to_string(character_id));
            dbQueryWriteCommon(queryText);
            return quantity;
        }
        else
        {
            const uint32_t removed_quantity=public_and_private_informations.warehouse_items.at(item);
            public_and_private_informations.warehouse_items.erase(item);
            std::string queryText=PreparedDBQueryCommon::db_query_delete_item_warehouse;
            stringreplace(queryText,"%1",std::to_string(item));
            stringreplace(queryText,"%2",std::to_string(character_id));
            dbQueryWriteCommon(queryText);
            return removed_quantity;
        }
    }
    else
        return 0;
}

void Client::saveObjectRetention(const uint16_t &item)
{
    if(public_and_private_informations.items.find(item)!=public_and_private_informations.items.cend())
    {
        std::string queryText=PreparedDBQueryCommon::db_query_update_item;
        stringreplace(queryText,"%1",std::to_string(public_and_private_informations.items.at(item)));
        stringreplace(queryText,"%2",std::to_string(item));
        stringreplace(queryText,"%3",std::to_string(character_id));
        dbQueryWriteCommon(queryText);
    }
    else
    {
        std::string queryText=PreparedDBQueryCommon::db_query_delete_item;
        stringreplace(queryText,"%1",std::to_string(item));
        stringreplace(queryText,"%2",std::to_string(character_id));
        dbQueryWriteCommon(queryText);
    }
}

uint32_t Client::removeObject(const uint16_t &item, const uint32_t &quantity)
{
    if(public_and_private_informations.items.find(item)!=public_and_private_informations.items.cend())
    {
        if(public_and_private_informations.items.at(item)>quantity)
        {
            public_and_private_informations.items[item]-=quantity;
            std::string queryText=PreparedDBQueryCommon::db_query_update_item;
            stringreplace(queryText,"%1",std::to_string(public_and_private_informations.items.at(item)));
            stringreplace(queryText,"%2",std::to_string(item));
            stringreplace(queryText,"%3",std::to_string(character_id));
            dbQueryWriteCommon(queryText);
            return quantity;
        }
        else
        {
            const uint32_t removed_quantity=public_and_private_informations.items.at(item);
            public_and_private_informations.items.erase(item);
            std::string queryText=PreparedDBQueryCommon::db_query_delete_item;
            stringreplace(queryText,"%1",std::to_string(item));
            stringreplace(queryText,"%2",std::to_string(character_id));
            dbQueryWriteCommon(queryText);
            return removed_quantity;
        }
    }
    else
        return 0;
}

void Client::sendRemoveObject(const uint16_t &item,const uint32_t &quantity)
{
    //add into the inventory
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint32_t)1;
    out << (uint16_t)item;
    out << (uint32_t)quantity;
    sendFullPacket(0xD0,0x03,outputData.constData(),outputData.size());
}

uint32_t Client::objectQuantity(const uint16_t &item) const
{
    if(public_and_private_informations.items.find(item)!=public_and_private_informations.items.cend())
        return public_and_private_informations.items.at(item);
    else
        return 0;
}

bool Client::addMarketCashWithoutSave(const quint64 &cash)
{
    market_cash+=cash;
    return true;
}

void Client::addCash(const quint64 &cash, const bool &forceSave)
{
    if(cash==0 && !forceSave)
        return;
    public_and_private_informations.cash+=cash;
    std::string queryText=PreparedDBQueryCommon::db_query_update_cash;
    stringreplace(queryText,"%1",std::to_string(public_and_private_informations.cash));
    stringreplace(queryText,"%2",std::to_string(character_id));
    dbQueryWriteCommon(queryText);
}

void Client::removeCash(const quint64 &cash)
{
    if(cash==0)
        return;
    public_and_private_informations.cash-=cash;
    std::string queryText=PreparedDBQueryCommon::db_query_update_cash;
    stringreplace(queryText,"%1",std::to_string(public_and_private_informations.cash));
    stringreplace(queryText,"%2",std::to_string(character_id));
    dbQueryWriteCommon(queryText);
}

void Client::addWarehouseCash(const quint64 &cash, const bool &forceSave)
{
    if(cash==0 && !forceSave)
        return;
    public_and_private_informations.warehouse_cash+=cash;
    std::string queryText=PreparedDBQueryCommon::db_query_update_warehouse_cash;
    stringreplace(queryText,"%1",std::to_string(public_and_private_informations.warehouse_cash));
    stringreplace(queryText,"%2",std::to_string(character_id));
    dbQueryWriteCommon(queryText);
}

void Client::removeWarehouseCash(const quint64 &cash)
{
    if(cash==0)
        return;
    public_and_private_informations.warehouse_cash-=cash;
    std::string queryText=PreparedDBQueryCommon::db_query_update_warehouse_cash;
    stringreplace(queryText,"%1",std::to_string(public_and_private_informations.warehouse_cash));
    stringreplace(queryText,"%2",std::to_string(character_id));
    dbQueryWriteCommon(queryText);
}

void Client::wareHouseStore(const qint64 &cash, const std::vector<std::pair<uint16_t, int32_t> > &items, const std::vector<uint32_t> &withdrawMonsters, const std::vector<uint32_t> &depositeMonsters)
{
    if(!wareHouseStoreCheck(cash,items,withdrawMonsters,depositeMonsters))
        return;
    {
        unsigned int index=0;
        while(index<items.size())
        {
            const std::pair<uint16_t, int32_t> &item=items.at(index);
            if(item.second>0)
            {
                removeWarehouseObject(item.first,item.second);
                addObject(item.first,item.second);
            }
            if(item.second<0)
            {
                removeObject(item.first,-item.second);
                addWarehouseObject(item.first,-item.second);
            }
            index++;
        }
    }
    //validate the change here
    if(cash>0)
    {
        removeWarehouseCash(cash);
        addCash(cash);
    }
    if(cash<0)
    {
        removeCash(-cash);
        addWarehouseCash(-cash);
    }
    {
        unsigned int index=0;
        while(index<withdrawMonsters.size())
        {
            unsigned int sub_index=0;
            while(sub_index<public_and_private_informations.warehouse_playerMonster.size())
            {
                const PlayerMonster &playerMonsterinWarehouse=public_and_private_informations.warehouse_playerMonster.at(sub_index);
                if(playerMonsterinWarehouse.id==withdrawMonsters.at(index))
                {
                    std::string queryText=PreparedDBQueryCommon::db_query_update_monster_move_to_player;
                    stringreplace(queryText,"%1",std::to_string(public_and_private_informations.playerMonster.size()+2));
                    stringreplace(queryText,"%2",std::to_string(playerMonsterinWarehouse.id));
                    dbQueryWriteCommon(queryText);
                    public_and_private_informations.playerMonster.push_back(public_and_private_informations.warehouse_playerMonster.at(sub_index));
                    public_and_private_informations.warehouse_playerMonster.erase(public_and_private_informations.warehouse_playerMonster.cbegin()+sub_index);
                    break;
                }
                sub_index++;
            }
            index++;
        }
    }
    {
        unsigned int index=0;
        while(index<depositeMonsters.size())
        {
            unsigned int sub_index=0;
            while(sub_index<public_and_private_informations.playerMonster.size())
            {
                const PlayerMonster &playerMonsterOnPlayer=public_and_private_informations.playerMonster.at(sub_index);
                if(playerMonsterOnPlayer.id==depositeMonsters.at(index))
                {
                    std::string queryText=PreparedDBQueryCommon::db_query_update_monster_move_to_warehouse;
                    stringreplace(queryText,"%1",std::to_string(public_and_private_informations.warehouse_playerMonster.size()+2));
                    stringreplace(queryText,"%2",std::to_string(playerMonsterOnPlayer.id));
                    dbQueryWriteCommon(queryText);
                    public_and_private_informations.warehouse_playerMonster.push_back(public_and_private_informations.playerMonster.at(sub_index));
                    public_and_private_informations.playerMonster.erase(public_and_private_informations.playerMonster.cbegin()+sub_index);
                    break;
                }
                sub_index++;
            }
            index++;
        }
    }
    if(depositeMonsters.size()>0 || withdrawMonsters.size()>0)
        if(GlobalServerData::serverSettings.fightSync==GameServerSettings::FightSync_AtTheDisconnexion)
            saveMonsterStat(public_and_private_informations.playerMonster.back());
}

bool Client::wareHouseStoreCheck(const qint64 &cash, const std::vector<std::pair<uint16_t, int32_t> > &items, const std::vector<uint32_t> &withdrawMonsters, const std::vector<uint32_t> &depositeMonsters)
{
    //check all
    if((cash>0 && (qint64)public_and_private_informations.warehouse_cash<cash) || (cash<0 && (qint64)public_and_private_informations.cash<-cash))
    {
        errorOutput("cash transfer is wrong");
        return false;
    }
    {
        unsigned int index=0;
        while(index<items.size())
        {
            const std::pair<uint16_t, int32_t> &item=items.at(index);
            if(item.second>0)
            {
                if(public_and_private_informations.warehouse_items.find(item.first)==public_and_private_informations.warehouse_items.cend())
                {
                    errorOutput("warehouse item transfer is wrong due to missing");
                    return false;
                }
                if((qint64)public_and_private_informations.warehouse_items.at(item.first)<item.second)
                {
                    errorOutput("warehouse item transfer is wrong due to wrong quantity");
                    return false;
                }
            }
            if(item.second<0)
            {
                if(public_and_private_informations.items.find(item.first)==public_and_private_informations.items.cend())
                {
                    errorOutput("item transfer is wrong due to missing");
                    return false;
                }
                if((qint64)public_and_private_informations.items.at(item.first)<-item.second)
                {
                    errorOutput("item transfer is wrong due to wrong quantity");
                    return false;
                }
            }
            index++;
        }
    }
    int count_change=0;
    {
        unsigned int index=0;
        while(index<withdrawMonsters.size())
        {
            unsigned int sub_index=0;
            while(sub_index<public_and_private_informations.warehouse_playerMonster.size())
            {
                if(public_and_private_informations.warehouse_playerMonster.at(sub_index).id==withdrawMonsters.at(index))
                {
                    count_change++;
                    break;
                }
                sub_index++;
            }
            if(sub_index==public_and_private_informations.warehouse_playerMonster.size())
            {
                errorOutput("no monster to withdraw");
                return false;
            }
            index++;
        }
    }
    {
        unsigned int index=0;
        while(index<depositeMonsters.size())
        {
            unsigned int sub_index=0;
            while(sub_index<public_and_private_informations.playerMonster.size())
            {
                if(public_and_private_informations.playerMonster.at(sub_index).id==depositeMonsters.at(index))
                {
                    count_change--;
                    break;
                }
                sub_index++;
            }
            if(sub_index==public_and_private_informations.playerMonster.size())
            {
                errorOutput("no monster to deposite");
                return false;
            }
            index++;
        }
    }
    if((public_and_private_informations.playerMonster.size()+count_change)>CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters)
    {
        errorOutput("have more monster to withdraw than the allowed");
        return false;
    }
    return true;
}

void Client::sendHandlerCommand(const std::string &command,const std::string &extraText)
{
    if(command==Client::text_give)
    {
        bool ok;
        std::vector<std::string> arguments=stringsplit(extraText,' ');
        vectorRemoveEmpty(arguments);
        if(arguments.size()==2)
            arguments.push_back(Client::text_1);
        while(arguments.size()>3)
        {
            const std::string arg1=arguments.at(arguments.size()-3);
            arguments.erase(arguments.end()-3);
            const std::string arg2=arguments.at(arguments.size()-2);
            arguments.erase(arguments.end()-2);
            arguments.insert(arguments.end(),arg1+Client::text_space+arg2);
        }
        if(arguments.size()!=3)
        {
            receiveSystemText("Wrong arguments number for the command, usage: /give objectId player [quantity=1]");
            return;
        }
        const uint32_t &objectId=stringtouint32(arguments.at(0),&ok);
        if(!ok)
        {
            receiveSystemText("objectId is not a number, usage: /give objectId player [quantity=1]");
            return;
        }
        if(CommonDatapack::commonDatapack.items.item.find(objectId)==CommonDatapack::commonDatapack.items.item.cend())
        {
            receiveSystemText("objectId is not a valid item, usage: /give objectId player [quantity=1]");
            return;
        }
        uint32_t quantity=1;
        if(arguments.size()==3)
        {
            quantity=stringtouint32(arguments.back(),&ok);
            if(!ok)
            {
                while(arguments.size()>2)
                {
                    const std::string arg1=arguments.at(arguments.size()-2);
                    arguments.erase(arguments.end()-2);
                    const std::string arg2=arguments.at(arguments.size()-1);
                    arguments.erase(arguments.end()-1);
                    arguments.insert(arguments.end(),arg1+Client::text_space+arg2);
                }
                quantity=1;
                //receiveSystemText(std::stringLiteral("quantity is not a number, usage: /give objectId player [quantity=1]"));
                //return;
            }
        }
        if(playerByPseudo.find(arguments.at(1))==playerByPseudo.cend())
        {
            receiveSystemText("player is not connected, usage: /give objectId player [quantity=1]");
            return;
        }
        normalOutput(public_and_private_informations.public_informations.pseudo+
                     " have give to "+
                     arguments.at(1)+
                     " the item with id: "+
                     std::to_string(objectId)+
                     " in quantity: "+
                     std::to_string(quantity));
        playerByPseudo.at(arguments.at(1))->addObjectAndSend(objectId,quantity);
    }
    else if(command==Client::text_setevent)
    {
        std::vector<std::string> arguments=stringsplit(extraText,' ');
        vectorRemoveEmpty(arguments);
        if(arguments.size()!=2)
        {
            receiveSystemText("Wrong arguments number for the command, usage: /give setevent [event] [value]");
            return;
        }
        unsigned int index=0,sub_index;
        while(index<CommonDatapack::commonDatapack.events.size())
        {
            const Event &event=CommonDatapack::commonDatapack.events.at(index);
            if(event.name==arguments.at(0))
            {
                sub_index=0;
                while(sub_index<event.values.size())
                {
                    if(event.values.at(sub_index)==arguments.at(1))
                    {
                        if(GlobalServerData::serverPrivateVariables.events.at(index)==sub_index)
                        {
                            receiveSystemText("The event have aready this value");
                            return;
                        }
                        else
                        {
                            setEvent(index,sub_index);
                            GlobalServerData::serverPrivateVariables.events[index]=sub_index;
                        }
                        break;
                    }
                    sub_index++;
                }
                if(sub_index==event.values.size())
                {
                    receiveSystemText("The event value is not found");
                    return;
                }
                break;
            }
            index++;
        }
        if(index==CommonDatapack::commonDatapack.events.size())
        {
            receiveSystemText("The event is not found");
            return;
        }
    }
    else if(command==Client::text_take)
    {
        bool ok;
        std::vector<std::string> arguments=stringsplit(extraText,' ');
        vectorRemoveEmpty(arguments);
        if(arguments.size()==2)
            arguments.push_back(Client::text_1);
        if(arguments.size()!=3)
        {
            receiveSystemText("Wrong arguments number for the command, usage: /take objectId player [quantity=1]");
            return;
        }
        const uint32_t objectId=stringtouint32(arguments.front(),&ok);
        if(!ok)
        {
            receiveSystemText("objectId is not a number, usage: /take objectId player [quantity=1]");
            return;
        }
        if(CommonDatapack::commonDatapack.items.item.find(objectId)==CommonDatapack::commonDatapack.items.item.cend())
        {
            receiveSystemText("objectId is not a valid item, usage: /take objectId player [quantity=1]");
            return;
        }
        const uint32_t quantity=stringtouint32(arguments.back(),&ok);
        if(!ok)
        {
            receiveSystemText("quantity is not a number, usage: /take objectId player [quantity=1]");
            return;
        }
        if(playerByPseudo.find(arguments.at(1))==playerByPseudo.end())
        {
            receiveSystemText("player is not connected, usage: /take objectId player [quantity=1]");
            return;
        }
        normalOutput(public_and_private_informations.public_informations.pseudo+" have take to "+arguments.at(1)+" the item with id: "+std::to_string(objectId)+" in quantity: "+std::to_string(quantity));
        playerByPseudo.at(arguments.at(1))->sendRemoveObject(objectId,playerByPseudo.at(arguments.at(1))->removeObject(objectId,quantity));
    }
    else if(command==Client::text_tp)
    {
        std::vector<std::string> arguments=stringsplit(extraText,' ');
        vectorRemoveEmpty(arguments);
        if(arguments.size()==3)
        {
            if(arguments.at(1)!=Client::text_to)
            {
                receiveSystemText("wrong second arguement: "+arguments.at(1)+", usage: /tp player1 to player2");
                return;
            }
            if(playerByPseudo.find(arguments.front())==playerByPseudo.end())
            {
                receiveSystemText(arguments.front()+" is not connected, usage: /tp player1 to player2");
                return;
            }
            if(playerByPseudo.find(arguments.back())==playerByPseudo.end())
            {
                receiveSystemText(arguments.back()+" is not connected, usage: /tp player1 to player2");
                return;
            }
            Client * const otherPlayerTo=playerByPseudo.at(arguments.back());
            playerByPseudo.at(arguments.front())->receiveTeleportTo(otherPlayerTo->map,otherPlayerTo->x,otherPlayerTo->y,MoveOnTheMap::directionToOrientation(otherPlayerTo->getLastDirection()));
        }
        else
        {
            receiveSystemText("Wrong arguments number for the command, usage: /tp player1 to player2");
            return;
        }
    }
    else if(command==Client::text_trade)
    {
        if(extraText.size()==0)
        {
            receiveSystemText("no player given, syntaxe: /trade player");
            return;
        }
        if(playerByPseudo.find(extraText)==playerByPseudo.end())
        {
            receiveSystemText(extraText+" is not connected");
            return;
        }
        if(public_and_private_informations.public_informations.pseudo==extraText)
        {
            receiveSystemText("You can't trade with yourself");
            return;
        }
        if(getInTrade())
        {
            receiveSystemText("You are already in trade");
            return;
        }
        if(isInBattle())
        {
            receiveSystemText("You are already in battle");
            return;
        }
        if(playerByPseudo.at(extraText)->getInTrade())
        {
            receiveSystemText(extraText+" is already in trade");
            return;
        }
        if(playerByPseudo.at(extraText)->isInBattle())
        {
            receiveSystemText(extraText+" is already in battle");
            return;
        }
        if(!otherPlayerIsInRange(playerByPseudo.at(extraText)))
        {
            receiveSystemText(extraText+" is not in range");
            return;
        }
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        normalOutput("Trade requested");
        #endif
        otherPlayerTrade=playerByPseudo.at(extraText);
        otherPlayerTrade->registerTradeRequest(this);
    }
    else if(command==Client::text_battle)
    {
        if(extraText.size()==0)
        {
            receiveSystemText("no player given, syntaxe: /battle player");
            return;
        }
        if(playerByPseudo.find(extraText)==playerByPseudo.end())
        {
            receiveSystemText(extraText+" is not connected");
            return;
        }
        if(public_and_private_informations.public_informations.pseudo==extraText)
        {
            receiveSystemText("You can't battle with yourself");
            return;
        }
        if(isInBattle())
        {
            receiveSystemText("you are already in battle");
            return;
        }
        if(getInTrade())
        {
            receiveSystemText("you are already in trade");
            return;
        }
        if(playerByPseudo.at(extraText)->isInBattle())
        {
            receiveSystemText(extraText+" is already in battle");
            return;
        }
        if(playerByPseudo.at(extraText)->getInTrade())
        {
            receiveSystemText(extraText+" is already in battle");
            return;
        }
        if(!otherPlayerIsInRange(playerByPseudo.at(extraText)))
        {
            receiveSystemText(extraText+" is not in range");
            return;
        }
        if(!playerByPseudo.at(extraText)->getAbleToFight())
        {
            receiveSystemText("The other player can't fight");
            return;
        }
        if(!getAbleToFight())
        {
            receiveSystemText("You can't fight");
            return;
        }
        if(playerByPseudo.at(extraText)->isInFight())
        {
            receiveSystemText("The other player is in fight");
            return;
        }
        if(isInFight())
        {
            receiveSystemText("You are in fight");
            return;
        }
        if(captureCityInProgress())
        {
            errorOutput("Try battle when is in capture city");
            return;
        }
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        normalOutput("Battle requested");
        #endif
        playerByPseudo.at(extraText)->registerBattleRequest(this);
    }
}

void Client::setEvent(const uint8_t &event, const uint8_t &new_value)
{
    const uint8_t &event_value=GlobalServerData::serverPrivateVariables.events.at(event);
    QDateTime currentDateTime=QDateTime::currentDateTime();
    std::vector<Client *> playerList;
    playerList.reserve(playerByPseudo.size());
    auto i=playerByPseudo.begin();
    while(i!=playerByPseudo.cend())
    {
        i->second->addEventInQueue(event,event_value,currentDateTime);
        playerList.push_back(i->second);
        ++i;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << event;
    out << new_value;
    unsigned int index=0;
    while(index<playerList.size())
    {
        playerList.at(index)->sendNewEvent(outputData);
        index++;
    }
    GlobalServerData::serverPrivateVariables.events[event]=new_value;
}

void Client::addEventInQueue(const uint8_t &event,const uint8_t &event_value,const QDateTime &currentDateTime)
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
    if(oldEvents.oldEventList.size()==00)
    {
        errorOutput("Not event in queue to remove");
        return;
    }
    oldEvents.oldEventList.erase(oldEvents.oldEventList.begin());
    if(oldEvents.oldEventList.size()>0)
        oldEvents.time=QDateTime::currentDateTime();
}

bool Client::learnSkill(const uint32_t &monsterId, const uint16_t &skill)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("learnSkill("+std::to_string(monsterId)+","+std::to_string(skill)+")");
    #endif
    CommonMap *map=this->map;
    uint8_t x=this->x;
    uint8_t y=this->y;
    Direction direction=getLastDirection();
    switch(getLastDirection())
    {
        case Direction_look_at_top:
        case Direction_look_at_right:
        case Direction_look_at_bottom:
        case Direction_look_at_left:
            direction=lookToMove(direction);
            if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                {
                    errorOutput("learnSkill() Can't move at top from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                    return false;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return false;
            }
        break;
        default:
        errorOutput("Wrong direction to use a learn skill");
        return false;
    }
    const MapServer * const mapServer=static_cast<MapServer*>(map);
    const std::pair<uint8_t,uint8_t> pos(x,y);
    if(mapServer->learn.find(pos)==mapServer->learn.end())
    {
        switch(direction)
        {
            case Direction_look_at_top:
            case Direction_look_at_right:
            case Direction_look_at_bottom:
            case Direction_look_at_left:
                if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
                {
                    if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                    {
                        errorOutput("learnSkill() Can't move at top from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                        return false;
                    }
                }
                else
                {
                    errorOutput("No valid map in this direction");
                    return false;
                }
            break;
            default:
            break;
        }
        if(mapServer->learn.find(pos)==mapServer->learn.end())
        {
            errorOutput("not learn skill into this direction");
            return false;
        }
    }
    return learnSkillInternal(monsterId,skill);
}

bool Client::otherPlayerIsInRange(Client * otherPlayer)
{
    if(getMap()==NULL)
        return false;
    return getMap()==otherPlayer->getMap();
}

void Client::destroyObject(const uint16_t &itemId,const uint32_t &quantity)
{
    normalOutput("The player have destroy them self "+std::to_string(quantity)+" item(s) with id: "+std::to_string(itemId));
    removeObject(itemId,quantity);
}

bool Client::useObjectOnMonster(const uint16_t &object,const uint32_t &monster)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("use the object: "+std::to_string(object)+" on monster "+std::to_string(monster));
    #endif
    if(public_and_private_informations.items.find(object)==public_and_private_informations.items.cend())
    {
        errorOutput("can't use the object: "+std::to_string(object)+" because don't have into the inventory");
        return false;
    }
    if(objectQuantity(object)<1)
    {
        errorOutput("have not quantity to use this object: "+std::to_string(object));
        return false;
    }
    if(CommonFightEngine::useObjectOnMonster(object,monster))
    {
        if(CommonDatapack::commonDatapack.items.item.at(object).consumeAtUse)
            removeObject(object);
    }
    return true;
}

void Client::useObject(const uint8_t &query_id,const uint16_t &itemId)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("use the object: "+std::to_string(itemId));
    #endif
    if(public_and_private_informations.items.find(itemId)==public_and_private_informations.items.cend())
    {
        errorOutput("can't use the object: "+std::to_string(itemId)+" because don't have into the inventory");
        return;
    }
    if(objectQuantity(itemId)<1)
    {
        errorOutput("have not quantity to use this object: "+std::to_string(itemId)+" because recipe already registred");
        return;
    }
    if(CommonDatapack::commonDatapack.items.item.at(itemId).consumeAtUse)
        removeObject(itemId);
    //if is crafting recipe
    if(CommonDatapack::commonDatapack.itemToCrafingRecipes.find(itemId)!=CommonDatapack::commonDatapack.itemToCrafingRecipes.cend())
    {
        const uint32_t &recipeId=CommonDatapack::commonDatapack.itemToCrafingRecipes.at(itemId);
        if(public_and_private_informations.recipes.find(recipeId)!=public_and_private_informations.recipes.cend())
        {
            errorOutput("can't use the object: "+std::to_string(itemId));
            return;
        }
        if(!haveReputationRequirements(CommonDatapack::commonDatapack.crafingRecipes.at(recipeId).requirements.reputation))
        {
            errorOutput("The player have not the requirement: "+std::to_string(recipeId)+" to to learn crafting recipe");
            return;
        }
        public_and_private_informations.recipes.insert(recipeId);
        //send the network reply
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (uint8_t)ObjectUsage_correctlyUsed;
        postReply(query_id,outputData.constData(),outputData.size());
        //add into db
        std::string queryText=PreparedDBQueryCommon::db_query_insert_recipe;
        stringreplace(queryText,"%1",std::to_string(character_id));
        stringreplace(queryText,"%2",std::to_string(recipeId));
        dbQueryWriteCommon(queryText);
    }
    //use trap into fight
    else if(CommonDatapack::commonDatapack.items.trap.find(itemId)!=CommonDatapack::commonDatapack.items.trap.cend())
    {
        if(!isInFight())
        {
            errorOutput("is not in fight to use trap: "+std::to_string(itemId));
            return;
        }
        if(!isInFightWithWild())
        {
            errorOutput("is not in fight with wild to use trap: "+std::to_string(itemId));
            return;
        }
        const uint32_t &maxMonsterId=tryCapture(itemId);
        //send the network reply
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (uint8_t)ObjectUsage_correctlyUsed;
        if(maxMonsterId>0)
            out << (uint32_t)maxMonsterId;
        else
            out << (uint32_t)0x00000000;
        postReply(query_id,outputData.constData(),outputData.size());
    }
    //use repel into fight
    else if(CommonDatapack::commonDatapack.items.repel.find(itemId)!=CommonDatapack::commonDatapack.items.repel.cend())
    {
        public_and_private_informations.repel_step+=CommonDatapack::commonDatapack.items.repel.at(itemId);
        //send the network reply
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (uint8_t)ObjectUsage_correctlyUsed;
        postReply(query_id,outputData.constData(),outputData.size());
    }
    else
    {
        errorOutput("can't use the object: "+std::to_string(itemId)+" because don't have an usage");
        return;
    }
}

void Client::receiveTeleportTo(CommonMap *map,const /*COORD_TYPE*/uint8_t &x,const /*COORD_TYPE*/uint8_t &y,const Orientation &orientation)
{
    teleportTo(map,x,y,orientation);
}

void Client::teleportValidatedTo(CommonMap *map,const /*COORD_TYPE*/uint8_t &x,const /*COORD_TYPE*/uint8_t &y,const Orientation &orientation)
{
    normalOutput("teleportValidatedTo("+map->map_file+
                                    ","+std::to_string(x)+
                                    ","+std::to_string(y)+
                                    ","+std::to_string((uint8_t)orientation)+")");
    #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    bool mapChange=this->map!=map;
    if(mapChange)
        removeNearPlant();
    #endif
    MapBasicMove::teleportValidatedTo(map,x,y,orientation);
    if(GlobalServerData::serverSettings.positionTeleportSync)
        savePosition();
    #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    if(mapChange)
        sendNearPlant();
    #endif
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

void Client::getShopList(const uint8_t &query_id,const uint16_t &shopId)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("getShopList("+std::to_string(query_id)+","+std::to_string(shopId)+")");
    #endif
    if(CommonDatapackServerSpec::commonDatapackServerSpec.shops.find(shopId)==CommonDatapackServerSpec::commonDatapackServerSpec.shops.cend())
    {
        errorOutput("shopId not found: "+std::to_string(shopId));
        return;
    }
    CommonMap *map=this->map;
    uint8_t x=this->x;
    uint8_t y=this->y;
    //resolv the object
    Direction direction=getLastDirection();
    switch(direction)
    {
        case Direction_look_at_top:
        case Direction_look_at_right:
        case Direction_look_at_bottom:
        case Direction_look_at_left:
            direction=lookToMove(direction);
            if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                {
                    errorOutput("getShopList() Can't move at this direction from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                    return;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return;
            }
        break;
        default:
        errorOutput("Wrong direction to use a shop");
        return;
    }
    const MapServer * const mapServer=static_cast<MapServer*>(map);
    const std::pair<uint8_t,uint8_t> pos(x,y);
    //check if is shop
    if(mapServer->shops.find(pos)==mapServer->shops.cend())
    {
        const std::vector<uint32_t> shops=mapServer->shops.at(pos);
        const int indexOf=vectorindexOf(shops,shopId);
        if(indexOf==-1)
        {
            switch(direction)
            {
                case Direction_look_at_top:
                case Direction_look_at_right:
                case Direction_look_at_bottom:
                case Direction_look_at_left:
                    if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
                    {
                        if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                        {
                            errorOutput("getShopList() Can't move at this direction from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                            return;
                        }
                    }
                    else
                    {
                        errorOutput("No valid map in this direction");
                        return;
                    }
                break;
                default:
                break;
            }
            {
                const MapServer * const mapServer=static_cast<MapServer*>(map);
                const std::pair<uint8_t,uint8_t> pos(x,y);
                if(mapServer->shops.find(pos)==mapServer->shops.cend())
                {
                    const std::vector<uint32_t> shops=mapServer->shops.at(pos);
                    const int indexOf=vectorindexOf(shops,shopId);
                    if(indexOf==-1)
                    {
                        errorOutput("not shop into this direction");
                        return;
                    }
                }
            }
        }
    }
    //send the shop items (no taxes from now)
    const Shop &shop=CommonDatapackServerSpec::commonDatapackServerSpec.shops.at(shopId);
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    QByteArray outputData2;
    QDataStream out2(&outputData2, QIODevice::WriteOnly);
    out2.setVersion(QDataStream::Qt_4_4);
    unsigned int index=0;
    unsigned int objectCount=0;
    while(index<shop.items.size())
    {
        if(shop.prices.at(index)>0)
        {
            if(shop.prices.at(index)>0)
            {
                out2 << (uint16_t)shop.items.at(index);
                out2 << (uint32_t)shop.prices.at(index);
                out2 << (uint32_t)0;
                objectCount++;
            }
        }
        index++;
    }
    out << (uint16_t)objectCount;
    const QByteArray outputNew(outputData+outputData2);
    postReply(query_id,outputNew.constData(),outputNew.size());
}

void Client::buyObject(const uint8_t &query_id,const uint16_t &shopId,const uint16_t &objectId,const uint32_t &quantity,const uint32_t &price)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("buyObject("+std::to_string(query_id)+","+std::to_string(shopId)+")");
    #endif
    if(CommonDatapackServerSpec::commonDatapackServerSpec.shops.find(shopId)==CommonDatapackServerSpec::commonDatapackServerSpec.shops.cend())
    {
        errorOutput("shopId not found: "+std::to_string(shopId));
        return;
    }
    if(quantity<=0)
    {
        errorOutput("quantity wrong: "+std::to_string(quantity));
        return;
    }
    CommonMap *map=this->map;
    uint8_t x=this->x;
    uint8_t y=this->y;
    //resolv the object
    Direction direction=getLastDirection();
    switch(direction)
    {
        case Direction_look_at_top:
        case Direction_look_at_right:
        case Direction_look_at_bottom:
        case Direction_look_at_left:
            direction=lookToMove(direction);
            if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                {
                    errorOutput("buyObject() Can't move at this direction from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                    return;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return;
            }
        break;
        default:
        errorOutput("Wrong direction to use a shop");
        return;
    }
    const MapServer * const mapServer=static_cast<MapServer*>(map);
    const std::pair<uint8_t,uint8_t> pos(x,y);
    //check if is shop
    if(mapServer->shops.find(pos)==mapServer->shops.cend())
    {
        const std::vector<uint32_t> shops=mapServer->shops.at(pos);
        const int indexOf=vectorindexOf(shops,shopId);
        if(indexOf==-1)
        {
            Direction direction=getLastDirection();
            switch(direction)
            {
                case Direction_look_at_top:
                case Direction_look_at_right:
                case Direction_look_at_bottom:
                case Direction_look_at_left:
                    direction=lookToMove(direction);
                    if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
                    {
                        if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                        {
                            errorOutput("buyObject() Can't move at this direction from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                            return;
                        }
                    }
                    else
                    {
                        errorOutput("No valid map in this direction");
                        return;
                    }
                break;
                default:
                errorOutput("Wrong direction to use a shop");
                return;
            }
            const MapServer * const mapServer=static_cast<MapServer*>(map);
            const std::pair<uint8_t,uint8_t> pos(x,y);
            if(mapServer->shops.find(pos)==mapServer->shops.cend())
            {
                const std::vector<uint32_t> shops=mapServer->shops.at(pos);
                const int indexOf=vectorindexOf(shops,shopId);
                if(indexOf==-1)
                {
                    errorOutput("not shop into this direction");
                    return;
                }
            }
        }
    }
    //send the shop items (no taxes from now)
    const int &priceIndex=vectorindexOf(CommonDatapackServerSpec::commonDatapackServerSpec.shops.at(shopId).items,objectId);
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    if(priceIndex==-1)
    {
        out << (uint8_t)BuyStat_HaveNotQuantity;
        postReply(query_id,outputData.constData(),outputData.size());
        return;
    }
    const uint32_t &realprice=CommonDatapackServerSpec::commonDatapackServerSpec.shops.at(shopId).prices.at(priceIndex);
    if(realprice==0)
    {
        out << (uint8_t)BuyStat_HaveNotQuantity;
        postReply(query_id,outputData.constData(),outputData.size());
        return;
    }
    if(realprice>price)
    {
        out << (uint8_t)BuyStat_PriceHaveChanged;
        postReply(query_id,outputData.constData(),outputData.size());
        return;
    }
    if(realprice<price)
    {
        out << (uint8_t)BuyStat_BetterPrice;
        out << (uint32_t)price;
    }
    else
        out << (uint8_t)BuyStat_Done;
    if(public_and_private_informations.cash>=(realprice*quantity))
        removeCash(realprice*quantity);
    else
    {
        errorOutput("The player have not the cash to buy "+std::to_string(quantity)+" item of id: "+std::to_string(objectId));
        return;
    }
    addObject(objectId,quantity);
    postReply(query_id,outputData.constData(),outputData.size());
}

void Client::sellObject(const uint8_t &query_id,const uint16_t &shopId,const uint16_t &objectId,const uint32_t &quantity,const uint32_t &price)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("sellObject("+std::to_string(query_id)+","+std::to_string(shopId)+")");
    #endif
    if(CommonDatapackServerSpec::commonDatapackServerSpec.shops.find(shopId)==CommonDatapackServerSpec::commonDatapackServerSpec.shops.cend())
    {
        errorOutput("shopId not found: "+std::to_string(shopId));
        return;
    }
    if(quantity<=0)
    {
        errorOutput("quantity wrong: "+std::to_string(quantity));
        return;
    }
    CommonMap *map=this->map;
    uint8_t x=this->x;
    uint8_t y=this->y;
    //resolv the object
    Direction direction=getLastDirection();
    switch(direction)
    {
        case Direction_look_at_top:
        case Direction_look_at_right:
        case Direction_look_at_bottom:
        case Direction_look_at_left:
            direction=lookToMove(direction);
            if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                {
                    errorOutput("sellObject() Can't move at this direction from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                    return;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return;
            }
        break;
        default:
        errorOutput("Wrong direction to use a shop");
        return;
    }
    const MapServer * const mapServer=static_cast<MapServer*>(map);
    const std::pair<uint8_t,uint8_t> pos(x,y);
    //check if is shop
    if(mapServer->shops.find(pos)==mapServer->shops.cend())
    {
        const std::vector<uint32_t> shops=mapServer->shops.at(pos);
        const int indexOf=vectorindexOf(shops,shopId);
        if(indexOf==-1)
        {
            Direction direction=getLastDirection();
            switch(direction)
            {
                case Direction_look_at_top:
                case Direction_look_at_right:
                case Direction_look_at_bottom:
                case Direction_look_at_left:
                    direction=lookToMove(direction);
                    if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
                    {
                        if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                        {
                            errorOutput("sellObject() Can't move at this direction from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                            return;
                        }
                    }
                    else
                    {
                        errorOutput("No valid map in this direction");
                        return;
                    }
                break;
                default:
                errorOutput("Wrong direction to use a shop");
                return;
            }
            const MapServer * const mapServer=static_cast<MapServer*>(map);
            const std::pair<uint8_t,uint8_t> pos(x,y);
            if(mapServer->shops.find(pos)==mapServer->shops.cend())
            {
                const std::vector<uint32_t> shops=mapServer->shops.at(pos);
                const int indexOf=vectorindexOf(shops,shopId);
                if(indexOf==-1)
                {
                    errorOutput("not shop into this direction");
                    return;
                }
            }
        }
    }
    //send the shop items (no taxes from now)
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    if(CommonDatapack::commonDatapack.items.item.find(objectId)==CommonDatapack::commonDatapack.items.item.cend())
    {
        errorOutput("this item don't exists");
        return;
    }
    if(objectQuantity(objectId)<quantity)
    {
        errorOutput("you have not this quantity to sell");
        return;
    }
    if(CommonDatapack::commonDatapack.items.item.at(objectId).price==0)
    {
        errorOutput("Can't sold %1"+std::to_string(objectId));
        return;
    }
    const uint32_t &realPrice=CommonDatapack::commonDatapack.items.item.at(objectId).price/2;
    if(realPrice<price)
    {
        out << (uint8_t)SoldStat_PriceHaveChanged;
        postReply(query_id,outputData.constData(),outputData.size());
        return;
    }
    if(realPrice>price)
    {
        out << (uint8_t)SoldStat_BetterPrice;
        out << (uint32_t)realPrice;
    }
    else
        out << (uint8_t)SoldStat_Done;
    removeObject(objectId,quantity);
    addCash(realPrice*quantity);
    postReply(query_id,outputData.constData(),outputData.size());
}

void Client::saveIndustryStatus(const uint32_t &factoryId,const IndustryStatus &industryStatus,const Industry &industry)
{
    std::vector<std::string> resourcesStringList,productsStringList;
    unsigned int index;
    //send the resource
    index=0;
    while(index<industry.resources.size())
    {
        const Industry::Resource &resource=industry.resources.at(index);
        const uint32_t &quantityInStock=industryStatus.resources.at(resource.item);
        resourcesStringList.push_back(std::to_string(resource.item)+':'+std::to_string(quantityInStock));
        index++;
    }
    //send the product
    index=0;
    while(index<industry.products.size())
    {
        const Industry::Product &product=industry.products.at(index);
        const uint32_t &quantityInStock=industryStatus.products.at(product.item);
        productsStringList.push_back(std::to_string(product.item)+':'+std::to_string(quantityInStock));
        index++;
    }

    //save in db
    if(GlobalServerData::serverPrivateVariables.industriesStatus.find(factoryId)==GlobalServerData::serverPrivateVariables.industriesStatus.cend())
    {
        std::string queryText=PreparedDBQueryServer::db_query_insert_factory;
        stringreplace(queryText,"%1",std::to_string(factoryId));
        stringreplace(queryText,"%2",stringimplode(resourcesStringList,';'));
        stringreplace(queryText,"%3",stringimplode(productsStringList,';'));
        stringreplace(queryText,"%4",std::to_string(industryStatus.last_update));
        dbQueryWriteServer(queryText);
    }
    else
    {
        std::string queryText=PreparedDBQueryServer::db_query_update_factory;
        stringreplace(queryText,"%1",std::to_string(factoryId));
        stringreplace(queryText,"%2",stringimplode(resourcesStringList,';'));
        stringreplace(queryText,"%3",stringimplode(productsStringList,';'));
        stringreplace(queryText,"%4",std::to_string(industryStatus.last_update));
        dbQueryWriteServer(queryText);
    }
    GlobalServerData::serverPrivateVariables.industriesStatus[factoryId]=industryStatus;
}

void Client::getFactoryList(const uint8_t &query_id, const uint16_t &factoryId)
{
    if(isInFight())
    {
        errorOutput("Try do inventory action when is in fight");
        return;
    }
    if(captureCityInProgress())
    {
        errorOutput("Try do inventory action when is in capture city");
        return;
    }
    if(CommonDatapack::commonDatapack.industriesLink.find(factoryId)==CommonDatapack::commonDatapack.industriesLink.cend())
    {
        errorOutput("factory id not found");
        return;
    }
    const Industry &industry=CommonDatapack::commonDatapack.industries.at(CommonDatapack::commonDatapack.industriesLink.at(factoryId).industry);
    //send the shop items (no taxes from now)
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    if(GlobalServerData::serverPrivateVariables.industriesStatus.find(factoryId)==GlobalServerData::serverPrivateVariables.industriesStatus.cend())
    {
        out << (uint32_t)0;
        out << (uint16_t)industry.resources.size();
        unsigned int index=0;
        while(index<industry.resources.size())
        {
            const Industry::Resource &resource=industry.resources.at(index);
            out << (uint16_t)resource.item;
            out << (uint32_t)CommonDatapack::commonDatapack.items.item.at(resource.item).price*(100+CATCHCHALLENGER_SERVER_FACTORY_PRICE_CHANGE)/100;
            out << (uint32_t)resource.quantity*industry.cycletobefull;
            index++;
        }
        out << (uint16_t)0x0000;//no product do
    }
    else
    {
        unsigned int index,count_item;
        const IndustryStatus &industryStatus=FacilityLib::industryStatusWithCurrentTime(GlobalServerData::serverPrivateVariables.industriesStatus.at(factoryId),industry);
        quint64 currentTime=QDateTime::currentMSecsSinceEpoch()/1000;
        if(industryStatus.last_update>currentTime)
            out << (uint32_t)0;
        else if((currentTime-industryStatus.last_update)>industry.time)
            out << (uint32_t)0;
        else
            out << (uint32_t)(industry.time-(currentTime-industryStatus.last_update));
        //send the resource
        count_item=0;
        index=0;
        while(index<industry.resources.size())
        {
            const Industry::Resource &resource=industry.resources.at(index);
            const uint32_t &quantityInStock=industryStatus.resources.at(resource.item);
            if(quantityInStock<resource.quantity*industry.cycletobefull)
                count_item++;
            index++;
        }
        out << (uint16_t)count_item;
        index=0;
        while(index<industry.resources.size())
        {
            const Industry::Resource &resource=industry.resources.at(index);
            const uint32_t &quantityInStock=industryStatus.resources.at(resource.item);
            if(quantityInStock<resource.quantity*industry.cycletobefull)
            {
                out << (uint16_t)resource.item;
                out << (uint32_t)FacilityLib::getFactoryResourcePrice(quantityInStock,resource,industry);
                out << (uint32_t)resource.quantity*industry.cycletobefull-quantityInStock;
            }
            index++;
        }
        //send the product
        count_item=0;
        index=0;
        while(index<industry.products.size())
        {
            const Industry::Product &product=industry.products.at(index);
            const uint32_t &quantityInStock=industryStatus.products.at(product.item);
            if(quantityInStock>0)
                count_item++;
            index++;
        }
        out << (uint16_t)count_item;
        index=0;
        while(index<industry.products.size())
        {
            const Industry::Product &product=industry.products.at(index);
            const uint32_t &quantityInStock=industryStatus.products.at(product.item);
            if(quantityInStock>0)
            {
                out << (uint16_t)product.item;
                out << (uint32_t)FacilityLib::getFactoryProductPrice(quantityInStock,product,industry);
                out << (uint32_t)quantityInStock;
            }
            index++;
        }
    }
    postReply(query_id,outputData.constData(),outputData.size());
}

void Client::buyFactoryProduct(const uint8_t &query_id,const uint16_t &factoryId,const uint16_t &objectId,const uint32_t &quantity,const uint32_t &price)
{
    if(isInFight())
    {
        errorOutput("Try do inventory action when is in fight");
        return;
    }
    if(captureCityInProgress())
    {
        errorOutput("Try do inventory action when is in capture city");
        return;
    }
    if(CommonDatapack::commonDatapack.industriesLink.find(factoryId)==CommonDatapack::commonDatapack.industriesLink.cend())
    {
        errorOutput("factory id not found");
        return;
    }
    if(CommonDatapack::commonDatapack.items.item.find(objectId)==CommonDatapack::commonDatapack.items.item.cend())
    {
        errorOutput("object id not found into the factory product list");
        return;
    }
    if(GlobalServerData::serverPrivateVariables.industriesStatus.find(factoryId)==GlobalServerData::serverPrivateVariables.industriesStatus.cend())
    {
        errorOutput("factory id not found in active list");
        return;
    }
    if(!haveReputationRequirements(CommonDatapack::commonDatapack.industriesLink.at(factoryId).requirements.reputation))
    {
        errorOutput("The player have not the requirement: "+std::to_string(factoryId)+" to use the factory");
        return;
    }
    const Industry &industry=CommonDatapack::commonDatapack.industries.at(CommonDatapack::commonDatapack.industriesLink.at(factoryId).industry);
    IndustryStatus industryStatus=FacilityLib::industryStatusWithCurrentTime(GlobalServerData::serverPrivateVariables.industriesStatus.at(factoryId),industry);
    uint32_t quantityInStock=0;
    uint32_t actualPrice=0;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    Industry::Product product;
    //get the right product
    {
        unsigned int index=0;
        while(index<industry.products.size())
        {
            product=industry.products.at(index);
            if(product.item==objectId)
            {
                quantityInStock=industryStatus.products.at(product.item);
                actualPrice=FacilityLib::getFactoryProductPrice(quantityInStock,product,industry);
                break;
            }
            index++;
        }
        if(index==industry.products.size())
        {
            errorOutput("internal bug, product for the factory not found");
            return;
        }
    }
    if(public_and_private_informations.cash<(actualPrice*quantity))
    {
        errorOutput("have not the cash to buy into this factory");
        return;
    }
    if(quantity>quantityInStock)
    {
        out << (uint8_t)0x03;
        postReply(query_id,outputData.constData(),outputData.size());
        return;
    }
    if(actualPrice>price)
    {
        out << (uint8_t)0x04;
        postReply(query_id,outputData.constData(),outputData.size());
        return;
    }
    if(actualPrice==price)
        out << (uint8_t)0x01;
    else
    {
        out << (uint8_t)0x02;
        out << (uint32_t)actualPrice;
    }
    quantityInStock-=quantity;
    if(quantityInStock==(product.item*industry.cycletobefull))
    {
        industryStatus.products[product.item]=quantityInStock;
        industryStatus=FacilityLib::factoryCheckProductionStart(industryStatus,industry);
    }
    else
        industryStatus.products[product.item]=quantityInStock;
    removeCash(actualPrice*quantity);
    saveIndustryStatus(factoryId,industryStatus,industry);
    addObject(objectId,quantity);
    appendReputationRewards(CommonDatapack::commonDatapack.industriesLink.at(factoryId).rewards.reputation);
    postReply(query_id,outputData.constData(),outputData.size());
}

void Client::sellFactoryResource(const uint8_t &query_id,const uint16_t &factoryId,const uint16_t &objectId,const uint32_t &quantity,const uint32_t &price)
{
    if(isInFight())
    {
        errorOutput("Try do inventory action when is in fight");
        return;
    }
    if(captureCityInProgress())
    {
        errorOutput("Try do inventory action when is in capture city");
        return;
    }
    if(CommonDatapack::commonDatapack.industriesLink.find(factoryId)==CommonDatapack::commonDatapack.industriesLink.cend())
    {
        errorOutput("factory id not found");
        return;
    }
    if(CommonDatapack::commonDatapack.items.item.find(objectId)==CommonDatapack::commonDatapack.items.item.cend())
    {
        errorOutput("object id not found");
        return;
    }
    if(objectQuantity(objectId)<quantity)
    {
        errorOutput("you have not the object quantity to sell at this factory");
        return;
    }
    if(!haveReputationRequirements(CommonDatapack::commonDatapack.industriesLink.at(factoryId).requirements.reputation))
    {
        errorOutput("The player have not the requirement: "+std::to_string(factoryId)+" to use the factory");
        return;
    }
    const Industry &industry=CommonDatapack::commonDatapack.industries.at(CommonDatapack::commonDatapack.industriesLink.at(factoryId).industry);
    IndustryStatus industryStatus;
    if(GlobalServerData::serverPrivateVariables.industriesStatus.find(factoryId)==GlobalServerData::serverPrivateVariables.industriesStatus.cend())
    {
        industryStatus.last_update=(QDateTime::currentMSecsSinceEpoch()/1000);
        unsigned int index;
        //send the resource
        index=0;
        while(index<industry.resources.size())
        {
            const Industry::Resource &resource=industry.resources.at(index);
            industryStatus.resources[resource.item]=0;
            index++;
        }
        //send the product
        index=0;
        while(index<industry.products.size())
        {
            const Industry::Product &product=industry.products.at(index);
            industryStatus.products[product.item]=0;
            index++;
        }
    }
    else
        industryStatus=FacilityLib::industryStatusWithCurrentTime(GlobalServerData::serverPrivateVariables.industriesStatus.at(factoryId),industry);
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    uint32_t resourcePrice;
    //check if not overfull
    {
        unsigned int index=0;
        while(index<industry.resources.size())
        {
            const Industry::Resource &resource=industry.resources.at(index);
            if(resource.item==objectId)
            {
                if((resource.quantity*industry.cycletobefull-industryStatus.resources.at(resource.item))<quantity)
                {
                    out << (uint8_t)0x03;
                    postReply(query_id,outputData.constData(),outputData.size());
                    return;
                }
                resourcePrice=FacilityLib::getFactoryResourcePrice(industryStatus.resources.at(resource.item),resource,industry);
                if(price>resourcePrice)
                {
                    out << (uint8_t)0x04;
                    postReply(query_id,outputData.constData(),outputData.size());
                    return;
                }
                if((industryStatus.resources.at(resource.item)+quantity)==resource.quantity)
                {
                    industryStatus.resources[resource.item]+=quantity;
                    industryStatus=FacilityLib::factoryCheckProductionStart(industryStatus,industry);
                }
                else
                    industryStatus.resources[resource.item]+=quantity;
                break;
            }
            index++;
        }
        if(index==industry.resources.size())
        {
            errorOutput("internal bug, resource for the factory not found");
            return;
        }
    }
    if(price==resourcePrice)
        out << (uint8_t)0x01;
    else
    {
        out << (uint8_t)0x02;
        out << (uint32_t)resourcePrice;
    }
    removeObject(objectId,quantity);
    addCash(resourcePrice*quantity);
    saveIndustryStatus(factoryId,industryStatus,industry);
    appendReputationRewards(CommonDatapack::commonDatapack.industriesLink.at(factoryId).rewards.reputation);
    postReply(query_id,outputData.constData(),outputData.size());
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
    std::string queryText=PreparedDBQueryCommon::db_query_insert_character_allow;
    stringreplace(queryText,"%1",std::to_string(character_id));
    stringreplace(queryText,"%2",std::to_string(DictionaryLogin::dictionary_allow_internal_to_database.at(allow)));
    dbQueryWriteCommon(queryText);
}

void Client::removeAllow(const ActionAllow &allow)
{
    if(public_and_private_informations.allow.find(allow)==public_and_private_informations.allow.cend())
        return;
    public_and_private_informations.allow.erase(allow);
    std::string queryText=PreparedDBQueryCommon::db_query_delete_character_allow;
    stringreplace(queryText,"%1",std::to_string(character_id));
    stringreplace(queryText,"%2",std::to_string(DictionaryLogin::dictionary_allow_internal_to_database.at(allow)));
    dbQueryWriteCommon(queryText);
}

void Client::appendReputationRewards(const std::vector<ReputationRewards> &reputationList)
{
    unsigned int index=0;
    while(index<reputationList.size())
    {
        const ReputationRewards &reputationRewards=reputationList.at(index);
        appendReputationPoint(reputationRewards.reputationId,reputationRewards.point);
        index++;
    }
}

//reputation
void Client::appendReputationPoint(const uint8_t &reputationId, const int32_t &point)
{
    if(point==0)
        return;
    const Reputation &reputation=CommonDatapack::commonDatapack.reputation.at(reputationId);
    bool isNewReputation=false;
    PlayerReputation *playerReputation=NULL;
    //search
    {
        if(public_and_private_informations.reputation.find(reputationId)==public_and_private_informations.reputation.cend())
        {
            PlayerReputation temp;
            temp.point=0;
            temp.level=0;
            isNewReputation=true;
            public_and_private_informations.reputation[reputationId]=temp;
        }
    }
    playerReputation=&public_and_private_informations.reputation[reputationId];

    #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
    normalOutput("Reputation "+std::to_string(reputationId)+" at level: "+std::to_string(playerReputation->level)+" with point: "+std::to_string(playerReputation->point));
    #endif
    FacilityLib::appendReputationPoint(playerReputation,point,reputation);
    if(isNewReputation)
    {
        std::string queryText=PreparedDBQueryCommon::db_query_insert_reputation;
        stringreplace(queryText,"%1",std::to_string(character_id));
        stringreplace(queryText,"%2",std::to_string(reputation.reverse_database_id));
        stringreplace(queryText,"%3",std::to_string(playerReputation->point));
        stringreplace(queryText,"%4",std::to_string(playerReputation->level));
        dbQueryWriteCommon(queryText);
    }
    else
    {
        std::string queryText=PreparedDBQueryCommon::db_query_update_reputation;
        stringreplace(queryText,"%1",std::to_string(character_id));
        stringreplace(queryText,"%2",std::to_string(reputation.reverse_database_id));
        stringreplace(queryText,"%3",std::to_string(playerReputation->point));
        stringreplace(queryText,"%4",std::to_string(playerReputation->level));
        dbQueryWriteCommon(queryText);
    }
}

void Client::heal()
{
    if(isInFight())
    {
        errorOutput("Try do heal action when is in fight");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("ask heal at "+this->map->map_file+" ("+std::to_string(this->x)+","+std::to_string(this->y)+")");
    #endif
    CommonMap *map=this->map;
    uint8_t x=this->x;
    uint8_t y=this->y;
    //resolv the object
    Direction direction=getLastDirection();
    switch(direction)
    {
        case Direction_look_at_top:
        case Direction_look_at_right:
        case Direction_look_at_bottom:
        case Direction_look_at_left:
            direction=lookToMove(direction);
            if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                {
                    errorOutput("heal() Can't move at this direction from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                    return;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return;
            }
        break;
        default:
        errorOutput("Wrong direction to use a heal");
        return;
    }
    const MapServer * const mapServer=static_cast<MapServer*>(map);
    const std::pair<uint8_t,uint8_t> pos(x,y);
    //check if is shop
    if(mapServer->heal.find(pos)==mapServer->heal.cend())
    {
        Direction direction=getLastDirection();
        switch(direction)
        {
            case Direction_look_at_top:
            case Direction_look_at_right:
            case Direction_look_at_bottom:
            case Direction_look_at_left:
                direction=lookToMove(direction);
                if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
                {
                    if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                    {
                        errorOutput("heal() Can't move at this direction from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                        return;
                    }
                }
                else
                {
                    errorOutput("No valid map in this direction");
                    return;
                }
            break;
            default:
            errorOutput("Wrong direction to use a heal");
            return;
        }
        const MapServer * const mapServer=static_cast<MapServer*>(map);
        const std::pair<uint8_t,uint8_t> pos(x,y);
        if(mapServer->heal.find(pos)==mapServer->heal.cend())
        {
            errorOutput("no heal point in this direction");
            return;
        }
    }
    //send the shop items (no taxes from now)
    healAllMonsters();
    rescue=unvalidated_rescue;
}

void Client::requestFight(const uint16_t &fightId)
{
    if(isInFight())
    {
        errorOutput("error: map: "+this->map->map_file+" ("+std::to_string(this->x)+","+std::to_string(this->y)+"), is in fight");
        return;
    }
    if(captureCityInProgress())
    {
        errorOutput("Try requestFight when is in capture city");
        return;
    }
    if(public_and_private_informations.bot_already_beaten.find(fightId)!=public_and_private_informations.bot_already_beaten.cend())
    {
        errorOutput("You can't rebeat this fighter");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("request fight at "+this->map->map_file+" ("+std::to_string(this->x)+","+std::to_string(this->y)+")");
    #endif
    CommonMap *map=this->map;
    uint8_t x=this->x;
    uint8_t y=this->y;
    //resolv the object
    Direction direction=getLastDirection();
    switch(direction)
    {
        case Direction_look_at_top:
        case Direction_look_at_right:
        case Direction_look_at_bottom:
        case Direction_look_at_left:
            direction=lookToMove(direction);
            if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                {
                    errorOutput("requestFight() Can't move at this direction from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                    return;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return;
            }
        break;
        default:
        errorOutput("Wrong direction to use a shop");
        return;
    }
    const MapServer * const mapServer=static_cast<MapServer*>(map);
    const std::pair<uint8_t,uint8_t> pos(x,y);
    //check if is shop
    bool found=false;
    if(mapServer->botsFight.find(pos)!=mapServer->botsFight.cend())
    {
        const std::vector<uint32_t> &botsFightList=mapServer->botsFight.at(pos);
        if(vectorcontains(botsFightList,fightId))
            found=true;
    }
    if(!found)
    {
        Direction direction=getLastDirection();
        switch(direction)
        {
            case Direction_look_at_top:
            case Direction_look_at_right:
            case Direction_look_at_bottom:
            case Direction_look_at_left:
                direction=lookToMove(direction);
                if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
                {
                    if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                    {
                        errorOutput("requestFight() Can't move at this direction from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                        return;
                    }
                }
                else
                {
                    errorOutput("No valid map in this direction");
                    return;
                }
            break;
            default:
            errorOutput("Wrong direction to use a shop");
            return;
        }
        const MapServer * const mapServer=static_cast<MapServer*>(map);
        const std::pair<uint8_t,uint8_t> pos(x,y);
        if(mapServer->botsFight.find(pos)!=mapServer->botsFight.cend())
        {
            const std::vector<uint32_t> &botsFightList=static_cast<MapServer*>(this->map)->botsFight.at(pos);
            if(vectorcontains(botsFightList,fightId))
                found=true;
        }
        if(!found)
        {
            errorOutput("no fight with id "+std::to_string(fightId)+" in this direction");
            return;
        }
    }
    normalOutput("is now in fight (after a request) on map "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+") with the bot "+std::to_string(fightId));
    botFightStart(fightId);
}

void Client::clanAction(const uint8_t &query_id,const uint8_t &action,const std::string &text)
{
    switch(action)
    {
        //create
        case 0x01:
        {
            if(public_and_private_informations.clan>0)
            {
                errorOutput("You are already in clan");
                return;
            }
            if(text.size()==0)
            {
                errorOutput("You can't create clan with empty name");
                return;
            }
            if(public_and_private_informations.allow.find(ActionAllow_Clan)==public_and_private_informations.allow.cend())
            {
                errorOutput("You have not the right to create clan");
                return;
            }
            ClanActionParam *clanActionParam=new ClanActionParam();
            clanActionParam->query_id=query_id;
            clanActionParam->action=action;
            clanActionParam->text=text;

            std::string queryText=PreparedDBQueryCommon::db_query_select_clan_by_name;
            stringreplace(queryText,"%1",SqlFunction::quoteSqlVariable(text));
            CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText.c_str(),this,&Client::addClan_static);
            if(callback==NULL)
            {
                std::cerr << "Sql error for: "+queryText+", error: "+GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;

                QByteArray outputData;
                QDataStream out(&outputData, QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
                out << (uint8_t)0x02;
                postReply(query_id,outputData.constData(),outputData.size());
                delete clanActionParam;
                return;
            }
            else
            {
                paramToPassToCallBack.push_back(clanActionParam);
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                paramToPassToCallBackType.push_back("ClanActionParam");
                #endif
                callbackRegistred.push_back(callback);
            }
            return;
        }
        break;
        //leave
        case 0x02:
        {
            if(public_and_private_informations.clan==0)
            {
                errorOutput("You have not a clan");
                return;
            }
            if(public_and_private_informations.clan_leader)
            {
                errorOutput("You can't leave if you are the leader");
                return;
            }
            removeFromClan();
            clanChangeWithoutDb(public_and_private_informations.clan);
            //send the network reply
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
            out << (uint8_t)0x01;
            postReply(query_id,outputData.constData(),outputData.size());
            //update the db
            std::string queryText=PreparedDBQueryCommon::db_query_update_character_clan;
            stringreplace(queryText,"%1",std::to_string(character_id));
            dbQueryWriteCommon(queryText);
        }
        break;
        //dissolve
        case 0x03:
        {
            if(public_and_private_informations.clan==0)
            {
                errorOutput("You have not a clan");
                return;
            }
            if(!public_and_private_informations.clan_leader)
            {
                errorOutput("You are not a leader to dissolve the clan");
                return;
            }
            if(clan->captureCityInProgress.size()>0)
            {
                errorOutput("You can't disolv the clan if is in city capture");
                return;
            }
            const std::vector<Client *> &players=clanList.at(public_and_private_informations.clan)->players;
            //send the network reply
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
            out << (uint8_t)0x01;
            postReply(query_id,outputData.constData(),outputData.size());
            //update the db
            unsigned int index=0;
            while(index<players.size())
            {
                std::string queryText=PreparedDBQueryCommon::db_query_update_character_clan;
                stringreplace(queryText,"%1",std::to_string(players.at(index)->getPlayerId()));
                dbQueryWriteCommon(queryText);
                index++;
            }
            std::string queryText=PreparedDBQueryCommon::db_query_delete_clan;
            stringreplace(queryText,"%1",std::to_string(public_and_private_informations.clan));
            dbQueryWriteCommon(queryText);
            queryText=PreparedDBQueryServer::db_query_delete_city;
            stringreplace(queryText,"%1",clan->capturedCity);
            dbQueryWriteServer(queryText);
            //update the object
            clanList.erase(public_and_private_informations.clan);
            GlobalServerData::serverPrivateVariables.cityStatusListReverse.erase(clan->clanId);
            GlobalServerData::serverPrivateVariables.cityStatusList[clan->captureCityInProgress].clan=0;
            delete clan;
            index=0;
            while(index<players.size())
            {
                if(players.at(index)==this)
                {
                    public_and_private_informations.clan=0;
                    clan=NULL;
                    clanChangeWithoutDb(public_and_private_informations.clan);//to send to another thread the clan change, 0 to remove
                }
                else
                    players.at(index)->dissolvedClan();
                index++;
            }
        }
        break;
        //invite
        case 0x04:
        {
            if(public_and_private_informations.clan==0)
            {
                errorOutput("You have not a clan");
                return;
            }
            if(!public_and_private_informations.clan_leader)
            {
                errorOutput("You are not a leader to invite into the clan");
                return;
            }
            bool haveAClan=true;
            if(playerByPseudo.find(text)!=playerByPseudo.cend())
                if(!playerByPseudo.at(text)->haveAClan())
                    haveAClan=false;
            bool isFound=playerByPseudo.find(text)!=playerByPseudo.cend();
            //send the network reply
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
            if(isFound && !haveAClan)
            {
                if(playerByPseudo.at(text)->inviteToClan(public_and_private_informations.clan))
                    out << (uint8_t)0x01;
                else
                    out << (uint8_t)0x02;
            }
            else
            {
                if(!isFound)
                    normalOutput("Clan invite: Player "+text+" not found, is connected?");
                if(haveAClan)
                    normalOutput("Clan invite: Player "+text+" is already into a clan");
                out << (uint8_t)0x02;
            }
            postReply(query_id,outputData.constData(),outputData.size());
        }
        break;
        //eject
        case 0x05:
        {
            if(public_and_private_informations.clan==0)
            {
                errorOutput("You have not a clan");
                return;
            }
            if(!public_and_private_informations.clan_leader)
            {
                errorOutput("You are not a leader to invite into the clan");
                return;
            }
            if(public_and_private_informations.public_informations.pseudo==text)
            {
                errorOutput("You can't eject your self");
                return;
            }
            bool isIntoTheClan=false;
            if(playerByPseudo.find(text)!=playerByPseudo.cend())
                if(playerByPseudo.at(text)->getClanId()==public_and_private_informations.clan)
                    isIntoTheClan=true;
            bool isFound=playerByPseudo.find(text)!=playerByPseudo.cend();
            //send the network reply
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
            if(isFound && isIntoTheClan)
                out << (uint8_t)0x01;
            else
            {
                if(!isFound)
                    normalOutput("Clan invite: Player "+text+" not found, is connected?");
                if(!isIntoTheClan)
                    normalOutput("Clan invite: Player "+text+" is not into your clan");
                out << (uint8_t)0x02;
            }
            postReply(query_id,outputData.constData(),outputData.size());
            if(!isFound)
            {
                std::string queryText=PreparedDBQueryCommon::db_query_update_character_clan_by_pseudo;
                stringreplace(queryText,"%1",SqlFunction::quoteSqlVariable(text));
                stringreplace(queryText,"%2",std::to_string(public_and_private_informations.clan));
                dbQueryWriteCommon(queryText);
                return;
            }
            else if(isIntoTheClan)
                playerByPseudo[text]->ejectToClan();
        }
        break;
        default:
            errorOutput("Action on the clan not found");
        return;
    }
}

void Client::addClan_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->addClan_object();
    GlobalServerData::serverPrivateVariables.db_common->clear();
}

void Client::addClan_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.size()==0)
    {
        std::cerr << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    #endif
    ClanActionParam *clanActionParam=static_cast<ClanActionParam *>(paramToPassToCallBack.front());
    paramToPassToCallBack.erase(paramToPassToCallBack.begin());
    addClan_return(clanActionParam->query_id,clanActionParam->action,clanActionParam->text);
    delete clanActionParam;
}

void Client::addClan_return(const uint8_t &query_id,const uint8_t &action,const std::string &text)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.front()!="ClanActionParam")
    {
        std::cerr << "is not ClanActionParam" << stringimplode(paramToPassToCallBackType,';') << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    paramToPassToCallBackType.erase(paramToPassToCallBackType.begin());
    #endif
    callbackRegistred.erase(callbackRegistred.begin());
    Q_UNUSED(action);
    if(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (uint8_t)0x02;
        postReply(query_id,outputData.constData(),outputData.size());
        return;
    }
    bool ok;
    const uint32_t clanId=getClanId(&ok);
    if(!ok)
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (uint8_t)0x02;
        postReply(query_id,outputData.constData(),outputData.size());
        return;
    }
    public_and_private_informations.clan=clanId;
    createMemoryClan();
    clan->name=text;
    public_and_private_informations.clan_leader=true;
    //send the network reply
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x01;
    out << (uint32_t)clanId;
    postReply(query_id,outputData.constData(),outputData.size());
    //add into db
    std::string queryText=PreparedDBQueryCommon::db_query_insert_clan;
    stringreplace(queryText,"%1",std::to_string(clanId));
    stringreplace(queryText,"%2",SqlFunction::quoteSqlVariable(text));
    stringreplace(queryText,"%3",std::to_string(QDateTime::currentMSecsSinceEpoch()/1000));
    dbQueryWriteCommon(queryText);
    insertIntoAClan(clanId);
}

uint32_t Client::getPlayerId() const
{
    if(character_loaded)
        return character_id;
    return 0;
}

void Client::haveClanInfo(const uint32_t &clanId,const std::string &clanName,const quint64 &cash)
{
    normalOutput("First client of the clan: "+clanName+", clanId: "+std::to_string(clanId)+" to connect");
    createMemoryClan();
    clanList[clanId]->name=clanName;
    clanList[clanId]->cash=cash;
}

void Client::sendClanInfo()
{
    if(public_and_private_informations.clan==0)
        return;
    if(clan==NULL)
        return;
    normalOutput("Send the clan info: "+clan->name+", clanId: "+std::to_string(public_and_private_informations.clan)+", get the info");
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    {
        const QByteArray outputText=FacilityLibGeneral::toUTF8WithHeader(clan->name);
        outputData+=outputText;
        out.device()->seek(out.device()->pos()+outputText.size());
    }
    sendFullPacket(0xC2,0x0A,outputData.constData(),outputData.size());
}

void Client::dissolvedClan()
{
    public_and_private_informations.clan=0;
    clan=NULL;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    sendFullPacket(0xC2,0x09);
    clanChangeWithoutDb(public_and_private_informations.clan);
}

bool Client::inviteToClan(const uint32_t &clanId)
{
    if(inviteToClanList.size()>0)
        return false;
    if(clan==NULL)
        return false;
    inviteToClanList.push_back(clanId);
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint32_t)clanId;
    {
        const QByteArray outputText=FacilityLibGeneral::toUTF8WithHeader(clan->name);
        outputData+=outputText;
        out.device()->seek(out.device()->pos()+outputText.size());
    }
    sendFullPacket(0xC2,0x0B,outputData.constData(),outputData.size());
    return false;
}

void Client::clanInvite(const bool &accept)
{
    if(!accept)
    {
        normalOutput("You have refused the clan invitation");
        inviteToClanList.erase(inviteToClanList.begin());
        return;
    }
    normalOutput("You have accepted the clan invitation");
    if(inviteToClanList.size()==0)
    {
        errorOutput("Can't responde to clan invite, because no in suspend");
        return;
    }
    public_and_private_informations.clan_leader=false;
    public_and_private_informations.clan=inviteToClanList.front();
    createMemoryClan();
    insertIntoAClan(inviteToClanList.front());
    inviteToClanList.erase(inviteToClanList.begin());
}

uint32_t Client::clanId() const
{
    return public_and_private_informations.clan;
}

void Client::insertIntoAClan(const uint32_t &clanId)
{
    //add into db
    std::string clan_leader;
    if(GlobalServerData::serverPrivateVariables.db_common->databaseType()!=DatabaseBase::DatabaseType::PostgreSQL)
    {
        if(public_and_private_informations.clan_leader)
            clan_leader=Client::text_1;
        else
            clan_leader=Client::text_0;
    }
    else
    {
        if(public_and_private_informations.clan_leader)
            clan_leader=Client::text_true;
        else
            clan_leader=Client::text_false;
    }
    std::string queryText=PreparedDBQueryCommon::db_query_update_character_clan_and_leader;
    stringreplace(queryText,"%1",std::to_string(clanId));
    stringreplace(queryText,"%2",clan_leader);
    stringreplace(queryText,"%3",std::to_string(character_id));
    dbQueryWriteCommon(queryText);
    sendClanInfo();
    clanChangeWithoutDb(public_and_private_informations.clan);
}

void Client::ejectToClan()
{
    dissolvedClan();
    std::string queryText=PreparedDBQueryCommon::db_query_update_character_clan;
    stringreplace(queryText,"%1",std::to_string(character_id));
    dbQueryWriteCommon(queryText);
}

uint32_t Client::getClanId() const
{
    return public_and_private_informations.clan;
}

bool Client::haveAClan() const
{
    return public_and_private_informations.clan>0;
}

void Client::waitingForCityCaputre(const bool &cancel)
{
    if(clan==NULL)
    {
        errorOutput("Try capture city when is not in clan");
        return;
    }
    if(!cancel)
    {
        if(captureCityInProgress())
        {
            errorOutput("Try capture city when is already into that's");
            return;
        }
        if(isInFight())
        {
            errorOutput("Try capture city when is in fight");
            return;
        }
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        normalOutput("ask zonecapture at "+this->map->map_file+" ("+std::to_string(this->x)+","+std::to_string(this->y)+")");
        #endif
        CommonMap *map=this->map;
        uint8_t x=this->x;
        uint8_t y=this->y;
        //resolv the object
        Direction direction=getLastDirection();
        switch(direction)
        {
            case Direction_look_at_top:
            case Direction_look_at_right:
            case Direction_look_at_bottom:
            case Direction_look_at_left:
                direction=lookToMove(direction);
                if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
                {
                    if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                    {
                        errorOutput("waitingForCityCaputre() Can't move at this direction from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                        return;
                    }
                }
                else
                {
                    errorOutput("No valid map in this direction");
                    return;
                }
            break;
            default:
            errorOutput("Wrong direction to use a zonecapture");
            return;
        }
        const MapServer * const mapServer=static_cast<MapServer*>(map);
        const std::pair<uint8_t,uint8_t> pos(x,y);
        //check if is zonecapture
        if(mapServer->zonecapture.find(pos)==mapServer->zonecapture.cend())
        {
            Direction direction=getLastDirection();
            switch(direction)
            {
                case Direction_look_at_top:
                case Direction_look_at_right:
                case Direction_look_at_bottom:
                case Direction_look_at_left:
                    direction=lookToMove(direction);
                    if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
                    {
                        if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                        {
                            errorOutput("waitingForCityCaputre() Can't move at this direction from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                            return;
                        }
                    }
                    else
                    {
                        errorOutput("No valid map in this direction");
                        return;
                    }
                break;
                default:
                errorOutput("Wrong direction to use a zonecapture");
                return;
            }
            const MapServer * const mapServer=static_cast<MapServer*>(map);
            const std::pair<uint8_t,uint8_t> pos(x,y);
            if(mapServer->zonecapture.find(pos)==mapServer->zonecapture.cend())
            {
                errorOutput("no zonecapture point in this direction");
                return;
            }
        }
        //send the zone capture
        const std::string &zoneName=static_cast<MapServer*>(map)->zonecapture.at(std::pair<uint8_t,uint8_t>(x,y));
        if(!public_and_private_informations.clan_leader)
        {
            if(clan->captureCityInProgress.size()==0)
            {
                QByteArray outputData;
                QDataStream out(&outputData, QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
                out << (uint8_t)0x01;
                sendFullPacket(0xF0,0x01,outputData.constData(),outputData.size());
                return;
            }
        }
        else
        {
            if(clan->captureCityInProgress.size()==0)
                clan->captureCityInProgress=zoneName;
        }
        if(clan->captureCityInProgress!=zoneName)
        {
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
            out << (uint8_t)0x02;
            {
                const QByteArray outputText=FacilityLibGeneral::toUTF8WithHeader(clan->captureCityInProgress);
                outputData+=outputText;
                out.device()->seek(out.device()->pos()+outputText.size());
            }
            sendFullPacket(0xF0,0x01,outputData.constData(),outputData.size());
            return;
        }
        if(captureCity.count(zoneName)>0)
        {
            errorOutput("already in capture city");
            return;
        }
        captureCity[zoneName].push_back(this);
        setInCityCapture(true);
    }
    else
    {
        if(clan->captureCityInProgress.size()==0)
        {
            errorOutput("your clan is not in capture city");
            return;
        }
        if(!vectorremoveOne(captureCity[clan->captureCityInProgress],this))
        {
            errorOutput("not in capture city");
            return;
        }
        leaveTheCityCapture();
    }
}

void Client::leaveTheCityCapture()
{
    if(clan==NULL)
        return;
    if(clan->captureCityInProgress.size()==0)
        return;
    if(vectorremoveOne(captureCity[clan->captureCityInProgress],this))
    {
        //drop all the capture because no body clam it
        if(captureCity.at(clan->captureCityInProgress).size()==0)
        {
            captureCity.erase(clan->captureCityInProgress);
            clan->captureCityInProgress.clear();
        }
        else
        {
            //drop the clan capture in no other player of the same clan is into it
            unsigned int index=0;
            const unsigned int &list_size=captureCity.at(clan->captureCityInProgress).size();
            while(index<list_size)
            {
                if(captureCity.at(clan->captureCityInProgress).at(index)->clanId()==clanId())
                    break;
                index++;
            }
            if(index==captureCity.at(clan->captureCityInProgress).size())
                clan->captureCityInProgress.clear();
        }
    }
    setInCityCapture(false);
    otherCityPlayerBattle=NULL;
}

void Client::startTheCityCapture()
{
    auto i=captureCity.begin();
    while(i!=captureCity.cend())
    {
        //the city is not free to capture
        if(captureCityValidatedList.find(i->first)!=captureCityValidatedList.cend())
        {
            unsigned int index=0;
            while(index<i->second.size())
            {
                i->second.at(index)->previousCityCaptureNotFinished();
                index++;
            }
        }
        //the city is ready to be captured
        else
        {
            CaptureCityValidated tempCaptureCityValidated;
            if(GlobalServerData::serverPrivateVariables.cityStatusList.find(i->first)==GlobalServerData::serverPrivateVariables.cityStatusList.cend())
                GlobalServerData::serverPrivateVariables.cityStatusList[i->first].clan=0;
            if(GlobalServerData::serverPrivateVariables.cityStatusList.at(i->first).clan==0)
                if(GlobalServerData::serverPrivateVariables.captureFightIdListByZoneToCaptureCity.find(i->first)!=GlobalServerData::serverPrivateVariables.captureFightIdListByZoneToCaptureCity.cend())
                    tempCaptureCityValidated.bots=GlobalServerData::serverPrivateVariables.captureFightIdListByZoneToCaptureCity.at(i->first);
            tempCaptureCityValidated.players=i->second;
            unsigned int index;
            unsigned int sub_index;
            //do the clan count
            int player_count=tempCaptureCityValidated.players.size()+tempCaptureCityValidated.bots.size();
            int clan_count=0;
            if(tempCaptureCityValidated.bots.size()>0)
                clan_count++;
            if(tempCaptureCityValidated.players.size()>0)
            {
                index=0;
                while(index<tempCaptureCityValidated.players.size())
                {
                    const uint32_t &clanId=tempCaptureCityValidated.players.at(index)->clanId();
                    if(tempCaptureCityValidated.clanSize.find(clanId)!=tempCaptureCityValidated.clanSize.cend())
                        tempCaptureCityValidated.clanSize[clanId]++;
                    else
                        tempCaptureCityValidated.clanSize[clanId]=1;
                    index++;
                }
                clan_count+=tempCaptureCityValidated.clanSize.size();
            }
            //do the PvP
            index=0;
            while(index<tempCaptureCityValidated.players.size())
            {
                sub_index=index+1;
                while(sub_index<tempCaptureCityValidated.players.size())
                {
                    if(tempCaptureCityValidated.players.at(index)->clanId()!=tempCaptureCityValidated.players.at(sub_index)->clanId())
                    {
                        tempCaptureCityValidated.players.at(index)->otherCityPlayerBattle=tempCaptureCityValidated.players.at(sub_index);
                        tempCaptureCityValidated.players.at(sub_index)->otherCityPlayerBattle=tempCaptureCityValidated.players.at(index);
                        tempCaptureCityValidated.players.at(index)->battleFakeAccepted(tempCaptureCityValidated.players.at(sub_index));
                        tempCaptureCityValidated.playersInFight.push_back(tempCaptureCityValidated.players.at(index));
                        tempCaptureCityValidated.playersInFight.back()->cityCaptureBattle(player_count,clan_count);
                        tempCaptureCityValidated.playersInFight.push_back(tempCaptureCityValidated.players.at(sub_index));
                        tempCaptureCityValidated.playersInFight.back()->cityCaptureBattle(player_count,clan_count);
                        tempCaptureCityValidated.players.erase(tempCaptureCityValidated.players.begin()+index);
                        index--;
                        tempCaptureCityValidated.players.erase(tempCaptureCityValidated.players.begin()+sub_index-1);
                        break;
                    }
                    sub_index++;
                }
                index++;
            }
            //bot the bot fight
            while(tempCaptureCityValidated.players.size()>0 && tempCaptureCityValidated.bots.size()>0)
            {
                tempCaptureCityValidated.playersInFight.push_back(tempCaptureCityValidated.players.front());
                tempCaptureCityValidated.playersInFight.back()->cityCaptureBotFight(player_count,clan_count,tempCaptureCityValidated.bots.front());
                tempCaptureCityValidated.botsInFight.push_back(tempCaptureCityValidated.bots.front());
                tempCaptureCityValidated.players.front()->botFightStart(tempCaptureCityValidated.bots.front());
                tempCaptureCityValidated.players.erase(tempCaptureCityValidated.players.begin());
                tempCaptureCityValidated.bots.erase(tempCaptureCityValidated.bots.begin());
            }
            //send the wait to the rest
            cityCaptureSendInWait(tempCaptureCityValidated,player_count,clan_count);

            captureCityValidatedList[i->first]=tempCaptureCityValidated;
        }
        ++i;
    }
    captureCity.clear();
}

//fightId == 0 if is in battle
void Client::fightOrBattleFinish(const bool &win, const uint32_t &fightId)
{
    if(clan!=NULL)
    {
        if(clan->captureCityInProgress.size()>0 && captureCityValidatedList.find(clan->captureCityInProgress)!=captureCityValidatedList.cend())
        {
            CaptureCityValidated &captureCityValidated=captureCityValidatedList[clan->captureCityInProgress];
            //check if this player is into the capture city with the other player of the team
            if(vectorcontains(captureCityValidated.playersInFight,this))
            {
                if(win)
                {
                    if(fightId!=0)
                        vectorremoveOne(captureCityValidated.botsInFight,fightId);
                    else
                    {
                        if(otherCityPlayerBattle!=NULL)
                        {
                            vectorremoveOne(captureCityValidated.playersInFight,otherCityPlayerBattle);
                            otherCityPlayerBattle=NULL;
                        }
                    }
                    uint16_t player_count=cityCapturePlayerCount(captureCityValidated);
                    uint16_t clan_count=cityCaptureClanCount(captureCityValidated);
                    bool newFightFound=false;
                    unsigned int index=0;
                    while(index<captureCityValidated.players.size())
                    {
                        if(clanId()!=captureCityValidated.players.at(index)->clanId())
                        {
                            battleFakeAccepted(captureCityValidated.players.at(index));
                            captureCityValidated.playersInFight.push_back(captureCityValidated.players.at(index));
                            captureCityValidated.playersInFight.back()->cityCaptureBattle(player_count,clan_count);
                            cityCaptureBattle(player_count,clan_count);
                            captureCityValidated.players.erase(captureCityValidated.players.begin()+index);
                            newFightFound=true;
                            break;
                        }
                        index++;
                    }
                    if(!newFightFound && captureCityValidated.bots.size()>0)
                    {
                        cityCaptureBotFight(player_count,clan_count,captureCityValidated.bots.front());
                        captureCityValidated.botsInFight.push_back(captureCityValidated.bots.front());
                        botFightStart(captureCityValidated.bots.front());
                        captureCityValidated.bots.erase(captureCityValidated.bots.begin());
                        newFightFound=true;
                    }
                    if(!newFightFound)
                    {
                        vectorremoveOne(captureCityValidated.playersInFight,this);
                        captureCityValidated.players.push_back(this);
                        otherCityPlayerBattle=NULL;
                    }
                }
                else
                {
                    if(fightId!=0)
                    {
                        vectorremoveOne(captureCityValidated.botsInFight,fightId);
                        vectorremoveOne(captureCityValidated.bots,fightId);
                    }
                    else
                    {
                        vectorremoveOne(captureCityValidated.playersInFight,this);
                        otherCityPlayerBattle=NULL;
                    }
                    captureCityValidated.clanSize[clanId()]--;
                    if(captureCityValidated.clanSize.at(clanId())==0)
                        captureCityValidated.clanSize.erase(clanId());
                }
                uint16_t player_count=cityCapturePlayerCount(captureCityValidated);
                uint16_t clan_count=cityCaptureClanCount(captureCityValidated);
                //city capture
                if(captureCityValidated.bots.size()==0 && captureCityValidated.botsInFight.size()==0 && captureCityValidated.playersInFight.size()==0)
                {
                    if(clan->capturedCity==clan->captureCityInProgress)
                        clan->captureCityInProgress.clear();
                    else
                    {
                        if(GlobalServerData::serverPrivateVariables.cityStatusList.find(clan->capturedCity)!=GlobalServerData::serverPrivateVariables.cityStatusList.cend())
                        {
                            GlobalServerData::serverPrivateVariables.cityStatusListReverse.erase(clan->clanId);
                            GlobalServerData::serverPrivateVariables.cityStatusList[clan->capturedCity].clan=0;
                        }
                        std::string queryText=PreparedDBQueryServer::db_query_delete_city;
                        stringreplace(queryText,"%1",clan->capturedCity);
                        dbQueryWriteServer(queryText);
                        if(GlobalServerData::serverPrivateVariables.cityStatusList.find(clan->captureCityInProgress)==GlobalServerData::serverPrivateVariables.cityStatusList.cend())
                            GlobalServerData::serverPrivateVariables.cityStatusList[clan->captureCityInProgress].clan=0;

                        if(GlobalServerData::serverPrivateVariables.cityStatusList.at(clan->captureCityInProgress).clan!=0)
                            queryText=PreparedDBQueryServer::db_query_update_city_clan;
                        else
                            queryText=PreparedDBQueryServer::db_query_insert_city;
                        stringreplace(queryText,"%1",std::to_string(clan->clanId));
                        stringreplace(queryText,"%2",clan->captureCityInProgress);
                        dbQueryWriteServer(queryText);
                        GlobalServerData::serverPrivateVariables.cityStatusListReverse[clan->clanId]=clan->captureCityInProgress;
                        GlobalServerData::serverPrivateVariables.cityStatusList[clan->captureCityInProgress].clan=clan->clanId;
                        clan->capturedCity=clan->captureCityInProgress;
                        clan->captureCityInProgress.clear();
                        unsigned int index=0;
                        while(index<captureCityValidated.players.size())
                        {
                            captureCityValidated.players.back()->cityCaptureWin();
                            index++;
                        }
                    }
                }
                else
                    cityCaptureSendInWait(captureCityValidated,player_count,clan_count);
                return;
            }
        }
    }
}

void Client::cityCaptureSendInWait(const CaptureCityValidated &captureCityValidated, const uint16_t &number_of_player, const uint16_t &number_of_clan)
{
    unsigned int index=0;
    while(index<captureCityValidated.players.size())
    {
        captureCityValidated.playersInFight.back()->cityCaptureInWait(number_of_player,number_of_clan);
        index++;
    }
}

uint16_t Client::cityCapturePlayerCount(const CaptureCityValidated &captureCityValidated)
{
    return captureCityValidated.bots.size()+captureCityValidated.botsInFight.size()+captureCityValidated.players.size()+captureCityValidated.playersInFight.size();
}

uint16_t Client::cityCaptureClanCount(const CaptureCityValidated &captureCityValidated)
{
    if(captureCityValidated.bots.size()==0 && captureCityValidated.botsInFight.size()==0)
        return captureCityValidated.clanSize.size();
    else
        return captureCityValidated.clanSize.size()+1;
}

void Client::cityCaptureBattle(const uint16_t &number_of_player,const uint16_t &number_of_clan)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x04;
    out << (uint16_t)number_of_player;
    out << (uint16_t)number_of_clan;
    sendFullPacket(0xF0,0x01,outputData.constData(),outputData.size());
}

void Client::cityCaptureBotFight(const uint16_t &number_of_player,const uint16_t &number_of_clan,const uint32_t &fightId)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x04;
    out << (uint16_t)number_of_player;
    out << (uint16_t)number_of_clan;
    out << (uint32_t)fightId;
    sendFullPacket(0xF0,0x01,outputData.constData(),outputData.size());
}

void Client::cityCaptureInWait(const uint16_t &number_of_player,const uint16_t &number_of_clan)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x05;
    out << (uint16_t)number_of_player;
    out << (uint16_t)number_of_clan;
    sendFullPacket(0xF0,0x01,outputData.constData(),outputData.size());
}

void Client::cityCaptureWin()
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x06;
    sendFullPacket(0xF0,0x01,outputData.constData(),outputData.size());
}

void Client::previousCityCaptureNotFinished()
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x02;
    sendFullPacket(0xF0,0x03,outputData.constData(),outputData.size());
}

void Client::moveMonster(const bool &up,const uint8_t &number)
{
    if(up)
        moveUpMonster(number-1);
    else
        moveDownMonster(number-1);
}

void Client::getMarketList(const uint32_t &query_id)
{
    if(getInTrade() || isInFight())
    {
        errorOutput("You can't use the market in trade/fight");
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint64)market_cash;
    unsigned int index;
    std::vector<MarketItem> marketItemList,marketOwnItemList;
    std::vector<MarketPlayerMonster> marketPlayerMonsterList,marketOwnPlayerMonsterList;
    //object filter
    index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketItemList.size())
    {
        const MarketItem &marketObject=GlobalServerData::serverPrivateVariables.marketItemList.at(index);
        if(marketObject.player==character_id)
            marketOwnItemList.push_back(marketObject);
        else
            marketItemList.push_back(marketObject);
        index++;
    }
    //monster filter
    index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.size())
    {
        const MarketPlayerMonster &marketPlayerMonster=GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.at(index);
        if(marketPlayerMonster.player==character_id)
            marketOwnPlayerMonsterList.push_back(marketPlayerMonster);
        else
            marketPlayerMonsterList.push_back(marketPlayerMonster);
        index++;
    }
    //object
    out << (uint32_t)marketItemList.size();
    index=0;
    while(index<marketItemList.size())
    {
        const MarketItem &marketObject=marketItemList.at(index);
        out << marketObject.marketObjectId;
        out << marketObject.item;
        out << marketObject.quantity;
        out << marketObject.cash;
        index++;
    }
    //monster
    out << (uint32_t)marketPlayerMonsterList.size();
    index=0;
    while(index<marketPlayerMonsterList.size())
    {
        const MarketPlayerMonster &marketPlayerMonster=marketPlayerMonsterList.at(index);
        out << marketPlayerMonster.monster.id;
        out << marketPlayerMonster.monster.monster;
        out << marketPlayerMonster.monster.level;
        out << marketPlayerMonster.cash;
        index++;
    }
    //own object
    out << (uint32_t)marketOwnItemList.size();
    index=0;
    while(index<marketOwnItemList.size())
    {
        const MarketItem &marketObject=marketOwnItemList.at(index);
        out << marketObject.marketObjectId;
        out << marketObject.item;
        out << marketObject.quantity;
        out << marketObject.cash;
        index++;
    }
    //own monster
    out << (uint32_t)marketPlayerMonsterList.size();
    index=0;
    while(index<marketPlayerMonsterList.size())
    {
        const MarketPlayerMonster &marketPlayerMonster=marketPlayerMonsterList.at(index);
        out << marketPlayerMonster.monster.id;
        out << marketPlayerMonster.monster.monster;
        out << marketPlayerMonster.monster.level;
        out << marketPlayerMonster.cash;
        index++;
    }

    postReply(query_id,outputData.constData(),outputData.size());
}

void Client::buyMarketObject(const uint32_t &query_id,const uint32_t &marketObjectId,const uint32_t &quantity)
{
    if(getInTrade() || isInFight())
    {
        errorOutput("You can't use the market in trade/fight");
        return;
    }
    if(quantity<=0)
    {
        errorOutput("You can't use the market with null quantity");
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    //search into the market
    unsigned int index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketItemList.size())
    {
        const MarketItem marketItem=GlobalServerData::serverPrivateVariables.marketItemList.at(index);
        if(marketItem.marketObjectId==marketObjectId)
        {
            if(marketItem.quantity<quantity)
            {
                out << (uint8_t)0x02;
                postReply(query_id,outputData.constData(),outputData.size());
                return;
            }
            //check if have the price
            if((quantity*marketItem.cash)>public_and_private_informations.cash)
            {
                out << (uint8_t)0x03;
                postReply(query_id,outputData.constData(),outputData.size());
                return;
            }
            //apply the buy
            if(marketItem.quantity==quantity)
            {
                std::string queryText=PreparedDBQueryServer::db_query_delete_item_market;
                stringreplace(queryText,"%1",std::to_string(marketItem.item));
                stringreplace(queryText,"%2",std::to_string(marketItem.player));
                dbQueryWriteServer(queryText);
                GlobalServerData::serverPrivateVariables.marketItemList.erase(GlobalServerData::serverPrivateVariables.marketItemList.begin()+index);
            }
            else
            {
                GlobalServerData::serverPrivateVariables.marketItemList[index].quantity=marketItem.quantity-quantity;
                std::string queryText=PreparedDBQueryServer::db_query_update_item_market;
                stringreplace(queryText,"%1",std::to_string(marketItem.quantity-quantity));
                stringreplace(queryText,"%2",std::to_string(marketItem.item));
                stringreplace(queryText,"%3",std::to_string(marketItem.player));
                dbQueryWriteServer(queryText);
            }
            removeCash(quantity*marketItem.cash);
            if(playerById.find(marketItem.player)!=playerById.cend())
            {
                if(!playerById.at(marketItem.player)->addMarketCashWithoutSave(quantity*marketItem.cash))
                    normalOutput("Problem at market cash adding");
            }
            std::string queryText=PreparedDBQueryServer::db_query_update_charaters_market_cash;
            stringreplace(queryText,"%1",std::to_string(quantity*marketItem.cash));
            stringreplace(queryText,"%2",std::to_string(marketItem.player));
            dbQueryWriteServer(queryText);
            addObject(marketItem.item,quantity);
            out << (uint8_t)0x01;
            postReply(query_id,outputData.constData(),outputData.size());
            return;
        }
        index++;
    }
    out << (uint8_t)0x03;
    postReply(query_id,outputData.constData(),outputData.size());
}

void Client::buyMarketMonster(const uint32_t &query_id,const uint32_t &monsterId)
{
    if(getInTrade() || isInFight())
    {
        errorOutput("You can't use the market in trade/fight");
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    if(public_and_private_informations.playerMonster.size()>=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters)
    {
        out << (uint8_t)0x02;
        postReply(query_id,outputData.constData(),outputData.size());
        return;
    }
    //search into the market
    unsigned int index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.size())
    {
        const MarketPlayerMonster marketPlayerMonster=GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.at(index);
        if(marketPlayerMonster.monster.id==monsterId)
        {
            //check if have the price
            if(marketPlayerMonster.cash>public_and_private_informations.cash)
            {
                out << (uint8_t)0x03;
                postReply(query_id,outputData.constData(),outputData.size());
                return;
            }
            //apply the buy
            GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.erase(GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.begin()+index);
            removeCash(marketPlayerMonster.cash);
            //entry created at first server connexion
            std::string queryText=PreparedDBQueryServer::db_query_update_charaters_market_cash;
            stringreplace(queryText,"%1",std::to_string(marketPlayerMonster.cash));
            stringreplace(queryText,"%2",std::to_string(marketPlayerMonster.player));
            dbQueryWriteServer(queryText);
            addPlayerMonster(marketPlayerMonster.monster);

            queryText=PreparedDBQueryServer::db_query_delete_monster_market_price;
            stringreplace(queryText,"%1",std::to_string(marketPlayerMonster.monster.id));
            dbQueryWriteServer(queryText);
            queryText=PreparedDBQueryCommon::db_query_update_monster_move_to_new_player;
            stringreplace(queryText,"%1",std::to_string(character_id));
            stringreplace(queryText,"%2",std::to_string(getPlayerMonster().size()));
            stringreplace(queryText,"%3",std::to_string(marketPlayerMonster.monster.id));
            dbQueryWriteCommon(queryText);
            out << (uint8_t)0x01;
            postReply(query_id,outputData.constData(),outputData.size());
            return;
        }
        index++;
    }
    out << (uint8_t)0x03;
    postReply(query_id,outputData.constData(),outputData.size());
}

void Client::putMarketObject(const uint32_t &query_id,const uint32_t &objectId,const uint32_t &quantity,const uint32_t &price)
{
    if(getInTrade() || isInFight())
    {
        errorOutput("You can't use the market in trade/fight");
        return;
    }
    if(quantity<=0)
    {
        errorOutput("You can't use the market with null quantity");
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    if(objectQuantity(objectId)<quantity)
    {
        out << (uint8_t)0x02;
        postReply(query_id,outputData.constData(),outputData.size());
        return;
    }
    //search into the market
    unsigned int index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketItemList.size())
    {
        const MarketItem &marketItem=GlobalServerData::serverPrivateVariables.marketItemList.at(index);
        if(marketItem.player==character_id && marketItem.item==objectId)
        {
            removeObject(objectId,quantity);
            GlobalServerData::serverPrivateVariables.marketItemList[index].cash=price;
            GlobalServerData::serverPrivateVariables.marketItemList[index].quantity+=quantity;
            out << (uint8_t)0x01;
            postReply(query_id,outputData.constData(),outputData.size());
            std::string queryText=PreparedDBQueryServer::db_query_update_item_market_and_price;
            stringreplace(queryText,"%1",std::to_string(GlobalServerData::serverPrivateVariables.marketItemList.at(index).quantity));
            stringreplace(queryText,"%2",std::to_string(price));
            stringreplace(queryText,"%3",std::to_string(objectId));
            stringreplace(queryText,"%4",std::to_string(character_id));
            dbQueryWriteServer(queryText);
            return;
        }
        index++;
    }
    if(marketObjectIdList.size()==0)
    {
        out << (uint8_t)0x02;
        postReply(query_id,outputData.constData(),outputData.size());
        normalOutput("No more id into marketObjectIdList");
        return;
    }
    //append to the market
    removeObject(objectId,quantity);
    std::string queryText=PreparedDBQueryServer::db_query_insert_item_market;
    stringreplace(queryText,"%1",std::to_string(objectId));
    stringreplace(queryText,"%2",std::to_string(character_id));
    stringreplace(queryText,"%3",std::to_string(quantity));
    stringreplace(queryText,"%4",std::to_string(price));
    dbQueryWriteServer(queryText);
    MarketItem marketItem;
    marketItem.cash=price;
    marketItem.item=objectId;
    marketItem.marketObjectId=marketObjectIdList.front();
    marketItem.player=character_id;
    marketItem.quantity=quantity;
    marketObjectIdList.erase(marketObjectIdList.begin());
    GlobalServerData::serverPrivateVariables.marketItemList.push_back(marketItem);
    out << (uint8_t)0x01;
    postReply(query_id,outputData.constData(),outputData.size());
}

void Client::putMarketMonster(const uint32_t &query_id,const uint32_t &monsterId,const uint32_t &price)
{
    if(getInTrade() || isInFight())
    {
        errorOutput("You can't use the market in trade/fight");
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    unsigned int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        const PlayerMonster &playerMonster=public_and_private_informations.playerMonster.at(index);
        if(playerMonster.id==monsterId)
        {
            if(!remainMonstersToFight(monsterId))
            {
                normalOutput("You can't put in market this msonter because you will be without monster to fight");
                out << (uint8_t)0x02;
                postReply(query_id,outputData.constData(),outputData.size());
                return;
            }
            MarketPlayerMonster marketPlayerMonster;
            marketPlayerMonster.cash=price;
            marketPlayerMonster.monster=playerMonster;
            marketPlayerMonster.player=character_id;
            public_and_private_informations.playerMonster.erase(public_and_private_informations.playerMonster.begin()+index);
            GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.push_back(marketPlayerMonster);

            std::string queryText=PreparedDBQueryCommon::db_query_update_monster_move_to_market;
            stringreplace(queryText,"%1",std::to_string(marketPlayerMonster.monster.id));
            dbQueryWriteCommon(queryText);
            queryText=PreparedDBQueryServer::db_query_insert_monster_market_price;
            stringreplace(queryText,"%1",std::to_string(marketPlayerMonster.monster.id));
            stringreplace(queryText,"%2",std::to_string(price));
            dbQueryWriteServer(queryText);
            while(index<public_and_private_informations.playerMonster.size())
            {
                const PlayerMonster &playerMonster=public_and_private_informations.playerMonster.at(index);
                std::string queryText=PreparedDBQueryCommon::db_query_update_monster_position;
                stringreplace(queryText,"%1",std::to_string(index+1));
                stringreplace(queryText,"%2",std::to_string(playerMonster.id));
                dbQueryWriteCommon(queryText);
                index++;
            }
            out << (uint8_t)0x01;
            postReply(query_id,outputData.constData(),outputData.size());
            return;
        }
        index++;
    }
    out << (uint8_t)0x02;
    postReply(query_id,outputData.constData(),outputData.size());
}

void Client::recoverMarketCash(const uint32_t &query_id)
{
    if(getInTrade() || isInFight())
    {
        errorOutput("You can't use the market in trade/fight");
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint64)market_cash;
    public_and_private_informations.cash+=market_cash;
    market_cash=0;
    std::string queryText=PreparedDBQueryServer::db_query_get_market_cash;
    stringreplace(queryText,"%1",std::to_string(public_and_private_informations.cash));
    stringreplace(queryText,"%1",std::to_string(character_id));
    dbQueryWriteServer(queryText);
    postReply(query_id,outputData.constData(),outputData.size());
}

void Client::withdrawMarketObject(const uint32_t &query_id,const uint32_t &objectId,const uint32_t &quantity)
{
    if(getInTrade() || isInFight())
    {
        errorOutput("You can't use the market in trade/fight");
        return;
    }
    if(quantity<=0)
    {
        errorOutput("You can't use the market with null quantity");
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    unsigned int index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketItemList.size())
    {
        const MarketItem &marketItem=GlobalServerData::serverPrivateVariables.marketItemList.at(index);
        if(marketItem.item==objectId)
        {
            if(marketItem.player!=character_id)
            {
                out << (uint8_t)0x02;
                postReply(query_id,outputData.constData(),outputData.size());
                return;
            }
            if(marketItem.quantity<quantity)
            {
                out << (uint8_t)0x02;
                postReply(query_id,outputData.constData(),outputData.size());
                return;
            }
            out << (uint8_t)0x01;
            out << (uint8_t)0x01;
            out << marketItem.item;
            out << marketItem.quantity;
            GlobalServerData::serverPrivateVariables.marketItemList[index].quantity=marketItem.quantity-quantity;
            if(GlobalServerData::serverPrivateVariables.marketItemList.at(index).quantity==0)
            {
                marketObjectIdList.push_back(marketItem.marketObjectId);
                GlobalServerData::serverPrivateVariables.marketItemList.erase(GlobalServerData::serverPrivateVariables.marketItemList.begin()+index);
                std::string queryText=PreparedDBQueryServer::db_query_delete_item_market;
                stringreplace(queryText,"%1",std::to_string(objectId));
                stringreplace(queryText,"%2",std::to_string(character_id));
                dbQueryWriteServer(queryText);
            }
            else
            {
                std::string queryText=PreparedDBQueryServer::db_query_update_item_market;
                stringreplace(queryText,"%1",std::to_string(GlobalServerData::serverPrivateVariables.marketItemList.at(index).quantity));
                stringreplace(queryText,"%2",std::to_string(objectId));
                stringreplace(queryText,"%3",std::to_string(character_id));
                dbQueryWriteServer(queryText);
            }
            addObject(objectId,quantity);
            postReply(query_id,outputData.constData(),outputData.size());
            return;
        }
        index++;
    }
    out << (uint8_t)0x02;
    postReply(query_id,outputData.constData(),outputData.size());
}

void Client::withdrawMarketMonster(const uint32_t &query_id,const uint32_t &monsterId)
{
    if(getInTrade() || isInFight())
    {
        errorOutput("You can't use the market in trade/fight");
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    unsigned int index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.size())
    {
        const MarketPlayerMonster &marketPlayerMonster=GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.at(index);
        if(marketPlayerMonster.monster.id==monsterId)
        {
            if(marketPlayerMonster.player!=character_id)
            {
                out << (uint8_t)0x02;
                postReply(query_id,outputData.constData(),outputData.size());
                return;
            }
            if(public_and_private_informations.playerMonster.size()>=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters)
            {
                out << (uint8_t)0x02;
                postReply(query_id,outputData.constData(),outputData.size());
                return;
            }
            GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.erase(GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.begin()+index);
            public_and_private_informations.playerMonster.push_back(marketPlayerMonster.monster);
            std::string queryText=PreparedDBQueryCommon::db_query_update_monster_move_to_player;
            stringreplace(queryText,"%1",std::to_string(marketPlayerMonster.monster.id));
            dbQueryWriteCommon(queryText);
            queryText=PreparedDBQueryServer::db_query_delete_monster_market_price;
            stringreplace(queryText,"%1",std::to_string(marketPlayerMonster.monster.id));
            dbQueryWriteServer(queryText);
            out << (uint8_t)0x01;
            out << (uint8_t)0x02;
            const QByteArray newData(outputData+FacilityLib::privateMonsterToBinary(public_and_private_informations.playerMonster.back()));
            postReply(query_id,newData.constData(),newData.size());
            return;
        }
        index++;
    }
    out << (uint8_t)0x02;
    postReply(query_id,outputData.constData(),outputData.size());
}

bool Client::haveReputationRequirements(const std::vector<ReputationRequirements> &reputationList) const
{
    unsigned int index=0;
    while(index<reputationList.size())
    {
        const CatchChallenger::ReputationRequirements &reputationRequierement=reputationList.at(index);
        if(public_and_private_informations.reputation.find(reputationRequierement.reputationId)!=public_and_private_informations.reputation.cend())
        {
            const PlayerReputation &playerReputation=public_and_private_informations.reputation.at(reputationRequierement.reputationId);
            if(!reputationRequierement.positif)
            {
                if(-reputationRequierement.level<playerReputation.level)
                {
                    normalOutput("reputation.level("+std::to_string(reputationRequierement.level)+")<playerReputation.level("+std::to_string(playerReputation.level)+")");
                    return false;
                }
            }
            else
            {
                if(reputationRequierement.level>playerReputation.level || playerReputation.point<0)
                {
                    normalOutput("reputation.level("+std::to_string(reputationRequierement.level)+
                                 ")>playerReputation.level("+std::to_string(playerReputation.level)+
                                 ") || playerReputation.point("+std::to_string(playerReputation.point)+")<0");
                    return false;
                }
            }
        }
        else
            if(!reputationRequierement.positif)//default level is 0, but required level is negative
            {
                normalOutput("reputation.level("+std::to_string(reputationRequierement.level)+
                             ")<0 and no reputation.type="+CommonDatapack::commonDatapack.reputation.at(reputationRequierement.reputationId).name);
                return false;
            }
        index++;
    }
    return true;
}
