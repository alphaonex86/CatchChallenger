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
#include "../../general/base/cpp11addition.h"
#include "LinkToGameServer.h"
#include "EpollServerLoginSlave.h"

//need host + port here to have datapack base

std::regex DatapackDownloaderBase::regex_DATAPACK_FILE_REGEX(DATAPACK_FILE_REGEX);
std::unordered_set<std::string> DatapackDownloaderBase::extensionAllowed;
std::regex DatapackDownloaderBase::excludePathBase("^(map[/\\\\]main[/\\\\]|pack[/\\\\]|datapack-list[/\\\\])");
std::string DatapackDownloaderBase::commandUpdateDatapackBase;
std::vector<std::string> DatapackDownloaderBase::httpDatapackMirrorBaseList;

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
    if(DatapackDownloaderBase::httpDatapackMirrorBaseList.empty())
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
        if(QProcess::execute(QString::fromStdString(DatapackDownloaderBase::commandUpdateDatapackBase),QStringList() << QString::fromStdString(mDatapackBase))<0)
            std::cerr << "Unable to execute " << DatapackDownloaderBase::commandUpdateDatapackBase << " " << mDatapackBase << std::endl;
}

void DatapackDownloaderBase::resetAll()
{
    httpModeBase=false;
    httpError=false;
    query_files_list_base.clear();
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
    if(datapackFilesListBase.empty() && size==1)
    {
        if(!httpModeBase)
            haveTheDatapack();
        return;
    }
    unsigned int pos=0;
    std::vector<bool> boolList;
    boolList.reserve(size*8);
    while((size-pos)>0)
    {
        const uint8_t &returnCode=data[pos];
        boolList.push_back(returnCode&0x01);
        boolList.push_back(returnCode&0x02);
        boolList.push_back(returnCode&0x04);
        boolList.push_back(returnCode&0x08);
        boolList.push_back(returnCode&0x10);
        boolList.push_back(returnCode&0x20);
        boolList.push_back(returnCode&0x40);
        boolList.push_back(returnCode&0x80);
        pos++;
    }
    if(boolList.size()<datapackFilesListBase.size())
    {
        std::cerr << "bool list too small with DatapackDownloaderBase::datapackFileList" << std::endl;
        return;
    }
    unsigned int index=0;
    while(index<datapackFilesListBase.size())
    {
        if(boolList.at(index))
        {
            std::cerr << "remove the file: " << mDatapackBase << '/' << datapackFilesListBase.at(index) << std::endl;
            QFile file(QString::fromStdString(mDatapackBase+'/'+datapackFilesListBase.at(index)));
            if(!file.remove())
                std::cerr << "unable to remove the file: " << datapackFilesListBase.at(index) << ": " << file.errorString().toStdString() << std::endl;
            //removeFile(datapackFilesListBase.at(index));
        }
        index++;
    }
    boolList.clear();
    datapackFilesListBase.clear();
    cleanDatapackBase(std::string());
    if(boolList.size()>=8)
    {
        std::cerr << "bool list too big with DatapackDownloaderBase::datapackFileList" << std::endl;
        return;
    }
    if(!httpModeBase)
        haveTheDatapack();
}

void DatapackDownloaderBase::writeNewFileBase(const std::string &fileName,const std::vector<char> &data)
{
    const std::string &fullPath=mDatapackBase+'/'+fileName;
    //to be sure the QFile is destroyed
    {
        QFile file(QString::fromStdString(fullPath));
        QFileInfo fileInfo(file);

        if(!QDir(fileInfo.absolutePath()).mkpath(fileInfo.absolutePath()))
        {
            qDebug() << "unable to make the path: " << fileInfo.absolutePath();
            abort();
        }

        if(file.exists())
            if(!file.remove())
            {
                std::cerr << "Can't remove: " << fileName << ": " << file.errorString().toStdString() << std::endl;
                return;
            }
        if(!file.open(QIODevice::WriteOnly))
        {
            std::cerr << "Can't open: " << fileName << ": " << file.errorString().toStdString() << std::endl;
            return;
        }
        if(file.write(data.data(),data.size())!=(int64_t)data.size())
        {
            file.close();
            std::cerr << "Can't write: " << fileName << ": " << file.errorString().toStdString() << std::endl;
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

    const std::string &fullPath=mDatapackBase+'/'+fileName;
    {
        QFile file(QString::fromStdString(fullPath));
        QFileInfo fileInfo(file);

        if(!QDir(fileInfo.absolutePath()).mkpath(fileInfo.absolutePath()))
        {
            qDebug() << "unable to make the path: " << fileInfo.absolutePath();
            abort();
        }
    }

    FILE *fp = fopen(fullPath.c_str(),"wb");
    if(fp!=NULL)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &fwrite);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        const CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        fclose(fp);
        if(res!=CURLE_OK || http_code!=200)
        {
            httpError=true;
            std::cerr << "get url " << url << ": " << res << " failed with code " << http_code << std::endl;
            datapackDownloadError();
            return false;
        }
        else
            return true;
    }
    else
    {
        std::cerr << "unable to open file to write:" << fileName << std::endl;
        return false;
    }
}

