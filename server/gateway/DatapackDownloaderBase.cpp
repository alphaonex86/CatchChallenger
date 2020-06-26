#include "DatapackDownloaderBase.hpp"

#include <stdio.h>

using namespace CatchChallenger;

#include <iostream>
#include <cmath>
#include <regex>
#include <cstdio>
#include <stdio.h>
#include <thread>
#include <chrono>

#include "../../general/libzstd/lib/zstd.h"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/GeneralVariable.hpp"
#include "../../general/base/cpp11addition.hpp"
#include "../../client/tarcompressed/TarDecode.hpp"
#include "LinkToGameServer.hpp"
#include "EpollServerLoginSlave.hpp"
#include "FacilityLibGateway.hpp"

//need host + port here to have datapack base

std::regex DatapackDownloaderBase::regex_DATAPACK_FILE_REGEX(DATAPACK_FILE_REGEX);
std::unordered_set<std::string> DatapackDownloaderBase::extensionAllowed;
std::regex DatapackDownloaderBase::excludePathBase("^(map[/\\\\]main[/\\\\]|pack[/\\\\]|datapack-list[/\\\\])");
std::string DatapackDownloaderBase::commandUpdateDatapackBase;
std::vector<std::string> DatapackDownloaderBase::httpDatapackMirrorBaseList;

DatapackDownloaderBase * DatapackDownloaderBase::datapackDownloaderBase=NULL;
CURLM *DatapackDownloaderBase::curlm=NULL;
unsigned int DatapackDownloaderBase::DatapackDownloaderBase::curlmCount=0;
std::vector<CURL *> DatapackDownloaderBase::DatapackDownloaderBase::curlSuspendList;
std::unordered_map<CURL *,void *> DatapackDownloaderBase::curlPrivateData;

DatapackDownloaderBase::DatapackDownloaderBase(const std::string &mDatapackBase) :
    mDatapackBase(mDatapackBase)
{
    httpModeBase=false;
    datapackTarBase=false;
    index_mirror_base=0;
    wait_datapack_content_base=false;
    curlm = curl_multi_init();
}

DatapackDownloaderBase::~DatapackDownloaderBase()
{
}

void DatapackDownloaderBase::haveTheDatapack()
{
    std::cout << "DatapackDownloaderBase::haveTheDatapack()" << std::endl;
    if(DatapackDownloaderBase::httpDatapackMirrorBaseList.empty())
        std::cout << "Have the datapack base" << std::endl;

    if(!DatapackDownloaderBase::commandUpdateDatapackBase.empty())
    {
        if(numberOfFileWritten>0)
        {
            const int ret = system(DatapackDownloaderBase::commandUpdateDatapackBase.c_str());
            if(ret==-1)
                std::cerr << "Unable to execute " << DatapackDownloaderBase::commandUpdateDatapackBase << std::endl;
            else
                std::cout << "Correctly execute " << DatapackDownloaderBase::commandUpdateDatapackBase << " with return code: " << std::to_string(ret) << std::endl;
        }
        else
            std::cout << "Not execute due to no file writen " << DatapackDownloaderBase::commandUpdateDatapackBase << std::endl;
    }
    else
        std::cout << "No command defined" << std::endl;

    unsigned int index=0;
    while(index<clientInSuspend.size())
    {
        LinkToGameServer * const clientLink=static_cast<LinkToGameServer *>(clientInSuspend.at(index));
        if(clientLink!=NULL)
            clientLink->sendDifferedA8Reply();
        index++;
    }
    clientInSuspend.clear();

    //regen the datapack cache
    if(LinkToGameServer::httpDatapackMirrorRewriteBase.size()<=1)
        EpollClientLoginSlave::datapack_file_base.datapack_file_hash_cache=EpollClientLoginSlave::datapack_file_list(mDatapackBase);

    resetAll();
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
        LinkToGameServer * const clientLink=static_cast<LinkToGameServer *>(clientInSuspend.at(index));
        if(clientLink!=NULL)
            clientLink->disconnectClient();
        index++;
    }
    clientInSuspend.clear();
    wait_datapack_content_base=false;
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
            if(remove((mDatapackBase+'/'+datapackFilesListBase.at(index)).c_str())!=0)
                std::cerr << "unable to remove the file: " << datapackFilesListBase.at(index) << ": " << errno << std::endl;
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
    numberOfFileWritten++;
    if(data.size()>CATCHCHALLENGER_MAX_FILE_SIZE)
    {
        std::cerr << "file too big: " << fileName << std::endl;
        abort();
    }
    std::string fullPath=mDatapackBase+'/'+fileName;
    stringreplaceAll(fullPath,"//","/");
    //to be sure the QFile is destroyed
    {
        if(!FacilityLibGateway::mkpath(FacilityLibGeneral::getFolderFromFile(fullPath)))
        {
            std::cerr << "unable to make the path: " << fullPath << std::endl;
            abort();
        }

        FILE *file=fopen(fullPath.c_str(),"wb");
        if(file==NULL)
        {
            std::cerr << "Can't open: " << fileName << ": " << errno << std::endl;
            return;
        }
        if(fwrite(data.data(),1,data.size(),file)!=data.size())
        {
            fclose(file);
            std::cerr << "Can't write: " << fileName << ": " << errno << std::endl;
            return;
        }
        fclose(file);
    }
}

