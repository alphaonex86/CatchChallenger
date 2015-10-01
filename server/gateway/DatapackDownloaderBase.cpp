#include "DatapackDownloaderBase.h"

using namespace CatchChallenger;

#include <iostream>
#include <cmath>
#include <regex>
#include <QNetworkReply>
#include <QProcess>

#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../client/base/qt-tar-xz/QTarDecode.h"
#include "../../general/base/GeneralVariable.h"
#include "LinkToGameServer.h"
#include "EpollServerLoginSlave.h"

//need host + port here to have datapack base

std::regex DatapackDownloaderBase::regex_DATAPACK_FILE_REGEX(DATAPACK_FILE_REGEX);
std::unordered_set<std::string> DatapackDownloaderBase::extensionAllowed;
std::regex DatapackDownloaderBase::excludePathBase("^(map[/\\\\]main[/\\\\]|pack[/\\\\]|datapack-list[/\\\\])");
std::string DatapackDownloaderBase::commandUpdateDatapackBase;

DatapackDownloaderBase * DatapackDownloaderBase::datapackDownloaderBase=NULL;

DatapackDownloaderBase::DatapackDownloaderBase(const std::string &mDatapackBase) :
    mDatapackBase(mDatapackBase),
    curl(NULL)
{
    datapackTarXzBase=false;
    index_mirror_base=0;
    wait_datapack_content_base=false;
}

DatapackDownloaderBase::~DatapackDownloaderBase()
{
}

void DatapackDownloaderBase::haveTheDatapack()
{
    if(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.empty())
    {
        if(curl!=NULL)
        {
            curl_easy_cleanup(curl);
            curl=NULL;
        }
    }

    unsigned int index=0;
    while(index<clientInSuspend.size())
    {
        LinkToGameServer * const clientLink=static_cast<LinkToGameServer * const>(clientInSuspend.at(index));
        if(clientLink!=NULL)
            clientLink->sendDiffered04Reply();
        index++;
    }
    clientInSuspend.clear();

    //regen the datapack cache
    if(LinkToGameServer::httpDatapackMirrorRewriteBase.size()<=1)
        EpollClientLoginSlave::datapack_file_base.datapack_file_hash_cache=EpollClientLoginSlave::datapack_file_list(mDatapackBase);

    resetAll();

    if(!DatapackDownloaderBase::commandUpdateDatapackBase.empty())
        if(QProcess::execute(DatapackDownloaderBase::commandUpdateDatapackBase,std::vector<std::string>() << mDatapackBase)<0)
            qDebug() << "Unable to execute " << DatapackDownloaderBase::commandUpdateDatapackBase << " " << mDatapackBase;
}

void DatapackDownloaderBase::resetAll()
{
    httpModeBase=false;
    httpError=false;
    query_files_list_base.clear();
    urlInWaitingListBase.clear();
    wait_datapack_content_base=false;
    clientInSuspend.clear();
}

void DatapackDownloaderBase::datapackDownloadError()
{
    unsigned int index=0;
    while(index<clientInSuspend.size())
    {
        LinkToGameServer * const clientLink=static_cast<LinkToGameServer * const>(clientInSuspend.at(index));
        if(clientLink!=NULL)
            clientLink->disconnectClient();
        index++;
    }
    clientInSuspend.clear();
}

void DatapackDownloaderBase::datapackFileList(const char * const data,const unsigned int &size)
{
    if(datapackFilesListBase.isEmpty() && size==1)
    {
        if(!httpModeBase)
            haveTheDatapack();
        return;
    }
    unsigned int pos=0;
    QList<bool> boolList;
    while((size-pos)>0)
    {
        const uint8_t &returnCode=data[pos];
        boolList.append(returnCode&0x01);
        boolList.append(returnCode&0x02);
        boolList.append(returnCode&0x04);
        boolList.append(returnCode&0x08);
        boolList.append(returnCode&0x10);
        boolList.append(returnCode&0x20);
        boolList.append(returnCode&0x40);
        boolList.append(returnCode&0x80);
        pos++;
    }
    if(boolList.size()<datapackFilesListBase.size())
    {
        qDebug() << (std::stringLiteral("bool list too small with DatapackDownloaderBase::datapackFileList"));
        return;
    }
    int index=0;
    while(index<datapackFilesListBase.size())
    {
        if(boolList.first())
        {
            DebugClass::debugConsole(std::stringLiteral("remove the file: %1").arg(mDatapackBase+text_slash+datapackFilesListBase.at(index)));
            QFile file(mDatapackBase+text_slash+datapackFilesListBase.at(index));
            if(!file.remove())
                DebugClass::debugConsole(std::stringLiteral("unable to remove the file: %1: %2").arg(datapackFilesListBase.at(index)).arg(file.errorString()));
            //removeFile(datapackFilesListBase.at(index));
        }
        boolList.removeFirst();
        index++;
    }
    datapackFilesListBase.clear();
    cleanDatapackBase(std::string());
    if(boolList.size()>=8)
    {
        qDebug() << (std::stringLiteral("bool list too big with DatapackDownloaderBase::datapackFileList"));
        return;
    }
    if(!httpModeBase)
        haveTheDatapack();
}

