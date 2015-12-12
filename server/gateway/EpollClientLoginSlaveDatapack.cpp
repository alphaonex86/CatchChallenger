#include "EpollClientLoginSlave.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "DatapackDownloaderBase.h"
#include "DatapackDownloaderMainSub.h"

#include <iostream>
#include <vector>
#include <regex>
#include <openssl/sha.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <algorithm>

using namespace CatchChallenger;

static const std::string text_double_slash="//";
static const std::string text_dotslash="./";
static const std::string text_antislash="\\";
static const std::string text_slash="/";

#ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
std::unordered_map<std::string,EpollClientLoginSlave::DatapackCacheFile> EpollClientLoginSlave::datapack_file_list(const std::string &path,const bool withHash)
{
    std::unordered_map<std::string,DatapackCacheFile> filesList;
    std::regex datapack_rightFileName(DATAPACK_FILE_REGEX);

    const std::vector<std::string> &returnList=FacilityLibGeneral::listFolder(path);
    int index=0;
    const int &size=returnList.size();
    while(index<size)
    {
        #ifdef _WIN32
        std::string fileName=returnList.at(index);
        #else
        const std::string &fileName=returnList.at(index);
        #endif
        if(regex_search(fileName,datapack_rightFileName))
        {
            if(DatapackDownloaderBase::extensionAllowed.find(FacilityLibGeneral::getSuffix(fileName))!=DatapackDownloaderBase::extensionAllowed.cend())
            {
                if(withHash)
                {
                    const std::string &fullPath=path+returnList.at(index);
                    FILE *file=fopen(fullPath.c_str(),"wb");
                    if(file!=NULL)
                    {
                        DatapackCacheFile datapackCacheFile;
                        #ifdef _WIN32
                        fileName.replace(EpollClientLoginSlave::text_antislash,EpollClientLoginSlave::text_slash);//remplace if is under windows server
                        #endif

                        const std::vector<char> &data=FacilityLibGeneral::readAllFileAndClose(file);
                        SHA224(reinterpret_cast<const unsigned char *>(data.data()),data.size(),reinterpret_cast<unsigned char *>(ProtocolParsingBase::tempBigBufferForOutput));
                        datapackCacheFile.partialHash=*reinterpret_cast<const int *>(ProtocolParsingBase::tempBigBufferForOutput);

                        filesList[fileName]=datapackCacheFile;
                    }
                    else
                    {
                        std::cerr << "Can't open: " << fileName << ": " << errno << std::endl;
                        return std::unordered_map<std::string,EpollClientLoginSlave::DatapackCacheFile>();
                    }
                }
            }
        }
        index++;
    }
    return filesList;
}

