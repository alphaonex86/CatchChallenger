#include "../Client.hpp"
#include "../GlobalServerData.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include <sys/stat.h>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>

// MinGW/Windows lacks O_CLOEXEC (it's POSIX). Define it as 0 so the
// `O_RDONLY|O_CLOEXEC` calls compile cleanly on the Windows cross-build.
// On Windows the close-on-exec semantic is achieved differently anyway
// (HANDLE inheritance flag); ::open() ignores the bit either way.
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif
#ifndef CATCHCHALLENGER_SERVER_NO_COMPRESSION
#include <zstd.h>
#endif

/// \todo solve disconnecting/destroy during the SQL loading

using namespace CatchChallenger;

#ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
catchchallenger_datapack_map<std::string, BaseServerMasterSendDatapack::DatapackCacheFile> Client::datapack_file_list_cached_base()
{
    if(!CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.empty())
        return catchchallenger_datapack_map<std::string, BaseServerMasterSendDatapack::DatapackCacheFile>();
    if(GlobalServerData::serverSettings.datapackCache==-1)
    {
        #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
        if(BaseServerMasterSendDatapack::extensionAllowed.empty())
        {
            const std::vector<std::string> &extensionAllowedTemp=stringsplit(std::string(CATCHCHALLENGER_EXTENSION_ALLOWED+std::string(";")+CATCHCHALLENGER_EXTENSION_COMPRESSED),';');
            BaseServerMasterSendDatapack::extensionAllowed=catchchallenger_datapack_set<std::string>(extensionAllowedTemp.begin(),extensionAllowedTemp.end());
        }
        #endif
        return datapack_file_list(GlobalServerData::serverSettings.datapack_basePath,"map/main/");
    }
    else if(GlobalServerData::serverSettings.datapackCache==0)
    {
        if(Client::datapack_list_cache_timestamp_base==0)
        {
            #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
            if(BaseServerMasterSendDatapack::extensionAllowed.empty())
            {
                const std::vector<std::string> &extensionAllowedTemp=stringsplit(std::string(CATCHCHALLENGER_EXTENSION_ALLOWED+std::string(";")+CATCHCHALLENGER_EXTENSION_COMPRESSED),';');
                BaseServerMasterSendDatapack::extensionAllowed=catchchallenger_datapack_set<std::string>(extensionAllowedTemp.begin(),extensionAllowedTemp.end());
            }
            #endif
            Client::datapack_list_cache_timestamp_base=sFrom1970();
            BaseServerMasterSendDatapack::datapack_file_hash_cache_base=datapack_file_list(GlobalServerData::serverSettings.datapack_basePath,"map/main/");
            #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
            BaseServerMasterSendDatapack::extensionAllowed.clear();
            #endif
        }
        return BaseServerMasterSendDatapack::datapack_file_hash_cache_base;
    }
    else
    {
        const uint64_t &currentTime=sFrom1970();
        if(Client::datapack_list_cache_timestamp_base<(currentTime-GlobalServerData::serverSettings.datapackCache))
        {
            Client::datapack_list_cache_timestamp_base=currentTime;
            BaseServerMasterSendDatapack::datapack_file_hash_cache_base=datapack_file_list(GlobalServerData::serverSettings.datapack_basePath,"map/main/");
        }
        return BaseServerMasterSendDatapack::datapack_file_hash_cache_base;
    }
}

