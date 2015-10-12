#include "DatapackChecksum.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <regex>
#include <string>
#include <unordered_set>
#include <iostream>

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
    {
        std::regex excludePath("^map[/\\\\]main[/\\\\]");

        const std::vector<std::string> &extensionAllowedList=stringsplit(CATCHCHALLENGER_EXTENSION_ALLOWED,';');
        const std::unordered_set<std::string> extensionAllowed(extensionAllowedList.cbegin(),extensionAllowedList.cend());
        std::regex datapack_rightFileName=std::regex(DATAPACK_FILE_REGEX);
        std::vector<std::string> returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath);
        std::sort(returnList.begin(),returnList.end());
        int index=0;
        const int &size=returnList.size();
        while(index<size)
        {
            const std::string &fileName=returnList.at(index);
            if(std::regex_match(fileName,datapack_rightFileName) && !std::regex_match(fileName,excludePath))
            {
                if(!QFileInfo(QString::fromStdString(fileName)).suffix().isEmpty() && extensionAllowed.find(QFileInfo(QString::fromStdString(fileName)).suffix().toStdString())!=extensionAllowed.cend())
                {
                    QFile file(QString::fromStdString(datapackPath+returnList.at(index)));
                    if(file.size()<=8*1024*1024)
                    {
                        if(file.open(QIODevice::ReadOnly))
                        {
                            hash.addData(file.readAll());
                            file.close();
                        }
                        else
                        {
                            std::cerr << "Unable to open the file to do the checksum: " << datapackPath << returnList.at(index) << std::endl;
                            return std::vector<char>();
                        }
                    }
                }
            }
            index++;
        }
    }
    std::vector<char> hashResult;
    {
        const QByteArray &QtData=hash.result();
        hashResult.resize(QtData.size());
        memcpy(hashResult.data(),QtData.constData(),QtData.size());
    }
    return hashResult;
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
    const std::vector<std::string> &extensionAllowedList=stringsplit(CATCHCHALLENGER_EXTENSION_ALLOWED,';');
    const std::unordered_set<std::string> extensionAllowed(extensionAllowedList.cbegin(),extensionAllowedList.cend());
    std::regex datapack_rightFileName=std::regex(DATAPACK_FILE_REGEX);
    std::vector<std::string> returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath);
    std::sort(returnList.begin(),returnList.end());
    int index=0;
    const int &size=returnList.size();
    while(index<size)
    {
        const std::string &fileName=returnList.at(index);
        if(std::regex_match(fileName,datapack_rightFileName) && !std::regex_match(fileName,excludePath))
        {
            if(!QFileInfo(QString::fromStdString(fileName)).suffix().isEmpty() && extensionAllowed.find(QFileInfo(QString::fromStdString(fileName)).suffix().toStdString())!=extensionAllowed.cend())
            {
                QFile file(QString::fromStdString(datapackPath+returnList.at(index)));
                if(file.size()<=8*1024*1024)
                {
                    fullDatapackChecksumReturn.datapackFilesList.push_back(returnList.at(index));
                    if(file.open(QIODevice::ReadOnly))
                    {
                        QCryptographicHash hashFile(QCryptographicHash::Sha224);
                        hashFile.addData(file.readAll());
                        fullDatapackChecksumReturn.partialHashList.push_back(*reinterpret_cast<const int *>(hashFile.result().constData()));
                        file.close();
                    }
                    else
                    {
                        fullDatapackChecksumReturn.partialHashList.push_back(0);
                        std::cerr << "Unable to open the file to do the checksum: " << datapackPath << returnList.at(index) << std::endl;
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
    {
        std::regex excludePath("^sub[/\\\\]");

        const std::vector<std::string> &extensionAllowedList=stringsplit(CATCHCHALLENGER_EXTENSION_ALLOWED,';');
        const std::unordered_set<std::string> extensionAllowed(extensionAllowedList.cbegin(),extensionAllowedList.cend());
        std::regex datapack_rightFileName=std::regex(DATAPACK_FILE_REGEX);
        std::vector<std::string> returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath);
        std::sort(returnList.begin(),returnList.end());
        int index=0;
        const int &size=returnList.size();
        while(index<size)
        {
            const std::string &fileName=returnList.at(index);
            if(std::regex_match(fileName,datapack_rightFileName) && !std::regex_match(fileName,excludePath))
            {
                if(!QFileInfo(QString::fromStdString(fileName)).suffix().isEmpty() && extensionAllowed.find(QFileInfo(QString::fromStdString(fileName)).suffix().toStdString())!=extensionAllowed.cend())
                {
                    QFile file(QString::fromStdString(datapackPath+returnList.at(index)));
                    if(file.size()<=8*1024*1024)
                    {
                        if(file.open(QIODevice::ReadOnly))
                        {
                            hash.addData(file.readAll());
                            file.close();
                        }
                        else
                        {
                            std::cerr << "Unable to open the file to do the checksum: " << datapackPath << returnList.at(index) << std::endl;
                            return std::vector<char>();
                        }
                    }
                }
            }
            index++;
        }
    }
    std::vector<char> hashResult;
    {
        const QByteArray &QtData=hash.result();
        hashResult.resize(QtData.size());
        memcpy(hashResult.data(),QtData.constData(),QtData.size());
    }
    return hashResult;
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
    const std::vector<std::string> &extensionAllowedList=stringsplit(CATCHCHALLENGER_EXTENSION_ALLOWED,';');
    const std::unordered_set<std::string> extensionAllowed(extensionAllowedList.cbegin(),extensionAllowedList.cend());
    std::regex datapack_rightFileName=std::regex(DATAPACK_FILE_REGEX);
    std::vector<std::string> returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath);
    std::sort(returnList.begin(),returnList.end());
    int index=0;
    const int &size=returnList.size();
    while(index<size)
    {
        const std::string &fileName=returnList.at(index);
        if(std::regex_match(fileName,datapack_rightFileName) && !std::regex_match(fileName,excludePath))
        {
            if(!QFileInfo(QString::fromStdString(fileName)).suffix().isEmpty() && extensionAllowed.find(QFileInfo(QString::fromStdString(fileName)).suffix().toStdString())!=extensionAllowed.cend())
            {
                QFile file(QString::fromStdString(datapackPath+returnList.at(index)));
                if(file.size()<=8*1024*1024)
                {
                    fullDatapackChecksumReturn.datapackFilesList.push_back(returnList.at(index));
                    if(file.open(QIODevice::ReadOnly))
                    {
                        QCryptographicHash hashFile(QCryptographicHash::Sha224);
                        hashFile.addData(file.readAll());
                        fullDatapackChecksumReturn.partialHashList.push_back(*reinterpret_cast<const int *>(hashFile.result().constData()));
                        file.close();
                    }
                    else
                    {
                        fullDatapackChecksumReturn.partialHashList.push_back(0);
                        std::cerr << "Unable to open the file to do the checksum: " << datapackPath << returnList.at(index) << std::endl;
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

    {
        const std::vector<std::string> &extensionAllowedList=stringsplit(CATCHCHALLENGER_EXTENSION_ALLOWED,';');
        const std::unordered_set<std::string> extensionAllowed(extensionAllowedList.cbegin(),extensionAllowedList.cend());
        std::regex datapack_rightFileName=std::regex(DATAPACK_FILE_REGEX);
        std::vector<std::string> returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath);
        std::sort(returnList.begin(),returnList.end());
        int index=0;
        const int &size=returnList.size();
        while(index<size)
        {
            const std::string &fileName=returnList.at(index);
            if(std::regex_match(fileName,datapack_rightFileName))
            {
                if(!QFileInfo(QString::fromStdString(fileName)).suffix().isEmpty() && extensionAllowed.find(QFileInfo(QString::fromStdString(fileName)).suffix().toStdString())!=extensionAllowed.cend())
                {
                    QFile file(QString::fromStdString(datapackPath+returnList.at(index)));
                    if(file.size()<=8*1024*1024)
                    {
                        if(file.open(QIODevice::ReadOnly))
                        {
                            hash.addData(file.readAll());
                            file.close();
                        }
                        else
                        {
                            std::cerr << "Unable to open the file to do the checksum: " << datapackPath << returnList.at(index) << std::endl;
                            return std::vector<char>();
                        }
                    }
                }
            }
            index++;
        }
    }
    std::vector<char> hashResult;
    {
        const QByteArray &QtData=hash.result();
        hashResult.resize(QtData.size());
        memcpy(hashResult.data(),QtData.constData(),QtData.size());
    }
    return hashResult;
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
    const std::vector<std::string> &extensionAllowedList=stringsplit(CATCHCHALLENGER_EXTENSION_ALLOWED,';');
    const std::unordered_set<std::string> extensionAllowed(extensionAllowedList.cbegin(),extensionAllowedList.cend());
    std::regex datapack_rightFileName=std::regex(DATAPACK_FILE_REGEX);
    std::vector<std::string> returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath);
    std::sort(returnList.begin(),returnList.end());
    int index=0;
    const int &size=returnList.size();
    while(index<size)
    {
        const std::string &fileName=returnList.at(index);
        if(std::regex_match(fileName,datapack_rightFileName))
        {
            if(!QFileInfo(QString::fromStdString(fileName)).suffix().isEmpty() && extensionAllowed.find(QFileInfo(QString::fromStdString(fileName)).suffix().toStdString())!=extensionAllowed.cend())
            {
                QFile file(QString::fromStdString(datapackPath+returnList.at(index)));
                if(file.size()<=8*1024*1024)
                {
                    fullDatapackChecksumReturn.datapackFilesList.push_back(returnList.at(index));
                    if(file.open(QIODevice::ReadOnly))
                    {
                        QCryptographicHash hashFile(QCryptographicHash::Sha224);
                        hashFile.addData(file.readAll());
                        fullDatapackChecksumReturn.partialHashList.push_back(*reinterpret_cast<const int *>(hashFile.result().constData()));
                        file.close();
                    }
                    else
                    {
                        fullDatapackChecksumReturn.partialHashList.push_back(0);
                        std::cerr << "Unable to open the file to do the checksum: " << datapackPath << returnList.at(index) << std::endl;
                    }
                }
            }
        }
        index++;
    }
    fullDatapackChecksumReturn.hash=doChecksumSub(datapackPath);
    return fullDatapackChecksumReturn;
}
