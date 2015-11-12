#include "FacilityLibGeneral.h"

#include <QFileInfo>
#include <QFileInfoList>
#include <QDir>

using namespace CatchChallenger;

std::string FacilityLibGeneral::text_slash="/";
std::string FacilityLibGeneral::text_male="male";
std::string FacilityLibGeneral::text_female="female";
std::string FacilityLibGeneral::text_unknown="unknown";
std::string FacilityLibGeneral::text_clan="clan";
std::string FacilityLibGeneral::text_dotcomma=";";

unsigned int FacilityLibGeneral::toUTF8WithHeader(const std::string &text,char * const data)
{
    if(text.empty() || text.size()>255)
        return 0;
    data[0]=text.size();
    memcpy(data+1,text.data(),text.size());
    return 1+text.size();
}

unsigned int FacilityLibGeneral::toUTF8With16BitsHeader(const std::string &text,char * const data)
{
    if(text.empty() || text.size()>65535)
        return 0;
    *reinterpret_cast<uint16_t *>(data+0)=(uint16_t)htole16((uint16_t)text.size());
    memcpy(data+2,text.data(),text.size());
    return 2+text.size();
}

std::vector<std::string> FacilityLibGeneral::listFolder(const std::string& folder,const std::string& suffix)
{
    std::vector<std::string> returnList;
    QFileInfoList entryList=QDir(QString::fromStdString(folder+suffix)).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
        {
            const std::vector<std::string> &newList=listFolder(folder,suffix+fileInfo.fileName().toStdString()+text_slash);//put unix separator because it's transformed into that's under windows too
            returnList.insert(returnList.end(),newList.begin(),newList.end());
        }
        else if(fileInfo.isFile())
            returnList.push_back(suffix+fileInfo.fileName().toStdString());
    }
    return returnList;
}

std::vector<std::string> FacilityLibGeneral::listFolderWithExclude(const std::string& folder,const std::string &exclude,const std::string& suffix)
{
    std::vector<std::string> returnList;
    if(suffix==exclude)
        return returnList;
    QFileInfoList entryList=QDir(QString::fromStdString(folder+suffix)).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
        {
            const std::vector<std::string> &newList=listFolderWithExclude(folder,exclude,suffix+fileInfo.fileName().toStdString()+text_slash);//put unix separator because it's transformed into that's under windows too
            returnList.insert(returnList.end(),newList.begin(),newList.end());
        }
        else if(fileInfo.isFile())
            returnList.push_back(suffix+fileInfo.fileName().toStdString());
    }
    return returnList;
}

std::string FacilityLibGeneral::randomPassword(const std::string& string,const uint8_t& length)
{
    if(string.size()<2)
        return std::string();
    std::string randomPassword;
    int index=0;
    while(index<length)
    {
        randomPassword+=string.at(rand()%string.size());
        index++;
    }
    return randomPassword;
}

std::vector<std::string> FacilityLibGeneral::skinIdList(const std::string& skinPath)
{
    std::vector<std::string> skinFolderList;
    QFileInfoList entryList=QDir(QString::fromStdString(skinPath)).entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            if(QFile(fileInfo.absoluteFilePath()+"/back.png").exists() &&
                    QFile(fileInfo.absoluteFilePath()+"/front.png").exists() &&
                    QFile(fileInfo.absoluteFilePath()+"/trainer.png").exists())
                skinFolderList.push_back(fileInfo.fileName().toStdString());
    }
    std::sort(skinFolderList.begin(), skinFolderList.end());
    while(skinFolderList.size()>255)
        skinFolderList.pop_back();
    return skinFolderList;
}

/*std::string FacilityLibGeneral::secondsToString(const uint64_t &seconds)
{
    if(seconds<60)
        return QObject::tr("%n second(s)","",seconds);
    else if(seconds<60*60)
        return QObject::tr("%n minute(s)","",seconds/60);
    else if(seconds<60*60*24)
        return QObject::tr("%n hour(s)","",seconds/(60*60));
    else
        return QObject::tr("%n day(s)","",seconds/(60*60*24));
}*/

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

bool FacilityLibGeneral::rmpath(const std::string &dirPath)
{
    const QDir dir(QString::fromStdString(dirPath));
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
            if(!FacilityLibGeneral::rmpath(dir.absolutePath().toStdString()+FacilityLibGeneral::text_slash+fileInfo.fileName().toStdString()+FacilityLibGeneral::text_slash))
                allHaveWork=false;
        }
    }
    if(!allHaveWork)
        return false;
    allHaveWork=dir.rmdir(dir.absolutePath());
    return allHaveWork;
}

/*std::string FacilityLibGeneral::timeToString(const uint32_t &time)
{
    if(time>=3600*24*10)
        return QObject::tr("%n day(s)","",time/(3600*24));
    else if(time>=3600*24)
        return QObject::tr("%n day(s) and %1","",time/(3600*24)).arg(QObject::tr("%n hour(s)","",(time%(3600*24))/3600));
    else if(time>=3600)
        return QObject::tr("%n hour(s) and %1","",time/3600).arg(QObject::tr("%n minute(s)","",(time%3600)/60));
    else
        return QObject::tr("%n minute(s) and %1","",time/60).arg(QObject::tr("%n second(s)","",time%60));
}*/
