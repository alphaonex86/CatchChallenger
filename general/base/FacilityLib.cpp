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

QByteArray FacilityLib::toUTF8(const QString &text)
{
    if(text.isEmpty() || text.size()>255)
        return UTF8EmptyData;
    QByteArray returnedData,data;
    data=text.toUtf8();
    if(data.size()==0 || data.size()>255)
        return UTF8EmptyData;
    returnedData[0]=data.size();
    returnedData+=data;
    return returnedData;
}

QStringList FacilityLib::listFolder(const QString& folder,const QString& suffix)
{
    QStringList returnList;
    QFileInfoList entryList=QDir(folder+suffix).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            returnList+=listFolder(folder,suffix+fileInfo.fileName()+text_slash);//put unix separator because it's transformed into that's under windows too
        else if(fileInfo.isFile())
            returnList+=suffix+fileInfo.fileName();
    }
    return returnList;
}

QString FacilityLib::randomPassword(const QString& string,const quint8& length)
{
    if(string.size()<2)
        return QString();
    QString randomPassword;
    int index=0;
    while(index<length)
    {
        randomPassword+=string.at(rand()%string.size());
        index++;
    }
    return randomPassword;
}

QStringList FacilityLib::skinIdList(const QString& skinPath)
{
    const QString &slashbackpng=QStringLiteral("/back.png");
    const QString &slashfrontpng=QStringLiteral("/front.png");
    const QString &slashtrainerpng=QStringLiteral("/trainer.png");
    QStringList skinFolderList;
    QFileInfoList entryList=QDir(skinPath).entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            if(QFile(fileInfo.absoluteFilePath()+slashbackpng).exists() && QFile(fileInfo.absoluteFilePath()+slashfrontpng).exists() && QFile(fileInfo.absoluteFilePath()+slashtrainerpng).exists())
                skinFolderList << fileInfo.fileName();
    }
    skinFolderList.sort();
    while(skinFolderList.size()>255)
        skinFolderList.removeLast();
    return skinFolderList;
}

QString FacilityLib::secondsToString(const quint64 &seconds)
{
    if(seconds<60)
        return QObject::tr("%n second(s)","",seconds);
    else if(seconds<60*60)
        return QObject::tr("%n minute(s)","",seconds/60);
    else if(seconds<60*60*24)
        return QObject::tr("%n hour(s)","",seconds/(60*60));
    else
        return QObject::tr("%n day(s)","",seconds/(60*60*24));
}

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
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)publicPlayerMonster.monster;
    out << (quint8)publicPlayerMonster.level;
    out << (quint32)publicPlayerMonster.hp;
    out << (quint32)publicPlayerMonster.catched_with;
    out << (quint8)publicPlayerMonster.gender;
    out << (quint32)publicPlayerMonster.buffs.size();
    int index=0;
    while(index<publicPlayerMonster.buffs.size())
    {
        out << (quint32)publicPlayerMonster.buffs.at(index).buff;
        out << (quint8)publicPlayerMonster.buffs.at(index).level;
        index++;
    }
    return outputData;
}

QByteArray playerMonsterToBinary(const PlayerMonster &playerMonster)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)playerMonster.monster;
    out << (quint8)playerMonster.level;
    out << (quint32)playerMonster.hp;
    out << (quint32)playerMonster.catched_with;
    out << (quint8)playerMonster.gender;
    out << (quint32)playerMonster.buffs.size();
    int index=0;
    while(index<playerMonster.buffs.size())
    {
        out << (quint32)playerMonster.buffs.at(index).buff;
        out << (quint8)playerMonster.buffs.at(index).level;
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

bool FacilityLib::rectTouch(QRect r1,QRect r2)
{
    if (r1.isNull() || r2.isNull())
        return false;

    if((r1.x()+r1.width())<r2.x())
        return false;
    if((r2.x()+r2.width())<r1.x())
        return false;

    if((r1.y()+r1.height())<r2.y())
        return false;
    if((r2.y()+r2.height())<r1.y())
        return false;

    return true;
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

QString FacilityLib::allowToString(const QSet<ActionAllow> &allowList)
{
    QStringList allowString;
    QSetIterator<ActionAllow> i(allowList);
    while (i.hasNext())
        switch(i.next())
        {
            case ActionAllow_Clan:
                allowString << FacilityLib::text_clan;
            break;
            default:
            break;
        }
    return allowString.join(FacilityLib::text_dotcomma);
}

QSet<ActionAllow> FacilityLib::StringToAllow(const QString &string)
{
    QSet<ActionAllow> allowList;
    const QStringList &allowStringList=string.split(FacilityLib::text_dotcomma);
    int index=0;
    while(index<allowStringList.size())
    {
        if(allowStringList.at(index)==FacilityLib::text_clan)
            allowList << ActionAllow_Clan;
        index++;
    }
    return allowList;
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
    out.setVersion(QDataStream::Qt_4_4);

    out << (quint32)monster.id;
    out << (quint32)monster.monster;
    out << (quint8)monster.level;
    out << (quint32)monster.remaining_xp;
    out << (quint32)monster.hp;
    out << (quint32)monster.sp;
    out << (quint32)monster.catched_with;
    out << (quint8)monster.gender;
    out << (quint32)monster.egg_step;
    int sub_index=0;
    int sub_size=monster.buffs.size();
    out << (quint32)sub_size;
    while(sub_index<sub_size)
    {
        out << (quint32)monster.buffs.at(sub_index).buff;
        out << (quint8)monster.buffs.at(sub_index).level;
        sub_index++;
    }
    sub_index=0;
    sub_size=monster.skills.size();
    out << (quint32)sub_size;
    while(sub_index<sub_size)
    {
        out << (quint32)monster.skills.at(sub_index).skill;
        out << (quint8)monster.skills.at(sub_index).level;
        out << (quint8)monster.skills.at(sub_index).endurance;
        sub_index++;
    }
    return outputData;
}

bool FacilityLib::rmpath(const QDir &dir)
{
    if(!dir.exists())
        return true;
    bool allHaveWork=true;
    QFileInfoList list = dir.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);
    for (int i = 0; i < list.size(); ++i)
    {
        QFileInfo fileInfo(list.at(i));
        if(!fileInfo.isDir())
        {
            if(!QFile(fileInfo.absoluteFilePath()).remove())
                allHaveWork=false;
        }
        else
        {
            //return the fonction for scan the new folder
            if(!FacilityLib::rmpath(dir.absolutePath()+FacilityLib::text_slash+fileInfo.fileName()+FacilityLib::text_slash))
                allHaveWork=false;
        }
    }
    if(!allHaveWork)
        return false;
    allHaveWork=dir.rmdir(dir.absolutePath());
    return allHaveWork;
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


QString FacilityLib::timeToString(const quint32 &time)
{
    if(time>=3600*24*10)
        return QObject::tr("%n day(s)","",time/(3600*24));
    else if(time>=3600*24)
        return QObject::tr("%n day(s) and %1","",time/(3600*24)).arg(QObject::tr("%n hour(s)","",(time%(3600*24))/3600));
    else if(time>=3600)
        return QObject::tr("%n hour(s) and %1","",time/3600).arg(QObject::tr("%n minute(s)","",(time%3600)/60));
    else
        return QObject::tr("%n minute(s) and %1","",time/60).arg(QObject::tr("%n second(s)","",time%60));
}