//check each element of the datapack, determine if need be removed, updated, add as new file all the missing file
void EpollClientLoginSlave::datapackList(const uint8_t &query_id,const std::vector<std::string> &files,const std::vector<uint32_t> &partialHashList)
{
    if(linkToGameServer==NULL)
    {
        std::cerr << "EpollClientLoginSlave::datapackList(const uint8_t &query_id,const std::vector<std::string> &files,const std::vector<uint32_t> &partialHashList) linkToGameServer==NULL" << std::endl;
        return;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    /// \see EpollClientLoginSlave::parseFullQuery() already checked here
    #endif
    tempDatapackListReplyArray.clear();
    tempDatapackListReplyTestCount=0;
    EpollClientLoginSlave::rawFilesBuffer.clear();
    EpollClientLoginSlave::compressedFilesBuffer.clear();
    EpollClientLoginSlave::rawFilesBufferCount=0;
    EpollClientLoginSlave::compressedFilesBufferCount=0;
    tempDatapackListReply=0;
    tempDatapackListReplySize=0;
    uint32_t fileToDelete=0;
    std::string datapackPath;
    std::unordered_map<std::string,DatapackCacheFile> filesList;
    switch(datapackStatus)
    {
        case DatapackStatus::Base:
            filesList=datapack_file_base.datapack_file_hash_cache;
            datapackPath=linkToGameServer->mDatapackBase;
        break;
        case DatapackStatus::Main:
            if(datapack_file_main.find(linkToGameServer->main)==datapack_file_main.cend())
            {
                std::cerr << "!datapack_file_main.contains " << linkToGameServer->main << std::endl;
                return;
            }
            if(DatapackDownloaderMainSub::datapackDownloaderMainSub.find(linkToGameServer->main)==DatapackDownloaderMainSub::datapackDownloaderMainSub.cend())
            {
                std::cerr << "!DatapackDownloaderMainSub::datapackDownloaderMainSub " << linkToGameServer->main << std::endl;
                return;
            }
            if(DatapackDownloaderMainSub::datapackDownloaderMainSub.at(linkToGameServer->main).find(linkToGameServer->sub)==DatapackDownloaderMainSub::datapackDownloaderMainSub.at(linkToGameServer->main).cend())
            {
                std::cerr << "!DatapackDownloaderMainSub::datapackDownloaderMainSub " << linkToGameServer->main << " " << linkToGameServer->sub << std::endl;
                return;
            }
            filesList=datapack_file_main.at(linkToGameServer->main).datapack_file_hash_cache;
            datapackPath=DatapackDownloaderMainSub::datapackDownloaderMainSub.at(linkToGameServer->main).at(linkToGameServer->sub)->mDatapackMain;
        break;
        case DatapackStatus::Sub:
            if(datapack_file_sub.find(linkToGameServer->main)==datapack_file_sub.cend())
            {
                std::cerr << "!datapack_file_sub.contains " << linkToGameServer->main << std::endl;
                return;
            }
            if(datapack_file_sub.at(linkToGameServer->main).find(linkToGameServer->sub)==datapack_file_sub.at(linkToGameServer->main).cend())
            {
                std::cerr << "!datapack_file_sub.contains " << linkToGameServer->main << " " << linkToGameServer->sub << std::endl;
                return;
            }
            if(DatapackDownloaderMainSub::datapackDownloaderMainSub.find(linkToGameServer->main)==DatapackDownloaderMainSub::datapackDownloaderMainSub.cend())
            {
                std::cerr << "!DatapackDownloaderMainSub::datapackDownloaderMainSub " << linkToGameServer->main << std::endl;
                return;
            }
            if(DatapackDownloaderMainSub::datapackDownloaderMainSub.at(linkToGameServer->main).find(linkToGameServer->sub)==DatapackDownloaderMainSub::datapackDownloaderMainSub.at(linkToGameServer->main).cend())
            {
                std::cerr << "!DatapackDownloaderMainSub::datapackDownloaderMainSub " << linkToGameServer->main << " " << linkToGameServer->sub << std::endl;
                return;
            }
            filesList=datapack_file_sub.at(linkToGameServer->main).at(linkToGameServer->sub).datapack_file_hash_cache;
            datapackPath=DatapackDownloaderMainSub::datapackDownloaderMainSub.at(linkToGameServer->main).at(linkToGameServer->sub)->mDatapackSub;
        break;
        default:
        return;
    }
    std::vector<FileToSend> fileToSendList;

    const int &loop_size=files.size();
    //send the size to download on the client
    {
        //clone to drop the ask file and remain the missing client files
        std::unordered_map<std::string,DatapackCacheFile> filesListForSize(filesList);
        int index=0;
        uint32_t datapckFileNumber=0;
        uint32_t datapckFileSize=0;
        while(index<loop_size)
        {
            const std::string &fileName=files.at(index);
            const uint32_t &clientPartialHash=partialHashList.at(index);
            if(fileName.find(text_dotslash) != std::string::npos)
            {
                std::cerr << "file name contains illegale char (1): " << fileName << std::endl;
                return;
            }
            if(fileName.find(text_antislash) != std::string::npos)
            {
                std::cerr << "file name contains illegale char (2): " << fileName << std::endl;
                return;
            }
            if(fileName.find(text_double_slash) != std::string::npos)
            {
                std::cerr << "file name contains illegale char (3): " << fileName << std::endl;
                return;
            }
            if(regex_search(fileName,fileNameStartStringRegex) || stringStartWith(fileName,text_slash))
            {
                std::cerr << "start with wrong string: " << fileName << std::endl;
                return;
            }
            if(!regex_search(fileName,EpollClientLoginSlave::datapack_rightFileName))
            {
                std::cerr << "file name sended wrong: " << fileName << std::endl;
                return;
            }
            if(filesListForSize.find(fileName)!=filesListForSize.cend())
            {
                const uint32_t &serverPartialHash=filesListForSize.at(fileName).partialHash;
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(serverPartialHash==0)
                {
                    std::cerr << "serverPartialHash==0 at " << __FILE__ << ":" << __LINE__ << std::endl;
                    abort();
                }
                #endif
                if(clientPartialHash==serverPartialHash)
                    addDatapackListReply(false);//file found don't need be updated
                else
                {
                    //todo: be sure at the startup sll the file is readable
                    struct stat myStat;
                    if(::stat((datapackPath+fileName).c_str(),&myStat)==0)
                    {
                        addDatapackListReply(false);//found but need an update
                        datapckFileNumber++;
                        datapckFileSize+=myStat.st_size;
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
            struct stat myStat;
            if(::stat((datapackPath+i->first).c_str(),&myStat)==0)
            {
                datapckFileNumber++;
                datapckFileSize+=myStat.st_size;
                FileToSend fileToSend;
                fileToSend.file=i->first;
                fileToSendList.push_back(fileToSend);
            }
            ++i;
        }

        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x75;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(8);//set the dynamic size

        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(datapckFileNumber);
        posOutput+=4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(datapckFileSize);
        posOutput+=4;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
    if(fileToSendList.empty() && fileToDelete==0)
    {
        std::cerr << "Ask datapack list where the checksum match" << std::endl;
        return;
    }
    std::sort(fileToSendList.begin(),fileToSendList.end());
    //validate, remove or update the file actualy on the client
    if(tempDatapackListReplyTestCount!=files.size())
    {
        std::cerr << "Bit count return not match" << std::endl;
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
        case DatapackStatus::Base:
            datapackStatus=DatapackStatus::Main;
        break;
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

bool CatchChallenger::operator<(const FileToSend &fileToSend1,const FileToSend &fileToSend2)
{
    if(fileToSend1.file<fileToSend2.file)
        return false;
    return true;
}

void EpollClientLoginSlave::addDatapackListReply(const bool &fileRemove)
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

void EpollClientLoginSlave::purgeDatapackListReply(const uint8_t &query_id)
{
    if(tempDatapackListReplySize>0)
    {
        tempDatapackListReplyArray.push_back(tempDatapackListReply);
        tempDatapackListReplySize=0;
        tempDatapackListReply=0;
    }
    if(tempDatapackListReplyArray.empty())
        tempDatapackListReplyArray.push_back(0x00);

    //send the network reply
    removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(tempDatapackListReplyArray.size());//set the dynamic size

    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,tempDatapackListReplyArray.data(),tempDatapackListReplyArray.size());
    posOutput+=tempDatapackListReplyArray.size();

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

    tempDatapackListReplyArray.clear();
}

void EpollClientLoginSlave::sendFileContent()
{
    if(EpollClientLoginSlave::rawFilesBuffer.size()>0 && EpollClientLoginSlave::rawFilesBufferCount>0)
    {
        EpollClientLoginSlave::rawFilesBuffer.insert(EpollClientLoginSlave::rawFilesBuffer.begin(),EpollClientLoginSlave::rawFilesBufferCount);

        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x76;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(EpollClientLoginSlave::rawFilesBuffer.size());//set the dynamic size

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=EpollClientLoginSlave::rawFilesBufferCount;
        posOutput+=1;
        if(EpollClientLoginSlave::rawFilesBuffer.size()>CATCHCHALLENGER_MAX_PACKET_SIZE)
        {
            std::cerr << "Client::sendFileContent too big to reply" << std::endl;
            return;
        }
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,EpollClientLoginSlave::rawFilesBuffer.data(),EpollClientLoginSlave::rawFilesBuffer.size());
        posOutput+=EpollClientLoginSlave::rawFilesBuffer.size();

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

        EpollClientLoginSlave::rawFilesBuffer.clear();
        EpollClientLoginSlave::rawFilesBufferCount=0;
    }
}

void EpollClientLoginSlave::sendCompressedFileContent()
{
    if(EpollClientLoginSlave::compressedFilesBuffer.size()>0 && EpollClientLoginSlave::compressedFilesBufferCount>0)
    {
        EpollClientLoginSlave::compressedFilesBuffer.insert(EpollClientLoginSlave::compressedFilesBuffer.begin(),EpollClientLoginSlave::compressedFilesBufferCount);

        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x77;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(EpollClientLoginSlave::compressedFilesBuffer.size());//set the dynamic size

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=EpollClientLoginSlave::compressedFilesBufferCount;
        posOutput+=1;
        if(EpollClientLoginSlave::compressedFilesBuffer.size()>CATCHCHALLENGER_MAX_PACKET_SIZE)
        {
            std::cerr << "Client::sendFileContent too big to reply" << std::endl;
            return;
        }
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,EpollClientLoginSlave::compressedFilesBuffer.data(),EpollClientLoginSlave::compressedFilesBuffer.size());
        posOutput+=EpollClientLoginSlave::compressedFilesBuffer.size();

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

        EpollClientLoginSlave::compressedFilesBuffer.clear();
        EpollClientLoginSlave::compressedFilesBufferCount=0;
    }
}

bool EpollClientLoginSlave::sendFile(const std::string &datapackPath,const std::string &fileName)
{
    if(fileName.size()>255 || fileName.empty())
    {
        errorParsingLayer("Unable to open into CatchChallenger::sendFile(): fileName.size()>255 || fileName.empty()");
        return false;
    }

    FILE *file=fopen((datapackPath+fileName).c_str(),"rb");
    if(file!=NULL)
    {
        std::vector<char> content=FacilityLibGeneral::readAllFileAndClose(file);
        const int &contentsize=content.size();

        const std::string &suffix=FacilityLibGeneral::getSuffix(fileName);
        if(EpollClientLoginSlave::compressedExtension.find(suffix)!=EpollClientLoginSlave::compressedExtension.cend() &&
                ProtocolParsing::compressionTypeServer!=ProtocolParsing::CompressionType::None &&
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
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
                posOutput+=1;
                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
                posOutput+=text.size();
            }
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(contentsize);
            posOutput+=4;

            binaryAppend(EpollClientLoginSlave::compressedFilesBuffer,ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            binaryAppend(EpollClientLoginSlave::compressedFilesBuffer,content);
            EpollClientLoginSlave::compressedFilesBufferCount++;
            switch(ProtocolParsing::compressionTypeServer)
            {
                case ProtocolParsing::CompressionType::Xz:
                if(EpollClientLoginSlave::compressedFilesBuffer.size()>CATCHCHALLENGER_SERVER_DATAPACK_XZ_COMPRESSEDFILEPURGE_KB*1024 || EpollClientLoginSlave::compressedFilesBufferCount>=255)
                    sendCompressedFileContent();
                break;
                default:
                case ProtocolParsing::CompressionType::Zlib:
                if(EpollClientLoginSlave::compressedFilesBuffer.size()>CATCHCHALLENGER_SERVER_DATAPACK_ZLIB_COMPRESSEDFILEPURGE_KB*1024 || EpollClientLoginSlave::compressedFilesBufferCount>=255)
                    sendCompressedFileContent();
                break;
            }
        }
        else
        {
            if(contentsize>CATCHCHALLENGER_SERVER_DATAPACK_MIN_FILEPURGE_KB*1024)
            {
                if((1+fileName.size()+4+contentsize)>=CATCHCHALLENGER_MAX_PACKET_SIZE)
                {
                    messageParsingLayer("Error: outputData2(1)+fileNameRaw("+
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
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(1+EpollClientLoginSlave::rawFilesBuffer.size());//set the dynamic size

                //number of file
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=1;
                posOutput+=1;
                //filename
                {
                    const std::string &text=fileName;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
                    posOutput+=1;
                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
                    posOutput+=text.size();
                }
                //file size
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(contentsize);
                posOutput+=4;

                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,content.data(),contentsize);
                posOutput+=contentsize;

                sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            }
            else
            {
                uint32_t posOutput=0;
                {
                    const std::string &text=fileName;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
                    posOutput+=1;
                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
                    posOutput+=text.size();
                }
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(contentsize);
                posOutput+=4;

                binaryAppend(EpollClientLoginSlave::rawFilesBuffer,ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                binaryAppend(EpollClientLoginSlave::rawFilesBuffer,content);
                EpollClientLoginSlave::rawFilesBufferCount++;
                if(EpollClientLoginSlave::rawFilesBuffer.size()>CATCHCHALLENGER_SERVER_DATAPACK_MAX_FILEPURGE_KB*1024 || EpollClientLoginSlave::rawFilesBufferCount>=255)
                    sendFileContent();
            }
        }
        return true;
    }
    else
    {
        errorParsingLayer("Unable to open into CatchChallenger::sendFile(): "+std::to_string(errno)+" for :"+datapackPath+fileName);
        return false;
    }
}
#endif
