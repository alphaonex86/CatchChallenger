#include "ProtocolParsing.h"
#include "ProtocolParsingCheck.h"
#include "DebugClass.h"
#include "GeneralVariable.h"

using namespace CatchChallenger;

ssize_t ProtocolParsingInputOutput::read(char * data, const int &size)
{
    #if defined (CATCHCHALLENGER_EXTRA_CHECK) && ! defined (EPOLLCATCHCHALLENGERSERVER)
    if(socket->openMode()|QIODevice::WriteOnly)
    {}
    else
    {
        messageParsingLayer(QStringLiteral("Socket open in read only!"));
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

ssize_t ProtocolParsingInputOutput::write(const char * data, const int &size)
{
    #if defined (CATCHCHALLENGER_EXTRA_CHECK) && ! defined (EPOLLCATCHCHALLENGERSERVER)
    if(socket->openMode()|QIODevice::WriteOnly)
    {}
    else
    {
        messageParsingLayer(QStringLiteral("Socket open in write only!"));
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
    if(Q_UNLIKELY(size!=byteWriten))
    {
        #ifdef EPOLLCATCHCHALLENGERSERVER
        messageParsingLayer(QStringLiteral("All the bytes have not be written byteWriten: %1, size: %2").arg(byteWriten).arg(size));
        #else
        messageParsingLayer(QStringLiteral("All the bytes have not be written: %1, byteWriten: %2").arg(socket->errorString()).arg(byteWriten));
        #endif
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            quint32 cursor=0;
            if(!protocolParsingCheck->parseIncommingDataRaw(data,size,cursor))
            {
                qDebug() << "Bug at data-sending";
                abort();
            }
            if(!protocolParsingCheck->valid)
            {
                qDebug() << "Bug at data-sending not tigger the function";
                abort();
            }
            if(cursor!=(quint32)size)
            {
                qDebug() << "Bug at data-sending cursor != size:" << cursor << "!=" << size;
                qDebug() << "raw write control bug:" << QString(QByteArray(data,size).toHex());
                abort();
            }
            protocolParsingCheck->valid=false;
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

void ProtocolParsingInputOutput::parseIncommingData()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(ProtocolParsingInputOutput::parseIncommingDataCount>0)
    {
        qDebug() << "Multiple client ont same section";
        abort();
    }
    ProtocolParsingInputOutput::parseIncommingDataCount++;
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    #ifdef PROTOCOLPARSINGDEBUG
    messageParsingLayer(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                QString::number(isClient)+
                #endif
                QStringLiteral(" parseIncommingData(): socket->bytesAvailable(): %1, header_cut: %2").arg(socket->bytesAvailable()).arg(header_cut.size()));
    #endif
    #endif

    while(1)
    {
        qint32 size;
        quint32 cursor=0;
        if(!header_cut.isEmpty())
        {
            const unsigned int &size_to_get=CATCHCHALLENGER_COMMONBUFFERSIZE-header_cut.size();
            memcpy(ProtocolParsingInputOutput::commonBuffer,header_cut.constData(),header_cut.size());
            size=read(ProtocolParsingInputOutput::commonBuffer,size_to_get)+header_cut.size();
            if(size>0)
            {
                //QByteArray tempDataToDebug(ProtocolParsingInputOutput::commonBuffer+header_cut.size(),size-header_cut.size());
                //qDebug() << "with header cut" << header_cut << tempDataToDebug.toHex() << "and size" << size;
            }
            header_cut.clear();
        }
        else
        {
            size=read(ProtocolParsingInputOutput::commonBuffer,CATCHCHALLENGER_COMMONBUFFERSIZE);
            if(size>0)
            {
                //QByteArray tempDataToDebug(ProtocolParsingInputOutput::commonBuffer,size);
                //qDebug() << "without header cut" << tempDataToDebug.toHex() << "and size" << size;
            }
        }
        if(size<=0)
        {
            /*messageParsingLayer(
#ifndef CATCHCHALLENGERSERVERDROPIFCLENT
QString::number(isClient)+
#endif
QStringLiteral(" parseIncommingData(): size returned is 0!"));*/
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            ProtocolParsingInputOutput::parseIncommingDataCount--;
            #endif
            return;
        }

        do
        {
            if(!parseIncommingDataRaw(ProtocolParsingInputOutput::commonBuffer,size,cursor))
                break;
        } while(cursor<(quint32)size);

        if(size<CATCHCHALLENGER_COMMONBUFFERSIZE)
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            ProtocolParsingInputOutput::parseIncommingDataCount--;
            #endif
            return;
        }
    }
    #ifdef PROTOCOLPARSINGDEBUG
    messageParsingLayer(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                QString::number(isClient)+
                #endif
    QStringLiteral(" parseIncommingData(): finish parse the input"));
    #endif
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    ProtocolParsingInputOutput::parseIncommingDataCount--;
    #endif
}

bool ProtocolParsingBase::parseIncommingDataRaw(const char *commonBuffer, const quint32 &size,quint32 &cursor)
{
    if(!parseHeader(commonBuffer,size,cursor))
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        //qDebug() << "Break due to need more in header";
        #endif
        return false;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(cursor==0 && !haveData)
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
    if(cursor==0 && !haveData)
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
    if(cursor==0 && !haveData)
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

bool ProtocolParsingBase::parseHeader(const char *commonBuffer,const quint32 &size,quint32 &cursor)
{
    if(!haveData)
    {
        if((size-cursor)<sizeof(quint8))//ignore because first int is cuted!
            return false;
        mainCodeType=*(commonBuffer+cursor);
        cursor+=sizeof(quint8);
        //def query without the sub code
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        if(isClient)
        {
            if(!mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType) && !toDebugValidMainCodeServerToClient.contains(mainCodeType) && !sizeMultipleCodePacketServerToClient.contains(mainCodeType))
            {
                //errorParsingLayer("Critical bug, mainCodeType not valid");//flood when web browser try connect on it
                return false;
            }
        }
        else
        #endif
        {
            if(!mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType) && !toDebugValidMainCodeClientToServer.contains(mainCodeType) && !sizeMultipleCodePacketClientToServer.contains(mainCodeType))
            {
                //errorParsingLayer("Critical bug, mainCodeType not valid");//flood when web browser try connect on it
                return false;
            }
        }
        if(cursor==0)
        {
            qDebug() << "Critical bug";
            abort();
        }
        #endif
        #ifdef PROTOCOLPARSINGDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    QString::number(isClient)+
                    #endif
        QStringLiteral(" parseIncommingData(): !haveData, mainCodeType: %1").arg(mainCodeType));
        #endif
        haveData=true;
        haveData_dataSize=false;
        have_subCodeType=false;
        have_query_number=false;
        is_reply=false;
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        if(isClient)
            need_subCodeType=!mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType);
        else
        #endif
            need_subCodeType=!mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType);
        need_query_number=false;
        /// \todo remplace by char*
        data_size_size=0;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(cursor==0 && !haveData)
    {
        qDebug() << "Critical bug";
        abort();
    }
    #endif

    if(!have_subCodeType)
    {
        #ifdef PROTOCOLPARSINGDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    QString::number(isClient)+
                    #endif
        QStringLiteral(" parseIncommingData(): !have_subCodeType"));
        #endif
        if(!need_subCodeType)
        {
            #ifdef PROTOCOLPARSINGDEBUG
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" parseIncommingData(): !need_subCodeType"));
            #endif
            //if is a reply
            if(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    (isClient && mainCodeType==replyCodeServerToClient) || (!isClient && mainCodeType==replyCodeClientToServer)
                    #else
                    mainCodeType==replyCodeClientToServer
                    #endif
            )
            {
                #ifdef PROTOCOLPARSINGDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" parseIncommingData(): need_query_number=true"));
                #endif
                is_reply=true;
                need_query_number=true;
                //the size with be resolved later
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(cursor==0 && !haveData)
                {
                    qDebug() << "Critical bug";
                    abort();
                }
                #endif
            }
            else
            {
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                if(isClient)
                {
                    //if is query with reply
                    if(mainCode_IsQueryServerToClient.contains(mainCodeType))
                    {
                        #ifdef PROTOCOLPARSINGDEBUG
                        messageParsingLayer(
                                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                    QString::number(isClient)+
                                    #endif
                        QStringLiteral(" parseIncommingData(): need_query_number=true, query with reply"));
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
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(cursor==0 && !haveData)
                    {
                        qDebug() << "Critical bug";
                        abort();
                    }
                    #endif
                }
                else
                #endif
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
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(cursor==0 && !haveData)
                    {
                        qDebug() << "Critical bug";
                        abort();
                    }
                    #endif
                }
            }
        }
        else
        {
            #ifdef PROTOCOLPARSINGDEBUG
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" parseIncommingData(): need_subCodeType"));
            #endif
            if((size-cursor)<sizeof(quint16))//ignore because first int is cuted!
            {
                if((size-cursor)>0)
                    header_cut.append(commonBuffer+cursor,(size-cursor));
                return false;
            }
            subCodeType=be16toh(*reinterpret_cast<const quint16 *>(commonBuffer+cursor));
            cursor+=sizeof(quint16);
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(cursor==0 && !haveData)
            {
                qDebug() << "Critical bug";
                abort();
            }
            #endif

            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            if(isClient)
            {
                #ifdef PROTOCOLPARSINGDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" parseIncommingData(): isClient"));
                #endif
                //if is query with reply
                if(mainCode_IsQueryServerToClient.contains(mainCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    messageParsingLayer(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" parseIncommingData(): need_query_number=true, query with reply (mainCode_IsQueryClientToServer)"));
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
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(cursor==0 && !haveData)
                {
                    qDebug() << "Critical bug";
                    abort();
                }
                #endif
            }
            else
            #endif
            {
                #ifdef PROTOCOLPARSINGDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" parseIncommingData(): !isClient"));
                #endif
                //if is query with reply
                if(mainCode_IsQueryClientToServer.contains(mainCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    messageParsingLayer(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" parseIncommingData(): need_query_number=true, query with reply (mainCode_IsQueryServerToClient)"));
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
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(cursor==0 && !haveData)
                {
                    qDebug() << "Critical bug";
                    abort();
                }
                #endif
            }
        }

        //set this parsing step is done
        have_subCodeType=true;
    }
    #ifdef PROTOCOLPARSINGDEBUG
    else
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    QString::number(isClient)+
                    #endif
        QStringLiteral(" parseIncommingData(): have_subCodeType"));
    #endif
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(cursor==0 && !haveData)
    {
        qDebug() << "Critical bug";
        abort();
    }
    #endif
    return true;
}

