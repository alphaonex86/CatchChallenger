#include "Client.hpp"

#include "GlobalServerData.hpp"
#include "PreparedDBQuery.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include <cstring>

using namespace CatchChallenger;

void Client::getMarketList(const uint8_t &query_id)
{
    if(getInTrade() || isInFight())
    {
        errorOutput("You can't use the market in trade/fight");
        return;
    }
    //send the network reply
    removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;

    const uint64_t converted_market_cash=htole64(market_cash);
    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&converted_market_cash,8);
    posOutput+=8;

    unsigned int index;
    std::vector<MarketItem> marketItemList,marketOwnItemList;
    std::vector<MarketPlayerMonster> marketMonsterList,marketOwnMonsterList;
    //object filter
    index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketItemList.size())
    {
        const MarketItem &marketObject=GlobalServerData::serverPrivateVariables.marketItemList.at(index);
        if(marketObject.player==character_id)
            marketOwnItemList.push_back(marketObject);
        else
            marketItemList.push_back(marketObject);
        index++;
    }
    //monster filter
    index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.size())
    {
        const MarketPlayerMonster &marketPlayerMonster=GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.at(index);
        if(marketPlayerMonster.player==character_id)
            marketOwnMonsterList.push_back(marketPlayerMonster);
        else
            marketMonsterList.push_back(marketPlayerMonster);
        index++;
    }
    //object
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(marketItemList.size());
    posOutput+=4;
    index=0;
    while(index<marketItemList.size())
    {
        const MarketItem &marketObject=marketItemList.at(index);
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(marketObject.marketObjectUniqueId);
        posOutput+=4;
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(marketObject.item);
        posOutput+=2;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(marketObject.quantity);
        posOutput+=4;
        const uint64_t &cash_temp=htole64(marketObject.price);
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&cash_temp,sizeof(uint64_t));
        posOutput+=sizeof(uint64_t);
        index++;
    }
    //monster
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(marketMonsterList.size());
    posOutput+=4;
    index=0;
    while(index<marketMonsterList.size())
    {
        const MarketPlayerMonster &marketPlayerMonster=marketMonsterList.at(index);
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(marketPlayerMonster.monster.id);
        posOutput+=4;
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(marketPlayerMonster.monster.monster);
        posOutput+=2;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=marketPlayerMonster.monster.level;
        posOutput+=1;
        const uint64_t &cash_temp=htole64(marketPlayerMonster.price);
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&cash_temp,sizeof(uint64_t));
        posOutput+=sizeof(uint64_t);
        index++;
    }
    //own object
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(marketOwnItemList.size());
    posOutput+=4;
    index=0;
    while(index<marketOwnItemList.size())
    {
        const MarketItem &marketObject=marketOwnItemList.at(index);
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(marketObject.marketObjectUniqueId);
        posOutput+=4;
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(marketObject.item);
        posOutput+=2;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(marketObject.quantity);
        posOutput+=4;
        const uint64_t &cash_temp=htole64(marketObject.price);
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&cash_temp,sizeof(uint64_t));
        posOutput+=sizeof(uint64_t);
        index++;
    }
    //own monster
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(marketOwnMonsterList.size());
    posOutput+=4;
    index=0;
    while(index<marketOwnMonsterList.size())
    {
        const MarketPlayerMonster &marketPlayerMonster=marketOwnMonsterList.at(index);
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(marketPlayerMonster.monster.id);
        posOutput+=4;
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(marketPlayerMonster.monster.monster);
        posOutput+=2;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=marketPlayerMonster.monster.level;
        posOutput+=1;
        const uint64_t &cash_temp=htole64(marketPlayerMonster.price);
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&cash_temp,sizeof(uint64_t));
        posOutput+=sizeof(uint64_t);
        index++;
    }

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);//set the dynamic size
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::buyMarketObject(const uint8_t &query_id,const uint32_t &marketObjectId,const uint32_t &quantity)
{
    if(getInTrade() || isInFight())
    {
        errorOutput("You can't use the market in trade/fight");
        return;
    }
    if(quantity<=0)
    {
        errorOutput("You can't use the market with null quantity");
        return;
    }
    //send the network reply
    removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;

    //search into the market
    unsigned int index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketItemList.size())
    {
        const MarketItem marketItem=GlobalServerData::serverPrivateVariables.marketItemList.at(index);
        if(marketItem.marketObjectUniqueId==marketObjectId)
        {
            if(marketItem.quantity<quantity)
            {
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
                posOutput+=1;
                sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                return;
            }
            //check if have the price
            if((quantity*marketItem.price)>public_and_private_informations.cash)
            {
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x03;
                posOutput+=1;
                sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                return;
            }
            //apply the buy
            if(marketItem.quantity==quantity)
            {
                GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_delete_item_market.asyncWrite({
                            std::to_string(marketItem.item),
                            std::to_string(marketItem.player)
                            });
                GlobalServerData::serverPrivateVariables.marketItemList.erase(GlobalServerData::serverPrivateVariables.marketItemList.begin()+index);
            }
            else
            {
                GlobalServerData::serverPrivateVariables.marketItemList[index].quantity=marketItem.quantity-quantity;
                GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_update_item_market.asyncWrite({
                            std::to_string(marketItem.quantity-quantity),
                            std::to_string(marketItem.item),
                            std::to_string(marketItem.player)
                            });
            }
            removeCash(quantity*marketItem.price);
            if(playerById.find(marketItem.player)!=playerById.cend())
            {
                if(!playerById.at(marketItem.player)->addMarketCashWithoutSave(quantity*marketItem.price))
                    normalOutput("Problem at market cash adding");
            }
            GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_update_charaters_market_cash.asyncWrite({
                        std::to_string(quantity*marketItem.price),
                        std::to_string(marketItem.player)
                        });
            addObject(marketItem.item,quantity);

            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
            posOutput+=1;
            sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            return;
        }
        index++;
    }
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x03;
    posOutput+=1;
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::buyMarketMonster(const uint8_t &query_id,const uint32_t &marketMonsterUniqueId)
{
    if(getInTrade() || isInFight())
    {
        errorOutput("You can't use the market in trade/fight");
        return;
    }
    //send the network reply
    removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;

    if(public_and_private_informations.playerMonster.size()>=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters)
    {
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
        posOutput+=1;
        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        return;
    }
    //search into the market
    unsigned int index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.size())
    {
        const MarketPlayerMonster marketPlayerMonster=GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.at(index);
        if(marketPlayerMonster.monster.id==marketMonsterUniqueId)
        {
            //check if have the price
            if(marketPlayerMonster.price>public_and_private_informations.cash)
            {
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x03;
                posOutput+=1;
                sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                return;
            }
            //apply the buy
            GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.erase(GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.begin()+index);
            removeCash(marketPlayerMonster.price);
            //entry created at first server connexion
            GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_update_charaters_market_cash.asyncWrite({
                        std::to_string(marketPlayerMonster.price),
                        std::to_string(marketPlayerMonster.player)
                        });
            if(playerById.find(marketPlayerMonster.player)!=playerById.cend())
            {
                if(!playerById.at(marketPlayerMonster.player)->addMarketCashWithoutSave(marketPlayerMonster.price))
                    normalOutput("Problem at market cash adding");
            }
            addPlayerMonster(marketPlayerMonster.monster);

            GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_delete_monster_market_price.asyncWrite({
                        std::to_string(marketPlayerMonster.monster.id)
                        });
            GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_move_to_new_player.asyncWrite({
                        std::to_string(character_id),
                        std::to_string(getPlayerMonster().size()),
                        std::to_string(marketPlayerMonster.monster.id)
                        });

            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
            posOutput+=1;

            //lot of data not exposed via the simplified data into the market list
            posOutput+=FacilityLib::privateMonsterToBinary(ProtocolParsingBase::tempBigBufferForOutput+posOutput,marketPlayerMonster.monster,marketPlayerMonster.player);

            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);//set the dynamic size

            sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            return;
        }
        index++;
    }
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x03;
    posOutput+=1;
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::putMarketObject(const uint8_t &query_id,const uint16_t &objectId,const uint32_t &quantity,const uint64_t &price)
{
    if(getInTrade() || isInFight())
    {
        errorOutput("You can't use the market in trade/fight");
        return;
    }
    if(quantity<=0)
    {
        errorOutput("You can't use the market with null quantity");
        return;
    }
    //send the network reply
    removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;

    if(objectQuantity(objectId)<quantity)
    {
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
        posOutput+=1;
        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        return;
    }
    //search into the market
    unsigned int index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketItemList.size())
    {
        const MarketItem &marketItem=GlobalServerData::serverPrivateVariables.marketItemList.at(index);
        if(marketItem.player==character_id && marketItem.item==objectId)
        {
            removeObject(objectId,quantity);
            GlobalServerData::serverPrivateVariables.marketItemList[index].price=price;
            GlobalServerData::serverPrivateVariables.marketItemList[index].quantity+=quantity;

            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
            posOutput+=1;
            sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

            GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_update_item_market_and_price.asyncWrite({
                        std::to_string(GlobalServerData::serverPrivateVariables.marketItemList.at(index).quantity),
                        std::to_string(price),
                        std::to_string(objectId),
                        std::to_string(character_id)
                        });
            return;
        }
        index++;
    }
    if(marketObjectUniqueIdList.size()==0)
    {
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
        posOutput+=1;
        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

        normalOutput("No more id into marketObjectIdList");
        return;
    }
    //append to the market
    removeObject(objectId,quantity);
    GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_insert_item_market.asyncWrite({
                std::to_string(objectId),
                std::to_string(character_id),
                std::to_string(quantity),
                std::to_string(price)
                });
    MarketItem marketItem;
    marketItem.price=price;
    marketItem.item=objectId;
    marketItem.marketObjectUniqueId=marketObjectUniqueIdList.front();
    marketItem.player=character_id;
    marketItem.quantity=quantity;
    marketObjectUniqueIdList.erase(marketObjectUniqueIdList.begin());
    GlobalServerData::serverPrivateVariables.marketItemList.push_back(marketItem);

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
    posOutput+=1;
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::putMarketMonster(const uint8_t &query_id, const uint8_t &monsterPosition, const uint64_t &price)
{
    if(getInTrade() || isInFight())
    {
        errorOutput("You can't use the market in trade/fight");
        return;
    }
    //send the network reply
    removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;
    if(monsterPosition>=public_and_private_informations.playerMonster.size())
    {
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
        posOutput+=1;
        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }

    const uint8_t index=monsterPosition;
    const PlayerMonster &playerMonster=public_and_private_informations.playerMonster.at(index);
    if(!remainMonstersToFightWithoutThisMonster(index))
    {
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
        posOutput+=1;
        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

        normalOutput("You can't put in market this msonter because you will be without monster to fight");
        return;
    }
    MarketPlayerMonster marketPlayerMonster;
    marketPlayerMonster.price=price;
    marketPlayerMonster.monster=playerMonster;
    marketPlayerMonster.player=character_id;
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_place.asyncWrite({
                           "3",
                           std::to_string(marketPlayerMonster.monster.id)
                       });
    public_and_private_informations.playerMonster.erase(public_and_private_informations.playerMonster.begin()+index);
    GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.push_back(marketPlayerMonster);
    //save the player drop monster
    //updateMonsterInDatabase();

    GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_insert_monster_market_price.asyncWrite({
            std::to_string(marketPlayerMonster.monster.id),
            std::to_string(price)
            });

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
    posOutput+=1;
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::withdrawMarketCash(const uint8_t &query_id)
{
    if(getInTrade() || isInFight())
    {
        errorOutput("You can't use the market in trade/fight");
        return;
    }
    //send the network reply
    removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(8);//set the dynamic size
    const uint64_t converted_market_cash=htole64(market_cash);
    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&converted_market_cash,8);
    posOutput+=8;

    if(market_cash>0)
    {
        addCash(market_cash);
        market_cash=0;
        GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_get_market_cash.asyncWrite({
                    std::to_string(character_id)
                    });
    }

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::withdrawMarketObject(const uint8_t &query_id,const uint16_t &objectId,const uint32_t &quantity)
{
    if(getInTrade() || isInFight())
    {
        errorOutput("You can't use the market in trade/fight");
        return;
    }
    if(quantity<=0)
    {
        errorOutput("You can't use the market with null quantity");
        return;
    }
    //send the network reply
    removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;

    unsigned int index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketItemList.size())
    {
        const MarketItem &marketItem=GlobalServerData::serverPrivateVariables.marketItemList.at(index);
        if(marketItem.item==objectId)
        {
            if(marketItem.player!=character_id)
            {
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
                posOutput+=1;
                sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                return;
            }
            if(marketItem.quantity<quantity)
            {
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
                posOutput+=1;
                sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                return;
            }
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
            posOutput+=1;
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(marketItem.item);
            posOutput+=4;
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(marketItem.quantity);
            posOutput+=4;

            GlobalServerData::serverPrivateVariables.marketItemList[index].quantity=marketItem.quantity-quantity;
            if(GlobalServerData::serverPrivateVariables.marketItemList.at(index).quantity==0)
            {
                marketObjectUniqueIdList.push_back(marketItem.marketObjectUniqueId);
                GlobalServerData::serverPrivateVariables.marketItemList.erase(GlobalServerData::serverPrivateVariables.marketItemList.begin()+index);
                GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_delete_item_market.asyncWrite({
                                std::to_string(objectId),
                                std::to_string(character_id)
                            });
            }
            else
                GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_update_item_market.asyncWrite({
                                std::to_string(GlobalServerData::serverPrivateVariables.marketItemList.at(index).quantity),
                                std::to_string(objectId),
                                std::to_string(character_id)
                            });
            addObject(objectId,quantity);

            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);//set the dynamic size
            sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

            return;
        }
        index++;
    }
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
    posOutput+=1;
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

