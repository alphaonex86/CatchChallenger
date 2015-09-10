#include "ProtocolParsing.h"
#include "GeneralVariable.h"
#include "ProtocolParsingCheck.h"
#include <iostream>

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
#include <lzma.h>
#include "lz4/lz4.h"
#endif

using namespace CatchChallenger;

void ProtocolParsingBase::newOutputQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber)
{
    if(waitedReply_mainCodeType.find(queryNumber)!=waitedReply_mainCodeType.cend())
    {
        errorParsingLayer("Query with this query number already found");
        return;
    }
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.find(mainCodeType)==mainCodeWithoutSubCodeTypeClientToServer.cend())
        {
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        std::to_string(isClient)+
                        #endif
                        " ProtocolParsingInputOutput::newOutputQuery(): queryNumber: "+std::to_string(queryNumber)+
                        ", mainCodeType: "+std::to_string(mainCodeType)+
                        ", try send without sub code, but not registred as is");
            return;
        }
        #endif
        if(replySizeOnlyMainCodePacketServerToClient.find(mainCodeType)!=replySizeOnlyMainCodePacketServerToClient.cend())
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionOnlyMainCodePacketServerToClient.find(mainCodeType)!=replyComressionOnlyMainCodePacketServerToClient.cend())
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            std::to_string(isClient)+
                            #endif
                            " ProtocolParsingInputOutput::newOutputQuery(): queryNumber: "+std::to_string(queryNumber)+
                            ", mainCodeType: "+std::to_string(mainCodeType)+
                            ", compression disabled because have fixed size");
            #endif
            #endif
        }
    }
    else
    #endif
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeServerToClient.find(mainCodeType)==mainCodeWithoutSubCodeTypeServerToClient.cend())
        {
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        std::to_string(isClient)+
                        #endif
                        " ProtocolParsingInputOutput::newOutputQuery(): queryNumber: "+std::to_string(queryNumber)+
                        ", mainCodeType: "+std::to_string(mainCodeType)+
                        ", try send without sub code, but not registred as is");
            return;
        }
        #endif
        if(replySizeOnlyMainCodePacketClientToServer.find(mainCodeType)!=replySizeOnlyMainCodePacketClientToServer.cend())
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionOnlyMainCodePacketClientToServer.find(mainCodeType)!=replyComressionOnlyMainCodePacketClientToServer.cend())
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            std::to_string(isClient)+
                            #endif
                            " ProtocolParsingInputOutput::newOutputQuery(): queryNumber: "+std::to_string(queryNumber)+
                            ", mainCodeType: "+std::to_string(mainCodeType)+
                            ", compression disabled because have fixed size");
            #endif
            #endif
        }
    }
    #ifdef ProtocolParsingInputOutputDEBUG
    messageParsingLayer(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                std::to_string(isClient)+
                #endif
    std::stringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2").arg(queryNumber).arg(mainCodeType));
    #endif
    waitedReply_mainCodeType[queryNumber]=mainCodeType;
}

void ProtocolParsingBase::newFullOutputQuery(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber)
{
    if(waitedReply_mainCodeType.find(queryNumber)!=waitedReply_mainCodeType.cend())
    {
        errorParsingLayer("Query with this query number already found");
        return;
    }
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.find(mainCodeType)!=mainCodeWithoutSubCodeTypeClientToServer.cend())
        {
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        std::to_string(isClient)+
                        #endif
                        " ProtocolParsingInputOutput::newOutputQuery(): queryNumber: "+std::to_string(queryNumber)+
                        ", mainCodeType: "+std::to_string(mainCodeType)+
                        ", subCodeType: "+std::to_string(subCodeType)+
                        ", try send with sub code, but not registred as is");
            return;
        }
        #endif
        if(replySizeMultipleCodePacketServerToClient.find(mainCodeType)!=replySizeMultipleCodePacketServerToClient.cend())
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionMultipleCodePacketServerToClient.find(mainCodeType)!=replyComressionMultipleCodePacketServerToClient.cend())
                if(replyComressionMultipleCodePacketServerToClient.at(mainCodeType).find(subCodeType)!=replyComressionMultipleCodePacketServerToClient.at(mainCodeType).cend())
                    messageParsingLayer(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                std::to_string(isClient)+
                                #endif
                                " ProtocolParsingInputOutput::newOutputQuery(): queryNumber: "+std::to_string(queryNumber)+
                                ", mainCodeType: "+std::to_string(mainCodeType)+
                                ", subCodeType: "+std::to_string(subCodeType)+
                                " compression disabled because have fixed size");
            #endif
            #endif
        }
    }
    else
    #endif
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeServerToClient.find(mainCodeType)!=mainCodeWithoutSubCodeTypeServerToClient.cend())
        {
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        std::to_string(isClient)+
                        #endif
                        " ProtocolParsingInputOutput::newOutputQuery(): queryNumber: "+std::to_string(queryNumber)+
                        ", mainCodeType: "+std::to_string(mainCodeType)+
                        ", subCodeType: "+std::to_string(subCodeType)+
                        ", try send with sub code, but not registred as is");
            return;
        }
        #endif
        if(replySizeMultipleCodePacketServerToClient.find(mainCodeType)!=replySizeMultipleCodePacketServerToClient.cend())
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionMultipleCodePacketClientToServer.find(mainCodeType)!=replyComressionMultipleCodePacketClientToServer.cend())
                if(replyComressionMultipleCodePacketClientToServer.at(mainCodeType).find(subCodeType)!=replyComressionMultipleCodePacketClientToServer.at(mainCodeType).cend())
                    messageParsingLayer(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                std::to_string(isClient)+
                                #endif
                                " ProtocolParsingInputOutput::newOutputQuery(): queryNumber: "+std::to_string(queryNumber)+
                                ", mainCodeType: "+std::to_string(mainCodeType)+
                                ", subCodeType: "+std::to_string(subCodeType)+
                                " compression disabled because have fixed size");
            #endif
            #endif
        }
    }
    #ifdef ProtocolParsingInputOutputDEBUG
    messageParsingLayer(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                std::to_string(isClient)+
                #endif
    std::stringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
    #endif
    waitedReply_mainCodeType[queryNumber]=mainCodeType;
    waitedReply_subCodeType[queryNumber]=subCodeType;
}