bool ProtocolParsingBase::parseQueryNumber(const char *commonBuffer,const quint32 &size,quint32 &cursor)
{
    if(!have_query_number && need_query_number)
    {
        #ifdef PROTOCOLPARSINGDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    QString::number(isClient)+
                    #endif
        QStringLiteral(" parseIncommingData(): need_query_number"));
        #endif
        if((size-cursor)<sizeof(quint8))
        {
            //todo, write message: need more bytes
            return false;
        }
        queryNumber=*(commonBuffer+cursor);
        cursor+=sizeof(quint8);

        // it's reply
        if(is_reply)
        {
            if(waitedReply_mainCodeType.contains(queryNumber))
            {
                mainCodeType=waitedReply_mainCodeType.value(queryNumber);
                if(!waitedReply_subCodeType.contains(queryNumber))
                {
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    if(isClient)
                    {
                        if(replySizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
                        {
                            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(replyComressionOnlyMainCodePacketServerToClient.contains(mainCodeType))
                            {
                                errorParsingLayer("Can't be compressed and fixed size");
                                return false;
                            }
                            #endif
                            #endif
                            dataSize=replySizeOnlyMainCodePacketServerToClient.value(mainCodeType);
                            haveData_dataSize=true;
                        }
                    }
                    else
                    #endif
                    {
                        if(replySizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
                        {
                            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(replyComressionOnlyMainCodePacketClientToServer.contains(mainCodeType))
                            {
                                errorParsingLayer("Can't be compressed and fixed size");
                                return false;
                            }
                            #endif
                            #endif
                            dataSize=replySizeOnlyMainCodePacketClientToServer.value(mainCodeType);
                            haveData_dataSize=true;
                        }
                    }
                }
                else
                {
                    subCodeType=waitedReply_subCodeType.value(queryNumber);
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    if(isClient)
                    {
                        if(replySizeMultipleCodePacketServerToClient.contains(mainCodeType))
                        {
                            if(replySizeMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                            {
                                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                                if(replyComressionMultipleCodePacketServerToClient.contains(mainCodeType))
                                {
                                    errorParsingLayer("Can't be compressed and fixed size");
                                    return false;
                                }
                                #endif
                                #endif
                                dataSize=replySizeMultipleCodePacketServerToClient.value(mainCodeType).value(subCodeType);
                                haveData_dataSize=true;
                            }
                        }
                    }
                    else
                    #endif
                    {
                        if(replySizeMultipleCodePacketClientToServer.contains(mainCodeType))
                        {
                            if(replySizeMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                            {
                                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                                if(replyComressionMultipleCodePacketClientToServer.contains(mainCodeType))
                                {
                                    errorParsingLayer("Can't be compressed and fixed size");
                                    return false;
                                }
                                #endif
                                #endif
                                dataSize=replySizeMultipleCodePacketClientToServer.value(mainCodeType).value(subCodeType);
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
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    QString::number(isClient)+
                    #endif
        QStringLiteral(" parseIncommingData(): not need_query_number"));
    #endif
    return true;
}

bool ProtocolParsingBase::parseDataSize(const char *commonBuffer, const quint32 &size,quint32 &cursor)
{
    if(!haveData_dataSize)
    {
        #ifdef PROTOCOLPARSINGDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    QString::number(isClient)+
                    #endif
        QStringLiteral(" parseIncommingData(): !haveData_dataSize"));
        #endif
        //temp data
        quint8 temp_size_8Bits;
        quint16 temp_size_16Bits;
        quint32 temp_size_32Bits;
        while(!haveData_dataSize)
        {
            switch(data_size_size)
            {
                case 0:
                {
                    if((size-cursor)<sizeof(quint8))
                        return false;
                    temp_size_8Bits=*(commonBuffer+cursor);
                    data_size_size+=sizeof(quint8);
                    cursor+=sizeof(quint8);
                    if(temp_size_8Bits!=0x00)
                    {
                        dataSize=temp_size_8Bits;
                        haveData_dataSize=true;
                        #ifdef PROTOCOLPARSINGDEBUG
                        messageParsingLayer(
                                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                    QString::number(isClient)+
                                    #endif
                        QStringLiteral(" parseIncommingData(): have 8Bits data size"));
                        #endif
                    }
                    else
                    {
                        #ifdef PROTOCOLPARSINGDEBUG
                        messageParsingLayer(
                                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                    QString::number(isClient)+
                                    #endif
                        QStringLiteral(" parseIncommingData(): have not 8Bits data size: %1 (%2), temp_size_8Bits: %3").arg(dataSize).arg(data_size_size).arg(temp_size_8Bits));
                        #endif
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        if(data_size_size==0)
                        {
                            messageParsingLayer(
                                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                        QString::number(isClient)+
                                        #endif
                            QStringLiteral(" parseIncommingData(): internal infinity packet read prevent"));
                            return false;
                        }
                        #endif
                    }
                }
                break;
                case sizeof(quint8):
                {
                    if((size-cursor)<sizeof(quint16))
                    {
                        if((size-cursor)>0)
                            header_cut.append(commonBuffer+cursor,(size-cursor));
                        return false;
                    }
                    temp_size_8Bits=*(commonBuffer+cursor);
                    if(temp_size_8Bits!=0x00)
                    {
                        temp_size_16Bits=be16toh(*(reinterpret_cast<const quint16 *>(commonBuffer+cursor)));
                        cursor+=sizeof(quint16);

                        dataSize=temp_size_16Bits;
                        haveData_dataSize=true;
                        #ifdef PROTOCOLPARSINGDEBUG
                        messageParsingLayer(
                                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                    QString::number(isClient)+
                                    #endif
                        QStringLiteral(" parseIncommingData(): have 16Bits data size: %1, temp_size_16Bits: %2").arg(dataSize).arg(data_size_size));
                        #endif
                    }
                    else
                    {
                        data_size_size+=sizeof(quint8);
                        cursor+=sizeof(quint8);//2x 0x00 when 32Bits size

                        #ifdef PROTOCOLPARSINGDEBUG
                        messageParsingLayer(
                                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                    QString::number(isClient)+
                                    #endif
                        QStringLiteral(" parseIncommingData(): have not 16Bits data size"));
                        #endif
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        if(data_size_size==sizeof(quint8))
                        {
                            messageParsingLayer(
                                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                        QString::number(isClient)+
                                        #endif
                            QStringLiteral(" parseIncommingData(): internal infinity packet read prevent"));
                            header_cut.append(commonBuffer+cursor,(size-cursor));
                            return false;
                        }
                        #endif
                    }
                }
                break;
                case sizeof(quint16):
                {
                    if((size-cursor)<sizeof(quint32))
                    {
                        if((size-cursor)>0)
                            header_cut.append(commonBuffer+cursor,(size-cursor));
                        return false;
                    }
                    temp_size_32Bits=be32toh(*reinterpret_cast<const quint32 *>(commonBuffer+cursor));
                    cursor+=sizeof(quint32);

                    if(temp_size_32Bits!=0x00000000)
                    {
                        dataSize=temp_size_32Bits;
                        haveData_dataSize=true;
                    }
                    else
                    {
                        errorParsingLayer("size is null");
                        if((size-cursor)>0)
                            header_cut.append(commonBuffer+cursor,(size-cursor));
                        return false;
                    }
                }
                break;
                default:
                errorParsingLayer(QStringLiteral("size not understand, internal bug: %1").arg(data_size_size));
                return false;
            }
        }
    }
    #ifdef PROTOCOLPARSINGDEBUG
    else
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    QString::number(isClient)+
                    #endif
        QStringLiteral(" parseIncommingData(): haveData_dataSize"));
    #endif
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(!haveData_dataSize)
    {
        errorParsingLayer("have not the size here!");
        return false;
    }
    #endif
    #ifdef PROTOCOLPARSINGDEBUG
    messageParsingLayer(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                QString::number(isClient)+
                #endif
    QStringLiteral(" parseIncommingData(): header informations is ready, dataSize: %1").arg(dataSize));
    #endif
    if(dataSize>16*1024*1024)
    {
        errorParsingLayer("packet size too big");
        return false;
    }
    return true;
}

bool ProtocolParsingBase::parseData(const char *commonBuffer, const quint32 &size,quint32 &cursor)
{
    if(dataSize==0)
        return parseDispatch(NULL,0);
    if(dataToWithoutHeader.isEmpty())
    {
        //if have too many data, or just the size
        if(dataSize<=(size-cursor))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(cursor==0 && !haveData)
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
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" parseIncommingData(): remaining data: %1").arg((size-cursor)));
            #endif
            return returnVal;
        }
    }
    //if have too many data, or just the size
    if((dataSize-dataToWithoutHeader.size())<=(size-cursor))
    {
        const quint32 &size_to_append=dataSize-dataToWithoutHeader.size();
        dataToWithoutHeader.append(commonBuffer+cursor,size_to_append);
        cursor+=size_to_append;
        #ifdef PROTOCOLPARSINGDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    QString::number(isClient)+
                    #endif
        QStringLiteral(" parseIncommingData(): remaining data: %1, buffer data: %2").arg((size-cursor)).arg(QString(QByteArray(commonBuffer,sizeof(commonBuffer)).toHex())));
        #endif
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(dataSize!=(quint32)dataToWithoutHeader.size())
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
                    QString::number(isClient)+
                    #endif
        QStringLiteral(" parseIncommingData(): need more to recompose: %1").arg(dataSize-dataToWithoutHeader.size()));
        #endif
        cursor=size;
        return false;
    }
}

bool ProtocolParsingBase::parseDispatch(const char *data, const int &size)
{
    #ifdef ProtocolParsingInputOutputDEBUG
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    QString::number(isClient)+
                    #endif
        QStringLiteral(" parseIncommingData(): parse message as client"));
    else
    #else
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    QString::number(isClient)+
                    #endif
        QStringLiteral(" parseIncommingData(): parse message as server"));
    #endif
    #endif
    #ifdef ProtocolParsingInputOutputDEBUG
    messageParsingLayer(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                QString::number(isClient)+
                #endif
    QStringLiteral(" parseIncommingData(): data: %1").arg(QString(data.toHex())));
    #endif
    //message
    if(!need_query_number)
    {
        if(!need_subCodeType)
        {
            #ifdef ProtocolParsingInputOutputDEBUG
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" parseIncommingData(): !need_query_number && !need_subCodeType, mainCodeType: %1").arg(mainCodeType));
            #endif
            parseMessage(mainCodeType,data,size);
        }
        else
        {
            #ifdef ProtocolParsingInputOutputDEBUG
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" parseIncommingData(): !need_query_number && need_subCodeType, mainCodeType: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
            #endif
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            if(isClient)
            {
                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                    if(compressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                    {
                        switch(compressionType)
                        {
                            case CompressionType_Xz:
                            {
                                const QByteArray &newData=lzmaUncompress(QByteArray(data,size));
                                if(newData.isEmpty())
                                {
                                    errorParsingLayer("Compressed data corrupted");
                                    return false;
                                }
                                parseFullMessage(mainCodeType,subCodeType,newData.constData(),newData.size());
                            }
                            break;
                            case CompressionType_Zlib:
                            default:
                            {
                                const QByteArray &newData=qUncompress(QByteArray(data,size));
                                if(newData.isEmpty())
                                {
                                    errorParsingLayer("Compressed data corrupted");
                                    return false;
                                }
                                parseFullMessage(mainCodeType,subCodeType,newData.constData(),newData.size());
                            }
                            break;
                            case CompressionType_None:
                                parseFullMessage(mainCodeType,subCodeType,data,size);
                            break;
                        }
                        return true;
                    }
                #endif
                parseFullMessage(mainCodeType,subCodeType,data,size);
                return true;
            }
            else
            #endif
            {
                #ifdef CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION
                    parseFullMessage(mainCodeType,subCodeType,data,size);
                #else
                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                    if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                    {
                        switch(compressionType)
                        {
                            case CompressionType_Xz:
                            {
                                const QByteArray &newData=lzmaUncompress(QByteArray(data,size));
                                if(newData.isEmpty())
                                {
                                    errorParsingLayer("Compressed data corrupted");
                                    return false;
                                }
                                parseFullMessage(mainCodeType,subCodeType,newData.constData(),newData.size());
                            }
                            break;
                            case CompressionType_Zlib:
                            default:
                            {
                                const QByteArray &newData=qUncompress(QByteArray(data,size));
                                if(newData.isEmpty())
                                {
                                    errorParsingLayer("Compressed data corrupted");
                                    return false;
                                }
                                parseFullMessage(mainCodeType,subCodeType,newData.constData(),newData.size());
                            }
                            break;
                            case CompressionType_None:
                                parseFullMessage(mainCodeType,subCodeType,data,size);
                            break;
                        }
                        return true;
                    }
                #endif
                parseFullMessage(mainCodeType,subCodeType,data,size);
                return true;
                #endif
            }
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
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" parseIncommingData(): need_query_number && !is_reply, mainCodeType: %1").arg(mainCodeType));
                #endif
                storeInputQuery(mainCodeType,queryNumber);
                parseQuery(mainCodeType,queryNumber,data,size);
            }
            else
            {
                #ifdef ProtocolParsingInputOutputDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" parseIncommingData(): need_query_number && !is_reply, mainCodeType: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
                #endif
                storeFullInputQuery(mainCodeType,subCodeType,queryNumber);
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                if(isClient)
                {
                    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                    if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                        if(compressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                        {
                            switch(compressionType)
                            {
                                case CompressionType_Xz:
                                {
                                    const QByteArray &newData=lzmaUncompress(QByteArray(data,size));
                                    if(newData.isEmpty())
                                    {
                                        errorParsingLayer("Compressed data corrupted");
                                        return false;
                                    }
                                    parseFullQuery(mainCodeType,subCodeType,queryNumber,newData.constData(),newData.size());
                                }
                                break;
                                case CompressionType_Zlib:
                                default:
                                {
                                    const QByteArray &newData=qUncompress(QByteArray(data,size));
                                    if(newData.isEmpty())
                                    {
                                        errorParsingLayer("Compressed data corrupted");
                                        return false;
                                    }
                                    parseFullQuery(mainCodeType,subCodeType,queryNumber,newData.constData(),newData.size());
                                }
                                break;
                                case CompressionType_None:
                                    parseFullQuery(mainCodeType,subCodeType,queryNumber,data,size);
                                break;
                            }
                            return true;
                        }
                    #endif
                    parseFullQuery(mainCodeType,subCodeType,queryNumber,data,size);
                    return true;
                }
                else
                #endif
                {
                    #ifdef CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION
                        parseFullQuery(mainCodeType,subCodeType,queryNumber,data,size);
                    #else
                    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                    if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                        if(compressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                        {
                            switch(compressionType)
                            {
                                case CompressionType_Xz:
                                {
                                    const QByteArray &newData=lzmaUncompress(QByteArray(data,size));
                                    if(newData.isEmpty())
                                    {
                                        errorParsingLayer("Compressed data corrupted");
                                        return false;
                                    }
                                    parseFullQuery(mainCodeType,subCodeType,queryNumber,newData.constData(),newData.size());
                                }
                                break;
                                case CompressionType_Zlib:
                                default:
                                {
                                    const QByteArray &newData=qUncompress(QByteArray(data,size));
                                    if(newData.isEmpty())
                                    {
                                        errorParsingLayer("Compressed data corrupted");
                                        return false;
                                    }
                                    parseFullQuery(mainCodeType,subCodeType,queryNumber,newData.constData(),newData.size());
                                }
                                break;
                                case CompressionType_None:
                                    parseFullQuery(mainCodeType,subCodeType,queryNumber,data,size);
                                break;
                            }
                            return true;
                        }
                    #endif
                    parseFullQuery(mainCodeType,subCodeType,queryNumber,data,size);
                    return true;
                    #endif
                }
            }
        }
        else
        {
            //reply
            if(!waitedReply_subCodeType.contains(queryNumber))
            {
                waitedReply_mainCodeType.remove(queryNumber);
                #ifdef ProtocolParsingInputOutputDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" parseIncommingData(): need_query_number && is_reply && reply_subCodeType.contains(queryNumber), queryNumber: %1, mainCodeType: %2").arg(queryNumber).arg(mainCodeType));
                #endif
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                if(isClient)
                {
                    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                    if(replyComressionOnlyMainCodePacketServerToClient.contains(mainCodeType))
                    {
                        switch(compressionType)
                        {
                            case CompressionType_Xz:
                            {
                                const QByteArray &newData=lzmaUncompress(QByteArray(data,size));
                                if(newData.isEmpty())
                                {
                                    errorParsingLayer("Compressed data corrupted");
                                    return false;
                                }
                                parseReplyData(mainCodeType,queryNumber,newData.constData(),newData.size());
                            }
                            break;
                            case CompressionType_Zlib:
                            default:
                            {
                                const QByteArray &newData=qUncompress(QByteArray(data,size));
                                if(newData.isEmpty())
                                {
                                    errorParsingLayer("Compressed data corrupted");
                                    return false;
                                }
                                parseReplyData(mainCodeType,queryNumber,newData.constData(),newData.size());
                            }
                            break;
                            case CompressionType_None:
                                parseReplyData(mainCodeType,queryNumber,data,size);
                            break;
                        }
                        return true;
                    }
                    #endif
                    parseReplyData(mainCodeType,queryNumber,data,size);
                    return true;
                }
                else
                #endif
                {
                    #ifdef CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION
                        parseReplyData(mainCodeType,queryNumber,data,size);
                    #else
                    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                    if(replyComressionOnlyMainCodePacketClientToServer.contains(mainCodeType))
                    {
                        switch(compressionType)
                        {
                            case CompressionType_Xz:
                            {
                                const QByteArray &newData=lzmaUncompress(QByteArray(data,size));
                                if(newData.isEmpty())
                                {
                                    errorParsingLayer("Compressed data corrupted");
                                    return false;
                                }
                                parseReplyData(mainCodeType,queryNumber,newData.constData(),newData.size());
                            }
                            break;
                            case CompressionType_Zlib:
                            default:
                            {
                                const QByteArray &newData=qUncompress(QByteArray(data,size));
                                if(newData.isEmpty())
                                {
                                    errorParsingLayer("Compressed data corrupted");
                                    return false;
                                }
                                parseReplyData(mainCodeType,queryNumber,newData.constData(),newData.size());
                            }
                            break;
                            case CompressionType_None:
                                parseReplyData(mainCodeType,queryNumber,data,size);
                            break;
                        }
                        return true;
                    }
                    #endif
                    parseReplyData(mainCodeType,queryNumber,data,size);
                    return true;
                    #endif
                }
            }
            else
            {
                waitedReply_mainCodeType.remove(queryNumber);
                waitedReply_subCodeType.remove(queryNumber);
                #ifdef ProtocolParsingInputOutputDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" parseIncommingData(): need_query_number && is_reply && reply_subCodeType.contains(queryNumber), queryNumber: %1, mainCodeType: %2, subCodeType: %3").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
                #endif
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                if(isClient)
                {
                    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                    if(replyComressionMultipleCodePacketServerToClient.contains(mainCodeType))
                        if(replyComressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                        {
                            switch(compressionType)
                            {
                                case CompressionType_Xz:
                                {
                                    const QByteArray &newData=lzmaUncompress(QByteArray(data,size));
                                    if(newData.isEmpty())
                                    {
                                        errorParsingLayer("Compressed data is buggy");
                                        return false;
                                    }
                                    parseFullReplyData(mainCodeType,subCodeType,queryNumber,newData.constData(),newData.size());
                                }
                                break;
                                case CompressionType_Zlib:
                                default:
                                {
                                    const QByteArray &newData=qUncompress(QByteArray(data,size));
                                    if(newData.isEmpty())
                                    {
                                        errorParsingLayer("Compressed data is buggy");
                                        return false;
                                    }
                                    parseFullReplyData(mainCodeType,subCodeType,queryNumber,newData.constData(),newData.size());
                                }
                                break;
                                case CompressionType_None:
                                    parseFullReplyData(mainCodeType,subCodeType,queryNumber,data,size);
                                break;
                            }
                            return true;
                        }
                    #endif
                    parseFullReplyData(mainCodeType,subCodeType,queryNumber,data,size);
                    return true;
                }
                else
                #endif
                {
                    #ifdef CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION
                        parseFullReplyData(mainCodeType,subCodeType,queryNumber,data,size);
                    #else
                    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                    if(replyComressionMultipleCodePacketClientToServer.contains(mainCodeType))
                        if(replyComressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                        {
                            switch(compressionType)
                            {
                                case CompressionType_Xz:
                                {
                                    const QByteArray &newData=lzmaUncompress(QByteArray(data,size));
                                    if(newData.isEmpty())
                                    {
                                        errorParsingLayer("Compressed data is buggy");
                                        return false;
                                    }
                                    parseFullReplyData(mainCodeType,subCodeType,queryNumber,newData.constData(),newData.size());
                                }
                                break;
                                case CompressionType_Zlib:
                                default:
                                {
                                    const QByteArray &newData=qUncompress(QByteArray(data,size));
                                    if(newData.isEmpty())
                                    {
                                        errorParsingLayer("Compressed data is buggy");
                                        return false;
                                    }
                                    parseFullReplyData(mainCodeType,subCodeType,queryNumber,newData.constData(),newData.size());
                                }
                                break;
                                case CompressionType_None:
                                    parseFullReplyData(mainCodeType,subCodeType,queryNumber,data,size);
                                break;
                            }
                            return true;
                        }
                    #endif
                    parseFullReplyData(mainCodeType,subCodeType,queryNumber,data,size);
                    return true;
                    #endif
                }
            }
        }
    }
    return true;
}

void ProtocolParsingBase::dataClear()
{
    dataToWithoutHeader.clear();
    dataSize=0;
    haveData=false;
}

#ifndef EPOLLCATCHCHALLENGERSERVER
QStringList ProtocolParsingBase::getQueryRunningList()
{
    QStringList returnedList;
    QHashIterator<quint8,quint8> i(waitedReply_mainCodeType);
    while (i.hasNext()) {
        i.next();
        if(waitedReply_subCodeType.contains(i.key()))
            returnedList << QStringLiteral("%1 %2").arg(i.value()).arg(waitedReply_subCodeType.value(i.key()));
        else
            returnedList << QString::number(i.value());
    }
    return returnedList;
}
#endif

void ProtocolParsingBase::storeInputQuery(const quint8 &mainCodeType,const quint8 &queryNumber)
{
    Q_UNUSED(mainCodeType);
    Q_UNUSED(queryNumber);
}

void ProtocolParsingBase::storeFullInputQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber)
{
    Q_UNUSED(mainCodeType);
    Q_UNUSED(subCodeType);
    Q_UNUSED(queryNumber);
}

