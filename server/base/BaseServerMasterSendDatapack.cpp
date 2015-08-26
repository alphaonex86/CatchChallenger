#include "BaseServerMasterSendDatapack.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/GeneralVariable.h"
#include "../VariableServer.h"

#include <QCryptographicHash>
#include <regex>
#include <iostream>

using namespace CatchChallenger;

std::unordered_map<std::string,quint8> BaseServerMasterSendDatapack::skinList;

std::unordered_set<std::string> BaseServerMasterSendDatapack::compressedExtension;
std::unordered_set<std::string> BaseServerMasterSendDatapack::extensionAllowed;

QByteArray BaseServerMasterSendDatapack::rawFilesBuffer;
QByteArray BaseServerMasterSendDatapack::compressedFilesBuffer;
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
    std::vector<std::string> extensionAllowedTemp=(std::string(CATCHCHALLENGER_EXTENSION_ALLOWED)+std::string(";")+std::string(CATCHCHALLENGER_EXTENSION_COMPRESSED)).split(";");
    extensionAllowed=extensionAllowedTemp.toSet();
    std::vector<std::string> compressedExtensionAllowedTemp=std::string(CATCHCHALLENGER_EXTENSION_COMPRESSED).split(";");
    compressedExtension=compressedExtensionAllowedTemp.toSet();
    std::regex datapack_rightFileName = std::regex(DATAPACK_FILE_REGEX);

    std::string text_datapack(datapack_basePathLogin);
    std::string text_exclude("map/main/");

    QCryptographicHash baseHash(QCryptographicHash::Sha224);
    std::vector<std::string> datapack_file_temp=FacilityLibGeneral::listFolder(text_datapack);
    datapack_file_temp.sort();

    int index=0;
    while(index<datapack_file_temp.size()) {
        QFile file(text_datapack+datapack_file_temp.at(index));
        if(datapack_file_temp.at(index).contains(datapack_rightFileName))
        {
            if(file.size()<=8*1024*1024)
            {
                if(!datapack_file_temp.at(index).startsWith(text_exclude))
                {
                    if(file.open(QIODevice::ReadOnly))
                    {
                        const QByteArray &data=file.readAll();
                        {
                            QCryptographicHash hashFile(QCryptographicHash::Sha224);
                            hashFile.addData(data);
                            BaseServerMasterSendDatapack::DatapackCacheFile cacheFile;
                            cacheFile.mtime=QFileInfo(file).lastModified().toTime_t();
                            cacheFile.partialHash=*reinterpret_cast<const int *>(hashFile.result().constData());
                            datapack_file_hash_cache[datapack_file_temp.at(index)]=cacheFile;
                        }
                        baseHash.addData(data);
                        file.close();
                    }
                    else
                    {
                        std::cerr << "Stop now! Unable to open the file " << file.fileName().toStdString() << " to do the datapack checksum for the mirror" << std::endl;
                        abort();
                    }
                }
            }
            else
                std::cerr << "File to big: " << datapack_file_temp.at(index).toStdString() << " size: " << file.size() << std::endl;
        }
        else
            std::cerr << "File excluded because don't match the regex: " << file.fileName().toStdString() << std::endl;
        index++;
    }

    CommonSettingsCommon::commonSettingsCommon.datapackHashBase=baseHash.result();
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