bool ProtocolParsingBase::postReplyData(const uint8_t &queryNumber, const char * const data,const int &size)
{
    int replyOutputSizeInt=-1;
    if(replyOutputSize.find(queryNumber)!=replyOutputSize.cend())
    {
        replyOutputSizeInt=replyOutputSize.at(queryNumber);
        replyOutputSize.erase(queryNumber);
    }
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    CompressionType compressionType=CompressionType::None;
    if(replyOutputSizeInt==-1)
    {
        if(replyOutputCompression.find(queryNumber)!=replyOutputCompression.cend())
        {
            compressionType=getCompressType();
            replyOutputCompression.erase(queryNumber);
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(replyOutputCompression.find(queryNumber)!=replyOutputCompression.cend())
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        std::to_string(isClient) <<
                        #endif
                        " postReplyData("
                        << std::to_string(queryNumber)
                        << ",{}) compression disabled because have fixed size"
                        << std::endl;
            abort();
        }
        #endif
    }
    #endif

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(queryReceived.find(queryNumber)==queryReceived.cend())
    {
        std::cerr <<
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(isClient) <<
                    #endif
                    " ProtocolParsingInputOutput::postReplyData(): try reply to queryNumber: "
                    << std::to_string(queryNumber)
                    << ", but this query is not into the list"
                    << std::endl;
        return 0;
    }
    else
        queryReceived.erase(queryNumber);
    #endif

    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size>(CATCHCHALLENGER_BIGBUFFERSIZE-16))
    {
        errorParsingLayer("Buffer in input is too big and will do buffer overflow, size: "+std::to_string(size)+", line "+std::to_string(__LINE__));
        abort();
    }
    #endif

    const int &newSize=ProtocolParsingBase::computeReplyData(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                isClient,
                #endif
                ProtocolParsingBase::tempBigBufferForOutput,queryNumber,data,size,replyOutputSizeInt
                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                ,compressionType
                #endif
                );
    if(newSize==0)
        return false;
    return internalPackOutcommingData(ProtocolParsingBase::tempBigBufferForOutput,newSize);
    #else
    QByteArray bigBufferForOutput;
    bigBufferForOutput.resize(16+size);
    const int &newSize=ProtocolParsingBase::computeReplyData(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                isClient,
                #endif
                bigBufferForOutput.data(),queryNumber,data,size,replyOutputSizeInt,compressionType);
    if(newSize==0)
        return false;
    return internalPackOutcommingData(bigBufferForOutput.data(),newSize);
    #endif
}

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
QByteArray ProtocolParsingBase::computeCompression(const QByteArray &data,const CompressionType &compressionType)
{
    switch(compressionType)
    {
        case CompressionType::None:
            return data;
        break;
        case CompressionType::Zlib:
        default:
            return qCompress(data,ProtocolParsing::compressionLevel);
        break;
        case CompressionType::Xz:
            return lzmaCompress(data);
        break;
        case CompressionType::Lz4:
            QByteArray dest;
            dest.resize(LZ4_compressBound(data.size()));
            dest.resize(LZ4_compress_default(data.constData(),dest.data(),data.size(),dest.size()));
            return dest;
        break;
    }
}
#endif

bool ProtocolParsingBase::packFullOutcommingData(const uint8_t &mainCodeType,const uint8_t &subCodeType,const char * const data,const int &size)
{
    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size>(CATCHCHALLENGER_BIGBUFFERSIZE-16))
    {
        errorParsingLayer("Buffer in input is too big and will do buffer overflow, size: "+std::to_string(size)+", line "+std::to_string(__LINE__));
        abort();
    }
    #endif

    const int &newSize=computeFullOutcommingData(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                isClient,
                #endif
                ProtocolParsingBase::tempBigBufferForOutput,
                mainCodeType,subCodeType,data,size);
    if(newSize==0)
        return false;
    return internalPackOutcommingData(ProtocolParsingBase::tempBigBufferForOutput,newSize);
    #else
    QByteArray bigBufferForOutput;
    bigBufferForOutput.resize(16+size);
    const int &newSize=computeFullOutcommingData(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                isClient,
                #endif
                bigBufferForOutput.data(),
                mainCodeType,subCodeType,data,size);
    if(newSize==0)
        return false;
    return internalPackOutcommingData(bigBufferForOutput.data(),newSize);
    #endif
}

