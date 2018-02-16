#include "../base/Client.h"
#include "../base/BroadCastWithoutSender.h"
#include "../base/MapServer.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "../base/GlobalServerData.h"
#include "../base/PreparedDBQuery.h"

#include <chrono>

using namespace CatchChallenger;

void Client::plantSeed(
        #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        const uint8_t &query_id,
        #endif
        const uint8_t &plant_id)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("plantSeed("+
                 #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
                 std::to_string(query_id)+","+
                 #endif
                 std::to_string(plant_id)+")");
    #endif
    if(CommonDatapack::commonDatapack.plants.find(plant_id)==CommonDatapack::commonDatapack.plants.cend())
    {
        errorOutput("plant_id not found: "+std::to_string(plant_id));
        return;
    }
    CommonMap *map=this->map;
    uint8_t x=this->x;
    uint8_t y=this->y;
    //resolv the dirt
    switch(getLastDirection())
    {
        case Direction_look_at_top:
            if(MoveOnTheMap::canGoTo(Direction_move_at_top,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_top,&map,&x,&y,false))
                {
                    errorOutput("plantSeed() Can't move at top from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                    return;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_right:
            if(MoveOnTheMap::canGoTo(Direction_move_at_right,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_right,&map,&x,&y,false))
                {
                    errorOutput("plantSeed() Can't move at right from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                    return;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_bottom:
            if(MoveOnTheMap::canGoTo(Direction_move_at_bottom,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_bottom,&map,&x,&y,false))
                {
                    errorOutput("plantSeed() Can't move at bottom from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                    return;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_left:
            if(MoveOnTheMap::canGoTo(Direction_move_at_left,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_left,&map,&x,&y,false))
                {
                    errorOutput("plantSeed() Can't move at left from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
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
        errorOutput("Wrong direction to plant a seed: "+std::to_string(getLastDirection()));
        return;
    }
    //check if is dirt
    if(static_cast<MapServer *>(map)->plants.find(std::pair<uint8_t,uint8_t>(x,y))==static_cast<MapServer *>(map)->plants.cend())
    {
        errorOutput("Try put seed out of the dirt");
        return;
    }
    //check if is free
    {
        const MapServer::PlantOnMap &plantOnMap=static_cast<MapServer *>(map)->plants.at(std::pair<uint8_t,uint8_t>(x,y));
        #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        if(public_and_private_informations.plantOnMap.find(plantOnMap.pointOnMapDbCode)!=public_and_private_informations.plantOnMap.cend())
        {
            errorOutput("Have already a plant in plantOnlyVisibleByPlayer==true");
            return;
        }
        #else
        if(plantOnMap.character!=0)
        {
            std::vector<char> data;
            data[0]=0x02;
            postReply(query_id,data.constData(),data.size());
            return;
        }
        #endif
    }
    //check if have into the inventory
    PlantInWaiting plantInWaiting;
    #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    plantInWaiting.query_id=query_id;
    #endif
    plantInWaiting.plant_id=plant_id;
    plantInWaiting.map=map;
    plantInWaiting.x=x;
    plantInWaiting.y=y;

    plant_list_in_waiting.push(plantInWaiting);
    useSeed(plant_id);
}

#ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
//plant
bool Client::syncDatabasePlant()
{
    if(public_and_private_informations.plantOnMap.size()*(1+1+8)>=sizeof(ProtocolParsingBase::tempBigBufferForOutput))
    {
        errorOutput("Too many plant");
        return false;
    }
    else
    {
        uint16_t lastPlantId=0;
        uint32_t posOutput=0;
        auto i=public_and_private_informations.plantOnMap.begin();
        while(i!=public_and_private_informations.plantOnMap.cend())
        {
            #ifdef MAXIMIZEPERFORMANCEOVERDATABASESIZE
            //not ordened
            uint16_t dirtOnMap;
            if(lastPlantId<=i->first)
            {
                dirtOnMap=i->first-lastPlantId;
                lastPlantId=i->first;
            }
            else
            {
                dirtOnMap=static_cast<uint16_t>(65536-static_cast<uint32_t>(lastPlantId)+static_cast<uint32_t>(i->first));
                lastPlantId=i->first;
            }
            #else
            //ordened
            const uint16_t &dirtOnMap=i->first-lastPlantId;
            lastPlantId=i->first;
            #endif
            const PlayerPlant &plant=i->second;
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(dirtOnMap);
            posOutput+=2;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=plant.plant;
            posOutput+=1;
            const uint64_t mature_at=htole64(plant.mature_at);
            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&mature_at,sizeof(mature_at));
            // *reinterpret_cast<uint64_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole64(plant.mature_at);
            posOutput+=8;

            ++i;
        }
        GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_update_plant.asyncWrite({
                    binarytoHexa(ProtocolParsingBase::tempBigBufferForOutput,posOutput),
                    std::to_string(character_id)
                    });
        return true;
    }
}
#endif

void Client::seedValidated()
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("seedValidated()");
    #endif
    /* useless, clean the protocol
    if(!ok)
    {
        std::vector<char> data;
        data[0]=0x02;
        postReply(plant_list_in_waiting.first().query_id,data);
        plant_list_in_waiting.removeFirst();
        return;
    }*/
    /* check if is free already done into Client::plantSeed()
    {
        const uint16_t &size=static_cast<MapServer *>(plant_list_in_waiting.first().map)->plants.size();
        uint16_t index=0;
        while(index<size)
        {
            if(x==static_cast<MapServer *>(plant_list_in_waiting.first().map)->plants.at(index).x && y==static_cast<MapServer *>(plant_list_in_waiting.first().map)->plants.at(index).y)
            {
                addObject(CommonDatapack::commonDatapack.plants.value(plant_list_in_waiting.first().plant_id).itemUsed);
                std::vector<char> data;
                data[0]=0x02;
                postReply(plant_list_in_waiting.first().query_id,data.constData(),data.size());
                plant_list_in_waiting.removeFirst();
                return;
            }
            index++;
        }
    }*/
    //post the reply
    #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    std::vector<char> data;
    data[0]=0x01;
    postReply(plant_list_in_waiting.front().query_id,data.constData(),data.size());
    #endif

    const auto &current_time=sFrom1970();
    const std::pair<uint8_t,uint8_t> pos(plant_list_in_waiting.front().x,plant_list_in_waiting.front().y);
    #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    const MapServer::PlantOnMap &plantOnMap=static_cast<MapServer *>(plant_list_in_waiting.front().map)->plants.at(pos);
    {
        PlayerPlant plantOnMapPlayer;
        plantOnMapPlayer.plant=plant_list_in_waiting.front().plant_id;
        plantOnMapPlayer.mature_at=current_time+CommonDatapack::commonDatapack.plants.at(plantOnMapPlayer.plant).fruits_seconds;
        public_and_private_informations.plantOnMap[plantOnMap.pointOnMapDbCode]=plantOnMapPlayer;
    }
    {
        syncDatabasePlant();
    }
    #else
    MapServer::PlantOnMap &plantOnMap=static_cast<MapServer *>(plant_list_in_waiting.first().map)->plants[pos];
    {
        plantOnMap.plant=plant_list_in_waiting.front().plant_id;
        plantOnMap.character=character_id;
        plantOnMap.mature_at=current_time+CommonDatapack::commonDatapack.plants.at(plantOnMap.plant).fruits_seconds;
        plantOnMap.player_owned_expire_at=current_time+CommonDatapack::commonDatapack.plants.at(plantOnMap.plant).fruits_seconds+CATCHCHALLENGER_SERVER_OWNER_TIMEOUT;
        static_cast<MapServer *>(plant_list_in_waiting.first().map)->plants[pos]=plantOnMap;
    }
    {
        const std::string &queryText=GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_insert_plant;
        stringreplaceOne(queryText,"%1",std::to_string(character_id));
        stringreplaceOne(queryText,"%2",std::to_string(plantOnMap.pointOnMapDbCode));
        stringreplaceOne(queryText,"%3",std::to_string(plant_list_in_waiting.front().plant_id));
        stringreplaceOne(queryText,"%4",std::to_string(current_time));
        dbQueryWriteServer(queryText);
    }
    #endif

    //send to all player
    #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    {
        std::vector<char> finalData;
        {
            //Insert plant on map
            out << (uint16_t)1;
            if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
                out << (uint8_t)map->id;
            else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
                out << (uint16_t)map->id;
            else
                out << (uint32_t)map->id;
            out << pos.first;
            out << pos.second;
            out << plantOnMap.plant;
            if(current_time>=plantOnMap.mature_at)
                out << (uint16_t)0;
            else if((plantOnMap.mature_at-current_time)>65535)
            {
                normalOutput(std::stringLiteral("sendNearPlant(): remaining seconds to mature is greater than the possibility: map: %1 (%2,%3), plant: %4").arg(map->map_file).arg(x).arg(y).arg(plantOnMap.plant));
                out << (uint16_t)(65535);
            }
            else
                out << (uint16_t)(plantOnMap.mature_at-current_time);
            finalData.resize(16+outputData.size());
            finalData.resize(ProtocolParsingBase::computeOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            false,
            #endif
                    finalData.data(),0xD1,outputData.constData(),outputData.size()));
        }

        uint16_t index=0;
        const uint16_t &size=static_cast<MapServer *>(plant_list_in_waiting.first().map)->clientsForBroadcast.size();
        while(index<size)
        {
            static_cast<MapServer *>(plant_list_in_waiting.first().map)->clientsForBroadcast.at(index)->sendRawSmallPacket(finalData.constData(),finalData.size());
            index++;
        }
    }
    #endif

    plant_list_in_waiting.pop();
}

#ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
void Client::sendNearPlant()
{
    //Insert plant on map
    if(static_cast<MapServer *>(map)->plants.isEmpty())
        return;
    out << static_cast<MapServer *>(map)->plants.size();

    std::unordered_mapIterator<std::pair<uint8_t,uint8_t>,MapServerCrafting::PlantOnMap> i(static_cast<MapServer *>(map)->plants);
    while (i.hasNext()) {
        i.next();

        const MapServerCrafting::PlantOnMap &plant=i.value();
        if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
            out << (uint8_t)map->id;
        else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
            out << (uint16_t)map->id;
        else
            out << (uint32_t)map->id;
        out << i.key().first;
        out << i.key().second;
        out << plant.plant;
        uint64_t current_time=sFrom1970();
        if(current_time>=plant.mature_at)
            out << (uint16_t)0;
        else if((plant.mature_at-current_time)>65535)
        {
            normalOutput(std::stringLiteral("sendNearPlant(): remaining seconds to mature is greater than the possibility: map: %1 (%2,%3), plant: %4").arg(map->map_file).arg(x).arg(y).arg(plant.plant));
            out << (uint16_t)(65535);
        }
        else
            out << (uint16_t)(plant.mature_at-current_time);
        #if defined(DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE) && defined(DEBUG_MESSAGE_MAP_PLANTS)
        int remaining_seconds_to_mature;
        if(current_time>=plant.mature_at)
            remaining_seconds_to_mature=0;
        else
            remaining_seconds_to_mature=(plant.mature_at-current_time);
        normalOutput(std::stringLiteral("insert near plant: map: %1 (%2,%3), plant: %4, seconds to mature: %5 (current_time: %6, plant.mature_at: %7)").arg(map->map_file).arg(x).arg(y).arg(plant.plant).arg(remaining_seconds_to_mature).arg(current_time).arg(plant.mature_at));
        #endif
    }
    sendPacket(0xD1,outputData.constData(),outputData.size());
}

/// \todo if confirmed to useless remove
void Client::removeNearPlant()
{
    #if defined(DEBUG_MESSAGE_MAP_PLANTS)
    normalOutput("removeNearPlant()");
    #endif
    //send the remove plant
    const uint16_t &plant_list_size=static_cast<MapServer *>(map)->plants.size();
    if(plant_list_size==0)
        return;
    out << plant_list_size;
    std::unordered_mapIterator<std::pair<uint8_t,uint8_t>,MapServerCrafting::PlantOnMap> i(static_cast<MapServer *>(map)->plants);
    while (i.hasNext()) {
        i.next();

        if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
            out << (uint8_t)map->id;
        else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
            out << (uint16_t)map->id;
        else
            out << (uint32_t)map->id;
        out << i.key().first;
        out << i.key().second;
        #if defined(DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE) && defined(DEBUG_MESSAGE_MAP_PLANTS)
        normalOutput(std::stringLiteral("remove near plant: map: %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
        #endif
    }
    sendPacket(0xD2,outputData.constData(),outputData.size());
}
#endif

void Client::collectPlant(
        #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        const uint8_t &query_id
        #endif
        )
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    normalOutput("collectPlant("+std::to_string(query_id)+")");
    #else
    normalOutput("collectPlant()");
    #endif
    #endif
    CommonMap *map=this->map;
    uint8_t x=this->x;
    uint8_t y=this->y;
    //resolv the dirt
    switch(getLastDirection())
    {
        case Direction_look_at_top:
            if(MoveOnTheMap::canGoTo(Direction_move_at_top,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_top,&map,&x,&y,false))
                {
                    errorOutput("collectPlant() Can't move at top from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                    return;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_right:
            if(MoveOnTheMap::canGoTo(Direction_move_at_right,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_right,&map,&x,&y,false))
                {
                    errorOutput("collectPlant() Can't move at right from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                    return;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_bottom:
            if(MoveOnTheMap::canGoTo(Direction_move_at_bottom,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_bottom,&map,&x,&y,false))
                {
                    errorOutput("collectPlant() Can't move at bottom from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                    return;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_left:
            if(MoveOnTheMap::canGoTo(Direction_move_at_left,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_left,&map,&x,&y,false))
                {
                    errorOutput("collectPlant() Can't move at left from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
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
            errorOutput("Wrong direction to collect plant (not look): "+MoveOnTheMap::directionToString(getLastDirection()));
        return;
    }
    //check if is dirt
    if(!MoveOnTheMap::isDirt(*map,x,y))
    {
        errorOutput("Try collect plant out of the dirt");
        return;
    }
    //check if is free
    const auto &current_time=sFrom1970();
    const MapServerCrafting::PlantOnMap &plant=static_cast<MapServer *>(map)->plants.at(std::pair<uint8_t,uint8_t>(x,y));
    #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    if(public_and_private_informations.plantOnMap.find(plant.pointOnMapDbCode)!=public_and_private_informations.plantOnMap.cend())
    {
        const PlayerPlant /*can't reference because deleted later*/playerPlant=public_and_private_informations.plantOnMap.at(plant.pointOnMapDbCode);
        if(CommonDatapack::commonDatapack.plants.find(playerPlant.plant)==CommonDatapack::commonDatapack.plants.cend())
        {
            errorOutput("plant not found to collect: "+std::to_string(playerPlant.plant));
            return;
        }
        if(current_time<playerPlant.mature_at)
        {
            errorOutput("current_time<plant.mature_at");
            return;
        }
        //remove from db
        /*std::string queryText=GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_delete_plant_by_index;
        stringreplaceOne(queryText,"%1",std::to_string(character_id));
        stringreplaceOne(queryText,"%2",std::to_string(plant.pointOnMapDbCode));
        dbQueryWriteServer(queryText);*/

        //clear the server dirt
        public_and_private_informations.plantOnMap.erase(plant.pointOnMapDbCode);
        syncDatabasePlant();

        //add into the inventory
        float quantity=CommonDatapack::commonDatapack.plants.at(playerPlant.plant).fix_quantity;
        if((rand()%RANDOM_FLOAT_PART_DIVIDER)<=CommonDatapack::commonDatapack.plants.at(playerPlant.plant).random_quantity)
            quantity++;

        //send the object collected to the current character
        addObjectAndSend(CommonDatapack::commonDatapack.plants.at(playerPlant.plant).itemUsed,quantity);
        return;
    }
    else
    {
        errorOutput("!public_and_private_informations.plantOnMap.contains(plant.indexOfOnMap): "+std::to_string(plant.pointOnMapDbCode)+
                    " at "+std::to_string(x)+","+std::to_string(y));
        return;
    }
    #else
    if(current_time<plant.mature_at)
    {
        std::vector<char> data;
        data[0]=Plant_collect_impossible;
        postReply(query_id,data.constData(),data.size());
        return;
    }
    if(plant.character==0)
    {
        std::vector<char> data;
        data[0]=Plant_collect_empty_dirt;
        postReply(query_id,data.constData(),data.size());
        return;
    }
    //check if owned
    if(plant.character==character_id ||
            current_time>plant.player_owned_expire_at ||
            public_and_private_informations.public_informations.type==Player_type_gm ||
            public_and_private_informations.public_informations.type==Player_type_dev
            )
    {
        //remove plant from db
        dbQueryWriteServer(GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_delete_plant_by_index.arg(character_id).arg(plant.pointOnMapDbCode));

        std::vector<char> finalData;
        {
            //Remove plant on map
            out << (uint16_t)1;
            if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
                out << (uint8_t)map->id;
            else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
                out << (uint16_t)map->id;
            else
                out << (uint32_t)map->id;
            out << x;
            out << y;
            finalData.resize(16+outputData.size());
            finalData.resize(ProtocolParsingBase::computeOutcommingData(
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    false,
    #endif
            finalData.data(),0xD2,outputData.data(),outputData.size()));
        }

        {
            //remove plan from all player display
            int sub_index=0;
            const uint16_t &size=static_cast<MapServer *>(map)->clientsForBroadcast.size();
            while(sub_index<size)
            {
                static_cast<MapServer *>(map)->clientsForBroadcast.at(sub_index)->sendRawSmallPacket(finalData.constData(),finalData.size());
                sub_index++;
            }
        }

        //add into the inventory
        float quantity=CommonDatapack::commonDatapack.plants.value(plant.plant).fix_quantity;
        if((rand()%RANDOM_FLOAT_PART_DIVIDER)<=CommonDatapack::commonDatapack.plants.value(plant.plant).random_quantity)
            quantity++;

        //send the object collected to the current character
        std::vector<char> data;
        data[0]=Plant_collect_correctly_collected;
        postReply(query_id,data.constData(),data.size());
        addObjectAndSend(CommonDatapack::commonDatapack.plants.value(plant.plant).itemUsed,quantity);

        //clear the server dirt
        GlobalServerData::serverPrivateVariables.plantUnusedId << plant.pointOnMapDbCode;

        static_cast<MapServer *>(map)->plants[std::pair<uint8_t,uint8_t>(x,y)].character=0;
        return;
    }
    else
    {
        std::vector<char> data;
        data[0]=Plant_collect_owned_by_another_player;
        postReply(query_id,data.constData(),data.size());
        return;
    }
    #endif
}
