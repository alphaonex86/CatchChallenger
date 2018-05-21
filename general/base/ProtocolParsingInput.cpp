#if ! defined (ONLYMAPRENDER)
#include "ProtocolParsing.h"
#include "ProtocolParsingCheck.h"
#include "GeneralVariable.h"
#include "cpp11addition.h"
#include <iostream>
#include <cstring>

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
#include "zstd.h"
#endif

using namespace CatchChallenger;

ssize_t ProtocolParsingInputOutput::read(char * data, const size_t &size)
{
    #if defined (CATCHCHALLENGER_EXTRA_CHECK) && ! defined (EPOLLCATCHCHALLENGERSERVER)
    if(socket==NULL)
        return -1;
    if(socket->openMode()|QIODevice::WriteOnly)
    {}
    else
    {
        messageParsingLayer("Socket open in read only!");
        disconnectClient();
        return false;
    }
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
        #ifdef EPOLLCATCHCHALLENGERSERVER
        const int &temp_size=epollSocket.read(data,size);
        #else
        if(socket==NULL)
            return -1;
        const int &temp_size=static_cast<int>(socket->read(data,size));
        #endif
        RXSize+=temp_size;
        return temp_size;
    #else
        #ifdef EPOLLCATCHCHALLENGERSERVER
        return epollSocket.read(data,size);
        #else
        if(socket==NULL)
            return -1;
        return socket->read(data,size);
        #endif
    #endif
}

ssize_t ProtocolParsingInputOutput::write(const char * const data, const size_t &size)
{
    #if defined (CATCHCHALLENGER_EXTRA_CHECK) && ! defined (EPOLLCATCHCHALLENGERSERVER)
    if(socket==NULL)
        return -1;
    if(socket->openMode()|QIODevice::WriteOnly)
    {}
    else
    {
        messageParsingLayer("Socket open in write only!");
        disconnectClient();
        return false;
    }
    #endif

    //control the content BEFORE send
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        if(ProtocolParsingBase::packetFixedSize[0x02]!=2)
        {
            std::cerr << "parseIncommingDataRaw() You have never call: ProtocolParsing::initialiseTheVariable()" << std::endl;
            abort();
        }
        uint32_t cursor=0;
        #ifdef DEBUG_PROTOCOLPARSING_RAW_NETWORK
        uint32_t old_cursor=0;
        #endif
        do
        {
            protocolParsingCheck->flags|=0x08;
            if(protocolParsingCheck->parseIncommingDataRaw(data,static_cast<uint32_t>(size),cursor)!=1)
            {
                if(cursor>0)
                    std::cerr << "Bug at data-sending: " << binarytoHexa(data,cursor) << " " <<
                                 binarytoHexa(data+cursor,static_cast<uint32_t>(size)-cursor) << ", cursor:" << cursor << std::endl;
                else
                    std::cerr << "Bug at data-sending: " << binarytoHexa(data+cursor,static_cast<uint32_t>(size)-cursor) << std::endl;
                abort();
            }
            if(!protocolParsingCheck->valid)
            {
                std::cerr << "Bug at data-sending not tigger the function" << std::endl;
                abort();
            }
            protocolParsingCheck->valid=false;
            if((cursor-size)>0)
            {
                const uint8_t &mainCode=data[0];
                if(mainCode!=0x01 && mainCode!=0x7F)
                {
                    if(ProtocolParsingBase::packetFixedSize[mainCode]==0xFF)
                    {
                        std::cerr << "unknow packet: " << mainCode << std::endl;
                        abort();
                    }
                }
            }
            #ifdef DEBUG_PROTOCOLPARSING_RAW_NETWORK
            {
                const uint32_t &splitedSize=(cursor-old_cursor);
                std::cout << "Splited packet size: " << splitedSize << ": " << binarytoHexa(data+old_cursor,splitedSize) << std::endl;
                old_cursor=cursor;
            }
            #endif // DEBUG_PROTOCOLPARSING_RAW_NETWORK
        } while(cursor!=(uint32_t)size);
        /*if(cursor!=(uint32_t)size)
        {
            // Muliple concatened packet
            qDebug() << "Bug at data-sending cursor != size:" << cursor << "!=" << size;
            qDebug() << "raw write control bug:" << std::string(std::vector<char>(data,size).toHex());
            //abort();
        }*/
    }
    #endif

    //send AFTER the content control
    #ifdef EPOLLCATCHCHALLENGERSERVER
    const ssize_t &byteWriten=epollSocket.write(data,size);
    #else
    if(socket==NULL)
        return -1;
    const ssize_t &byteWriten=socket->write(data,size);
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    TXSize+=byteWriten;
    #endif
    /*if(Q_UNLIKELY((ssize_t)size!=byteWriten))
    {
        #ifdef EPOLLCATCHCHALLENGERSERVER
        messageParsingLayer("All the bytes have not be written byteWriten: "+std::to_string(byteWriten)+", size: "+std::to_string(size));
        #else
        messageParsingLayer("All the bytes have not be written: "+socket->errorString().toStdString()+", byteWriten: "+std::to_string(byteWriten));
        #endif
    }*/

    return byteWriten;
}

