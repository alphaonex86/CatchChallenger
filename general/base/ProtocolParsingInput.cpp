#include "ProtocolParsing.h"
#include "DebugClass.h"
#include "GeneralVariable.h"

using namespace CatchChallenger;

void ProtocolParsingInputOutput::parseIncommingData()
{
    #ifdef PROTOCOLPARSINGDEBUG
    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): socket->bytesAvailable(): %1").arg(socket->bytesAvailable()));
    #endif

    while(1)
    {
        quint32 size;
        quint32 cursor=0;
        #ifdef EPOLLCATCHCHALLENGERSERVER
        if(!header_cut.isEmpty())
        {
            const unsigned int &size_to_get=CATCHCHALLENGER_COMMONBUFFERSIZE-header_cut.size();
            memcpy(ProtocolParsingInputOutput::commonBuffer,header_cut.constData(),header_cut.size());
            size=socket->readData(ProtocolParsingInputOutput::commonBuffer,size_to_get)+header_cut.size();
            header_cut.resize(0);
        }
        else
            size=socket->readData(ProtocolParsingInputOutput::commonBuffer,CATCHCHALLENGER_COMMONBUFFERSIZE);
        #else
        if(!header_cut.isEmpty())
        {
            const unsigned int &size_to_get=CATCHCHALLENGER_COMMONBUFFERSIZE-header_cut.size();
            memcpy(ProtocolParsingInputOutput::commonBuffer,header_cut.constData(),header_cut.size());
            size=socket->read(ProtocolParsingInputOutput::commonBuffer,size_to_get)+header_cut.size();
            header_cut.resize(0);
        }
        else
            size=socket->read(ProtocolParsingInputOutput::commonBuffer,CATCHCHALLENGER_COMMONBUFFERSIZE);
        #endif
        if(size==0)
        {
            //DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): size returned is 0!"));
            return;
        }

        do
        {
            if(!parseHeader(size,cursor))
                break;
            if(!parseQueryNumber(size,cursor))
                break;
            if(!parseDataSize(size,cursor))
                break;
            if(!parseData(size,cursor))
                break;
            parseDispatch();
            dataClear();
        } while(cursor<size);

        if(size<CATCHCHALLENGER_COMMONBUFFERSIZE)
            return;
    }
    #ifdef PROTOCOLPARSINGDEBUG
    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): finish parse the input"));
    #endif
}

