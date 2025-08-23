#include "../Client.hpp"
#include "../MapManagement/Map_server_MapVisibility_Simple_StoreOnSender.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/CommonDatapack.hpp"

using namespace CatchChallenger;

void Client::getShopList(const uint8_t &query_id)
{
    if(mapIndex>=65535)
        return;
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("getShopList("+std::to_string(query_id)+","+std::to_string(shopId)+")");
    #endif
    COORD_TYPE new_x=0,new_y=0;
    const Map_server_MapVisibility_Simple_StoreOnSender * new_map=Client::mapAndPosIfMoveInLookingDirectionJumpColision(new_x,new_y);
    if(new_map==nullptr)
    {
        errorOutput("Can't move at this direction from "+std::to_string(mapIndex)+" ("+std::to_string(x)+","+std::to_string(y)+")");
        return;
    }
    Shop shop;
    //check if is shop
    if(new_map->shops.find(std::pair<uint8_t,uint8_t>(new_x,new_y))==new_map->shops.cend())
    {
        errorOutput("not shop into this direction");
        return;
    }
    else
        shop=new_map->shops.at(std::pair<uint8_t,uint8_t>(new_x,new_y));
    //send the shop items (no taxes from now)

    removeFromQueryReceived(query_id);
    //send the network reply

    {
        uint32_t posOutput=0;

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
        posOutput+=1+4;

        //size
        posOutput+=2;

        unsigned int objectCount=0;
        for (const auto& [key, value] : shop.items)
        {
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(key);
            posOutput+=2;
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(value);
            posOutput+=4;
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(0);
            posOutput+=4;
            objectCount++;
        }
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1+4)=htole16(objectCount);

        //set the dynamic size
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
}

void Client::buyObject(const uint8_t &query_id, const CATCHCHALLENGER_TYPE_ITEM &objectId, const CATCHCHALLENGER_TYPE_ITEM_QUANTITY &quantity, const uint32_t &price)
{
    if(mapIndex>=65535)
        return;
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("buyObject("+std::to_string(query_id)+","+std::to_string(shopId)+")");
    #endif
    if(quantity<=0)
    {
        errorOutput("quantity wrong: "+std::to_string(quantity));
        return;
    }
    COORD_TYPE new_x=0,new_y=0;
    const Map_server_MapVisibility_Simple_StoreOnSender * new_map=Client::mapAndPosIfMoveInLookingDirectionJumpColision(new_x,new_y);
    if(new_map==nullptr)
    {
        errorOutput("Can't move at this direction from "+std::to_string(mapIndex)+" ("+std::to_string(x)+","+std::to_string(y)+")");
        return;
    }
    Shop shop;
    //check if is shop
    if(new_map->shops.find(std::pair<uint8_t,uint8_t>(new_x,new_y))==new_map->shops.cend())
    {
        errorOutput("not shop into this direction");
        return;
    }
    else
        shop=new_map->shops.at(std::pair<uint8_t,uint8_t>(new_x,new_y));

    removeFromQueryReceived(query_id);
    //send the network reply

    uint32_t posOutput=0;

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;

    if(shop.items.find(objectId)==shop.items.cend())
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)BuyStat_HaveNotQuantity;
        posOutput+=1;

        //set the dynamic size
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        return;
    }
    const uint32_t &realprice=shop.items.at(objectId);
    if(realprice==0)
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)BuyStat_HaveNotQuantity;
        posOutput+=1;

        //set the dynamic size
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        return;
    }
    if(realprice>price)
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)BuyStat_PriceHaveChanged;
        posOutput+=1;

        //set the dynamic size
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        return;
    }
    if(realprice<price)
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)BuyStat_BetterPrice;
        posOutput+=1;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(price);
        posOutput+=4;
    }
    else
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)BuyStat_Done;
        posOutput+=1;
    }
    if(public_and_private_informations.cash>=(realprice*quantity))
        removeCash(realprice*quantity);
    else
    {
        errorOutput("The player have not the cash to buy "+std::to_string(quantity)+" item of id: "+std::to_string(objectId));
        return;
    }
    addObject(objectId,quantity);

    //set the dynamic size
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::sellObject(const uint8_t &query_id,const CATCHCHALLENGER_TYPE_ITEM &objectId,const CATCHCHALLENGER_TYPE_ITEM_QUANTITY &quantity,const uint32_t &price)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("sellObject("+std::to_string(query_id)+","+std::to_string(shopId)+")");
    #endif
    if(quantity<=0)
    {
        errorOutput("quantity wrong: "+std::to_string(quantity));
        return;
    }
    COORD_TYPE new_x=0,new_y=0;
    const Map_server_MapVisibility_Simple_StoreOnSender * new_map=Client::mapAndPosIfMoveInLookingDirectionJumpColision(new_x,new_y);
    if(new_map==nullptr)
    {
        errorOutput("Can't move at this direction from "+std::to_string(mapIndex)+" ("+std::to_string(x)+","+std::to_string(y)+")");
        return;
    }
    Shop shop;
    //check if is shop
    if(new_map->shops.find(std::pair<uint8_t,uint8_t>(new_x,new_y))==new_map->shops.cend())
    {
        errorOutput("not shop into this direction");
        return;
    }
    else
        shop=new_map->shops.at(std::pair<uint8_t,uint8_t>(new_x,new_y));
    removeFromQueryReceived(query_id);
    //send the network reply

    //send the shop items (no taxes from now)
    uint32_t posOutput=0;

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;
    if(CommonDatapack::commonDatapack.get_items().item.find(objectId)==CommonDatapack::commonDatapack.get_items().item.cend())
    {
        errorOutput("this item don't exists");
        return;
    }
    if(objectQuantity(objectId)<quantity)
    {
        errorOutput("you have not this quantity to sell");
        return;
    }
    if(CommonDatapack::commonDatapack.get_items().item.at(objectId).price==0)
    {
        errorOutput("Can't sold %1"+std::to_string(objectId));
        return;
    }

    /*uint32_t realPrice=0;
    const std::vector<uint16_t> shops=mapServer->shops.at(pos);
    if(!vectorcontainsAtLeastOne(shops,shopId))
        realPrice=shops.at(indexOf).at(objectId).price/2;
    else
        realPrice=CommonDatapack::commonDatapack.items.item.at(objectId).price/2;*/

    const uint32_t &realPrice=CommonDatapack::commonDatapack.get_items().item.at(objectId).price/2;

    if(realPrice<price)
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)SoldStat_PriceHaveChanged;
        posOutput+=1;

        //set the dynamic size
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        return;
    }
    if(realPrice>price)
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)SoldStat_BetterPrice;
        posOutput+=1;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(realPrice);
        posOutput+=4;
    }
    else
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)SoldStat_Done;
        posOutput+=1;
    }
    removeObject(objectId,quantity);
    addCash(realPrice*quantity);

    //set the dynamic size
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}
