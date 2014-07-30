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
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, try send without sub code, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return;
        }
        #endif
        if(replySizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionOnlyMainCodePacketServerToClient.contains(mainCodeType))
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, compression disabled because have fixed size").arg(queryNumber).arg(mainCodeType));
            #endif
        }
    }
    else
    #endif
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, try send without sub code, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return;
        }
        #endif
        if(replySizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionOnlyMainCodePacketClientToServer.contains(mainCodeType))
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, compression disabled because have fixed size").arg(queryNumber).arg(mainCodeType));
            #endif
        }
    }
    #ifdef ProtocolParsingInputOutputDEBUG
    DebugClass::debugConsole(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                QString::number(isClient)+
                #endif
    QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2").arg(queryNumber).arg(mainCodeType));
    #endif
    waitedReply_mainCodeType[queryNumber]=mainCodeType;
}

void ProtocolParsingBase::newFullOutputQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber)
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
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return;
        }
        #endif
        if(replySizeMultipleCodePacketServerToClient.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(replyComressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                    DebugClass::debugConsole(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3 compression disabled because have fixed size").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            #endif
        }
    }
    else
    #endif
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return;
        }
        #endif
        if(replySizeMultipleCodePacketServerToClient.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(replyComressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                    DebugClass::debugConsole(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3 compression disabled because have fixed size").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            #endif
        }
    }
    #ifdef ProtocolParsingInputOutputDEBUG
    DebugClass::debugConsole(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                QString::number(isClient)+
                #endif
    QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
    #endif
    waitedReply_mainCodeType[queryNumber]=mainCodeType;
    waitedReply_subCodeType[queryNumber]=subCodeType;
}

bool ProtocolParsingBase::postReplyData(const quint8 &queryNumber, const char *data,const int &size)
{
    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size>(CATCHCHALLENGER_BIGBUFFERSIZE-16))
    {
        errorParsingLayer(QString("Buffer in input is too big and will do buffer overflow, line %1").arg(__LINE__));
        return false;
    }
    #endif

    const int &newSize=ProtocolParsingBase::computeReplyData(ProtocolParsingBase::tempBigBufferForOutput,queryNumber,data,size);
    if(newSize==0)
        return false;
    return internalPackOutcommingData(ProtocolParsingBase::tempBigBufferForOutput,newSize);
    #else
    QByteArray bigBufferForOutput;
    bigBufferForOutput.resize(16+size);
    const int &newSize=ProtocolParsingBase::computeReplyData(bigBufferForOutput.data(),queryNumber,data,size);
    if(newSize==0)
        return false;
    return internalPackOutcommingData(bigBufferForOutput.data(),newSize);
    #endif
}

QByteArray ProtocolParsingBase::computeCompression(const QByteArray &data)
{
    switch(compressionType)
    {
        case CompressionType_Xz:
            return lzmaCompress(data);
        break;
        case CompressionType_Zlib:
        default:
            return qCompress(data,9);
        break;
        case CompressionType_None:
            return data;
        break;
    }
}

