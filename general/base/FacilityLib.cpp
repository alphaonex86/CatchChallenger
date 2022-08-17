#include "FacilityLib.hpp"
#include "CommonDatapack.hpp"
#include "CommonSettingsServer.hpp"

#ifdef CATCHCHALLENGER_EXTRA_CHECK
#include <iostream>
#endif

#include <ctime>
#include <time.h>

using namespace CatchChallenger;

std::vector<char> FacilityLib::UTF8EmptyData={0x00};
std::string FacilityLib::text_slash="/";
std::string FacilityLib::text_male="male";
std::string FacilityLib::text_female="female";
std::string FacilityLib::text_unknown="unknown";
std::string FacilityLib::text_clan="clan";
std::string FacilityLib::text_dotcomma=";";

PublicPlayerMonster FacilityLib::playerMonsterToPublicPlayerMonster(const PlayerMonster &playerMonster)
{
    PublicPlayerMonster returnVar;
    returnVar.buffs=playerMonster.buffs;
    returnVar.catched_with=playerMonster.catched_with;
    returnVar.gender=playerMonster.gender;
    returnVar.hp=playerMonster.hp;
    returnVar.level=playerMonster.level;
    returnVar.monster=playerMonster.monster;
    return returnVar;
}

uint16_t FacilityLib::publicPlayerMonsterToBinary(char *data,const PublicPlayerMonster &publicPlayerMonster)
{
    uint16_t posOutput=0;
    *reinterpret_cast<uint16_t *>(data+posOutput)=htole16(publicPlayerMonster.monster);
    posOutput+=2;
    data[posOutput]=publicPlayerMonster.level;
    posOutput+=1;
    *reinterpret_cast<uint32_t *>(data+posOutput)=htole32(publicPlayerMonster.hp);
    posOutput+=4;
    *reinterpret_cast<uint16_t *>(data+posOutput)=htole16(publicPlayerMonster.catched_with);
    posOutput+=2;
    data[posOutput]=(uint8_t)publicPlayerMonster.gender;
    posOutput+=1;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(publicPlayerMonster.buffs.size()>255)
    {
        std::cerr << "FacilityLib::publicPlayerMonsterToBinary() buffs.size()>255" << std::endl;
        abort();
    }
    #endif
    data[posOutput]=static_cast<uint8_t>(publicPlayerMonster.buffs.size());
    posOutput+=1;
    uint8_t index=0;
    while(index<publicPlayerMonster.buffs.size())
    {
        data[posOutput]=publicPlayerMonster.buffs.at(index).buff;
        posOutput+=1;
        data[posOutput]=publicPlayerMonster.buffs.at(index).level;
        posOutput+=1;
        index++;
    }
    return posOutput;
}

uint16_t FacilityLib::playerMonsterToBinary(char *data,const PlayerMonster &playerMonster)
{
    uint16_t posOutput=0;
    *reinterpret_cast<uint16_t *>(data+posOutput)=htole16(playerMonster.monster);
    posOutput+=2;
    data[posOutput]=playerMonster.level;
    posOutput+=1;
    *reinterpret_cast<uint32_t *>(data+posOutput)=htole32(playerMonster.hp);
    posOutput+=4;
    *reinterpret_cast<uint16_t *>(data+posOutput)=htole16(playerMonster.catched_with);
    posOutput+=2;
    data[posOutput]=(uint8_t)playerMonster.gender;
    posOutput+=1;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(playerMonster.buffs.size()>255)
    {
        std::cerr << "FacilityLib::publicPlayerMonsterToBinary() buffs.size()>255" << std::endl;
        abort();
    }
    #endif
    data[posOutput]=static_cast<uint8_t>(playerMonster.buffs.size());
    posOutput+=1;
    uint8_t index=0;
    while(index<playerMonster.buffs.size())
    {
        data[posOutput]=playerMonster.buffs.at(index).buff;
        posOutput+=1;
        data[posOutput]=playerMonster.buffs.at(index).level;
        posOutput+=1;
        index++;
    }
    return posOutput;
}

