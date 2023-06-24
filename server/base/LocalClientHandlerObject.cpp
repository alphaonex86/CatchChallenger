#include "Client.hpp"
#include "GlobalServerData.hpp"
#include "../../general/base/CommonDatapack.hpp"

using namespace CatchChallenger;

void Client::addObjectAndSend(const uint16_t &item,const uint32_t &quantity)
{
    addObject(item,quantity);
    //add into the inventory
    uint32_t posOutput=0;

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x55;
    posOutput=1;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(2+2+4);
    posOutput+=4;

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x00;
    posOutput+=1;
    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(item);
    posOutput+=2;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(quantity);
    posOutput+=4;

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::addObject(const uint16_t &item, const uint32_t &quantity, bool databaseSync)
{
    if(CommonDatapack::commonDatapack.get_items().item.find(item)==CommonDatapack::commonDatapack.get_items().item.cend())
    {
        errorOutput("Object "+std::to_string(item)+" is not found into the item list");
        return;
    }
    if(public_and_private_informations.items.find(item)!=public_and_private_informations.items.cend())
    {
        public_and_private_informations.items[item]+=quantity;
        /*std::string queryText=GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_item;
        stringreplaceOne(queryText,"%1",std::to_string(public_and_private_informations.items.at(item)));
        stringreplaceOne(queryText,"%2",std::to_string(item));
        stringreplaceOne(queryText,"%3",std::to_string(character_id));
        dbQueryWriteCommon(queryText);*/
        if(databaseSync)
            updateObjectInDatabase();
    }
    else
    {
        /*std::string queryText=GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_insert_item;
        stringreplaceOne(queryText,"%1",std::to_string(item));
        stringreplaceOne(queryText,"%2",std::to_string(character_id));
        stringreplaceOne(queryText,"%3",std::to_string(quantity));
        dbQueryWriteCommon(queryText);*/
        public_and_private_informations.items[item]=quantity;
        if(public_and_private_informations.encyclopedia_item==NULL)
        {
            if(databaseSync)
                updateObjectInDatabase();
        }
        else if(!(public_and_private_informations.encyclopedia_item[item/8] & (1<<(7-item%8))))
        {
            public_and_private_informations.encyclopedia_item[item/8]|=(1<<(7-item%8));
            if(databaseSync)
                updateObjectInDatabaseAndEncyclopedia();
        }
        else
        {
            if(databaseSync)
                updateObjectInDatabase();
        }
    }
}

void Client::saveObjectRetention(const uint16_t &item)
{
    (void)item;
    /*if(public_and_private_informations.items.find(item)!=public_and_private_informations.items.cend())
    {
        std::string queryText=GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_item;
        stringreplaceOne(queryText,"%1",std::to_string(public_and_private_informations.items.at(item)));
        stringreplaceOne(queryText,"%2",std::to_string(item));
        stringreplaceOne(queryText,"%3",std::to_string(character_id));
        dbQueryWriteCommon(queryText);
    }
    else
    {
        std::string queryText=GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_delete_item;
        stringreplaceOne(queryText,"%1",std::to_string(item));
        stringreplaceOne(queryText,"%2",std::to_string(character_id));
        dbQueryWriteCommon(queryText);
    }*/
    updateObjectInDatabase();
}

uint32_t Client::removeObject(const uint16_t &item, const uint32_t &quantity, bool databaseSync)
{
    if(public_and_private_informations.items.find(item)!=public_and_private_informations.items.cend())
    {
        if(public_and_private_informations.items.at(item)>quantity)
        {
            public_and_private_informations.items[item]-=quantity;
            /*std::string queryText=GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_item;
            stringreplaceOne(queryText,"%1",std::to_string(public_and_private_informations.items.at(item)));
            stringreplaceOne(queryText,"%2",std::to_string(item));
            stringreplaceOne(queryText,"%3",std::to_string(character_id));
            dbQueryWriteCommon(queryText);*/
            if(databaseSync)
                updateObjectInDatabase();
            return quantity;
        }
        else
        {
            const uint32_t removed_quantity=public_and_private_informations.items.at(item);
            public_and_private_informations.items.erase(item);
            /*
            std::string queryText=GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_delete_item;
            stringreplaceOne(queryText,"%1",std::to_string(item));
            stringreplaceOne(queryText,"%2",std::to_string(character_id));
            dbQueryWriteCommon(queryText);*/
            if(databaseSync)
                updateObjectInDatabase();
            return removed_quantity;
        }
    }
    else
        return 0;
}

void Client::sendRemoveObject(const uint16_t &item,const uint32_t &quantity)
{
    //remove into the inventory
    uint32_t posOutput=0;

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x56;
    posOutput=1;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(2+2+4);
    posOutput+=4;

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x00;
    posOutput+=1;
    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(item);
    posOutput+=2;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(quantity);
    posOutput+=4;

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

uint32_t Client::objectQuantity(const uint16_t &item) const
{
    if(public_and_private_informations.items.find(item)!=public_and_private_informations.items.cend())
        return public_and_private_informations.items.at(item);
    else
        return 0;
}

void Client::destroyObject(const uint16_t &itemId,const uint32_t &quantity)
{
    normalOutput("The player have destroy them self "+std::to_string(quantity)+" item(s) with id: "+std::to_string(itemId));
    removeObject(itemId,quantity);
}

bool Client::useObjectOnMonsterByPosition(const uint16_t &object, const uint8_t &monsterPosition)
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
    if(CommonFightEngine::useObjectOnMonsterByPosition(object,monsterPosition))
    {
        if(CommonDatapack::commonDatapack.get_items().item.at(object).consumeAtUse)
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
    if(CommonDatapack::commonDatapack.get_items().item.at(itemId).consumeAtUse)
        removeObject(itemId);
    //if is crafting recipe
    if(CommonDatapack::commonDatapack.get_itemToCraftingRecipes().find(itemId)!=CommonDatapack::commonDatapack.get_itemToCraftingRecipes().cend())
    {
        const uint16_t &recipeId=CommonDatapack::commonDatapack.get_itemToCraftingRecipes().at(itemId);
        if(public_and_private_informations.recipes[recipeId/8] & (1<<(7-recipeId%8)))
        {
            errorOutput("Can't use the object: "+std::to_string(itemId)+", recipe already registred");
            return;
        }
        if(!haveReputationRequirements(CommonDatapack::commonDatapack.get_craftingRecipes().at(recipeId).requirements.reputation))
        {
            errorOutput("The player have not the requirement: "+std::to_string(recipeId)+" to to learn crafting recipe");
            return;
        }
        public_and_private_informations.recipes[recipeId/8]|=(1<<(7-recipeId%8));

        removeFromQueryReceived(query_id);
        //send the network reply

        uint32_t posOutput=0;

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
        posOutput+=1;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(1);
        posOutput+=4;

        //type
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)ObjectUsage_correctlyUsed;
        posOutput+=1;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
        //add into db, bit to save
        {
            GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_recipe.asyncWrite({
                        binarytoHexa(public_and_private_informations.recipes,CommonDatapack::commonDatapack.get_craftingRecipesMaxId()/8+1),
                        std::to_string(character_id)
                        });
        }
        #elif CATCHCHALLENGER_DB_BLACKHOLE
        #else
        #error Define what do here
        #endif
    }
    //use trap into fight
    else if(CommonDatapack::commonDatapack.get_items().trap.find(itemId)!=CommonDatapack::commonDatapack.get_items().trap.cend())
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

        removeFromQueryReceived(query_id);
        //send the network reply

        uint32_t posOutput=0;

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
        posOutput+=1;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(1);
        posOutput+=4;

        //type
        if(maxMonsterId>0)
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)ObjectUsage_correctlyUsed;
        else
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)ObjectUsage_failedWithConsumption;
        posOutput+=1;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
    //use repel into fight
    else if(CommonDatapack::commonDatapack.get_items().repel.find(itemId)!=CommonDatapack::commonDatapack.get_items().repel.cend())
    {
        public_and_private_informations.repel_step+=CommonDatapack::commonDatapack.get_items().repel.at(itemId);

        removeFromQueryReceived(query_id);
        //send the network reply

        uint32_t posOutput=0;

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
        posOutput+=1;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(1);
        posOutput+=4;

        //type
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)ObjectUsage_correctlyUsed;
        posOutput+=1;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
    else
    {
        errorOutput("can't use the object: "+std::to_string(itemId)+" because don't have an usage");
        return;
    }
}