bool ProtocolParsingInputOutput::parseHeader(const quint32 &size,quint32 &cursor)
{
    if(!haveData)
    {
        if((size-cursor)<sizeof(quint8))//ignore because first int is cuted!
            return false;
        mainCodeType=*(ProtocolParsingInputOutput::commonBuffer+cursor);
        cursor+=sizeof(quint8);
        #ifdef PROTOCOLPARSINGDEBUG
        DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): !haveData, mainCodeType: %1").arg(mainCodeType));
        #endif
        haveData=true;
        haveData_dataSize=false;
        have_subCodeType=false;
        have_query_number=false;
        is_reply=false;
        if(isClient)
            need_subCodeType=!mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType);
        else
            need_subCodeType=!mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType);
        need_query_number=false;
        data_size.resize(0);
    }

    if(!have_subCodeType)
    {
        #ifdef PROTOCOLPARSINGDEBUG
        DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): !have_subCodeType"));
        #endif
        if(!need_subCodeType)
        {
            #ifdef PROTOCOLPARSINGDEBUG
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): !need_subCodeType"));
            #endif
            //if is a reply
            if((isClient && mainCodeType==replyCodeServerToClient) || (!isClient && mainCodeType==replyCodeClientToServer))
            {
                #ifdef PROTOCOLPARSINGDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): need_query_number=true"));
                #endif
                is_reply=true;
                need_query_number=true;
                //the size with be resolved later
            }
            else
            {
                if(isClient)
                {
                    //if is query with reply
                    if(mainCode_IsQueryServerToClient.contains(mainCodeType))
                    {
                        #ifdef PROTOCOLPARSINGDEBUG
                        DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): need_query_number=true, query with reply"));
                        #endif
                        need_query_number=true;
                    }

                    //check if have defined size
                    if(sizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
                    {
                        //have fixed size
                        dataSize=sizeOnlyMainCodePacketServerToClient.value(mainCodeType);
                        haveData_dataSize=true;
                    }
                }
                else
                {
                    //if is query with reply
                    if(mainCode_IsQueryClientToServer.contains(mainCodeType))
                        need_query_number=true;

                    //check if have defined size
                    if(sizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
                    {
                        //have fixed size
                        dataSize=sizeOnlyMainCodePacketClientToServer.value(mainCodeType);
                        haveData_dataSize=true;
                    }
                }
            }
        }
        else
        {
            #ifdef PROTOCOLPARSINGDEBUG
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): need_subCodeType"));
            #endif
            if((size-cursor)<sizeof(quint16))//ignore because first int is cuted!
            {
                RXSize+=size;
                if((size-cursor)>0)
                    header_cut.append(ProtocolParsingInputOutput::commonBuffer+cursor,(size-cursor));
                return false;
            }
            qDebug() << QString(QByteArray(ProtocolParsingInputOutput::commonBuffer,size).toHex());
            subCodeType=be16toh(*reinterpret_cast<quint16 *>(ProtocolParsingInputOutput::commonBuffer+cursor));
            cursor+=sizeof(quint16);

            if(isClient)
            {
                #ifdef PROTOCOLPARSINGDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): isClient"));
                #endif
                //if is query with reply
                if(mainCode_IsQueryServerToClient.contains(mainCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): need_query_number=true, query with reply (mainCode_IsQueryClientToServer)"));
                    #endif
                    need_query_number=true;
                }

                //check if have defined size
                if(sizeMultipleCodePacketServerToClient.contains(mainCodeType))
                {
                    if(sizeMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                    {
                        //have fixed size
                        dataSize=sizeMultipleCodePacketServerToClient.value(mainCodeType).value(subCodeType);
                        haveData_dataSize=true;
                    }
                }
            }
            else
            {
                #ifdef PROTOCOLPARSINGDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): !isClient"));
                #endif
                //if is query with reply
                if(mainCode_IsQueryClientToServer.contains(mainCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): need_query_number=true, query with reply (mainCode_IsQueryServerToClient)"));
                    #endif
                    need_query_number=true;
                }

                //check if have defined size
                if(sizeMultipleCodePacketClientToServer.contains(mainCodeType))
                {
                    if(sizeMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                    {
                        //have fixed size
                        dataSize=sizeMultipleCodePacketClientToServer.value(mainCodeType).value(subCodeType);
                        haveData_dataSize=true;
                    }
                }
            }
        }

        //set this parsing step is done
        have_subCodeType=true;
    }
    #ifdef PROTOCOLPARSINGDEBUG
    else
        DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): have_subCodeType"));
    #endif
    return true;
}