void ProtocolParsingInputOutput::closeSocket()
{
    #ifdef EPOLLCATCHCHALLENGERSERVER
    return epollSocket.close();
    #else
    if(socket!=NULL)
        return socket->disconnectFromHost();
    #endif
}

#if defined(EPOLLCATCHCHALLENGERSERVER)
bool ProtocolParsingInputOutput::socketIsOpen()//for epoll delete
{
    return epollSocket.isValid();
}

bool ProtocolParsingInputOutput::socketIsClosed()//for epoll delete
{
    return !epollSocket.isValid();
}
#endif

void ProtocolParsingInputOutput::parseIncommingData()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(parseIncommingDataCount>0)
    {
        std::cerr << "Multiple client on same section" << std::endl;
        abort();
    }
    parseIncommingDataCount++;
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    #ifdef PROTOCOLPARSINGDEBUG
    messageParsingLayer(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                std::to_string(flags & 0x10)+
                #endif
                std::string(" parseIncommingData(): socket->bytesAvailable(): ")+std::to_string(socket->bytesAvailable())+(", header_cut: ")+std::to_string(header_cut.size()));
    #endif
    #endif

    while(1)
    {
        ssize_t size;//bytesAvailable() and then read() can return negative value
        if(!header_cut.empty())
        {
            const unsigned int &size_to_get=CATCHCHALLENGER_COMMONBUFFERSIZE-static_cast<unsigned int>(header_cut.size());
            memcpy(ProtocolParsingInputOutput::tempBigBufferForInput,header_cut.data(),header_cut.size());
            size=read(ProtocolParsingInputOutput::tempBigBufferForInput,size_to_get)+header_cut.size();
            if(size<0)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                parseIncommingDataCount--;
                #endif
                return;
            }
            if(size>0)
            {
                size+=header_cut.size();
                #ifdef PROTOCOLPARSINGDEBUG
                std::cout << "with header cut " << binarytoHexa(ProtocolParsingInputOutput::tempBigBufferForInput,size) << " and size " << size << std::endl;
                #endif
            }
            else
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                parseIncommingDataCount--;
                #endif
                return;
            }
            header_cut.clear();
        }
        else
        {
            size=read(ProtocolParsingInputOutput::tempBigBufferForInput,CATCHCHALLENGER_COMMONBUFFERSIZE);
            if(size<0)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                parseIncommingDataCount--;
                #endif
                return;
            }
            if(size>0)
            {
                #ifdef PROTOCOLPARSINGDEBUG
                std::cout << "without header cut " << binarytoHexa(ProtocolParsingInputOutput::tempBigBufferForInput,size) << " and size " << size << std::endl;
                #endif
            }
        }
        if(size<=0)
        {
            /*messageParsingLayer(
#ifndef CATCHCHALLENGERSERVERDROPIFCLENT
std::to_string(flags & 0x10)+
#endif
std::string(" parseIncommingData(): size returned is 0!"));*/
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            parseIncommingDataCount--;
            #endif
            return;
        }
        uint32_t cursor=0;
        int8_t returnVar;
        do
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            const uint32_t oldcursor=cursor;
            #endif
            #ifdef PROTOCOLPARSINGDEBUG
            std::cout << "Start split: " << binarytoHexa(ProtocolParsingInputOutput::tempBigBufferForInput+cursor,size-cursor) << " and size " << size-cursor << ", cursor: " << cursor << std::endl;
            #endif
            returnVar=parseIncommingDataRaw(ProtocolParsingInputOutput::tempBigBufferForInput,static_cast<uint32_t>(size),cursor);
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(oldcursor==cursor && returnVar==1)
            {
                std::cerr << "Cursor don't move but the function have returned 1" << std::endl;
                abort();
            }
            #endif
            //this interface allow 0 copy method
            if(returnVar<0)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                parseIncommingDataCount--;
                #endif
                return;
            }
        } while(cursor<(uint32_t)size && returnVar==1);
    }
    #ifdef PROTOCOLPARSINGDEBUG
    messageParsingLayer(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                std::to_string(flags & 0x10)+
                #endif
    std::string(" parseIncommingData(): finish parse the input"));
    #endif
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    parseIncommingDataCount--;
    #if defined(EPOLLCATCHCHALLENGERSERVER)
    if(epollSocket.bytesAvailable()>0)
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(flags & 0x10)+
                    #endif
        std::string(" parseIncommingData(): remain byte to purge!"));
    #endif
    #endif
}