PlayerMonster FacilityLib::botFightMonsterToPlayerMonster(const BotFight::BotFightMonster &botFightMonster,const Monster::Stat &stat)
{
    PlayerMonster tempPlayerMonster;
    #ifndef CATCHCHALLENGER_VERSION_SINGLESERVER
    tempPlayerMonster.id=0;
    #endif
    tempPlayerMonster.catched_with=0;
    tempPlayerMonster.egg_step=0;
    tempPlayerMonster.gender=Gender_Unknown;
    tempPlayerMonster.hp=stat.hp;
    tempPlayerMonster.level=botFightMonster.level;
    tempPlayerMonster.monster=botFightMonster.id;
    tempPlayerMonster.remaining_xp=0;
    tempPlayerMonster.skills=botFightMonster.attacks;
    tempPlayerMonster.sp=0;
    return tempPlayerMonster;
}

std::string FacilityLib::genderToString(const Gender &gender)
{
    switch(gender)
    {
        case Gender_Male:
            return FacilityLib::text_male;
        case Gender_Female:
            return FacilityLib::text_female;
        default:
            break;
    }
    return FacilityLib::text_unknown;
}

uint64_t FacilityLib::nextCaptureTime(const City &city)
{
    time_t t=time(0);   // get time now
    struct std::tm * nextCityCapture=localtime(&t);
    nextCityCapture->tm_sec=0;
    nextCityCapture->tm_min=city.capture.minute;
    nextCityCapture->tm_hour=city.capture.hour;
    if(city.capture.frenquency==City::Capture::Frequency_week)
    {
        while(nextCityCapture->tm_wday!=(int)city.capture.day)
        {
            t+=3600*24;
            nextCityCapture=localtime(&t);
        }
    }
    else
    {
        if(nextCityCapture->tm_mon>=12)
        {
            nextCityCapture->tm_mon=0;
            nextCityCapture->tm_year++;
        }
        else
            nextCityCapture->tm_mon++;
    }
    return std::mktime(nextCityCapture);
}

uint32_t FacilityLib::privateMonsterToBinary(char *data,const PlayerMonster &monster,const uint32_t &character_id)
{
    //send the network reply
    uint32_t posOutput=0;

    *reinterpret_cast<uint16_t *>(data+posOutput)=htole16(monster.monster);
    posOutput+=2;
    data[posOutput]=monster.level;
    posOutput+=1;
    *reinterpret_cast<uint32_t *>(data+posOutput)=htole32(monster.remaining_xp);
    posOutput+=4;
    *reinterpret_cast<uint32_t *>(data+posOutput)=htole32(monster.hp);
    posOutput+=4;
    *reinterpret_cast<uint32_t *>(data+posOutput)=htole32(monster.sp);
    posOutput+=4;
    *reinterpret_cast<uint16_t *>(data+posOutput)=htole16(monster.catched_with);
    posOutput+=2;
    data[posOutput]=(uint8_t)monster.gender;
    posOutput+=1;
    *reinterpret_cast<uint32_t *>(data+posOutput)=htole32(monster.egg_step);
    posOutput+=4;

    data[posOutput]=(uint8_t)monster.character_origin==character_id;
    posOutput+=1;

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(monster.buffs.size()>255)
    {
        std::cerr << "FacilityLib::publicPlayerMonsterToBinary() buffs.size()>255" << std::endl;
        abort();
    }
    #endif
    uint16_t sub_index=0;
    data[posOutput]=static_cast<uint8_t>(monster.buffs.size());
    posOutput+=1;
    while(sub_index<monster.buffs.size())
    {
        const PlayerBuff &playerBuff=monster.buffs.at(sub_index);
        data[posOutput]=(uint8_t)playerBuff.buff;
        posOutput+=1;
        data[posOutput]=(uint8_t)playerBuff.level;
        posOutput+=1;
        data[posOutput]=(uint8_t)playerBuff.remainingNumberOfTurn;
        posOutput+=1;

        sub_index++;
    }
    sub_index=0;
    *reinterpret_cast<uint16_t *>(data+posOutput)=htole16(monster.skills.size());
    posOutput+=2;
    while(sub_index<monster.skills.size())
    {
        const PlayerMonster::PlayerSkill &playerSkill=monster.skills.at(sub_index);

        *reinterpret_cast<uint16_t *>(data+posOutput)=htole16(playerSkill.skill);
        posOutput+=2;
        data[posOutput]=playerSkill.level;
        posOutput+=1;
        data[posOutput]=playerSkill.endurance;
        posOutput+=1;

        sub_index++;
    }
    return posOutput;
}

