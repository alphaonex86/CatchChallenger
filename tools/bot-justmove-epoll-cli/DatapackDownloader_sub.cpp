#include "DatapackDownloaderMainSub.hpp"

using namespace CatchChallenger;

#include <utime.h>
#include <sys/stat.h>
#include <iostream>
#include <cmath>
#include <regex>
#include <thread>
#include <chrono>
#include <utime.h>

#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/GeneralVariable.hpp"
#include "../../general/base/cpp11addition.hpp"
#include "../../general/libzstd/lib/zstd.h"
#include "../../general/xxhash/xxhash.h"
#include "../../client/libcatchchallenger/TarDecode.hpp"
#include "DatapackDownloaderBase.hpp"

void DatapackDownloaderMainSub::writeNewFileSub(const std::string &fileName, const std::string &data)
{
    std::vector<char> newFormat;
    newFormat.resize(data.size());
    memcpy(newFormat.data(),data.data(),data.size());
    if(data.size()>CATCHCHALLENGER_MAX_FILE_SIZE)
    {
        std::cerr << "file too big: " << fileName << std::endl;
        abort();
    }
    std::string fullPath=datapackPathSub()+'/'+fileName;
    stringreplaceAll(fullPath,"//","/");
    //to be sure the QFile is destroyed
    {
        if(!mkpath(FacilityLibGeneral::getFolderFromFile(fullPath)))
        {
            std::cerr << "unable to make the path: " << fullPath << std::endl;
            abort();
        }

        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            FILE *pFile = fopen(fullPath.c_str(),"rb");
            if(pFile!=NULL)
            {
                size_t lSize;
                char * buffer;
                size_t result;

                // obtain file size:
                fseek (pFile , 0 , SEEK_END);
                lSize = ftell (pFile);
                rewind (pFile);

                // allocate memory to contain the whole file:
                buffer = (char*) malloc (sizeof(char)*lSize);
                if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

                // copy the file into the buffer:
                result = fread (buffer,1,lSize,pFile);
                if (result != lSize) {fputs ("Reading error",stderr); exit (3);}

                if(result==data.size())
                {
                    if(memcmp(buffer,data.data(),result)==0)
                    {
                        std::cerr << "duplicate download detected: " << fullPath << ", the file on hdd is same than downloaded file (abort) " << __FILE__ << ":" << __LINE__ << std::endl << std::endl;
                        //abort();
                    }
                }

                fclose(pFile);
                free (buffer);
            }
        }
        #endif
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
        uint32_t h=0;
        XXH32_canonical_t htemp;
        XXH32_canonicalFromHash(&htemp,XXH32(data.data(),data.size(),0));
        memcpy(&h,&htemp.digest,sizeof(h));
        utimbuf butime;butime.actime=h;butime.modtime=h;
        #ifndef CATCHCHALLENGER_EXTRA_CHECK
        utime(fullPath.c_str(),&butime);
        #else
        if(utime(fullPath.c_str(),&butime)!=0)
        {
            std::cerr << "hash cache into modification time set failed (abort) " << __FILE__ << ":" << __LINE__ << std::endl << std::endl;
            abort();
        }
        #endif

        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        struct stat sb;
        if (stat(fullPath.c_str(), &sb) == 0)
        {
            if(sb.st_mtime!=h)
            {
                std::cerr << "hash cache into modification time wrong (abort) " << __FILE__ << ":" << __LINE__ << std::endl << std::endl;
                abort();
            }
        }
        else
        {
            std::cerr << "unable to open modification time of datapack to check the hash (abort) " << __FILE__ << ":" << __LINE__ << std::endl << std::endl;
            abort();
        }
        #endif
    }
}

