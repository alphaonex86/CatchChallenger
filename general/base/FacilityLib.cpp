#include "FacilityLib.h"
#include "CommonDatapack.h"
#include "CommonSettings.h"

#include <QFileInfo>
#include <QFileInfoList>
#include <QDir>

using namespace CatchChallenger;

QByteArray FacilityLib::UTF8EmptyData=QByteArray().fill(0x00,1);
QString FacilityLib::text_slash=QLatin1Literal("/");
QString FacilityLib::text_male=QLatin1Literal("male");
QString FacilityLib::text_female=QLatin1Literal("female");
QString FacilityLib::text_unknown=QLatin1Literal("unknown");
QString FacilityLib::text_clan=QLatin1Literal("clan");
QString FacilityLib::text_dotcomma=QLatin1Literal(";");

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

QByteArray FacilityLib::publicPlayerMonsterToBinary(const PublicPlayerMonster &publicPlayerMonster)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << publicPlayerMonster.monster;
    out << publicPlayerMonster.level;
    out << publicPlayerMonster.hp;
    out << publicPlayerMonster.catched_with;
    out << (quint8)publicPlayerMonster.gender;
    out << (quint32)publicPlayerMonster.buffs.size();
    int index=0;
    while(index<publicPlayerMonster.buffs.size())
    {
        out << publicPlayerMonster.buffs.at(index).buff;
        out << publicPlayerMonster.buffs.at(index).level;
        index++;
    }
    return outputData;
}

QByteArray playerMonsterToBinary(const PlayerMonster &playerMonster)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << playerMonster.monster;
    out << playerMonster.level;
    out << playerMonster.hp;
    out << playerMonster.catched_with;
    out << (quint8)playerMonster.gender;
    out << (quint32)playerMonster.buffs.size();
    int index=0;
    while(index<playerMonster.buffs.size())
    {
        out << playerMonster.buffs.at(index).buff;
        out << playerMonster.buffs.at(index).level;
        index++;
    }
    return outputData;
}

PlayerMonster FacilityLib::botFightMonsterToPlayerMonster(const BotFight::BotFightMonster &botFightMonster,const Monster::Stat &stat)
{
    PlayerMonster tempPlayerMonster;
    tempPlayerMonster.catched_with=0;
    tempPlayerMonster.egg_step=0;
    tempPlayerMonster.gender=Gender_Unknown;
    tempPlayerMonster.hp=stat.hp;
    tempPlayerMonster.id=0;
    tempPlayerMonster.level=botFightMonster.level;
    tempPlayerMonster.monster=botFightMonster.id;
    tempPlayerMonster.remaining_xp=0;
    tempPlayerMonster.skills=botFightMonster.attacks;
    tempPlayerMonster.sp=0;
    return tempPlayerMonster;
}

QString FacilityLib::genderToString(const Gender &gender)
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

QDateTime FacilityLib::nextCaptureTime(const City &city)
{
    QDateTime nextCityCapture=QDateTime::currentDateTime();
    nextCityCapture.setTime(QTime(city.capture.hour,city.capture.minute));
    if(city.capture.frenquency==City::Capture::Frequency_week)
    {
        while(nextCityCapture.date().dayOfWeek()!=(int)city.capture.day || nextCityCapture<=QDateTime::currentDateTime())
            nextCityCapture=nextCityCapture.addDays(1);
    }
    else
    {
        while(nextCityCapture<=QDateTime::currentDateTime())
        {
            QDate tempDate=nextCityCapture.date();
            if(tempDate.month()>=12)
                tempDate.setDate(tempDate.year()+1,1,1);
            else
                tempDate.setDate(tempDate.year(),tempDate.month()+1,1);
            nextCityCapture.setDate(tempDate);
        }
    }
    return nextCityCapture;
}

QByteArray FacilityLib::privateMonsterToBinary(const PlayerMonster &monster)
{
    //send the network reply
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);

    out << monster.id;
    out << monster.monster;
    out << monster.level;
    out << monster.remaining_xp;
    out << monster.hp;
    out << monster.sp;
    out << monster.catched_with;
    out << (quint8)monster.gender;
    out << monster.egg_step;
    out << monster.character_origin;
    int sub_index=0;
    int sub_size=monster.buffs.size();
    out << (quint8)sub_size;
    while(sub_index<sub_size)
    {
        out << monster.buffs.at(sub_index).buff;
        out << monster.buffs.at(sub_index).level;
        sub_index++;
    }
    sub_index=0;
    sub_size=monster.skills.size();
    out << (quint16)sub_size;
    while(sub_index<sub_size)
    {
        out << monster.skills.at(sub_index).skill;
        out << monster.skills.at(sub_index).level;
        out << monster.skills.at(sub_index).endurance;
        sub_index++;
    }
    return outputData;
}

