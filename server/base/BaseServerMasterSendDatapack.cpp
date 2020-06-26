#include "BaseServerMasterSendDatapack.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/GeneralVariable.hpp"
#include "../VariableServer.hpp"
#include "../../general/base/cpp11addition.hpp"
#include "../../general/sha224/sha224.hpp"
#include "../../general/xxhash/xxhash.h"

#include <regex>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

using namespace CatchChallenger;

std::unordered_map<std::string,uint8_t> BaseServerMasterSendDatapack::skinList;

#ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
std::unordered_set<std::string> BaseServerMasterSendDatapack::compressedExtension;
std::vector<char> BaseServerMasterSendDatapack::compressedFilesBuffer;
uint8_t BaseServerMasterSendDatapack::compressedFilesBufferCount;
#endif
std::unordered_set<std::string> BaseServerMasterSendDatapack::extensionAllowed;

std::vector<char> BaseServerMasterSendDatapack::rawFilesBuffer;
uint8_t BaseServerMasterSendDatapack::rawFilesBufferCount;

std::unordered_map<std::string,uint32_t> BaseServerMasterSendDatapack::datapack_file_list_cache;
std::unordered_map<std::string,BaseServerMasterSendDatapack::DatapackCacheFile> BaseServerMasterSendDatapack::datapack_file_hash_cache_base;
#endif

std::regex BaseServerMasterSendDatapack::fileNameStartStringRegex=std::regex("^[a-zA-Z]:/");

BaseServerMasterSendDatapack::BaseServerMasterSendDatapack()
{
}

BaseServerMasterSendDatapack::~BaseServerMasterSendDatapack()
{
}

void BaseServerMasterSendDatapack::load(const std::string &datapack_basePath)
{
    this->datapack_basePathLogin=datapack_basePath;
    preload_the_skin();
}

void BaseServerMasterSendDatapack::preload_the_skin()
{
    const std::vector<std::string> &skinFolderList=FacilityLibGeneral::skinIdList(datapack_basePathLogin+DATAPACK_BASE_PATH_SKIN);
    unsigned int index=0;
    while(index<skinFolderList.size())
    {
        skinList[skinFolderList.at(index)]=static_cast<uint8_t>(index);
        index++;
    }
    loadTheDatapackFileList();
}

