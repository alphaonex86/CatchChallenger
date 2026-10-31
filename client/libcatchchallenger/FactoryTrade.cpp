#include "FactoryTrade.hpp"
#include "../../general/base/FacilityLib.hpp"

#include <iostream>

using namespace CatchChallenger;

void FactoryTrade::reset()
{
    buys_.clear();
    sells_.clear();
}

FactoryTrade::Answer FactoryTrade::emptyAnswer()
{
    Answer answer;
    answer.result=Result_WrongReply;
    answer.done=false;
    answer.item=0;
    answer.itemsToAdd=0;
    answer.cashToAdd=0;
    answer.cashToRemove=0;
    return answer;
}

uint32_t FactoryTrade::buy(const CATCHCHALLENGER_TYPE_ITEM &item,const uint32_t &quantity,const uint32_t &unitPrice)
{
    ItemToSellOrBuy query;
    query.object=item;
    query.quantity=quantity;
    query.price=unitPrice;
    buys_.push_back(query);
    return unitPrice*quantity;
}

void FactoryTrade::sell(const CATCHCHALLENGER_TYPE_ITEM &item,const uint32_t &quantity,const uint32_t &unitPrice)
{
    ItemToSellOrBuy query;
    query.object=item;
    query.quantity=quantity;
    query.price=unitPrice;
    sells_.push_back(query);
}

FactoryTrade::Answer FactoryTrade::buyAnswer(const BuyStat &stat,const uint32_t &newUnitPrice,IndustryStatus &status,const Industry &industry,const uint64_t &nowSec)
{
    Answer answer=emptyAnswer();
    if(buys_.empty())
    {
        std::cerr << "FactoryTrade::buyAnswer(): reply without buy waiting" << std::endl;
        return answer;
    }
    const ItemToSellOrBuy query=buys_.front();
    buys_.erase(buys_.cbegin());
    answer.item=query.object;
    switch(stat)
    {
        case BuyStat_Done:
            answer.result=Result_Bought;
        break;
        case BuyStat_BetterPrice:
            if(newUnitPrice==0)
                std::cerr << "FactoryTrade::buyAnswer(): better price at 0$" << std::endl;
            else
            {
                answer.result=Result_BoughtBetterPrice;
                answer.cashToAdd=query.price*query.quantity;
                answer.cashToRemove=newUnitPrice*query.quantity;
            }
        break;
        case BuyStat_HaveNotQuantity:
            answer.result=Result_NotTheQuantity;
            answer.cashToAdd=query.price*query.quantity;
        break;
        case BuyStat_PriceHaveChanged:
            answer.result=Result_PriceWorse;
            answer.cashToAdd=query.price*query.quantity;
        break;
        default:
            std::cerr << "FactoryTrade::buyAnswer(): unknown stat " << std::to_string(stat) << std::endl;
        break;
    }
    if(answer.result==Result_Bought || answer.result==Result_BoughtBetterPrice)
    {
        answer.done=true;
        answer.itemsToAdd=query.quantity;
        const bool wasInProduction=FacilityLib::factoryProductionStarted(status,industry);
        if(status.products.find(query.object)!=status.products.cend())
        {
            if(status.products.at(query.object)>query.quantity)
                status.products[query.object]-=query.quantity;
            else
                status.products.erase(query.object);
        }
        if(!wasInProduction)
            status.last_update=nowSec;
    }
    return answer;
}

FactoryTrade::Answer FactoryTrade::sellAnswer(const SoldStat &stat,const uint32_t &newUnitPrice,IndustryStatus &status,const Industry &industry,const uint64_t &nowSec)
{
    Answer answer=emptyAnswer();
    if(sells_.empty())
    {
        std::cerr << "FactoryTrade::sellAnswer(): reply without sell waiting" << std::endl;
        return answer;
    }
    const ItemToSellOrBuy query=sells_.front();
    sells_.erase(sells_.cbegin());
    answer.item=query.object;
    switch(stat)
    {
        case SoldStat_Done:
            answer.result=Result_Sold;
            answer.cashToAdd=query.price*query.quantity;
        break;
        case SoldStat_BetterPrice:
            if(newUnitPrice==0)
                std::cerr << "FactoryTrade::sellAnswer(): better price at 0$" << std::endl;
            else
            {
                answer.result=Result_SoldBetterPrice;
                answer.cashToAdd=newUnitPrice*query.quantity;
            }
        break;
        case SoldStat_WrongQuantity:
            answer.result=Result_NotTheQuantity;
            answer.itemsToAdd=query.quantity;
        break;
        case SoldStat_PriceHaveChanged:
            answer.result=Result_PriceWorse;
            answer.itemsToAdd=query.quantity;
        break;
        default:
            std::cerr << "FactoryTrade::sellAnswer(): unknown stat " << std::to_string(stat) << std::endl;
        break;
    }
    if(answer.result==Result_Sold || answer.result==Result_SoldBetterPrice)
    {
        answer.done=true;
        const bool wasInProduction=FacilityLib::factoryProductionStarted(status,industry);
        status.resources[query.object]+=query.quantity;
        if(!wasInProduction)
            status.last_update=nowSec;
    }
    return answer;
}

