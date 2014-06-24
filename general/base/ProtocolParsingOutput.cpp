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
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, try send without sub code, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return;
        }
        #endif
        if(replySizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionOnlyMainCodePacketServerToClient.contains(mainCodeType))
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, compression disabled because have fixed size").arg(queryNumber).arg(mainCodeType));
            #endif
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, try send without sub code, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return;
        }
        #endif
        if(replySizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionOnlyMainCodePacketClientToServer.contains(mainCodeType))
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, compression disabled because have fixed size").arg(queryNumber).arg(mainCodeType));
            #endif
        }
    }
    #ifdef ProtocolParsingInputOutputDEBUG
    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2").arg(queryNumber).arg(mainCodeType));
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
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return;
        }
        #endif
        if(replySizeMultipleCodePacketServerToClient.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(replyComressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3 compression disabled because have fixed size").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            #endif
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return;
        }
        #endif
        if(replySizeMultipleCodePacketServerToClient.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(replyComressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3 compression disabled because have fixed size").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            #endif
        }
    }
    #ifdef ProtocolParsingInputOutputDEBUG
    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
    #endif
    waitedReply_mainCodeType[queryNumber]=mainCodeType;
    waitedReply_subCodeType[queryNumber]=subCodeType;
}

bool ProtocolParsingInputOutput::postReplyData(const quint8 &queryNumber, const QByteArray &data)
{
    const QByteArray &newData=computeReplyData(queryNumber,data);
    if(newData.isEmpty())
        return false;
    return internalPackOutcommingData(newData);
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
    }
}

bool ProtocolParsingInputOutput::packFullOutcommingData(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data)
{
    const QByteArray &newData=computeFullOutcommingData(isClient,mainCodeType,subCodeType,data);
    if(newData.isEmpty())
        return false;
    return internalPackOutcommingData(newData);
}

bool ProtocolParsingInputOutput::packOutcommingData(const quint8 &mainCodeType,const QByteArray &data)
{
    const QByteArray &newData=computeOutcommingData(isClient,mainCodeType,data);
    if(newData.isEmpty())
        return false;
    return internalPackOutcommingData(newData);
}

bool ProtocolParsingInputOutput::packOutcommingQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    const QByteArray &newData=computeOutcommingQuery(isClient,mainCodeType,queryNumber,data);
    if(newData.isEmpty())
        return false;
    newOutputQuery(mainCodeType,queryNumber);
    return internalPackOutcommingData(newData);
}

bool ProtocolParsingInputOutput::packFullOutcommingQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    const QByteArray &newData=computeFullOutcommingQuery(isClient,mainCodeType,subCodeType,queryNumber,data);
    if(newData.isEmpty())
        return false;
    newFullOutputQuery(mainCodeType,subCodeType,queryNumber);
    return internalPackOutcommingData(newData);
}

