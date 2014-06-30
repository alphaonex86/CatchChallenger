#include "ProtocolParsing.h"
#include "DebugClass.h"
#include "GeneralVariable.h"

using namespace CatchChallenger;

void ProtocolParsingInputOutput::newOutputQuery(const quint8 &mainCodeType,const quint8 &queryNumber)
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

void ProtocolParsingInputOutput::newFullOutputQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber)
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

bool ProtocolParsingInputOutput::postReplyData(const quint8 &queryNumber, const char *data,const int &size)
{
    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    const int &newSize=ProtocolParsingInputOutput::computeReplyData(ProtocolParsingInputOutput::tempBigBufferForOutput,queryNumber,data,size);
    if(newSize==0)
        return false;
    return internalPackOutcommingData(ProtocolParsingInputOutput::tempBigBufferForOutput,newSize);
    #else
    QByteArray bigBufferForOutput;
    bigBufferForOutput.resize(16+size);
    const int &newSize=ProtocolParsingInputOutput::computeReplyData(bigBufferForOutput.data(),queryNumber,data,size);
    if(newSize==0)
        return false;
    return internalPackOutcommingData(bigBufferForOutput.data(),newSize);
    #endif
}

QByteArray ProtocolParsingInputOutput::computeCompression(const QByteArray &data)
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

bool ProtocolParsingInputOutput::packFullOutcommingData(const quint8 &mainCodeType,const quint16 &subCodeType,const char *data,const int &size)
{
    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    const int &newSize=computeFullOutcommingData(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                isClient,
                #endif
                ProtocolParsingInputOutput::tempBigBufferForOutput,
                mainCodeType,subCodeType,data,size);
    if(newSize==0)
        return false;
    return internalPackOutcommingData(ProtocolParsingInputOutput::tempBigBufferForOutput,newSize);
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

bool ProtocolParsingInputOutput::packOutcommingData(const quint8 &mainCodeType,const char *data,const int &size)
{
    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    const int &newSize=computeOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            isClient,
            #endif
            ProtocolParsingInputOutput::tempBigBufferForOutput,
            mainCodeType,data,size);
    if(newSize==0)
        return false;
    return internalPackOutcommingData(ProtocolParsingInputOutput::tempBigBufferForOutput,newSize);
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

bool ProtocolParsingInputOutput::packOutcommingQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    const int &newSize=computeOutcommingQuery(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            isClient,
            #endif
            ProtocolParsingInputOutput::tempBigBufferForOutput,
            mainCodeType,queryNumber,data,size);
    if(newSize==0)
        return false;
    newOutputQuery(mainCodeType,queryNumber);
    return internalPackOutcommingData(ProtocolParsingInputOutput::tempBigBufferForOutput,newSize);
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

bool ProtocolParsingInputOutput::packFullOutcommingQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    const int &newSize=computeFullOutcommingQuery(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            isClient,
            #endif
            ProtocolParsingInputOutput::tempBigBufferForOutput,
            mainCodeType,subCodeType,queryNumber,data,size);
    if(newSize==0)
        return false;
    newFullOutputQuery(mainCodeType,subCodeType,queryNumber);
    return internalPackOutcommingData(ProtocolParsingInputOutput::tempBigBufferForOutput,newSize);
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

bool ProtocolParsingInputOutput::internalPackOutcommingData(const char *data,const int &size)
{
    #ifdef PROTOCOLPARSINGDEBUG
    DebugClass::debugConsole("internalPackOutcommingData(): start");
    #endif
    #ifdef DEBUG_PROTOCOLPARSING_RAW_NETWORK
    message(QStringLiteral("Sended packet size: %1: %2").arg(size).arg(QString(data.toHex())));
    #endif // DEBUG_PROTOCOLPARSING_RAW_NETWORK
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(socket->openMode()|QIODevice::WriteOnly)
    {
    #endif
        if(size<=CATCHCHALLENGER_MAX_PACKET_SIZE)
        {
            int byteWriten = socket->write(data,size);
            if(Q_UNLIKELY(size!=byteWriten))
            {
                DebugClass::debugConsole(QStringLiteral("All the bytes have not be written: %1, byteWriten: %2").arg(socket->errorString()).arg(byteWriten));
                errorParsingLayer(QStringLiteral("All the bytes have not be written: %1, byteWriten: %2").arg(socket->errorString()).arg(byteWriten));
                return false;
            }
            TXSize+=size;
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
                byteWriten = socket->write(data+cursor,size_to_send);
                if(Q_UNLIKELY(size_to_send!=byteWriten))
                {
                    DebugClass::debugConsole(QStringLiteral("All the bytes have not be written: %1, byteWriten: %2").arg(socket->errorString()).arg(byteWriten));
                    errorParsingLayer(QStringLiteral("All the bytes have not be written: %1, byteWriten: %2").arg(socket->errorString()).arg(byteWriten));
                    return false;
                }
                TXSize+=size_to_send;
                cursor+=size_to_send;
            }
            return true;
        }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    }
    else
    {
        DebugClass::debugConsole(QStringLiteral("Socket open in read only!"));
        errorParsingLayer(QStringLiteral("Socket open in read only!"));
        return false;
    }
    #endif
}

