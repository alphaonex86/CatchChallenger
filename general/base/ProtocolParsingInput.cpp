#include "ProtocolParsing.h"
#include "ProtocolParsingCheck.h"
#include "GeneralVariable.h"
#include <iostream>

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
#include <lzma.h>
#include "lz4/lz4.h"
#endif

using namespace CatchChallenger;

ssize_t ProtocolParsingInputOutput::read(char * data, const size_t &size)
{
    #if defined (CATCHCHALLENGER_EXTRA_CHECK) && ! defined (EPOLLCATCHCHALLENGERSERVER)
    if(socket->openMode()|QIODevice::WriteOnly)
    {}
    else
    {
        messageParsingLayer(std::stringLiteral("Socket open in read only!"));
        disconnectClient();
        return false;
    }
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
        #ifdef EPOLLCATCHCHALLENGERSERVER
        const int &temp_size=epollSocket.read(data,size);
        #else
        const int &temp_size=socket->read(data,size);
        #endif
        RXSize+=temp_size;
        return temp_size;
    #else
        #ifdef EPOLLCATCHCHALLENGERSERVER
        return epollSocket.read(data,size);
        #else
        return socket->read(data,size);
        #endif
    #endif
}

ssize_t ProtocolParsingInputOutput::write(const char * const data, const size_t &size)
{
    #if defined (CATCHCHALLENGER_EXTRA_CHECK) && ! defined (EPOLLCATCHCHALLENGERSERVER)
    if(socket->openMode()|QIODevice::WriteOnly)
    {}
    else
    {
        messageParsingLayer(std::stringLiteral("Socket open in write only!"));
        disconnectClient();
        return false;
    }
    #endif
    #ifdef EPOLLCATCHCHALLENGERSERVER
    const ssize_t &byteWriten=epollSocket.write(data,size);
    #else
    const ssize_t &byteWriten=socket->write(data,size);
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    TXSize+=byteWriten;
    #endif
    if(Q_UNLIKELY((ssize_t)size!=byteWriten))
    {
        #ifdef EPOLLCATCHCHALLENGERSERVER
        messageParsingLayer("All the bytes have not be written byteWriten: "+std::to_string(byteWriten)+", size: "+std::to_string(size));
        #else
        messageParsingLayer("All the bytes have not be written: "+socket->errorString()+", byteWriten: "+std::to_string(byteWriten));
        #endif
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            uint32_t cursor=0;
            do
            {
                if(!protocolParsingCheck->parseIncommingDataRaw(data,size,cursor))
                {
                    std::cerr << "Bug at data-sending: " << std::string(std::vector<char>(data,size).toHex()).toStdString() << std::endl;
                    abort();
                }
                if(!protocolParsingCheck->valid)
                {
                    qDebug() << "Bug at data-sending not tigger the function";
                    abort();
                }
                protocolParsingCheck->valid=false;
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
    }
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

bool ProtocolParsingBase::forwardTo(ProtocolParsingBase * const destination)
{
    uint32_t size=0;
    if(!header_cut.isEmpty())
    {
        const unsigned int &size_to_get=CATCHCHALLENGER_COMMONBUFFERSIZE-header_cut.size();
        memcpy(ProtocolParsingInputOutput::tempBigBufferForUncompressedInput,header_cut.constData(),header_cut.size());
        size=read(ProtocolParsingInputOutput::tempBigBufferForUncompressedInput,size_to_get)+header_cut.size();
        if(size>0)
        {
            //std::vector<char> tempDataToDebug(ProtocolParsingInputOutput::commonBuffer+header_cut.size(),size-header_cut.size());
            //qDebug() << "with header cut" << header_cut << tempDataToDebug.toHex() << "and size" << size;
        }
        header_cut.clear();
    }
    else
    {
        size=read(ProtocolParsingInputOutput::tempBigBufferForUncompressedInput,CATCHCHALLENGER_COMMONBUFFERSIZE);
        if(size>0)
        {
            //std::vector<char> tempDataToDebug(ProtocolParsingInputOutput::commonBuffer,size);
            //qDebug() << "without header cut" << tempDataToDebug.toHex() << "and size" << size;
        }
    }
    if(size>0)
        destination->internalSendRawSmallPacket(ProtocolParsingInputOutput::tempBigBufferForUncompressedInput,size);
    return true;
}

void ProtocolParsingInputOutput::parseIncommingData()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(parseIncommingDataCount>0)
    {
        qDebug() << "Multiple client on same section";
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
                std::stringLiteral(" parseIncommingData(): socket->bytesAvailable(): %1, header_cut: %2").arg(socket->bytesAvailable()).arg(header_cut.size()));
    #endif
    #endif

    while(1)
    {
        int32_t size;
        uint32_t cursor=0;
        if(!header_cut.isEmpty())
        {
            const unsigned int &size_to_get=CATCHCHALLENGER_COMMONBUFFERSIZE-header_cut.size();
            memcpy(ProtocolParsingInputOutput::tempBigBufferForUncompressedInput,header_cut.constData(),header_cut.size());
            size=read(ProtocolParsingInputOutput::tempBigBufferForUncompressedInput,size_to_get)+header_cut.size();
            if(size>0)
            {
                //std::vector<char> tempDataToDebug(ProtocolParsingInputOutput::commonBuffer+header_cut.size(),size-header_cut.size());
                //qDebug() << "with header cut" << header_cut << tempDataToDebug.toHex() << "and size" << size;
            }
            header_cut.clear();
        }
        else
        {
            size=read(ProtocolParsingInputOutput::tempBigBufferForUncompressedInput,CATCHCHALLENGER_COMMONBUFFERSIZE);
            if(size>0)
            {
                //std::vector<char> tempDataToDebug(ProtocolParsingInputOutput::commonBuffer,size);
                //qDebug() << "without header cut" << tempDataToDebug.toHex() << "and size" << size;
            }
        }
        if(size<=0)
        {
            /*messageParsingLayer(
#ifndef CATCHCHALLENGERSERVERDROPIFCLENT
std::to_string(flags & 0x10)+
#endif
std::stringLiteral(" parseIncommingData(): size returned is 0!"));*/
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            parseIncommingDataCount--;
            #endif
            return;
        }

        do
        {
            if(!parseIncommingDataRaw(ProtocolParsingInputOutput::tempBigBufferForUncompressedInput,size,cursor))
                break;
        } while(cursor<(uint32_t)size);

        if(size<CATCHCHALLENGER_COMMONBUFFERSIZE)
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            parseIncommingDataCount--;
            #endif
            return;
        }
    }
    #ifdef PROTOCOLPARSINGDEBUG
    messageParsingLayer(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                std::to_string(flags & 0x10)+
                #endif
    std::stringLiteral(" parseIncommingData(): finish parse the input"));
    #endif
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    parseIncommingDataCount--;
    #endif
}

