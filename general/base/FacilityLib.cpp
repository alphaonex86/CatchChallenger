#include "FacilityLib.h"

#include <QFileInfo>
#include <QFileInfoList>
#include <QDir>

using namespace CatchChallenger;

QByteArray FacilityLib::UTF8EmptyData=QByteArray().fill(0x00,1);

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
    QFileInfoList entryList=QDir(folder+suffix).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            returnList+=listFolder(folder,suffix+fileInfo.fileName()+"/");//put unix separator because it's transformed into that's under windows too
        else if(fileInfo.isFile())
            returnList+=suffix+fileInfo.fileName();
    }
    return returnList;
}

QString FacilityLib::randomPassword(const QString& string,const quint8& length)
{
    if(string.size()<2)
        return "";
    QString randomPassword;
    int index=0;
    while(index<length)
    {
        randomPassword+=string[rand()%string.size()];
        index++;
    }
    return randomPassword;
}

QStringList FacilityLib::skinIdList(const QString& skinPath)
{
    QStringList skinFolderList;
    QFileInfoList entryList=QDir(skinPath).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            if(QFile(fileInfo.absoluteFilePath()+"/back.png").exists() && QFile(fileInfo.absoluteFilePath()+"/front.png").exists() && QFile(fileInfo.absoluteFilePath()+"/trainer.png").exists())
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
    returnVar.captured_with=playerMonster.captured_with;
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
    out << (quint32)publicPlayerMonster.captured_with;
    out << (quint8)publicPlayerMonster.gender;
    out << (quint32)publicPlayerMonster.buffs.size();
    int index=0;
    while(index<publicPlayerMonster.buffs.size())
    {
        out << (quint32)publicPlayerMonster.buffs[index].buff;
        out << (quint8)publicPlayerMonster.buffs[index].level;
        index++;
    }
    return outputData;
}

PlayerMonster FacilityLib::botFightMonsterToPlayerMonster(const BotFight::BotFightMonster &botFightMonster,const Monster::Stat &stat)
{
    PlayerMonster tempPlayerMonster;
    tempPlayerMonster.captured_with=0;
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
            return "male";
        case Gender_Female:
            return "female";
        default:
            break;
    }
    return "unknown";
}
