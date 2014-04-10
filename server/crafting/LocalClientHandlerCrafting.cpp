#include "../base/LocalClientHandler.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/CommonDatapack.h"
#include "../base/GlobalServerData.h"

using namespace CatchChallenger;

void LocalClientHandler::useSeed(const quint8 &plant_id)
{
    if(!haveReputationRequirements(CommonDatapack::commonDatapack.plants.value(plant_id).requirements.reputation))
    {
        emit error(QStringLiteral("The player have not the requirement: %1 to plant as seed").arg(CommonDatapack::commonDatapack.plants.value(plant_id).itemUsed));
        return;
    }
    else if(objectQuantity(CommonDatapack::commonDatapack.plants.value(plant_id).itemUsed)==0)
    {
        emit error(QStringLiteral("The player have not the item id: %1 to plant as seed").arg(CommonDatapack::commonDatapack.plants.value(plant_id).itemUsed));
        return;
    }
    else
    {
        appendReputationRewards(CommonDatapack::commonDatapack.plants.value(plant_id).rewards.reputation);
        removeObject(CommonDatapack::commonDatapack.plants.value(plant_id).itemUsed);
        emit seedValidated();
    }
}

void LocalClientHandler::useRecipe(const quint8 &query_id,const quint32 &recipe_id)
{
    //don't check if the recipe exists, because the loading of the know recide do that's
    if(!player_informations->public_and_private_informations.recipes.contains(recipe_id))
    {
        emit error(QStringLiteral("The player have not this recipe as know: %1").arg(recipe_id));
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
            emit error(QStringLiteral("The player have only: %1 of item: %2, can't craft").arg(objectQuantity(recipe.materials.at(index).item)).arg(recipe.materials.at(index).item));
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
    out.setVersion(QDataStream::Qt_4_4);
    if(success)
        out << (quint8)RecipeUsage_ok;
    else
        out << (quint8)RecipeUsage_failed;
    emit postReply(query_id,outputData);
}