bool DatapackDownloaderBase::getHttpFileBase(const std::string &url, const std::string &fileName)
{
    if(httpError)
        return false;
    if(!httpModeBase)
        httpModeBase=true;

    std::string fullPath=mDatapackBase+'/'+fileName;
    stringreplaceAll(fullPath,"//","/");

    MemoryStruct *chunk=new MemoryStruct;
    chunk->fileName=fullPath;
    chunk->memory = static_cast<char *>(malloc(1));  /* will be grown as needed by the realloc above */
    chunk->size = 0;    /* no data at this point */

    CURL *curl=curl_easy_init();
    if(!curl)
    {
        std::cerr << "curl_easy_init() failed abort" << std::endl;
        abort();
    }
    if(curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L)!=CURLE_OK)
        std::cerr << "Unable to set the curl keep alive" << std::endl;
    if(curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L)!=CURLE_OK)
        std::cerr << "Unable to set the curl keep alive" << std::endl;
    if(curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L)!=CURLE_OK)
        std::cerr << "Unable to set the curl keep alive" << std::endl;
    if(curl_easy_setopt(curl, CURLOPT_URL, url.c_str())!=CURLE_OK)
    {
        std::cerr << "Unable to set the curl url: " << url << std::endl;
        abort();
    }
    if(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, EpollServerLoginSlave::WriteMemoryCallback)!=CURLE_OK)
    {
        std::cerr << "Unable to set curl CURLOPT_WRITEFUNCTION" << std::endl;
        abort();
    }
    if(curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk)!=CURLE_OK)
    {
        std::cerr << "Unable to set curl CURLOPT_WRITEDATA" << std::endl;
        abort();
    }
    DatapackDownloaderBase::curlPrivateData[curl]=chunk;
    if(DatapackDownloaderBase::DatapackDownloaderBase::curlmCount<10)
    {
        curl_multi_add_handle(DatapackDownloaderBase::curlm, curl);
        DatapackDownloaderBase::DatapackDownloaderBase::curlmCount++;
    }
    else
        DatapackDownloaderBase::curlSuspendList.push_back(curl);
    return true;
}

