#include "DatapackChecksum.h"

#include <regex>
#include <string>
#include <unordered_set>
#include <iostream>
#include "../../general/base/CatchChallenger_Hash.hpp"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/CommonSettingsServer.h"

using namespace CatchChallenger;

#if ! defined(QT_NO_EMIT) && ! defined(CATCHCHALLENGER_SERVER) && !defined(NOTHREADS)
QThread DatapackChecksum::thread;
#endif

DatapackChecksum::DatapackChecksum()
{
    #if ! defined(QT_NO_EMIT) && ! defined(CATCHCHALLENGER_SERVER) && !defined(NOTHREADS)
    if(!thread.isRunning())
        thread.start();
    moveToThread(&thread);
    thread.setObjectName("DatapackChecksum");
    #endif
}

DatapackChecksum::~DatapackChecksum()
{
    #if ! defined(QT_NO_EMIT) && ! defined(CATCHCHALLENGER_SERVER) && !defined(NOTHREADS)
    stopThread();
    #endif
}

#if ! defined(QT_NO_EMIT) && ! defined(CATCHCHALLENGER_SERVER) && !defined(NOTHREADS)
void DatapackChecksum::stopThread()
{
    thread.exit();
    thread.quit();
    thread.wait();
}
#endif

std::vector<char> DatapackChecksum::doChecksumBase(const std::string &datapackPath)
{
    CatchChallenger::Hash hash;
    {
        std::regex excludePath("^map[/\\\\]main[/\\\\]");

        const std::vector<std::string> &extensionAllowedList=stringsplit(CATCHCHALLENGER_EXTENSION_ALLOWED,';');
        const std::unordered_set<std::string> extensionAllowed(extensionAllowedList.cbegin(),extensionAllowedList.cend());
        std::regex datapack_rightFileName=std::regex(DATAPACK_FILE_REGEX);
        std::vector<std::string> returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath);
        std::sort(returnList.begin(),returnList.end());
        unsigned int index=0;
        while(index<returnList.size())
        {
            const std::string &fileName=returnList.at(index);
            if(regex_search(fileName,datapack_rightFileName) && !regex_search(fileName,excludePath))
            {
                const std::string &suffix=CatchChallenger::FacilityLibGeneral::getSuffix(fileName);
                if(!suffix.empty() && extensionAllowed.find(suffix)!=extensionAllowed.cend())
                {
                    std::string fullPathFileToOpen=datapackPath+fileName;
                    #ifdef Q_OS_WIN32
                    stringreplaceAll(fullPathFileToOpen,"/","\\");
                    #endif
                    FILE *file=fopen(fullPathFileToOpen.c_str(),"rb");
                    if(file!=NULL)
                    {
                        const std::vector<char> &data=CatchChallenger::FacilityLibGeneral::readAllFileAndClose(file);
                        hash.update(reinterpret_cast<const unsigned char *>(data.data()),data.size());
                    }
                    else
                    {
                        std::cerr << "Unable to open the file to do the checksum: " << datapackPath << returnList.at(index) << std::endl;
                        return std::vector<char>();
                    }
                }
            }
            index++;
        }
    }
    std::vector<char> hashResult;
    {
        hashResult.resize(CATCHCHALLENGER_HASH_SIZE);
        hash.final(reinterpret_cast<unsigned char *>(hashResult.data()));
    }
    return hashResult;
}