bool ProtocolParsingBase::packOutcommingData(const uint8_t &mainCodeType,const char * const data,const int &size)
{
    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size>(CATCHCHALLENGER_BIGBUFFERSIZE-16))
    {
        errorParsingLayer("Buffer in input is too big and will do buffer overflow, size: "+std::to_string(size)+", line "+std::to_string(__LINE__));
        abort();
    }
    #endif

    const int &newSize=computeOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            isClient,
            #endif
            ProtocolParsingBase::tempBigBufferForOutput,
            mainCodeType,data,size);
    if(newSize==0)
        return false;
    return internalPackOutcommingData(ProtocolParsingBase::tempBigBufferForOutput,newSize);
    #else
    QByteArray bigBufferForOutput;
    bigBufferForOutput.resize(16+size);
    const int &newSize=computeOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            isClient,
            #endif
            bigBufferForOutput.data(),
            mainCodeType,data,size);
    if(newSize==0)
        return false;
    return internalPackOutcommingData(bigBufferForOutput.data(),newSize);
    #endif
}

bool ProtocolParsingBase::packOutcommingQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const int &size)
{
    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size>(CATCHCHALLENGER_BIGBUFFERSIZE-16))
    {
        errorParsingLayer("Buffer in input is too big and will do buffer overflow, size: "+std::to_string(size)+", line "+std::to_string(__LINE__));
        abort();
    }
    #endif

    const int &newSize=computeOutcommingQuery(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            isClient,
            #endif
            ProtocolParsingBase::tempBigBufferForOutput,
            mainCodeType,queryNumber,data,size);
    if(newSize==0)
        return false;
    newOutputQuery(mainCodeType,queryNumber);
    return internalPackOutcommingData(ProtocolParsingBase::tempBigBufferForOutput,newSize);
    #else
    QByteArray bigBufferForOutput;
    bigBufferForOutput.resize(16+size);
    const int &newSize=computeOutcommingQuery(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            isClient,
            #endif
            bigBufferForOutput.data(),
            mainCodeType,queryNumber,data,size);
    if(newSize==0)
        return false;
    newOutputQuery(mainCodeType,queryNumber);
    return internalPackOutcommingData(bigBufferForOutput.data(),newSize);
    #endif
}

bool ProtocolParsingBase::packFullOutcommingQuery(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber,const char * const data,const int &size)
{
    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size>(CATCHCHALLENGER_BIGBUFFERSIZE-16))
    {
        errorParsingLayer("Buffer in input is too big and will do buffer overflow, size: "+std::to_string(size)+", line "+std::to_string(__LINE__));
        abort();
    }
    #endif

    const int &newSize=computeFullOutcommingQuery(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            isClient,
            #endif
            ProtocolParsingBase::tempBigBufferForOutput,
            mainCodeType,subCodeType,queryNumber,data,size);
    if(newSize==0)
        return false;
    newFullOutputQuery(mainCodeType,subCodeType,queryNumber);
    return internalPackOutcommingData(ProtocolParsingBase::tempBigBufferForOutput,newSize);
    #else
    QByteArray bigBufferForOutput;
    bigBufferForOutput.resize(16+size+(200+size*2/*overhead of empty data char* compression*/));
    const int &newSize=computeFullOutcommingQuery(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            isClient,
            #endif
            bigBufferForOutput.data(),
            mainCodeType,subCodeType,queryNumber,data,size);
    if(newSize==0)
        return false;
    newFullOutputQuery(mainCodeType,subCodeType,queryNumber);
    const bool returnedValue=internalPackOutcommingData(bigBufferForOutput.data(),newSize);
    return returnedValue;
    #endif
}

bool ProtocolParsingBase::internalPackOutcommingData(const char * const data,const int &size)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size<=0)
    {
        std::cerr << "ProtocolParsingInputOutput::internalPackOutcommingData size is null: " << __LINE__ << std::endl;
        return false;
    }
    #endif
    #ifdef PROTOCOLPARSINGDEBUG
    messageParsingLayer("internalPackOutcommingData(): start");
    #endif
    #ifdef DEBUG_PROTOCOLPARSING_RAW_NETWORK
    qDebug() << std::string(std::stringLiteral("Sended packet size: %1: %2").arg(size).arg(std::string(QByteArray(data,size).toHex())));
    #endif // DEBUG_PROTOCOLPARSING_RAW_NETWORK

    if(size<=CATCHCHALLENGER_MAX_PACKET_SIZE)
    {
        const int &byteWriten = write(data,size);
        if(Q_UNLIKELY(size!=byteWriten))
        {
            disconnectClient();
            return false;
        }
        return true;
    }
    else
    {
        int cursor=0;
        int byteWriten;
        while(Q_LIKELY(cursor<size))
        {
            const int &remaining_size=size-cursor;
            int size_to_send;
            if(remaining_size<CATCHCHALLENGER_MAX_PACKET_SIZE)
                size_to_send=remaining_size;
            else
                size_to_send=CATCHCHALLENGER_MAX_PACKET_SIZE;
            byteWriten = write(data+cursor,size_to_send);
            if(Q_UNLIKELY(size_to_send!=byteWriten))
            {
                disconnectClient();
                return false;
            }
            cursor+=size_to_send;
        }
        return true;
    }
    return true;
}