void BaseServerMasterSendDatapack::loadTheDatapackFileList()
{
    if(!CommonSettingsCommon::commonSettingsCommon.datapackHashBase.empty()
            #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
            && !datapack_file_hash_cache_base.empty()
            #endif
            )
        return;//base already calculated
    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    char tempBigBufferForOutput[CATCHCHALLENGER_SHA224HASH_SIZE];
    #endif

    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
        std::vector<std::string> extensionAllowedTemp=stringsplit(std::string(CATCHCHALLENGER_EXTENSION_ALLOWED+std::string(";")+CATCHCHALLENGER_EXTENSION_COMPRESSED),';');
        extensionAllowed=std::unordered_set<std::string>(extensionAllowedTemp.begin(),extensionAllowedTemp.end());
        #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
        std::vector<std::string> compressedExtensionAllowedTemp=stringsplit(std::string(CATCHCHALLENGER_EXTENSION_COMPRESSED),';');
        compressedExtension=std::unordered_set<std::string>(compressedExtensionAllowedTemp.begin(),compressedExtensionAllowedTemp.end());
        #endif
    #else
        std::vector<std::string> extensionAllowedTemp=stringsplit(std::string(CATCHCHALLENGER_EXTENSION_ALLOWED+std::string(";")+CATCHCHALLENGER_EXTENSION_COMPRESSED),';');
        std::unordered_set<std::string> extensionAllowed=std::unordered_set<std::string>(extensionAllowedTemp.begin(),extensionAllowedTemp.end());
    #endif
    std::regex datapack_rightFileName = std::regex(DATAPACK_FILE_REGEX);

    std::string text_datapack(datapack_basePathLogin);
    std::string text_exclude("map/main/");

    SHA224 hashBase;
    hashBase.init();
    std::vector<std::string> datapack_file_temp=FacilityLibGeneral::listFolder(text_datapack);
    std::sort(datapack_file_temp.begin(),datapack_file_temp.end());

    unsigned int index=0;
    while(index<datapack_file_temp.size()) {
        if(regex_search(datapack_file_temp.at(index),datapack_rightFileName) && extensionAllowed.find(FacilityLibGeneral::getSuffix(datapack_file_temp.at(index)))!=extensionAllowed.cend())
        {
            if(!stringStartWith(datapack_file_temp.at(index),text_exclude))
            {
                struct stat buf;
                if(stat((text_datapack+datapack_file_temp.at(index)).c_str(),&buf)==0)
                {
                    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
                    if(buf.st_size<=CATCHCHALLENGER_MAX_FILE_SIZE)
                    #endif
                    {
                        std::string fullPathFileToOpen=text_datapack+datapack_file_temp.at(index);
                        #ifdef Q_OS_WIN32
                        stringreplaceAll(fullPathFileToOpen,"/","\\");
                        #endif
                        FILE *filedesc=fopen(fullPathFileToOpen.c_str(),"rb");
                        if(filedesc!=NULL)
                        {
                            const std::vector<char> &data=FacilityLibGeneral::readAllFileAndClose(filedesc);
                            if(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.empty())
                            {
                                #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
                                //from sha224 to xxhash
                                BaseServerMasterSendDatapack::DatapackCacheFile cacheFile;
                                cacheFile.partialHash=XXH32(data.data(),data.size(),0);
                                datapack_file_hash_cache_base[datapack_file_temp.at(index)]=cacheFile;
                             /*   SHA224 hashFile;
                                hashFile.init();
                                SHA224_Update(&hashFile,data.data(),data.size());
                                BaseServerMasterSendDatapack::DatapackCacheFile cacheFile;
                                //cacheFile.mtime=buf.st_mtime;
                                SHA224_Final(reinterpret_cast<unsigned char *>(tempBigBufferForOutput),&hashFile);
                                ::memcpy(&cacheFile.partialHash,tempBigBufferForOutput,sizeof(uint32_t));
                                datapack_file_hash_cache_base[datapack_file_temp.at(index)]=cacheFile;
                                */
                                #endif
                            }
                            hashBase.update(reinterpret_cast<const unsigned char *>(data.data()),data.size());
                        }
                        #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
                        else
                        {
                            std::cerr << "Stop now! Unable to open the file " << text_datapack+datapack_file_temp.at(index) << " to do the datapack checksum for the mirror" << std::endl;
                            abort();
                        }
                        #endif
                    }
                    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
                    else
                        std::cerr << "File too big: " << text_datapack+datapack_file_temp.at(index) << " size: " << buf.st_size << ">CATCHCHALLENGER_MAX_FILE_SIZE: " << CATCHCHALLENGER_MAX_FILE_SIZE << std::endl;
                    #endif
                }
                else
                {
                    std::cerr << "Unable to stat the file: " << text_datapack+datapack_file_temp.at(index) << std::endl;
                    abort();
                }
            }
        }
        else
            std::cerr << "File excluded because don't match the regex (4): " << datapack_file_temp.at(index) << std::endl;
        index++;
    }

    CommonSettingsCommon::commonSettingsCommon.datapackHashBase.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
    hashBase.final(reinterpret_cast<unsigned char *>(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.data()));

    std::cout << datapack_file_temp.size() << " files for datapack loaded" << std::endl;
}

void BaseServerMasterSendDatapack::unload()
{
    skinList.clear();

    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    compressedExtension.clear();
    compressedFilesBuffer.clear();
    #endif
    extensionAllowed.clear();
    rawFilesBuffer.clear();
    datapack_file_list_cache.clear();
    datapack_file_hash_cache_base.clear();
    #endif
}