bool ProtocolParsingInputOutput::parseQueryNumber(const quint32 &size,quint32 &cursor)
{
    if(!have_query_number && need_query_number)
    {
        #ifdef PROTOCOLPARSINGDEBUG
        DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): need_query_number"));
        #endif
        if((size-cursor)<sizeof(quint8))
        {
            RXSize+=size;//todo, write message: need more bytes
            return false;
        }
        queryNumber=*(ProtocolParsingInputOutput::commonBuffer+cursor);
        cursor+=sizeof(quint8);

        // it's reply
        if(is_reply)
        {
            if(waitedReply_mainCodeType.contains(queryNumber))
            {
                mainCodeType=waitedReply_mainCodeType.value(queryNumber);
                if(!waitedReply_subCodeType.contains(queryNumber))
                {
                    if(isClient)
                    {
                        if(replySizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
                        {
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(replyComressionOnlyMainCodePacketServerToClient.contains(mainCodeType))
                            {
                                errorParsingLayer("Can't be compressed and fixed size");
                                return false;
                            }
                            #endif
                            dataSize=replySizeOnlyMainCodePacketServerToClient.value(mainCodeType);
                            haveData_dataSize=true;
                        }
                    }
                    else
                    {
                        if(replySizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
                        {
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(replyComressionOnlyMainCodePacketClientToServer.contains(mainCodeType))
                            {
                                errorParsingLayer("Can't be compressed and fixed size");
                                return false;
                            }
                            #endif
                            dataSize=replySizeOnlyMainCodePacketClientToServer.value(mainCodeType);
                            haveData_dataSize=true;
                        }
                    }
                }
                else
                {
                    subCodeType=waitedReply_subCodeType.value(queryNumber);
                    if(isClient)
                    {
                        if(replySizeMultipleCodePacketServerToClient.contains(mainCodeType))
                        {
                            if(replySizeMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                            {
                                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                                if(replyComressionMultipleCodePacketServerToClient.contains(mainCodeType))
                                {
                                    errorParsingLayer("Can't be compressed and fixed size");
                                    return false;
                                }
                                #endif
                                dataSize=replySizeMultipleCodePacketServerToClient.value(mainCodeType).value(subCodeType);
                                haveData_dataSize=true;
                            }
                        }
                    }
                    else
                    {
                        if(replySizeMultipleCodePacketServerToClient.contains(mainCodeType))
                        {
                            if(replySizeMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                            {
                                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                                if(replyComressionMultipleCodePacketClientToServer.contains(mainCodeType))
                                {
                                    errorParsingLayer("Can't be compressed and fixed size");
                                    return false;
                                }
                                #endif
                                dataSize=replySizeMultipleCodePacketServerToClient.value(mainCodeType).value(subCodeType);
                                haveData_dataSize=true;
                            }
                        }
                    }
                }
            }
            else
            {
                errorParsingLayer("It's not reply to a query");
                return false;
            }
        }
        else // it's query with reply
        {
            //size resolved before, into subCodeType step
        }

        //set this parsing step is done
        have_query_number=true;
    }
    #ifdef PROTOCOLPARSINGDEBUG
    else
        DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): not need_query_number"));
    #endif
    return true;
}

bool ProtocolParsingInputOutput::parseDataSize(const quint32 &size,quint32 &cursor)
{
    if(!haveData_dataSize)
    {
        #ifdef PROTOCOLPARSINGDEBUG
        DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): !haveData_dataSize"));
        #endif
        //temp data
        quint8 temp_size_8Bits;
        quint16 temp_size_16Bits;
        quint32 temp_size_32Bits;
        while(!haveData_dataSize)
        {
            switch(data_size.size())
            {
                case 0:
                {
                    if((size-cursor)<sizeof(quint8))
                    {
                        RXSize+=size;
                        return false;
                    }
                    data_size.append(*(ProtocolParsingInputOutput::commonBuffer+cursor));
                    cursor+=sizeof(quint8);
                    QDataStream in_size(data_size);
                    in_size.setVersion(QDataStream::Qt_4_4);
                    in_size >> temp_size_8Bits;
                    if(temp_size_8Bits!=0x00)
                    {
                        dataSize=temp_size_8Bits;
                        haveData_dataSize=true;
                        #ifdef PROTOCOLPARSINGDEBUG
                        DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): have 8Bits data size"));
                        #endif
                    }
                    else
                    {
                        #ifdef PROTOCOLPARSINGDEBUG
                        DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): have not 8Bits data size: %1, temp_size_8Bits: %2").arg(QString(data_size.toHex())).arg(temp_size_8Bits));
                        #endif
                        if(data_size.size()==0)
                        {
                            RXSize+=size;
                            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): internal infinity packet read prevent"));
                            return false;
                        }
                    }
                }
                break;
                case sizeof(quint8):
                {
                    if((size-cursor)<sizeof(quint16))
                    {
                        RXSize+=size;
                        if((size-cursor)>0)
                            header_cut.append(ProtocolParsingInputOutput::commonBuffer+cursor,(size-cursor));
                        return false;
                    }
                    data_size.append(*(ProtocolParsingInputOutput::commonBuffer+cursor));
                    cursor+=sizeof(quint8);
                    {
                        QDataStream in_size(data_size);
                        in_size.setVersion(QDataStream::Qt_4_4);
                        in_size >> temp_size_16Bits;
                    }
                    if(temp_size_16Bits!=0x0000)
                    {
                        data_size.append(*(ProtocolParsingInputOutput::commonBuffer+cursor));
                        cursor+=sizeof(quint8);
                        QDataStream in_size(data_size);
                        in_size.setVersion(QDataStream::Qt_4_4);
                        //in_size.device()->seek(sizeof(quint8)); or in_size >> temp_size_8Bits;, not both
                        in_size >> temp_size_8Bits;
                        in_size >> temp_size_16Bits;
                        dataSize=temp_size_16Bits;
                        haveData_dataSize=true;
                        #ifdef PROTOCOLPARSINGDEBUG
                        DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): have 16Bits data size: %1, temp_size_16Bits: %2").arg(QString(data_size.toHex())).arg(dataSize));
                        #endif
                    }
                    else
                    {
                        #ifdef PROTOCOLPARSINGDEBUG
                        DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): have not 16Bits data size"));
                        #endif
                        if(data_size.size()==sizeof(quint8))
                        {
                            RXSize+=size;
                            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): internal infinity packet read prevent"));
                            header_cut.append(ProtocolParsingInputOutput::commonBuffer+cursor,(size-cursor));
                            return false;
                        }
                    }
                }
                break;
                case sizeof(quint16):
                {
                    if((size-cursor)<sizeof(quint32))
                    {
                        RXSize+=size;
                        if((size-cursor)>0)
                            header_cut.append(ProtocolParsingInputOutput::commonBuffer+cursor,(size-cursor));
                        return false;
                    }
                    data_size.append(be32toh(*reinterpret_cast<quint32 *>(ProtocolParsingInputOutput::commonBuffer+cursor)));
                    cursor+=sizeof(quint32);
                    QDataStream in_size(data_size);
                    in_size.setVersion(QDataStream::Qt_4_4);
                    in_size >> temp_size_16Bits;
                    in_size >> temp_size_32Bits;
                    if(temp_size_32Bits!=0x00000000)
                    {
                        dataSize=temp_size_32Bits;
                        haveData_dataSize=true;
                    }
                    else
                    {
                        errorParsingLayer("size is null");
                        if((size-cursor)>0)
                            header_cut.append(ProtocolParsingInputOutput::commonBuffer+cursor,(size-cursor));
                        return false;
                    }
                }
                break;
                default:
                errorParsingLayer(QStringLiteral("size not understand, internal bug: %1").arg(data_size.size()));
                return false;
            }
        }
    }
    #ifdef PROTOCOLPARSINGDEBUG
    else
        DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): haveData_dataSize"));
    #endif
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(!haveData_dataSize)
    {
        errorParsingLayer("have not the size here!");
        return false;
    }
    #endif
    #ifdef PROTOCOLPARSINGDEBUG
    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): header informations is ready, dataSize: %1").arg(dataSize));
    #endif
    if(dataSize>16*1024*1024)
    {
        errorParsingLayer("packet size too big");
        return false;
    }
    RXSize+=size;
    return true;
}