IndustryStatus &FactoryTrade::industryStatus(Player_private_and_public_informations &informations,const CATCHCHALLENGER_TYPE_MAPID &mapId,const uint16_t &factoryId)
{
    std::vector<IndustryStatus> &industriesStatus=informations.mapData[mapId].industriesStatus;
    while(industriesStatus.size()<=factoryId)
    {
        IndustryStatus status;
        status.last_update=0;
        industriesStatus.push_back(status);
    }
    return industriesStatus[factoryId];
}

void FactoryTrade::statusFromServerList(IndustryStatus &status,const Industry &industry,const uint32_t &remainingProductionTime,const uint64_t &nowSec,
                                        const std::vector<ItemToSellOrBuy> &resources,const std::vector<ItemToSellOrBuy> &products)
{
    status.resources.clear();
    status.products.clear();
    status.last_update=nowSec+remainingProductionTime-industry.time;
    unsigned int index=0;
    while(index<industry.resources.size())
    {
        const Industry::Resource &resource=industry.resources.at(index);
        const uint32_t full=industry.cycletobefull*resource.quantity;
        uint32_t accepted=0;
        unsigned int listIndex=0;
        while(listIndex<resources.size() && resources.at(listIndex).object!=resource.item)
            listIndex++;
        if(listIndex<resources.size())
            accepted=resources.at(listIndex).quantity;
        if(accepted>full)
        {
            std::cerr << "FactoryTrade::statusFromServerList(): resource " << resource.item << " accepts " << accepted << " over the full stock " << full << std::endl;
            accepted=full;
        }
        status.resources[resource.item]=full-accepted;
        index++;
    }
    index=0;
    while(index<resources.size())
    {
        if(status.resources.find(resources.at(index).object)==status.resources.cend())
            std::cerr << "FactoryTrade::statusFromServerList(): resource " << resources.at(index).object << " is not in the industry" << std::endl;
        index++;
    }
    index=0;
    while(index<products.size())
    {
        status.products[products.at(index).object]=products.at(index).quantity;
        index++;
    }
}

uint32_t FactoryTrade::remainingProductionTime(const IndustryStatus &status,const Industry &industry,const uint64_t &nowSec)
{
    if(industry.time==0)
        return 0;
    const uint64_t cycleEnd=status.last_update+industry.time;
    if(cycleEnd>nowSec)
        return static_cast<uint32_t>(cycleEnd-nowSec);
    //the production loops, remaining time of the current cycle
    return static_cast<uint32_t>(industry.time-(nowSec-status.last_update)%industry.time);
}

bool FactoryTrade::productPrice(const Industry &industry,const CATCHCHALLENGER_TYPE_ITEM &item,const uint32_t &quantityLeft,uint32_t &price)
{
    unsigned int index=0;
    while(index<industry.products.size())
    {
        if(industry.products.at(index).item==item)
        {
            price=FacilityLib::getFactoryProductPrice(quantityLeft,industry.products.at(index),industry);
            return true;
        }
        index++;
    }
    return false;
}

bool FactoryTrade::resourcePrice(const Industry &industry,const CATCHCHALLENGER_TYPE_ITEM &item,const uint32_t &quantityAccepted,uint32_t &price)
{
    unsigned int index=0;
    while(index<industry.resources.size())
    {
        const Industry::Resource &resource=industry.resources.at(index);
        if(resource.item==item)
        {
            const uint32_t full=resource.quantity*industry.cycletobefull;
            uint32_t stock=0;
            if(quantityAccepted<full)
                stock=full-quantityAccepted;
            price=FacilityLib::getFactoryResourcePrice(stock,resource,industry);
            return true;
        }
        index++;
    }
    return false;
}
