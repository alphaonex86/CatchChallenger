#include "DatapackChecksum.hpp"

#include <regex>
#include <string>
#include <unordered_set>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <utime.h>

#include "../../general/base/GeneralVariable.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/sha224/sha224.hpp"

using namespace CatchChallenger;

DatapackChecksum::DatapackChecksum()
{
}

DatapackChecksum::~DatapackChecksum()
{
}

std::vector<char> DatapackChecksum::doChecksumBase(const std::string &datapackPath)
{
    SHA224 hash = SHA224();
    hash.init();
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
                    #ifdef __WIN32__
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
        hashResult.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
        hash.final(reinterpret_cast<unsigned char *>(hashResult.data()));
    }
    return hashResult;
}

DatapackChecksum::FullDatapackChecksumReturn DatapackChecksum::doFullSyncChecksumBase(const std::string &datapackPath)
{
    DatapackChecksum::FullDatapackChecksumReturn fullDatapackChecksumReturn;
    std::regex excludePath("^map[/\\\\]main[/\\\\]");
    const std::vector<std::string> &extensionAllowedList=stringsplit(CATCHCHALLENGER_EXTENSION_ALLOWED,';');
    const std::unordered_set<std::string> extensionAllowed(extensionAllowedList.cbegin(),extensionAllowedList.cend());
    std::regex datapack_rightFileName=std::regex(DATAPACK_FILE_REGEX);
    std::vector<std::string> returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath);
    std::sort(returnList.begin(),returnList.end());
    //std::cout << "doFullSyncChecksumBase " << returnList.size() << " for " << datapackPath << " " << __FILE__ << ":" << __LINE__ << std::endl;
    unsigned int index=0;
    while(index<returnList.size())
    {
        //std::cout << "doFullSyncChecksumBase " << __FILE__ << ":" << __LINE__ << std::endl;
        const std::string &fileName=returnList.at(index);
        if(regex_search(fileName,datapack_rightFileName) && !regex_search(fileName,excludePath))
        {
            //std::cout << "doFullSyncChecksumBase " << __FILE__ << ":" << __LINE__ << std::endl;
            const std::string &suffix=CatchChallenger::FacilityLibGeneral::getSuffix(fileName);
            if(!suffix.empty() && extensionAllowed.find(suffix)!=extensionAllowed.cend())
            {
                //std::cout << "doFullSyncChecksumBase " << __FILE__ << ":" << __LINE__ << std::endl;
                std::string fullPathFileToOpen=datapackPath+fileName;
                #ifdef __WIN32__
                stringreplaceAll(fullPathFileToOpen,"/","\\");
                #endif
                fullDatapackChecksumReturn.datapackFilesList.push_back(fileName);
                fullDatapackChecksumReturn.partialHashList.push_back(readCachePartialHash(fullPathFileToOpen));//use sb.st_mtime as big endian xxhash cache
            }
        }
        index++;
    }
    fullDatapackChecksumReturn.hash=doChecksumBase(datapackPath);
    return fullDatapackChecksumReturn;
}

std::vector<char> DatapackChecksum::doChecksumMain(const std::string &datapackPath)
{
    SHA224 hash = SHA224();
    hash.init();
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
                    #ifdef __WIN32__
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
                        std::cerr << "Unable to open the file to do the checksum: " << datapackPath << returnList.at(index) << ", errno: " << errno << std::endl;
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
        hash.final(reinterpret_cast<unsigned char *>(hashResult.data()));
    }
    return hashResult;
}

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
                #ifdef __WIN32__
                stringreplaceAll(fullPathFileToOpen,"/","\\");
                #endif
                fullDatapackChecksumReturn.datapackFilesList.push_back(fileName);
                fullDatapackChecksumReturn.partialHashList.push_back(readCachePartialHash(fullPathFileToOpen));//use sb.st_mtime as big endian xxhash cache
            }
        }
        index++;
    }
    fullDatapackChecksumReturn.hash=doChecksumMain(datapackPath);
    return fullDatapackChecksumReturn;
}

std::vector<char> DatapackChecksum::doChecksumSub(const std::string &datapackPath)
{
    SHA224 hash = SHA224();
    hash.init();
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
                    #ifdef __WIN32__
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
                        std::cerr << "Unable to open the file to do the checksum: " << datapackPath << returnList.at(index) << ", errno: " << errno << std::endl;
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
        hash.final(reinterpret_cast<unsigned char *>(hashResult.data()));
    }
    return hashResult;
}

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
                #ifdef __WIN32__
                stringreplaceAll(fullPathFileToOpen,"/","\\");
                #endif
                fullDatapackChecksumReturn.datapackFilesList.push_back(fileName);
                fullDatapackChecksumReturn.partialHashList.push_back(readCachePartialHash(fullPathFileToOpen));//use sb.st_mtime as big endian xxhash cache
            }
        }
        index++;
    }
    fullDatapackChecksumReturn.hash=doChecksumSub(datapackPath);
    return fullDatapackChecksumReturn;
}