void DatapackDownloaderBase::writeNewFileBase(const std::string &fileName,const QByteArray &data)
{
    const std::string &fullPath=mDatapackBase+text_slash+fileName;
    //to be sure the QFile is destroyed
    {
        QFile file(fullPath);
        QFileInfo fileInfo(file);

        if(!QDir(fileInfo.absolutePath()).mkpath(fileInfo.absolutePath()))
        {
            qDebug() << "unable to make the path: " << fileInfo.absolutePath();
            abort();
        }

        if(file.exists())
            if(!file.remove())
            {
                DebugClass::debugConsole(std::stringLiteral("Can't remove: %1: %2").arg(fileName).arg(file.errorString()));
                return;
            }
        if(!file.open(QIODevice::WriteOnly))
        {
            DebugClass::debugConsole(std::stringLiteral("Can't open: %1: %2").arg(fileName).arg(file.errorString()));
            return;
        }
        if(file.write(data)!=data.size())
        {
            file.close();
            DebugClass::debugConsole(std::stringLiteral("Can't write: %1: %2").arg(fileName).arg(file.errorString()));
            return;
        }
        file.flush();
        file.close();
    }
}

bool DatapackDownloaderBase::getHttpFileBase(const std::string &url, const std::string &fileName)
{
    if(httpError)
        return false;
    if(!httpModeBase)
        httpModeBase=true;

    const std::string &fullPath=mDatapackBase+text_slash+fileName;
    {
        QFile file(fullPath);
        QFileInfo fileInfo(file);

        if(!QDir(fileInfo.absolutePath()).mkpath(fileInfo.absolutePath()))
        {
            qDebug() << "unable to make the path: " << fileInfo.absolutePath();
            abort();
        }
    }

    FILE *fp = fopen(fullPath.toLocal8Bit().constData(),"wb");
    if(fp!=NULL)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &fwrite);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        const CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        fclose(fp);
        if(res!=CURLE_OK || http_code!=200)
        {
            httpError=true;
            qDebug() << (std::stringLiteral("get url %1: %2 failed with code %3").arg(url).arg(res).arg(http_code));
            datapackDownloadError();
            return false;
        }
        else
            return true;
    }
    else
    {
        qDebug() << "unable to open file to write:" << fileName;
        return false;
    }
}

void DatapackDownloaderBase::datapackDownloadFinishedBase()
{
    haveTheDatapack();
}