bool ProtocolParsingInputOutput::parseData(const quint32 &size,quint32 &cursor)
{
    if(dataSize>0)
    {
        //if have too many data, or just the size
        if((dataSize-data.size())<=(size-cursor))
        {
            const quint32 &size_to_append=dataSize-data.size();
            RXSize+=size_to_append;
            data.append(ProtocolParsingInputOutput::commonBuffer+cursor,size_to_append);
            cursor+=size_to_append;
            #ifdef PROTOCOLPARSINGDEBUG
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): remaining data: %1").arg((size-cursor)));
            #endif
        }
        else //if need more data
        {
            RXSize+=(size-cursor);
            data.append(ProtocolParsingInputOutput::commonBuffer+cursor,(size-cursor));
            #ifdef PROTOCOLPARSINGDEBUG
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): need more to recompose: %1").arg(dataSize-data.size()));
            #endif
            return false;
        }
    }
    else
    {
        #ifdef PROTOCOLPARSINGDEBUG
        DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): no need data"));
        #endif
    }
    #ifdef PROTOCOLPARSINGDEBUG
    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): data.size(): %1").arg(data.size()));
    #endif
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(dataSize!=(quint32)data.size())
    {
        errorParsingLayer("wrong data size here");
        return false;
    }
    #endif
    return true;
}

