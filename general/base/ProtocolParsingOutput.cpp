#include "ProtocolParsing.h"
#include "DebugClass.h"
#include "GeneralVariable.h"
#include "ProtocolParsingCheck.h"

using namespace CatchChallenger;

void ProtocolParsingBase::newOutputQuery(const quint8 &mainCodeType,const quint8 &queryNumber)
{
    if(waitedReply_mainCodeType.contains(queryNumber))
    {
        errorParsingLayer("Query with this query number already found");
        return;
    }
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, try send without sub code, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return;
        }
        #endif
        if(replySizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionOnlyMainCodePacketServerToClient.contains(mainCodeType))
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, compression disabled because have fixed size").arg(queryNumber).arg(mainCodeType));
            #endif
            #endif
        }
    }
    else
    #endif
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, try send without sub code, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return;
        }
        #endif
        if(replySizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionOnlyMainCodePacketClientToServer.contains(mainCodeType))
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, compression disabled because have fixed size").arg(queryNumber).arg(mainCodeType));
            #endif
            #endif
        }
    }
    #ifdef ProtocolParsingInputOutputDEBUG
    messageParsingLayer(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                QString::number(isClient)+
                #endif
    QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2").arg(queryNumber).arg(mainCodeType));
    #endif
    waitedReply_mainCodeType[queryNumber]=mainCodeType;
}

void ProtocolParsingBase::newFullOutputQuery(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber)
{
    if(waitedReply_mainCodeType.contains(queryNumber))
    {
        errorParsingLayer("Query with this query number already found");
        return;
    }
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return;
        }
        #endif
        if(replySizeMultipleCodePacketServerToClient.contains(mainCodeType))
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(replyComressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                    messageParsingLayer(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3 compression disabled because have fixed size").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            #endif
            #endif
        }
    }
    else
    #endif
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return;
        }
        #endif
        if(replySizeMultipleCodePacketServerToClient.contains(mainCodeType))
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(replyComressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                    messageParsingLayer(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3 compression disabled because have fixed size").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            #endif
            #endif
        }
    }
    #ifdef ProtocolParsingInputOutputDEBUG
    messageParsingLayer(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                QString::number(isClient)+
                #endif
    QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
    #endif
    waitedReply_mainCodeType[queryNumber]=mainCodeType;
    waitedReply_subCodeType[queryNumber]=subCodeType;
}

bool ProtocolParsingBase::postReplyData(const quint8 &queryNumber, const char * const data,const int &size)
{
    int replyOutputSizeInt=-1;
    if(replyOutputSize.contains(queryNumber))
    {
        replyOutputSizeInt=replyOutputSize.value(queryNumber);
        replyOutputSize.remove(queryNumber);
    }
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    CompressionType compressionType=CompressionType::None;
    if(replyOutputSizeInt==-1)
    {
        if(replyOutputCompression.contains(queryNumber))
        {
            compressionType=getCompressType();
            replyOutputCompression.remove(queryNumber);
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(replyOutputCompression.contains(queryNumber))
        {
            qDebug() << (
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" postReplyData(%1,{}) compression disabled because have fixed size").arg(queryNumber));
            abort();
        }
        #endif
    }
    #endif

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(!queryReceived.contains(queryNumber))
    {
        qDebug() << (
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    QString::number(isClient)+
                    #endif
        QStringLiteral(" ProtocolParsingInputOutput::postReplyData(): try reply to queryNumber: %1, but this query is not into the list").arg(queryNumber));
        return 0;
    }
    else
        queryReceived.remove(queryNumber);
    #endif

    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size>(CATCHCHALLENGER_BIGBUFFERSIZE-16))
    {
        errorParsingLayer(QString("Buffer in input is too big and will do buffer overflow, size: %1, line %2").arg(size).arg(__LINE__));
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
        case CompressionType::Xz:
            return lzmaCompress(data);
        break;
        case CompressionType::Zlib:
        default:
            return qCompress(data,9);
        break;
        case CompressionType::None:
            return data;
        break;
    }
}
#endif