#if ! defined(QT_NO_EMIT) && ! defined(CATCHCHALLENGER_SERVER)
void DatapackChecksum::doDifferedChecksumBase(const std::string &datapackPath)
{
    std::cerr << "DatapackChecksum::doDifferedChecksumBase" << std::endl;
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
    unsigned int index=0;
    while(index<returnList.size())
    {
        const std::string &fileName=returnList.at(index);
        if(regex_search(fileName,datapack_rightFileName) && !regex_search(fileName,excludePath))
        {
            const std::string &suffix=CatchChallenger::FacilityLibGeneral::getSuffix(fileName);
            if(!suffix.empty() && extensionAllowed.find(suffix)!=extensionAllowed.cend())
            {
                std::string fullPathFileToOpen=datapackPath+fileName;
                #ifdef Q_OS_WIN32
                stringreplaceAll(fullPathFileToOpen,"/","\\");
                #endif
                FILE *file=fopen(fullPathFileToOpen.c_str(),"rb");
                fullDatapackChecksumReturn.datapackFilesList.push_back(returnList.at(index));
                if(file!=NULL)
                {
                    const std::vector<char> &data=CatchChallenger::FacilityLibGeneral::readAllFileAndClose(file);
                    std::vector<char> hashResult;
                    hashResult.resize(CATCHCHALLENGER_HASH_SIZE);
                    {
                        CatchChallenger::Hash oneshot;
                        oneshot.update(reinterpret_cast<const unsigned char *>(data.data()),data.size());
                        oneshot.final(reinterpret_cast<unsigned char *>(hashResult.data()));
                    }
                    fullDatapackChecksumReturn.partialHashList.push_back(*reinterpret_cast<const int *>(hashResult.data()));
                }
                else
                {
                    fullDatapackChecksumReturn.partialHashList.push_back(0);
                    std::cerr << "Unable to open the file to do the checksum: " << datapackPath << returnList.at(index) << std::endl;
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
    CatchChallenger::Hash hash;
    {
        std::regex excludePath("^sub[/\\\\]");

        const std::vector<std::string> &extensionAllowedList=stringsplit(CATCHCHALLENGER_EXTENSION_ALLOWED,';');
        const std::unordered_set<std::string> extensionAllowed(extensionAllowedList.cbegin(),extensionAllowedList.cend());
        std::regex datapack_rightFileName=std::regex(DATAPACK_FILE_REGEX);
        std::vector<std::string> returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath);
        std::sort(returnList.begin(),returnList.end());
        unsigned int index=0;
        while(index<returnList.size())
        {
            const std::string &fileName=returnList.at(index);
            if(regex_search(fileName,datapack_rightFileName) && !regex_search(fileName,excludePath))
            {
                const std::string &suffix=CatchChallenger::FacilityLibGeneral::getSuffix(fileName);
                if(!suffix.empty() && extensionAllowed.find(suffix)!=extensionAllowed.cend())
                {
                    std::string fullPathFileToOpen=datapackPath+fileName;
                    #ifdef Q_OS_WIN32
                    stringreplaceAll(fullPathFileToOpen,"/","\\");
                    #endif
                    FILE *file=fopen(fullPathFileToOpen.c_str(),"rb");
                    if(file!=NULL)
                    {
                        const std::vector<char> &data=CatchChallenger::FacilityLibGeneral::readAllFileAndClose(file);
                        hash.update(reinterpret_cast<const unsigned char *>(data.data()),data.size());
                    }
                    else
                    {
                        std::cerr << "Unable to open the file to do the checksum: " << datapackPath << returnList.at(index) << std::endl;
                        return std::vector<char>();
                    }
                }
            }
            index++;
        }
    }
    std::vector<char> hashResult;
    {
        hashResult.resize(CATCHCHALLENGER_HASH_SIZE);
        hash.final(reinterpret_cast<unsigned char *>(hashResult.data()));
    }
    return hashResult;
}

#if ! defined(QT_NO_EMIT) && ! defined(CATCHCHALLENGER_SERVER)
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
    unsigned int index=0;
    while(index<returnList.size())
    {
        const std::string &fileName=returnList.at(index);
        if(regex_search(fileName,datapack_rightFileName) && !regex_search(fileName,excludePath))
        {
            const std::string &suffix=CatchChallenger::FacilityLibGeneral::getSuffix(fileName);
            if(!suffix.empty() && extensionAllowed.find(suffix)!=extensionAllowed.cend())
            {
                std::string fullPathFileToOpen=datapackPath+fileName;
                #ifdef Q_OS_WIN32
                stringreplaceAll(fullPathFileToOpen,"/","\\");
                #endif
                FILE *file=fopen(fullPathFileToOpen.c_str(),"rb");
                fullDatapackChecksumReturn.datapackFilesList.push_back(returnList.at(index));
                if(file!=NULL)
                {
                    const std::vector<char> &data=CatchChallenger::FacilityLibGeneral::readAllFileAndClose(file);
                    std::vector<char> hashResult;
                    hashResult.resize(CATCHCHALLENGER_HASH_SIZE);
                    {
                        CatchChallenger::Hash oneshot;
                        oneshot.update(reinterpret_cast<const unsigned char *>(data.data()),data.size());
                        oneshot.final(reinterpret_cast<unsigned char *>(hashResult.data()));
                    }
                    fullDatapackChecksumReturn.partialHashList.push_back(*reinterpret_cast<const int *>(hashResult.data()));
                }
                else
                {
                    fullDatapackChecksumReturn.partialHashList.push_back(0);
                    std::cerr << "Unable to open the file to do the checksum: " << datapackPath << returnList.at(index) << std::endl;
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
    CatchChallenger::Hash hash;
    {
        const std::vector<std::string> &extensionAllowedList=stringsplit(CATCHCHALLENGER_EXTENSION_ALLOWED,';');
        const std::unordered_set<std::string> extensionAllowed(extensionAllowedList.cbegin(),extensionAllowedList.cend());
        std::regex datapack_rightFileName=std::regex(DATAPACK_FILE_REGEX);
        std::vector<std::string> returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath);
        std::sort(returnList.begin(),returnList.end());
        unsigned int index=0;
        while(index<returnList.size())
        {
            const std::string &fileName=returnList.at(index);
            if(regex_search(fileName,datapack_rightFileName))
            {
                const std::string &suffix=CatchChallenger::FacilityLibGeneral::getSuffix(fileName);
                if(!suffix.empty() && extensionAllowed.find(suffix)!=extensionAllowed.cend())
                {
                    std::string fullPathFileToOpen=datapackPath+fileName;
                    #ifdef Q_OS_WIN32
                    stringreplaceAll(fullPathFileToOpen,"/","\\");
                    #endif
                    FILE *file=fopen(fullPathFileToOpen.c_str(),"rb");
                    if(file!=NULL)
                    {
                        const std::vector<char> &data=CatchChallenger::FacilityLibGeneral::readAllFileAndClose(file);
                        hash.update(reinterpret_cast<const unsigned char *>(data.data()),data.size());
                    }
                    else
                    {
                        std::cerr << "Unable to open the file to do the checksum: " << datapackPath << returnList.at(index) << std::endl;
                        return std::vector<char>();
                    }
                }
            }
            index++;
        }
    }
    std::vector<char> hashResult;
    {
        hashResult.resize(CATCHCHALLENGER_HASH_SIZE);
        hash.final(reinterpret_cast<unsigned char *>(hashResult.data()));
    }
    return hashResult;
}

#if ! defined(QT_NO_EMIT) && ! defined(CATCHCHALLENGER_SERVER)
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
    unsigned int index=0;
    while(index<returnList.size())
    {
        const std::string &fileName=returnList.at(index);
        if(regex_search(fileName,datapack_rightFileName))
        {
            const std::string &suffix=CatchChallenger::FacilityLibGeneral::getSuffix(fileName);
            if(!suffix.empty() && extensionAllowed.find(suffix)!=extensionAllowed.cend())
            {
                std::string fullPathFileToOpen=datapackPath+fileName;
                #ifdef Q_OS_WIN32
                stringreplaceAll(fullPathFileToOpen,"/","\\");
                #endif
                FILE *file=fopen(fullPathFileToOpen.c_str(),"rb");
                fullDatapackChecksumReturn.datapackFilesList.push_back(returnList.at(index));
                if(file!=NULL)
                {
                    const std::vector<char> &data=CatchChallenger::FacilityLibGeneral::readAllFileAndClose(file);
                    std::vector<char> hashResult;
                    hashResult.resize(CATCHCHALLENGER_HASH_SIZE);
                    {
                        CatchChallenger::Hash oneshot;
                        oneshot.update(reinterpret_cast<const unsigned char *>(data.data()),data.size());
                        oneshot.final(reinterpret_cast<unsigned char *>(hashResult.data()));
                    }
                    fullDatapackChecksumReturn.partialHashList.push_back(*reinterpret_cast<const int *>(hashResult.data()));
                }
                else
                {
                    fullDatapackChecksumReturn.partialHashList.push_back(0);
                    std::cerr << "Unable to open the file to do the checksum: " << datapackPath << returnList.at(index) << std::endl;
                }
            }
        }
        index++;
    }
    fullDatapackChecksumReturn.hash=doChecksumSub(datapackPath);
    return fullDatapackChecksumReturn;
}