bool DatapackDownloaderMainSub::getHttpFileSub(const std::string &url, const std::string &fileName)
{
    if(subDatapackCode().empty())
    {
        std::cerr << "subDatapackCode().empty() to get from mirror" << std::endl;
        abort();
    }
    if(httpError)
        return false;
    if(!httpModeSub)
        httpModeSub=true;

    std::string fullPath=datapackPathSub()+'/'+fileName;
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
    if(DatapackDownloaderBase::proxyStringForCurl!=NULL)
    {
        /*curl_easy_setopt(curl, CURLOPT_PROXY, DatapackDownloaderBase::proxyStringForCurl);
        curl_easy_setopt(curl, CURLOPT_PROXYPORT, EpollServerLoginSlave::epollServerLoginSlave->destination_proxy_port);
        curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5_HOSTNAME);*/
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
    if(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback)!=CURLE_OK)
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

void DatapackDownloaderMainSub::datapackDownloadFinishedSub()
{
    haveTheDatapackMainSub();
    resetAll();
}

void DatapackDownloaderMainSub::datapackChecksumDoneSub(const std::vector<std::string> &datapackFilesList,const std::vector<char> &hash,const std::vector<uint32_t> &partialHashList)
{
    if(datapackFilesListSub.size()!=partialHashList.size())
    {
        std::cerr << "datapackFilesListSub.size()!=partialHash.size():" << datapackFilesListSub.size() << "!=" << partialHashList.size() << std::endl;
        std::cerr << "this->datapackFilesList:" << stringimplode(this->datapackFilesListSub,"\n") << std::endl;
        std::cerr << "datapackFilesList:" << stringimplode(datapackFilesList,"\n") << std::endl;
        abort();
    }

    hashSub=hash;
    this->datapackFilesListSub=datapackFilesList;
    this->partialHashListSub=partialHashList;
    if(!datapackFilesListSub.empty() && hash==CommonSettingsServer::commonSettingsServer.datapackHashServerSub)
    {
        std::cout << "Datapack is not empty and get nothing from serveur because the local datapack hash match with the remote" << std::endl;
        datapackDownloadFinishedSub();
        return;
    }

    if(DatapackDownloaderMainSub::httpDatapackMirrorServerList.empty())
    {
        {
            if(remove((datapackPathSub()+"/pack/datapack-sub-"+mainDatapackCode()+"-"+subDatapackCode()+".tar.zst").c_str())!=0 && errno!=ENOENT)
            {
                std::cerr << "Unable to remove " << datapackPathSub() << "/pack/datapack-sub-"+mainDatapackCode()+"-"+subDatapackCode()+".tar.zst" << std::endl;
                abort();
            }
            std::cout << "remove: " << datapackPathSub()+"/pack/datapack-sub-"+mainDatapackCode()+"-"+subDatapackCode()+".tar.zst" << std::endl;
        }
        {
            if(remove((datapackPathSub()+"/datapack-list/sub-"+mainDatapackCode()+"-"+subDatapackCode()+".txt").c_str())!=0 && errno!=ENOENT)
            {
                std::cerr << "Unable to remove " << datapackPathSub() << "/datapack-list/sub-"+mainDatapackCode()+"-"+subDatapackCode()+".txt" << std::endl;
                abort();
            }
            std::cout << "remove: " << datapackPathSub()+"/datapack-list/sub-"+mainDatapackCode()+"-"+subDatapackCode()+".txt" << std::endl;
        }
        if(CommonSettingsServer::commonSettingsServer.datapackHashServerSub.empty())
        {
            std::cerr << "Datapack checksum done but not send by the server" << std::endl;
            abort();//need CommonSettings::commonSettings.datapackHash send by the server
        }
    }
    else
    {
        if(datapackFilesListSub.empty())
        {
            index_mirror_sub=0;
            test_mirror_sub();
            std::cerr << "Datapack is empty, get from mirror into" << datapackPathSub() << std::endl;
        }
        else
        {
            std::cerr << "Datapack don't match with server hash, get from mirror for sub: local: " << binarytoHexa(hash) << " and remote: " << binarytoHexa(CommonSettingsServer::commonSettingsServer.datapackHashServerSub) << std::endl;

            if(index_mirror_sub>=DatapackDownloaderMainSub::httpDatapackMirrorServerList.size())
                index_mirror_sub=0;
            const std::string url=DatapackDownloaderMainSub::httpDatapackMirrorServerList.at(index_mirror_sub)+"pack/diff/datapack-sub-"+binarytoHexa(hash)+".tar.zst";

            struct MemoryStruct chunk;
            chunk.memory = static_cast<char *>(malloc(1));  /* will be grown as needed by the realloc above */
            chunk.size = 0;    /* no data at this point */
            CURL *curl=curl_easy_init();
            if(!curl)
            {
                std::cerr << "curl_easy_init() failed abort" << std::endl;
                abort();
            }
            if(DatapackDownloaderBase::proxyStringForCurl!=NULL)
            {
                /*curl_easy_setopt(curl, CURLOPT_PROXY, DatapackDownloaderBase::proxyStringForCurl);
                curl_easy_setopt(curl, CURLOPT_PROXYPORT, EpollServerLoginSlave::epollServerLoginSlave->destination_proxy_port);
                curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5_HOSTNAME);*/
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
            if(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback)!=CURLE_OK)
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
                httpFinishedForDatapackListSub();
                return;
            }
            std::vector<char> data(chunk.size);
            memcpy(data.data(),chunk.memory,chunk.size);
            httpFinishedForDatapackListSub(data);
            free(chunk.memory);
        }
    }
}