//no control to be more fast
bool ProtocolParsingBase::internalSendRawSmallPacket(const char * const data,const int &size)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size<=0)
    {
        std::cerr << "ProtocolParsingInputOutput::internalSendRawSmallPacket size is null" << __LINE__ << std::endl;
        return false;
    }
    #endif
    #ifdef PROTOCOLPARSINGDEBUG
    messageParsingLayer("internalPackOutcommingData(): start");
    #endif
    #ifdef DEBUG_PROTOCOLPARSING_RAW_NETWORK
    messageParsingLayer(std::stringLiteral("Sended packet size: %1: %2").arg(size).arg(std::string(QByteArray(data,size).toHex())));
    #endif // DEBUG_PROTOCOLPARSING_RAW_NETWORK
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size>CATCHCHALLENGER_MAX_PACKET_SIZE)
    {
        errorParsingLayer("ProtocolParsingInputOutput::sendRawSmallPacket(): Packet to big: "+std::to_string(size));
        return false;
    }
    #endif

    const int &byteWriten = write(data,size);
    if(Q_UNLIKELY(size!=byteWriten))
    {
        disconnectClient();
        return false;
    }
    return true;
}

qint8 ProtocolParsingBase::encodeSize(char *data,const uint32_t &size)
{
    if(size<=0xFF)
    {
        memcpy(data,&size,sizeof(uint8_t));
        return sizeof(uint8_t);
    }
    else if(size<=0xFFFF)
    {
        const uint16_t &newSize=htole16(size);
        memcpy(data,&ProtocolParsingBase::sizeHeaderNulluint16_t,sizeof(uint8_t));
        memcpy(data+sizeof(uint8_t),&newSize,sizeof(newSize));
        return sizeof(uint8_t)+sizeof(uint16_t);
    }
    else
    {
        const uint32_t &newSize=htole32(size);
        memcpy(data,&ProtocolParsingBase::sizeHeaderNulluint16_t,sizeof(uint16_t));
        memcpy(data+sizeof(uint16_t),&newSize,sizeof(newSize));
        return sizeof(uint16_t)+sizeof(uint32_t);
    }
}

int ProtocolParsingBase::computeOutcommingData(
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        const bool &isClient,
        #endif
        char *buffer,
        const uint8_t &mainCodeType,const char * const data,const int &size)
{
    buffer[0]=mainCodeType;

    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.find(mainCodeType)==mainCodeWithoutSubCodeTypeClientToServer.cend())
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
                        " ProtocolParsingInputOutput::packOutcommingData(): mainCodeType: "
                        << mainCodeType
                        << ", try send without sub code, but not registred as is"
                        << std::endl;
            return 0;
        }
        if(mainCode_IsQueryClientToServer.find(mainCodeType)!=mainCode_IsQueryClientToServer.cend())
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
                        " ProtocolParsingInputOutput::packOutcommingQuery(): mainCodeType: "
                        << mainCodeType
                        << ", try send as normal data, but not registred as is" << std::endl;
            return 0;
        }
        #endif
        if(sizeOnlyMainCodePacketClientToServer.find(mainCodeType)==sizeOnlyMainCodePacketClientToServer.cend())
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                            " packOutcommingData("
                            << mainCodeType
                            << ",{}) dropped because can be size==0 if not fixed size"
                            << std::endl;
                return 0;
            }
            #endif
            const int &newSize=encodeSize(buffer+1,size);
            if(size>0)
            {
                memcpy(buffer+1+newSize,data,size);
                return 1+newSize+size;
            }
            else
                return 1+newSize;
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=sizeOnlyMainCodePacketClientToServer.at(mainCodeType))
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                            " packOutcommingData("
                            << mainCodeType
                            << ",{}) dropped because can be size!=fixed size"
                            << std::endl;
                return 0;
            }
            #endif
            if(size>0)
            {
                memcpy(buffer+1,data,size);
                return 1+size;
            }
            else
                return 1;
        }
    }
    else
    #endif
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeServerToClient.find(mainCodeType)==mainCodeWithoutSubCodeTypeServerToClient.cend())
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
                        " ProtocolParsingInputOutput::packOutcommingData(): mainCodeType: "
                        << mainCodeType
                        << ", try send without sub code, but not registred as is"
                        << std::endl;
            return 0;
        }
        if(mainCode_IsQueryServerToClient.find(mainCodeType)!=mainCode_IsQueryServerToClient.cend())
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
                        " ProtocolParsingInputOutput::packOutcommingQuery(): mainCodeType: "
                        << mainCodeType
                        << ", try send as normal data, but not registred as is"
                        << std::endl;
            return 0;
        }
        #endif
        if(sizeOnlyMainCodePacketServerToClient.find(mainCodeType)==sizeOnlyMainCodePacketServerToClient.cend())
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                            " packOutcommingData("
                            << mainCodeType
                            << ",{}) dropped because can be size==0 if not fixed size"
                            << std::endl;
                return 0;
            }
            #endif
            const int &newSize=encodeSize(buffer+1,size);
            if(size>0)
            {
                memcpy(buffer+1+newSize,data,size);
                return 1+newSize+size;
            }
            else
                return 1+newSize;
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=sizeOnlyMainCodePacketServerToClient.at(mainCodeType))
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                            " packOutcommingData("
                            << mainCodeType
                            << ",{}) dropped because can be size!=fixed size"
                            << std::endl;
                return 0;
            }
            #endif
            if(size>0)
            {
                memcpy(buffer+1,data,size);
                return 1+size;
            }
            else
                return 1;
        }
    }
}

