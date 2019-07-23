#include "QtDatapackChecksum.h"

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
QThread QtDatapackChecksum::thread;
#endif

QtDatapackChecksum::QtDatapackChecksum()
{
    #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER) && !defined(NOTHREADS)
    if(!thread.isRunning())
        thread.start();
    moveToThread(&thread);
    thread.setObjectName("QtDatapackChecksum");
    #endif
}

QtDatapackChecksum::~QtDatapackChecksum()
{
    #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER) && !defined(NOTHREADS)
    stopThread();
    #endif
}

#if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER) && !defined(NOTHREADS)
void QtDatapackChecksum::stopThread()
{
    thread.exit();
    thread.quit();
    thread.wait();
}
#endif

std::vector<char> QtDatapackChecksum::doChecksumBase(const std::string &datapackPath)
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
void QtDatapackChecksum::doDifferedChecksumBase(const std::string &datapackPath)
{
    std::cerr << "QtDatapackChecksum::doDifferedChecksumBase" << std::endl;
    const FullQtDatapackChecksumReturn &fullQtDatapackChecksumReturn=doFullSyncChecksumBase(datapackPath);
    emit datapackChecksumDoneBase(fullQtDatapackChecksumReturn.datapackFilesList,fullQtDatapackChecksumReturn.hash,fullQtDatapackChecksumReturn.partialHashList);
}
#endif

QtDatapackChecksum::FullQtDatapackChecksumReturn QtDatapackChecksum::doFullSyncChecksumBase(const std::string &datapackPath)
{
    QtDatapackChecksum::FullQtDatapackChecksumReturn fullQtDatapackChecksumReturn;
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
                fullQtDatapackChecksumReturn.datapackFilesList.push_back(returnList.at(index));
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
                    fullQtDatapackChecksumReturn.partialHashList.push_back(*reinterpret_cast<const int *>(hashResult.data()));
                }
                else
                {
                    fullQtDatapackChecksumReturn.partialHashList.push_back(0);
                    std::cerr << "Unable to open the file to do the checksum: " << datapackPath << returnList.at(index) << std::endl;
                }
            }
        }
        index++;
    }
    fullQtDatapackChecksumReturn.hash=doChecksumBase(datapackPath);
    return fullQtDatapackChecksumReturn;
}

std::vector<char> QtDatapackChecksum::doChecksumMain(const std::string &datapackPath)
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
void QtDatapackChecksum::doDifferedChecksumMain(const std::string &datapackPath)
{
    const FullQtDatapackChecksumReturn &fullQtDatapackChecksumReturn=doFullSyncChecksumMain(datapackPath);
    emit datapackChecksumDoneMain(fullQtDatapackChecksumReturn.datapackFilesList,fullQtDatapackChecksumReturn.hash,fullQtDatapackChecksumReturn.partialHashList);
}
#endif

QtDatapackChecksum::FullQtDatapackChecksumReturn QtDatapackChecksum::doFullSyncChecksumMain(const std::string &datapackPath)
{
    QtDatapackChecksum::FullQtDatapackChecksumReturn fullQtDatapackChecksumReturn;
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
                fullQtDatapackChecksumReturn.datapackFilesList.push_back(returnList.at(index));
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
                    fullQtDatapackChecksumReturn.partialHashList.push_back(*reinterpret_cast<const int *>(hashResult.data()));
                }
                else
                {
                    fullQtDatapackChecksumReturn.partialHashList.push_back(0);
                    std::cerr << "Unable to open the file to do the checksum: " << datapackPath << returnList.at(index) << std::endl;
                }
            }
        }
        index++;
    }
    fullQtDatapackChecksumReturn.hash=doChecksumMain(datapackPath);
    return fullQtDatapackChecksumReturn;
}

std::vector<char> QtDatapackChecksum::doChecksumSub(const std::string &datapackPath)
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
void QtDatapackChecksum::doDifferedChecksumSub(const std::string &datapackPath)
{
    const FullQtDatapackChecksumReturn &fullQtDatapackChecksumReturn=doFullSyncChecksumSub(datapackPath);
    emit datapackChecksumDoneSub(fullQtDatapackChecksumReturn.datapackFilesList,fullQtDatapackChecksumReturn.hash,fullQtDatapackChecksumReturn.partialHashList);
}
#endif

QtDatapackChecksum::FullQtDatapackChecksumReturn QtDatapackChecksum::doFullSyncChecksumSub(const std::string &datapackPath)
{
    QtDatapackChecksum::FullQtDatapackChecksumReturn fullQtDatapackChecksumReturn;
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
                fullQtDatapackChecksumReturn.datapackFilesList.push_back(returnList.at(index));
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
                    fullQtDatapackChecksumReturn.partialHashList.push_back(*reinterpret_cast<const int *>(hashResult.data()));
                }
                else
                {
                    fullQtDatapackChecksumReturn.partialHashList.push_back(0);
                    std::cerr << "Unable to open the file to do the checksum: " << datapackPath << returnList.at(index) << std::endl;
                }
            }
        }
        index++;
    }
    fullQtDatapackChecksumReturn.hash=doChecksumSub(datapackPath);
    return fullQtDatapackChecksumReturn;
}