bool ProtocolParsingInputOutput::internalPackOutcommingData(QByteArray data)
{
    #ifdef PROTOCOLPARSINGDEBUG
    DebugClass::debugConsole("internalPackOutcommingData(): start");
    #endif
    #ifdef DEBUG_PROTOCOLPARSING_RAW_NETWORK
    message(QStringLiteral("Sended packet size: %1: %2").arg(data.size()).arg(QString(data.toHex())));
    #endif // DEBUG_PROTOCOLPARSING_RAW_NETWORK
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(socket->openMode()|QIODevice::WriteOnly)
    {
    #endif
        if(data.size()<=CATCHCHALLENGER_MAX_PACKET_SIZE)
        {
            TXSize+=data.size();
            byteWriten = socket->write(data);
            if(Q_UNLIKELY(data.size()!=byteWriten))
            {
                DebugClass::debugConsole(QStringLiteral("All the bytes have not be written: %1, byteWriten: %2").arg(socket->errorString()).arg(byteWriten));
                errorParsingLayer(QStringLiteral("All the bytes have not be written: %1, byteWriten: %2").arg(socket->errorString()).arg(byteWriten));
                return false;
            }
            return true;
        }
        else
        {
            QByteArray dataToSend;
            while(Q_LIKELY(!data.isEmpty()))
            {
                dataToSend=data.mid(0,CATCHCHALLENGER_MAX_PACKET_SIZE);
                TXSize+=dataToSend.size();
                byteWriten = socket->write(dataToSend);
                if(Q_UNLIKELY(dataToSend.size()!=byteWriten))
                {
                    DebugClass::debugConsole(QStringLiteral("All the bytes have not be written: %1, byteWriten: %2").arg(socket->errorString()).arg(byteWriten));
                    errorParsingLayer(QStringLiteral("All the bytes have not be written: %1, byteWriten: %2").arg(socket->errorString()).arg(byteWriten));
                    return false;
                }
                data.remove(0,dataToSend.size());
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
bool ProtocolParsingInputOutput::internalSendRawSmallPacket(const QByteArray &data)
{
    #ifdef PROTOCOLPARSINGDEBUG
    DebugClass::debugConsole("internalPackOutcommingData(): start");
    #endif
    #ifdef DEBUG_PROTOCOLPARSING_RAW_NETWORK
    message(QStringLiteral("Sended packet size: %1: %2").arg(data.size()).arg(QString(data.toHex())));
    #endif // DEBUG_PROTOCOLPARSING_RAW_NETWORK
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(data.size()>CATCHCHALLENGER_MAX_PACKET_SIZE)
    {
        DebugClass::debugConsole(QStringLiteral("ProtocolParsingInputOutput::sendRawSmallPacket(): Packet to big: %1").arg(data.size()));
        errorParsingLayer(QStringLiteral("ProtocolParsingInputOutput::sendRawSmallPacket(): Packet to big: %1").arg(data.size()));
        return false;
    }
    #endif

    TXSize+=data.size();
    byteWriten = socket->write(data);
    if(Q_UNLIKELY(data.size()!=byteWriten))
    {
        DebugClass::debugConsole(QStringLiteral("All the bytes have not be written: %1, byteWriten: %2").arg(socket->errorString()).arg(byteWriten));
        errorParsingLayer(QStringLiteral("All the bytes have not be written: %1, byteWriten: %2").arg(socket->errorString()).arg(byteWriten));
        return false;
    }
    return true;
}

QByteArray ProtocolParsingInputOutput::encodeSize(quint32 size)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    if(size<=0xFF)
        out << quint8(size);
    else if(size<=0xFFFF)
        out << quint8(0x00) << quint16(size);
    else
        out << quint16(0x0000) << quint32(size);
    return block;
}

QByteArray ProtocolParsingInputOutput::computeOutcommingData(const bool &isClient,const quint8 &mainCodeType,const QByteArray &data)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << mainCodeType;

    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::packOutcommingData(): mainCodeType: %1, try send without sub code, but not registred as is").arg(mainCodeType));
            return QByteArray();
        }
        if(mainCode_IsQueryClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): mainCodeType: %1, try send as normal data, but not registred as is").arg(mainCodeType));
            return QByteArray();
        }
        #endif
        if(!sizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingData(%1,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType));
                return QByteArray();
            }
            #endif
            block+=encodeSize(data.size());
            return block+data;
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()!=sizeOnlyMainCodePacketClientToServer.value(mainCodeType))
            {
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingData(%1,{}) dropped because can be size!=fixed size").arg(mainCodeType));
                return QByteArray();
            }
            #endif
            return block+data;
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::packOutcommingData(): mainCodeType: %1, try send without sub code, but not registred as is").arg(mainCodeType));
            return QByteArray();
        }
        if(mainCode_IsQueryServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): mainCodeType: %1, try send as normal data, but not registred as is").arg(mainCodeType));
            return QByteArray();
        }
        #endif
        if(!sizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingData(%1,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType));
                return QByteArray();
            }
            #endif
            block+=encodeSize(data.size());
            return block+data;
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()!=sizeOnlyMainCodePacketServerToClient.value(mainCodeType))
            {
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingData(%1,{}) dropped because can be size!=fixed size").arg(mainCodeType));
                return QByteArray();
            }
            #endif
            return block+data;
        }
    }
}

QByteArray ProtocolParsingInputOutput::computeOutcommingQuery(const bool &isClient,const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << mainCodeType;
    out << queryNumber;

    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, try send without sub code, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return QByteArray();
        }
        if(!mainCode_IsQueryClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, try send as query, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return QByteArray();
        }
        #endif
        if(!sizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingQuery(%1,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType));
                return QByteArray();
            }
            #endif
            block+=encodeSize(data.size());
            return block+data;
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()!=sizeOnlyMainCodePacketClientToServer.value(mainCodeType))
            {
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingQuery(%1,{}) dropped because can be size!=fixed size").arg(mainCodeType));
                return QByteArray();
            }
            #endif
            return block+data;
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, try send without sub code, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return QByteArray();
        }
        if(!mainCode_IsQueryServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, try send as query, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return QByteArray();
        }
        #endif
        if(!sizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingQuery(%1,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType));
                return QByteArray();
            }
            #endif
            block+=encodeSize(data.size());
            return block+data;
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()!=sizeOnlyMainCodePacketServerToClient.value(mainCodeType))
            {
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingQuery(%1,{}) dropped because can be size!=fixed size").arg(mainCodeType));
                return QByteArray();
            }
            #endif
            return block+data;
        }
    }
}

