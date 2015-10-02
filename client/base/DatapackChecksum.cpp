#include "DatapackChecksum.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <std::regex>
#include <std::string>
#include <std::unordered_set>

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/CommonSettingsServer.h"

using namespace CatchChallenger;

#ifndef QT_NO_EMIT
QThread DatapackChecksum::thread;
#endif

DatapackChecksum::DatapackChecksum()
{
    #ifndef QT_NO_EMIT
    if(!thread.isRunning())
        thread.start();
    moveToThread(&thread);
    #endif
}

DatapackChecksum::~DatapackChecksum()
{
}

std::vector<char> DatapackChecksum::doChecksumBase(const std::string &datapackPath)
{
    QCryptographicHash hash(QCryptographicHash::Sha224);
    std::regex excludePath("^map[/\\\\]main[/\\\\]");

    const std::unordered_set<std::string> &extensionAllowed=std::string(CATCHCHALLENGER_EXTENSION_ALLOWED).split(";").toSet();
    std::regex datapack_rightFileName=std::regex(DATAPACK_FILE_REGEX);
    std::vector<std::string> returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath);
    returnList.sort();
    int index=0;
    const int &size=returnList.size();
    while(index<size)
    {
        const std::string &fileName=returnList.at(index);
        if(fileName.contains(datapack_rightFileName) && !fileName.contains(excludePath))
        {
            if(!QFileInfo(fileName).suffix().isEmpty() && extensionAllowed.contains(QFileInfo(fileName).suffix()))
            {
                QFile file(datapackPath+returnList.at(index));
                if(file.size()<=8*1024*1024)
                {
                    if(file.open(QIODevice::ReadOnly))
                    {
                        const std::vector<char> &data=file.readAll();
                        hash.addData(data);
                        file.close();
                    }
                    else
                    {
                        qDebug() << std::stringLiteral("Unable to open the file to do the checksum: %1").arg(file.fileName());
                        return std::vector<char>();
                    }
                }
            }
        }
        index++;
    }
    return hash.result();
}

#ifndef QT_NO_EMIT
void DatapackChecksum::doDifferedChecksumBase(const std::string &datapackPath)
{
    const FullDatapackChecksumReturn &fullDatapackChecksumReturn=doFullSyncChecksumBase(datapackPath);
    emit datapackChecksumDoneBase(fullDatapackChecksumReturn.datapackFilesList,fullDatapackChecksumReturn.hash,fullDatapackChecksumReturn.partialHashList);
}
#endif

DatapackChecksum::FullDatapackChecksumReturn DatapackChecksum::doFullSyncChecksumBase(const std::string &datapackPath)
{
    DatapackChecksum::FullDatapackChecksumReturn fullDatapackChecksumReturn;
    std::regex excludePath("^map[/\\\\]main[/\\\\]");
    const std::unordered_set<std::string> &extensionAllowed=std::string(CATCHCHALLENGER_EXTENSION_ALLOWED).split(";").toSet();
    std::regex datapack_rightFileName=std::regex(DATAPACK_FILE_REGEX);
    std::vector<std::string> returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath);
    returnList.sort();
    int index=0;
    const int &size=returnList.size();
    while(index<size)
    {
        const std::string &fileName=returnList.at(index);
        if(fileName.contains(datapack_rightFileName) && !fileName.contains(excludePath))
        {
            if(!QFileInfo(fileName).suffix().isEmpty() && extensionAllowed.contains(QFileInfo(fileName).suffix()))
            {
                QFile file(datapackPath+returnList.at(index));
                if(file.size()<=8*1024*1024)
                {
                    fullDatapackChecksumReturn.datapackFilesList << returnList.at(index);
                    if(file.open(QIODevice::ReadOnly))
                    {
                        const std::vector<char> &data=file.readAll();
                        {
                            QCryptographicHash hashFile(QCryptographicHash::Sha224);
                            hashFile.addData(data);
                            fullDatapackChecksumReturn.partialHashList << *reinterpret_cast<const int *>(hashFile.result().constData());
                        }
                        file.close();
                    }
                    else
                    {
                        fullDatapackChecksumReturn.partialHashList << 0;
                        qDebug() << std::stringLiteral("Unable to open the file to do the checksum: %1").arg(file.fileName());
                    }
                }
            }
        }
        index++;
    }
    fullDatapackChecksumReturn.hash=doChecksumBase(datapackPath);
    return fullDatapackChecksumReturn;
}