void DatapackDownloaderBase::datapackDownloadFinishedBase()
{
    haveTheDatapack();
}

void DatapackDownloaderBase::datapackChecksumDoneBase(const std::vector<std::string> &datapackFilesList,const std::vector<char> &hash,const std::vector<uint32_t> &partialHashList)
{
    if(datapackFilesListBase.size()!=partialHashList.size())
    {
        std::cerr << "datapackFilesListBase.size()!=partialHash.size():" << datapackFilesListBase.size() << "!=" << partialHashList.size() << std::endl;
        std::cerr << "this->datapackFilesList:" << stringimplode(this->datapackFilesListBase,"\n") << std::endl;
        std::cerr << "datapackFilesList:" << stringimplode(datapackFilesList,"\n") << std::endl;
        abort();
    }

    hashBase=hash;
    this->datapackFilesListBase=datapackFilesList;
    this->partialHashListBase=partialHashList;
    if(!datapackFilesListBase.empty() && hash==sendedHashBase)
    {
        qDebug() << "Datapack is not empty and get nothing from serveur because the local datapack hash match with the remote";
        datapackDownloadFinishedBase();
        return;
    }

    if(DatapackDownloaderBase::httpDatapackMirrorBaseList.empty())
    {
        {
            QFile file(QString::fromStdString(mDatapackBase+"/pack/datapack.tar.xz"));
            if(file.exists())
                if(!file.remove())
                {
                    qDebug() << "Unable to remove "+file.fileName();
                    return;
                }
        }
        {
            QFile file(QString::fromStdString(mDatapackBase+"/datapack-list/base.txt"));
            if(file.exists())
                if(!file.remove())
                {
                    qDebug() << "Unable to remove "+file.fileName();
                    return;
                }
        }
        if(sendedHashBase.empty())
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
        std::cerr << "Datapack is empty or hash don't match, get from server, hash local: " << binarytoHexa(hash) << ", hash on server: "
                  << binarytoHexa(CommonSettingsServer::commonSettingsServer.datapackHashServerSub) << std::endl;

        //send the network query
        client->registerOutputQuery(datapack_content_query_number);
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0xA1;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=datapack_content_query_number;
        posOutput+=1+4;

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
        posOutput+=1;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(datapackFilesListBase.size());
        posOutput+=4;
        unsigned int index=0;
        while(index<datapackFilesListBase.size())
        {
            {
                const std::string &text=datapackFilesListBase.at(index);
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
                posOutput+=1;
                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
                posOutput+=text.size();
            }
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(partialHashList.at(index));
            posOutput+=4;
            index++;
        }

        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);//set the dynamic size
        client->sendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
    else
    {
        curl=curl_easy_init();
        if(!curl)
        {
            std::cerr << "curl_easy_init() failed abort" << std::endl;
            abort();
        }
        if(datapackFilesListBase.empty())
        {
            index_mirror_base=0;
            test_mirror_base();
            std::cerr << "Datapack is empty, get from mirror into" << mDatapackBase << std::endl;
        }
        else
        {
            std::cerr << "Datapack don't match with server hash, get from mirror" << std::endl;

            const std::string url=DatapackDownloaderBase::httpDatapackMirrorBaseList.at(index_mirror_base)+"pack/diff/datapack-base-"+binarytoHexa(hash)+".tar.xz";

            struct MemoryStruct chunk;
            chunk.memory = static_cast<char *>(malloc(1));  /* will be grown as needed by the realloc above */
            chunk.size = 0;    /* no data at this point */
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, EpollServerLoginSlave::WriteMemoryCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
            const CURLcode res = curl_easy_perform(curl);
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            if(res!=CURLE_OK || http_code!=200)
            {
                std::cerr << "get url " << url << ": " << res << " failed with code " << http_code << std::endl;
                httpFinishedForDatapackListBase();
                return;
            }
            std::vector<char> data(chunk.size);
            memcpy(data.data(),chunk.memory,chunk.size);
            httpFinishedForDatapackListBase(data);
            free(chunk.memory);
        }
    }
}

