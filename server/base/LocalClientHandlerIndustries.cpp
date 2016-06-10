#include "Client.h"
#include "MapServer.h"
#include "GlobalServerData.h"
#include "PreparedDBQuery.h"
#include "../../general/base/FacilityLib.h"

using namespace CatchChallenger;

void Client::saveIndustryStatus(const uint32_t &factoryId,const IndustryStatus &industryStatus,const Industry &industry)
{
    std::vector<std::string> resourcesStringList,productsStringList;
    unsigned int index;
    //send the resource
    index=0;
    while(index<industry.resources.size())
    {
        const Industry::Resource &resource=industry.resources.at(index);
        const uint32_t &quantityInStock=industryStatus.resources.at(resource.item);
        resourcesStringList.push_back(std::to_string(resource.item)+':'+std::to_string(quantityInStock));
        index++;
    }
    //send the product
    index=0;
    while(index<industry.products.size())
    {
        const Industry::Product &product=industry.products.at(index);
        const uint32_t &quantityInStock=industryStatus.products.at(product.item);
        productsStringList.push_back(std::to_string(product.item)+':'+std::to_string(quantityInStock));
        index++;
    }

    //save in db
    if(GlobalServerData::serverPrivateVariables.industriesStatus.find(factoryId)==GlobalServerData::serverPrivateVariables.industriesStatus.cend())
    {
        const std::string &queryText=PreparedDBQueryServer::db_query_insert_factory.compose(
                    std::to_string(factoryId),
                    stringimplode(resourcesStringList,';'),
                    stringimplode(productsStringList,';'),
                    std::to_string(industryStatus.last_update)
                    );
        dbQueryWriteServer(queryText);
    }
    else
    {
        const std::string &queryText=PreparedDBQueryServer::db_query_update_factory.compose(
                    stringimplode(resourcesStringList,';'),
                    stringimplode(productsStringList,';'),
                    std::to_string(industryStatus.last_update),
                    std::to_string(factoryId)
                    );
        dbQueryWriteServer(queryText);
    }
    GlobalServerData::serverPrivateVariables.industriesStatus[factoryId]=industryStatus;
}