bool ProtocolParsingBase::packFullOutcommingData(const quint8 &mainCodeType,const quint16 &subCodeType,const char *data,const int &size)
{
    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size>(CATCHCHALLENGER_BIGBUFFERSIZE-16))
    {
        errorParsingLayer(QString("Buffer in input is too big and will do buffer overflow, line %1").arg(__LINE__));
        return false;
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

bool ProtocolParsingBase::packOutcommingData(const quint8 &mainCodeType,const char *data,const int &size)
{
    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size>(CATCHCHALLENGER_BIGBUFFERSIZE-16))
    {
        errorParsingLayer(QString("Buffer in input is too big and will do buffer overflow, line %1").arg(__LINE__));
        return false;
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

bool ProtocolParsingBase::packOutcommingQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size>(CATCHCHALLENGER_BIGBUFFERSIZE-16))
    {
        errorParsingLayer(QString("Buffer in input is too big and will do buffer overflow, line %1").arg(__LINE__));
        return false;
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

bool ProtocolParsingBase::packFullOutcommingQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size>(CATCHCHALLENGER_BIGBUFFERSIZE-16))
    {
        errorParsingLayer(QString("Buffer in input is too big and will do buffer overflow, line %1").arg(__LINE__));
        return false;
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
    bigBufferForOutput.resize(16+size);
    const int &newSize=computeFullOutcommingQuery(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            isClient,
            #endif
            bigBufferForOutput.data(),
            mainCodeType,subCodeType,queryNumber,data,size);
    if(newSize==0)
        return false;
    newFullOutputQuery(mainCodeType,subCodeType,queryNumber);
    return internalPackOutcommingData(bigBufferForOutput.data(),newSize);
    #endif
}

bool ProtocolParsingBase::internalPackOutcommingData(const char *data,const int &size)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size<=0)
    {
        qDebug() << QString("ProtocolParsingInputOutput::internalPackOutcommingData size is null").arg(__LINE__);
        return false;
    }
    #endif
    #ifdef PROTOCOLPARSINGDEBUG
    DebugClass::debugConsole("internalPackOutcommingData(): start");
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
bool ProtocolParsingBase::internalSendRawSmallPacket(const char *data,const int &size)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size<=0)
    {
        qDebug() << QString("ProtocolParsingInputOutput::internalSendRawSmallPacket size is null").arg(__LINE__);
        return false;
    }
    #endif
    #ifdef PROTOCOLPARSINGDEBUG
    DebugClass::debugConsole("internalPackOutcommingData(): start");
    #endif
    #ifdef DEBUG_PROTOCOLPARSING_RAW_NETWORK
    DebugClass::debugConsole(QStringLiteral("Sended packet size: %1: %2").arg(size).arg(QString(QByteArray(data,size).toHex())));
    #endif // DEBUG_PROTOCOLPARSING_RAW_NETWORK
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size>CATCHCHALLENGER_MAX_PACKET_SIZE)
    {
        DebugClass::debugConsole(QStringLiteral("ProtocolParsingInputOutput::sendRawSmallPacket(): Packet to big: %1").arg(size));
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
        const quint16 &newSize=htobe16(size);
        memcpy(data,&ProtocolParsingBase::sizeHeaderNullquint16,sizeof(quint8));
        memcpy(data+sizeof(quint8),&newSize,sizeof(newSize));
        return sizeof(quint8)+sizeof(quint16);
    }
    else
    {
        const quint32 &newSize=htobe32(size);
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
        const quint8 &mainCodeType,const char *data,const int &size)
{
    buffer[0]=mainCodeType;

    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingData(): mainCodeType: %1, try send without sub code, but not registred as is").arg(mainCodeType));
            return 0;
        }
        if(mainCode_IsQueryClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(
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
                DebugClass::debugConsole(
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
                DebugClass::debugConsole(
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
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingData(): mainCodeType: %1, try send without sub code, but not registred as is").arg(mainCodeType));
            return 0;
        }
        if(mainCode_IsQueryServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(
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
                DebugClass::debugConsole(
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
                DebugClass::debugConsole(
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
        const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    buffer[0]=mainCodeType;
    buffer[1]=queryNumber;

    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, try send without sub code, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return 0;
        }
        if(!mainCode_IsQueryClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(
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
                DebugClass::debugConsole(
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
                DebugClass::debugConsole(
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
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, try send without sub code, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return 0;
        }
        if(!mainCode_IsQueryServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(
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
                DebugClass::debugConsole(
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
                DebugClass::debugConsole(
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
        const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    buffer[0]=mainCodeType;
    const quint16 &tempSubCodeType=htobe16(subCodeType);
    memcpy(buffer+1,&tempSubCodeType,sizeof(subCodeType));
    buffer[3]=queryNumber;

    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return 0;
        }
        if(!mainCode_IsQueryClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send as query, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return 0;
        }
        #endif
        if(!sizeMultipleCodePacketClientToServer.contains(mainCodeType))
        {
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    switch(compressionType)
                    {
                        case CompressionType_Xz:
                        case CompressionType_Zlib:
                        default:
                        {
                            const QByteArray &compressedData(computeCompression(QByteArray(data,size)));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(size==0)
                            {
                                DebugClass::debugConsole(
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                                return 0;
                            }
                            #endif
                            const int &newSize=encodeSize(buffer+4,compressedData.size());
                            if(compressedData.size()>0)
                            {
                                memcpy(buffer+4+newSize,compressedData.constData(),compressedData.size());
                                return 4+newSize+compressedData.size();
                            }
                            else
                                return 4+newSize;
                        }
                        break;
                        case CompressionType_None:
                        break;
                    }
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return 0;
            }
            #endif
            const int &newSize=encodeSize(buffer+4,size);
            if(size>0)
            {
                memcpy(buffer+4+newSize,data,size);
                return 4+newSize+size;
            }
            else
                return 4+newSize;
        }
        else if(!sizeMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
        {
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    switch(compressionType)
                    {
                        case CompressionType_Xz:
                        case CompressionType_Zlib:
                        default:
                        {
                            const QByteArray &compressedData(computeCompression(QByteArray(data,size)));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(size==0)
                            {
                                DebugClass::debugConsole(
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                                return 0;
                            }
                            #endif
                            const int &newSize=encodeSize(buffer+4,compressedData.size());
                            if(compressedData.size()>0)
                            {
                                memcpy(buffer+4+newSize,compressedData.constData(),compressedData.size());
                                return 4+newSize+compressedData.size();
                            }
                            else
                                return 4+newSize;
                        }
                        break;
                        case CompressionType_None:
                        break;
                    }
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return 0;
            }
            #endif
            const int &newSize=encodeSize(buffer+4,size);
            if(size>0)
            {
                memcpy(buffer+4+newSize,data,size);
                return 4+newSize+size;
            }
            else
                return 4+newSize;
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                    DebugClass::debugConsole(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingQuery(%1,%2,%3) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            if(size!=sizeMultipleCodePacketClientToServer.value(mainCodeType).value(subCodeType))
            {
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
                return 0;
            }
            #endif
            if(size>0)
            {
                memcpy(buffer+4,data,size);
                return 4+size;
            }
            else
                return 4;
        }
    }
    else
    #endif
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return 0;
        }
        if(!mainCode_IsQueryServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send as query, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return 0;
        }
        #endif
        if(!sizeMultipleCodePacketServerToClient.contains(mainCodeType))
        {
            if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(compressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    switch(compressionType)
                    {
                        case CompressionType_Xz:
                        case CompressionType_Zlib:
                        default:
                        {
                            const QByteArray &compressedData(computeCompression(QByteArray(data,size)));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(size==0)
                            {
                                DebugClass::debugConsole(
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                                return 0;
                            }
                            #endif
                            const int &newSize=encodeSize(buffer+4,compressedData.size());
                            if(compressedData.size()>0)
                            {
                                memcpy(buffer+4+newSize,compressedData.constData(),compressedData.size());
                                return 4+newSize+compressedData.size();
                            }
                            else
                                return 4+newSize;
                        }
                        break;
                        case CompressionType_None:
                        break;
                    }
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return 0;
            }
            #endif
            const int &newSize=encodeSize(buffer+4,size);
            if(size>0)
            {
                memcpy(buffer+4+newSize,data,size);
                return 4+newSize+size;
            }
            else
                return 4+newSize;
        }
        else if(!sizeMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
        {
            if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(compressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    switch(compressionType)
                    {
                        case CompressionType_Xz:
                        case CompressionType_Zlib:
                        default:
                        {
                            const QByteArray &compressedData(computeCompression(QByteArray(data,size)));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(size==0)
                            {
                                DebugClass::debugConsole(
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                                return 0;
                            }
                            #endif
                            const int &newSize=encodeSize(buffer+4,compressedData.size());
                            if(compressedData.size()>0)
                            {
                                memcpy(buffer+4+newSize,compressedData.constData(),compressedData.size());
                                return 4+newSize+compressedData.size();
                            }
                            else
                                return 4+newSize;
                        }
                        break;
                        case CompressionType_None:
                        break;
                    }
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return 0;
            }
            #endif
            const int &newSize=encodeSize(buffer+4,size);
            if(size>0)
            {
                memcpy(buffer+4+newSize,data,size);
                return 4+newSize+size;
            }
            else
                return 4+newSize;
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(compressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                    DebugClass::debugConsole(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingQuery(%1,%2,%3) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            if(size!=sizeMultipleCodePacketServerToClient.value(mainCodeType).value(subCodeType))
            {
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
                return 0;
            }
            #endif
            if(size>0)
            {
                memcpy(buffer+4,data,size);
                return 4+size;
            }
            else
                return 4;
        }
    }
}

int ProtocolParsingBase::computeFullOutcommingData(
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        const bool &isClient,
        #endif
        char *buffer,
        const quint8 &mainCodeType,const quint16 &subCodeType,const char *data,const int &size)
{
    buffer[0]=mainCodeType;
    const quint16 &tempSubCodeType=htobe16(subCodeType);
    memcpy(buffer+1,&tempSubCodeType,sizeof(subCodeType));

    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingData(): mainCodeType: %1, subCodeType: %2, try send with sub code, but not registred as is").arg(mainCodeType).arg(subCodeType));
            return 0;
        }
        if(mainCode_IsQueryClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): mainCodeType: %1, subCodeType: %2, try send as normal data, but not registred as is").arg(mainCodeType).arg(subCodeType));
            return 0;
        }
        #endif
        if(!sizeMultipleCodePacketClientToServer.contains(mainCodeType))
        {
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingData(%1,%2) compression enabled").arg(mainCodeType).arg(subCodeType));
                    #endif
                    switch(compressionType)
                    {
                        case CompressionType_Xz:
                        case CompressionType_Zlib:
                        default:
                        {
                            const QByteArray &compressedData(computeCompression(QByteArray(data,size)));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(compressedData.size()==0)
                            {
                                DebugClass::debugConsole(
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                        case CompressionType_None:
                        break;
                    }
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                return 3+newSize+size;
        }
        else if(!sizeMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
        {
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingData(%1,%2) compression enabled").arg(mainCodeType).arg(subCodeType));
                    #endif
                    switch(compressionType)
                    {
                        case CompressionType_Xz:
                        case CompressionType_Zlib:
                        default:
                        {
                            const QByteArray &compressedData(computeCompression(QByteArray(data,size)));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(compressedData.size()==0)
                            {
                                DebugClass::debugConsole(
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                        case CompressionType_None:
                        break;
                    }
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                    DebugClass::debugConsole(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType));
            if(size!=sizeMultipleCodePacketClientToServer.value(mainCodeType).value(subCodeType))
            {
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
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
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingData(): mainCodeType: %1, subCodeType: %2, try send with sub code, but not registred as is").arg(mainCodeType).arg(subCodeType));
            return 0;
        }
        if(mainCode_IsQueryServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): mainCodeType: %1, subCodeType: %2, try send as normal data, but not registred as is").arg(mainCodeType).arg(subCodeType));
            return 0;
        }
        #endif
        if(!sizeMultipleCodePacketServerToClient.contains(mainCodeType))
        {
            if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(compressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType));
                    #endif
                    switch(compressionType)
                    {
                        case CompressionType_Xz:
                        case CompressionType_Zlib:
                        default:
                        {
                            const QByteArray &compressedData(computeCompression(QByteArray(data,size)));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(compressedData.size()==0)
                            {
                                DebugClass::debugConsole(
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                        case CompressionType_None:
                        break;
                    }
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
            if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(compressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType));
                    #endif
                    switch(compressionType)
                    {
                        case CompressionType_Xz:
                        case CompressionType_Zlib:
                        default:
                        {
                            const QByteArray &compressedData(computeCompression(QByteArray(data,size)));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(compressedData.size()==0)
                            {
                                DebugClass::debugConsole(
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                        case CompressionType_None:
                        break;
                    }
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size==0)
            {
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                    DebugClass::debugConsole(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType));
            if(size!=sizeMultipleCodePacketServerToClient.value(mainCodeType).value(subCodeType))
            {
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
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

#ifdef CATCHCHALLENGER_EXTRA_CHECK
bool ProtocolParsingBase::removeFromQueryReceived(const quint8 &queryNumber)
{
    return queryReceived.remove(queryNumber);
}
#endif

int ProtocolParsingBase::computeReplyData(char *dataBuffer, const quint8 &queryNumber, const char *data, const int &size)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(!queryReceived.contains(queryNumber))
    {
        DebugClass::debugConsole(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    QString::number(isClient)+
                    #endif
        QStringLiteral(" ProtocolParsingInputOutput::postReplyData(): try reply to queryNumber: %1, but this query is not into the list").arg(queryNumber));
        return 0;
    }
    else
        queryReceived.remove(queryNumber);
    #endif

    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
        memcpy(dataBuffer,&replyCodeClientToServer,sizeof(quint8));
    else
    #endif
        memcpy(dataBuffer,&replyCodeServerToClient,sizeof(quint8));
    memcpy(dataBuffer+sizeof(quint8),&queryNumber,sizeof(quint8));

    if(!replyOutputSize.contains(queryNumber))
    {
        if(replyOutputCompression.contains(queryNumber))
        {
            #ifdef PROTOCOLPARSINGDEBUG
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" postReplyData(%1) is now compressed").arg(queryNumber));
            #endif
            switch(compressionType)
            {
                case CompressionType_Xz:
                case CompressionType_Zlib:
                default:
                {
                    const QByteArray &compressedData(computeCompression(QByteArray(data,size)));
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(compressedData.size()==0)
                    {
                        DebugClass::debugConsole(
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
                case CompressionType_None:
                break;
            }
        }
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(size==0)
        {
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" postReplyData(%1,{}) dropped because can be size==0 if not fixed size").arg(queryNumber));
            return 0;
        }
        #endif
        replyOutputCompression.remove(queryNumber);
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
        if(replyOutputCompression.contains(queryNumber))
        {
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" postReplyData(%1,{}) compression disabled because have fixed size").arg(queryNumber));
        }
        if(size!=replyOutputSize.value(queryNumber))
        {
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" postReplyData(%1,{}) dropped because can be size!=fixed size").arg(queryNumber));
            return 0;
        }
        #endif
        replyOutputSize.remove(queryNumber);
        if(size>0)
        {
            memcpy(dataBuffer+sizeof(quint8)*2,data,size);
            return sizeof(quint8)*2+size;
        }
        else
            return sizeof(quint8)*2;
    }
}
