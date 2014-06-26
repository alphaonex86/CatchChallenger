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
    #else
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
    #endif
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
    #else
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
    #endif
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

bool ProtocolParsingInputOutput::postReplyData(const quint8 &queryNumber, const QByteArray &data)
{
    #ifdef CATCHCHALLENGER_BIGBUFFERSIZE
    const int &size=ProtocolParsingInputOutput::computeReplyData(ProtocolParsingInputOutput::tempBigBuffer,queryNumber,data);
    if(size==0)
        return false;
    return internalPackOutcommingData(ProtocolParsingInputOutput::tempBigBuffer,size);
    #else
    const QByteArray &tempData(ProtocolParsingInputOutput::computeReplyData(queryNumber,data));
    if(tempData.size()==0)
        return false;
    return internalPackOutcommingData(tempData);
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
    }
}

bool ProtocolParsingInputOutput::packFullOutcommingData(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data)
{
    const QByteArray &newData=computeFullOutcommingData(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                isClient,
                #endif
                mainCodeType,subCodeType,data);
    if(newData.isEmpty())
        return false;
    return internalPackOutcommingData(newData);
}

bool ProtocolParsingInputOutput::packOutcommingData(const quint8 &mainCodeType,const QByteArray &data)
{
    const QByteArray &newData=computeOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            isClient,
            #endif
            mainCodeType,data);
    if(newData.isEmpty())
        return false;
    return internalPackOutcommingData(newData);
}

bool ProtocolParsingInputOutput::packOutcommingQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    const QByteArray &newData=computeOutcommingQuery(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            isClient,
            #endif
            mainCodeType,queryNumber,data);
    if(newData.isEmpty())
        return false;
    newOutputQuery(mainCodeType,queryNumber);
    return internalPackOutcommingData(newData);
}

bool ProtocolParsingInputOutput::packFullOutcommingQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    const QByteArray &newData=computeFullOutcommingQuery(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            isClient,
            #endif
            mainCodeType,subCodeType,queryNumber,data);
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
            const int &byteWriten = socket->write(data);
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
            int byteWriten;
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
    const int &byteWriten = socket->write(data);
    if(Q_UNLIKELY(data.size()!=byteWriten))
    {
        DebugClass::debugConsole(QStringLiteral("All the bytes have not be written: %1, byteWriten: %2").arg(socket->errorString()).arg(byteWriten));
        errorParsingLayer(QStringLiteral("All the bytes have not be written: %1, byteWriten: %2").arg(socket->errorString()).arg(byteWriten));
        return false;
    }
    return true;
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

QByteArray ProtocolParsingInputOutput::encodeSize(const quint32 &size)
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

QByteArray ProtocolParsingInputOutput::computeOutcommingData(
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        const bool &isClient,
        #endif
        const quint8 &mainCodeType,const QByteArray &data)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << mainCodeType;

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
            return QByteArray();
        }
        if(mainCode_IsQueryClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): mainCodeType: %1, try send as normal data, but not registred as is").arg(mainCodeType));
            return QByteArray();
        }
        #endif
        if(!sizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType));
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
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,{}) dropped because can be size!=fixed size").arg(mainCodeType));
                return QByteArray();
            }
            #endif
            return block+data;
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
            return QByteArray();
        }
        if(mainCode_IsQueryServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): mainCodeType: %1, try send as normal data, but not registred as is").arg(mainCodeType));
            return QByteArray();
        }
        #endif
        if(!sizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType));
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
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,{}) dropped because can be size!=fixed size").arg(mainCodeType));
                return QByteArray();
            }
            #endif
            return block+data;
        }
    }
}

QByteArray ProtocolParsingInputOutput::computeOutcommingQuery(
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        const bool &isClient,
        #endif
        const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << mainCodeType;
    out << queryNumber;

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
            return QByteArray();
        }
        if(!mainCode_IsQueryClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, try send as query, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return QByteArray();
        }
        #endif
        if(!sizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType));
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
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,{}) dropped because can be size!=fixed size").arg(mainCodeType));
                return QByteArray();
            }
            #endif
            return block+data;
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
            return QByteArray();
        }
        if(!mainCode_IsQueryServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, try send as query, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return QByteArray();
        }
        #endif
        if(!sizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType));
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
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,{}) dropped because can be size!=fixed size").arg(mainCodeType));
                return QByteArray();
            }
            #endif
            return block+data;
        }
    }
}

