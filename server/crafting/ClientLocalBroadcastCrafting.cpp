#include "../base/Client.hpp"
#include "../base/MapServer.hpp"
#include "../base/GlobalServerData.hpp"
#include "../base/PreparedDBQuery.hpp"
#include "../base/MapManagement/Map_server_MapVisibility_Simple_StoreOnSender.hpp"
#include "../../general/base/ProtocolParsing.hpp"
#include "../../general/base/CommonDatapack.hpp"

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
    COORD_TYPE new_x=0,new_y=0;
    const Map_server_MapVisibility_Simple_StoreOnSender * new_map=Client::mapAndPosIfMoveInLookingDirectionJumpColision(new_x,new_y);
    if(new_map==nullptr)
    {
        errorOutput("Can't move at this direction from "+std::to_string(mapIndex)+" ("+std::to_string(x)+","+std::to_string(y)+")");
        return;
    }
    //check if is dirt
    if(!MoveOnTheMap::isDirt(*new_map,x,y))
    {
        errorOutput("Try put seed out of the dirt: "+std::to_string(x)+","+std::to_string(y));
        return;
    }
    //check if is free
    {
        const Player_private_and_public_informations_Map &mapData=public_and_private_informations.mapData.at(new_map->id);
        if(mapData.plants.find(std::pair<uint8_t,uint8_t>(x,y))!=mapData.plants.cend())
        {
            errorOutput("Have already a plant in plantOnlyVisibleByPlayer==true");
            return;
        }
    }
    useSeed(plant_id,new_map->id,x,y);
}

//plant
bool Client::syncDatabasePlant()
{
    return true;
    //if(public_and_private_informations.plantOnMap.size()*(2/*pointOnMap*/+1/*plant*/+8/*timestamps*/)>=sizeof(ProtocolParsingBase::tempBigBufferForOutput))
    {
        errorOutput("Too many plant");
        return false;
    }
    /*else
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
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
        GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_update_plant.asyncWrite({
                    binarytoHexa(ProtocolParsingBase::tempBigBufferForOutput,posOutput),
                    std::to_string(character_id)
                    });
        #elif CATCHCHALLENGER_DB_BLACKHOLE
        #elif CATCHCHALLENGER_DB_FILE
        #else
        #error Define what do here
        #endif
        return true;
    }*/
}

void Client::seedValidated(const uint8_t &plant_id,const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const COORD_TYPE &x,const COORD_TYPE &y)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("seedValidated()");
    #endif
    //post the reply

    const auto &current_time=sFrom1970();
    const std::pair<uint8_t,uint8_t> pos(x,y);
    Player_private_and_public_informations_Map &mapData=public_and_private_informations.mapData[mapIndex];
    PlayerPlant plantOnMapPlayer;
    plantOnMapPlayer.plant=plant_id;
    plantOnMapPlayer.mature_at=current_time+CommonDatapack::commonDatapack.get_plants().at(plantOnMapPlayer.plant).fruits_seconds;
    mapData.plants[pos]=plantOnMapPlayer;
    syncDatabasePlant();
}

void Client::collectPlant()
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("collectPlant()");
    #endif
    COORD_TYPE new_x=0,new_y=0;
    const Map_server_MapVisibility_Simple_StoreOnSender * new_map=Client::mapAndPosIfMoveInLookingDirectionJumpColision(new_x,new_y);
    if(new_map==nullptr)
    {
        errorOutput("Can't move at this direction from "+std::to_string(mapIndex)+" ("+std::to_string(x)+","+std::to_string(y)+")");
        return;
    }
    //check if is dirt
    if(!MoveOnTheMap::isDirt(*new_map,x,y))
    {
        errorOutput("Try collect plant out of the dirt");
        return;
    }
    //check if is free
    const auto &current_time=sFrom1970();
    Player_private_and_public_informations_Map &mapData=public_and_private_informations.mapData[new_map->id];
    if(mapData.plants.find(std::pair<uint8_t,uint8_t>(x,y))!=mapData.plants.cend())
    {
        const PlayerPlant /*can't reference because deleted later*/playerPlant=mapData.plants.at(std::pair<uint8_t,uint8_t>(x,y));
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

        //clear the server dirt
        mapData.plants.erase(std::pair<uint8_t,uint8_t>(x,y));
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
        errorOutput("!public_and_private_informations.plantOnMap.contains(plant.indexOfOnMap): "+std::to_string(new_map->id)+","+
                    " at "+std::to_string(x)+","+std::to_string(y));
        return;
    }
}
