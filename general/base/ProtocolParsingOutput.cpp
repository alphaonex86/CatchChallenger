#include "ProtocolParsing.h"
#include "GeneralVariable.h"
#include "ProtocolParsingCheck.h"
#include <iostream>

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
#include <lzma.h>
#include "lz4/lz4.h"
#endif

using namespace CatchChallenger;

void ProtocolParsingBase::newOutputQuery(const uint8_t &packetCode,const uint8_t &queryNumber)
{
    if(outputQueryNumberToPacketCode.find(queryNumber)!=outputQueryNumberToPacketCode.cend())
    {
        errorParsingLayer("Query with this query number already found");
        return;
    }
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(flags & 0x10)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.find(packetCode)==mainCodeWithoutSubCodeTypeClientToServer.cend())
        {
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        std::to_string(flags & 0x10)+
                        #endif
                        " ProtocolParsingInputOutput::newOutputQuery(): queryNumber: "+std::to_string(queryNumber)+
                        ", packetCode: "+std::to_string(packetCode)+
                        ", try send without sub code, but not registred as is");
            return;
        }
        #endif
        if(replySizeOnlyMainCodePacketServerToClient.find(packetCode)!=replySizeOnlyMainCodePacketServerToClient.cend())
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionOnlyMainCodePacketServerToClient.find(packetCode)!=replyComressionOnlyMainCodePacketServerToClient.cend())
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            std::to_string(flags & 0x10)+
                            #endif
                            " ProtocolParsingInputOutput::newOutputQuery(): queryNumber: "+std::to_string(queryNumber)+
                            ", packetCode: "+std::to_string(packetCode)+
                            ", compression disabled because have fixed size");
            #endif
            #endif
        }
    }
    else
    #endif
    {
        if(replySizeOnlyMainCodePacketClientToServer.find(packetCode)!=replySizeOnlyMainCodePacketClientToServer.cend())
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionOnlyMainCodePacketClientToServer.find(packetCode)!=replyComressionOnlyMainCodePacketClientToServer.cend())
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            std::to_string(flags & 0x10)+
                            #endif
                            " ProtocolParsingInputOutput::newOutputQuery(): queryNumber: "+std::to_string(queryNumber)+
                            ", packetCode: "+std::to_string(packetCode)+
                            ", compression disabled because have fixed size");
            #endif
            #endif
        }
    }
    #ifdef ProtocolParsingInputOutputDEBUG
    messageParsingLayer(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                std::to_string(flags & 0x10)+
                #endif
    std::stringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, packetCode: %2").arg(queryNumber).arg(packetCode));
    #endif
    outputQueryNumberToPacketCode[queryNumber]=packetCode;
}