bool ProtocolParsingBase::parseIncommingDataRaw(const char * const commonBuffer, const uint32_t &size, uint32_t &cursor)
{
    if(!parseHeader(commonBuffer,size,cursor))
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        qDebug() << "Break due to need more in header";
        #endif
        return false;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(cursor==0 && !(flags & 0x80))
    {
        qDebug() << "Critical bug";
        abort();
    }
    #endif
    if(!parseQueryNumber(commonBuffer,size,cursor))
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        qDebug() << "Break due to need more in query number";
        #endif
        return false;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(cursor==0 && !(flags & 0x80))
    {
        qDebug() << "Critical bug";
        abort();
    }
    #endif
    if(!parseDataSize(commonBuffer,size,cursor))
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        //qDebug() << "Break due to need more in parse data size";
        #endif
        return false;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(cursor==0 && !(flags & 0x80))
    {
        qDebug() << "Critical bug";
        abort();
    }
    #endif
    if(!parseData(commonBuffer,size,cursor))
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        //qDebug() << "Break due to need more in parse data";
        #endif
        return false;
    }
    //parseDispatch(); do into above function
    dataClear();
    return true;
}

bool ProtocolParsingBase::isReply() const
{
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
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
    return false;
}

bool ProtocolParsingBase::parseHeader(const char * const commonBuffer,const uint32_t &size,uint32_t &cursor)
{
    if(!(flags & 0x80))
    {
        if((size-cursor)<sizeof(uint8_t))//ignore because first int is cuted!
            return false;
        packetCode=*(commonBuffer+cursor);
        cursor+=sizeof(uint8_t);

        //def query without the sub code

        if(isReply())
            return true;
        else
        {
            dataSize=ProtocolParsingBase::packetFixedSize[packetCode];
            if(dataSize==0xFF)
                return false;//packetCode code wrong
            else if(dataSize!=0xFE)
                flags |= 0x40;
            return true;
        }
    }
    else
        return true;
}