//apply on after FacilityLib::industryStatusWithCurrentTime()
IndustryStatus FacilityLib::factoryCheckProductionStart(const IndustryStatus &industryStatus,const Industry &industry)
{
    IndustryStatus industryStatusCopy=industryStatus;
    if(factoryProductionStarted(industryStatus,industry))
        industryStatusCopy.last_update=QDateTime::currentMSecsSinceEpoch()/1000;
    return industryStatusCopy;
}

//apply on after FacilityLib::industryStatusWithCurrentTime()
/// \warning the resource can be not found due to the client
bool FacilityLib::factoryProductionStarted(const IndustryStatus &industryStatus,const Industry &industry)
{
    int index;
    index=0;
    while(index<industry.resources.size())
    {
        const Industry::Resource &resource=industry.resources.at(index);
        quint32 quantityInStock=0;
        if(industryStatus.resources.contains(resource.item))
            quantityInStock=industryStatus.resources.value(resource.item);
        const quint32 tempCycleCount=quantityInStock/resource.quantity;
        if(tempCycleCount<=0)
            return false;
        index++;
    }
    index=0;
    while(index<industry.products.size())
    {
        const Industry::Product &product=industry.products.at(index);
        quint32 quantityInStock=0;
        if(industryStatus.products.contains(product.item))
            quantityInStock=industryStatus.products.value(product.item);
        const quint32 tempCycleCount=(product.quantity*industry.cycletobefull-quantityInStock)/product.quantity;
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
    quint32 ableToProduceCycleCount=0;
    if(industryStatus.last_update<(QDateTime::currentMSecsSinceEpoch()/1000))
    {
        ableToProduceCycleCount=(QDateTime::currentMSecsSinceEpoch()/1000-industryStatus.last_update)/industry.time;
        if(ableToProduceCycleCount>industry.cycletobefull)
            ableToProduceCycleCount=industry.cycletobefull;
    }
    if(ableToProduceCycleCount<=0)
        return industryStatusCopy;
    int index;
    index=0;
    while(index<industry.resources.size())
    {
        const Industry::Resource &resource=industry.resources.at(index);
        const quint32 &quantityInStock=industryStatusCopy.resources.value(resource.item);
        const quint32 tempCycleCount=quantityInStock/resource.quantity;
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
        const quint32 &quantityInStock=industryStatusCopy.products.value(product.item);
        const quint32 tempCycleCount=(product.quantity*industry.cycletobefull-quantityInStock)/product.quantity;
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

quint32 FacilityLib::getFactoryResourcePrice(const quint32 &quantityInStock,const Industry::Resource &resource,const Industry &industry)
{
    quint32 max_items=resource.quantity*industry.cycletobefull;
    quint8 price_temp_change;
    if(quantityInStock>=max_items)
        price_temp_change=0;
    else
        price_temp_change=((max_items-quantityInStock)*CommonSettings::commonSettings.factoryPriceChange*2)/max_items;
    return CommonDatapack::commonDatapack.items.item.value(resource.item).price*(100-CommonSettings::commonSettings.factoryPriceChange+price_temp_change)/100;
}

quint32 FacilityLib::getFactoryProductPrice(const quint32 &quantityInStock, const Industry::Product &product, const Industry &industry)
{
    quint32 max_items=product.quantity*industry.cycletobefull;
    quint8 price_temp_change;
    if(quantityInStock>=max_items)
        price_temp_change=0;
    else
        price_temp_change=((max_items-quantityInStock)*CommonSettings::commonSettings.factoryPriceChange*2)/max_items;
    return CommonDatapack::commonDatapack.items.item.value(product.item).price*(100-CommonSettings::commonSettings.factoryPriceChange+price_temp_change)/100;
}

//reputation
void FacilityLib::appendReputationPoint(PlayerReputation *playerReputation,const qint32 &point,const Reputation &reputation)
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
        if(reputation.reputation_negative.isEmpty())
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
            if(playerReputation->level<=(-(reputation.reputation_negative.size()-1)) && playerReputation->point<=0)
            {
                playerReputation->point=0;
                playerReputation->level=(-(reputation.reputation_negative.size()-1));
                break;
            }
        }
        if(reputation.reputation_positive.isEmpty())
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
            if(playerReputation->level>=(reputation.reputation_positive.size()-1) && playerReputation->point>=0)
            {
                playerReputation->point=0;
                playerReputation->level=(reputation.reputation_positive.size()-1);
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
        if(playerReputation->level<=0 && playerReputation->point<0 && !reputation.reputation_negative.isEmpty())
        {
            if(playerReputation->point<=reputation.reputation_negative.at(-playerReputation->level+1))
            {
                playerReputation->point-=reputation.reputation_negative.at(-playerReputation->level+1);
                playerReputation->level--;
                needContinue=true;
            }
        }
        if(playerReputation->level>=0 && playerReputation->point>0 && !reputation.reputation_positive.isEmpty())
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
