#include "FacilityLib.h"

#include <QFileInfo>
#include <QFileInfoList>
#include <QDir>

using namespace Pokecraft;

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