void ProtocolParsingInputOutput::parseDispatch()
{
    #ifdef ProtocolParsingInputOutputDEBUG
    if(isClient)
        DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): parse message as client"));
    else
        DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): parse message as server"));
    #endif
    #ifdef ProtocolParsingInputOutputDEBUG
    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): data: %1").arg(QString(data.toHex())));
    #endif
    //message
    if(!need_query_number)
    {
        if(!need_subCodeType)
        {
            #ifdef ProtocolParsingInputOutputDEBUG
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): !need_query_number && !need_subCodeType, mainCodeType: %1").arg(mainCodeType));
            #endif
            parseMessage(mainCodeType,data);
        }
        else
        {
            #ifdef ProtocolParsingInputOutputDEBUG
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): !need_query_number && need_subCodeType, mainCodeType: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
            #endif
            if(isClient)
            {
                if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                    if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                        switch(compressionType)
                        {
                            case CompressionType_Xz:
                                data=lzmaUncompress(data);
                            break;
                            case CompressionType_Zlib:
                            default:
                                data=qUncompress(data);
                            break;
                            case CompressionType_None:
                            break;
                        }
            }
            else
            {
                if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                    if(compressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                        switch(compressionType)
                        {
                            case CompressionType_Xz:
                                data=lzmaUncompress(data);
                            break;
                            case CompressionType_Zlib:
                            default:
                                data=qUncompress(data);
                            break;
                            case CompressionType_None:
                            break;
                        }
            }
            parseFullMessage(mainCodeType,subCodeType,data);
        }
    }
    else
    {
        //query
        if(!is_reply)
        {
            if(!need_subCodeType)
            {
                #ifdef ProtocolParsingInputOutputDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): need_query_number && !is_reply, mainCodeType: %1").arg(mainCodeType));
                #endif
                storeInputQuery(mainCodeType,queryNumber);
                parseQuery(mainCodeType,queryNumber,data);
            }
            else
            {
                #ifdef ProtocolParsingInputOutputDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): need_query_number && !is_reply, mainCodeType: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
                #endif
                if(isClient)
                {
                    if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                        if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                            switch(compressionType)
                            {
                                case CompressionType_Xz:
                                    data=lzmaUncompress(data);
                                break;
                                case CompressionType_Zlib:
                                default:
                                    data=qUncompress(data);
                                break;
                                case CompressionType_None:
                                break;
                            }
                }
                else
                {
                    if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                        if(compressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                            switch(compressionType)
                            {
                                case CompressionType_Xz:
                                    data=lzmaUncompress(data);
                                break;
                                case CompressionType_Zlib:
                                default:
                                    data=qUncompress(data);
                                break;
                                case CompressionType_None:
                                break;
                            }
                }
                storeFullInputQuery(mainCodeType,subCodeType,queryNumber);
                parseFullQuery(mainCodeType,subCodeType,queryNumber,data);
            }
        }
        else
        {
            //reply
            if(!waitedReply_subCodeType.contains(queryNumber))
            {
                waitedReply_mainCodeType.remove(queryNumber);
                #ifdef ProtocolParsingInputOutputDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): need_query_number && is_reply && reply_subCodeType.contains(queryNumber), queryNumber: %1, mainCodeType: %2").arg(queryNumber).arg(mainCodeType));
                #endif
                if(isClient)
                {
                    if(replyComressionOnlyMainCodePacketClientToServer.contains(mainCodeType))
                        switch(compressionType)
                        {
                            case CompressionType_Xz:
                                data=lzmaUncompress(data);
                            break;
                            case CompressionType_Zlib:
                            default:
                                data=qUncompress(data);
                            break;
                            case CompressionType_None:
                            break;
                        }
                }
                else
                {
                    if(replyComressionOnlyMainCodePacketServerToClient.contains(mainCodeType))
                        switch(compressionType)
                        {
                            case CompressionType_Xz:
                                data=lzmaUncompress(data);
                            break;
                            case CompressionType_Zlib:
                            default:
                                data=qUncompress(data);
                            break;
                            case CompressionType_None:
                            break;
                        }
                }
                parseReplyData(mainCodeType,queryNumber,data);
            }
            else
            {
                waitedReply_mainCodeType.remove(queryNumber);
                waitedReply_subCodeType.remove(queryNumber);
                #ifdef ProtocolParsingInputOutputDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" parseIncommingData(): need_query_number && is_reply && reply_subCodeType.contains(queryNumber), queryNumber: %1, mainCodeType: %2, subCodeType: %3").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
                #endif
                if(isClient)
                {
                    if(replyComressionMultipleCodePacketServerToClient.contains(mainCodeType))
                        if(replyComressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                            switch(compressionType)
                            {
                                case CompressionType_Xz:
                                    data=lzmaUncompress(data);
                                break;
                                case CompressionType_Zlib:
                                default:
                                    data=qUncompress(data);
                                break;
                                case CompressionType_None:
                                break;
                            }
                }
                else
                {
                    if(replyComressionMultipleCodePacketClientToServer.contains(mainCodeType))
                        if(replyComressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                            switch(compressionType)
                            {
                                case CompressionType_Xz:
                                    data=lzmaUncompress(data);
                                break;
                                case CompressionType_Zlib:
                                default:
                                    data=qUncompress(data);
                                break;
                                case CompressionType_None:
                                break;
                            }
                }
                parseFullReplyData(mainCodeType,subCodeType,queryNumber,data);
            }
        }
    }
}

void ProtocolParsingInputOutput::dataClear()
{
    data.clear();
    dataSize=0;
    haveData=false;
}

void ProtocolParsingInputOutput::storeInputQuery(const quint8 &mainCodeType,const quint8 &queryNumber)
{
    if(queryReceived.contains(queryNumber))
    {
        DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" storeInputQuery(%1,%2) query with same id previously say").arg(mainCodeType).arg(queryNumber));
        return;
    }
    queryReceived << queryNumber;
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::storeInputQuery(): queryNumber: %1, mainCodeType: %2, try send without sub code, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return;
        }
        #endif
        if(replySizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionOnlyMainCodePacketServerToClient.contains(mainCodeType))
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" storeInputQuery(%1,%2) compression can't be enabled with fixed size").arg(mainCodeType).arg(queryNumber));
            #endif
            //register the size of the reply to send
            replyOutputSize[queryNumber]=replySizeOnlyMainCodePacketServerToClient.value(mainCodeType);
        }
        else
        {
            if(replyComressionOnlyMainCodePacketServerToClient.contains(mainCodeType))
            {
                #ifdef PROTOCOLPARSINGDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" storeInputQuery(%1,%2) compression enabled").arg(mainCodeType).arg(queryNumber));
                #endif
                //register the compression of the reply to send
                replyOutputCompression << queryNumber;
            }
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::storeInputQuery(): queryNumber: %1, mainCodeType: %2, try send without sub code, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return;
        }
        #endif
        if(replySizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionOnlyMainCodePacketClientToServer.contains(mainCodeType))
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" storeInputQuery(%1,%2) compression can't be enabled with fixed size").arg(mainCodeType).arg(queryNumber));
            #endif
            //register the size of the reply to send
            replyOutputSize[queryNumber]=replySizeOnlyMainCodePacketClientToServer.value(mainCodeType);
        }
        else
        {
            if(replyComressionOnlyMainCodePacketClientToServer.contains(mainCodeType))
            {
                #ifdef PROTOCOLPARSINGDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" storeInputQuery(%1,%2) compression enabled").arg(mainCodeType).arg(queryNumber));
                #endif
                //register the compression of the reply to send
                replyOutputCompression << queryNumber;
            }
        }
    }
}