void DatapackDownloaderBase::datapackChecksumDoneBase(const std::vector<std::string> &datapackFilesList,const QByteArray &hash,const QList<uint32_t> &partialHashList)
{
    if(datapackFilesListBase.size()!=partialHashList.size())
    {
        qDebug() << "datapackFilesListBase.size()!=partialHash.size():" << datapackFilesListBase.size() << "!=" << partialHashList.size();
        qDebug() << "this->datapackFilesList:" << this->datapackFilesListBase.join("\n");
        qDebug() << "datapackFilesList:" << datapackFilesListBase.join("\n");
        abort();
    }

    hashBase=hash;
    this->datapackFilesListBase=datapackFilesList;
    this->partialHashListBase=partialHashList;
    if(!datapackFilesListBase.isEmpty() && hash==sendedHashBase)
    {
        qDebug() << "Datapack is not empty and get nothing from serveur because the local datapack hash match with the remote";
        datapackDownloadFinishedBase();
        return;
    }

    if(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.isEmpty())
    {
        {
            QFile file(mDatapackBase+"/pack/datapack.tar.xz");
            if(file.exists())
                if(!file.remove())
                {
                    qDebug() << "Unable to remove "+file.fileName();
                    return;
                }
        }
        {
            QFile file(mDatapackBase+"/datapack-list/base.txt");
            if(file.exists())
                if(!file.remove())
                {
                    qDebug() << "Unable to remove "+file.fileName();
                    return;
                }
        }
        if(sendedHashBase.isEmpty())
        {
            qDebug() << "Datapack checksum done but not send by the server";
            abort();//need CommonSettings::commonSettings.datapackHash send by the server
        }
        uint8_t datapack_content_query_number=0;
        LinkToGameServer * client=NULL;
        {
            unsigned int indexForClient=0;
            while(indexForClient<clientInSuspend.size())
            {
                if(clientInSuspend.at(indexForClient)!=NULL)
                {
                    client=static_cast<LinkToGameServer *>(clientInSuspend.at(0));
                    datapack_content_query_number=client->freeQueryNumberToServer();
                    break;
                }
                indexForClient++;
            }
            if(indexForClient>=clientInSuspend.size())
            {
                qDebug() << "no client in suspend to do the query to do in protocol datapack download";
                resetAll();
                return;//need CommonSettings::commonSettings.datapackHash send by the server
            }
        }
        qDebug() << "Datapack is empty or hash don't match, get from server, hash local: " << std::string(hash.toHex()) << ", hash on server: " << std::string(CommonSettingsServer::commonSettingsServer.datapackHashServerSub.toHex());
        out << (uint8_t)0x01;//DatapackStatus::Base
        out << (uint32_t)datapackFilesListBase.size();
        int index=0;
        while(index<datapackFilesListBase.size())
        {
            const QByteArray &rawFileName=FacilityLibGeneral::toUTF8WithHeader(datapackFilesListBase.at(index));
            if(rawFileName.size()>255 || rawFileName.isEmpty())
            {
                DebugClass::debugConsole(std::stringLiteral("rawFileName too big or not compatible with utf8"));
                return;
            }
            outputData+=rawFileName;
            out.device()->seek(out.device()->size());

            /*struct stat info;
            stat(std::string(mDatapackBase+datapackFilesListBase.at(index)).toLatin1().data(),&info);*/
            out << (uint32_t)partialHashList.at(index);
            index++;
        }
        client->packFullOutcommingQuery(0x02,0x0C,datapack_content_query_number,outputData.constData(),outputData.size());
    }
    else
    {
        curl=curl_easy_init();
        if(!curl)
        {
            std::cerr << "curl_easy_init() failed abort" << std::endl;
            abort();
        }
        if(datapackFilesListBase.isEmpty())
        {
            index_mirror_base=0;
            test_mirror_base();
            qDebug() << "Datapack is empty, get from mirror into" << mDatapackBase;
        }
        else
        {
            qDebug() << "Datapack don't match with server hash, get from mirror";

            const std::string url=CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.split(DatapackDownloaderBase::text_dotcoma,std::string::SkipEmptyParts).at(index_mirror_base)+std::stringLiteral("pack/diff/datapack-base-%1.tar.xz").arg(std::string(hash.toHex()));

            struct MemoryStruct chunk;
            chunk.memory = static_cast<char *>(malloc(1));  /* will be grown as needed by the realloc above */
            chunk.size = 0;    /* no data at this point */
            curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, EpollServerLoginSlave::WriteMemoryCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
            const CURLcode res = curl_easy_perform(curl);
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            if(res!=CURLE_OK || http_code!=200)
            {
                qDebug() << (std::stringLiteral("get url %1: %2 failed with code %3").arg(url).arg(res).arg(http_code));
                httpFinishedForDatapackListBase();
                return;
            }
            httpFinishedForDatapackListBase(QByteArray(chunk.memory,chunk.size));
            free(chunk.memory);
        }
    }
}

void DatapackDownloaderBase::test_mirror_base()
{
    const std::vector<std::string> &httpDatapackMirrorList=CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.split(DatapackDownloaderBase::text_dotcoma,std::string::SkipEmptyParts);
    if(!datapackTarXzBase)
    {
        const std::string url=httpDatapackMirrorList.at(index_mirror_base)+std::stringLiteral("pack/datapack.tar.xz");

        struct MemoryStruct chunk;
        chunk.memory = static_cast<char *>(malloc(1));  /* will be grown as needed by the realloc above */
        chunk.size = 0;    /* no data at this point */
        curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, EpollServerLoginSlave::WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        const CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if(res!=CURLE_OK || http_code!=200)
        {
            qDebug() << (std::stringLiteral("get url %1: %2 failed with code %3").arg(url).arg(res).arg(http_code));
            httpFinishedForDatapackListBase();
            return;
        }
        httpFinishedForDatapackListBase(QByteArray(chunk.memory,chunk.size));
        free(chunk.memory);
    }
    else
    {
        if(index_mirror_base>=httpDatapackMirrorList.size())
            /* here and not above because at last mirror you need try the tar.xz and after the datapack-list/base.txt, and only after that's quit */
            return;

        const std::string url=httpDatapackMirrorList.at(index_mirror_base)+std::stringLiteral("datapack-list/base.txt");

        struct MemoryStruct chunk;
        chunk.memory = static_cast<char *>(malloc(1));  /* will be grown as needed by the realloc above */
        chunk.size = 0;    /* no data at this point */
        curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, EpollServerLoginSlave::WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        const CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if(res!=CURLE_OK || http_code!=200)
        {
            qDebug() << (std::stringLiteral("get url %1: %2 failed with code %3").arg(url).arg(res).arg(http_code));
            httpFinishedForDatapackListBase();
            return;
        }
        httpFinishedForDatapackListBase(QByteArray(chunk.memory,chunk.size));
        free(chunk.memory);
    }
}