catchchallenger_datapack_map<std::string, BaseServerMasterSendDatapack::DatapackCacheFile> Client::datapack_file_list_cached_main()
{
    if(GlobalServerData::serverSettings.datapackCache==-1)
    {
        #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
        if(BaseServerMasterSendDatapack::extensionAllowed.empty())
        {
            const std::vector<std::string> &extensionAllowedTemp=stringsplit(std::string(CATCHCHALLENGER_EXTENSION_ALLOWED+std::string(";")+CATCHCHALLENGER_EXTENSION_COMPRESSED),';');
            BaseServerMasterSendDatapack::extensionAllowed=catchchallenger_datapack_set<std::string>(extensionAllowedTemp.begin(),extensionAllowedTemp.end());
        }
        #endif
        return datapack_file_list(GlobalServerData::serverPrivateVariables.mainDatapackFolder,"sub/");
    }
    else if(GlobalServerData::serverSettings.datapackCache==0)
    {
        if(Client::datapack_list_cache_timestamp_main==0)
        {
            #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
            if(BaseServerMasterSendDatapack::extensionAllowed.empty())
            {
                const std::vector<std::string> &extensionAllowedTemp=stringsplit(std::string(CATCHCHALLENGER_EXTENSION_ALLOWED+std::string(";")+CATCHCHALLENGER_EXTENSION_COMPRESSED),';');
                BaseServerMasterSendDatapack::extensionAllowed=catchchallenger_datapack_set<std::string>(extensionAllowedTemp.begin(),extensionAllowedTemp.end());
            }
            #endif
            Client::datapack_list_cache_timestamp_main=sFrom1970();
            Client::datapack_file_hash_cache_main=datapack_file_list(GlobalServerData::serverPrivateVariables.mainDatapackFolder,"sub/");
            #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
            BaseServerMasterSendDatapack::extensionAllowed.clear();
            #endif
        }
        return Client::datapack_file_hash_cache_main;
    }
    else
    {
        const uint64_t &currentTime=sFrom1970();
        if(Client::datapack_list_cache_timestamp_main<(currentTime-GlobalServerData::serverSettings.datapackCache))
        {
            Client::datapack_list_cache_timestamp_main=currentTime;
            Client::datapack_file_hash_cache_main=datapack_file_list(GlobalServerData::serverPrivateVariables.mainDatapackFolder,"sub/");
        }
        return Client::datapack_file_hash_cache_main;
    }
}

catchchallenger_datapack_map<std::string, BaseServerMasterSendDatapack::DatapackCacheFile> Client::datapack_file_list_cached_sub()
{
    if(GlobalServerData::serverSettings.datapackCache==-1)
    {
        #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
        if(BaseServerMasterSendDatapack::extensionAllowed.empty())
        {
            const std::vector<std::string> &extensionAllowedTemp=stringsplit(std::string(CATCHCHALLENGER_EXTENSION_ALLOWED+std::string(";")+CATCHCHALLENGER_EXTENSION_COMPRESSED),';');
            BaseServerMasterSendDatapack::extensionAllowed=catchchallenger_datapack_set<std::string>(extensionAllowedTemp.begin(),extensionAllowedTemp.end());
        }
        #endif
        return datapack_file_list(GlobalServerData::serverPrivateVariables.subDatapackFolder,"");
    }
    else if(GlobalServerData::serverSettings.datapackCache==0)
    {
        if(Client::datapack_list_cache_timestamp_sub==0)
        {
            #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
            if(BaseServerMasterSendDatapack::extensionAllowed.empty())
            {
                const std::vector<std::string> &extensionAllowedTemp=stringsplit(std::string(CATCHCHALLENGER_EXTENSION_ALLOWED+std::string(";")+CATCHCHALLENGER_EXTENSION_COMPRESSED),';');
                BaseServerMasterSendDatapack::extensionAllowed=catchchallenger_datapack_set<std::string>(extensionAllowedTemp.begin(),extensionAllowedTemp.end());
            }
            #endif
            Client::datapack_list_cache_timestamp_sub=sFrom1970();
            Client::datapack_file_hash_cache_sub=datapack_file_list(GlobalServerData::serverPrivateVariables.subDatapackFolder,"");
            #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
            BaseServerMasterSendDatapack::extensionAllowed.clear();
            #endif
        }
        return Client::datapack_file_hash_cache_sub;
    }
    else
    {
        const uint64_t &currentTime=sFrom1970();
        if(Client::datapack_list_cache_timestamp_sub<(currentTime-GlobalServerData::serverSettings.datapackCache))
        {
            Client::datapack_list_cache_timestamp_sub=currentTime;
            Client::datapack_file_hash_cache_sub=datapack_file_list(GlobalServerData::serverPrivateVariables.subDatapackFolder,"");
        }
        return Client::datapack_file_hash_cache_sub;
    }
}