std::vector<char> DatapackChecksum::doChecksumMain(const std::string &datapackPath)
{
    QCryptographicHash hash(QCryptographicHash::Sha224);
    std::regex excludePath("^sub[/\\\\]");

    const std::unordered_set<std::string> &extensionAllowed=std::string(CATCHCHALLENGER_EXTENSION_ALLOWED).split(";").toSet();
    std::regex datapack_rightFileName=std::regex(DATAPACK_FILE_REGEX);
    std::vector<std::string> returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath);
    returnList.sort();
    int index=0;
    const int &size=returnList.size();
    while(index<size)
    {
        const std::string &fileName=returnList.at(index);
        if(fileName.contains(datapack_rightFileName) && !fileName.contains(excludePath))
        {
            if(!QFileInfo(fileName).suffix().isEmpty() && extensionAllowed.contains(QFileInfo(fileName).suffix()))
            {
                QFile file(datapackPath+returnList.at(index));
                if(file.size()<=8*1024*1024)
                {
                    if(file.open(QIODevice::ReadOnly))
                    {
                        const std::vector<char> &data=file.readAll();
                        hash.addData(data);
                        file.close();
                    }
                    else
                    {
                        qDebug() << std::stringLiteral("Unable to open the file to do the checksum: %1").arg(file.fileName());
                        return std::vector<char>();
                    }
                }
            }
        }
        index++;
    }
    return hash.result();
}

#ifndef QT_NO_EMIT
void DatapackChecksum::doDifferedChecksumMain(const std::string &datapackPath)
{
    const FullDatapackChecksumReturn &fullDatapackChecksumReturn=doFullSyncChecksumMain(datapackPath);
    emit datapackChecksumDoneMain(fullDatapackChecksumReturn.datapackFilesList,fullDatapackChecksumReturn.hash,fullDatapackChecksumReturn.partialHashList);
}
#endif

DatapackChecksum::FullDatapackChecksumReturn DatapackChecksum::doFullSyncChecksumMain(const std::string &datapackPath)
{
    DatapackChecksum::FullDatapackChecksumReturn fullDatapackChecksumReturn;
    std::regex excludePath("^sub[/\\\\]");
    const std::unordered_set<std::string> &extensionAllowed=std::string(CATCHCHALLENGER_EXTENSION_ALLOWED).split(";").toSet();
    std::regex datapack_rightFileName=std::regex(DATAPACK_FILE_REGEX);
    std::vector<std::string> returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath);
    returnList.sort();
    int index=0;
    const int &size=returnList.size();
    while(index<size)
    {
        const std::string &fileName=returnList.at(index);
        if(fileName.contains(datapack_rightFileName) && !fileName.contains(excludePath))
        {
            if(!QFileInfo(fileName).suffix().isEmpty() && extensionAllowed.contains(QFileInfo(fileName).suffix()))
            {
                QFile file(datapackPath+returnList.at(index));
                if(file.size()<=8*1024*1024)
                {
                    fullDatapackChecksumReturn.datapackFilesList << returnList.at(index);
                    if(file.open(QIODevice::ReadOnly))
                    {
                        const std::vector<char> &data=file.readAll();
                        {
                            QCryptographicHash hashFile(QCryptographicHash::Sha224);
                            hashFile.addData(data);
                            fullDatapackChecksumReturn.partialHashList << *reinterpret_cast<const int *>(hashFile.result().constData());
                        }
                        file.close();
                    }
                    else
                    {
                        fullDatapackChecksumReturn.partialHashList << 0;
                        qDebug() << std::stringLiteral("Unable to open the file to do the checksum: %1").arg(file.fileName());
                    }
                }
            }
        }
        index++;
    }
    fullDatapackChecksumReturn.hash=doChecksumMain(datapackPath);
    return fullDatapackChecksumReturn;
}

