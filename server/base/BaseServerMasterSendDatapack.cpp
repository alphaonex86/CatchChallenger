#include "BaseServerMasterSendDatapack.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/GeneralVariable.h"
#include "../VariableServer.h"

#include <QDebug>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <iostream>

using namespace CatchChallenger;

QHash<QString,quint8> BaseServerMasterSendDatapack::skinList;

QSet<QString> BaseServerMasterSendDatapack::compressedExtension;
QSet<QString> BaseServerMasterSendDatapack::extensionAllowed;

QByteArray BaseServerMasterSendDatapack::rawFilesBuffer;
QByteArray BaseServerMasterSendDatapack::compressedFilesBuffer;
int BaseServerMasterSendDatapack::rawFilesBufferCount;
int BaseServerMasterSendDatapack::compressedFilesBufferCount;

QHash<QString,quint32> BaseServerMasterSendDatapack::datapack_file_list_cache;
QHash<QString,BaseServerMasterSendDatapack::DatapackCacheFile> BaseServerMasterSendDatapack::datapack_file_hash_cache;
QRegularExpression BaseServerMasterSendDatapack::fileNameStartStringRegex=QRegularExpression(QLatin1String("^[a-zA-Z]:/"));

BaseServerMasterSendDatapack::BaseServerMasterSendDatapack()
{
}

BaseServerMasterSendDatapack::~BaseServerMasterSendDatapack()
{
}

void BaseServerMasterSendDatapack::load(const QString &datapack_basePath)
{
    this->datapack_basePathLogin=datapack_basePath;
    preload_the_skin();
}

void BaseServerMasterSendDatapack::preload_the_skin()
{
    const QStringList &skinFolderList=FacilityLibGeneral::skinIdList(datapack_basePathLogin+DATAPACK_BASE_PATH_SKIN);
    int index=0;
    const int &listsize=skinFolderList.size();
    while(index<listsize)
    {
        skinList[skinFolderList.at(index)]=index;
        index++;
    }
    loadTheDatapackFileList();
}

void BaseServerMasterSendDatapack::loadTheDatapackFileList()
{
    QStringList extensionAllowedTemp=(QString(CATCHCHALLENGER_EXTENSION_ALLOWED)+QString(";")+QString(CATCHCHALLENGER_EXTENSION_COMPRESSED)).split(";");
    extensionAllowed=extensionAllowedTemp.toSet();
    QStringList compressedExtensionAllowedTemp=QString(CATCHCHALLENGER_EXTENSION_COMPRESSED).split(";");
    compressedExtension=compressedExtensionAllowedTemp.toSet();
    QRegularExpression datapack_rightFileName = QRegularExpression(DATAPACK_FILE_REGEX);

    QString text_datapack(datapack_basePathLogin);
    QString text_exclude("map/main/");

    QCryptographicHash baseHash(QCryptographicHash::Sha224);
    QStringList datapack_file_temp=FacilityLibGeneral::listFolder(text_datapack);
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