//apply on after FacilityLib::industryStatusWithCurrentTime()
IndustryStatus FacilityLib::factoryCheckProductionStart(const IndustryStatus &industryStatus,const Industry &industry)
{
    IndustryStatus industryStatusCopy=industryStatus;
    if(factoryProductionStarted(industryStatus,industry))
        industryStatusCopy.last_update=time(0);
    return industryStatusCopy;
}

//apply on after FacilityLib::industryStatusWithCurrentTime()
/// \warning the resource can be not found due to the client
bool FacilityLib::factoryProductionStarted(const IndustryStatus &industryStatus,const Industry &industry)
{
    unsigned int index;
    index=0;
    while(index<industry.resources.size())
    {
        const Industry::Resource &resource=industry.resources.at(index);
        uint32_t quantityInStock=0;
        if(industryStatus.resources.find(resource.item)!=industryStatus.resources.cend())
            quantityInStock=industryStatus.resources.at(resource.item);
        const uint32_t tempCycleCount=quantityInStock/resource.quantity;
        if(tempCycleCount<=0)
            return false;
        index++;
    }
    index=0;
    while(index<industry.products.size())
    {
        const Industry::Product &product=industry.products.at(index);
        uint32_t quantityInStock=0;
        if(industryStatus.products.find(product.item)!=industryStatus.products.cend())
            quantityInStock=industryStatus.products.at(product.item);
        const uint32_t tempCycleCount=(product.quantity*industry.cycletobefull-quantityInStock)/product.quantity;
        if(tempCycleCount<=0)
            return false;
        index++;
    }
    return true;
}

IndustryStatus FacilityLib::industryStatusWithCurrentTime(const IndustryStatus &industryStatus,const Industry &industry)
{
    IndustryStatus industryStatusCopy=industryStatus;
    //do the generated item
    uint64_t ableToProduceCycleCount=0;
    if(industryStatus.last_update<static_cast<uint64_t>(time(0)))
    {
        ableToProduceCycleCount=(time(0)-industryStatus.last_update)/industry.time;
        if(ableToProduceCycleCount>industry.cycletobefull)
            ableToProduceCycleCount=industry.cycletobefull;
    }
    if(ableToProduceCycleCount<=0)
        return industryStatusCopy;
    unsigned int index;
    index=0;
    while(index<industry.resources.size())
    {
        const Industry::Resource &resource=industry.resources.at(index);
        const uint32_t &quantityInStock=industryStatusCopy.resources.at(resource.item);
        const uint32_t tempCycleCount=quantityInStock/resource.quantity;
        if(tempCycleCount<=0)
            return industryStatusCopy;
        else if(tempCycleCount<ableToProduceCycleCount)
            ableToProduceCycleCount=tempCycleCount;
        index++;
    }
    index=0;
    while(index<industry.products.size())
    {
        const Industry::Product &product=industry.products.at(index);
        const uint32_t &quantityInStock=industryStatusCopy.products.at(product.item);
        const uint32_t tempCycleCount=(product.quantity*industry.cycletobefull-quantityInStock)/product.quantity;
        if(tempCycleCount<=0)
            return industryStatusCopy;
        else if(tempCycleCount<ableToProduceCycleCount)
            ableToProduceCycleCount=tempCycleCount;
        index++;
    }
    industryStatusCopy.last_update+=industry.time*ableToProduceCycleCount;
    index=0;
    while(index<industry.resources.size())
    {
        industryStatusCopy.resources[industry.resources.at(index).item]-=industry.resources.at(index).quantity*ableToProduceCycleCount;
        index++;
    }
    index=0;
    while(index<industry.products.size())
    {
        industryStatusCopy.products[industry.products.at(index).item]+=industry.products.at(index).quantity*ableToProduceCycleCount;
        index++;
    }
    return industryStatusCopy;
}

