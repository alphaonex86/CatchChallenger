#include "../base/Client.hpp"
#include "../base/BroadCastWithoutSender.hpp"
#include "../base/MapServer.hpp"
#include "../../general/base/ProtocolParsing.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../base/GlobalServerData.hpp"
#include "../base/PreparedDBQuery.hpp"

#include <chrono>

using namespace CatchChallenger;

void Client::plantSeed(const uint8_t &plant_id)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("plantSeed("+std::to_string(plant_id)+")");
    #endif
    if(CommonDatapack::commonDatapack.get_plants().find(plant_id)==CommonDatapack::commonDatapack.get_plants().cend())
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
        errorOutput("Try put seed out of the dirt: "+std::to_string(x)+","+std::to_string(y));
        return;
    }
    //check if is free
    {
        const MapServer::PlantOnMap &plantOnMap=static_cast<MapServer *>(map)->plants.at(std::pair<uint8_t,uint8_t>(x,y));
        if(public_and_private_informations.plantOnMap.find(plantOnMap.pointOnMapDbCode)!=public_and_private_informations.plantOnMap.cend())
        {
            errorOutput("Have already a plant in plantOnlyVisibleByPlayer==true");
            return;
        }
    }
    //check if have into the inventory
    PlantInWaiting plantInWaiting;
    plantInWaiting.plant_id=plant_id;
    plantInWaiting.map=map;
    plantInWaiting.x=x;
    plantInWaiting.y=y;

    plant_list_in_waiting.push(plantInWaiting);
    useSeed(plant_id);
}

//plant
bool Client::syncDatabasePlant()
{
    if(public_and_private_informations.plantOnMap.size()*(2/*pointOnMap*/+1/*plant*/+8/*timestamps*/)>=sizeof(ProtocolParsingBase::tempBigBufferForOutput))
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

    const auto &current_time=sFrom1970();
    const std::pair<uint8_t,uint8_t> pos(plant_list_in_waiting.front().x,plant_list_in_waiting.front().y);
    const MapServer::PlantOnMap &plantOnMap=static_cast<MapServer *>(plant_list_in_waiting.front().map)->plants.at(pos);
    {
        PlayerPlant plantOnMapPlayer;
        plantOnMapPlayer.plant=plant_list_in_waiting.front().plant_id;
        plantOnMapPlayer.mature_at=current_time+CommonDatapack::commonDatapack.get_plants().at(plantOnMapPlayer.plant).fruits_seconds;
        public_and_private_informations.plantOnMap[plantOnMap.pointOnMapDbCode]=plantOnMapPlayer;
    }
    syncDatabasePlant();

    plant_list_in_waiting.pop();
}

void Client::collectPlant()
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("collectPlant()");
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
    if(public_and_private_informations.plantOnMap.find(plant.pointOnMapDbCode)!=public_and_private_informations.plantOnMap.cend())
    {
        const PlayerPlant /*can't reference because deleted later*/playerPlant=public_and_private_informations.plantOnMap.at(plant.pointOnMapDbCode);
        if(CommonDatapack::commonDatapack.get_plants().find(playerPlant.plant)==CommonDatapack::commonDatapack.get_plants().cend())
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
        float quantity=CommonDatapack::commonDatapack.get_plants().at(playerPlant.plant).fix_quantity;
        if((rand()%RANDOM_FLOAT_PART_DIVIDER)<=CommonDatapack::commonDatapack.get_plants().at(playerPlant.plant).random_quantity)
            quantity++;

        //send the object collected to the current character
        addObjectAndSend(CommonDatapack::commonDatapack.get_plants().at(playerPlant.plant).itemUsed,quantity);
        return;
    }
    else
    {
        errorOutput("!public_and_private_informations.plantOnMap.contains(plant.indexOfOnMap): "+std::to_string(plant.pointOnMapDbCode)+
                    " at "+std::to_string(x)+","+std::to_string(y));
        return;
    }
}