void ProtocolParsingBase::newFullOutputQuery(const uint8_t &packetCode,const uint8_t &subCodeType,const uint8_t &queryNumber)
{
    if(outputQueryNumberToPacketCode.find(queryNumber)!=outputQueryNumberToPacketCode.cend())
    {
        errorParsingLayer("Query with this query number already found");
        return;
    }
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(flags & 0x10)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.find(packetCode)!=mainCodeWithoutSubCodeTypeClientToServer.cend())
        {
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        std::to_string(flags & 0x10)+
                        #endif
                        " ProtocolParsingInputOutput::newOutputQuery(): queryNumber: "+std::to_string(queryNumber)+
                        ", packetCode: "+std::to_string(packetCode)+
                        ", subCodeType: "+std::to_string(subCodeType)+
                        ", try send with sub code, but not registred as is");
            return;
        }
        #endif
        if(replySizeMultipleCodePacketServerToClient.find(packetCode)!=replySizeMultipleCodePacketServerToClient.cend())
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionMultipleCodePacketServerToClient.find(packetCode)!=replyComressionMultipleCodePacketServerToClient.cend())
                if(replyComressionMultipleCodePacketServerToClient.at(packetCode).find(subCodeType)!=replyComressionMultipleCodePacketServerToClient.at(packetCode).cend())
                    messageParsingLayer(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                std::to_string(flags & 0x10)+
                                #endif
                                " ProtocolParsingInputOutput::newOutputQuery(): queryNumber: "+std::to_string(queryNumber)+
                                ", packetCode: "+std::to_string(packetCode)+
                                ", subCodeType: "+std::to_string(subCodeType)+
                                " compression disabled because have fixed size");
            #endif
            #endif
        }
    }
    else
    #endif
    {
        if(replySizeMultipleCodePacketServerToClient.find(packetCode)!=replySizeMultipleCodePacketServerToClient.cend())
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionMultipleCodePacketClientToServer.find(packetCode)!=replyComressionMultipleCodePacketClientToServer.cend())
                if(replyComressionMultipleCodePacketClientToServer.at(packetCode).find(subCodeType)!=replyComressionMultipleCodePacketClientToServer.at(packetCode).cend())
                    messageParsingLayer(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                std::to_string(flags & 0x10)+
                                #endif
                                " ProtocolParsingInputOutput::newOutputQuery(): queryNumber: "+std::to_string(queryNumber)+
                                ", packetCode: "+std::to_string(packetCode)+
                                ", subCodeType: "+std::to_string(subCodeType)+
                                " compression disabled because have fixed size");
            #endif
            #endif
        }
    }
    #ifdef ProtocolParsingInputOutputDEBUG
    messageParsingLayer(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                std::to_string(flags & 0x10)+
                #endif
    std::stringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, packetCode: %2, subCodeType: %3").arg(queryNumber).arg(packetCode).arg(subCodeType));
    #endif
    outputQueryNumberToPacketCode[queryNumber]=packetCode;
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
                        std::to_string(flags & 0x10) <<
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
                    std::to_string(flags & 0x10) <<
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

bool ProtocolParsingBase::packFullOutcommingData(const uint8_t &packetCode,const uint8_t &subCodeType,const char * const data,const int &size)
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
                packetCode,subCodeType,data,size);
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
                packetCode,subCodeType,data,size);
    if(newSize==0)
        return false;
    return internalPackOutcommingData(bigBufferForOutput.data(),newSize);
    #endif
}

bool ProtocolParsingBase::packOutcommingData(const uint8_t &packetCode,const char * const data,const int &size)
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
            packetCode,data,size);
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
            packetCode,data,size);
    if(newSize==0)
        return false;
    return internalPackOutcommingData(bigBufferForOutput.data(),newSize);
    #endif
}

bool ProtocolParsingBase::packOutcommingQuery(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const int &size)
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
            packetCode,queryNumber,data,size);
    if(newSize==0)
        return false;
    newOutputQuery(packetCode,queryNumber);
    return internalPackOutcommingData(ProtocolParsingBase::tempBigBufferForOutput,newSize);
    #else
    QByteArray bigBufferForOutput;
    bigBufferForOutput.resize(16+size);
    const int &newSize=computeOutcommingQuery(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            isClient,
            #endif
            bigBufferForOutput.data(),
            packetCode,queryNumber,data,size);
    if(newSize==0)
        return false;
    newOutputQuery(packetCode,queryNumber);
    return internalPackOutcommingData(bigBufferForOutput.data(),newSize);
    #endif
}

bool ProtocolParsingBase::packFullOutcommingQuery(const uint8_t &packetCode,const uint8_t &subCodeType,const uint8_t &queryNumber,const char * const data,const int &size)
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
            packetCode,subCodeType,queryNumber,data,size);
    if(newSize==0)
        return false;
    newFullOutputQuery(packetCode,subCodeType,queryNumber);
    return internalPackOutcommingData(ProtocolParsingBase::tempBigBufferForOutput,newSize);
    #else
    QByteArray bigBufferForOutput;
    bigBufferForOutput.resize(16+size+(200+size*2/*overhead of empty data char* compression*/));
    const int &newSize=computeFullOutcommingQuery(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            isClient,
            #endif
            bigBufferForOutput.data(),
            packetCode,subCodeType,queryNumber,data,size);
    if(newSize==0)
        return false;
    newFullOutputQuery(packetCode,subCodeType,queryNumber);
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
    if(size<0xFF)
    {
        data[0x00]=size;
        return 1;//sizeof(uint8_t)
    }
    else if(size<0xFFFF)
    {
        data[0x00]=0xFF;
        *reinterpret_cast<quint16 *>(data+0x01)=htole16(size);
        return 3;//sizeof(uint8_t)+sizeof(uint16_t)
    }
    else
    {
        data[0x00]=0xFF;
        data[0x01]=0xFF;
        *reinterpret_cast<quint32 *>(data+0x02)=htole32(size);
        return 6;//sizeof(uint16_t)+sizeof(uint32_t)
    }
}

