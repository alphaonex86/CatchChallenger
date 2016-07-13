#include "DatapackDownloaderMainSub.h"

using namespace CatchChallenger;

#include <iostream>
#include <cmath>
#include <regex>
#include <thread>
#include <chrono>

#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../client/base/qt-tar-xz/QTarDecode.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/cpp11addition.h"
#include "LinkToGameServer.h"
#include "EpollServerLoginSlave.h"
#include "FacilityLibGateway.h"
#include "DatapackDownloaderBase.h"

void DatapackDownloaderMainSub::writeNewFileMain(const std::string &fileName,const std::vector<char> &data)
{
    if(data.size()>CATCHCHALLENGER_MAX_FILE_SIZE)
    {
        std::cerr << "file too big: " << fileName << std::endl;
        abort();
    }
    std::string fullPath=mDatapackMain+'/'+fileName;
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

bool DatapackDownloaderMainSub::getHttpFileMain(const std::string &url, const std::string &fileName)
{
    if(httpError)
        return false;
    if(!httpModeMain)
        httpModeMain=true;

    std::string fullPath=mDatapackMain+'/'+fileName;
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

void DatapackDownloaderMainSub::datapackChecksumDoneMain(const std::vector<std::string> &datapackFilesList,const std::vector<char> &hash,const std::vector<uint32_t> &partialHashList)
{
    if(datapackFilesListMain.size()!=partialHashList.size())
    {
        std::cerr << "datapackFilesListMain.size()!=partialHash.size():" << datapackFilesListMain.size() << "!=" << partialHashList.size() << std::endl;
        std::cerr << "this->datapackFilesList:" << stringimplode(this->datapackFilesListMain,"\n") << std::endl;
        std::cerr << "datapackFilesList:" << stringimplode(datapackFilesList,"\n") << std::endl;
        abort();
    }

    hashMain=hash;
    this->datapackFilesListMain=datapackFilesList;
    this->partialHashListMain=partialHashList;
    if(!datapackFilesListMain.empty() && hash==sendedHashMain)
    {
        std::cout << "Datapack is not empty and get nothing from serveur because the local datapack hash match with the remote" << std::endl;
        checkIfContinueOrFinished();
        return;
    }

    if(DatapackDownloaderMainSub::httpDatapackMirrorServerList.empty())
    {
        {
            if(remove((mDatapackMain+"/pack/datapack.tar.xz").c_str())!=0 && errno!=ENOENT)
            {
                std::cerr << "Unable to remove " << mDatapackMain << "/pack/datapack.tar.xz" << std::endl;
                abort();
            }
        }
        {
            if(remove((mDatapackMain+"/datapack-list/Main.txt").c_str())!=0 && errno!=ENOENT)
            {
                std::cerr << "Unable to remove " << mDatapackMain << "/datapack-list/main.txt" << std::endl;
                abort();
            }
        }
        if(sendedHashMain.empty())
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
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(datapackFilesListMain.size());
        posOutput+=4;
        unsigned int index=0;
        while(index<datapackFilesListMain.size())
        {
            {
                const std::string &text=datapackFilesListMain.at(index);
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
        if(datapackFilesListMain.empty())
        {
            index_mirror_main=0;
            test_mirror_main();
            std::cerr << "Datapack is empty, get from mirror into" << mDatapackMain << std::endl;
        }
        else
        {
            std::cerr << "Datapack don't match with server hash, get from mirror" << std::endl;

            const std::string url=DatapackDownloaderMainSub::httpDatapackMirrorServerList.at(index_mirror_main)+"pack/diff/datapack-main-"+binarytoHexa(hash)+".tar.xz";

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
            std::cout << "Download: " << url << std::endl;
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
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            if(res!=CURLE_OK || http_code!=200)
            {
                std::cerr << "get url " << url << ": " << res << " failed with code " << http_code << ", error string: " << curl_easy_strerror(res) << ", file: " << __FILE__ << ":" << __LINE__ << std::endl;
                httpFinishedForDatapackListMain();
                return;
            }
            std::vector<char> data(chunk.size);
            memcpy(data.data(),chunk.memory,chunk.size);
            httpFinishedForDatapackListMain(data);
            free(chunk.memory);
        }
    }
}

void DatapackDownloaderMainSub::test_mirror_main()
{
    if(!datapackTarXzMain)
    {
        const std::string url=DatapackDownloaderMainSub::httpDatapackMirrorServerList.at(index_mirror_main)+"pack/datapack.tar.xz";

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
        std::cout << "Download: " << url << std::endl;
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
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if(res!=CURLE_OK || http_code!=200)
        {
            std::cerr << "get url " << url << ": " << res << " failed with code " << http_code << ", error string: " << curl_easy_strerror(res) << ", file: " << __FILE__ << ":" << __LINE__ << std::endl;
            httpFinishedForDatapackListMain();
            return;
        }
        std::vector<char> data(chunk.size);
        memcpy(data.data(),chunk.memory,chunk.size);
        httpFinishedForDatapackListMain(data);
        free(chunk.memory);
    }
    else
    {
        if(index_mirror_main>=DatapackDownloaderMainSub::httpDatapackMirrorServerList.size())
            /* here and not above because at last mirror you need try the tar.xz and after the datapack-list/main.txt, and only after that's quit */
            return;

        const std::string url=DatapackDownloaderMainSub::httpDatapackMirrorServerList.at(index_mirror_main)+"datapack-list/main.txt";

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
        std::cout << "Download: " << url << std::endl;
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
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if(res!=CURLE_OK || http_code!=200)
        {
            std::cerr << "get url " << url << ": " << res << " failed with code " << http_code << ", error string: " << curl_easy_strerror(res) << ", file: " << __FILE__ << ":" << __LINE__ << std::endl;
            httpFinishedForDatapackListMain();
            return;
        }
        std::vector<char> data(chunk.size);
        memcpy(data.data(),chunk.memory,chunk.size);
        httpFinishedForDatapackListMain(data);
        free(chunk.memory);
    }
}

void DatapackDownloaderMainSub::decodedIsFinishMain(QXzDecode &xzDecodeMain)
{
    if(!xzDecodeMain.decode())
        test_mirror_main();
    else
    {
        const std::vector<char> &decodedData=xzDecodeMain.decodedData();
        QTarDecode tarDecode;
        if(tarDecode.decodeData(decodedData))
        {
            const std::vector<std::string> &fileList=tarDecode.getFileList();
            const std::vector<std::vector<char> > &dataList=tarDecode.getDataList();
            unsigned int index=0;
            while(index<fileList.size())
            {
                if(!FacilityLibGateway::mkpath(FacilityLibGeneral::getFolderFromFile(mDatapackMain+fileList.at(index))))
                {
                    std::cerr << "unable to mkpath file of datapack " << mDatapackMain+fileList.at(index) << ": " << errno << std::endl;
                    return;
                }

                if(extensionAllowed.find(FacilityLibGeneral::getSuffix(fileList.at(index)))!=extensionAllowed.cend())
                {
                    FILE *file=::fopen((mDatapackMain+fileList.at(index)).c_str(),"wb");
                    if(file)
                    {
                        if(fwrite(dataList.at(index).data(),1,dataList.at(index).size(),file)!=dataList.at(index).size())
                        {
                            fclose(file);
                            std::cerr << "unable to write file content of datapack " << mDatapackMain+fileList.at(index) << ": " << errno << std::endl;
                            return;
                        }
                        fclose(file);
                    }
                    else
                    {
                        std::cerr << "unable to write file of datapack " << mDatapackMain+fileList.at(index) << ": " << errno << std::endl;
                        return;
                    }
                }
                else
                {
                    std::cerr << "file not allowed: " << mDatapackMain+fileList.at(index) << std::endl;
                    return;
                }
                index++;
            }
            wait_datapack_content_main=false;
            checkIfContinueOrFinished();
        }
        else
            test_mirror_main();
    }
}

bool DatapackDownloaderMainSub::mirrorTryNextMain()
{
    if(datapackTarXzMain==false)
    {
        datapackTarXzMain=true;
        test_mirror_main();
    }
    else
    {
        datapackTarXzMain=false;
        index_mirror_main++;
        if(index_mirror_main>=DatapackDownloaderMainSub::httpDatapackMirrorServerList.size())
        {
            std::cerr << "Get the list failed" << std::endl;
            datapackDownloadError();
            return false;
        }
        else
            test_mirror_main();
    }
    return true;
}

void DatapackDownloaderMainSub::httpFinishedForDatapackListMain(const std::vector<char> data)
{
    if(data.empty())
    {
        mirrorTryNextMain();
        return;
    }
    else
    {
        if(!datapackTarXzMain)
        {
            std::cerr << "datapack.tar.xz size:" << data.size()/1000 << "KB" << std::endl;
            datapackTarXzMain=true;
            QXzDecode xzDecodeMain(data,16*1024*1024);
            decodedIsFinishMain(xzDecodeMain);
            return;
        }
        else
        {
            /*ref crash here*/const std::string selectedMirror=DatapackDownloaderMainSub::httpDatapackMirrorServerList.at(index_mirror_main);

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
                    if(regex_search(fileString,DatapackDownloaderMainSub::regex_DATAPACK_FILE_REGEX))
                    {
                        int indexInDatapackList=vectorindexOf(datapackFilesListMain,fileString);
                        if(indexInDatapackList!=-1)
                        {
                            const uint32_t &hashFileOnDisk=partialHashListMain.at(indexInDatapackList);
                            if(!FacilityLibGeneral::isFile(mDatapackMain+fileString))
                            {
                                if(!getHttpFileMain(selectedMirror+fileString,fileString))
                                {
                                    datapackDownloadError();
                                    return;
                                }
                            }
                            else if(hashFileOnDisk!=partialHashString)
                            {
                                if(!getHttpFileMain(selectedMirror+fileString,fileString))
                                {
                                    datapackDownloadError();
                                    return;
                                }
                            }
                            partialHashListMain.erase(partialHashListMain.cbegin()+indexInDatapackList);
                            datapackFilesListMain.erase(datapackFilesListMain.cbegin()+indexInDatapackList);
                        }
                        else
                        {
                            if(!getHttpFileMain(selectedMirror+fileString,fileString))
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
                            std::cerr << "curl_easy_getinfo(curl, CURLINFO_PRIVATE,chunk) is null" << std::endl;
                            abort();
                        }
                        curl_multi_remove_handle(DatapackDownloaderBase::curlm,curl);
                        curl_easy_cleanup(curl);
                    }
                }
                if(handle_count == 0)
                    break;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            while(handle_count>0 || !DatapackDownloaderBase::curlSuspendList.empty() || DatapackDownloaderBase::curlmCount>0);
            std::cout << "handle_count == 0: leave the curl loop" << std::endl;
            curl_multi_cleanup(DatapackDownloaderBase::curlm);

            index=0;
            while(index<datapackFilesListMain.size())
            {
                if(::remove((mDatapackMain+datapackFilesListMain.at(index)).c_str())!=0)
                {
                    std::cerr << "Unable to remove" << datapackFilesListMain.at(index) << std::endl;
                    abort();
                }
                index++;
            }
            datapackFilesListMain.clear();
            if(correctContent==0)
            {
                std::cerr << "Error, no valid content: correctContent==0\n" << stringimplode(content,'\n') << std::endl;
                return;
            }
            checkIfContinueOrFinished();
        }
    }
}

const std::vector<std::string> DatapackDownloaderMainSub::listDatapackMain(std::string suffix)
{
    if(regex_search(suffix,excludePathMain))
        return std::vector<std::string>();

    std::vector<std::string> returnFile;
    const std::vector<FacilityLibGeneral::InodeDescriptor> &entryList=FacilityLibGeneral::listFolderNotRecursive(mDatapackMain+suffix);//possible wait time here
    int sizeEntryList=entryList.size();
    for(int index=0;index<sizeEntryList;++index)
    {
        const FacilityLibGeneral::InodeDescriptor &fileInfo=entryList.at(index);
        if(fileInfo.type==FacilityLibGeneral::InodeDescriptor::Dir)
        {
            const std::vector<std::string> &newReturnFile=listDatapackMain(suffix+fileInfo.name+'/');
            returnFile.insert(returnFile.cend(),newReturnFile.cbegin(),newReturnFile.cend());//put unix separator because it's transformed into that's under windows too
        }
        else
        {
            //if match with correct file name, considere as valid
            if(regex_search(suffix+fileInfo.name,DatapackDownloaderMainSub::regex_DATAPACK_FILE_REGEX) && extensionAllowed.find(FacilityLibGeneral::getSuffix(fileInfo.name))!=extensionAllowed.cend())
                returnFile.push_back(suffix+fileInfo.name);
            //is invalid
            else
            {
                std::cerr << "listDatapack(): remove invalid file: " << suffix << fileInfo.absoluteFilePath << std::endl;
                if(::remove((mDatapackMain+suffix+fileInfo.name).c_str())!=0)
                    std::cerr << "listDatapack(): unable remove invalid file: " << suffix << fileInfo.absoluteFilePath << ": " << errno << std::endl;
            }
        }
    }
    std::sort(returnFile.begin(),returnFile.end());
    return returnFile;
}

void DatapackDownloaderMainSub::cleanDatapackMain(std::string suffix)
{
    std::vector<FacilityLibGeneral::InodeDescriptor> entryList=FacilityLibGeneral::listFolderNotRecursive(mDatapackMain+suffix);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        const FacilityLibGeneral::InodeDescriptor &fileInfo=entryList.at(index);
        if(fileInfo.type==FacilityLibGeneral::InodeDescriptor::Dir)
            cleanDatapackMain(suffix+fileInfo.name+'/');//put unix separator because it's transformed into that's under windows too
        else
            return;
    }
    entryList=FacilityLibGeneral::listFolderNotRecursive(mDatapackMain+suffix);//possible wait time here
    if(entryList.size()==0)
        FacilityLibGeneral::rmpath(mDatapackMain+suffix);
}

void DatapackDownloaderMainSub::sendDatapackContentMain()
{
    if(wait_datapack_content_main)
    {
        std::cerr << "already in wait of datapack content" << std::endl;
        return;
    }

    datapackTarXzMain=false;
    wait_datapack_content_main=true;
    FacilityLibGateway::mkpath(mDatapackMain);
    datapackFilesListMain=listDatapackMain(std::string());
    std::sort(datapackFilesListMain.begin(),datapackFilesListMain.end());
    const DatapackChecksum::FullDatapackChecksumReturn &fullDatapackChecksumReturn=DatapackChecksum::doFullSyncChecksumMain(mDatapackMain);
    datapackChecksumDoneMain(fullDatapackChecksumReturn.datapackFilesList,fullDatapackChecksumReturn.hash,fullDatapackChecksumReturn.partialHashList);
}
