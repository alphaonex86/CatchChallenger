#include "../base/Client.hpp"
#include "../../general/base/ProtocolParsing.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../base/MapManagement/Map_server_MapVisibility_Simple_StoreOnSender.hpp"
#include "../base/GlobalServerData.hpp"
#include "../base/MapServer.hpp"
#include "../base/PreparedDBQuery.hpp"

using namespace CatchChallenger;

void Client::useSeed(const uint8_t &plant_id,const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const COORD_TYPE &x,const COORD_TYPE &y)
{
    if(!haveReputationRequirements(CommonDatapack::commonDatapack.get_plants().at(plant_id).requirements.reputation))
    {
        errorOutput("Don't have the requirement: "+std::to_string(CommonDatapack::commonDatapack.get_plants().at(plant_id).itemUsed)+" to plant as seed");
        return;
    }
    else if(objectQuantity(CommonDatapack::commonDatapack.get_plants().at(plant_id).itemUsed)==0)
    {
        errorOutput("Don't have the item id: "+std::to_string(CommonDatapack::commonDatapack.get_plants().at(plant_id).itemUsed)+" to plant as seed");
        return;
    }
    else
    {
        appendReputationRewards(CommonDatapack::commonDatapack.get_plants().at(plant_id).rewards.reputation);
        removeObject(CommonDatapack::commonDatapack.get_plants().at(plant_id).itemUsed);
        seedValidated(plant_id,mapIndex,x,y);
    }
}

void Client::useRecipe(const uint8_t &query_id,const uint16_t &recipe_id)
{
    //don't check if the recipe exists, because the loading of the know recide do that's
    if(!(public_and_private_informations.recipes[recipe_id/8] & (1<<(7-recipe_id%8))))
    {
        errorOutput("The player have not this recipe registred: "+std::to_string(recipe_id));
        return;
    }
    const CraftingRecipe &recipe=CommonDatapack::commonDatapack.get_craftingRecipes().at(recipe_id);
    //check if have material
    unsigned int index=0;
    while(index<recipe.materials.size())
    {
        if(objectQuantity(recipe.materials.at(index).item)<recipe.materials.at(index).quantity)
        {
            errorOutput("Have only: "+std::to_string(objectQuantity(recipe.materials.at(index).item))+" of item: "+std::to_string(recipe.materials.at(index).item)+", can't craft");
            return;
        }
        index++;
    }
    //do the random part
    bool success=true;
    if(recipe.success<100)
        if(rand()%100>(recipe.success-1))
            success=false;
    //take the material
    index=0;
    while(index<recipe.materials.size())
    {
        removeObject(recipe.materials.at(index).item,recipe.materials.at(index).quantity);
        index++;
    }
    //give the item
    if(success)
    {
        appendReputationRewards(recipe.rewards.reputation);
        addObject(recipe.doItemId,recipe.quantity);
    }

    //send the network reply
    removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size

    if(success)
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)RecipeUsage_ok;
    else
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)RecipeUsage_failed;
    posOutput+=1;

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::takeAnObjectOnMap()
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("takeAnObjectOnMap()");
    #endif
    COORD_TYPE new_x=0,new_y=0;
    const Map_server_MapVisibility_Simple_StoreOnSender * new_map=Client::mapAndPosIfMoveInLookingDirectionJumpColision(new_x,new_y);
    if(new_map==nullptr)
    {
        errorOutput("Can't move at this direction from "+std::to_string(mapIndex)+" ("+std::to_string(x)+","+std::to_string(y)+")");
        return;
    }
    //check if is item
    if(new_map->items.find(std::pair<uint8_t,uint8_t>(new_x,new_y))==new_map->items.cend())
    {
        errorOutput("Not on map item on this place: "+std::to_string(new_map->id)+" at "+std::to_string(new_x)+","+std::to_string(new_y));
        return;
    }
    Player_private_and_public_informations_Map &mapData=public_and_private_informations.mapData[new_map->id];
    if(mapData.plantOnMap.find(std::pair<uint8_t,uint8_t>(new_x,new_y))!=mapData.plantOnMap.cend())
    {
        errorOutput("Have already this item: "+std::to_string(new_map->id)+" at "+std::to_string(new_x)+","+std::to_string(new_y));
        return;
    }
    const ItemOnMap &item=new_map->items.at(std::pair<uint8_t,uint8_t>(new_x,new_y));
    //add get item from db
    if(!item.infinite)
    {
        mapData.items.insert(std::pair<uint8_t,uint8_t>(new_x,new_y));

        /*const std::string &queryText=GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_update_itemonmap.co;
        stringreplaceOne(queryText,"%1",std::to_string(character_id));
        stringreplaceOne(queryText,"%2",std::to_string(item.pointOnMapDbCode));
        dbQueryWriteServer(queryText);*/
        syncDatabaseItemOnMap();
    }
    addObject(item.item);
}

//item on map
bool Client::syncDatabaseItemOnMap()
{
    return true;
    //if(public_and_private_informations.plantOnMap.size()*(1)>=sizeof(ProtocolParsingBase::tempBigBufferForOutput))
    /*{
        errorOutput("Too many item on map");
        return false;
    }
    else
    {
        uint16_t lastItemonmapId=0;
        uint32_t posOutput=0;
        auto i=public_and_private_informations.itemOnMap.begin();
        while(i!=public_and_private_informations.itemOnMap.cend())
        {
            #ifdef MAXIMIZEPERFORMANCEOVERDATABASESIZE
            //not ordened
            uint16_t itemonmapInt;
            if(lastItemonmapId<=*i)
            {
                itemonmapInt=*i-lastItemonmapId;
                lastItemonmapId=*i;
            }
            else
            {
                itemonmapInt=static_cast<uint16_t>(65536-static_cast<uint32_t>(lastItemonmapId)+static_cast<uint32_t>(*i));
                lastItemonmapId=*i;
            }
            #else
            //ordened
            const uint16_t &itemonmapInt=*i-lastItemonmapId;
            lastItemonmapId=*i;
            #endif
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(itemonmapInt);
            posOutput+=2;

            ++i;
        }
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
        GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_update_itemonmap.asyncWrite({
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
