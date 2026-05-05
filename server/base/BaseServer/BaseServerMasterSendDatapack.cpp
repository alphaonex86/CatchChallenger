#include "BaseServerMasterSendDatapack.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/GeneralVariable.hpp"
#include "../VariableServer.hpp"
#include "../../general/base/cpp11addition.hpp"
#ifndef CATCHCHALLENGER_NOXML
#include "../../general/base/CatchChallenger_Hash.hpp"
#include <xxhash.h>
#endif

#include <regex>
#include <iostream>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

using namespace CatchChallenger;

catchchallenger_datapack_map<std::string,uint8_t> BaseServerMasterSendDatapack::skinList;

#ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
#ifndef CATCHCHALLENGER_SERVER_NO_COMPRESSION
catchchallenger_datapack_set<std::string> BaseServerMasterSendDatapack::compressedExtension;
std::vector<BaseServerMasterSendDatapack::PendingFile> BaseServerMasterSendDatapack::compressedFilesPending;
uint64_t BaseServerMasterSendDatapack::compressedFilesPendingRawSize=0;
#endif
catchchallenger_datapack_set<std::string> BaseServerMasterSendDatapack::extensionAllowed;

std::vector<BaseServerMasterSendDatapack::PendingFile> BaseServerMasterSendDatapack::rawFilesPending;
uint64_t BaseServerMasterSendDatapack::rawFilesPendingRawSize=0;

catchchallenger_datapack_map<std::string,uint32_t> BaseServerMasterSendDatapack::datapack_file_list_cache;
catchchallenger_datapack_map<std::string,BaseServerMasterSendDatapack::DatapackCacheFile> BaseServerMasterSendDatapack::datapack_file_hash_cache_base;
#endif

#ifndef CATCHCHALLENGER_NOXML
std::regex BaseServerMasterSendDatapack::fileNameStartStringRegex=std::regex("^[a-zA-Z]:/");
#endif

BaseServerMasterSendDatapack::BaseServerMasterSendDatapack()
{
}

BaseServerMasterSendDatapack::~BaseServerMasterSendDatapack()
{
}

#ifndef CATCHCHALLENGER_NOXML
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
    //char tempBigBufferForOutput[CATCHCHALLENGER_HASH_SIZE];
    #endif

    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
        std::vector<std::string> extensionAllowedTemp=stringsplit(std::string(CATCHCHALLENGER_EXTENSION_ALLOWED+std::string(";")+CATCHCHALLENGER_EXTENSION_COMPRESSED),';');
        extensionAllowed=catchchallenger_datapack_set<std::string>(extensionAllowedTemp.begin(),extensionAllowedTemp.end());
        #ifndef CATCHCHALLENGER_SERVER_NO_COMPRESSION
        std::vector<std::string> compressedExtensionAllowedTemp=stringsplit(std::string(CATCHCHALLENGER_EXTENSION_COMPRESSED),';');
        compressedExtension=catchchallenger_datapack_set<std::string>(compressedExtensionAllowedTemp.begin(),compressedExtensionAllowedTemp.end());
        #endif
    #else
        std::vector<std::string> extensionAllowedTemp=stringsplit(std::string(CATCHCHALLENGER_EXTENSION_ALLOWED+std::string(";")+CATCHCHALLENGER_EXTENSION_COMPRESSED),';');
        std::unordered_set<std::string> extensionAllowed=std::unordered_set<std::string>(extensionAllowedTemp.begin(),extensionAllowedTemp.end());
    #endif
    std::regex datapack_rightFileName = std::regex(DATAPACK_FILE_REGEX);

    std::string text_datapack(datapack_basePathLogin);
    std::string text_exclude("map/main/");

    CatchChallenger::Hash hashBase;
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
                                //from hash to xxhash
                                BaseServerMasterSendDatapack::DatapackCacheFile cacheFile;
                                XXH32_canonical_t htemp;
                                XXH32_canonicalFromHash(&htemp,XXH32(data.data(),data.size(),0));
                                memcpy(&cacheFile.partialHash,&htemp.digest,sizeof(cacheFile.partialHash));
                                datapack_file_hash_cache_base[datapack_file_temp.at(index)]=cacheFile;
                             /*   Hash hashFile;
                                hashFile.update(data.data(),data.size());
                                BaseServerMasterSendDatapack::DatapackCacheFile cacheFile;
                                //cacheFile.mtime=buf.st_mtime;
                                hashFile.final(reinterpret_cast<unsigned char *>(tempBigBufferForOutput));
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

    CommonSettingsCommon::commonSettingsCommon.datapackHashBase.resize(CATCHCHALLENGER_HASH_SIZE);
    hashBase.final(reinterpret_cast<unsigned char *>(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.data()));

    std::cout << datapack_file_temp.size() << " files for datapack loaded" << std::endl;
}
#endif

void BaseServerMasterSendDatapack::unload()
{
    skinList.clear();

    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    #ifndef CATCHCHALLENGER_SERVER_NO_COMPRESSION
    compressedExtension.clear();
    //Free the pending list. swap-with-empty actually releases the storage,
    //unlike .clear() which keeps capacity. Matches the "free when finished"
    //policy the user asked for.
    std::vector<PendingFile>().swap(compressedFilesPending);
    compressedFilesPendingRawSize=0;
    #endif
    extensionAllowed.clear();
    std::vector<PendingFile>().swap(rawFilesPending);
    rawFilesPendingRawSize=0;
    datapack_file_list_cache.clear();
    datapack_file_hash_cache_base.clear();
    #endif
}