//this interface allow 0 copy method
int8_t ProtocolParsingBase::parseIncommingDataRaw(const char * const commonBuffer, const uint32_t &size, uint32_t &cursor)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(ProtocolParsingBase::packetFixedSize[0x02]!=2)
    {
        std::cerr << "parseIncommingDataRaw() You have never call: ProtocolParsing::initialiseTheVariable()" << std::endl;
        abort();
    }
    #endif
    {
        const int8_t &returnVar=parseHeader(commonBuffer,size,cursor);
        if(returnVar!=1)
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(returnVar==0)
                errorParsingLayer("Break due to need more in query number");
            else
                errorParsingLayer("parseIncommingDataRaw() Have bug");
            #endif
            return returnVar;
        }
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(cursor==0 && !(flags & 0x80))
        {
            std::cerr << "Critical bug" << std::endl;
            abort();
        }
        #endif
    }
    {
        const int8_t &returnVar=parseQueryNumber(commonBuffer,size,cursor);
        if(returnVar!=1)
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(returnVar==0)
                errorParsingLayer("Break due to need more in query number");
            else
                errorParsingLayer("Not a reply to a query or similar bug");
            #endif
            return returnVar;
        }
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(cursor==0 && !(flags & 0x80))
        {
            std::cerr << "Critical bug" << std::endl;
            abort();
        }
        #endif
    }
    {
        const int8_t &returnVar=parseDataSize(commonBuffer,size,cursor);
        if(returnVar!=1)
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            //qDebug() << "Break due to need more in parse data size";
            #endif
            return returnVar;
        }
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(cursor==0 && !(flags & 0x80))
        {
            std::cerr << "Critical bug" << std::endl;
            abort();
        }
        #endif
    }
    {
        const int8_t &returnVar=parseData(commonBuffer,size,cursor);
        if(returnVar!=1)
        {
            breakNeedMoreData();
            return returnVar;
        }
    }
    //parseDispatch(); do into above function
    //dataClear();-> not return if failed or just stop parsing, then do into parseDispatch()
    return true;
}

void ProtocolParsingBase::breakNeedMoreData()
{
}

bool ProtocolParsingBase::isReply() const
{
    return packetCode==CATCHCHALLENGER_PROTOCOL_REPLY_CLIENT_TO_SERVER || packetCode==CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    /*#ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(flags & 0x10)
    {
        if(packetCode==CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT)
            return true;
    }
    else
    #endif
    {
        if(packetCode==CATCHCHALLENGER_PROTOCOL_REPLY_CLIENT_TO_SERVER)
            return false;
    }
    return false;*/
}

int8_t ProtocolParsingBase::parseHeader(const char * const commonBuffer,const uint32_t &size,uint32_t &cursor)
{
    if(Q_LIKELY(!(flags & 0x80)))
    {
        if((size-cursor)<sizeof(uint8_t))//ignore because first int is cuted!
            return 0;
        packetCode=*(commonBuffer+cursor);
        cursor+=sizeof(uint8_t);
        flags |= 0x80;

        //def query without the sub code

        if(isReply())
            return 1;
        else
        {
            dataSize=ProtocolParsingBase::packetFixedSize[packetCode];
            if(dataSize==0xFF)
            {
                errorParsingLayer("wrong packet code (header): "+std::to_string(packetCode)+
                                  ", data: "+binarytoHexa(commonBuffer,size)+
                                  ", cursor: "+std::to_string(cursor));
                return -1;//packetCode code wrong
            }
            else if(dataSize!=0xFE)
                flags |= 0x40;
            else
            {
                if(!(flags & 0x08))
                {
                    errorParsingLayer("dynamic size blocked (header) for packet code: "+std::to_string(packetCode));
                    return -1;
                }
            }
            return 1;
        }
    }
    else
        return 1;
}