void DatapackDownloaderBase::decodedIsFinishBase()
{
    if(xzDecodeThreadBase.errorFound())
        test_mirror_base();
    else
    {
        const QByteArray &decodedData=xzDecodeThreadBase.decodedData();
        QTarDecode tarDecode;
        if(tarDecode.decodeData(decodedData))
        {
            QDir dir;
            const std::vector<std::string> &fileList=tarDecode.getFileList();
            const QList<QByteArray> &dataList=tarDecode.getDataList();
            int index=0;
            while(index<fileList.size())
            {
                QFile file(mDatapackBase+fileList.at(index));
                QFileInfo fileInfo(file);
                dir.mkpath(fileInfo.absolutePath());
                if(extensionAllowed.contains(fileInfo.suffix()))
                {
                    if(file.open(QIODevice::Truncate|QIODevice::WriteOnly))
                    {
                        file.write(dataList.at(index));
                        file.close();
                    }
                    else
                    {
                        qDebug() << (std::stringLiteral("unable to write file of datapack %1: %2").arg(file.fileName()).arg(file.errorString()));
                        return;
                    }
                }
                else
                {
                    qDebug() << (std::stringLiteral("file not allowed: %1").arg(file.fileName()));
                    return;
                }
                index++;
            }
            wait_datapack_content_base=false;
            datapackDownloadFinishedBase();
        }
        else
            test_mirror_base();
    }
}

bool DatapackDownloaderBase::mirrorTryNextBase()
{
    if(datapackTarXzBase==false)
    {
        datapackTarXzBase=true;
        test_mirror_base();
    }
    else
    {
        datapackTarXzBase=false;
        index_mirror_base++;
        if(index_mirror_base>=CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.split(DatapackDownloaderBase::text_dotcoma,std::string::SkipEmptyParts).size())
        {
            qDebug() << (std::stringLiteral("Get the list failed"));
            return false;
        }
        else
            test_mirror_base();
    }
    return true;
}

void DatapackDownloaderBase::httpFinishedForDatapackListBase(const QByteArray data)
{
    if(data.isEmpty())
    {
        mirrorTryNextBase();
        return;
    }
    else
    {
        if(!datapackTarXzBase)
        {
            qDebug() << "datapack.tar.xz size:" << std::string("%1KB").arg(data.size()/1000);
            datapackTarXzBase=true;
            xzDecodeThreadBase.setData(data,16*1024*1024);
            xzDecodeThreadBase.run();
            decodedIsFinishBase();
            return;
        }
        else
        {
            httpError=false;
            const std::vector<std::string> &content=std::string::fromUtf8(data).split(std::regex("[\n\r]+"));
            int index=0;
            std::regex splitReg("^(.*) (([0-9a-f][0-9a-f])+) ([0-9]+)$");
            std::regex fileMatchReg(DATAPACK_FILE_REGEX);
            if(datapackFilesListBase.size()!=partialHashListBase.size())
            {
                qDebug() << "datapackFilesListBase.size()!=partialHashList.size(), CRITICAL";
                abort();
                return;
            }
            /*ref crash here*/const std::string selectedMirror=CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.split(DatapackDownloaderBase::text_dotcoma,std::string::SkipEmptyParts).at(index_mirror_base);
            int correctContent=0;
            while(index<content.size())
            {
                if(content.at(index).contains(splitReg))
                {
                    correctContent++;
                    std::string fileString=content.at(index);
                    std::string partialHashString=content.at(index);
                    std::string sizeString=content.at(index);
                    fileString.replace(splitReg,"\\1");
                    partialHashString.replace(splitReg,"\\2");
                    sizeString.replace(splitReg,"\\4");
                    if(fileString.contains(fileMatchReg))
                    {
                        int indexInDatapackList=datapackFilesListBase.indexOf(fileString);
                        if(indexInDatapackList!=-1)
                        {
                            const uint32_t &hashFileOnDisk=partialHashListBase.at(indexInDatapackList);
                            QFileInfo file(mDatapackBase+fileString);
                            if(!file.exists())
                            {
                                if(!getHttpFileBase(selectedMirror+fileString,fileString))
                                {
                                    datapackDownloadError();
                                    return;
                                }
                            }
                            else if(hashFileOnDisk!=*reinterpret_cast<const uint32_t *>(QByteArray::fromHex(partialHashString.toLatin1()).constData()))
                            {
                                if(!getHttpFileBase(selectedMirror+fileString,fileString))
                                {
                                    datapackDownloadError();
                                    return;
                                }
                            }
                        }
                        else
                        {
                            if(!getHttpFileBase(selectedMirror+fileString,fileString))
                            {
                                datapackDownloadError();
                                return;
                            }
                        }
                        partialHashListBase.removeAt(indexInDatapackList);
                        datapackFilesListBase.removeAt(indexInDatapackList);
                    }
                }
                index++;
            }
            index=0;
            while(index<datapackFilesListBase.size())
            {
                if(!QFile(mDatapackBase+datapackFilesListBase.at(index)).remove())
                {
                    qDebug() << "Unable to remove" << datapackFilesListBase.at(index);
                    abort();
                }
                index++;
            }
            datapackFilesListBase.clear();
            if(correctContent==0)
            {
                qDebug() << "Error, no valid content: correctContent==0\n" << content.join("\n");
                abort();
                return;
            }
            datapackDownloadFinishedBase();
        }
    }
}