void DatapackDownloaderBase::test_mirror_base()
{
    if(!datapackTarXzBase)
    {
        const std::string url=DatapackDownloaderBase::httpDatapackMirrorBaseList.at(index_mirror_base)+"pack/datapack.tar.xz";

        struct MemoryStruct chunk;
        chunk.memory = static_cast<char *>(malloc(1));  /* will be grown as needed by the realloc above */
        chunk.size = 0;    /* no data at this point */
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, EpollServerLoginSlave::WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        const CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if(res!=CURLE_OK || http_code!=200)
        {
            std::cerr << "get url " << url << ": " << res << " failed with code " << http_code << std::endl;
            httpFinishedForDatapackListBase();
            return;
        }
        std::vector<char> data(chunk.size);
        memcpy(data.data(),chunk.memory,chunk.size);
        httpFinishedForDatapackListBase(data);
        free(chunk.memory);
    }
    else
    {
        if(index_mirror_base>=DatapackDownloaderBase::httpDatapackMirrorBaseList.size())
            /* here and not above because at last mirror you need try the tar.xz and after the datapack-list/base.txt, and only after that's quit */
            return;

        const std::string url=DatapackDownloaderBase::httpDatapackMirrorBaseList.at(index_mirror_base)+"datapack-list/base.txt";

        struct MemoryStruct chunk;
        chunk.memory = static_cast<char *>(malloc(1));  /* will be grown as needed by the realloc above */
        chunk.size = 0;    /* no data at this point */
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, EpollServerLoginSlave::WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        const CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if(res!=CURLE_OK || http_code!=200)
        {
            std::cerr << "get url " << url << ": " << res << " failed with code " << http_code << std::endl;
            httpFinishedForDatapackListBase();
            return;
        }
        std::vector<char> data(chunk.size);
        memcpy(data.data(),chunk.memory,chunk.size);
        httpFinishedForDatapackListBase(data);
        free(chunk.memory);
    }
}