Player_private_and_public_informations &Client::get_public_and_private_informations()
{
    return public_and_private_informations;
}

const Player_private_and_public_informations &Client::get_public_and_private_informations_ro() const
{
    return public_and_private_informations;
}

void Client::withdrawMarketMonster(const uint8_t &query_id,const uint32_t &monsterId)
{
    if(getInTrade() || isInFight())
    {
        errorOutput("You can't use the market in trade/fight");
        return;
    }
    //send the network reply
    removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;

    unsigned int index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.size())
    {
        const MarketPlayerMonster &marketPlayerMonster=GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.at(index);
        if(marketPlayerMonster.monster.id==monsterId)
        {
            if(marketPlayerMonster.player!=character_id)
            {
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
                posOutput+=1;
                sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                return;
            }
            if(public_and_private_informations.playerMonster.size()>=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters)
            {
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
                posOutput+=1;
                sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                return;
            }

            GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_delete_monster_market_price.asyncWrite({std::to_string(marketPlayerMonster.monster.id)});
            GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.erase(GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.begin()+index);
            addPlayerMonster(marketPlayerMonster.monster);
            GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_place.asyncWrite({"1",std::to_string(marketPlayerMonster.monster.id)});

            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
            posOutput+=1;

            posOutput+=FacilityLib::privateMonsterToBinary(ProtocolParsingBase::tempBigBufferForOutput+posOutput,public_and_private_informations.playerMonster.back(),character_id);

            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);//set the dynamic size
            sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            return;
        }
        index++;
    }
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
    posOutput+=1;
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}
