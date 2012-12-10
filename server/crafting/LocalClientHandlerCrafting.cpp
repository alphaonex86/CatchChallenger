#include "../base/LocalClientHandler.h"
#include "../../general/base/ProtocolParsing.h"
#include "../base/GlobalData.h"

using namespace Pokecraft;

void LocalClientHandler::useSeed(const quint8 &plant_id)
{
    if(objectQuantity(GlobalData::serverPrivateVariables.plants[plant_id].itemUsed)==0)
    {
        emit error(QString("The player have not the item id: %1 to plant as seed").arg(GlobalData::serverPrivateVariables.plants[plant_id].itemUsed));
        return;
    }
    else
    {
        removeObject(GlobalData::serverPrivateVariables.plants[plant_id].itemUsed);
        emit seedValidated();
    }
}

void LocalClientHandler::useRecipe(const quint8 &query_id,const quint32 &recipe_id)
{
    //don't check if the recipe exists, because the loading of the know recide do that's
    if(!player_informations->public_and_private_informations.recipes.contains(recipe_id))
    {
        emit error(QString("The player have not this recipe as know: %1").arg(recipe_id));
        return;
    }
    const CrafingRecipe &recipe=GlobalData::serverPrivateVariables.crafingRecipes[recipe_id];
    //check if have material
    int index=0;
    int materials_size=recipe.materials.size();
    while(index<materials_size)
    {
        if(objectQuantity(recipe.materials.at(index).itemId)<recipe.materials.at(index).quantity)
        {
            emit error(QString("The player have only: %1 of item: %2, can't craft").arg(objectQuantity(recipe.materials.at(index).itemId)).arg(recipe.materials.at(index).itemId));
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
        removeObject(recipe.materials.at(index).itemId,recipe.materials.at(index).quantity);
        index++;
    }
    //give the item
    if(success)
        addObject(recipe.doItemId,recipe.quantity);
    //send the reply
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    if(success)
        out << (quint32)0x01;
    else
        out << (quint32)0x03;
    emit postReply(query_id,outputData);
}