void DatapackDownloaderBase::datapackDownloadFinishedBase()
{
    std::cout << "DatapackDownloaderBase::datapackDownloadFinishedBase()" << std::endl;
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
        std::cout << "Datapack is not empty and get nothing from serveur because the local datapack hash match with the remote" << std::endl;
        datapackDownloadFinishedBase();
        return;
    }

    if(DatapackDownloaderBase::httpDatapackMirrorBaseList.empty())
    {
        {
            if(remove((mDatapackBase+"/pack/datapack.tar.zst").c_str())!=0 && errno!=ENOENT)
            {
                std::cerr << "Unable to remove " << mDatapackBase << "/pack/datapack.tar.zst" << std::endl;
                abort();
            }
        }
        {
            if(remove((mDatapackBase+"/datapack-list/base.txt").c_str())!=0 && errno!=ENOENT)
            {
                std::cerr << "Unable to remove " << mDatapackBase << "/datapack-list/base.txt" << std::endl;
                abort();
            }
        }
        if(sendedHashBase.empty())
        {
            std::cerr << "Datapack checksum done but not send by the server" << std::endl;
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
                std::cerr << "no client in suspend to do the query to do in protocol datapack download" << std::endl;
                resetAll();
                return;//need CommonSettings::commonSettings.datapackHash send by the server
            }
        }
        std::cerr << "Datapack is empty or hash don't match, get from server, hash local: " << binarytoHexa(hash) << ", hash on server: "
                  << binarytoHexa(CommonSettingsServer::commonSettingsServer.datapackHashServerSub) << std::endl;

        //send the network query
        client->registerOutputQuery(datapack_content_query_number,0xA1);
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
            const std::string &text=datapackFilesListBase.at(index);
            if(!text.empty() && text.size()<255)
            {
                posOutput+=FacilityLibGeneral::toUTF8WithHeader(text,ProtocolParsingBase::tempBigBufferForOutput+posOutput);
            }
            else
                std::cerr << "file name too long or sort: " << text << std::endl;
            index++;
        }
        index=0;
        while(index<datapackFilesListBase.size())
        {
            const std::string &text=datapackFilesListBase.at(index);
            if(!text.empty() && text.size()<255)
            {
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(partialHashList.at(index));
                posOutput+=4;
            }
            else
                std::cerr << "file name too long or sort: " << text << std::endl;
            index++;
        }

        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);//set the dynamic size
        client->sendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
    else
    {
        if(datapackFilesListBase.empty())
        {
            index_mirror_base=0;
            test_mirror_base();
            std::cerr << "Datapack is empty, get from mirror into" << mDatapackBase << std::endl;
        }
        else
        {
            std::cerr << "Datapack don't match with server hash, get from mirror for base: local: " << binarytoHexa(hash) << " and remote: " << binarytoHexa(sendedHashBase) << std::endl;

            if(index_mirror_base>=DatapackDownloaderBase::httpDatapackMirrorBaseList.size())
                index_mirror_base=0;
            const std::string url=DatapackDownloaderBase::httpDatapackMirrorBaseList.at(index_mirror_base)+"pack/diff/datapack-base-"+binarytoHexa(hash)+".tar.zst";

            CURL *curl=curl_easy_init();
            if(!curl)
            {
                std::cerr << "curl_easy_init() failed abort" << std::endl;
                abort();
            }
            if(curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L)!=CURLE_OK)
                std::cerr << "Unable to set the curl keep alive" << std::endl;
            if(curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L)!=CURLE_OK)
                std::cerr << "Unable to set the curl keep alive" << std::endl;
            if(curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L)!=CURLE_OK)
                std::cerr << "Unable to set the curl keep alive" << std::endl;
            struct MemoryStruct chunk;
            chunk.memory = static_cast<char *>(malloc(1));  /* will be grown as needed by the realloc above */
            chunk.size = 0;    /* no data at this point */
            std::cout << "Download: " << url << " into " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
            if(curl_easy_setopt(curl, CURLOPT_URL, url.c_str())!=CURLE_OK)
            {
                std::cerr << "Unable to set the curl url: " << url << std::endl;
                abort();
            }
            if(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, EpollServerLoginSlave::WriteMemoryCallback)!=CURLE_OK)
            {
                std::cerr << "Unable to set curl CURLOPT_WRITEFUNCTION" << std::endl;
                abort();
            }
            if(curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk)!=CURLE_OK)
            {
                std::cerr << "Unable to set curl CURLOPT_WRITEDATA" << std::endl;
                abort();
            }
            const CURLcode res = curl_easy_perform(curl);
            long http_code = 0;
            unsigned int retry=0;
            do
            {
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
                retry++;
            } while(retry<3 && res==CURLE_GOT_NOTHING);
            if(res!=CURLE_OK || http_code!=200)
            {
                std::cerr << "get url " << url << ": " << res << " failed with code " << http_code << ", error string: " << curl_easy_strerror(res) << ", file: " << __FILE__ << ":" << __LINE__ << std::endl;
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
    if(!datapackTarBase)
    {
        const std::string url=DatapackDownloaderBase::httpDatapackMirrorBaseList.at(index_mirror_base)+"pack/datapack.tar.zst";

        struct MemoryStruct chunk;
        chunk.memory = static_cast<char *>(malloc(1));  /* will be grown as needed by the realloc above */
        chunk.size = 0;    /* no data at this point */
        CURL *curl=curl_easy_init();
        if(!curl)
        {
            std::cerr << "curl_easy_init() failed abort" << std::endl;
            abort();
        }
        if(curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L)!=CURLE_OK)
            std::cerr << "Unable to set the curl keep alive" << std::endl;
        if(curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L)!=CURLE_OK)
            std::cerr << "Unable to set the curl keep alive" << std::endl;
        if(curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L)!=CURLE_OK)
            std::cerr << "Unable to set the curl keep alive" << std::endl;
        std::cout << "Download: " << url << " into " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
        if(curl_easy_setopt(curl, CURLOPT_URL, url.c_str())!=CURLE_OK)
        {
            std::cerr << "Unable to set the curl url: " << url << std::endl;
            abort();
        }
        if(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, EpollServerLoginSlave::WriteMemoryCallback)!=CURLE_OK)
        {
            std::cerr << "Unable to set curl CURLOPT_WRITEFUNCTION" << std::endl;
            abort();
        }
        if(curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk)!=CURLE_OK)
        {
            std::cerr << "Unable to set curl CURLOPT_WRITEDATA" << std::endl;
            abort();
        }
        const CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        unsigned int retry=0;
        do
        {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            retry++;
        } while(retry<3 && res==CURLE_GOT_NOTHING);
        if(res!=CURLE_OK || http_code!=200)
        {
            std::cerr << "get url " << url << ": " << res << " failed with code " << http_code << ", error string: " << curl_easy_strerror(res) << ", file: " << __FILE__ << ":" << __LINE__ << std::endl;
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
            /* here and not above because at last mirror you need try the tar.zst and after the datapack-list/base.txt, and only after that's quit */
            return;

        const std::string url=DatapackDownloaderBase::httpDatapackMirrorBaseList.at(index_mirror_base)+"datapack-list/base.txt";

        struct MemoryStruct chunk;
        chunk.memory = static_cast<char *>(malloc(1));  /* will be grown as needed by the realloc above */
        chunk.size = 0;    /* no data at this point */
        CURL *curl=curl_easy_init();
        if(!curl)
        {
            std::cerr << "curl_easy_init() failed abort" << std::endl;
            abort();
        }
        if(curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L)!=CURLE_OK)
            std::cerr << "Unable to set the curl keep alive" << std::endl;
        if(curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L)!=CURLE_OK)
            std::cerr << "Unable to set the curl keep alive" << std::endl;
        if(curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L)!=CURLE_OK)
            std::cerr << "Unable to set the curl keep alive" << std::endl;
        std::cout << "Download: " << url << " into " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
        if(curl_easy_setopt(curl, CURLOPT_URL, url.c_str())!=CURLE_OK)
        {
            std::cerr << "Unable to set the curl url: " << url << std::endl;
            abort();
        }
        if(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, EpollServerLoginSlave::WriteMemoryCallback)!=CURLE_OK)
        {
            std::cerr << "Unable to set curl CURLOPT_WRITEFUNCTION" << std::endl;
            abort();
        }
        if(curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk)!=CURLE_OK)
        {
            std::cerr << "Unable to set curl CURLOPT_WRITEDATA" << std::endl;
            abort();
        }
        const CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        unsigned int retry=0;
        do
        {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            retry++;
        } while(retry<3 && res==CURLE_GOT_NOTHING);
        if(res!=CURLE_OK || http_code!=200)
        {
            std::cerr << "get url " << url << ": " << res << " failed with code " << http_code << ", error string: " << curl_easy_strerror(res) << ", file: " << __FILE__ << ":" << __LINE__ << std::endl;
            httpFinishedForDatapackListBase();
            return;
        }
        std::vector<char> data(chunk.size);
        memcpy(data.data(),chunk.memory,chunk.size);
        httpFinishedForDatapackListBase(data);
        free(chunk.memory);
    }
}