QByteArray ProtocolParsingInputOutput::computeFullOutcommingQuery(const bool &isClient,const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << mainCodeType;
    out << subCodeType;
    out << queryNumber;

    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return QByteArray();
        }
        if(!mainCode_IsQueryClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send as query, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return QByteArray();
        }
        #endif
        if(!sizeMultipleCodePacketClientToServer.contains(mainCodeType))
        {
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    switch(compressionType)
                    {
                        case CompressionType_Xz:
                        case CompressionType_Zlib:
                        default:
                        {
                            QByteArray compressedData(computeCompression(data));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(data.size()==0)
                            {
                                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                                return QByteArray();
                            }
                            #endif
                            block+=encodeSize(compressedData.size());
                            return block+compressedData;
                        }
                        break;
                        case CompressionType_None:
                        break;
                    }
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return QByteArray();
            }
            #endif
            block+=encodeSize(data.size());
            return block+data;
        }
        else if(!sizeMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
        {
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    switch(compressionType)
                    {
                        case CompressionType_Xz:
                        case CompressionType_Zlib:
                        default:
                        {
                            QByteArray compressedData(computeCompression(data));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(data.size()==0)
                            {
                                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                                return QByteArray();
                            }
                            #endif
                            block+=encodeSize(compressedData.size());
                            return block+compressedData;
                        }
                        break;
                        case CompressionType_None:
                        break;
                    }
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return QByteArray();
            }
            #endif
            block+=encodeSize(data.size());
            return block+data;
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingQuery(%1,%2,%3) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            if(data.size()!=sizeMultipleCodePacketClientToServer.value(mainCodeType).value(subCodeType))
            {
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
                return QByteArray();
            }
            #endif
            return block+data;
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return QByteArray();
        }
        if(!mainCode_IsQueryServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send as query, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return QByteArray();
        }
        #endif
        if(!sizeMultipleCodePacketServerToClient.contains(mainCodeType))
        {
            if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(compressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    switch(compressionType)
                    {
                        case CompressionType_Xz:
                        case CompressionType_Zlib:
                        default:
                        {
                            QByteArray compressedData(computeCompression(data));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(data.size()==0)
                            {
                                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                                return QByteArray();
                            }
                            #endif
                            block+=encodeSize(compressedData.size());
                            return block+compressedData;
                        }
                        break;
                        case CompressionType_None:
                        break;
                    }
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return QByteArray();
            }
            #endif
            block+=encodeSize(data.size());
            return block+data;
        }
        else if(!sizeMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
        {
            if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(compressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    switch(compressionType)
                    {
                        case CompressionType_Xz:
                        case CompressionType_Zlib:
                        default:
                        {
                            QByteArray compressedData(computeCompression(data));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(data.size()==0)
                            {
                                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                                return QByteArray();
                            }
                            #endif
                            block+=encodeSize(compressedData.size());
                            return block+compressedData;
                        }
                        break;
                        case CompressionType_None:
                        break;
                    }
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return QByteArray();
            }
            #endif
            block+=encodeSize(data.size());
            return block+data;
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(compressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingQuery(%1,%2,%3) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            if(data.size()!=sizeMultipleCodePacketServerToClient.value(mainCodeType).value(subCodeType))
            {
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
                return QByteArray();
            }
            #endif
            return block+data;
        }
    }
}