void DatapackDownloaderMainSub::test_mirror_sub()
{
    if(!datapackTarSub)
    {
        const std::string url=DatapackDownloaderMainSub::httpDatapackMirrorServerList.at(index_mirror_sub)+"pack/datapack-sub-"+mainDatapackCode()+"-"+subDatapackCode()+".tar.zst";

        struct MemoryStruct chunk;
        chunk.memory = static_cast<char *>(malloc(1));  /* will be grown as needed by the realloc above */
        chunk.size = 0;    /* no data at this point */
        CURL *curl=curl_easy_init();
        if(!curl)
        {
            std::cerr << "curl_easy_init() failed abort" << std::endl;
            abort();
        }
        if(DatapackDownloaderBase::proxyStringForCurl!=NULL)
        {
            /*curl_easy_setopt(curl, CURLOPT_PROXY, DatapackDownloaderBase::proxyStringForCurl);
            curl_easy_setopt(curl, CURLOPT_PROXYPORT, EpollServerLoginSlave::epollServerLoginSlave->destination_proxy_port);
            curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5_HOSTNAME);*/
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
        if(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback)!=CURLE_OK)
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
            httpFinishedForDatapackListSub();
            return;
        }
        std::vector<char> data(chunk.size);
        memcpy(data.data(),chunk.memory,chunk.size);
        httpFinishedForDatapackListSub(data);
        free(chunk.memory);
    }
    else
    {
        if(index_mirror_sub>=DatapackDownloaderMainSub::httpDatapackMirrorServerList.size())
            /* here and not above because at last mirror you need try the tar.zst and after the datapack-list/sub.txt, and only after that's quit */
            return;

        const std::string url=DatapackDownloaderMainSub::httpDatapackMirrorServerList.at(index_mirror_sub)+"datapack-list/sub-"+mainDatapackCode()+"-"+subDatapackCode()+".txt";

        struct MemoryStruct chunk;
        chunk.memory = static_cast<char *>(malloc(1));  /* will be grown as needed by the realloc above */
        chunk.size = 0;    /* no data at this point */
        CURL *curl=curl_easy_init();
        if(!curl)
        {
            std::cerr << "curl_easy_init() failed abort" << std::endl;
            abort();
        }
        if(DatapackDownloaderBase::proxyStringForCurl!=NULL)
        {
            /*curl_easy_setopt(curl, CURLOPT_PROXY, DatapackDownloaderBase::proxyStringForCurl);
            curl_easy_setopt(curl, CURLOPT_PROXYPORT, EpollServerLoginSlave::epollServerLoginSlave->destination_proxy_port);
            curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5_HOSTNAME);*/
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
        if(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback)!=CURLE_OK)
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
            httpFinishedForDatapackListSub();
            return;
        }
        std::vector<char> data(chunk.size);
        memcpy(data.data(),chunk.memory,chunk.size);
        httpFinishedForDatapackListSub(data);
        free(chunk.memory);
    }
}