bool ProtocolParsingBase::parseQueryNumber(const char * const commonBuffer,const uint32_t &size,uint32_t &cursor)
{
    if(!(flags & 0x20) && (isReply() || packetCode>=0x80))
    {
        if((size-cursor)<sizeof(uint8_t))
        {
            //todo, write message: need more bytes
            return false;
        }
        queryNumber=*(commonBuffer+cursor);
        cursor+=sizeof(uint8_t);

        // it's reply
        if(isReply())
        {
            const uint8_t &replyTo=outputQueryNumberToPacketCode[queryNumber];
            //not a reply to a query
            if(replyTo==0x00)
                return false;
            dataSize=ProtocolParsingBase::packetFixedSize[256+replyTo-128];
            if(dataSize==0xFF)
                abort();//packetCode code wrong, how the output allow this! filter better the output
            else if(dataSize!=0xFE)
                flags |= 0x40;
        }
        else // it's query with reply
        {
            //size resolved before, into subCodeType step
        }
    }

    //set this parsing step is done
    flags |= 0x20;

    return true;
}

bool ProtocolParsingBase::parseDataSize(const char * const commonBuffer, const uint32_t &size,uint32_t &cursor)
{
    if(!(flags & 0x40))
    {
        #ifdef PROTOCOLPARSINGDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(flags & 0x10)+
                    #endif
        std::stringLiteral(" parseIncommingData(): !haveData_dataSize"));
        #endif
        //temp data
        if((size-cursor)<sizeof(uint32_t))
        {
            if((size-cursor)>0)
                header_cut.append(commonBuffer+cursor,(size-cursor));
            return false;
        }
        dataSize=le32toh(*reinterpret_cast<const uint32_t *>(commonBuffer+cursor));
        cursor+=sizeof(uint32_t);

        if(dataSize>(CATCHCHALLENGER_BIGBUFFERSIZE-8))
        {
            errorParsingLayer("packet size too big (define)");
            return false;
        }
        if(dataSize>16*1024*1024)
        {
            errorParsingLayer("packet size too big (hard)");
            return false;
        }

        flags |= 0x40;
    }
    return true;
}

bool ProtocolParsingBase::parseData(const char * const commonBuffer, const uint32_t &size,uint32_t &cursor)
{
    if(dataSize==0)
        return parseDispatch(NULL,0);
    if(dataToWithoutHeader.isEmpty())
    {
        //if have too many data, or just the size
        if(dataSize<=(size-cursor))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(cursor==0 && !(flags & 0x80))
            {
                qDebug() << "Critical bug";
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
            std::stringLiteral(" parseIncommingData(): remaining data: %1").arg((size-cursor)));
            #endif
            return returnVal;
        }
    }
    //if have too many data, or just the size
    if((dataSize-dataToWithoutHeader.size())<=(size-cursor))
    {
        const uint32_t &size_to_append=dataSize-dataToWithoutHeader.size();
        dataToWithoutHeader.append(commonBuffer+cursor,size_to_append);
        cursor+=size_to_append;
        #ifdef PROTOCOLPARSINGDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(flags & 0x10)+
                    #endif
        std::stringLiteral(" parseIncommingData(): remaining data: %1, buffer data: %2").arg((size-cursor)).arg(std::string(std::vector<char>(commonBuffer,sizeof(commonBuffer)).toHex())));
        #endif
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(dataSize!=(uint32_t)dataToWithoutHeader.size())
        {
            errorParsingLayer("wrong data size here");
            return false;
        }
        #endif
        return parseDispatch(dataToWithoutHeader.constData(),dataToWithoutHeader.size());
    }
    else //if need more data
    {
        dataToWithoutHeader.append(commonBuffer+cursor,(size-cursor));
        #ifdef PROTOCOLPARSINGDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(flags & 0x10)+
                    #endif
        std::stringLiteral(" parseIncommingData(): need more to recompose: %1").arg(dataSize-dataToWithoutHeader.size()));
        #endif
        cursor=size;
        return false;
    }
}