void ProtocolParsingInputOutput::storeInputQuery(const quint8 &mainCodeType,const quint8 &queryNumber)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    protocolParsingCheck->waitedReply_mainCodeType[queryNumber]=mainCodeType;
    if(queryReceived.contains(queryNumber))
    {
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    QString::number(isClient)+
                    #endif
        QStringLiteral(" storeInputQuery(%1,%2) query with same id previously say").arg(mainCodeType).arg(queryNumber));
        return;
    }
    queryReceived << queryNumber;
    #endif
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::storeInputQuery(): queryNumber: %1, mainCodeType: %2, try send without sub code, but not registred as is").arg(queryNumber).arg(mainCodeType));
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
                QStringLiteral(" storeInputQuery(%1,%2) compression can't be enabled with fixed size").arg(mainCodeType).arg(queryNumber));
            #endif
            #endif
            //register the size of the reply to send
            replyOutputSize[queryNumber]=replySizeOnlyMainCodePacketServerToClient.value(mainCodeType);
        }
        else
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(replyComressionOnlyMainCodePacketServerToClient.contains(mainCodeType))
            {
                #ifdef PROTOCOLPARSINGDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" storeInputQuery(%1,%2) compression enabled").arg(mainCodeType).arg(queryNumber));
                #endif
                //register the compression of the reply to send
                replyOutputCompression << queryNumber;
            }
            #endif
        }
    }
    else
    #endif
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::storeInputQuery(): queryNumber: %1, mainCodeType: %2, try send without sub code, but not registred as is").arg(queryNumber).arg(mainCodeType));
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
                QStringLiteral(" storeInputQuery(%1,%2) compression can't be enabled with fixed size").arg(mainCodeType).arg(queryNumber));
            #endif
            #endif
            //register the size of the reply to send
            replyOutputSize[queryNumber]=replySizeOnlyMainCodePacketClientToServer.value(mainCodeType);
        }
        else
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(replyComressionOnlyMainCodePacketClientToServer.contains(mainCodeType))
            {
                #ifdef PROTOCOLPARSINGDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" storeInputQuery(%1,%2) compression enabled").arg(mainCodeType).arg(queryNumber));
                #endif
                //register the compression of the reply to send
                replyOutputCompression << queryNumber;
            }
            #endif
        }
    }
}