QByteArray ProtocolParsingInputOutput::computeFullOutcommingData(const bool &isClient,const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << mainCodeType;
    out << subCodeType;

    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::packOutcommingData(): mainCodeType: %1, subCodeType: %2, try send with sub code, but not registred as is").arg(mainCodeType).arg(subCodeType));
            return QByteArray();
        }
        if(mainCode_IsQueryClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): mainCodeType: %1, subCodeType: %2, try send as normal data, but not registred as is").arg(mainCodeType).arg(subCodeType));
            return QByteArray();
        }
        #endif
        if(!sizeMultipleCodePacketClientToServer.contains(mainCodeType))
        {
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingData(%1,%2) compression enabled").arg(mainCodeType).arg(subCodeType));
                    #endif
                    switch(compressionType)
                    {
                        case CompressionType_Xz:
                        case CompressionType_Zlib:
                        default:
                        {
                            QByteArray compressedData(computeCompression(data));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(compressedData.size()==0)
                            {
                                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                                return QByteArray();
                            }
                            #endif
                            block+=encodeSize(compressedData.size());
                            return block+compressedData;
                        }
                        break;
                        case CompressionType_None:
                        break;
                    }
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return QByteArray();
            }
            #endif
            block+=encodeSize(data.size());
            return block+data;
        }
        else if(!sizeMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
        {
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingData(%1,%2) compression enabled").arg(mainCodeType).arg(subCodeType));
                    #endif
                    switch(compressionType)
                    {
                        case CompressionType_Xz:
                        case CompressionType_Zlib:
                        default:
                        {
                            QByteArray compressedData(computeCompression(data));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(compressedData.size()==0)
                            {
                                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                                return QByteArray();
                            }
                            #endif
                            block+=encodeSize(compressedData.size());
                            return block+compressedData;
                        }
                        break;
                        case CompressionType_None:
                        break;
                    }
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return QByteArray();
            }
            #endif
            block+=encodeSize(data.size());
            return block+data;
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType));
            if(data.size()!=sizeMultipleCodePacketClientToServer.value(mainCodeType).value(subCodeType))
            {
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
                return QByteArray();
            }
            #endif
            return block+data;
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::packOutcommingData(): mainCodeType: %1, subCodeType: %2, try send with sub code, but not registred as is").arg(mainCodeType).arg(subCodeType));
            return QByteArray();
        }
        if(mainCode_IsQueryServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): mainCodeType: %1, subCodeType: %2, try send as normal data, but not registred as is").arg(mainCodeType).arg(subCodeType));
            return QByteArray();
        }
        #endif
        if(!sizeMultipleCodePacketServerToClient.contains(mainCodeType))
        {
            if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(compressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType));
                    #endif
                    switch(compressionType)
                    {
                        case CompressionType_Xz:
                        case CompressionType_Zlib:
                        default:
                        {
                            QByteArray compressedData(computeCompression(data));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(compressedData.size()==0)
                            {
                                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                                return QByteArray();
                            }
                            #endif
                            block+=encodeSize(compressedData.size());
                            return block+compressedData;
                        }
                        break;
                        case CompressionType_None:
                        break;
                    }
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return QByteArray();
            }
            #endif
            block+=encodeSize(data.size());
            return block+data;
        }
        else if(!sizeMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
        {
            if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(compressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType));
                    #endif
                    switch(compressionType)
                    {
                        case CompressionType_Xz:
                        case CompressionType_Zlib:
                        default:
                        {
                            QByteArray compressedData(computeCompression(data));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(compressedData.size()==0)
                            {
                                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                                return QByteArray();
                            }
                            #endif
                            block+=encodeSize(compressedData.size());
                            return block+compressedData;
                        }
                        break;
                        case CompressionType_None:
                        break;
                    }
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return QByteArray();
            }
            #endif
            block+=encodeSize(data.size());
            return block+data;
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType));
            if(data.size()!=sizeMultipleCodePacketServerToClient.value(mainCodeType).value(subCodeType))
            {
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
                return QByteArray();
            }
            #endif
            return block+data;
        }
    }
}

QByteArray ProtocolParsingInputOutput::computeReplyData(const quint8 &queryNumber, const QByteArray &data)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(!queryReceived.contains(queryNumber))
    {
        DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::postReplyData(): try reply to queryNumber: %1, but this query is not into the list").arg(queryNumber));
        return QByteArray();
    }
    else
    #endif
        queryReceived.remove(queryNumber);

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    if(isClient)
        out << replyCodeClientToServer;
    else
        out << replyCodeServerToClient;
    out << queryNumber;

    if(!replyOutputSize.contains(queryNumber))
    {
        if(replyOutputCompression.contains(queryNumber))
        {
            #ifdef PROTOCOLPARSINGDEBUG
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" postReplyData(%1) is now compressed").arg(queryNumber));
            #endif
            switch(compressionType)
            {
                case CompressionType_Xz:
                case CompressionType_Zlib:
                default:
                {
                    QByteArray compressedData(computeCompression(data));
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(data.size()==0)
                    {
                        DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" postReplyData(%1,{}) dropped because can be size==0 if not fixed size").arg(queryNumber));
                        return QByteArray();
                    }
                    #endif
                    block+=encodeSize(compressedData.size());
                    return block+compressedData;
                }
                break;
                case CompressionType_None:
                break;
            }
        }
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(data.size()==0)
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" postReplyData(%1,{}) dropped because can be size==0 if not fixed size").arg(queryNumber));
            return QByteArray();
        }
        #endif
        block+=encodeSize(data.size());
        replyOutputCompression.remove(queryNumber);
        return block+data;
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(replyOutputCompression.contains(queryNumber))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" postReplyData(%1,{}) compression disabled because have fixed size").arg(queryNumber));
        }
        if(data.size()!=replyOutputSize.value(queryNumber))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" postReplyData(%1,{}) dropped because can be size!=fixed size").arg(queryNumber));
            return QByteArray();
        }
        #endif
        replyOutputSize.remove(queryNumber);
        return block+data;
    }
}