int ProtocolParsingBase::computeOutcommingQuery(
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        const bool &isClient,
        #endif
        char *buffer,
        const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const int &size)
{
    buffer[0]=mainCodeType;
    buffer[1]=queryNumber;

    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.find(mainCodeType)==mainCodeWithoutSubCodeTypeClientToServer.cend())
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
                        " ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: "
                        << queryNumber
                        << ", mainCodeType: "
                        << mainCodeType
                        << ", try send without sub code, but not registred as is"
                        << std::endl;
            return 0;
        }
        if(mainCode_IsQueryClientToServer.find(mainCodeType)==mainCode_IsQueryClientToServer.cend())
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
                        " ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: "
                        << queryNumber
                        << ", mainCodeType: "
                        << mainCodeType
                        << ", try send as query, but not registred as is"
                        << std::endl;
            return 0;
        }
        #endif
        if(sizeOnlyMainCodePacketClientToServer.find(mainCodeType)==sizeOnlyMainCodePacketClientToServer.cend())
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                            " packOutcommingQuery("
                            << mainCodeType
                            << ",{}) dropped because can be size==0 if not fixed size"
                            << std::endl;
                return 0;
            }
            #endif
            const int &newSize=encodeSize(buffer+2,size);
            if(size>0)
            {
                memcpy(buffer+2+newSize,data,size);
                return 2+newSize+size;
            }
            else
                return 2+newSize;
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=sizeOnlyMainCodePacketClientToServer.at(mainCodeType))
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                            " packOutcommingQuery("
                            << mainCodeType
                            << ",{}) dropped because can be size!=fixed size"
                            << std::endl;
                return 0;
            }
            #endif
            if(size>0)
            {
                memcpy(buffer+2,data,size);
                return 2+size;
            }
            else
                return 2;
        }
    }
    else
    #endif
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeServerToClient.find(mainCodeType)==mainCodeWithoutSubCodeTypeServerToClient.cend())
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
                        " ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: "
                        << queryNumber
                        << ", mainCodeType: "
                        << mainCodeType
                        << ", try send without sub code, but not registred as is"
                        << std::endl;
            return 0;
        }
        if(mainCode_IsQueryServerToClient.find(mainCodeType)==mainCode_IsQueryServerToClient.cend())
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
                        " ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: "
                        << queryNumber
                        << ", mainCodeType: "
                        << mainCodeType
                        << ", try send as query, but not registred as is"
                        << std::endl;
            return 0;
        }
        #endif
        if(sizeOnlyMainCodePacketServerToClient.find(mainCodeType)==sizeOnlyMainCodePacketServerToClient.cend())
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                            " packOutcommingQuery("
                            << mainCodeType
                            << ",{}) dropped because can be size==0 if not fixed size"
                            << std::endl;
                return 0;
            }
            #endif
            const int &newSize=encodeSize(buffer+2,size);
            if(size>0)
            {
                memcpy(buffer+2+newSize,data,size);
                return 2+newSize+size;
            }
            else
                return 2+newSize;
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=sizeOnlyMainCodePacketServerToClient.at(mainCodeType))
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                            " packOutcommingQuery("
                            << mainCodeType
                            << ",{}) dropped because can be size!=fixed size"
                            << std::endl;
                return 0;
            }
            #endif
            if(size>0)
            {
                memcpy(buffer+2,data,size);
                return 2+size;
            }
            else
                return 2;
        }
    }
}