void DatapackDownloaderMainSub::decodedIsFinishSub(const std::vector<char> &rawData)
{
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
        test_mirror_sub();
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
                if(!mkpath(FacilityLibGeneral::getFolderFromFile(datapackPathSub()+fileList.at(index))))
                {
                    std::cerr << "unable to mkpath file of datapack " << datapackPathSub()+fileList.at(index) << ": " << errno << std::endl;
                    return;
                }

                if(extensionAllowed.find(FacilityLibGeneral::getSuffix(fileList.at(index)))!=extensionAllowed.cend())
                {
                    FILE *file=::fopen((datapackPathSub()+fileList.at(index)).c_str(),"wb");
                    if(file)
                    {
                        if(fwrite(dataList.at(index).data(),1,dataList.at(index).size(),file)!=dataList.at(index).size())
                        {
                            fclose(file);
                            std::cerr << "unable to write file content of datapack " << datapackPathSub()+fileList.at(index) << ": " << errno << std::endl;
                            return;
                        }
                        fclose(file);
                        uint32_t h=0;
                        XXH32_canonical_t htemp;
                        XXH32_canonicalFromHash(&htemp,XXH32(dataList.at(index).data(),dataList.at(index).size(),0));
                        memcpy(&h,&htemp.digest,sizeof(h));
                        utimbuf butime;butime.actime=h;butime.modtime=h;
                        #ifndef CATCHCHALLENGER_EXTRA_CHECK
                        utime((datapackPathSub()+fileList.at(index)).c_str(),&butime);
                        #else
                        if(utime((datapackPathSub()+fileList.at(index)).c_str(),&butime)!=0)
                        {
                            std::cerr << "hash cache into modification time set failed (abort) " << __FILE__ << ":" << __LINE__ << std::endl << std::endl;
                            abort();
                        }
                        #endif

                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        struct stat sb;
                        if (stat((datapackPathSub()+fileList.at(index)).c_str(), &sb) == 0)
                        {
                            if(sb.st_mtime!=h)
                            {
                                std::cerr << "hash cache into modification time wrong (abort) " << __FILE__ << ":" << __LINE__ << std::endl << std::endl;
                                abort();
                            }
                        }
                        else
                        {
                            std::cerr << "unable to open modification time of datapack to check the hash (abort) " << __FILE__ << ":" << __LINE__ << std::endl << std::endl;
                            abort();
                        }
                        #endif
                    }
                    else
                    {
                        std::cerr << "unable to write file of datapack " << datapackPathSub()+fileList.at(index) << ": " << errno << std::endl;
                        return;
                    }
                }
                else
                {
                    std::cerr << "file not allowed: " << datapackPathSub()+fileList.at(index) << std::endl;
                    return;
                }
                index++;
            }
            wait_datapack_content_sub=false;
            datapackDownloadFinishedSub();
        }
        else
            test_mirror_sub();
    }
}

bool DatapackDownloaderMainSub::mirrorTryNextSub()
{
    if(datapackTarSub==false)
    {
        datapackTarSub=true;
        test_mirror_sub();
    }
    else
    {
        datapackTarSub=false;
        index_mirror_sub++;
        if(index_mirror_sub>=DatapackDownloaderMainSub::httpDatapackMirrorServerList.size())
        {
            std::cerr << "Get the list failed" << std::endl;
            datapackDownloadError();
            return false;
        }
        else
            test_mirror_sub();
    }
    return true;
}

