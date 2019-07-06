#include "DatapackChecksum.h"

#include <regex>
#include <string>
#include <unordered_set>
#include <iostream>
#if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
//with Qt client
#include <QCryptographicHash>
#else
//with gateway
#include <openssl/sha.h>
#endif

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/CommonSettingsServer.h"

using namespace CatchChallenger;

#if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER) && !defined(NOTHREADS)
QThread DatapackChecksum::thread;
#endif

DatapackChecksum::DatapackChecksum()
{
    #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER) && !defined(NOTHREADS)
    if(!thread.isRunning())
        thread.start();
    moveToThread(&thread);
    thread.setObjectName("DatapackChecksum");
    #endif
}

DatapackChecksum::~DatapackChecksum()
{
    #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER) && !defined(NOTHREADS)
    stopThread();
    #endif
}

#if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER) && !defined(NOTHREADS)
void DatapackChecksum::stopThread()
{
    thread.exit();
    thread.quit();
    thread.wait();
}
#endif

std::vector<char> DatapackChecksum::doChecksumBase(const std::string &datapackPath)
{
    #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
    QCryptographicHash hash(QCryptographicHash::Sha224);
    #else
    SHA256_CTX hash;
    if(SHA224_Init(&hash)!=1)
    {
        std::cerr << "SHA224_Init(&hash)!=1" << std::endl;
        abort();
    }
    #endif
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
                        #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
                        hash.addData(QByteArray(reinterpret_cast<const char *>(data.data()),data.size()));
                        #else
                        SHA224_Update(&hash,data.data(),data.size());
                        #endif
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
        hashResult.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
        #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
        memcpy(hashResult.data(),hash.result().constData(),hashResult.size());
        #else
        SHA224_Final(reinterpret_cast<unsigned char *>(hashResult.data()),&hash);
        #endif
    }
    return hashResult;
}

#if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
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
                    hashResult.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
                    #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
                    memcpy(reinterpret_cast<unsigned char *>(hashResult.data()),
                        QCryptographicHash::hash(QByteArray(reinterpret_cast<const char *>(data.data()),data.size()),QCryptographicHash::Sha224).constData(),
                        hashResult.size());
                    #else
                    SHA224(reinterpret_cast<const unsigned char *>(data.data()),data.size(),reinterpret_cast<unsigned char *>(hashResult.data()));
                    #endif
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
    #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
    QCryptographicHash hash(QCryptographicHash::Sha224);
    #else
    SHA256_CTX hash;
    if(SHA224_Init(&hash)!=1)
    {
        std::cerr << "SHA224_Init(&hash)!=1" << std::endl;
        abort();
    }
    #endif
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
                        #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
                        hash.addData(QByteArray(reinterpret_cast<const char *>(data.data()),data.size()));
                        #else
                        SHA224_Update(&hash,data.data(),data.size());
                        #endif
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
        hashResult.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
        #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
        memcpy(hashResult.data(),hash.result().constData(),hashResult.size());
        #else
        SHA224_Final(reinterpret_cast<unsigned char *>(hashResult.data()),&hash);
        #endif
    }
    return hashResult;
}

#if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
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
                    hashResult.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
                    #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
                    memcpy(reinterpret_cast<unsigned char *>(hashResult.data()),
                        QCryptographicHash::hash(QByteArray(reinterpret_cast<const char *>(data.data()),data.size()),QCryptographicHash::Sha224).constData(),
                        hashResult.size());
                    #else
                    SHA224(reinterpret_cast<const unsigned char *>(data.data()),data.size(),reinterpret_cast<unsigned char *>(hashResult.data()));
                    #endif
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
    #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
    QCryptographicHash hash(QCryptographicHash::Sha224);
    #else
    SHA256_CTX hash;
    if(SHA224_Init(&hash)!=1)
    {
        std::cerr << "SHA224_Init(&hash)!=1" << std::endl;
        abort();
    }
    #endif
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
                        #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
                        hash.addData(QByteArray(reinterpret_cast<const char *>(data.data()),data.size()));
                        #else
                        SHA224_Update(&hash,data.data(),data.size());
                        #endif
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
        hashResult.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
        #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
        memcpy(hashResult.data(),hash.result().constData(),hashResult.size());
        #else
        SHA224_Final(reinterpret_cast<unsigned char *>(hashResult.data()),&hash);
        #endif
    }
    return hashResult;
}

#if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
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
                    hashResult.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
                    #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
                    memcpy(reinterpret_cast<unsigned char *>(hashResult.data()),
                        QCryptographicHash::hash(QByteArray(reinterpret_cast<const char *>(data.data()),data.size()),QCryptographicHash::Sha224).constData(),
                        hashResult.size());
                    #else
                    SHA224(reinterpret_cast<const unsigned char *>(data.data()),data.size(),reinterpret_cast<unsigned char *>(hashResult.data()));
                    #endif
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