int8_t ProtocolParsingBase::parseQueryNumber(const char * const commonBuffer,const uint32_t &size,uint32_t &cursor)
{
    if(Q_LIKELY(!(flags & 0x20) && (isReply() || packetCode>=0x80)))
    {
        if(Q_UNLIKELY((size-cursor)<sizeof(uint8_t)))
        {
            //todo, write message: need more bytes
            return 0;
        }
        queryNumber=*(commonBuffer+cursor);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(Q_UNLIKELY(cursor==0))
        {
            errorParsingLayer("cursor==0, don't have read the header?"
                              ", data: "+binarytoHexa(commonBuffer,size)+
                              ", cursor: "+std::to_string(cursor));
            return -1;
        }
        #endif
        cursor+=sizeof(uint8_t);
        if(Q_UNLIKELY(queryNumber>(CATCHCHALLENGER_MAXPROTOCOLQUERY-1)))
        {
            errorParsingLayer("query number >"+std::to_string(CATCHCHALLENGER_MAXPROTOCOLQUERY-1)+
                              ", data: "+binarytoHexa(commonBuffer,size)+
                              ", cursor: "+std::to_string(cursor));
            return -1;
        }
        //set this parsing step is done
        flags |= 0x20;

        // it's reply
        if(isReply())
        {
            //put to 0x00 at ProtocolParsingBase::parseDispatch()
            const uint8_t &replyTo=outputQueryNumberToPacketCode[queryNumber];
            //not a reply to a query
            if(replyTo==0x00)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                std::string returnedList;
                unsigned int index=0;
                while(index<sizeof(outputQueryNumberToPacketCode))
                {
                    if(outputQueryNumberToPacketCode[index]!=0x00)
                    {
                        if(!returnedList.empty())
                            returnedList+=",";
                        returnedList+="["+std::to_string(index)+"]="+std::to_string(outputQueryNumberToPacketCode[index]);
                    }
                    index++;
                }
                errorParsingLayer("not a reply to a known query (parseQueryNumber): "+std::to_string(queryNumber)+", query in progress: "+returnedList);
                #else
                errorParsingLayer("not a reply to a known query (parseQueryNumber): "+std::to_string(queryNumber));
                #endif
                return -1;
            }
            dataSize=ProtocolParsingBase::packetFixedSize[256+replyTo-128];
            if(dataSize==0xFF)
                abort();//packetCode code wrong, how the output allow this! filter better the output
            else if(dataSize!=0xFE)
                flags |= 0x40;
            else
            {
                if(!(flags & 0x08))
                {
                    errorParsingLayer("dynamic size blocked (query number)");
                    return -1;
                }
            }
        }
        else // it's query with reply
        {
            //size resolved before, into subCodeType step
        }
    }

    return 1;
}

int8_t ProtocolParsingBase::parseDataSize(const char * const commonBuffer, const uint32_t &size,uint32_t &cursor)
{
    if(Q_LIKELY(!(flags & 0x40)))
    {
        #ifdef PROTOCOLPARSINGDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(flags & 0x10)+
                    #endif
        std::string(" parseIncommingData(): !haveData_dataSize"));
        #endif
        //temp data
        if((size-cursor)<sizeof(uint32_t))
        {
            if((size-cursor)>0)
                binaryAppend(header_cut,commonBuffer+cursor,(size-cursor));
            return 0;
        }
        dataSize=le32toh(*reinterpret_cast<const uint32_t *>(commonBuffer+cursor));
        cursor+=sizeof(uint32_t);

        if(dataSize>(CATCHCHALLENGER_BIGBUFFERSIZE-8))
        {
            errorParsingLayer("packet size too big (define)"
                              ", data: "+binarytoHexa(commonBuffer,size)+
                              ", cursor: "+std::to_string(cursor));
            return -1;
        }
        #ifdef CATCHCHALLENGER_CLASS_MASTER
        if(dataSize>4*1024)
        #else
            #ifdef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
            if(dataSize>64*1024)
            #else
            if(dataSize>16*1024*1024)
            #endif
        #endif
        {
            errorParsingLayer("packet size too big (hard)");
            return -1;
        }

        flags |= 0x40;
    }
    return 1;
}