QByteArray ProtocolParsingInputOutput::computeFullOutcommingQuery(
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        const bool &isClient,
        #endif
        const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << mainCodeType;
    out << subCodeType;
    out << queryNumber;

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
            return QByteArray();
        }
        if(!mainCode_IsQueryClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send as query, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return QByteArray();
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
                            QByteArray compressedData(computeCompression(data));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(data.size()==0)
                            {
                                DebugClass::debugConsole(
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                            QByteArray compressedData(computeCompression(data));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(data.size()==0)
                            {
                                DebugClass::debugConsole(
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                    DebugClass::debugConsole(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingQuery(%1,%2,%3) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            if(data.size()!=sizeMultipleCodePacketClientToServer.value(mainCodeType).value(subCodeType))
            {
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
                return QByteArray();
            }
            #endif
            return block+data;
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
            return QByteArray();
        }
        if(!mainCode_IsQueryServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send as query, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return QByteArray();
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
                            QByteArray compressedData(computeCompression(data));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(data.size()==0)
                            {
                                DebugClass::debugConsole(
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                            QByteArray compressedData(computeCompression(data));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(data.size()==0)
                            {
                                DebugClass::debugConsole(
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                    DebugClass::debugConsole(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingQuery(%1,%2,%3) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            if(data.size()!=sizeMultipleCodePacketServerToClient.value(mainCodeType).value(subCodeType))
            {
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingQuery(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
                return QByteArray();
            }
            #endif
            return block+data;
        }
    }
}

QByteArray ProtocolParsingInputOutput::computeFullOutcommingData(
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        const bool &isClient,
        #endif
        const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << mainCodeType;
    out << subCodeType;

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
            return QByteArray();
        }
        if(mainCode_IsQueryClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): mainCodeType: %1, subCodeType: %2, try send as normal data, but not registred as is").arg(mainCodeType).arg(subCodeType));
            return QByteArray();
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
                            QByteArray compressedData(computeCompression(data));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(compressedData.size()==0)
                            {
                                DebugClass::debugConsole(
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                            QByteArray compressedData(computeCompression(data));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(compressedData.size()==0)
                            {
                                DebugClass::debugConsole(
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                    DebugClass::debugConsole(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType));
            if(data.size()!=sizeMultipleCodePacketClientToServer.value(mainCodeType).value(subCodeType))
            {
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
                return QByteArray();
            }
            #endif
            return block+data;
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
            return QByteArray();
        }
        if(mainCode_IsQueryServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::packOutcommingQuery(): mainCodeType: %1, subCodeType: %2, try send as normal data, but not registred as is").arg(mainCodeType).arg(subCodeType));
            return QByteArray();
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
                            QByteArray compressedData(computeCompression(data));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(compressedData.size()==0)
                            {
                                DebugClass::debugConsole(
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                            QByteArray compressedData(computeCompression(data));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(compressedData.size()==0)
                            {
                                DebugClass::debugConsole(
                                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                            QString::number(isClient)+
                                            #endif
                                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
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
                    DebugClass::debugConsole(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType));
            if(data.size()!=sizeMultipleCodePacketServerToClient.value(mainCodeType).value(subCodeType))
            {
                DebugClass::debugConsole(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" packOutcommingData(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
                return QByteArray();
            }
            #endif
            return block+data;
        }
    }
}

QByteArray ProtocolParsingInputOutput::computeReplyData(const quint8 &queryNumber, const QByteArray &data)
{
    QByteArray tempData;
    tempData.resize(16+data.size());
    tempData.resize(computeReplyData(tempData.data(),queryNumber,data));
    return data;
}

int ProtocolParsingInputOutput::computeReplyData(char *dataBuffer,const quint8 &queryNumber, const QByteArray &data)
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
                    const QByteArray &compressedData(computeCompression(data));
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(data.size()==0)
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
                    memcpy(dataBuffer+fullSize,compressedData.constData(),compressedData.size());
                    return fullSize+compressedData.size();
                }
                break;
                case CompressionType_None:
                break;
            }
        }
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(data.size()==0)
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
        const quint8 &fullSize=sizeof(quint8)*2+encodeSize(dataBuffer+sizeof(quint8)*2,data.size());
        memcpy(dataBuffer+fullSize,data.constData(),data.size());
        return fullSize+data.size();
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
        if(data.size()!=replyOutputSize.value(queryNumber))
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
        memcpy(dataBuffer+sizeof(quint8)*2,data.constData(),data.size());
        return sizeof(quint8)*2+data.size();
    }
}