bool ProtocolParsingBase::packFullOutcommingData(const quint8 &mainCodeType,const quint8 &subCodeType,const char * const data,const int &size)
{
    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size>(CATCHCHALLENGER_BIGBUFFERSIZE-16))
    {
        errorParsingLayer(QString("Buffer in input is too big and will do buffer overflow, size: %1, line %2").arg(size).arg(__LINE__));
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

bool ProtocolParsingBase::packOutcommingData(const quint8 &mainCodeType,const char * const data,const int &size)
{
    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size>(CATCHCHALLENGER_BIGBUFFERSIZE-16))
    {
        errorParsingLayer(QString("Buffer in input is too big and will do buffer overflow, size: %1, line %2").arg(size).arg(__LINE__));
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

bool ProtocolParsingBase::packOutcommingQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const int &size)
{
    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size>(CATCHCHALLENGER_BIGBUFFERSIZE-16))
    {
        errorParsingLayer(QString("Buffer in input is too big and will do buffer overflow, size: %1, line %2").arg(size).arg(__LINE__));
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

bool ProtocolParsingBase::packFullOutcommingQuery(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char * const data,const int &size)
{
    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size>(CATCHCHALLENGER_BIGBUFFERSIZE-16))
    {
        errorParsingLayer(QString("Buffer in input is too big and will do buffer overflow, size: %1, line %2").arg(size).arg(__LINE__));
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
        qDebug() << QString("ProtocolParsingInputOutput::internalPackOutcommingData size is null").arg(__LINE__);
        return false;
    }
    #endif
    #ifdef PROTOCOLPARSINGDEBUG
    messageParsingLayer("internalPackOutcommingData(): start");
    #endif
    #ifdef DEBUG_PROTOCOLPARSING_RAW_NETWORK
    qDebug() << QString(QStringLiteral("Sended packet size: %1: %2").arg(size).arg(QString(QByteArray(data,size).toHex())));
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
        qDebug() << QString("ProtocolParsingInputOutput::internalSendRawSmallPacket size is null").arg(__LINE__);
        return false;
    }
    #endif
    #ifdef PROTOCOLPARSINGDEBUG
    messageParsingLayer("internalPackOutcommingData(): start");
    #endif
    #ifdef DEBUG_PROTOCOLPARSING_RAW_NETWORK
    messageParsingLayer(QStringLiteral("Sended packet size: %1: %2").arg(size).arg(QString(QByteArray(data,size).toHex())));
    #endif // DEBUG_PROTOCOLPARSING_RAW_NETWORK
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size>CATCHCHALLENGER_MAX_PACKET_SIZE)
    {
        messageParsingLayer(QStringLiteral("ProtocolParsingInputOutput::sendRawSmallPacket(): Packet to big: %1").arg(size));
        errorParsingLayer(QStringLiteral("ProtocolParsingInputOutput::sendRawSmallPacket(): Packet to big: %1").arg(size));
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

qint8 ProtocolParsingBase::encodeSize(char *data,const quint32 &size)
{
    if(size<=0xFF)
    {
        memcpy(data,&size,sizeof(quint8));
        return sizeof(quint8);
    }
    else if(size<=0xFFFF)
    {
        const quint16 &newSize=htole16(size);
        memcpy(data,&ProtocolParsingBase::sizeHeaderNullquint16,sizeof(quint8));
        memcpy(data+sizeof(quint8),&newSize,sizeof(newSize));
        return sizeof(quint8)+sizeof(quint16);
    }
    else
    {
        const quint32 &newSize=htole32(size);
        memcpy(data,&ProtocolParsingBase::sizeHeaderNullquint16,sizeof(quint16));
        memcpy(data+sizeof(quint16),&newSize,sizeof(newSize));
        return sizeof(quint16)+sizeof(quint32);
    }
}

int ProtocolParsingBase::computeOutcommingData(
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        const bool &isClient,
        #endif
        char *buffer,
        const quint8 &mainCodeType,const char * const data,const int &size)
{
    buffer[0]=mainCodeType;

    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            qDebug() << (
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingData(): mainCodeType: %1, try send without sub code, but not registred as is").arg(mainCodeType));
            return 0;
        }
        if(mainCode_IsQueryClientToServer.contains(mainCodeType))
        {
            qDebug() << (
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): mainCodeType: %1, try send as normal data, but not registred as is").arg(mainCodeType));
            return 0;
        }
        #endif
        if(!sizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                qDebug() << (
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType));
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
            if(size!=sizeOnlyMainCodePacketClientToServer.value(mainCodeType))
            {
                qDebug() << (
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,{}) dropped because can be size!=fixed size").arg(mainCodeType));
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
        if(!mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            qDebug() << (
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingData(): mainCodeType: %1, try send without sub code, but not registred as is").arg(mainCodeType));
            return 0;
        }
        if(mainCode_IsQueryServerToClient.contains(mainCodeType))
        {
            qDebug() << (
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): mainCodeType: %1, try send as normal data, but not registred as is").arg(mainCodeType));
            return 0;
        }
        #endif
        if(!sizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                qDebug() << (
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType));
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
            if(size!=sizeOnlyMainCodePacketServerToClient.value(mainCodeType))
            {
                qDebug() << (
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,{}) dropped because can be size!=fixed size").arg(mainCodeType));
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
        const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const int &size)
{
    buffer[0]=mainCodeType;
    buffer[1]=queryNumber;

    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            qDebug() << (
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, try send without sub code, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return 0;
        }
        if(!mainCode_IsQueryClientToServer.contains(mainCodeType))
        {
            qDebug() << (
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, try send as query, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return 0;
        }
        #endif
        if(!sizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                qDebug() << (
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType));
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
            if(size!=sizeOnlyMainCodePacketClientToServer.value(mainCodeType))
            {
                qDebug() << (
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,{}) dropped because can be size!=fixed size").arg(mainCodeType));
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
        if(!mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            qDebug() << (
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, try send without sub code, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return 0;
        }
        if(!mainCode_IsQueryServerToClient.contains(mainCodeType))
        {
            qDebug() << (
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, try send as query, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return 0;
        }
        #endif
        if(!sizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                qDebug() << (
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType));
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
            if(size!=sizeOnlyMainCodePacketServerToClient.value(mainCodeType))
            {
                qDebug() << (
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,{}) dropped because can be size!=fixed size").arg(mainCodeType));
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
        const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char * const data,const int &size)
{
    buffer[0]=mainCodeType;
    buffer[1]=subCodeType;
    buffer[2]=queryNumber;

    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            qDebug() << (
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return 0;
        }
        if(!mainCode_IsQueryClientToServer.contains(mainCodeType))
        {
            qDebug() << (
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send as query, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
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
                    qDebug() << (
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
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
                                qDebug() << (
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                qDebug() << (
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                    qDebug() << (
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
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
                                qDebug() << (
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                qDebug() << (
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                    qDebug() << (
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingQuery(%1,%2,%3) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            #endif
            if(size!=sizeMultipleCodePacketClientToServer.value(mainCodeType).value(subCodeType))
            {
                qDebug() << (
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
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
            qDebug() << (
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return 0;
        }
        if(!mainCode_IsQueryServerToClient.contains(mainCodeType))
        {
            qDebug() << (
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send as query, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
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
                    qDebug() << (
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
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
                                qDebug() << (
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                qDebug() << (
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                    qDebug() << (
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
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
                                qDebug() << (
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                qDebug() << (
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                    qDebug() << (
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingQuery(%1,%2,%3) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            #endif
            if(size!=sizeMultipleCodePacketServerToClient.value(mainCodeType).value(subCodeType))
            {
                qDebug() << (
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
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
        const quint8 &mainCodeType,const quint8 &subCodeType,const char * const data,const int &size)
{
    buffer[0]=mainCodeType;
    buffer[1]=subCodeType;

    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            qDebug() << (
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingData(): mainCodeType: %1, subCodeType: %2, try send with sub code, but not registred as is").arg(mainCodeType).arg(subCodeType));
            return 0;
        }
        if(mainCode_IsQueryClientToServer.contains(mainCodeType))
        {
            qDebug() << (
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): mainCodeType: %1, subCodeType: %2, try send as normal data, but not registred as is").arg(mainCodeType).arg(subCodeType));
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
                    qDebug() << (
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingData(%1,%2) compression enabled").arg(mainCodeType).arg(subCodeType));
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
                                qDebug() << (
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                qDebug() << (
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                    qDebug() << (
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingData(%1,%2) compression enabled").arg(mainCodeType).arg(subCodeType));
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
                                qDebug() << (
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                qDebug() << (
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                    qDebug() << (
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType));
            #endif
            if(size!=sizeMultipleCodePacketClientToServer.value(mainCodeType).value(subCodeType))
            {
                qDebug() << (
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
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
            qDebug() << (
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingData(): mainCodeType: %1, subCodeType: %2, try send with sub code, but not registred as is").arg(mainCodeType).arg(subCodeType));
            return 0;
        }
        if(mainCode_IsQueryServerToClient.contains(mainCodeType))
        {
            qDebug() << (
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): mainCodeType: %1, subCodeType: %2, try send as normal data, but not registred as is").arg(mainCodeType).arg(subCodeType));
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
                    qDebug() << (
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType));
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
                                qDebug() << (
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                qDebug() << (
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                    qDebug() << (
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType));
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
                                qDebug() << (
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                qDebug() << (
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                    qDebug() << (
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType));
            #endif
            if(size!=sizeMultipleCodePacketServerToClient.value(mainCodeType).value(subCodeType))
            {
                qDebug() << (
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
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
bool ProtocolParsingBase::removeFromQueryReceived(const quint8 &queryNumber)
{
    return queryReceived.remove(queryNumber);
}
#endif

int ProtocolParsingBase::computeReplyData(
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    const bool &isClient,
    #endif
    char *dataBuffer, const quint8 &queryNumber, const char * const data, const int &size,
    const qint32 &replyOutputSizeInt
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    , const CompressionType &compressionType
    #endif
    )
{

    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
        memcpy(dataBuffer,&replyCodeClientToServer,sizeof(quint8));
    else
    #endif
        memcpy(dataBuffer,&replyCodeServerToClient,sizeof(quint8));
    memcpy(dataBuffer+sizeof(quint8),&queryNumber,sizeof(quint8));

    if(replyOutputSizeInt==-1)
    {
        #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
        if(compressionType!=CompressionType::None)
        {
            #ifdef PROTOCOLPARSINGDEBUG
            qDebug() << (
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" postReplyData(%1) is now compressed").arg(queryNumber));
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
                        qDebug() << (
                                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                    QString::number(isClient)+
                                    #endif
                        QStringLiteral(" postReplyData(%1,{}) dropped because can be size==0 if not fixed size").arg(queryNumber));
                        return 0;
                    }
                    #endif
                    const quint8 &fullSize=sizeof(quint8)*2+encodeSize(dataBuffer+sizeof(quint8)*2,compressedData.size());
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
            qDebug() << (
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" postReplyData(%1,{}) dropped because can be size==0 if not fixed size").arg(queryNumber));
            return 0;
        }
        #endif
        const quint8 &fullSize=sizeof(quint8)*2+encodeSize(dataBuffer+sizeof(quint8)*2,size);
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
            qDebug() << (
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" postReplyData(%1,{}) compression disabled because have fixed size").arg(queryNumber));
            abort();
        }
        #endif
        if(size!=replyOutputSizeInt)
        {
            qDebug() << (
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" postReplyData(%1,{}) dropped because can be size!=fixed size").arg(queryNumber));
            return 0;
        }
        #endif
        if(size>0)
        {
            memcpy(dataBuffer+sizeof(quint8)*2,data,size);
            return sizeof(quint8)*2+size;
        }
        else
            return sizeof(quint8)*2;
    }
}