std::vector<char> DatapackChecksum::doChecksumSub(const std::string &datapackPath)
{
    QCryptographicHash hash(QCryptographicHash::Sha224);

    const std::unordered_set<std::string> &extensionAllowed=std::string(CATCHCHALLENGER_EXTENSION_ALLOWED).split(";").toSet();
    std::regex datapack_rightFileName=std::regex(DATAPACK_FILE_REGEX);
    std::vector<std::string> returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath);
    returnList.sort();
    int index=0;
    const int &size=returnList.size();
    while(index<size)
    {
        const std::string &fileName=returnList.at(index);
        if(fileName.contains(datapack_rightFileName))
        {
            if(!QFileInfo(fileName).suffix().isEmpty() && extensionAllowed.contains(QFileInfo(fileName).suffix()))
            {
                QFile file(datapackPath+returnList.at(index));
                if(file.size()<=8*1024*1024)
                {
                    if(file.open(QIODevice::ReadOnly))
                    {
                        const std::vector<char> &data=file.readAll();
                        hash.addData(data);
                        file.close();
                    }
                    else
                    {
                        qDebug() << std::stringLiteral("Unable to open the file to do the checksum: %1").arg(file.fileName());
                        return std::vector<char>();
                    }
                }
            }
        }
        index++;
    }
    qDebug() << std::stringLiteral("sub hash 1: %1").arg(std::string(hash.result().toHex()));
    return hash.result();
}

#ifndef QT_NO_EMIT
void DatapackChecksum::doDifferedChecksumSub(const std::string &datapackPath)
{
    const FullDatapackChecksumReturn &fullDatapackChecksumReturn=doFullSyncChecksumSub(datapackPath);
    emit datapackChecksumDoneSub(fullDatapackChecksumReturn.datapackFilesList,fullDatapackChecksumReturn.hash,fullDatapackChecksumReturn.partialHashList);
}
#endif

DatapackChecksum::FullDatapackChecksumReturn DatapackChecksum::doFullSyncChecksumSub(const std::string &datapackPath)
{
    DatapackChecksum::FullDatapackChecksumReturn fullDatapackChecksumReturn;
    const std::unordered_set<std::string> &extensionAllowed=std::string(CATCHCHALLENGER_EXTENSION_ALLOWED).split(";").toSet();
    std::regex datapack_rightFileName=std::regex(DATAPACK_FILE_REGEX);
    std::vector<std::string> returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath);
    returnList.sort();
    int index=0;
    const int &size=returnList.size();
    while(index<size)
    {
        const std::string &fileName=returnList.at(index);
        if(fileName.contains(datapack_rightFileName))
        {
            if(!QFileInfo(fileName).suffix().isEmpty() && extensionAllowed.contains(QFileInfo(fileName).suffix()))
            {
                QFile file(datapackPath+returnList.at(index));
                if(file.size()<=8*1024*1024)
                {
                    fullDatapackChecksumReturn.datapackFilesList << returnList.at(index);
                    if(file.open(QIODevice::ReadOnly))
                    {
                        const std::vector<char> &data=file.readAll();
                        {
                            QCryptographicHash hashFile(QCryptographicHash::Sha224);
                            hashFile.addData(data);
                            fullDatapackChecksumReturn.partialHashList << *reinterpret_cast<const int *>(hashFile.result().constData());
                        }
                        file.close();
                    }
                    else
                    {
                        fullDatapackChecksumReturn.partialHashList << 0;
                        qDebug() << std::stringLiteral("Unable to open the file to do the checksum: %1").arg(file.fileName());
                    }
                }
            }
        }
        index++;
    }
    fullDatapackChecksumReturn.hash=doChecksumSub(datapackPath);
    return fullDatapackChecksumReturn;
}