uint32_t FacilityLib::getFactoryResourcePrice(const uint32_t &quantityInStock,const Industry::Resource &resource,const Industry &industry)
{
    uint32_t max_items=resource.quantity*industry.cycletobefull;
    uint8_t price_temp_change;
    if(quantityInStock>=max_items)
        price_temp_change=0;
    else
        price_temp_change=static_cast<uint8_t>(((max_items-quantityInStock)*CommonSettingsServer::commonSettingsServer.factoryPriceChange*2)/max_items);
    return CommonDatapack::commonDatapack.get_items().item.at(resource.item).price*(100-CommonSettingsServer::commonSettingsServer.factoryPriceChange+price_temp_change)/100;
}

uint32_t FacilityLib::getFactoryProductPrice(const uint32_t &quantityInStock, const Industry::Product &product, const Industry &industry)
{
    uint32_t max_items=product.quantity*industry.cycletobefull;
    uint8_t price_temp_change;
    if(quantityInStock>=max_items)
        price_temp_change=0;
    else
        price_temp_change=static_cast<uint8_t>(((max_items-quantityInStock)*CommonSettingsServer::commonSettingsServer.factoryPriceChange*2)/max_items);
    return CommonDatapack::commonDatapack.get_items().item.at(product.item).price*(100-CommonSettingsServer::commonSettingsServer.factoryPriceChange+price_temp_change)/100;
}

//reputation
void FacilityLib::appendReputationPoint(PlayerReputation *playerReputation,const int32_t &point,const Reputation &reputation)
{
    if(point==0)
        return;
    bool needContinue;
    //do input fix
    if(playerReputation->level>0 && playerReputation->point<0)
        playerReputation->point=0;
    if(playerReputation->level<0 && playerReputation->point>0)
        playerReputation->point=0;
    playerReputation->point+=point;
    do
    {
        needContinue=false;
        //at the limit
        if(reputation.reputation_negative.empty())
        {
            if(playerReputation->point<0)
            {
                playerReputation->point=0;
                playerReputation->level=0;
                break;
            }
        }
        else
        {
            if(playerReputation->level<=(-((int32_t)reputation.reputation_negative.size()-1)) && playerReputation->point<=0)
            {
                playerReputation->point=0;
                playerReputation->level=static_cast<uint8_t>((-((int32_t)reputation.reputation_negative.size()-1)));
                break;
            }
        }
        if(reputation.reputation_positive.empty())
        {
            if(playerReputation->point>0)
            {
                playerReputation->point=0;
                playerReputation->level=0;
                break;
            }
        }
        else
        {
            if(playerReputation->level>=((int32_t)reputation.reputation_positive.size()-1) && playerReputation->point>=0)
            {
                playerReputation->point=0;
                playerReputation->level=static_cast<uint8_t>(((int32_t)reputation.reputation_positive.size()-1));
                break;
            }
        }
        //lost point in level
        if(playerReputation->level<0 && playerReputation->point>0)
        {
            playerReputation->level++;
            playerReputation->point+=reputation.reputation_negative.at(-playerReputation->level+1);
            needContinue=true;
        }
        if(playerReputation->level>0 && playerReputation->point<0)
        {
            playerReputation->level--;
            playerReputation->point+=reputation.reputation_positive.at(playerReputation->level+1);
            needContinue=true;
        }
        //gain point in level
        if(playerReputation->level<=0 && playerReputation->point<0 && !reputation.reputation_negative.empty())
        {
            if(playerReputation->point<=reputation.reputation_negative.at(-playerReputation->level+1))
            {
                playerReputation->point-=reputation.reputation_negative.at(-playerReputation->level+1);
                playerReputation->level--;
                needContinue=true;
            }
        }
        if(playerReputation->level>=0 && playerReputation->point>0 && !reputation.reputation_positive.empty())
        {
            if(playerReputation->point>=reputation.reputation_positive.at(playerReputation->level+1))
            {
                playerReputation->point-=reputation.reputation_positive.at(playerReputation->level+1);
                playerReputation->level++;
                needContinue=true;
            }
        }
    } while(needContinue);
}
