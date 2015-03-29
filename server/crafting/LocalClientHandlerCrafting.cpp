#include "../base/Client.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/CommonDatapack.h"
#include "../base/GlobalServerData.h"
#include "../base/MapServer.h"

using namespace CatchChallenger;

void Client::useSeed(const quint8 &plant_id)
{
    if(!haveReputationRequirements(CommonDatapack::commonDatapack.plants.value(plant_id).requirements.reputation))
    {
        errorOutput(QStringLiteral("The player have not the requirement: %1 to plant as seed").arg(CommonDatapack::commonDatapack.plants.value(plant_id).itemUsed));
        return;
    }
    else if(objectQuantity(CommonDatapack::commonDatapack.plants.value(plant_id).itemUsed)==0)
    {
        errorOutput(QStringLiteral("The player have not the item id: %1 to plant as seed").arg(CommonDatapack::commonDatapack.plants.value(plant_id).itemUsed));
        return;
    }
    else
    {
        appendReputationRewards(CommonDatapack::commonDatapack.plants.value(plant_id).rewards.reputation);
        removeObject(CommonDatapack::commonDatapack.plants.value(plant_id).itemUsed);
        seedValidated();
    }
}

void Client::useRecipe(const quint8 &query_id,const quint32 &recipe_id)
{
    //don't check if the recipe exists, because the loading of the know recide do that's
    if(!public_and_private_informations.recipes.contains(recipe_id))
    {
        errorOutput(QStringLiteral("The player have not this recipe as know: %1").arg(recipe_id));
        return;
    }
    const CrafingRecipe &recipe=CommonDatapack::commonDatapack.crafingRecipes.value(recipe_id);
    //check if have material
    int index=0;
    const int &materials_size=recipe.materials.size();
    while(index<materials_size)
    {
        if(objectQuantity(recipe.materials.at(index).item)<recipe.materials.at(index).quantity)
        {
            errorOutput(QStringLiteral("The player have only: %1 of item: %2, can't craft").arg(objectQuantity(recipe.materials.at(index).item)).arg(recipe.materials.at(index).item));
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
    //send the reply
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    if(success)
        out << (quint8)RecipeUsage_ok;
    else
        out << (quint8)RecipeUsage_failed;
    postReply(query_id,outputData);
}

void Client::takeAnObjectOnMap()
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QStringLiteral("takeAnObjectOnMap()"));
    #endif
    CommonMap *map=this->map;
    quint8 x=this->x;
    quint8 y=this->y;
    //resolv the dirt
    switch(getLastDirection())
    {
        case Direction_look_at_top:
            if(MoveOnTheMap::canGoTo(Direction_move_at_top,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_top,&map,&x,&y,false))
                {
                    errorOutput(QStringLiteral("takeAnObjectOnMap() Can't move at top from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
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
                    errorOutput(QStringLiteral("takeAnObjectOnMap() Can't move at right from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
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
                    errorOutput(QStringLiteral("takeAnObjectOnMap() Can't move at bottom from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
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
                    errorOutput(QStringLiteral("takeAnObjectOnMap() Can't move at left from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
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
    //check if is dirt
    if(!static_cast<MapServer *>(map)->itemsOnMap.contains(QPair<quint8,quint8>(x,y)))
    {
        errorOutput("Not on map item on this place");
        return;
    }
    const MapServer::ItemOnMap &item=static_cast<MapServer *>(map)->itemsOnMap.value(QPair<quint8,quint8>(x,y));
    if(public_and_private_informations.itemOnMap.contains(item.itemIndexOnMap))
    {
        errorOutput("Have already this item");
        return;
    }
    public_and_private_informations.itemOnMap << item.itemIndexOnMap;
    //add get item from db
    if(!item.infinite)
        dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_itemonmap.arg(character_id).arg(item.itemDbCode));
    addObject(item.item);
}