void DatapackDownloaderBase::decodedIsFinishBase()
{
    if(xzDecodeThreadBase.errorFound())
        test_mirror_base();
    else
    {
        const std::vector<char> &decodedData=xzDecodeThreadBase.decodedData();
        QTarDecode tarDecode;
        if(tarDecode.decodeData(decodedData))
        {
            QDir dir;
            const std::vector<std::string> &fileList=tarDecode.getFileList();
            const std::vector<std::vector<char> > &dataList=tarDecode.getDataList();
            unsigned int index=0;
            while(index<fileList.size())
            {
                QFile file(QString::fromStdString(mDatapackBase+fileList.at(index)));
                QFileInfo fileInfo(file);
                dir.mkpath(fileInfo.absolutePath());
                if(extensionAllowed.find(fileInfo.suffix().toStdString())!=extensionAllowed.cend())
                {
                    if(file.open(QIODevice::Truncate|QIODevice::WriteOnly))
                    {
                        file.write(dataList.at(index).data(),dataList.at(index).size());
                        file.close();
                    }
                    else
                    {
                        std::cerr << "unable to write file of datapack " << file.fileName().toStdString() << ": " << file.errorString().toStdString() << std::endl;
                        return;
                    }
                }
                else
                {
                    std::cerr << "file not allowed: " << file.fileName().toStdString() << std::endl;
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
        if(index_mirror_base>=DatapackDownloaderBase::httpDatapackMirrorBaseList.size())
        {
            std::cerr << "Get the list failed" << std::endl;
            return false;
        }
        else
            test_mirror_base();
    }
    return true;
}

void DatapackDownloaderBase::httpFinishedForDatapackListBase(const std::vector<char> data)
{
    if(data.empty())
    {
        mirrorTryNextBase();
        return;
    }
    else
    {
        if(!datapackTarXzBase)
        {
            std::cerr << "datapack.tar.xz size:" << data.size()/1000 << "KB" << std::endl;
            datapackTarXzBase=true;
            xzDecodeThreadBase.setData(data,16*1024*1024);
            xzDecodeThreadBase.run();
            decodedIsFinishBase();
            return;
        }
        else
        {
            httpError=false;

            size_t endOfText;
            {
                std::string text(data.data(),data.size());
                endOfText=text.find("\n-\n");
            }
            if(endOfText==std::string::npos)
            {
                std::cerr << "not text delimitor into file list" << std::endl;
                return;
            }
            std::vector<std::string> content;
            std::vector<char> partialHashListRaw(data.cbegin()+endOfText+3,data.cend()-endOfText-3);
            {
                if(partialHashListRaw.size()%4!=0)
                {
                    std::cerr << "partialHashList not divisible by 4" << std::endl;
                    return;
                }
                {
                    std::string text(data.data(),endOfText);
                    content=stringsplit(text,'\n');
                }
                if(partialHashListRaw.size()/4!=content.size())
                {
                    std::cerr << "partialHashList/4!=content.size()" << std::endl;
                    return;
                }
            }

            /*ref crash here*/const std::string selectedMirror=DatapackDownloaderBase::httpDatapackMirrorBaseList.at(index_mirror_base);
            unsigned int correctContent=0;
            unsigned int index=0;
            while(index<content.size())
            {
                size_t const &found=content.at(index).find(' ');
                if(found!=std::string::npos)
                {
                    correctContent++;
                    const std::string &line=content.at(index);
                    const std::string &fileString=line.substr(0,found);
                    const uint32_t &partialHashString=*reinterpret_cast<uint32_t *>(partialHashListRaw.data()+index*4);
                    //const std::string &sizeString=line.substr(found+1,line.size()-found-1);
                    if(std::regex_match(fileString,DatapackDownloaderBase::regex_DATAPACK_FILE_REGEX))
                    {
                        int indexInDatapackList=vectorindexOf(datapackFilesListBase,fileString);
                        if(indexInDatapackList!=-1)
                        {
                            const uint32_t &hashFileOnDisk=partialHashListBase.at(indexInDatapackList);
                            QFileInfo file(QString::fromStdString(mDatapackBase+fileString));
                            if(!file.exists())
                            {
                                if(!getHttpFileBase(selectedMirror+fileString,fileString))
                                {
                                    datapackDownloadError();
                                    return;
                                }
                            }
                            else if(hashFileOnDisk!=partialHashString)
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
                        partialHashListBase.erase(partialHashListBase.cbegin()+indexInDatapackList);
                        datapackFilesListBase.erase(datapackFilesListBase.cbegin()+indexInDatapackList);
                    }
                }
                index++;
            }
            index=0;
            while(index<datapackFilesListBase.size())
            {
                if(!QFile(QString::fromStdString(mDatapackBase+datapackFilesListBase.at(index))).remove())
                {
                    std::cerr << "Unable to remove" << datapackFilesListBase.at(index) << std::endl;
                    abort();
                }
                index++;
            }
            datapackFilesListBase.clear();
            if(correctContent==0)
            {
                std::cerr << "Error, no valid content: correctContent==0\n" << stringimplode(content,'\n') << std::endl;
                return;
            }
            datapackDownloadFinishedBase();
        }
    }
}

const std::vector<std::string> DatapackDownloaderBase::listDatapackBase(std::string suffix)
{
    if(std::regex_match(suffix,excludePathBase))
        return std::vector<std::string>();

    std::vector<std::string> returnFile;
    QDir finalDatapackFolder(QString::fromStdString(mDatapackBase+suffix));
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for(int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
        {
            const std::vector<std::string> &newReturnFile=listDatapackBase(suffix+fileInfo.fileName().toStdString()+'/');
            returnFile.insert(returnFile.cend(),newReturnFile.cbegin(),newReturnFile.cend());//put unix separator because it's transformed into that's under windows too
        }
        else
        {
            //if match with correct file name, considere as valid
            if(std::regex_match(suffix+fileInfo.fileName().toStdString(),DatapackDownloaderBase::regex_DATAPACK_FILE_REGEX) && extensionAllowed.find(fileInfo.suffix().toStdString())!=extensionAllowed.cend())
                returnFile.push_back(suffix+fileInfo.fileName().toStdString());
            //is invalid
            else
            {
                std::cerr << "listDatapack(): remove invalid file: " << suffix << fileInfo.fileName().toStdString() << std::endl;
                QFile file(QString::fromStdString(mDatapackBase+suffix+fileInfo.fileName().toStdString()));
                if(!file.remove())
                    std::cerr << "listDatapack(): unable remove invalid file: " << suffix << fileInfo.fileName().toStdString() << ": " << file.errorString().toStdString() << std::endl;
            }
        }
    }
    std::sort(returnFile.begin(),returnFile.end());
    return returnFile;
}

void DatapackDownloaderBase::cleanDatapackBase(std::string suffix)
{
    QDir finalDatapackFolder(QString::fromStdString(mDatapackBase+suffix));
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            cleanDatapackBase(suffix+fileInfo.fileName().toStdString()+'/');//put unix separator because it's transformed into that's under windows too
        else
            return;
    }
    entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    if(entryList.size()==0)
        finalDatapackFolder.rmpath(QString::fromStdString(mDatapackBase+suffix));
}

void DatapackDownloaderBase::sendDatapackContentBase()
{
    if(wait_datapack_content_base)
    {
        std::cerr << "already in wait of datapack content" << std::endl;
        return;
    }

    datapackTarXzBase=false;
    wait_datapack_content_base=true;
    datapackFilesListBase=listDatapackBase(std::string());
    std::sort(datapackFilesListBase.begin(),datapackFilesListBase.end());
    const DatapackChecksum::FullDatapackChecksumReturn &fullDatapackChecksumReturn=DatapackChecksum::doFullSyncChecksumBase(mDatapackBase);
    datapackChecksumDoneBase(fullDatapackChecksumReturn.datapackFilesList,fullDatapackChecksumReturn.hash,fullDatapackChecksumReturn.partialHashList);
}