int ProtocolParsingBase::computeFullOutcommingQuery(
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        const bool &isClient,
        #endif
        char *buffer,
        const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber,const char * const data,const int &size)
{
    buffer[0]=mainCodeType;
    buffer[1]=subCodeType;
    buffer[2]=queryNumber;

    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.find(mainCodeType)!=mainCodeWithoutSubCodeTypeClientToServer.cend()
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
                        " ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: "
                        << queryNumber
                        << ", mainCodeType: "
                        << mainCodeType
                        << ", subCodeType: "
                        << subCodeType
                        << ", try send with sub code, but not registred as is"
                        << std::endl;
            return 0;
        }
        if(mainCode_IsQueryClientToServer.find(mainCodeType)==mainCode_IsQueryClientToServer.cend())
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
                        " ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: "
                        << queryNumber
                        << ", mainCodeType: "
                        << mainCodeType
                        << ", subCodeType: "
                        << subCodeType
                        << ", try send as query, but not registred as is"
                        << std::endl;
            return 0;
        }
        #endif
        if(sizeMultipleCodePacketClientToServer.find(mainCodeType)==sizeMultipleCodePacketClientToServer.cend())
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(compressionMultipleCodePacketClientToServer.find(mainCodeType)!=compressionMultipleCodePacketClientToServer.cend())
                if(compressionMultipleCodePacketClientToServer.at(mainCodeType).find(subCodeType)!=compressionMultipleCodePacketClientToServer.at(mainCodeType).cend())
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    switch(compressionTypeClient)
                    {
                        case CompressionType::Xz:
                        case CompressionType::Zlib:
                        default:
                        {
                            const QByteArray &compressedData(computeCompression(QByteArray(data,size),compressionTypeClient));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(size==0)
                            {
                                std::cerr <<
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            isClient <<
                                            #endif
                                std::stringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                                return 0;
                            }
                            #endif
                            const int &newSize=encodeSize(buffer+3,compressedData.size());
                            if(compressedData.size()>0)
                            {
                                memcpy(buffer+3+newSize,compressedData.constData(),compressedData.size());
                                return 3+newSize+compressedData.size();
                            }
                            else
                                return 3+newSize;
                        }
                        break;
                        case CompressionType::None:
                        break;
                    }
                }
            #endif
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                std::stringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return 0;
            }
            #endif
            const int &newSize=encodeSize(buffer+3,size);
            if(size>0)
            {
                memcpy(buffer+3+newSize,data,size);
                return 3+newSize+size;
            }
            else
                return 3+newSize;
        }
        else if(!sizeMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    switch(compressionTypeClient)
                    {
                        case CompressionType::Xz:
                        case CompressionType::Zlib:
                        default:
                        {
                            const QByteArray &compressedData(computeCompression(QByteArray(data,size),compressionTypeClient));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(size==0)
                            {
                                std::cerr <<
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            isClient <<
                                            #endif
                                std::stringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                                return 0;
                            }
                            #endif
                            const int &newSize=encodeSize(buffer+3,compressedData.size());
                            if(compressedData.size()>0)
                            {
                                memcpy(buffer+3+newSize,compressedData.constData(),compressedData.size());
                                return 3+newSize+compressedData.size();
                            }
                            else
                                return 3+newSize;
                        }
                        break;
                        case CompressionType::None:
                        break;
                    }
                }
            #endif
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                std::stringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return 0;
            }
            #endif
            const int &newSize=encodeSize(buffer+3,size);
            if(size>0)
            {
                memcpy(buffer+3+newSize,data,size);
                return 3+newSize+size;
            }
            else
                return 3+newSize;
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingQuery(%1,%2,%3) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            #endif
            if(size!=sizeMultipleCodePacketClientToServer.value(mainCodeType).value(subCodeType))
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                std::stringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
                return 0;
            }
            #endif
            if(size>0)
            {
                memcpy(buffer+3,data,size);
                return 3+size;
            }
            else
                return 3;
        }
    }
    else
    #endif
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
            std::stringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return 0;
        }
        if(!mainCode_IsQueryServerToClient.contains(mainCodeType))
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
            std::stringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send as query, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return 0;
        }
        #endif
        if(!sizeMultipleCodePacketServerToClient.contains(mainCodeType))
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(compressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    switch(compressionTypeServer)
                    {
                        case CompressionType::Xz:
                        case CompressionType::Zlib:
                        default:
                        {
                            const QByteArray &compressedData(computeCompression(QByteArray(data,size),compressionTypeServer));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(size==0)
                            {
                                std::cerr <<
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            isClient <<
                                            #endif
                                std::stringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                                return 0;
                            }
                            #endif
                            const int &newSize=encodeSize(buffer+3,compressedData.size());
                            if(compressedData.size()>0)
                            {
                                memcpy(buffer+3+newSize,compressedData.constData(),compressedData.size());
                                return 3+newSize+compressedData.size();
                            }
                            else
                                return 3+newSize;
                        }
                        break;
                        case CompressionType::None:
                        break;
                    }
                }
            #endif
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                std::stringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return 0;
            }
            #endif
            const int &newSize=encodeSize(buffer+3,size);
            if(size>0)
            {
                memcpy(buffer+3+newSize,data,size);
                return 3+newSize+size;
            }
            else
                return 3+newSize;
        }
        else if(!sizeMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(compressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    switch(compressionTypeServer)
                    {
                        case CompressionType::Xz:
                        case CompressionType::Zlib:
                        default:
                        {
                            const QByteArray &compressedData(computeCompression(QByteArray(data,size),compressionTypeServer));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(size==0)
                            {
                                std::cerr <<
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            isClient <<
                                            #endif
                                std::stringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                                return 0;
                            }
                            #endif
                            const int &newSize=encodeSize(buffer+3,compressedData.size());
                            if(compressedData.size()>0)
                            {
                                memcpy(buffer+3+newSize,compressedData.constData(),compressedData.size());
                                return 3+newSize+compressedData.size();
                            }
                            else
                                return 3+newSize;
                        }
                        break;
                        case CompressionType::None:
                        break;
                    }
                }
            #endif
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                std::stringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return 0;
            }
            #endif
            const int &newSize=encodeSize(buffer+3,size);
            if(size>0)
            {
                memcpy(buffer+3+newSize,data,size);
                return 3+newSize+size;
            }
            else
                return 3+newSize;
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(compressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingQuery(%1,%2,%3) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            #endif
            if(size!=sizeMultipleCodePacketServerToClient.value(mainCodeType).value(subCodeType))
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                std::stringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
                return 0;
            }
            #endif
            if(size>0)
            {
                memcpy(buffer+3,data,size);
                return 3+size;
            }
            else
                return 3;
        }
    }
}

