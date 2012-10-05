#include "FacilityLib.h"

#include <QFileInfo>
#include <QFileInfoList>
#include <QDir>

using namespace Pokecraft;

QByteArray FacilityLib::toUTF8(const QString &text)
{
    if(text.isEmpty() || text.size()>255)
        return QByteArray();
    QByteArray returnedData,data;
    data=text.toUtf8();
    if(data.size()==0 || data.size()>255)
        return QByteArray();
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