int8_t ProtocolParsingBase::parseData(const char * const commonBuffer, const uint32_t &size,uint32_t &cursor)
{
    if(dataSize==0)
    {
        const bool &returnVal=parseDispatch(NULL,0);
        dataClear();
        if(returnVal)
            return 1;
        else
            return -1;
    }
    if(dataToWithoutHeader.empty())
    {
        //if have too many data, or just the size
        if(dataSize<=(size-cursor))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(cursor==0 && !(flags & 0x80))
            {
                std::cerr << "Critical bug" << std::endl;
                abort();
            }
            #endif
            const bool &returnVal=parseDispatch(commonBuffer+cursor,dataSize);
            cursor+=dataSize;
            #ifdef PROTOCOLPARSINGDEBUG
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        std::to_string(flags & 0x10)+
                        #endif
            std::string(" parseIncommingData(): remaining data: ")+std::to_string(size-cursor)+
                        ", returnVal: "+std::to_string(returnVal));
            #endif
            dataClear();
            if(returnVal)
                return 1;
            else
                return -1;
        }
    }
    //if have too many data, or just the size
    if((dataSize-dataToWithoutHeader.size())<=(size-cursor))
    {
        const uint32_t &size_to_append=dataSize-static_cast<uint32_t>(dataToWithoutHeader.size());
        binaryAppend(dataToWithoutHeader,commonBuffer+cursor,size_to_append);
        cursor+=size_to_append;
        #ifdef PROTOCOLPARSINGDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(flags & 0x10)+
                    #endif
        std::string(" parseIncommingData(): remaining data: ")+std::to_string(size-cursor)+
                    (", buffer data: ")+binarytoHexa(commonBuffer,sizeof(commonBuffer)));
        #endif
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(dataSize!=(uint32_t)dataToWithoutHeader.size())
        {
            errorParsingLayer("wrong data size here");
            return false;
        }
        #endif
        const bool &returnVal=parseDispatch(dataToWithoutHeader.data(),static_cast<uint32_t>(dataToWithoutHeader.size()));
        dataClear();
        if(returnVal)
            return 1;
        else
            return -1;
    }
    else //if need more data
    {
        binaryAppend(dataToWithoutHeader,commonBuffer+cursor,(size-cursor));
        #ifdef PROTOCOLPARSINGDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(flags & 0x10)+
                    #endif
        std::string(" parseIncommingData(): need more to recompose: ")+std::to_string(dataSize-dataToWithoutHeader.size()));
        #endif
        cursor=size;
        return 0;
    }
}

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
int32_t ProtocolParsing::computeDecompression(const char* const source, char* const dest, const unsigned int &sourceSize, const unsigned int &maxDecompressedSize, const CompressionType &compressionType)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(maxDecompressedSize<sourceSize)
    {
        std::cerr << "maxDecompressedSize<compressedSize in ProtocolParsingBase::computeDecompression" << std::endl;
        abort();
    }
    #endif
    switch(compressionType)
    {
        case CompressionType::None:
            std::cerr << "CompressionType::None in ProtocolParsingBase::computeDecompression, do direct mapping" << std::endl;
            abort();
            return -1;
        break;
        case CompressionType::Zstandard:
        default:
            return ProtocolParsing::decompressZstandard(source,sourceSize,dest,maxDecompressedSize);
        break;
    }
}

int32_t ProtocolParsing::computeCompression(const char* const source, char* const dest, const unsigned int &sourceSize, const unsigned int &maxDecompressedSize, const CompressionType &compressionType)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(maxDecompressedSize<sourceSize)
    {
        std::cerr << "maxDecompressedSize<compressedSize in ProtocolParsingBase::computeDecompression" << std::endl;
        abort();
    }
    #endif
    switch(compressionType)
    {
        case CompressionType::None:
            std::cerr << "CompressionType::None in ProtocolParsingBase::computeDecompression, do direct mapping" << std::endl;
            abort();
            return -1;
        break;
        case CompressionType::Zstandard:
        default:
            return ProtocolParsing::compressZstandard(source,sourceSize,dest,maxDecompressedSize);
        break;
    }
}
#endif