void DatapackDownloaderMainSub::httpFinishedForDatapackListSub(const std::vector<char> data)
{
    if(data.empty())
    {
        mirrorTryNextSub();
        return;
    }
    else
    {
        if(!datapackTarSub)
        {
            std::cerr << "datapack-sub-"+mainDatapackCode()+"-"+subDatapackCode()+".tar.zst size:" << data.size()/1000 << "KB" << std::endl;
            datapackTarSub=true;
            decodedIsFinishSub(data);
            return;
        }
        else
        {
            /*ref crash here*/const std::string selectedMirror=DatapackDownloaderMainSub::httpDatapackMirrorServerList.at(index_mirror_sub)+"map/main/"+mainDatapackCode()+"/sub/"+subDatapackCode()+"/";

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
                        int indexInDatapackList=vectorindexOf(datapackFilesListSub,fileString);
                        if(indexInDatapackList!=-1)
                        {
                            const uint32_t &hashFileOnDisk=partialHashListSub.at(indexInDatapackList);
                            if(!FacilityLibGeneral::isFile(datapackPathSub()+fileString))
                            {
                                if(!getHttpFileSub(selectedMirror+fileString,fileString))
                                {
                                    datapackDownloadError();
                                    return;
                                }
                            }
                            else if(hashFileOnDisk!=partialHashString)
                            {
                                if(!getHttpFileSub(selectedMirror+fileString,fileString))
                                {
                                    datapackDownloadError();
                                    return;
                                }
                            }
                            partialHashListSub.erase(partialHashListSub.cbegin()+indexInDatapackList);
                            datapackFilesListSub.erase(datapackFilesListSub.cbegin()+indexInDatapackList);
                        }
                        else
                        {
                            if(!getHttpFileSub(selectedMirror+fileString,fileString))
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
                                if(DatapackDownloaderBase::proxyStringForCurl!=NULL)
                                {
                                    /*curl_easy_setopt(curl, CURLOPT_PROXY, DatapackDownloaderBase::proxyStringForCurl);
                                    curl_easy_setopt(curl, CURLOPT_PROXYPORT, EpollServerLoginSlave::epollServerLoginSlave->destination_proxy_port);
                                    curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5_HOSTNAME);*/
                                }
                                DatapackDownloaderBase::DatapackDownloaderBase::curlmCount++;
                                continue;
                            }
                        }
                        if(res!=CURLE_OK || http_code!=200)
                        {
                            httpError=true;
                            std::cerr << "get url " << url << ": " << res << " failed with code " << http_code << ", error string: " << curl_easy_strerror(res) << ", file: " << __FILE__ << ":" << __LINE__ << std::endl;
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
                            datapackDownloadError();
                            return;
                        }
                        std::cout << "Downloaded: " << url << std::endl;
                        if(DatapackDownloaderBase::DatapackDownloaderBase::curlmCount<10 && !DatapackDownloaderBase::curlSuspendList.empty())
                        {
                            curl_multi_add_handle(DatapackDownloaderBase::curlm, DatapackDownloaderBase::curlSuspendList.back());
                            if(DatapackDownloaderBase::proxyStringForCurl!=NULL)
                            {
                                /*curl_easy_setopt(DatapackDownloaderBase::curlSuspendList.back(), CURLOPT_PROXY, DatapackDownloaderBase::proxyStringForCurl);
                                curl_easy_setopt(DatapackDownloaderBase::curlSuspendList.back(), CURLOPT_PROXYPORT, EpollServerLoginSlave::epollServerLoginSlave->destination_proxy_port);
                                curl_easy_setopt(DatapackDownloaderBase::curlSuspendList.back(), CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5_HOSTNAME);*/
                            }
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
                            if(!mkpath(FacilityLibGeneral::getFolderFromFile(chunk->fileName)))
                            {
                                std::cerr << "unable to make the path: " << chunk->fileName << ", file: " << __FILE__ << ":" << __LINE__ << std::endl << std::endl;
                                abort();
                            }

                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            {
                                FILE *pFile = fopen(chunk->fileName.c_str(),"rb");
                                if(pFile!=NULL)
                                {
                                    size_t lSize;
                                    char * buffer;
                                    size_t result;

                                    // obtain file size:
                                    fseek (pFile , 0 , SEEK_END);
                                    lSize = ftell (pFile);
                                    rewind (pFile);

                                    // allocate memory to contain the whole file:
                                    buffer = (char*) malloc (sizeof(char)*lSize);
                                    if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

                                    // copy the file into the buffer:
                                    result = fread (buffer,1,lSize,pFile);
                                    if (result != lSize) {fputs ("Reading error",stderr); exit (3);}

                                    if(result==chunk->size)
                                    {
                                        if(memcmp(buffer,chunk->memory,result)==0)
                                        {
                                            std::cerr << "duplicate download detected: " << chunk->fileName << ", the file on hdd is same than downloaded file (abort) " << __FILE__ << ":" << __LINE__ << std::endl << std::endl;
                                            //abort();
                                        }
                                    }

                                    fclose(pFile);
                                    free (buffer);
                                }
                            }
                            #endif
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
                            uint32_t h=0;
                            XXH32_canonical_t htemp;
                            XXH32_canonicalFromHash(&htemp,XXH32(chunk->memory,chunk->size,0));
                            memcpy(&h,&htemp.digest,sizeof(h));
                            utimbuf butime;butime.actime=h;butime.modtime=h;
                            #ifndef CATCHCHALLENGER_EXTRA_CHECK
                            utime(chunk->fileName.c_str(),&butime);
                            #else
                            if(utime(chunk->fileName.c_str(),&butime)!=0)
                            {
                                std::cerr << "hash cache into modification time set failed (abort) " << __FILE__ << ":" << __LINE__ << std::endl << std::endl;
                                abort();
                            }
                            #endif

                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            struct stat sb;
                            if (stat(chunk->fileName.c_str(), &sb) == 0)
                            {
                                if(sb.st_mtime!=h)
                                {
                                    std::cerr << "hash cache into modification time wrong (abort) " << __FILE__ << ":" << __LINE__ << std::endl << std::endl;
                                    abort();
                                }
                            }
                            else
                            {
                                std::cerr << "unable to open modification time of datapack to check the hash (abort) " << __FILE__ << ":" << __LINE__ << std::endl << std::endl;
                                abort();
                            }
                            #endif

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
            std::cout << "handle_count == 0: leave the curl loop" << " into " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
            curl_multi_cleanup(DatapackDownloaderBase::curlm);

            index=0;
            while(index<datapackFilesListSub.size())
            {
                if(::remove((datapackPathSub()+datapackFilesListSub.at(index)).c_str())!=0)
                {
                    std::cerr << "Unable to remove" << datapackFilesListSub.at(index) << std::endl;
                    abort();
                }
                std::cout << "remove: " << datapackPathSub()+datapackFilesListSub.at(index) << std::endl;
                index++;
            }
            datapackFilesListSub.clear();
            if(correctContent==0)
            {
                std::cerr << "Error, no valid content: correctContent==0\n" << stringimplode(content,'\n') << std::endl;
                return;
            }
            datapackDownloadFinishedSub();
        }
    }
}