int ProtocolParsingBase::computeOutcommingData(
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        const bool &isClient,
        #endif
        char *buffer,
        const uint8_t &packetCode,const char * const data,const int &size)
{
    buffer[0]=packetCode;

    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(flags & 0x10)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.find(packetCode)==mainCodeWithoutSubCodeTypeClientToServer.cend())
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
                        " ProtocolParsingInputOutput::packOutcommingData(): packetCode: "
                        << packetCode
                        << ", try send without sub code, but not registred as is"
                        << std::endl;
            return 0;
        }
        if(mainCode_IsQueryClientToServer.find(packetCode)!=mainCode_IsQueryClientToServer.cend())
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
                        " ProtocolParsingInputOutput::packOutcommingQuery(): packetCode: "
                        << packetCode
                        << ", try send as normal data, but not registred as is" << std::endl;
            return 0;
        }
        #endif
        if(sizeOnlyMainCodePacketClientToServer.find(packetCode)==sizeOnlyMainCodePacketClientToServer.cend())
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                            " packOutcommingData("
                            << packetCode
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
            if(size!=sizeOnlyMainCodePacketClientToServer.at(packetCode))
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                            " packOutcommingData("
                            << packetCode
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
        if(mainCode_IsQueryServerToClient.find(packetCode)!=mainCode_IsQueryServerToClient.cend())
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
                        " ProtocolParsingInputOutput::packOutcommingQuery(): packetCode: "
                        << packetCode
                        << ", try send as normal data, but not registred as is"
                        << std::endl;
            return 0;
        }
        #endif
        if(sizeOnlyMainCodePacketServerToClient.find(packetCode)==sizeOnlyMainCodePacketServerToClient.cend())
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                            " packOutcommingData("
                            << packetCode
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
            if(size!=sizeOnlyMainCodePacketServerToClient.at(packetCode))
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                            " packOutcommingData("
                            << packetCode
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
        const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const int &size)
{
    buffer[0]=packetCode;
    buffer[1]=queryNumber;

    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(flags & 0x10)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.find(packetCode)==mainCodeWithoutSubCodeTypeClientToServer.cend())
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
                        " ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: "
                        << queryNumber
                        << ", packetCode: "
                        << packetCode
                        << ", try send without sub code, but not registred as is"
                        << std::endl;
            return 0;
        }
        if(mainCode_IsQueryClientToServer.find(packetCode)==mainCode_IsQueryClientToServer.cend())
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
                        " ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: "
                        << queryNumber
                        << ", packetCode: "
                        << packetCode
                        << ", try send as query, but not registred as is"
                        << std::endl;
            return 0;
        }
        #endif
        if(sizeOnlyMainCodePacketClientToServer.find(packetCode)==sizeOnlyMainCodePacketClientToServer.cend())
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                            " packOutcommingQuery("
                            << packetCode
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
            if(size!=sizeOnlyMainCodePacketClientToServer.at(packetCode))
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                            " packOutcommingQuery("
                            << packetCode
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
        if(mainCode_IsQueryServerToClient.find(packetCode)==mainCode_IsQueryServerToClient.cend())
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
                        " ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: "
                        << queryNumber
                        << ", packetCode: "
                        << packetCode
                        << ", try send as query, but not registred as is"
                        << std::endl;
            return 0;
        }
        #endif
        if(sizeOnlyMainCodePacketServerToClient.find(packetCode)==sizeOnlyMainCodePacketServerToClient.cend())
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                            " packOutcommingQuery("
                            << packetCode
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
            if(size!=sizeOnlyMainCodePacketServerToClient.at(packetCode))
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                            " packOutcommingQuery("
                            << packetCode
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
        const uint8_t &packetCode,const uint8_t &subCodeType,const uint8_t &queryNumber,const char * const data,const int &size)
{
    buffer[0]=packetCode;
    buffer[1]=subCodeType;
    buffer[2]=queryNumber;

    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(flags & 0x10)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.find(packetCode)!=mainCodeWithoutSubCodeTypeClientToServer.cend())
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
                        " ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: "
                        << queryNumber
                        << ", packetCode: "
                        << packetCode
                        << ", subCodeType: "
                        << subCodeType
                        << ", try send with sub code, but not registred as is"
                        << std::endl;
            return 0;
        }
        if(mainCode_IsQueryClientToServer.find(packetCode)==mainCode_IsQueryClientToServer.cend())
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
                        " ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: "
                        << queryNumber
                        << ", packetCode: "
                        << packetCode
                        << ", subCodeType: "
                        << subCodeType
                        << ", try send as query, but not registred as is"
                        << std::endl;
            return 0;
        }
        #endif
        if(sizeMultipleCodePacketClientToServer.find(packetCode)==sizeMultipleCodePacketClientToServer.cend())
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(compressionMultipleCodePacketClientToServer.find(packetCode)!=compressionMultipleCodePacketClientToServer.cend())
                if(compressionMultipleCodePacketClientToServer.at(packetCode).find(subCodeType)!=compressionMultipleCodePacketClientToServer.at(packetCode).cend())
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(packetCode).arg(subCodeType).arg(queryNumber));
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
                                            " packOutcommingQuery("
                                            << packetCode
                                            << ","
                                            << subCodeType
                                            << ",{}) dropped because can be size==0 if not fixed size"
                                            << std::endl;
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
                            " packOutcommingQuery("
                            << packetCode
                            << ","
                            << subCodeType
                            << ",{}) dropped because can be size==0 if not fixed size"
                            << std::endl;
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
        else if(sizeMultipleCodePacketClientToServer.at(packetCode).find(subCodeType)==sizeMultipleCodePacketClientToServer.at(packetCode).cend())
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(compressionMultipleCodePacketClientToServer.find(packetCode)!=compressionMultipleCodePacketClientToServer.cend())
                if(compressionMultipleCodePacketClientToServer.at(packetCode).find(subCodeType)!=compressionMultipleCodePacketClientToServer.at(packetCode).cend())
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(packetCode).arg(subCodeType).arg(queryNumber));
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
                                            " packOutcommingQuery("
                                            << packetCode
                                            << ","
                                            << subCodeType
                                            << ",{}) dropped because can be size==0 if not fixed size"
                                            << std::endl;
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
                            " packOutcommingQuery("
                            << packetCode
                            << ","
                            << subCodeType
                            << ",{}) dropped because can be size==0 if not fixed size"
                            << std::endl;
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
            if(compressionMultipleCodePacketClientToServer.contains(packetCode))
                if(compressionMultipleCodePacketClientToServer.value(packetCode).contains(subCodeType))
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingQuery(%1,%2,%3) compression can't be enabled due to fixed size").arg(packetCode).arg(subCodeType).arg(queryNumber));
            #endif
            if(size!=sizeMultipleCodePacketClientToServer.value(packetCode).value(subCodeType))
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                std::stringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size!=fixed size").arg(packetCode).arg(subCodeType));
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
        if(!mainCode_IsQueryServerToClient.contains(packetCode))
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
            std::stringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, packetCode: %2, subCodeType: %3, try send as query, but not registred as is").arg(queryNumber).arg(packetCode).arg(subCodeType));
            return 0;
        }
        #endif
        if(!sizeMultipleCodePacketServerToClient.contains(packetCode))
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(compressionMultipleCodePacketServerToClient.contains(packetCode))
                if(compressionMultipleCodePacketServerToClient.value(packetCode).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(packetCode).arg(subCodeType).arg(queryNumber));
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
                                std::stringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(packetCode).arg(subCodeType));
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
                std::stringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(packetCode).arg(subCodeType));
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
        else if(!sizeMultipleCodePacketServerToClient.value(packetCode).contains(subCodeType))
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(compressionMultipleCodePacketServerToClient.contains(packetCode))
                if(compressionMultipleCodePacketServerToClient.value(packetCode).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(packetCode).arg(subCodeType).arg(queryNumber));
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
                                std::stringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(packetCode).arg(subCodeType));
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
                std::stringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(packetCode).arg(subCodeType));
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
            if(compressionMultipleCodePacketServerToClient.contains(packetCode))
                if(compressionMultipleCodePacketServerToClient.value(packetCode).contains(subCodeType))
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingQuery(%1,%2,%3) compression can't be enabled due to fixed size").arg(packetCode).arg(subCodeType).arg(queryNumber));
            #endif
            if(size!=sizeMultipleCodePacketServerToClient.value(packetCode).value(subCodeType))
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                std::stringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size!=fixed size").arg(packetCode).arg(subCodeType));
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
        const uint8_t &packetCode,const uint8_t &subCodeType,const char * const data,const int &size)
{
    buffer[0]=packetCode;
    buffer[1]=subCodeType;

    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(flags & 0x10)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.contains(packetCode))
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
            std::stringLiteral(" ProtocolParsingInputOutput::packOutcommingData(): packetCode: %1, subCodeType: %2, try send with sub code, but not registred as is").arg(packetCode).arg(subCodeType));
            return 0;
        }
        if(mainCode_IsQueryClientToServer.contains(packetCode))
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
            std::stringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): packetCode: %1, subCodeType: %2, try send as normal data, but not registred as is").arg(packetCode).arg(subCodeType));
            return 0;
        }
        #endif
        if(!sizeMultipleCodePacketClientToServer.contains(packetCode))
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(compressionMultipleCodePacketClientToServer.contains(packetCode))
                if(compressionMultipleCodePacketClientToServer.value(packetCode).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingData(%1,%2) compression enabled").arg(packetCode).arg(subCodeType));
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
                                std::stringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(packetCode).arg(subCodeType));
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
                std::stringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(packetCode).arg(subCodeType));
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
        else if(!sizeMultipleCodePacketClientToServer.value(packetCode).contains(subCodeType))
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(compressionMultipleCodePacketClientToServer.contains(packetCode))
                if(compressionMultipleCodePacketClientToServer.value(packetCode).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingData(%1,%2) compression enabled").arg(packetCode).arg(subCodeType));
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
                                std::stringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(packetCode).arg(subCodeType));
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
                std::stringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(packetCode).arg(subCodeType));
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
            if(compressionMultipleCodePacketClientToServer.contains(packetCode))
                if(compressionMultipleCodePacketClientToServer.value(packetCode).contains(subCodeType))
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(packetCode).arg(subCodeType));
            #endif
            if(size!=sizeMultipleCodePacketClientToServer.value(packetCode).value(subCodeType))
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                std::stringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size!=fixed size").arg(packetCode).arg(subCodeType));
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
        if(mainCode_IsQueryServerToClient.contains(packetCode))
        {
            std::cerr <<
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        isClient <<
                        #endif
            std::stringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): packetCode: %1, subCodeType: %2, try send as normal data, but not registred as is").arg(packetCode).arg(subCodeType));
            return 0;
        }
        #endif
        if(!sizeMultipleCodePacketServerToClient.contains(packetCode))
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(compressionMultipleCodePacketServerToClient.contains(packetCode))
                if(compressionMultipleCodePacketServerToClient.value(packetCode).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(packetCode).arg(subCodeType));
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
                                std::stringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(packetCode).arg(subCodeType));
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
                std::stringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(packetCode).arg(subCodeType));
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
        else if(!sizeMultipleCodePacketServerToClient.value(packetCode).contains(subCodeType))
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(compressionMultipleCodePacketServerToClient.contains(packetCode))
                if(compressionMultipleCodePacketServerToClient.value(packetCode).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(packetCode).arg(subCodeType));
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
                                std::stringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(packetCode).arg(subCodeType));
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
                std::stringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(packetCode).arg(subCodeType));
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
            if(compressionMultipleCodePacketClientToServer.contains(packetCode))
                if(compressionMultipleCodePacketClientToServer.value(packetCode).contains(subCodeType))
                    std::cerr <<
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                isClient <<
                                #endif
                    std::stringLiteral(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(packetCode).arg(subCodeType));
            #endif
            if(size!=sizeMultipleCodePacketServerToClient.value(packetCode).value(subCodeType))
            {
                std::cerr <<
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            isClient <<
                            #endif
                std::stringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size!=fixed size").arg(packetCode).arg(subCodeType));
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
    if(flags & 0x10)
        dataBuffer[0x00]=replyCodeClientToServer;
    else
    #endif
        dataBuffer[0x00]=replyCodeServerToClient;
    dataBuffer[0x01]=queryNumber;

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