//check each element of the datapack, determine if need be removed, updated, add as new file all the missing file
void Client::datapackList(const uint8_t &query_id,const std::vector<std::string> &files,const std::vector<uint32_t> &partialHashList)
{
    #ifdef CATCHCHALLENGER_HARDENED
    /// \see Client::parseFullQuery() already checked here
    #endif
    tempDatapackListReplyArray.clear();
    tempDatapackListReplyTestCount=0;
    //Reset the per-batch pending lists. swap-with-empty actually frees
    //the heap storage from any prior batch that errored mid-flush.
    std::vector<BaseServerMasterSendDatapack::PendingFile>().swap(BaseServerMasterSendDatapack::rawFilesPending);
    BaseServerMasterSendDatapack::rawFilesPendingRawSize=0;
    #ifndef CATCHCHALLENGER_SERVER_NO_COMPRESSION
    std::vector<BaseServerMasterSendDatapack::PendingFile>().swap(BaseServerMasterSendDatapack::compressedFilesPending);
    BaseServerMasterSendDatapack::compressedFilesPendingRawSize=0;
    #endif
    tempDatapackListReply=0;
    tempDatapackListReplySize=0;
    std::string datapackPath;
    catchchallenger_datapack_map<std::string,BaseServerMasterSendDatapack::DatapackCacheFile> filesList;
    switch(datapackStatus)
    {
        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        case DatapackStatus::Base:
            filesList=datapack_file_list_cached_base();
            datapackPath=GlobalServerData::serverSettings.datapack_basePath;
            if(filesList.empty())
            {
                std::cerr << "datapack_file_list_cached_base() can't return empty list" << std::endl;
                abort();
            }
        break;
        #endif
        case DatapackStatus::Main:
            filesList=datapack_file_list_cached_main();
            datapackPath=GlobalServerData::serverPrivateVariables.mainDatapackFolder;
            if(filesList.empty())
            {
                std::cerr << "datapack_file_list_cached_main() can't return empty list" << std::endl;
                abort();
            }
        break;
        case DatapackStatus::Sub:
            filesList=datapack_file_list_cached_sub();
            datapackPath=GlobalServerData::serverPrivateVariables.subDatapackFolder;
        break;
        default:
        return;
    }
    std::vector<FileToSend> fileToSendList;

    uint32_t fileToDelete=0;
    const size_t &loop_size=files.size();
    //send the size to download on the client
    {
        //clone to drop the ask file and remain the missing client files
        catchchallenger_datapack_map<std::string,BaseServerMasterSendDatapack::DatapackCacheFile> filesListForSize(filesList);
        unsigned int index=0;
        uint32_t datapckFileNumber=0;
        uint32_t datapckFileSize=0;
        while(index<loop_size)
        {
            const std::string &fileName=files.at(index);
            const uint32_t &clientPartialHash=partialHashList.at(index);
            if(fileName.find("./")!=std::string::npos)
            {
                errorOutput("file name contains illegale char (1): "+fileName);
                return;
            }
            if(fileName.find("\\")!=std::string::npos)
            {
                errorOutput("file name contains illegale char (2): "+fileName);
                return;
            }
            if(fileName.find("//")!=std::string::npos)
            {
                errorOutput("file name contains illegale char (3): "+fileName);
                return;
            }
            if(regex_search(fileName,fileNameStartStringRegex) || stringStartWith(fileName,"/"))
            {
                errorOutput("start with wrong string: "+fileName);
                return;
            }
            if(!regex_search(fileName,GlobalServerData::serverPrivateVariables.datapack_rightFileName))
            {
                errorOutput("file name sended wrong: "+fileName);
                return;
            }
            if(filesListForSize.find(fileName)!=filesListForSize.cend())
            {
                const uint32_t &serverPartialHash=filesListForSize.at(fileName).partialHash;
                #ifdef CATCHCHALLENGER_HARDENED
                if(serverPartialHash==0)
                {
                    errorOutput("serverPartialHash==0 at " __FILE__ ":"+std::to_string(__LINE__));
                    abort();
                }
                #endif
                if(clientPartialHash==serverPartialHash)
                    addDatapackListReply(false);//file found don't need be updated
                else
                {
                    //todo: be sure at the startup sll the file is readable
                    struct stat buf;
                    if(::stat((datapackPath+fileName).c_str(),&buf)!=-1)
                    {
                        addDatapackListReply(false);//found but need an update
                        datapckFileNumber++;
                        datapckFileSize+=buf.st_size;
                        FileToSend fileToSend;
                        fileToSend.file=fileName;
                        fileToSendList.push_back(fileToSend);
                    }
                }
                filesListForSize.erase(fileName);
            }
            else
            {
                addDatapackListReply(true);//to delete
                fileToDelete++;
            }
            index++;
        }
        catchchallenger_datapack_map<std::string,BaseServerMasterSendDatapack::DatapackCacheFile>::iterator i=filesListForSize.begin();
        while(i!=filesListForSize.cend())
        {
            std::string fullPathFileToOpen=datapackPath+i->first;
            #ifdef Q_OS_WIN32
            stringreplaceAll(fullPathFileToOpen,"/","\\");
            #endif
            FILE *filedesc=fopen(fullPathFileToOpen.c_str(),"rb");
            if(filedesc!=NULL)
            {
                struct stat buf;
                if(::stat((datapackPath+i->first).c_str(),&buf)!=-1)
                {
                    datapckFileNumber++;
                    datapckFileSize+=buf.st_size;
                    FileToSend fileToSend;
                    fileToSend.file=i->first;
                    fileToSendList.push_back(fileToSend);
                }
                fclose(filedesc);
            }
            ++i;
        }
        {
            //send the network message
            uint32_t posOutput=0;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x75;
            posOutput+=1;

            {const uint32_t _tmp_le=(htole32(datapckFileNumber));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

            posOutput+=4;
            {const uint32_t _tmp_le=(htole32(datapckFileSize));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

            posOutput+=4;

            sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        }
    }
    if(fileToSendList.empty() && fileToDelete==0)
    {
        switch(datapackStatus)
        {
            #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
            case DatapackStatus::Base:
                errorOutput("ERROR or DDOS: Checksum send previously to the client, then the client should have see it match (and have already the same version of datapack than the server) and not ask again the files list: Base");
            break;
            #endif
            case DatapackStatus::Main:
                errorOutput("ERROR or DDOS: Checksum send previously to the client, then the client should have see it match (and have already the same version of datapack than the server) and not ask again the files list: Main");
            break;
            case DatapackStatus::Sub:
                errorOutput("ERROR or DDOS: Checksum send previously to the client, then the client should have see it match (and have already the same version of datapack than the server) and not ask again the files list: Sub");
            break;
            default:
            return;
        }
        return;
    }
    std::sort(fileToSendList.begin(),fileToSendList.end());
    //validate, remove or update the file actualy on the client
    if(tempDatapackListReplyTestCount!=files.size())
    {
        errorOutput("Bit count return not match");
        return;
    }
    //send not in the list
    {
        unsigned int index=0;
        while(index<fileToSendList.size())
        {
            if(!sendFile(datapackPath,fileToSendList.at(index).file))
                return;
            index++;
        }
    }
    sendFileContent();
    sendCompressedFileContent();
    purgeDatapackListReply(query_id);

    switch(datapackStatus)
    {
        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        case DatapackStatus::Base:
            datapackStatus=DatapackStatus::Main;
        break;
        #endif
        case DatapackStatus::Main:
            datapackStatus=DatapackStatus::Sub;
        break;
        case DatapackStatus::Sub:
            datapackStatus=DatapackStatus::Finished;
        break;
        default:
        return;
    }
}

void Client::addDatapackListReply(const bool &fileRemove)
{
    tempDatapackListReplyTestCount++;
    switch(tempDatapackListReplySize)
    {
        case 0:
            if(fileRemove)
                tempDatapackListReply|=0x01;
            else
                tempDatapackListReply&=~0x01;
        break;
        case 1:
            if(fileRemove)
                tempDatapackListReply|=0x02;
            else
                tempDatapackListReply&=~0x02;
        break;
        case 2:
            if(fileRemove)
                tempDatapackListReply|=0x04;
            else
                tempDatapackListReply&=~0x04;
        break;
        case 3:
            if(fileRemove)
                tempDatapackListReply|=0x08;
            else
                tempDatapackListReply&=~0x08;
        break;
        case 4:
            if(fileRemove)
                tempDatapackListReply|=0x10;
            else
                tempDatapackListReply&=~0x10;
        break;
        case 5:
            if(fileRemove)
                tempDatapackListReply|=0x20;
            else
                tempDatapackListReply&=~0x20;
        break;
        case 6:
            if(fileRemove)
                tempDatapackListReply|=0x40;
            else
                tempDatapackListReply&=~0x40;
        break;
        case 7:
            if(fileRemove)
                tempDatapackListReply|=0x80;
            else
                tempDatapackListReply&=~0x80;
        break;
        default:
        break;
    }
    tempDatapackListReplySize++;
    if(tempDatapackListReplySize>=8)
    {
        tempDatapackListReplyArray.push_back(tempDatapackListReply);
        tempDatapackListReplySize=0;
        tempDatapackListReply=0;
    }
}

void Client::purgeDatapackListReply(const uint8_t &query_id)
{
    if(tempDatapackListReplySize>0)
    {
        tempDatapackListReplyArray.push_back(tempDatapackListReply);
        tempDatapackListReplySize=0;
        tempDatapackListReply=0;
    }
    if(tempDatapackListReplyArray.empty())
        tempDatapackListReplyArray.push_back(0x00);

    {
        //send the network reply
        removeFromQueryReceived(query_id);
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
        posOutput+=1+4;
        {const uint32_t _tmp_le=(htole32(tempDatapackListReplyArray.size()));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1,&_tmp_le,sizeof(_tmp_le));}//set the dynamic size

        if(tempDatapackListReplyArray.size()>64*1024)
        {
            errorOutput("Client::purgeDatapackListReply too big to reply");
            return;
        }
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,tempDatapackListReplyArray.data(),tempDatapackListReplyArray.size());
        posOutput+=tempDatapackListReplyArray.size();

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
    tempDatapackListReplyArray.clear();
}

void Client::sendFileContent()
{
    //Zero-copy raw datapack send. Walks the pending list, computes the
    //total packet body size up-front (fstat-derived), writes the small
    //packet/per-file headers via sendRawBlock, and streams each file's
    //content via sendfile(2) (kernel page-cache -> socket, no userspace
    //buffer). Replaces the former in-RAM concatenation in
    //BaseServerMasterSendDatapack::rawFilesBuffer.
    std::vector<BaseServerMasterSendDatapack::PendingFile> &pending=
            BaseServerMasterSendDatapack::rawFilesPending;
    if(pending.empty())
        return;

    //bodySize = 1 (count) + sum(1+namelen+4+filesize)
    uint64_t bodySize=1;
    {
        unsigned int i=0;
        while(i<pending.size())
        {
            bodySize+=1+pending[i].name.size()+4+pending[i].size;
            i++;
        }
    }
    //Whole packet must still fit one wire packet.
    if(bodySize+1+4>CATCHCHALLENGER_MAX_PACKET_SIZE)
    {
        errorOutput("Client::sendFileContent too big to reply");
        std::vector<BaseServerMasterSendDatapack::PendingFile>().swap(pending);
        BaseServerMasterSendDatapack::rawFilesPendingRawSize=0;
        return;
    }

    //Packet header: [0x76][bodySize_le32][count_u8]
    char hdr[1+4+1];
    hdr[0]=0x76;
    {const uint32_t le=htole32(static_cast<uint32_t>(bodySize));memcpy(hdr+1,&le,sizeof(le));}
    hdr[5]=static_cast<uint8_t>(pending.size());
    if(!sendRawBlock(hdr,sizeof(hdr)))
    {
        std::vector<BaseServerMasterSendDatapack::PendingFile>().swap(pending);
        BaseServerMasterSendDatapack::rawFilesPendingRawSize=0;
        return;
    }

    //One file at a time, in protocol order.
    unsigned int i=0;
    while(i<pending.size())
    {
        const BaseServerMasterSendDatapack::PendingFile &p=pending[i];
        //Per-file metadata: [namelen_u8][name][filesize_le32]
        char meta[1+255+4];
        size_t mp=0;
        meta[mp]=static_cast<uint8_t>(p.name.size());
        mp+=1;
        memcpy(meta+mp,p.name.data(),p.name.size());
        mp+=p.name.size();
        {const uint32_t le=htole32(p.size);memcpy(meta+mp,&le,sizeof(le));}
        mp+=4;
        if(!sendRawBlock(meta,static_cast<unsigned int>(mp)))
            break;

        //Open + sendfile + close. fopen/fclose are not needed; raw fd is
        //all we want for sendfile(2).
        const int fd=::open(p.fullPath.c_str(),O_RDONLY|O_CLOEXEC);
        if(fd<0)
        {
            //We've already committed bytes for the next file's content
            //to the wire; the only safe thing left is to drop the link.
            errorOutput("sendFileContent: open() failed for "+p.fullPath);
            disconnectClient();
            break;
        }
        off_t off=0;
        size_t remaining=p.size;
        while(remaining>0)
        {
            const ssize_t sent=writeFileToSocket(fd,&off,remaining);
            if(sent<=0)
            {
                if(sent==0)
                    break;//EOF; file shorter than fstat? caller error
                errorOutput("sendFileContent: writeFileToSocket() failed");
                ::close(fd);
                disconnectClient();
                pending.clear();
                BaseServerMasterSendDatapack::rawFilesPendingRawSize=0;
                return;
            }
            remaining-=static_cast<size_t>(sent);
        }
        ::close(fd);
        i++;
    }

    //Free pending list — matches the user's "free related buffer when
    //finished sending datapack" rule.
    std::vector<BaseServerMasterSendDatapack::PendingFile>().swap(pending);
    BaseServerMasterSendDatapack::rawFilesPendingRawSize=0;
}

#ifndef CATCHCHALLENGER_SERVER_NO_COMPRESSION
void Client::sendCompressedFileContent()
{
    //Streaming compression for the 0x77 packet. Replaces the former
    //"accumulate raw bytes in compressedFilesBuffer, then ZSTD_compress
    //in one shot" approach. Now the raw content NEVER lives in RAM:
    //we feed each file in protocol order through ZSTD_compressStream2
    //chunk-by-chunk (one shared 64 KB read scratch). Output goes into
    //a single bounded heap vector that we hand to sendRawBlock and free
    //on return.
    std::vector<BaseServerMasterSendDatapack::PendingFile> &pending=
            BaseServerMasterSendDatapack::compressedFilesPending;
    if(pending.empty())
        return;

    if(CompressionProtocol::compressionTypeServer==CompressionProtocol::CompressionType::None)
    {
        //Compression disabled at runtime — caller should have routed
        //this through the raw path instead.
        errorOutput("sendCompressedFileContent called with compression off");
        std::vector<BaseServerMasterSendDatapack::PendingFile>().swap(pending);
        BaseServerMasterSendDatapack::compressedFilesPendingRawSize=0;
        return;
    }

    ZSTD_CStream * const cctx=ZSTD_createCStream();
    if(cctx==nullptr)
    {
        errorOutput("ZSTD_createCStream failed");
        std::vector<BaseServerMasterSendDatapack::PendingFile>().swap(pending);
        BaseServerMasterSendDatapack::compressedFilesPendingRawSize=0;
        return;
    }
    ZSTD_initCStream(cctx,CompressionProtocol::compressionLevel);

    //Output buffer holds the compressed payload only. ZSTD_compressBound
    //is the worst case for the raw input; in practice the compressed
    //size is much smaller. We only ever appended bytes to this vector;
    //it never gets read or memcpy'd elsewhere, so growth is fine.
    std::vector<uint8_t> compressed;
    compressed.reserve(ZSTD_compressBound(BaseServerMasterSendDatapack::compressedFilesPendingRawSize));

    //64 KB shared read scratch. One file at a time -> sequential reads,
    //no per-file buffer fan-out (matches the user's preference: avoid
    //growing memory by parallelising reads).
    char readBuf[64*1024];
    uint8_t outBuf[64*1024];

    bool failed=false;
    {
        unsigned int i=0;
        while(i<pending.size() && !failed)
        {
            const BaseServerMasterSendDatapack::PendingFile &p=pending[i];

            //Per-file prefix: [namelen_u8][name][filesize_le32]. Push
            //through the stream as raw input bytes (the prefix bytes
            //get compressed too — receiver decompresses then parses).
            char meta[1+255+4];
            size_t mp=0;
            meta[mp]=static_cast<uint8_t>(p.name.size());
            mp+=1;
            memcpy(meta+mp,p.name.data(),p.name.size());
            mp+=p.name.size();
            {const uint32_t le=htole32(p.size);memcpy(meta+mp,&le,sizeof(le));}
            mp+=4;

            ZSTD_inBuffer inMeta={meta,mp,0};
            while(inMeta.pos<inMeta.size && !failed)
            {
                ZSTD_outBuffer out={outBuf,sizeof(outBuf),0};
                const size_t r=ZSTD_compressStream2(cctx,&out,&inMeta,ZSTD_e_continue);
                if(ZSTD_isError(r))
                {
                    errorOutput(std::string("ZSTD_compressStream2 (meta): ")+ZSTD_getErrorName(r));
                    failed=true;
                    break;
                }
                if(out.pos>0)
                    compressed.insert(compressed.end(),outBuf,outBuf+out.pos);
            }
            if(failed)
                break;

            //File content, chunked. Open + read + close per file; never
            //hold more than one fd or one chunk at a time.
            const int fd=::open(p.fullPath.c_str(),O_RDONLY|O_CLOEXEC);
            if(fd<0)
            {
                errorOutput("sendCompressedFileContent: open() failed for "+p.fullPath);
                failed=true;
                break;
            }
            uint32_t remaining=p.size;
            while(remaining>0 && !failed)
            {
                const size_t chunk=remaining<sizeof(readBuf) ? remaining : sizeof(readBuf);
                const ssize_t got=::read(fd,readBuf,chunk);
                if(got<=0)
                {
                    errorOutput("sendCompressedFileContent: read() short for "+p.fullPath);
                    failed=true;
                    break;
                }
                ZSTD_inBuffer in={readBuf,static_cast<size_t>(got),0};
                while(in.pos<in.size && !failed)
                {
                    ZSTD_outBuffer out={outBuf,sizeof(outBuf),0};
                    const size_t r=ZSTD_compressStream2(cctx,&out,&in,ZSTD_e_continue);
                    if(ZSTD_isError(r))
                    {
                        errorOutput(std::string("ZSTD_compressStream2 (data): ")+ZSTD_getErrorName(r));
                        failed=true;
                        break;
                    }
                    if(out.pos>0)
                        compressed.insert(compressed.end(),outBuf,outBuf+out.pos);
                }
                remaining-=static_cast<uint32_t>(got);
            }
            ::close(fd);
            i++;
        }
    }

    //Flush the frame.
    while(!failed)
    {
        ZSTD_inBuffer in={nullptr,0,0};
        ZSTD_outBuffer out={outBuf,sizeof(outBuf),0};
        const size_t r=ZSTD_compressStream2(cctx,&out,&in,ZSTD_e_end);
        if(ZSTD_isError(r))
        {
            errorOutput(std::string("ZSTD_compressStream2 (end): ")+ZSTD_getErrorName(r));
            failed=true;
            break;
        }
        if(out.pos>0)
            compressed.insert(compressed.end(),outBuf,outBuf+out.pos);
        if(r==0)
            break;//frame finalised
    }
    ZSTD_freeCStream(cctx);

    if(!failed)
    {
        //bodySize = 1 (count) + compressed.size().
        const uint64_t bodySize=1+compressed.size();
        if(bodySize+1+4>CATCHCHALLENGER_MAX_PACKET_SIZE)
            errorOutput("Client::sendCompressedFileContent too big to reply");
        else
        {
            //Header [0x77][bodySize_le32][count_u8] then the compressed
            //payload. Two sendRawBlock() calls — receiver sees one
            //continuous packet on the TCP stream.
            char hdr[1+4+1];
            hdr[0]=0x77;
            {const uint32_t le=htole32(static_cast<uint32_t>(bodySize));memcpy(hdr+1,&le,sizeof(le));}
            hdr[5]=static_cast<uint8_t>(pending.size());
            if(sendRawBlock(hdr,sizeof(hdr)))
                sendRawBlock(reinterpret_cast<const char *>(compressed.data()),
                             static_cast<unsigned int>(compressed.size()));
        }
    }

    //Free buffers regardless of failure.
    std::vector<uint8_t>().swap(compressed);
    std::vector<BaseServerMasterSendDatapack::PendingFile>().swap(pending);
    BaseServerMasterSendDatapack::compressedFilesPendingRawSize=0;
}
#endif

bool Client::sendFile(const std::string &datapackPath,const std::string &fileName)
{
    if(fileName.size()>255 || fileName.empty())
    {
        errorOutput("Unable to open into CatchChallenger::sendFile(): fileName.size()>255 || fileName.empty()");
        return false;
    }

    //Stat-only path: the file content is no longer materialised in
    //userspace here. We only need {fullPath, name, size} so the eventual
    //flush (sendFileContent / sendCompressedFileContent) can stream the
    //file through sendfile(2) or ZSTD_compressStream2.
    std::string fullPathFileToOpen=datapackPath+fileName;
    #ifdef Q_OS_WIN32
    stringreplaceAll(fullPathFileToOpen,"/","\\");
    #endif
    struct stat st;
    if(::stat(fullPathFileToOpen.c_str(),&st)!=0)
    {
        errorOutput("Unable to stat into CatchChallenger::sendFile(): "+fileName);
        return false;
    }
    const uint32_t contentsize=static_cast<uint32_t>(st.st_size);
    const std::string &suffix=FacilityLibGeneral::getSuffix(fileName);
    #ifndef CATCHCHALLENGER_SERVER_NO_COMPRESSION
    if(CompressionProtocol::compressionTypeServer!=CompressionProtocol::CompressionType::None &&
            BaseServerMasterSendDatapack::compressedExtension.find(suffix)!=BaseServerMasterSendDatapack::compressedExtension.cend() &&
            (
                contentsize<CATCHCHALLENGER_SERVER_DATAPACK_DONT_COMPRESS_GREATER_THAN_KB*1024
                ||
                contentsize>CATCHCHALLENGER_MAX_PACKET_SIZE
            )
        )
    {
        BaseServerMasterSendDatapack::PendingFile pf;
        pf.fullPath=fullPathFileToOpen;
        pf.name=fileName;
        pf.size=contentsize;
        BaseServerMasterSendDatapack::compressedFilesPending.push_back(pf);
        BaseServerMasterSendDatapack::compressedFilesPendingRawSize+=1+fileName.size()+4+contentsize;
        if(BaseServerMasterSendDatapack::compressedFilesPendingRawSize>CATCHCHALLENGER_SERVER_DATAPACK_ZLIB_COMPRESSEDFILEPURGE_KB*1024
                || BaseServerMasterSendDatapack::compressedFilesPending.size()>=255)
            sendCompressedFileContent();
    }
    else
    #endif
    {
        if(contentsize>CATCHCHALLENGER_SERVER_DATAPACK_MIN_FILEPURGE_KB*1024)
        {
            if((1+fileName.size()+4+contentsize)>=CATCHCHALLENGER_MAX_PACKET_SIZE)
            {
                normalOutput("Error: outputData2(1)+fileNameRaw("+
                             std::to_string(fileName.size()+4)+
                             ")+content("+
                             std::to_string(contentsize)+
                             ")>CATCHCHALLENGER_MAX_PACKET_SIZE for file "+
                             fileName
                             );
                return false;
            }
            //Big-but-fits-one-packet file: route through the same pending
            //list as a count=1 batch, then flush immediately. This way
            //the sendfile(2) path is the single code path for raw data.
            //Flush any prior pending bundle first to preserve ordering.
            if(!BaseServerMasterSendDatapack::rawFilesPending.empty())
                sendFileContent();
            BaseServerMasterSendDatapack::PendingFile pf;
            pf.fullPath=fullPathFileToOpen;
            pf.name=fileName;
            pf.size=contentsize;
            BaseServerMasterSendDatapack::rawFilesPending.push_back(pf);
            BaseServerMasterSendDatapack::rawFilesPendingRawSize+=1+fileName.size()+4+contentsize;
            sendFileContent();
        }
        else
        {
            BaseServerMasterSendDatapack::PendingFile pf;
            pf.fullPath=fullPathFileToOpen;
            pf.name=fileName;
            pf.size=contentsize;
            BaseServerMasterSendDatapack::rawFilesPending.push_back(pf);
            BaseServerMasterSendDatapack::rawFilesPendingRawSize+=1+fileName.size()+4+contentsize;
            if(BaseServerMasterSendDatapack::rawFilesPendingRawSize>CATCHCHALLENGER_SERVER_DATAPACK_MAX_FILEPURGE_KB*1024
                    || BaseServerMasterSendDatapack::rawFilesPending.size()>=255)
                sendFileContent();
        }
    }
    return true;
}
#endif