void DatapackDownloaderBase::decodedIsFinishBase(const std::vector<char> &rawData)
{
    std::cout << "DatapackDownloaderBase::decodedIsFinishBase" << std::endl;

    std::vector<char> mDataToDecode=rawData;
    std::string mErrorString;
    {
        std::vector<char> dataToDecoded;
        unsigned long long const rSize = ZSTD_getDecompressedSize(mDataToDecode.data(), mDataToDecode.size());
        if (rSize==ZSTD_CONTENTSIZE_ERROR) {
            mErrorString="it was not compressed by zstd";
        } else if (rSize==ZSTD_CONTENTSIZE_UNKNOWN) {
            mErrorString="original size unknown. Use streaming decompression instead.";
        } else {
            dataToDecoded.resize(rSize);
            size_t const dSize = ZSTD_decompress(dataToDecoded.data(), rSize, mDataToDecode.data(), mDataToDecode.size());
            if (dSize != rSize)
                mErrorString=std::string("error decoding: ")+ZSTD_getErrorName(dSize);
            else
            {
                dataToDecoded.resize(dSize);
                mDataToDecode=dataToDecoded;
            }
        }
    }

    if(!mErrorString.empty())
    {
        std::cerr << "mErrorString: " << mErrorString << std::endl;
        test_mirror_base();
    }
    else
    {
        const std::vector<char> &decodedData=mDataToDecode;
        TarDecode tarDecode;
        if(tarDecode.decodeData(decodedData))
        {
            const std::vector<std::string> &fileList=tarDecode.getFileList();
            const std::vector<std::vector<char> > &dataList=tarDecode.getDataList();
            unsigned int index=0;
            while(index<fileList.size())
            {
                if(!FacilityLibGateway::mkpath(FacilityLibGeneral::getFolderFromFile(mDatapackBase+fileList.at(index))))
                {
                    std::cerr << "unable to mkpath file of datapack " << mDatapackBase+fileList.at(index) << ": " << errno << std::endl;
                    return;
                }

                if(extensionAllowed.find(FacilityLibGeneral::getSuffix(fileList.at(index)))!=extensionAllowed.cend())
                {
                    FILE *file=::fopen((mDatapackBase+fileList.at(index)).c_str(),"wb");
                    if(file)
                    {
                        if(fwrite(dataList.at(index).data(),1,dataList.at(index).size(),file)!=dataList.at(index).size())
                        {
                            fclose(file);
                            std::cerr << "unable to write file content of datapack " << mDatapackBase+fileList.at(index) << ": " << errno << std::endl;
                            return;
                        }
                        fclose(file);
                    }
                    else
                    {
                        std::cerr << "unable to write file of datapack " << mDatapackBase+fileList.at(index) << ": " << errno << std::endl;
                        return;
                    }
                }
                else
                {
                    std::cerr << "file not allowed: " << mDatapackBase+fileList.at(index) << std::endl;
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
    if(datapackTarBase==false)
    {
        datapackTarBase=true;
        test_mirror_base();
    }
    else
    {
        datapackTarBase=false;
        index_mirror_base++;
        if(index_mirror_base>=DatapackDownloaderBase::httpDatapackMirrorBaseList.size())
        {
            std::cerr << "Get the list failed" << std::endl;
            datapackDownloadError();
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
        std::cout << "DatapackDownloaderBase::httpFinishedForDatapackListBase() data emtpy" << std::endl;
        mirrorTryNextBase();
        return;
    }
    else
    {
        if(!datapackTarBase)
        {
            std::cout << "DatapackDownloaderBase::httpFinishedForDatapackListBase() !datapackTarBase" << std::endl;
            std::cerr << "datapack.tar.zst size:" << data.size()/1000 << "KB" << std::endl;
            datapackTarBase=true;
            decodedIsFinishBase(data);
            return;
        }
        else
        {
            std::cout << "DatapackDownloaderBase::httpFinishedForDatapackListBase() datapackTarBase" << std::endl;
            /*ref crash here*/const std::string selectedMirror=DatapackDownloaderBase::httpDatapackMirrorBaseList.at(index_mirror_base);

            httpError=false;

            size_t endOfText;
            {
                std::string text(data.data(),data.size());
                endOfText=text.find("\n-\n");
            }
            if(endOfText==std::string::npos)
            {
                if(data.size()<50)
                    std::cerr << "not text delimitor into file list: " << std::string(data.data(),data.size()) << std::endl;
                else
                    std::cerr << "not text delimitor into file list" << std::endl;
                return;
            }
            std::vector<std::string> content;
            std::vector<char> partialHashListRaw(data.cbegin()+endOfText+3,data.cend());
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

            unsigned int correctContent=0;
            unsigned int index=0;
            while(index<content.size())
            {
                const std::string &line=content.at(index);
                const auto &found=line.find(' ');
                if(found!=std::string::npos)
                {
                    correctContent++;
                    const std::string &fileString=line.substr(0,found);
                    const uint32_t &partialHashString=*reinterpret_cast<uint32_t *>(partialHashListRaw.data()+index*4);
                    //const std::string &sizeString=line.substr(found+1,line.size()-found-1);
                    if(regex_search(fileString,DatapackDownloaderBase::regex_DATAPACK_FILE_REGEX))
                    {
                        int indexInDatapackList=vectorindexOf(datapackFilesListBase,fileString);
                        if(indexInDatapackList!=-1)
                        {
                            const uint32_t &hashFileOnDisk=partialHashListBase.at(indexInDatapackList);
                            if(!FacilityLibGeneral::isFile(mDatapackBase+fileString))
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
                            partialHashListBase.erase(partialHashListBase.cbegin()+indexInDatapackList);
                            datapackFilesListBase.erase(datapackFilesListBase.cbegin()+indexInDatapackList);
                        }
                        else
                        {
                            if(!getHttpFileBase(selectedMirror+fileString,fileString))
                            {
                                datapackDownloadError();
                                return;
                            }
                        }
                    }
                }
                index++;
            }

            int handle_count=0;
            CURLMsg *msg;
            std::unordered_map<std::string,uint8_t> retry;
            do
            {
                /*const CURLMcode res = */curl_multi_perform(DatapackDownloaderBase::curlm, &handle_count);
                int msgs_in_queue=0;
                while((msg = curl_multi_info_read(DatapackDownloaderBase::curlm, &msgs_in_queue)))
                {
                    if(msg->msg == CURLMSG_DONE)
                    {
                        DatapackDownloaderBase::DatapackDownloaderBase::curlmCount--;
                        char *url=NULL;
                        CURL *curl = msg->easy_handle;
                        if(curl==NULL)
                        {
                            std::cerr << "msg->easy_handle==NULL" << ", file: " << __FILE__ << ":" << __LINE__ << std::endl << std::endl;
                            abort();
                        }
                        const CURLcode res = msg->data.result;
                        long http_code = 0;
                        if(curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code)!=CURLE_OK)
                        {
                            std::cerr << "curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code)!=CURLE_OK: " << curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code) << ", file: " << __FILE__ << ":" << __LINE__ << std::endl << std::endl;
                            abort();
                        }
                        if(curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url)!=CURLE_OK)
                        {
                            std::cerr << "curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url)!=CURLE_OK: " << curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url) << ", file: " << __FILE__ << ":" << __LINE__ << std::endl << std::endl;
                            abort();
                        }
                        if(res==CURLE_GOT_NOTHING)
                        {
                            uint8_t count=1;
                            if(retry.find(url)!=retry.cend())
                            {
                                retry[url]++;
                                count=retry.at(url);
                            }
                            else
                                retry[url]=1;
                            if(count<3)
                            {
                                curl_multi_remove_handle(DatapackDownloaderBase::curlm,curl);
                                curl_easy_cleanup(curl);
                                curl_multi_add_handle(DatapackDownloaderBase::curlm, url);
                                DatapackDownloaderBase::DatapackDownloaderBase::curlmCount++;
                                continue;
                            }
                        }
                        if(res!=CURLE_OK || http_code!=200)
                        {
                            httpError=true;
                            std::cerr << "get url " << url << ": " << res << " failed with code " << http_code << ", error string: " << curl_easy_strerror(res) << ", file: " << __FILE__ << ":" << __LINE__ << std::endl;
                            datapackDownloadError();
                            curl_multi_remove_handle(DatapackDownloaderBase::curlm,curl);
                            while((msg = curl_multi_info_read(DatapackDownloaderBase::curlm, &msgs_in_queue)))
                            {
                                MemoryStruct *chunk=NULL;
                                if(DatapackDownloaderBase::curlPrivateData.find(curl)==DatapackDownloaderBase::curlPrivateData.cend())
                                {
                                    std::cerr << "DatapackDownloaderBase::curlPrivateData.find(curl) not found: " << ", file: " << __FILE__ << ":" << __LINE__ << std::endl << std::endl;
                                    abort();
                                }
                                chunk=static_cast<MemoryStruct *>(DatapackDownloaderBase::curlPrivateData.at(curl));
                                DatapackDownloaderBase::curlPrivateData.erase(curl);
                                if(chunk!=NULL)
                                {
                                    if(chunk->memory!=NULL)
                                        delete chunk->memory;
                                    delete chunk;
                                }
                                curl_multi_remove_handle(DatapackDownloaderBase::curlm,curl);
                            }
                            curl_easy_cleanup(curl);
                            return;
                        }
                        std::cout << "Downloaded: " << url << std::endl;
                        numberOfFileWritten++;
                        if(DatapackDownloaderBase::DatapackDownloaderBase::curlmCount<10 && !DatapackDownloaderBase::curlSuspendList.empty())
                        {
                            curl_multi_add_handle(DatapackDownloaderBase::curlm, DatapackDownloaderBase::curlSuspendList.back());
                            DatapackDownloaderBase::DatapackDownloaderBase::curlmCount++;
                            DatapackDownloaderBase::curlSuspendList.pop_back();
                        }

                        MemoryStruct *chunk=NULL;
                        if(DatapackDownloaderBase::curlPrivateData.find(curl)==DatapackDownloaderBase::curlPrivateData.cend())
                        {
                            std::cerr << "DatapackDownloaderBase::curlPrivateData.find(curl) not found: " << ", file: " << __FILE__ << ":" << __LINE__ << std::endl << std::endl;
                            abort();
                        }
                        chunk=static_cast<MemoryStruct *>(DatapackDownloaderBase::curlPrivateData.at(curl));
                        DatapackDownloaderBase::curlPrivateData.erase(curl);

                        if(chunk!=NULL)
                        {
                            if(!FacilityLibGateway::mkpath(FacilityLibGeneral::getFolderFromFile(chunk->fileName)))
                            {
                                std::cerr << "unable to make the path: " << chunk->fileName << ", file: " << __FILE__ << ":" << __LINE__ << std::endl << std::endl;
                                abort();
                            }

                            FILE *fp = fopen(chunk->fileName.c_str(),"wb");
                            if(fp!=NULL)
                            {
                                fwrite(chunk->memory,1,chunk->size,fp);
                                fclose(fp);
                            }
                            else
                            {
                                httpError=true;
                                datapackDownloadError();
                                std::cerr << "unable to open file to write:" << chunk->fileName << std::endl;
                                abort();
                            }

                            if(chunk->memory!=NULL)
                                delete chunk->memory;
                            delete chunk;
                        }
                        else
                        {
                            std::cerr << "chunk is null" << std::endl;
                            abort();
                        }
                        curl_multi_remove_handle(DatapackDownloaderBase::curlm,curl);
                        curl_easy_cleanup(curl);
                    }
                }
                if(handle_count == 0)
                    break;
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            while(handle_count>0 || !DatapackDownloaderBase::curlSuspendList.empty() || DatapackDownloaderBase::curlmCount>0);
            std::cout << "handle_count == 0: leave the curl loop into " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
            curl_multi_cleanup(DatapackDownloaderBase::curlm);

            index=0;
            while(index<datapackFilesListBase.size())
            {
                numberOfFileWritten++;
                if(::remove((mDatapackBase+datapackFilesListBase.at(index)).c_str())!=0)
                {
                    std::cerr << "Unable to remove" << datapackFilesListBase.at(index) << std::endl;
                    abort();
                }
                std::cout << "remove: " << mDatapackBase+datapackFilesListBase.at(index) << std::endl;
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
    if(regex_search(suffix,excludePathBase))
        return std::vector<std::string>();

    std::vector<std::string> returnFile;
    const std::vector<FacilityLibGeneral::InodeDescriptor> &entryList=FacilityLibGeneral::listFolderNotRecursive(mDatapackBase+suffix);//possible wait time here
    int sizeEntryList=entryList.size();
    for(int index=0;index<sizeEntryList;++index)
    {
        const FacilityLibGeneral::InodeDescriptor &fileInfo=entryList.at(index);
        if(fileInfo.type==FacilityLibGeneral::InodeDescriptor::Dir)
        {
            const std::vector<std::string> &newReturnFile=listDatapackBase(suffix+fileInfo.name+'/');
            returnFile.insert(returnFile.cend(),newReturnFile.cbegin(),newReturnFile.cend());//put unix separator because it's transformed into that's under windows too
        }
        else
        {
            //if match with correct file name, considere as valid
            if(regex_search(suffix+fileInfo.name,DatapackDownloaderBase::regex_DATAPACK_FILE_REGEX) && extensionAllowed.find(FacilityLibGeneral::getSuffix(fileInfo.name))!=extensionAllowed.cend())
                returnFile.push_back(suffix+fileInfo.name);
            //is invalid
            else
            {
                numberOfFileWritten++;
                std::cerr << "listDatapack(): remove invalid file: " << suffix << fileInfo.absoluteFilePath << std::endl;
                if(::remove((mDatapackBase+suffix+fileInfo.name).c_str())!=0)
                    std::cerr << "listDatapack(): unable remove invalid file: " << suffix << fileInfo.absoluteFilePath << ": " << errno << std::endl;
                std::cout << "remove: " << mDatapackBase+suffix+fileInfo.name << std::endl;
            }
        }
    }
    std::sort(returnFile.begin(),returnFile.end());
    return returnFile;
}

void DatapackDownloaderBase::cleanDatapackBase(std::string suffix)
{
    std::vector<FacilityLibGeneral::InodeDescriptor> entryList=FacilityLibGeneral::listFolderNotRecursive(mDatapackBase+suffix);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        const FacilityLibGeneral::InodeDescriptor &fileInfo=entryList.at(index);
        if(fileInfo.type==FacilityLibGeneral::InodeDescriptor::Dir)
            cleanDatapackBase(suffix+fileInfo.name+'/');//put unix separator because it's transformed into that's under windows too
        else
            return;
    }
    entryList=FacilityLibGeneral::listFolderNotRecursive(mDatapackBase+suffix);//possible wait time here
    if(entryList.size()==0)
        FacilityLibGeneral::rmpath(mDatapackBase+suffix);
}

void DatapackDownloaderBase::sendDatapackContentBase()
{
    if(wait_datapack_content_base)
    {
        std::cerr << "already in wait of datapack content base" << std::endl;
        return;
    }

    numberOfFileWritten=0;
    index_mirror_base=0;
    datapackTarBase=false;
    wait_datapack_content_base=true;
    FacilityLibGateway::mkpath(mDatapackBase);
    datapackFilesListBase=listDatapackBase(std::string());
    std::sort(datapackFilesListBase.begin(),datapackFilesListBase.end());
    const DatapackChecksum::FullDatapackChecksumReturn &fullDatapackChecksumReturn=DatapackChecksum::doFullSyncChecksumBase(mDatapackBase);
    datapackChecksumDoneBase(fullDatapackChecksumReturn.datapackFilesList,fullDatapackChecksumReturn.hash,fullDatapackChecksumReturn.partialHashList);

}

void DatapackDownloaderBase::sendDatapackProgressionBase(void * client)
{
    if(client==NULL)
    {
        std::cerr << "DatapackDownloaderBase::sendDatapackProgressionBase, client is NULL" << std::endl;
        return;
    }
    EpollClientLoginSlave * const client_real=static_cast<EpollClientLoginSlave *>(client);
    uint8_t progression=0;//do the adaptative from curl progression
    if(clientInSuspend.empty())
        progression=0;//not specific to base client, put 0 as generic value
    client_real->sendDatapackProgression(progression);
}