int64_t DatapackChecksum::readFileMDateTime(const std::string &file)
{
    /** Why not do it with Qt? Because it not support setModificationTime(), and get the time with Qt, that's mean use local time where in C is UTC time */
    #ifndef __WIN32__
        struct stat info;
        if(stat(file.c_str(),&info)!=0)
        {
            std::cerr << "Unable to read modification time path: " << file << ", errno: " << std::to_string(errno) << std::endl;
            return -1;
        }
        #ifdef __APPLE__
        return info.st_mtimespec.tv_sec;
        #else
        return info.st_mtim.tv_sec;
        #endif
    #else
        HANDLE hFileSouce = CreateFileW(toFinalPath(file).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
        if(hFileSouce == INVALID_HANDLE_VALUE)
            return -1;
        FILETIME ftCreate, ftAccess, ftWrite;
        if(!GetFileTime(hFileSouce, &ftCreate, &ftAccess, &ftWrite))
        {
            CloseHandle(hFileSouce);
            std::cerr << "Unable to read modification time path: " << file << ", error: " << GetLastErrorStdStr() << std::endl;
            return -1;
        }
        CloseHandle(hFileSouce);
        //const int64_t UNIX_TIME_START = 0x019DB1DED53E8000; //January 1, 1970 (start of Unix epoch) in "ticks"
        //const int64_t TICKS_PER_SECOND = 10000000; //a tick is 100ns
        LARGE_INTEGER li;
        li.LowPart  = ftWrite.dwLowDateTime;
        li.HighPart = ftWrite.dwHighDateTime;
        //return (li.QuadPart - UNIX_TIME_START) / TICKS_PER_SECOND;
        return (li.QuadPart - 0x019DB1DED53E8000) / 10000000;
    #endif
    return -1;
}

uint32_t DatapackChecksum::readCachePartialHash(const std::string &file)
{
    #ifndef Q_OS_MTIMENOTSUPPORTED
    //in case hash is store into modification file time
    return readFileMDateTime(file);
    #else
    QFile file(QString::fromStdString(file));
    if(file.open(QIODevice::ReadOnly))
    {
        QByteArray data=file.readAll();
        uint32_t h=0;
        XXH32_canonical_t htemp;
        XXH32_canonicalFromHash(&htemp,XXH32(data.data(),data.size(),0));
        memcpy(&h,&htemp.digest,sizeof(h));
        file.close();
        return h;
    }
    return 0;
    #endif
}

bool DatapackChecksum::writeFileMDateTime(const std::string &file,const int64_t &date)
{
    /** Why not do it with Qt? Because it not support setModificationTime(), and get the time with Qt, that's mean use local time where in C is UTC time */
    #ifndef __WIN32__
        struct utimbuf butime;
        butime.actime=date;
        butime.modtime=date;
        #ifndef CATCHCHALLENGER_EXTRA_CHECK
        return utime(file.c_str(),&butime)==0;
        #else
        if(utime(file.c_str(),&butime)!=0)
        {
            std::cerr << "hash cache into modification time set failed on path: " << file
                      << ", errno: " << std::to_string(errno)
                      << " (abort) " << __FILE__ << ":" << __LINE__ << std::endl << std::endl;
            return false;
        }
        struct stat sb;
        if (stat(file.c_str(), &sb) == 0)
        {
            if(sb.st_mtime!=date)
            {
                std::cerr << "hash cache into modification time wrong  on path: " << file
                          << ", errno: " << std::to_string(errno)
                          << " (abort) " << __FILE__ << ":" << __LINE__ << std::endl << std::endl;
                return false;
            }
        }
        else
        {
            std::cerr << "unable to open modification time of datapack to check the hash  on path: " << file
                      << ", errno: " << std::to_string(errno)
                      << " (abort) " << __FILE__ << ":" << __LINE__ << std::endl << std::endl;
            return false;
        }
        return true;
        #endif
    #else
        HANDLE hFileDestination = CreateFileW(toFinalPath(file).c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if(hFileDestination == INVALID_HANDLE_VALUE)
        {
            std::cerr << "Unable to read modification (write windows) time path: " << file << ", error: " << GetLastErrorStdStr() << std::endl;
            return false;
        }
        ULARGE_INTEGER time_value;
        time_value.QuadPart = (date * 10000000LL) + 116444736000000000LL;
        pft->dwLowDateTime = time_value.LowPart;
        pft->dwHighDateTime = time_value.HighPart;

        FILETIME ftCreate, ftAccess, ftWrite;
        ftCreate=time_value;
        ftAccess=time_value;
        ftWrite=time_value;
        if(!SetFileTime(hFileDestination, &ftCreate, &ftAccess, &ftWrite))
        {
            CloseHandle(hFileDestination);
            std::cerr << "Unable to read modification time path: " << file << ", error: " << GetLastErrorStdStr() << std::endl;
            return false;
        }
        CloseHandle(hFileDestination);
        return true;
    #endif
    return false;
}

bool DatapackChecksum::writeCachePartialHash(const std::string &file,const uint32_t &hash)
{
    #ifndef Q_OS_MTIMENOTSUPPORTED
    //if can be store into modification time
    return writeFileMDateTime(file,hash);
    #else
    return true;
    #endif
}

#ifdef __WIN32__
std::string DatapackChecksum::GetLastErrorStdStr()
{
  DWORD error = GetLastError();
  if (error)
  {
    LPVOID lpMsgBuf;
    DWORD bufLen = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_SYSTEM_DEFAULT, SUBLANG_DEFAULT),
        (LPSTR) &lpMsgBuf,
        0, NULL );
    if (bufLen)
    {
      std::string result((char *)lpMsgBuf, (int)bufLen);
      LocalFree(lpMsgBuf);
      return result+" ("+std::to_string(error)+")";
    }
    return "1: "+std::to_string(error);
  }
  return "2: "+std::to_string(error);
}

std::string DatapackChecksum::toFinalPath(std::string path)
{
    if(path.size()==2 && path.at(1)==':')
        path+="\\";
    stringreplaceAll(path,"/","\\");
    std::string pathW;
    if(path.size()>2 && path.substr(0,2)=="\\\\")//nas
        pathW="\\\\?\\UNC\\"+path.substr(2);
    else
        pathW="\\\\?\\"+path;
    return pathW;
}
#endif