void ProtocolParsingInputOutput::storeFullInputQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    protocolParsingCheck->waitedReply_mainCodeType[queryNumber]=mainCodeType;
    protocolParsingCheck->waitedReply_subCodeType[queryNumber]=subCodeType;
    if(queryReceived.contains(queryNumber))
    {
        errorParsingLayer(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            QString::number(isClient)+
            #endif
    QStringLiteral(" storeInputQuery(%1,%2,%3) query with same id previously say").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
        return;
    }
    queryReceived << queryNumber;
    #endif
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::storeInputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return;
        }
        #endif
        if(replySizeMultipleCodePacketClientToServer.contains(mainCodeType))
        {
            if(replySizeMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
            {
                #ifdef PROTOCOLPARSINGDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" storeInputQuery(%1,%2,%3) fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                #endif
                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(replyComressionMultipleCodePacketClientToServer.contains(mainCodeType))
                    if(replyComressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                        messageParsingLayer(
                                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                    QString::number(isClient)+
                                    #endif
                        QStringLiteral(" storeInputQuery(%1,%2,%3) compression can't be enabled with fixed size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                #endif
                #endif
                replyOutputSize[queryNumber]=replySizeMultipleCodePacketClientToServer.value(mainCodeType).value(subCodeType);
            }
            else
            {
                #ifdef PROTOCOLPARSINGDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" storeInputQuery(%1,%2,%3) 1) not fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                #endif
                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                if(replyComressionMultipleCodePacketClientToServer.contains(mainCodeType))
                    if(replyComressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                    {
                        #ifdef PROTOCOLPARSINGDEBUG
                        messageParsingLayer(
                                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                    QString::number(isClient)+
                                    #endif
                        QStringLiteral(" storeInputQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                        #endif
                        //register the compression of the reply to send
                        replyOutputCompression << queryNumber;
                    }
                #endif
            }
        }
        else
        {
            #ifdef PROTOCOLPARSINGDEBUG
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" storeInputQuery(%1,%2,%3) 1) not fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            #endif
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(replyComressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(replyComressionMultipleCodePacketClientToServer.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    messageParsingLayer(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" storeInputQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    //register the compression of the reply to send
                    replyOutputCompression << queryNumber;
                }
            #endif
        }
    }
    else
    #endif
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" ProtocolParsingInputOutput::storeInputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return;
        }
        #endif
        if(replySizeMultipleCodePacketServerToClient.contains(mainCodeType))
        {
            if(replySizeMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
            {
                #ifdef PROTOCOLPARSINGDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" storeInputQuery(%1,%2,%3) fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                #endif
                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(replyComressionMultipleCodePacketServerToClient.contains(mainCodeType))
                    if(replyComressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                        messageParsingLayer(
                                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                    QString::number(isClient)+
                                    #endif
                        QStringLiteral(" storeInputQuery(%1,%2,%3) compression can't be enabled with fixed size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                #endif
                #endif
                replyOutputSize[queryNumber]=replySizeMultipleCodePacketServerToClient.value(mainCodeType).value(subCodeType);
            }
            else
            {
                #ifdef PROTOCOLPARSINGDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            QString::number(isClient)+
                            #endif
                QStringLiteral(" storeInputQuery(%1,%2,%3) 1) not fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                #endif
                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                if(replyComressionMultipleCodePacketServerToClient.contains(mainCodeType))
                    if(replyComressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                    {
                        #ifdef PROTOCOLPARSINGDEBUG
                        messageParsingLayer(
                                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                    QString::number(isClient)+
                                    #endif
                        QStringLiteral(" storeInputQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                        #endif
                        //register the compression of the reply to send
                        replyOutputCompression << queryNumber;
                    }
                #endif
            }
        }
        else
        {
            #ifdef PROTOCOLPARSINGDEBUG
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        QString::number(isClient)+
                        #endif
            QStringLiteral(" storeInputQuery(%1,%2,%3) 1) not fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            #endif
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(replyComressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(replyComressionMultipleCodePacketServerToClient.value(mainCodeType).contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    messageParsingLayer(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                QString::number(isClient)+
                                #endif
                    QStringLiteral(" storeInputQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    //register the compression of the reply to send
                    replyOutputCompression << queryNumber;
               }
            #endif
        }
    }
}