uint32_t ProtocolParsing::computeDecompression(const char* const source, char* const dest, int compressedSize, int maxDecompressedSize, const CompressionType &compressionType)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(maxDecompressedSize<compressedSize)
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
            /* memcpy(dest,source,compressedSize);
            return compressedSize;*/
        break;
        case CompressionType::Zlib:
        default:
        {
            const std::vector<char> &newData=qUncompress(std::vector<char>(source,compressedSize));
            if(newData.size()>maxDecompressedSize)
                return -1;
            memcpy(dest,newData.constData(),newData.size());
            return newData.size();
        }
        break;
        case CompressionType::Xz:
        {
            const std::vector<char> &newData=ProtocolParsingBase::lzmaUncompress(std::vector<char>(source,compressedSize));
            if(newData.size()>maxDecompressedSize)
                return -1;
            memcpy(dest,newData.constData(),newData.size());
            return newData.size();
        }
        break;
        case CompressionType::Lz4:
            return LZ4_decompress_safe(source,dest,compressedSize,maxDecompressedSize);
        break;
    }
}

bool ProtocolParsingBase::parseDispatch(const char * const data, const int &size)
{
    #ifdef ProtocolParsingInputOutputDEBUG
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(flags & 0x10)
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(flags & 0x10)+
                    #endif
        std::stringLiteral(" parseIncommingData(): parse message as client"));
    else
    #else
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(flags & 0x10)+
                    #endif
        std::stringLiteral(" parseIncommingData(): parse message as server"));
    #endif
    #endif
    #ifdef ProtocolParsingInputOutputDEBUG
    messageParsingLayer(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                std::to_string(flags & 0x10)+
                #endif
    std::stringLiteral(" parseIncommingData(): data: %1").arg(std::string(data.toHex())));
    #endif
    //message
    if(isReply())
    {
        #ifdef ProtocolParsingInputOutputDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(flags & 0x10)+
                    #endif
        std::stringLiteral(" parseIncommingData(): need_query_number && is_reply && reply_subCodeType.contains(queryNumber), queryNumber: %1, packetCode: %2").arg(queryNumber).arg(packetCode));
        #endif
        const uint8_t &replyTo=outputQueryNumberToPacketCode[queryNumber];

        const bool &returnValue=parseReplyData(replyTo,queryNumber,data,size);
        outputQueryNumberToPacketCode[queryNumber]=0x00;
        return returnValue;
    }
    if(packetCode<0x80)
    {
        #ifdef ProtocolParsingInputOutputDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(flags & 0x10)+
                    #endif
        std::stringLiteral(" parseIncommingData(): !need_query_number && !need_subCodeType, packetCode: %1").arg(packetCode));
        #endif
        return parseMessage(packetCode,data,size);
    }
    else
    {
        //query
        #ifdef ProtocolParsingInputOutputDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(flags & 0x10)+
                    #endif
        std::stringLiteral(" parseIncommingData(): need_query_number && !is_reply, packetCode: %1").arg(packetCode));
        #endif
        storeInputQuery(packetCode,queryNumber);
        return parseQuery(packetCode,queryNumber,data,size);
    }
}

void ProtocolParsingBase::dataClear()
{
    dataToWithoutHeader.clear();
    flags &= 0x10;
}

#ifndef EPOLLCATCHCHALLENGERSERVER
std::vector<std::string> ProtocolParsingBase::getQueryRunningList()
{
    std::vector<std::string> returnedList;
    std::unordered_mapIterator<uint8_t,uint8_t> i(waitedReply_packetCode);
    while (i.hasNext()) {
        i.next();
        if(waitedReply_subCodeType.contains(i.key()))
            returnedList << std::stringLiteral("%1 %2").arg(i.value()).arg(waitedReply_subCodeType.value(i.key()));
        else
            returnedList << std::to_string(i.value());
    }
    return returnedList;
}
#endif

void ProtocolParsingBase::storeInputQuery(const uint8_t &packetCode,const uint8_t &queryNumber)
{
    Q_UNUSED(packetCode);
    Q_UNUSED(queryNumber);
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
    //register the size of the reply to send
    inputQueryNumberToPacketCode[queryNumber]=packetCode;
}
