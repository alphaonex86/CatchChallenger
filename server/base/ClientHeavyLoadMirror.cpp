#include "Client.hpp"
#include "GlobalServerData.hpp"
#include "StaticText.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include <sys/stat.h>
#include <cstring>

/// \todo solve disconnecting/destroy during the SQL loading

using namespace CatchChallenger;

#ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
std::unordered_map<std::string, BaseServerMasterSendDatapack::DatapackCacheFile> Client::datapack_file_list_cached_base()
{
    if(!CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.empty())
        return std::unordered_map<std::string, BaseServerMasterSendDatapack::DatapackCacheFile>();
    if(GlobalServerData::serverSettings.datapackCache==-1)
        return datapack_file_list(GlobalServerData::serverSettings.datapack_basePath,"map/main/");
    else if(GlobalServerData::serverSettings.datapackCache==0)
    {
        if(Client::datapack_list_cache_timestamp_base==0)
        {
            Client::datapack_list_cache_timestamp_base=sFrom1970();
            BaseServerMasterSendDatapack::datapack_file_hash_cache_base=datapack_file_list(GlobalServerData::serverSettings.datapack_basePath,"map/main/");
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

std::unordered_map<std::string, BaseServerMasterSendDatapack::DatapackCacheFile> Client::datapack_file_list_cached_main()
{
    if(GlobalServerData::serverSettings.datapackCache==-1)
        return datapack_file_list(GlobalServerData::serverPrivateVariables.mainDatapackFolder,"sub/");
    else if(GlobalServerData::serverSettings.datapackCache==0)
    {
        if(Client::datapack_list_cache_timestamp_main==0)
        {
            Client::datapack_list_cache_timestamp_main=sFrom1970();
            Client::datapack_file_hash_cache_main=datapack_file_list(GlobalServerData::serverPrivateVariables.mainDatapackFolder,"sub/");
        }
        return Client::datapack_file_hash_cache_main;
    }
    else
    {
        const auto &currentTime=sFrom1970();
        if(Client::datapack_list_cache_timestamp_main<(currentTime-GlobalServerData::serverSettings.datapackCache))
        {
            Client::datapack_list_cache_timestamp_main=currentTime;
            Client::datapack_file_hash_cache_main=datapack_file_list(GlobalServerData::serverPrivateVariables.mainDatapackFolder,"sub/");
        }
        return Client::datapack_file_hash_cache_main;
    }
}

std::unordered_map<std::string, BaseServerMasterSendDatapack::DatapackCacheFile> Client::datapack_file_list_cached_sub()
{
    if(GlobalServerData::serverSettings.datapackCache==-1)
        return datapack_file_list(GlobalServerData::serverPrivateVariables.subDatapackFolder,"");
    else if(GlobalServerData::serverSettings.datapackCache==0)
    {
        if(Client::datapack_list_cache_timestamp_sub==0)
        {
            Client::datapack_list_cache_timestamp_sub=sFrom1970();
            Client::datapack_file_hash_cache_sub=datapack_file_list(GlobalServerData::serverPrivateVariables.subDatapackFolder,"");
        }
        return Client::datapack_file_hash_cache_sub;
    }
    else
    {
        const auto &currentTime=sFrom1970();
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
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    /// \see Client::parseFullQuery() already checked here
    #endif
    tempDatapackListReplyArray.clear();
    tempDatapackListReplyTestCount=0;
    BaseServerMasterSendDatapack::rawFilesBuffer.clear();
    BaseServerMasterSendDatapack::compressedFilesBuffer.clear();
    BaseServerMasterSendDatapack::rawFilesBufferCount=0;
    BaseServerMasterSendDatapack::compressedFilesBufferCount=0;
    tempDatapackListReply=0;
    tempDatapackListReplySize=0;
    std::string datapackPath;
    std::unordered_map<std::string,BaseServerMasterSendDatapack::DatapackCacheFile> filesList;
    switch(datapackStatus)
    {
        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        case DatapackStatus::Base:
            filesList=datapack_file_list_cached_base();
            datapackPath=GlobalServerData::serverSettings.datapack_basePath;
        break;
        #endif
        case DatapackStatus::Main:
            filesList=datapack_file_list_cached_main();
            datapackPath=GlobalServerData::serverPrivateVariables.mainDatapackFolder;
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
        std::unordered_map<std::string,BaseServerMasterSendDatapack::DatapackCacheFile> filesListForSize(filesList);
        unsigned int index=0;
        uint32_t datapckFileNumber=0;
        uint32_t datapckFileSize=0;
        while(index<loop_size)
        {
            const std::string &fileName=files.at(index);
            const uint32_t &clientPartialHash=partialHashList.at(index);
            if(fileName.find(StaticText::text_dotslash) != std::string::npos)
            {
                errorOutput("file name contains illegale char (1): "+fileName);
                return;
            }
            if(fileName.find(StaticText::text_antislash) != std::string::npos)
            {
                errorOutput("file name contains illegale char (2): "+fileName);
                return;
            }
            if(fileName.find(StaticText::text_double_slash) != std::string::npos)
            {
                errorOutput("file name contains illegale char (3): "+fileName);
                return;
            }
            if(regex_search(fileName,fileNameStartStringRegex) || stringStartWith(fileName,StaticText::text_slash))
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
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
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
        auto i=filesListForSize.begin();
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

            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(datapckFileNumber);
            posOutput+=4;
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(datapckFileSize);
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
                errorOutput("Ask datapack list where the checksum match Base");
            break;
            #endif
            case DatapackStatus::Main:
                errorOutput("Ask datapack list where the checksum match Main");
            break;
            case DatapackStatus::Sub:
                errorOutput("Ask datapack list where the checksum match Sub");
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
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(tempDatapackListReplyArray.size());//set the dynamic size

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
    if(BaseServerMasterSendDatapack::rawFilesBuffer.size()>0 && BaseServerMasterSendDatapack::rawFilesBufferCount>0)
    {
        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x76;
        posOutput+=1+4;

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=BaseServerMasterSendDatapack::rawFilesBufferCount;
        posOutput+=1;
        if(BaseServerMasterSendDatapack::rawFilesBuffer.size()>CATCHCHALLENGER_MAX_PACKET_SIZE)
        {
            errorOutput("Client::sendFileContent too big to reply");
            return;
        }
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,BaseServerMasterSendDatapack::rawFilesBuffer.data(),BaseServerMasterSendDatapack::rawFilesBuffer.size());
        posOutput+=BaseServerMasterSendDatapack::rawFilesBuffer.size();

        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

        BaseServerMasterSendDatapack::rawFilesBuffer.clear();
        BaseServerMasterSendDatapack::rawFilesBufferCount=0;
    }
}

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
void Client::sendCompressedFileContent()
{
    if(BaseServerMasterSendDatapack::compressedFilesBuffer.size()>0 && BaseServerMasterSendDatapack::compressedFilesBufferCount>0)
    {
        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x77;
        posOutput+=1+4;

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(BaseServerMasterSendDatapack::compressedFilesBufferCount);
        posOutput+=1;
        if(BaseServerMasterSendDatapack::compressedFilesBuffer.size()>CATCHCHALLENGER_MAX_PACKET_SIZE)
        {
            errorOutput("Client::sendFileContent too big to reply");
            return;
        }

        const uint32_t &compressedSize=CompressionProtocol::computeCompression(
                    BaseServerMasterSendDatapack::compressedFilesBuffer.data(),
                    ProtocolParsingBase::tempBigBufferForOutput+posOutput,
                    static_cast<uint32_t>(BaseServerMasterSendDatapack::compressedFilesBuffer.size()),
                    sizeof(ProtocolParsingBase::tempBigBufferForOutput)-posOutput,
                    CompressionProtocol::compressionTypeServer
                    );
        posOutput+=compressedSize;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

        BaseServerMasterSendDatapack::compressedFilesBuffer.clear();
        BaseServerMasterSendDatapack::compressedFilesBufferCount=0;
    }
}
#endif

bool Client::sendFile(const std::string &datapackPath,const std::string &fileName)
{
    if(fileName.size()>255 || fileName.empty())
    {
        errorOutput("Unable to open into CatchChallenger::sendFile(): fileName.size()>255 || fileName.empty()");
        return false;
    }

    std::string fullPathFileToOpen=datapackPath+fileName;
    #ifdef Q_OS_WIN32
    stringreplaceAll(fullPathFileToOpen,"/","\\");
    #endif
    FILE *filedesc=fopen(fullPathFileToOpen.c_str(),"rb");
    if(filedesc!=NULL)
    {
        const std::vector<char> &content=FacilityLibGeneral::readAllFileAndClose(filedesc);
        const unsigned int &contentsize=static_cast<uint32_t>(content.size());

        const std::string &suffix=FacilityLibGeneral::getSuffix(fileName);
        #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
        if(CompressionProtocol::compressionTypeServer!=CompressionProtocol::CompressionType::None &&
                BaseServerMasterSendDatapack::compressedExtension.find(suffix)!=BaseServerMasterSendDatapack::compressedExtension.cend() &&
                (
                    contentsize<CATCHCHALLENGER_SERVER_DATAPACK_DONT_COMPRESS_GREATER_THAN_KB*1024
                    ||
                    contentsize>CATCHCHALLENGER_MAX_PACKET_SIZE
                )
            )
        {
            uint32_t posOutput=0;
            {
                const std::string &text=fileName;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(text.size());
                posOutput+=1;
                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
                posOutput+=text.size();
            }
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(contentsize);
            posOutput+=4;

            binaryAppend(BaseServerMasterSendDatapack::compressedFilesBuffer,ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            binaryAppend(BaseServerMasterSendDatapack::compressedFilesBuffer,content);
            BaseServerMasterSendDatapack::compressedFilesBufferCount++;
            switch(CompressionProtocol::compressionTypeServer)
            {
                default:
                case CompressionProtocol::CompressionType::Zstandard:
                if(BaseServerMasterSendDatapack::compressedFilesBuffer.size()>CATCHCHALLENGER_SERVER_DATAPACK_ZLIB_COMPRESSEDFILEPURGE_KB*1024 || BaseServerMasterSendDatapack::compressedFilesBufferCount>=255)
                    sendCompressedFileContent();
                break;
            }
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

                //send the network message
                uint32_t posOutput=0;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x76;
                posOutput+=1+4;

                //number of file
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=1;
                posOutput+=1;
                //filename
                {
                    const std::string &text=fileName;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(text.size());
                    posOutput+=1;
                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
                    posOutput+=text.size();
                }
                //file size
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(contentsize);
                posOutput+=4;

                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,content.data(),contentsize);
                posOutput+=contentsize;

                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size

                //std::cout << binaryToHex() << std::endl;

                sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            }
            else
            {
                uint32_t posOutput=0;
                {
                    const std::string &text=fileName;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(text.size());
                    posOutput+=1;
                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
                    posOutput+=text.size();
                }
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(contentsize);
                posOutput+=4;

                binaryAppend(BaseServerMasterSendDatapack::rawFilesBuffer,ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                binaryAppend(BaseServerMasterSendDatapack::rawFilesBuffer,content);
                BaseServerMasterSendDatapack::rawFilesBufferCount++;
                if(BaseServerMasterSendDatapack::rawFilesBuffer.size()>CATCHCHALLENGER_SERVER_DATAPACK_MAX_FILEPURGE_KB*1024 || BaseServerMasterSendDatapack::rawFilesBufferCount>=255)
                    sendFileContent();
            }
        }
        return true;
    }
    else
    {
        errorOutput("Unable to open into CatchChallenger::sendFile(): "+fileName);
        return false;
    }
}
#endif