void ProtocolParsingInputOutput::storeFullInputQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber)
{
    if(queryReceived.contains(queryNumber))
    {
        errorParsingLayer(QString::number(isClient)+QStringLiteral(" storeInputQuery(%1,%2,%3) query with same id previously say").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
        return;
    }
    queryReceived << queryNumber;
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::storeInputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return;
        }
        #endif
        if(replySizeMultipleCodePacketClientToServer.contains(mainCodeType))
        {
            if(replySizeMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
            {
                #ifdef PROTOCOLPARSINGDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" storeInputQuery(%1,%2,%3) fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                #endif
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(replyComressionMultipleCodePacketClientToServer.contains(mainCodeType))
                    if(replyComressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                        DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" storeInputQuery(%1,%2,%3) compression can't be enabled with fixed size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                #endif
                replyOutputSize[queryNumber]=replySizeMultipleCodePacketClientToServer.value(mainCodeType).value(subCodeType);
            }
            else
            {
                #ifdef PROTOCOLPARSINGDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" storeInputQuery(%1,%2,%3) 1) not fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                #endif
                if(replyComressionMultipleCodePacketClientToServer.contains(mainCodeType))
                    if(replyComressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                    {
                        #ifdef PROTOCOLPARSINGDEBUG
                        DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" storeInputQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                        #endif
                        //register the compression of the reply to send
                        replyOutputCompression << queryNumber;
                    }
            }
        }
        else
        {
            #ifdef PROTOCOLPARSINGDEBUG
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" storeInputQuery(%1,%2,%3) 1) not fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            #endif
            if(replyComressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(replyComressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" storeInputQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    //register the compression of the reply to send
                    replyOutputCompression << queryNumber;
                }
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" ProtocolParsingInputOutput::storeInputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return;
        }
        #endif
        if(replySizeMultipleCodePacketServerToClient.contains(mainCodeType))
        {
            if(replySizeMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
            {
                #ifdef PROTOCOLPARSINGDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" storeInputQuery(%1,%2,%3) fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                #endif
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(replyComressionMultipleCodePacketServerToClient.contains(mainCodeType))
                    if(replyComressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                        DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" storeInputQuery(%1,%2,%3) compression can't be enabled with fixed size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                #endif
                replyOutputSize[queryNumber]=replySizeMultipleCodePacketServerToClient.value(mainCodeType).value(subCodeType);
            }
            else
            {
                #ifdef PROTOCOLPARSINGDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" storeInputQuery(%1,%2,%3) 1) not fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                #endif
                if(replyComressionMultipleCodePacketServerToClient.contains(mainCodeType))
                    if(replyComressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                    {
                        #ifdef PROTOCOLPARSINGDEBUG
                        DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" storeInputQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                        #endif
                        //register the compression of the reply to send
                        replyOutputCompression << queryNumber;
                    }
            }
        }
        else
        {
            #ifdef PROTOCOLPARSINGDEBUG
            DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" storeInputQuery(%1,%2,%3) 1) not fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            #endif
            if(replyComressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(replyComressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QStringLiteral(" storeInputQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    //register the compression of the reply to send
                    replyOutputCompression << queryNumber;
               }
        }
    }
}
