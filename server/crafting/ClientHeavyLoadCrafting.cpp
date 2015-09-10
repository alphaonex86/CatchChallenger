#include "../base/Client.h"
#include "../base/PreparedDBQuery.h"
#include "../base/GlobalServerData.h"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLib.h"

using namespace CatchChallenger;

void Client::loadRecipes()
{
    //recipes
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_select_recipes_by_player_id.empty())
    {
        errorOutput("loadRecipes() Query is empty, bug");
        return;
    }
    #endif
    std::string queryText=PreparedDBQueryCommon::db_query_select_recipes_by_player_id;
    stringreplaceOne(queryText,"%1",std::to_string(character_id));

    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&Client::loadRecipes_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        loadMonsters();
        return;
    }
    else
        callbackRegistred.push(callback);
}

void Client::loadRecipes_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadRecipes_return();
}

void Client::loadRecipes_return()
{
    callbackRegistred.pop();
    bool ok;
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        const uint32_t &recipeId=stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
        if(ok)
        {
            if(CommonDatapack::commonDatapack.crafingRecipes.find(recipeId)!=CommonDatapack::commonDatapack.crafingRecipes.cend())
                public_and_private_informations.recipes.insert(recipeId);
            else
                normalOutput("recipeId: "+std::to_string(recipeId)+" is not into recipe list");
        }
        else
            normalOutput("recipeId: "+GlobalServerData::serverPrivateVariables.db_common->value(0)+" is not a number");
    }
    loadMonsters();
}

void Client::loadItems()
{
    //do the query
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_select_items_by_player_id.empty())
    {
        errorOutput("loadItems() Query is empty, bug");
        return;
    }
    #endif
    std::string queryText=PreparedDBQueryCommon::db_query_select_items_by_player_id;
    stringreplaceOne(queryText,"%1",std::to_string(character_id));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&Client::loadItems_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        loadItemsWarehouse();
        return;
    }
    else
        callbackRegistred.push(callback);
}

void Client::loadItems_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadItems_return();
}

void Client::loadItems_return()
{
    callbackRegistred.pop();
    bool ok;
    //parse the result
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        const uint32_t &id=stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
        if(!ok)
        {
            normalOutput("item id is not a number, skip");
            continue;
        }
        const uint32_t &quantity=stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(1),&ok);
        if(!ok)
        {
            normalOutput("quantity is not a number, skip");
            continue;
        }
        if(quantity==0)
        {
            normalOutput("The item "+std::to_string(id)+" have been dropped because the quantity is 0");
            continue;
        }
        if(CommonDatapack::commonDatapack.items.item.find(id)==CommonDatapack::commonDatapack.items.item.cend())
        {
            normalOutput("The item "+std::to_string(id)+" is ignored because it's not into the items list");
            continue;
        }
        public_and_private_informations.items[id]=quantity;
    }
    loadItemsWarehouse();
}

void Client::loadItemsWarehouse()
{
    //do the query
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_select_items_warehouse_by_player_id.empty())
    {
        errorOutput("loadItems() Query is empty, bug");
        return;
    }
    #endif
    std::string queryText=PreparedDBQueryCommon::db_query_select_items_warehouse_by_player_id;
    stringreplaceOne(queryText,"%1",std::to_string(character_id));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&Client::loadItemsWarehouse_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        loadRecipes();
        return;
    }
    else
        callbackRegistred.push(callback);
}

void Client::loadItemsWarehouse_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadItemsWarehouse_return();
}

void Client::loadItemsWarehouse_return()
{
    callbackRegistred.pop();
    bool ok;
    //parse the result
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        const uint32_t &id=stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
        if(!ok)
        {
            normalOutput("item id is not a number, skip");
            continue;
        }
        const uint32_t &quantity=stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(1),&ok);
        if(!ok)
        {
            normalOutput("quantity is not a number, skip");
            continue;
        }
        if(quantity==0)
        {
            normalOutput("The item "+std::to_string(id)+" have been dropped because the quantity is 0");
            continue;
        }
        if(CommonDatapack::commonDatapack.items.item.find(id)==CommonDatapack::commonDatapack.items.item.cend())
        {
            normalOutput("The item "+std::to_string(id)+" is ignored because it's not into the items list");
            continue;
        }
        public_and_private_informations.warehouse_items[id]=quantity;
    }
    loadRecipes();
}

void Client::sendInventory()
{
    //network send
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);

    out << (uint16_t)public_and_private_informations.items.size();
    {
        auto i=public_and_private_informations.items.begin();
        while(i!=public_and_private_informations.items.cend())
        {
            out << (uint16_t)i->first;
            out << (uint32_t)i->second;
            ++i;
        }
    }
    out << (uint16_t)public_and_private_informations.warehouse_items.size();
    {
        auto  j=public_and_private_informations.warehouse_items.begin();
        while(j!=public_and_private_informations.warehouse_items.cend())
        {
            out << (uint16_t)j->first;
            out << (uint32_t)j->second;
            ++j;
        }
    }
    //send the items
    sendFullPacket(0xD0,0x01,outputData.constData(),outputData.size());
}