const std::vector<std::string> DatapackDownloaderBase::listDatapackBase(std::string suffix)
{
    if(suffix.contains(excludePathBase))
        return std::vector<std::string>();

    std::vector<std::string> returnFile;
    QDir finalDatapackFolder(mDatapackBase+suffix);
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for(int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            returnFile << listDatapackBase(suffix+fileInfo.fileName()+text_slash);//put unix separator because it's transformed into that's under windows too
        else
        {
            //if match with correct file name, considere as valid
            if((suffix+fileInfo.fileName()).contains(DatapackDownloaderBase::regex_DATAPACK_FILE_REGEX) && extensionAllowed.contains(fileInfo.suffix()))
                returnFile << suffix+fileInfo.fileName();
            //is invalid
            else
            {
                DebugClass::debugConsole(std::stringLiteral("listDatapack(): remove invalid file: %1").arg(suffix+fileInfo.fileName()));
                QFile file(mDatapackBase+suffix+fileInfo.fileName());
                if(!file.remove())
                    DebugClass::debugConsole(std::stringLiteral("listDatapack(): unable remove invalid file: %1: %2").arg(suffix+fileInfo.fileName()).arg(file.errorString()));
            }
        }
    }
    returnFile.sort();
    return returnFile;
}

void DatapackDownloaderBase::cleanDatapackBase(std::string suffix)
{
    QDir finalDatapackFolder(mDatapackBase+suffix);
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            cleanDatapackBase(suffix+fileInfo.fileName()+text_slash);//put unix separator because it's transformed into that's under windows too
        else
            return;
    }
    entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    if(entryList.size()==0)
        finalDatapackFolder.rmpath(mDatapackBase+suffix);
}

void DatapackDownloaderBase::sendDatapackContentBase()
{
    if(wait_datapack_content_base)
    {
        DebugClass::debugConsole(std::stringLiteral("already in wait of datapack content"));
        return;
    }

    //compute the mirror
    {
        std::vector<std::string> values=CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.split(DatapackDownloaderBase::text_dotcoma,std::string::SkipEmptyParts);
        {
            std::string slash(std::stringLiteral("/"));
            int index=0;
            while(index<values.size())
            {
                if(!values.at(index).endsWith(slash))
                    values[index]+=slash;
                index++;
            }
        }
        CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase=values.join(DatapackDownloaderBase::text_dotcoma);
    }

    datapackTarXzBase=false;
    wait_datapack_content_base=true;
    datapackFilesListBase=listDatapackBase(std::string());
    datapackFilesListBase.sort();
    const DatapackChecksum::FullDatapackChecksumReturn &fullDatapackChecksumReturn=DatapackChecksum::doFullSyncChecksumBase(mDatapackBase);
    datapackChecksumDoneBase(fullDatapackChecksumReturn.datapackFilesList,fullDatapackChecksumReturn.hash,fullDatapackChecksumReturn.partialHashList);
}