bool ProtocolParsingBase::parseDispatch(const char * const data, const int &size)
{
    #ifdef PROTOCOLPARSINGINPUTOUTPUTDEBUG
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(flags & 0x10)
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(flags & 0x10)+
                    #endif
        std::string(" parseIncommingData(): parse message as client"));
    else
    #else
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(flags & 0x10)+
                    #endif
        std::string(" parseIncommingData(): parse message as server"));
    #endif
    #endif
    #ifdef PROTOCOLPARSINGINPUTOUTPUTDEBUG
    messageParsingLayer(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                std::to_string(flags & 0x10)+
                #endif
    std::string(" parseIncommingData(): data: ")+binarytoHexa(data,size));
    #endif
    //message
    if(isReply())
    {
        //copy because can resend query with same queryNumber, in this case the outputQueryNumberToPacketCode[queryNumber] need be 0x00
        const uint8_t replyTo=outputQueryNumberToPacketCode[queryNumber];
        outputQueryNumberToPacketCode[queryNumber]=0x00;

        const bool &returnValue=parseReplyData(replyTo,queryNumber,data,size);
        #ifdef PROTOCOLPARSINGINPUTOUTPUTDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(flags & 0x10)+
                    #endif
        std::string(" parseIncommingData(): need_query_number && is_reply && reply_subCodeType.contains(queryNumber), queryNumber: ")+
                    std::to_string(queryNumber)+(", packetCode: ")+std::to_string(packetCode));
        #endif
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!returnValue)
            errorParsingLayer("parseReplyData(): return false, need be aborted before, packetCode: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
        #endif
        return returnValue;
    }
    if(packetCode<0x80)
    {
        #ifdef PROTOCOLPARSINGINPUTOUTPUTDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(flags & 0x10)+
                    #endif
        std::string(" parseIncommingData(): !need_query_number && !need_subCodeType, packetCode: ")+std::to_string(packetCode));
        #endif
        const bool &returnValue=parseMessage(packetCode,data,size);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!returnValue)
            errorParsingLayer("parseMessage(): return false, need be aborted before, packetCode: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
        #endif
        return returnValue;
    }
    else
    {
        //query
        #ifdef PROTOCOLPARSINGINPUTOUTPUTDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(flags & 0x10)+
                    #endif
        std::string(" parseIncommingData(): need_query_number && !is_reply, packetCode: ")+std::to_string(packetCode));
        #endif
        storeInputQuery(packetCode,queryNumber);
        const bool &returnValue=parseQuery(packetCode,queryNumber,data,size);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!returnValue)
            errorParsingLayer("parseQuery(): return false, need be aborted before, packetCode: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
        #endif
        return returnValue;
    }
}

void ProtocolParsingBase::dataClear()
{
    dataToWithoutHeader.clear();
    flags &= 0x18;
}

#ifndef EPOLLCATCHCHALLENGERSERVER
std::vector<std::string> ProtocolParsingBase::getQueryRunningList()
{
    std::vector<std::string> returnedList;
    unsigned int index=0;
    while(index<sizeof(outputQueryNumberToPacketCode))
    {
        if(outputQueryNumberToPacketCode[index]!=0x00)
            returnedList.push_back(std::to_string(outputQueryNumberToPacketCode[index]));
        index++;
    }
    return returnedList;
}
#endif

void ProtocolParsingBase::storeInputQuery(const uint8_t &,const uint8_t &)
{
}

void ProtocolParsingInputOutput::storeInputQuery(const uint8_t &packetCode,const uint8_t &queryNumber)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(inputQueryNumberToPacketCode[queryNumber]!=0x00)
    {
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(flags & 0x10)+
                    #endif
        " storeInputQuery("+std::to_string(packetCode)+","+std::to_string(queryNumber)+") query with same id previously say");
        return;
    }
    #endif
    //registrer on the check
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    protocolParsingCheck->storeInputQuery(packetCode,queryNumber);
    #endif
    //register the size of the reply to send
    inputQueryNumberToPacketCode[queryNumber]=packetCode;
}
#endif