int ProtocolParsingBase::computeFullOutcommingData(
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        const bool &isClient,
        #endif
        char *buffer,
        const uint8_t &mainCodeType,const uint8_t &subCodeType,const char * const data,const int &size)
{
    buffer[0]=mainCodeType;
    buffer[1]=subCodeType;

    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
            std::stringLiteral(" ProtocolParsingInputOutput::packOutcommingData(): mainCodeType: %1, subCodeType: %2, try send with sub code, but not registred as is").arg(mainCodeType).arg(subCodeType));
            return 0;
        }
        if(mainCode_IsQueryClientToServer.contains(mainCodeType))
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
            std::stringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): mainCodeType: %1, subCodeType: %2, try send as normal data, but not registred as is").arg(mainCodeType).arg(subCodeType));
            return 0;
        }
        #endif
        if(!sizeMultipleCodePacketClientToServer.contains(mainCodeType))
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingData(%1,%2) compression enabled").arg(mainCodeType).arg(subCodeType));
                    #endif
                    switch(compressionTypeClient)
                    {
                        case CompressionType::Xz:
                        case CompressionType::Zlib:
                        default:
                        {
                            const QByteArray &compressedData(computeCompression(QByteArray(data,size),compressionTypeClient));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(compressedData.size()==0)
                            {
                                std::cerr <<
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            isClient <<
                                            #endif
                                std::stringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                                return 0;
                            }
                            #endif
                            const int &newSize=encodeSize(buffer+2,compressedData.size());
                            if(compressedData.size()>0)
                            {
                                memcpy(buffer+2+newSize,compressedData.constData(),compressedData.size());
                                return 2+newSize+compressedData.size();
                            }
                            else
                                return 2+newSize;
                        }
                        break;
                        case CompressionType::None:
                        break;
                    }
                }
            #endif
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                std::stringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return 0;
            }
            #endif
            const int &newSize=encodeSize(buffer+2,size);
            if(size>0)
            {
                memcpy(buffer+2+newSize,data,size);
                return 2+newSize+size;
            }
            else
                return 2+newSize+size;
        }
        else if(!sizeMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingData(%1,%2) compression enabled").arg(mainCodeType).arg(subCodeType));
                    #endif
                    switch(compressionTypeClient)
                    {
                        case CompressionType::Xz:
                        case CompressionType::Zlib:
                        default:
                        {
                            const QByteArray &compressedData(computeCompression(QByteArray(data,size),compressionTypeClient));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(compressedData.size()==0)
                            {
                                std::cerr <<
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            isClient <<
                                            #endif
                                std::stringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                                return 0;
                            }
                            #endif
                            const int &newSize=encodeSize(buffer+2,compressedData.size());
                            if(compressedData.size()>0)
                            {
                                memcpy(buffer+2+newSize,compressedData.constData(),compressedData.size());
                                return 2+newSize+compressedData.size();
                            }
                            else
                                return 2+newSize;
                        }
                        break;
                        case CompressionType::None:
                        break;
                    }
                }
            #endif
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                std::stringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return 0;
            }
            #endif
            const int &newSize=encodeSize(buffer+2,size);
            if(size>0)
            {
                memcpy(buffer+2+newSize,data,size);
                return 2+newSize+size;
            }
            else
                return 2+newSize;
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType));
            #endif
            if(size!=sizeMultipleCodePacketClientToServer.value(mainCodeType).value(subCodeType))
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                std::stringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
                return 0;
            }
            #endif
            if(size>0)
            {
                memcpy(buffer+2,data,size);
                return 2+size;
            }
            else
                return 2;
        }
    }
    else
    #endif
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
            std::stringLiteral(" ProtocolParsingInputOutput::packOutcommingData(): mainCodeType: %1, subCodeType: %2, try send with sub code, but not registred as is").arg(mainCodeType).arg(subCodeType));
            return 0;
        }
        if(mainCode_IsQueryServerToClient.contains(mainCodeType))
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
            std::stringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): mainCodeType: %1, subCodeType: %2, try send as normal data, but not registred as is").arg(mainCodeType).arg(subCodeType));
            return 0;
        }
        #endif
        if(!sizeMultipleCodePacketServerToClient.contains(mainCodeType))
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(compressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType));
                    #endif
                    switch(compressionTypeServer)
                    {
                        case CompressionType::Xz:
                        case CompressionType::Zlib:
                        default:
                        {
                            const QByteArray &compressedData(computeCompression(QByteArray(data,size),compressionTypeServer));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(compressedData.size()==0)
                            {
                                std::cerr <<
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            isClient <<
                                            #endif
                                std::stringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                                return 0;
                            }
                            #endif
                            const int &newSize=encodeSize(buffer+2,compressedData.size());
                            if(compressedData.size()>0)
                            {
                                memcpy(buffer+2+newSize,compressedData.constData(),compressedData.size());
                                return 2+newSize+compressedData.size();
                            }
                            else
                                return 2+newSize;
                        }
                        break;
                        case CompressionType::None:
                        break;
                    }
                }
            #endif
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                std::stringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return 0;
            }
            #endif
            const int &newSize=encodeSize(buffer+2,size);
            if(size>0)
            {
                memcpy(buffer+2+newSize,data,size);
                return 2+newSize+size;
            }
            else
                return 2+newSize;
        }
        else if(!sizeMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(compressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType));
                    #endif
                    switch(compressionTypeServer)
                    {
                        case CompressionType::Xz:
                        case CompressionType::Zlib:
                        default:
                        {
                            const QByteArray &compressedData(computeCompression(QByteArray(data,size),compressionTypeServer));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(compressedData.size()==0)
                            {
                                std::cerr <<
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            isClient <<
                                            #endif
                                std::stringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                                return 0;
                            }
                            #endif
                            const int &newSize=encodeSize(buffer+2,compressedData.size());
                            if(compressedData.size()>0)
                            {
                                memcpy(buffer+2+newSize,compressedData.constData(),compressedData.size());
                                return 2+newSize+compressedData.size();
                            }
                            else
                                return 2+newSize;
                        }
                        break;
                        case CompressionType::None:
                        break;
                    }
                }
            #endif
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                std::stringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return 0;
            }
            #endif
            const int &newSize=encodeSize(buffer+2,size);
            if(size>0)
            {
                memcpy(buffer+2+newSize,data,size);
                return 2+newSize+size;
            }
            else
                return 2+newSize;
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType));
            #endif
            if(size!=sizeMultipleCodePacketServerToClient.value(mainCodeType).value(subCodeType))
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                std::stringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
                return 0;
            }
            #endif
            if(size>0)
            {
                memcpy(buffer+2,data,size);
                return 2+size;
            }
            else
                return 2;
        }
    }
}