const std::vector<std::string> DatapackDownloaderMainSub::listDatapackSub(std::string suffix)
{
    if(regex_search(suffix,excludePathMain))
        return std::vector<std::string>();

    std::vector<std::string> returnFile;
    const std::vector<FacilityLibGeneral::InodeDescriptor> &entryList=FacilityLibGeneral::listFolderNotRecursive(datapackPathSub()+suffix);//possible wait time here
    int sizeEntryList=entryList.size();
    for(int index=0;index<sizeEntryList;++index)
    {
        const FacilityLibGeneral::InodeDescriptor &fileInfo=entryList.at(index);
        if(fileInfo.type==FacilityLibGeneral::InodeDescriptor::Dir)
        {
            const std::vector<std::string> &newReturnFile=listDatapackSub(suffix+fileInfo.name+'/');
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
                if(::remove((datapackPathSub()+suffix+fileInfo.name).c_str())!=0)
                    std::cerr << "listDatapack(): unable remove invalid file: " << suffix << fileInfo.absoluteFilePath << ": " << errno << std::endl;
                std::cout << "remove: " <<datapackPathSub()+suffix+fileInfo.name << std::endl;
            }
        }
    }
    std::sort(returnFile.begin(),returnFile.end());
    return returnFile;
}

void DatapackDownloaderMainSub::cleanDatapackSub(std::string suffix)
{
    std::vector<FacilityLibGeneral::InodeDescriptor> entryList=FacilityLibGeneral::listFolderNotRecursive(datapackPathSub()+suffix);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        const FacilityLibGeneral::InodeDescriptor &fileInfo=entryList.at(index);
        if(fileInfo.type==FacilityLibGeneral::InodeDescriptor::Dir)
            cleanDatapackSub(suffix+fileInfo.name+'/');//put unix separator because it's transformed into that's under windows too
        else
            return;
    }
    entryList=FacilityLibGeneral::listFolderNotRecursive(datapackPathSub()+suffix);//possible wait time here
    if(entryList.size()==0)
        FacilityLibGeneral::rmpath(datapackPathSub()+suffix);
}

void DatapackDownloaderMainSub::sendDatapackContentSub()
{
    if(wait_datapack_content_sub)
    {
        std::cerr << "already in wait of datapack content sub" << std::endl;
        return;
    }

    index_mirror_sub=0;
    datapackTarSub=false;
    wait_datapack_content_sub=true;
    mkpath(datapackPathSub());
    datapackFilesListSub=listDatapackSub(std::string());
    std::sort(datapackFilesListSub.begin(),datapackFilesListSub.end());
    const DatapackChecksum::FullDatapackChecksumReturn &fullDatapackChecksumReturn=DatapackChecksum::doFullSyncChecksumSub(datapackPathSub());
    datapackChecksumDoneSub(fullDatapackChecksumReturn.datapackFilesList,fullDatapackChecksumReturn.hash,fullDatapackChecksumReturn.partialHashList);
}
