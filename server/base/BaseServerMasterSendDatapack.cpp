#include "BaseServerMasterSendDatapack.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/GeneralVariable.h"
#include "../VariableServer.h"
#include "../../general/base/cpp11addition.h"

#include <regex>
#include <iostream>
#include <openssl/sha.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace CatchChallenger;

std::unordered_map<std::string,uint8_t> BaseServerMasterSendDatapack::skinList;

std::unordered_set<std::string> BaseServerMasterSendDatapack::compressedExtension;
std::unordered_set<std::string> BaseServerMasterSendDatapack::extensionAllowed;

std::vector<char> BaseServerMasterSendDatapack::rawFilesBuffer;
std::vector<char> BaseServerMasterSendDatapack::compressedFilesBuffer;
int BaseServerMasterSendDatapack::rawFilesBufferCount;
int BaseServerMasterSendDatapack::compressedFilesBufferCount;

std::unordered_map<std::string,uint32_t> BaseServerMasterSendDatapack::datapack_file_list_cache;
std::unordered_map<std::string,BaseServerMasterSendDatapack::DatapackCacheFile> BaseServerMasterSendDatapack::datapack_file_hash_cache;
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
    const unsigned int &listsize=skinFolderList.size();
    while(index<listsize)
    {
        skinList[skinFolderList.at(index)]=index;
        index++;
    }
    loadTheDatapackFileList();
}

void BaseServerMasterSendDatapack::loadTheDatapackFileList()
{
    char tempBigBufferForOutput[CATCHCHALLENGER_SHA224HASH_SIZE];

    std::vector<std::string> extensionAllowedTemp=stringsplit(std::string(CATCHCHALLENGER_EXTENSION_ALLOWED+std::string(";")+CATCHCHALLENGER_EXTENSION_COMPRESSED),';');
    extensionAllowed=std::unordered_set<std::string>(extensionAllowedTemp.begin(),extensionAllowedTemp.end());
    std::vector<std::string> compressedExtensionAllowedTemp=stringsplit(std::string(CATCHCHALLENGER_EXTENSION_COMPRESSED),';');
    compressedExtension=std::unordered_set<std::string>(compressedExtensionAllowedTemp.begin(),compressedExtensionAllowedTemp.end());
    std::regex datapack_rightFileName = std::regex(DATAPACK_FILE_REGEX);

    std::string text_datapack(datapack_basePathLogin);
    std::string text_exclude("map/main/");

    SHA256_CTX hashBase;
    if(SHA224_Init(&hashBase)!=1)
    {
        std::cerr << "SHA224_Init(&hashBase)!=1" << std::endl;
        abort();
    }
    std::vector<std::string> datapack_file_temp=FacilityLibGeneral::listFolder(text_datapack);
    std::sort(datapack_file_temp.begin(),datapack_file_temp.end());

    unsigned int index=0;
    while(index<datapack_file_temp.size()) {
        if(regex_search(datapack_file_temp.at(index),datapack_rightFileName))
        {
            if(!stringStartWith(datapack_file_temp.at(index),text_exclude))
            {
                struct stat buf;
                if(stat((text_datapack+datapack_file_temp.at(index)).c_str(),&buf)==0)
                {
                    if(buf.st_size<=CATCHCHALLENGER_MAX_FILE_SIZE)
                    {
                        FILE *filedesc = fopen((text_datapack+datapack_file_temp.at(index)).c_str(), "rb");
                        if(filedesc!=NULL)
                        {
                            const std::vector<char> &data=FacilityLibGeneral::readAllFileAndClose(filedesc);
                            {
                                SHA256_CTX hashFile;
                                if(SHA224_Init(&hashFile)!=1)
                                {
                                    std::cerr << "SHA224_Init(&hashFile)!=1" << std::endl;
                                    abort();
                                }
                                SHA224_Update(&hashFile,data.data(),data.size());
                                BaseServerMasterSendDatapack::DatapackCacheFile cacheFile;
                                cacheFile.mtime=buf.st_mtime;
                                SHA224_Final(reinterpret_cast<unsigned char *>(tempBigBufferForOutput),&hashFile);
                                cacheFile.partialHash=*reinterpret_cast<const uint32_t *>(tempBigBufferForOutput);
                                datapack_file_hash_cache[datapack_file_temp.at(index)]=cacheFile;
                            }
                            SHA224_Update(&hashBase,data.data(),data.size());
                        }
                        else
                            std::cerr << "Stop now! Unable to open the file " << text_datapack+datapack_file_temp.at(index) << " to do the datapack checksum for the mirror" << std::endl;
                    }
                    else
                        std::cerr << "File to big: " << text_datapack+datapack_file_temp.at(index) << " size: " << buf.st_size << std::endl;
                }
                else
                    std::cerr << "Unable to stat the file: " << text_datapack+datapack_file_temp.at(index) << std::endl;
            }
        }
        else
            std::cerr << "File excluded because don't match the regex (4): " << datapack_file_temp.at(index) << std::endl;
        index++;
    }

    CommonSettingsCommon::commonSettingsCommon.datapackHashBase.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
    SHA224_Final(reinterpret_cast<unsigned char *>(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.data()),&hashBase);

    std::cout << datapack_file_temp.size() << " files for datapack loaded" << std::endl;
}

void BaseServerMasterSendDatapack::unload()
{
    skinList.clear();

    compressedExtension.clear();
    extensionAllowed.clear();
    rawFilesBuffer.clear();
    compressedFilesBuffer.clear();
    datapack_file_list_cache.clear();
    datapack_file_hash_cache.clear();
}