#ifdef CATCHCHALLENGER_EXTRA_CHECK
bool ProtocolParsingBase::removeFromQueryReceived(const uint8_t &queryNumber)
{
    return queryReceived.remove(queryNumber);
}
#endif

int ProtocolParsingBase::computeReplyData(
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    const bool &isClient,
    #endif
    char *dataBuffer, const uint8_t &queryNumber, const char * const data, const int &size,
    const int32_t &replyOutputSizeInt
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    , const CompressionType &compressionType
    #endif
    )
{

    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
        memcpy(dataBuffer,&replyCodeClientToServer,sizeof(uint8_t));
    else
    #endif
        memcpy(dataBuffer,&replyCodeServerToClient,sizeof(uint8_t));
    memcpy(dataBuffer+sizeof(uint8_t),&queryNumber,sizeof(uint8_t));

    if(replyOutputSizeInt==-1)
    {
        #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
        if(compressionType!=CompressionType::None)
        {
            #ifdef PROTOCOLPARSINGDEBUG
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
            std::stringLiteral(" postReplyData(%1) is now compressed").arg(queryNumber));
            #endif
            switch(compressionType)
            {
                case CompressionType::Xz:
                case CompressionType::Zlib:
                default:
                {
                    const QByteArray &compressedData(computeCompression(QByteArray(data,size),compressionType));
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(compressedData.size()==0)
                    {
                        std::cerr <<
                                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                    isClient <<
                                    #endif
                        std::stringLiteral(" postReplyData(%1,{}) dropped because can be size==0 if not fixed size").arg(queryNumber));
                        return 0;
                    }
                    #endif
                    const uint8_t &fullSize=sizeof(uint8_t)*2+encodeSize(dataBuffer+sizeof(uint8_t)*2,compressedData.size());
                    if(compressedData.size()>0)
                    {
                        memcpy(dataBuffer+fullSize,compressedData.constData(),compressedData.size());
                        return fullSize+compressedData.size();
                    }
                    else
                        return fullSize;
                }
                break;
            }
        }
        #endif
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(size==0)
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
            std::stringLiteral(" postReplyData(%1,{}) dropped because can be size==0 if not fixed size").arg(queryNumber));
            return 0;
        }
        #endif
        const uint8_t &fullSize=sizeof(uint8_t)*2+encodeSize(dataBuffer+sizeof(uint8_t)*2,size);
        if(size>0)
        {
            memcpy(dataBuffer+fullSize,data,size);
            return fullSize+size;
        }
        else
            return fullSize;
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
        if(compressionType!=CompressionType::None)
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
            std::stringLiteral(" postReplyData(%1,{}) compression disabled because have fixed size").arg(queryNumber));
            abort();
        }
        #endif
        if(size!=replyOutputSizeInt)
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
            std::stringLiteral(" postReplyData(%1,{}) dropped because can be size!=fixed size").arg(queryNumber));
            return 0;
        }
        #endif
        if(size>0)
        {
            memcpy(dataBuffer+sizeof(uint8_t)*2,data,size);
            return sizeof(uint8_t)*2+size;
        }
        else
            return sizeof(uint8_t)*2;
    }
}