//no control to be more fast
bool ProtocolParsingInputOutput::internalSendRawSmallPacket(const char *data,const int &size)
{
    #ifdef PROTOCOLPARSINGDEBUG
    DebugClass::debugConsole("internalPackOutcommingData(): start");
    #endif
    #ifdef DEBUG_PROTOCOLPARSING_RAW_NETWORK
    message(QStringLiteral("Sended packet size: %1: %2").arg(size).arg(QString(data.toHex())));
    #endif // DEBUG_PROTOCOLPARSING_RAW_NETWORK
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size>CATCHCHALLENGER_MAX_PACKET_SIZE)
    {
        DebugClass::debugConsole(QStringLiteral("ProtocolParsingInputOutput::sendRawSmallPacket(): Packet to big: %1").arg(size));
        errorParsingLayer(QStringLiteral("ProtocolParsingInputOutput::sendRawSmallPacket(): Packet to big: %1").arg(size));
        return false;
    }
    #endif

    TXSize+=size;
    const int &byteWriten = socket->write(data,size);
    if(Q_UNLIKELY(size!=byteWriten))
    {
        DebugClass::debugConsole(QStringLiteral("All the bytes have not be written: %1, byteWriten: %2").arg(socket->errorString()).arg(byteWriten));
        errorParsingLayer(QStringLiteral("All the bytes have not be written: %1, byteWriten: %2").arg(socket->errorString()).arg(byteWriten));
        return false;
    }
    return true;
}

qint8 ProtocolParsingInputOutput::encodeSize(char *data,const quint32 &size)
{
    if(size<=0xFF)
    {
        memcpy(data,&size,sizeof(quint8));
        return sizeof(quint8);
    }
    else if(size<=0xFFFF)
    {
        const quint16 &newSize=htobe16(size);
        memcpy(data,&ProtocolParsingInputOutput::sizeHeaderNullquint16,sizeof(quint8));
        memcpy(data,&newSize,sizeof(newSize));
        return sizeof(quint8)+sizeof(quint16);
    }
    else
    {
        const quint32 &newSize=htobe32(size);
        memcpy(data,&ProtocolParsingInputOutput::sizeHeaderNullquint16,sizeof(quint16));
        memcpy(data,&newSize,sizeof(newSize));
        return sizeof(quint16)+sizeof(quint32);
    }
}

int ProtocolParsingInputOutput::computeOutcommingData(
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

int ProtocolParsingInputOutput::computeOutcommingQuery(
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

int ProtocolParsingInputOutput::computeFullOutcommingQuery(
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

int ProtocolParsingInputOutput::computeFullOutcommingData(
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

bool ProtocolParsingInputOutput::removeFromQueryReceived(const quint8 &queryNumber)
{
    return queryReceived.remove(queryNumber);
}

int ProtocolParsingInputOutput::computeReplyData(char *dataBuffer, const quint8 &queryNumber, const char *data, const int &size)
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
    #endif
        queryReceived.remove(queryNumber);

    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
        memcpy(dataBuffer,&replyCodeClientToServer,sizeof(quint8));
    else
    #else
        memcpy(dataBuffer,&replyCodeServerToClient,sizeof(quint8));
    #endif
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
                        QByteArray tempBuffer(dataBuffer,fullSize+compressedData.size());
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
