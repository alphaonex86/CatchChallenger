#include "FacilityLibGeneral.h"

#include <QFileInfo>
#include <QFileInfoList>
#include <QDir>

using namespace CatchChallenger;

QByteArray FacilityLibGeneral::UTF8EmptyData=QByteArray().fill(0x00,1);
QString FacilityLibGeneral::text_slash=QLatin1Literal("/");
QString FacilityLibGeneral::text_male=QLatin1Literal("male");
QString FacilityLibGeneral::text_female=QLatin1Literal("female");
QString FacilityLibGeneral::text_unknown=QLatin1Literal("unknown");
QString FacilityLibGeneral::text_clan=QLatin1Literal("clan");
QString FacilityLibGeneral::text_dotcomma=QLatin1Literal(";");

QByteArray FacilityLibGeneral::toUTF8WithHeader(const QString &text)
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

int FacilityLibGeneral::toUTF8WithHeader(const QString &text,char * const data)
{
    if(text.isEmpty() || text.size()>255)
        return 0;
    QByteArray utf8data;
    utf8data=text.toUtf8();
    if(utf8data.size()==0 || utf8data.size()>255)
        return 0;
    data[0]=utf8data.size();
    memcpy(data+1,utf8data.constData(),utf8data.size());
    return 1+utf8data.size();
}

int FacilityLibGeneral::toUTF8With16BitsHeader(const QString &text,char * const data)
{
    if(text.isEmpty() || text.size()>65535)
        return 0;
    QByteArray utf8data;
    utf8data=text.toUtf8();
    if(utf8data.size()==0 || utf8data.size()>65535)
        return 0;
    *reinterpret_cast<quint16 *>(data+0)=(quint16)htole16((quint16)utf8data.size());
    memcpy(data+2,utf8data.constData(),utf8data.size());
    return 2+utf8data.size();
}

QStringList FacilityLibGeneral::listFolder(const QString& folder,const QString& suffix)
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

QString FacilityLibGeneral::randomPassword(const QString& string,const quint8& length)
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

QStringList FacilityLibGeneral::skinIdList(const QString& skinPath)
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

QString FacilityLibGeneral::secondsToString(const quint64 &seconds)
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

bool FacilityLibGeneral::rectTouch(QRect r1,QRect r2)
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

bool FacilityLibGeneral::rmpath(const QDir &dir)
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
            if(!FacilityLibGeneral::rmpath(dir.absolutePath()+FacilityLibGeneral::text_slash+fileInfo.fileName()+FacilityLibGeneral::text_slash))
                allHaveWork=false;
        }
    }
    if(!allHaveWork)
        return false;
    allHaveWork=dir.rmdir(dir.absolutePath());
    return allHaveWork;
}

QString FacilityLibGeneral::timeToString(const quint32 &time)
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
