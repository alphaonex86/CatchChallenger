#include "Client.hpp"
#include "MapServer.hpp"
#include "GlobalServerData.hpp"
#include "PreparedDBQuery.hpp"
#include "../../general/base/FacilityLib.hpp"

using namespace CatchChallenger;

void Client::getShopList(const uint8_t &query_id,const SHOP_TYPE &shopId)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("getShopList("+std::to_string(query_id)+","+std::to_string(shopId)+")");
    #endif
    if(CommonDatapackServerSpec::commonDatapackServerSpec.get_shops().find(shopId)==CommonDatapackServerSpec::commonDatapackServerSpec.get_shops().cend())
    {
        errorOutput("shopId not found: "+std::to_string(shopId));
        return;
    }
    CommonMap *map=this->map;
    uint8_t x=this->x;
    uint8_t y=this->y;
    //resolv the object
    Direction direction=getLastDirection();
    switch(direction)
    {
        case Direction_look_at_top:
        case Direction_look_at_right:
        case Direction_look_at_bottom:
        case Direction_look_at_left:
            direction=lookToMove(direction);
            if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                {
                    errorOutput("getShopList() Can't move at this direction from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
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
        errorOutput("Wrong direction to use a shop");
        return;
    }
    const MapServer * const mapServer=static_cast<MapServer*>(map);
    const std::pair<uint8_t,uint8_t> pos(x,y);
    //check if is shop
    if(mapServer->shops.find(pos)==mapServer->shops.cend())
    {
        switch(direction)
        {
            /// \warning: Not loop but move here due to first transform set: direction=lookToMove(direction);
            case Direction_look_at_top:
            case Direction_look_at_right:
            case Direction_look_at_bottom:
            case Direction_look_at_left:
            direction=lookToMove(direction);
            break;
            default:
            break;
        }
        switch(direction)
        {
            /// \warning: Not loop but move here due to first transform set: direction=lookToMove(direction);
            case Direction_look_at_top:
            case Direction_look_at_right:
            case Direction_look_at_bottom:
            case Direction_look_at_left:
            case Direction_move_at_top:
            case Direction_move_at_right:
            case Direction_move_at_bottom:
            case Direction_move_at_left:
                if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
                {
                    if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                    {
                        errorOutput("getShopList() Can't move at this direction from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
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
            break;
        }
        {
            const MapServer * const mapServer=static_cast<MapServer*>(map);
            const std::pair<uint8_t,uint8_t> pos(x,y);
            if(mapServer->shops.find(pos)==mapServer->shops.cend())
            {
                errorOutput("not shop into this direction");
                return;
            }
            else
            {
                const std::vector<uint16_t> shops=mapServer->shops.at(pos);
                if(!vectorcontainsAtLeastOne(shops,shopId))
                {
                    errorOutput("not shop into this direction");
                    return;
                }
            }
        }
    }
    else
    {
        const std::vector<uint16_t> shops=mapServer->shops.at(pos);
        if(!vectorcontainsAtLeastOne(shops,shopId))
        {
            errorOutput("not shop into this direction");
            return;
        }
    }
    //send the shop items (no taxes from now)
    const Shop &shop=CommonDatapackServerSpec::commonDatapackServerSpec.get_shops().at(shopId);

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

        unsigned int index=0;
        unsigned int objectCount=0;
        while(index<shop.items.size())
        {
            if(shop.prices.at(index)>0)
            {
                if(shop.prices.at(index)>0)
                {
                    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(shop.items.at(index));
                    posOutput+=2;
                    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(shop.prices.at(index));
                    posOutput+=4;
                    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(0);
                    posOutput+=4;
                    objectCount++;
                }
            }
            index++;
        }
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1+4)=htole16(objectCount);

        //set the dynamic size
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
}

void Client::buyObject(const uint8_t &query_id,const SHOP_TYPE &shopId,const uint16_t &objectId,const uint32_t &quantity,const uint32_t &price)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("buyObject("+std::to_string(query_id)+","+std::to_string(shopId)+")");
    #endif
    if(CommonDatapackServerSpec::commonDatapackServerSpec.get_shops().find(shopId)==CommonDatapackServerSpec::commonDatapackServerSpec.get_shops().cend())
    {
        errorOutput("shopId not found: "+std::to_string(shopId));
        return;
    }
    if(quantity<=0)
    {
        errorOutput("quantity wrong: "+std::to_string(quantity));
        return;
    }
    CommonMap *map=this->map;
    uint8_t x=this->x;
    uint8_t y=this->y;
    //resolv the object
    Direction direction=getLastDirection();
    switch(direction)
    {
        case Direction_look_at_top:
        case Direction_look_at_right:
        case Direction_look_at_bottom:
        case Direction_look_at_left:
            direction=lookToMove(direction);
            if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                {
                    errorOutput("buyObject() Can't move at this direction from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
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
        errorOutput("Wrong direction to use a shop");
        return;
    }
    const MapServer * const mapServer=static_cast<MapServer*>(map);
    const std::pair<uint8_t,uint8_t> pos(x,y);
    //check if is shop
    if(mapServer->shops.find(pos)==mapServer->shops.cend())
    {
        //not shop
        Direction direction=getLastDirection();
        switch(direction)
        {
            /// \warning: Not loop but move here due to first transform set: direction=lookToMove(direction);
            case Direction_look_at_top:
            case Direction_look_at_right:
            case Direction_look_at_bottom:
            case Direction_look_at_left:
            direction=lookToMove(direction);
            break;
            default:
            return;
        }
        switch(direction)
        {
            /// \warning: Not loop but move here due to first transform set: direction=lookToMove(direction);
            case Direction_look_at_top:
            case Direction_look_at_right:
            case Direction_look_at_bottom:
            case Direction_look_at_left:
            case Direction_move_at_top:
            case Direction_move_at_right:
            case Direction_move_at_bottom:
            case Direction_move_at_left:
                if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
                {
                    if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                    {
                        errorOutput("buyObject() Can't move at this direction from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
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
            errorOutput("Wrong direction to use a shop");
            return;
        }
        const MapServer * const mapServer=static_cast<MapServer*>(map);
        const std::pair<uint8_t,uint8_t> pos(x,y);
        if(mapServer->shops.find(pos)==mapServer->shops.cend())
        {
            errorOutput("not shop into this direction");
            return;
        }
        else
        {
            const std::vector<uint16_t> shops=mapServer->shops.at(pos);
            if(!vectorcontainsAtLeastOne(shops,shopId))
            {
                errorOutput("not shop into this direction");
                return;
            }
        }
    }
    else
    {
        const std::vector<uint16_t> shops=mapServer->shops.at(pos);
        if(!vectorcontainsAtLeastOne(shops,shopId))
        {
            errorOutput("not shop into this direction");
            return;
        }
    }
    //send the shop items (no taxes from now)
    const Shop &shop=CommonDatapackServerSpec::commonDatapackServerSpec.get_shops().at(shopId);
    const int &priceIndex=vectorindexOf(shop.items,objectId);

    removeFromQueryReceived(query_id);
    //send the network reply

    uint32_t posOutput=0;

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;

    if(priceIndex==-1)
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)BuyStat_HaveNotQuantity;
        posOutput+=1;

        //set the dynamic size
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        return;
    }
    const uint32_t &realprice=CommonDatapackServerSpec::commonDatapackServerSpec.get_shops().at(shopId).prices.at(priceIndex);
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

void Client::sellObject(const uint8_t &query_id,const SHOP_TYPE &shopId,const CATCHCHALLENGER_TYPE_ITEM &objectId,const uint32_t &quantity,const uint32_t &price)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("sellObject("+std::to_string(query_id)+","+std::to_string(shopId)+")");
    #endif
    if(CommonDatapackServerSpec::commonDatapackServerSpec.get_shops().find(shopId)==CommonDatapackServerSpec::commonDatapackServerSpec.get_shops().cend())
    {
        errorOutput("shopId not found: "+std::to_string(shopId));
        return;
    }
    if(quantity<=0)
    {
        errorOutput("quantity wrong: "+std::to_string(quantity));
        return;
    }
    CommonMap *map=this->map;
    uint8_t x=this->x;
    uint8_t y=this->y;
    //resolv the object
    Direction direction=getLastDirection();
    switch(direction)
    {
        case Direction_look_at_top:
        case Direction_look_at_right:
        case Direction_look_at_bottom:
        case Direction_look_at_left:
            direction=lookToMove(direction);
            if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                {
                    errorOutput("sellObject() Can't move at this direction from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
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
        errorOutput("Wrong direction to use a shop");
        return;
    }
    const MapServer * const mapServer=static_cast<MapServer*>(map);
    const std::pair<uint8_t,uint8_t> pos(x,y);
    //check if is shop
    if(mapServer->shops.find(pos)==mapServer->shops.cend())
    {
        Direction direction=getLastDirection();
        switch(direction)
        {
            /// \warning: Not loop but move here due to first transform set: direction=lookToMove(direction);
            case Direction_look_at_top:
            case Direction_look_at_right:
            case Direction_look_at_bottom:
            case Direction_look_at_left:
            direction=lookToMove(direction);
            break;
            default:
            return;
        }
        switch(direction)
        {
            /// \warning: Not loop but move here due to first transform set: direction=lookToMove(direction);
            case Direction_look_at_top:
            case Direction_look_at_right:
            case Direction_look_at_bottom:
            case Direction_look_at_left:
            case Direction_move_at_top:
            case Direction_move_at_right:
            case Direction_move_at_bottom:
            case Direction_move_at_left:
                if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
                {
                    if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                    {
                        errorOutput("sellObject() Can't move at this direction from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
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
            errorOutput("Wrong direction to use a shop");
            return;
        }
        const MapServer * const mapServer=static_cast<MapServer*>(map);
        const std::pair<uint8_t,uint8_t> pos(x,y);
        if(mapServer->shops.find(pos)==mapServer->shops.cend())
        {
            errorOutput("not shop into this direction");
            return;
        }
        else
        {
            const std::vector<uint16_t> shops=mapServer->shops.at(pos);
            if(!vectorcontainsAtLeastOne(shops,shopId))
            {
                errorOutput("not shop into this direction");
                return;
            }
        }
    }
    else
    {
        const std::vector<uint16_t> shops=mapServer->shops.at(pos);
        if(!vectorcontainsAtLeastOne(shops,shopId))
        {
            errorOutput("not shop into this direction");
            return;
        }
    }
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
