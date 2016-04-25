#include "../base/Client.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/CommonDatapack.h"
#include "../base/GlobalServerData.h"
#include "../base/MapServer.h"
#include "../base/PreparedDBQuery.h"

using namespace CatchChallenger;

void Client::useSeed(const uint8_t &plant_id)
{
    if(!haveReputationRequirements(CommonDatapack::commonDatapack.plants.at(plant_id).requirements.reputation))
    {
        errorOutput("Don't have the requirement: "+std::to_string(CommonDatapack::commonDatapack.plants.at(plant_id).itemUsed)+" to plant as seed");
        return;
    }
    else if(objectQuantity(CommonDatapack::commonDatapack.plants.at(plant_id).itemUsed)==0)
    {
        errorOutput("Don't have the item id: "+std::to_string(CommonDatapack::commonDatapack.plants.at(plant_id).itemUsed)+" to plant as seed");
        return;
    }
    else
    {
        appendReputationRewards(CommonDatapack::commonDatapack.plants.at(plant_id).rewards.reputation);
        removeObject(CommonDatapack::commonDatapack.plants.at(plant_id).itemUsed);
        seedValidated();
    }
}

void Client::useRecipe(const uint8_t &query_id,const uint32_t &recipe_id)
{
    //don't check if the recipe exists, because the loading of the know recide do that's
    if(public_and_private_informations.recipes[recipe_id/8] & (1<<(7-recipe_id%8)))
    {
        errorOutput("The player have not this recipe as know: "+std::to_string(recipe_id));
        return;
    }
    const CrafingRecipe &recipe=CommonDatapack::commonDatapack.crafingRecipes.at(recipe_id);
    //check if have material
    int index=0;
    const int &materials_size=recipe.materials.size();
    while(index<materials_size)
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
    while(index<materials_size)
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
                    errorOutput("takeAnObjectOnMap() Can't move at top from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
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
                    errorOutput("takeAnObjectOnMap() Can't move at right from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
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
                    errorOutput("takeAnObjectOnMap() Can't move at bottom from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
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
                    errorOutput("takeAnObjectOnMap() Can't move at left from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
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
        errorOutput("Wrong direction to plant a seed");
        return;
    }
    std::pair<uint8_t,uint8_t> pos(x,y);
    //check if is dirt
    if(static_cast<MapServer *>(map)->itemsOnMap.find(pos)==static_cast<MapServer *>(map)->itemsOnMap.cend())
    {
        errorOutput("Not on map item on this place");
        return;
    }
    const MapServer::ItemOnMap &item=static_cast<MapServer *>(map)->itemsOnMap.at(pos);
    //add get item from db
    if(!item.infinite)
    {
        if(public_and_private_informations.itemOnMap.find(item.pointOnMapDbCode)!=public_and_private_informations.itemOnMap.cend())
        {
            errorOutput("Have already this item");
            return;
        }
        public_and_private_informations.itemOnMap.insert(item.pointOnMapDbCode);

        /*const std::string &queryText=PreparedDBQueryServer::db_query_update_itemonmap.co;
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
    if(public_and_private_informations.plantOnMap.size()*(1)>=sizeof(ProtocolParsingBase::tempBigBufferForOutput))
    {
        errorOutput("Too many item on map");
        return false;
    }
    else
    {
        uint32_t posOutput=0;
        auto i=public_and_private_informations.itemOnMap.begin();
        while(i!=public_and_private_informations.itemOnMap.cend())
        {
            const uint8_t &itemOnMap=*i;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=itemOnMap;
            posOutput+=1;

            ++i;
        }
        const std::string &queryText=PreparedDBQueryServer::db_query_update_itemonmap.compose(
                    binarytoHexa(ProtocolParsingBase::tempBigBufferForOutput,posOutput),
                    std::to_string(character_id)
                    );
        dbQueryWriteServer(queryText);
        return true;
    }
}