void Client::getFactoryList(const uint8_t &query_id, const uint16_t &factoryId)
{
    if(isInFight())
    {
        errorOutput("Try do inventory action when is in fight");
        return;
    }
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(captureCityInProgress())
    {
        errorOutput("Try do inventory action when is in capture city");
        return;
    }
    #endif
    if(CommonDatapack::commonDatapack.industriesLink.find(factoryId)==CommonDatapack::commonDatapack.industriesLink.cend())
    {
        errorOutput("factory link id not found");
        return;
    }
    const IndustryLink &industryLink=CommonDatapack::commonDatapack.industriesLink.at(factoryId);
    if(CommonDatapack::commonDatapack.industries.find(industryLink.industry)==CommonDatapack::commonDatapack.industries.cend())
    {
        errorOutput("factory id not found");
        return;
    }
    const Industry &industry=CommonDatapack::commonDatapack.industries.at(industryLink.industry);
    //send the shop items (no taxes from now)
    removeFromQueryReceived(query_id);
    //send the network reply

    uint32_t posOutput=0;

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;

    if(GlobalServerData::serverPrivateVariables.industriesStatus.find(factoryId)==GlobalServerData::serverPrivateVariables.industriesStatus.cend())
    {
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=0;
        posOutput+=4;
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(industry.resources.size());
        posOutput+=2;
        unsigned int index=0;
        while(index<industry.resources.size())
        {
            const Industry::Resource &resource=industry.resources.at(index);
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(resource.item);
            posOutput+=2;
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(CommonDatapack::commonDatapack.items.item.at(resource.item).price*(100+CATCHCHALLENGER_SERVER_FACTORY_PRICE_CHANGE)/100);
            posOutput+=4;
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(resource.quantity*industry.cycletobefull);
            posOutput+=4;
            index++;
        }
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=0;
        posOutput+=2;
    }
    else
    {
        unsigned int index,count_item;
        const IndustryStatus &industryStatus=FacilityLib::industryStatusWithCurrentTime(GlobalServerData::serverPrivateVariables.industriesStatus.at(factoryId),industry);
        const auto currentTime=sFrom1970();
        if(industryStatus.last_update>currentTime)
        {
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=0;
            posOutput+=4;
        }
        else if((currentTime-industryStatus.last_update)>industry.time)
        {
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=0;
            posOutput+=4;
        }
        else
        {
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(industry.time-(currentTime-industryStatus.last_update));
            posOutput+=4;
        }
        //send the resource
        count_item=0;
        index=0;
        while(index<industry.resources.size())
        {
            const Industry::Resource &resource=industry.resources.at(index);
            const uint32_t &quantityInStock=industryStatus.resources.at(resource.item);
            if(quantityInStock<resource.quantity*industry.cycletobefull)
                count_item++;
            index++;
        }
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(count_item);
        posOutput+=2;
        index=0;
        while(index<industry.resources.size())
        {
            const Industry::Resource &resource=industry.resources.at(index);
            const uint32_t &quantityInStock=industryStatus.resources.at(resource.item);
            if(quantityInStock<resource.quantity*industry.cycletobefull)
            {
                *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(resource.item);
                posOutput+=2;
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(FacilityLib::getFactoryResourcePrice(quantityInStock,resource,industry));
                posOutput+=4;
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(resource.quantity*industry.cycletobefull-quantityInStock);
                posOutput+=4;
            }
            index++;
        }
        //send the product
        count_item=0;
        index=0;
        while(index<industry.products.size())
        {
            const Industry::Product &product=industry.products.at(index);
            const uint32_t &quantityInStock=industryStatus.products.at(product.item);
            if(quantityInStock>0)
                count_item++;
            index++;
        }
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(count_item);
        posOutput+=2;
        index=0;
        while(index<industry.products.size())
        {
            const Industry::Product &product=industry.products.at(index);
            const uint32_t &quantityInStock=industryStatus.products.at(product.item);
            if(quantityInStock>0)
            {
                *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(product.item);
                posOutput+=2;
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(FacilityLib::getFactoryProductPrice(quantityInStock,product,industry));
                posOutput+=4;
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(quantityInStock);
                posOutput+=4;
            }
            index++;
        }
    }

    //set the dynamic size
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::buyFactoryProduct(const uint8_t &query_id,const uint16_t &factoryId,const uint16_t &objectId,const uint32_t &quantity,const uint32_t &price)
{
    if(isInFight())
    {
        errorOutput("Try do inventory action when is in fight");
        return;
    }
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(captureCityInProgress())
    {
        errorOutput("Try do inventory action when is in capture city");
        return;
    }
    #endif
    if(CommonDatapack::commonDatapack.industriesLink.find(factoryId)==CommonDatapack::commonDatapack.industriesLink.cend())
    {
        errorOutput("factory id not found");
        return;
    }
    const IndustryLink &industryLink=CommonDatapack::commonDatapack.industriesLink.at(factoryId);
    if(CommonDatapack::commonDatapack.items.item.find(objectId)==CommonDatapack::commonDatapack.items.item.cend())
    {
        errorOutput("object id not found into the factory product list");
        return;
    }
    if(GlobalServerData::serverPrivateVariables.industriesStatus.find(factoryId)==GlobalServerData::serverPrivateVariables.industriesStatus.cend())
    {
        errorOutput("factory id not found in active list");
        return;
    }
    if(!haveReputationRequirements(industryLink.requirements.reputation))
    {
        errorOutput("The player have not the requirement: "+std::to_string(factoryId)+" to use the factory");
        return;
    }
    const Industry &industry=CommonDatapack::commonDatapack.industries.at(industryLink.industry);
    IndustryStatus industryStatus=FacilityLib::industryStatusWithCurrentTime(GlobalServerData::serverPrivateVariables.industriesStatus.at(factoryId),industry);
    uint32_t quantityInStock=0;
    uint32_t actualPrice=0;

    Industry::Product product;
    product.item=0;
    product.quantity=0;
    //get the right product
    {
        unsigned int index=0;
        while(index<industry.products.size())
        {
            product=industry.products.at(index);
            if(product.item==objectId)
            {
                quantityInStock=industryStatus.products.at(product.item);
                actualPrice=FacilityLib::getFactoryProductPrice(quantityInStock,product,industry);
                break;
            }
            index++;
        }
        if(index==industry.products.size())
        {
            errorOutput("internal bug, product for the factory not found");
            return;
        }
    }
    if(public_and_private_informations.cash<(actualPrice*quantity))
    {
        errorOutput("have not the cash to buy into this factory");
        return;
    }

    removeFromQueryReceived(query_id);
    //send the network reply

    uint32_t posOutput=0;

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;

    if(quantity>quantityInStock)
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x03;
        posOutput+=1;

        //set the dynamic size
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);
        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        return;
    }
    if(actualPrice>price)
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x04;
        posOutput+=1;

        //set the dynamic size
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);
        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        return;
    }
    if(actualPrice==price)
    {
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
        posOutput+=1;
    }
    else
    {
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1+4);
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
        posOutput+=1;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(actualPrice);
        posOutput+=4;
    }
    quantityInStock-=quantity;
    if(quantityInStock==(product.item*industry.cycletobefull))
    {
        industryStatus.products[product.item]=quantityInStock;
        industryStatus=FacilityLib::factoryCheckProductionStart(industryStatus,industry);
    }
    else
        industryStatus.products[product.item]=quantityInStock;
    removeCash(actualPrice*quantity);
    saveIndustryStatus(factoryId,industryStatus,industry);
    addObject(objectId,quantity);
    appendReputationRewards(CommonDatapack::commonDatapack.industriesLink.at(factoryId).rewards.reputation);

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::sellFactoryResource(const uint8_t &query_id,const uint16_t &factoryId,const uint16_t &objectId,const uint32_t &quantity,const uint32_t &price)
{
    if(isInFight())
    {
        errorOutput("Try do inventory action when is in fight");
        return;
    }
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(captureCityInProgress())
    {
        errorOutput("Try do inventory action when is in capture city");
        return;
    }
    #endif
    if(CommonDatapack::commonDatapack.industriesLink.find(factoryId)==CommonDatapack::commonDatapack.industriesLink.cend())
    {
        errorOutput("factory id not found");
        return;
    }
    if(CommonDatapack::commonDatapack.items.item.find(objectId)==CommonDatapack::commonDatapack.items.item.cend())
    {
        errorOutput("object id not found");
        return;
    }
    if(objectQuantity(objectId)<quantity)
    {
        errorOutput("you have not the object quantity to sell at this factory");
        return;
    }
    if(!haveReputationRequirements(CommonDatapack::commonDatapack.industriesLink.at(factoryId).requirements.reputation))
    {
        errorOutput("The player have not the requirement: "+std::to_string(factoryId)+" to use the factory");
        return;
    }
    const Industry &industry=CommonDatapack::commonDatapack.industries.at(CommonDatapack::commonDatapack.industriesLink.at(factoryId).industry);
    IndustryStatus industryStatus;
    if(GlobalServerData::serverPrivateVariables.industriesStatus.find(factoryId)==GlobalServerData::serverPrivateVariables.industriesStatus.cend())
    {
        industryStatus.last_update=sFrom1970();
        unsigned int index;
        //send the resource
        index=0;
        while(index<industry.resources.size())
        {
            const Industry::Resource &resource=industry.resources.at(index);
            industryStatus.resources[resource.item]=0;
            index++;
        }
        //send the product
        index=0;
        while(index<industry.products.size())
        {
            const Industry::Product &product=industry.products.at(index);
            industryStatus.products[product.item]=0;
            index++;
        }
    }
    else
        industryStatus=FacilityLib::industryStatusWithCurrentTime(GlobalServerData::serverPrivateVariables.industriesStatus.at(factoryId),industry);
    //send the network reply
    removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;

    uint32_t resourcePrice=0;
    //check if not overfull
    {
        unsigned int index=0;
        while(index<industry.resources.size())
        {
            const Industry::Resource &resource=industry.resources.at(index);
            if(resource.item==objectId)
            {
                if((resource.quantity*industry.cycletobefull-industryStatus.resources.at(resource.item))<quantity)
                {
                    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x03;
                    posOutput+=1;
                    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                    return;
                }
                resourcePrice=FacilityLib::getFactoryResourcePrice(industryStatus.resources.at(resource.item),resource,industry);
                if(price>resourcePrice)
                {
                    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x04;
                    posOutput+=1;
                    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                    return;
                }
                if((industryStatus.resources.at(resource.item)+quantity)==resource.quantity)
                {
                    industryStatus.resources[resource.item]+=quantity;
                    industryStatus=FacilityLib::factoryCheckProductionStart(industryStatus,industry);
                }
                else
                    industryStatus.resources[resource.item]+=quantity;
                break;
            }
            index++;
        }
        if(index==industry.resources.size())
        {
            errorOutput("internal bug, resource for the factory not found");
            return;
        }
    }
    if(price==resourcePrice)
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
        posOutput+=1;
        /**reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(quantity);
        posOutput+=4;*/
    }
    else
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
        posOutput+=1;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(resourcePrice);
        posOutput+=4;
    }
    removeObject(objectId,quantity);
    addCash(resourcePrice*quantity);
    saveIndustryStatus(factoryId,industryStatus,industry);
    appendReputationRewards(CommonDatapack::commonDatapack.industriesLink.at(factoryId).rewards.reputation);

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);//set the dynamic size
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}
